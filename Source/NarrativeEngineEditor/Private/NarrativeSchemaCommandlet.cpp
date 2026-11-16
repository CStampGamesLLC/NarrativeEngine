#include "NarrativeSchemaCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/ActorComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "FileHelpers.h"
#include "GameplayTagContainer.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "NarrativeCoreData.h"
#include "NarrativeLints.h"
#include "NarrativeStaticData.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogNarrativeSchema, Log, All);

namespace
{
	template <typename T>
	TArray<T*> LoadAll(IAssetRegistry& Registry)
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(T::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses*/ true);
		Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });

		TArray<T*> Loaded;
		for (const FAssetData& Asset : Assets)
		{
			if (T* Object = Cast<T>(Asset.GetAsset()))
			{
				Loaded.Add(Object);
			}
		}
		return Loaded;
	}

	template <typename T>
	T* FindByName(const TArray<T*>& Assets, const FString& Name)
	{
		T* const* Found = Assets.FindByPredicate([&Name](const T* Asset) { return Asset->GetName() == Name; });
		return Found ? *Found : nullptr;
	}

	/** Dialog/<...>/<Family>/DA_Card gets Dialog.Family.<Family>, when that tag is registered. */
	bool SeedFamily(UNarrativeDialogDef& Card, FString& OutReport)
	{
		const FString Folder = FPackageName::GetLongPackagePath(Card.GetOutermost()->GetName());
		const int32 DialogAt = Folder.Find(TEXT("/Dialog/"));
		if (DialogAt == INDEX_NONE)
		{
			return false;
		}

		const FString Leaf = FPaths::GetCleanFilename(Folder);
		const FGameplayTag Family = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Dialog.Family.%s"), *Leaf)), /*ErrorIfNotFound*/ false);
		if (!Family.IsValid())
		{
			OutReport += FString::Printf(TEXT("  %s: no Dialog.Family.%s tag; family left empty.\n"), *Card.GetName(), *Leaf);
			return false;
		}

		Card.Family = Family;
		OutReport += FString::Printf(TEXT("  %s: family %s\n"), *Card.GetName(), *Family.ToString());
		return true;
	}

	FString DescribeMask(const FNarrativeMask& Mask)
	{
		TArray<FString> Names;
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
		{
			Names.Add(Axis.ToSoftObjectPath().GetAssetName());
		}
		return FString::Join(Names, TEXT(", "));
	}

	bool ApplySheet(UNarrativeEntityDef& Entity, const TMap<FString, FString>& Params,
		const TArray<UNarrativeBasisVector*>& Axes, const TArray<UNarrativeDialogDef*>& Cards, FString& OutReport)
	{
		const FString* CalibrationArg = Params.Find(TEXT("Calibration"));
		const FString* ProfileArg = Params.Find(TEXT("Profile"));
		if (!CalibrationArg || !ProfileArg)
		{
			OutReport += TEXT("  -ApplySheet needs -Calibration=<json> and -Profile=<name>.\n");
			return false;
		}

		const FString Path = FPaths::IsRelative(*CalibrationArg) ? FPaths::Combine(FPaths::ProjectDir(), *CalibrationArg) : *CalibrationArg;
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root)
		{
			OutReport += FString::Printf(TEXT("  Could not read calibration %s.\n"), *Path);
			return false;
		}

		const TSharedPtr<FJsonObject>* Sheet = nullptr;
		const TSharedPtr<FJsonObject>* Profile = nullptr;
		if (!Root->TryGetObjectField(TEXT("sheet"), Sheet) || !(*Sheet)->TryGetObjectField(*ProfileArg, Profile))
		{
			OutReport += FString::Printf(TEXT("  %s has no sheet.%s.\n"), *Path, **ProfileArg);
			return false;
		}

		FNarrativeMindSheet& Mind = Entity.Mind;
		Mind.bHasMind = true;
		Mind.PositionWeight = (*Profile)->GetNumberField(TEXT("w_pos"));
		Mind.RegardWeight = (*Profile)->GetNumberField(TEXT("w_reg"));
		Mind.Bias = (*Profile)->GetNumberField(TEXT("bias"));
		Mind.HostileThreshold = (*Profile)->GetNumberField(TEXT("th"));
		Mind.FriendlyThreshold = (*Profile)->GetNumberField(TEXT("tf"));
		Mind.Hysteresis = (*Profile)->GetNumberField(TEXT("delta"));
		Mind.NoiseMemory = (*Profile)->GetNumberField(TEXT("tz"));
		Mind.DecisionInterval = 1.f;
		if (const FString* LoyaltyArg = Params.Find(TEXT("Loyalty")))
		{
			Mind.Loyalty = FCString::Atof(**LoyaltyArg);
		}

		double Theta = 0.0;
		if ((*Sheet)->TryGetNumberField(TEXT("theta_join"), Theta))
		{
			Mind.Gates.Add(FGameplayTag::RequestGameplayTag(TEXT("Gate.Join")), Theta);
		}
		if ((*Sheet)->TryGetNumberField(TEXT("theta_obey"), Theta))
		{
			Mind.Gates.Add(FGameplayTag::RequestGameplayTag(TEXT("Gate.Obey")), Theta);
		}
		OutReport += FString::Printf(TEXT("  %s: sheet.%s from %s\n"), *Entity.GetName(), **ProfileArg, **CalibrationArg);

		if (const FString* MaskArg = Params.Find(TEXT("Mask")))
		{
			TArray<FString> Names;
			MaskArg->ParseIntoArray(Names, TEXT(","));
			FNarrativeMask Mask;
			for (const FString& Name : Names)
			{
				UNarrativeBasisVector* Axis = FindByName(Axes, Name.TrimStartAndEnd());
				if (!Axis)
				{
					OutReport += FString::Printf(TEXT("  No basis vector named %s; mask left unchanged.\n"), *Name);
					return true;
				}
				Mask.Add(Axis);
			}
			Entity.Mask = Mask;
			OutReport += FString::Printf(TEXT("  %s: mask %s\n"), *Entity.GetName(), *DescribeMask(Mask));
		}

		if (const FString* TellArg = Params.Find(TEXT("TellCard")))
		{
			if (UNarrativeDialogDef* Tell = FindByName(Cards, *TellArg))
			{
				Mind.TellCard = Tell;
				OutReport += FString::Printf(TEXT("  %s: tell %s\n"), *Entity.GetName(), *Tell->GetName());
			}
			else
			{
				OutReport += FString::Printf(TEXT("  No dialog card named %s; tell left unchanged.\n"), **TellArg);
			}
		}
		return true;
	}
}

namespace
{
	/** An object reference as the asset's short name, the key every other section of the export uses. */
	FString ReferenceName(const FSoftObjectPath& Path)
	{
		return Path.IsNull() ? FString() : Path.GetAssetName();
	}

	TArray<TSharedPtr<FJsonValue>> ExportVector(const FVectorND& Vector)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (int32 Index = 0; Index < Vector.Num(); ++Index)
		{
			TSharedRef<FJsonObject> Coordinate = MakeShared<FJsonObject>();
			Coordinate->SetStringField(TEXT("axis"), Vector.GetBasis()[Index].ToSoftObjectPath().GetAssetName());
			Coordinate->SetNumberField(TEXT("value"), Vector[Index]);
			Out.Add(MakeShared<FJsonValueObject>(Coordinate));
		}
		return Out;
	}

	TArray<TSharedPtr<FJsonValue>> ExportMask(const FNarrativeMask& Mask)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
		{
			Out.Add(MakeShared<FJsonValueString>(Axis.ToSoftObjectPath().GetAssetName()));
		}
		return Out;
	}

	TSharedRef<FJsonObject> ExportSheet(const FNarrativeMindSheet& Sheet)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetBoolField(TEXT("has_mind"), Sheet.bHasMind);
		Out->SetNumberField(TEXT("pi"), Sheet.PriorSelfWeight);
		Out->SetNumberField(TEXT("s_prior"), Sheet.PriorSpread);
		Out->SetNumberField(TEXT("w_pos"), Sheet.PositionWeight);
		Out->SetNumberField(TEXT("w_reg"), Sheet.RegardWeight);
		Out->SetNumberField(TEXT("bias"), Sheet.Bias);
		Out->SetNumberField(TEXT("th"), Sheet.HostileThreshold);
		Out->SetNumberField(TEXT("tf"), Sheet.FriendlyThreshold);
		Out->SetNumberField(TEXT("delta"), Sheet.Hysteresis);
		Out->SetNumberField(TEXT("tz"), Sheet.NoiseMemory);
		Out->SetNumberField(TEXT("dt"), Sheet.DecisionInterval);
		Out->SetNumberField(TEXT("t_regard"), Sheet.RegardMemory);
		Out->SetNumberField(TEXT("loyalty"), Sheet.Loyalty);
		Out->SetNumberField(TEXT("tell_s"), Sheet.TellWarningDuration);
		Out->SetStringField(TEXT("tell_ability"), Sheet.TellAbilityTag.ToString());
		Out->SetStringField(TEXT("tell_card"), ReferenceName(Sheet.TellCard.ToSoftObjectPath()));
		TSharedRef<FJsonObject> Worldview = MakeShared<FJsonObject>();
		for (const TPair<TSoftObjectPtr<UArchetypeDef>, FVectorND>& Override : Sheet.WorldviewOverrides)
		{
			Worldview->SetArrayField(ReferenceName(Override.Key.ToSoftObjectPath()), ExportVector(Override.Value));
		}
		Out->SetObjectField(TEXT("worldview"), Worldview);
		TSharedRef<FJsonObject> Gates = MakeShared<FJsonObject>();
		for (const TPair<FGameplayTag, float>& Gate : Sheet.Gates)
		{
			Gates->SetNumberField(Gate.Key.ToString(), Gate.Value);
		}
		Out->SetObjectField(TEXT("gates"), Gates);
		return Out;
	}

	/** Does this property, or an element, key or value of it, reference a narrative data asset? */
	bool RefersToNarrative(const FProperty* Property)
	{
		if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property))
		{
			return Object->PropertyClass && Object->PropertyClass->IsChildOf(UNarrativeDataAsset::StaticClass());
		}
		if (const FArrayProperty* Array = CastField<FArrayProperty>(Property))
		{
			return RefersToNarrative(Array->Inner);
		}
		if (const FSetProperty* Set = CastField<FSetProperty>(Property))
		{
			return RefersToNarrative(Set->ElementProp);
		}
		if (const FMapProperty* Map = CastField<FMapProperty>(Property))
		{
			return RefersToNarrative(Map->KeyProp) || RefersToNarrative(Map->ValueProp);
		}
		return false;
	}

	/**
	 * One reflected value as JSON, for assets and components the plugin can't name (factions, game components).
	 * References become asset names, maps keyed by a reference become objects, and narrative vectors use ExportVector.
	 */
	TSharedPtr<FJsonValue> ExportValue(const FProperty* Property, const void* Value)
	{
		if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property))
		{
			FSoftObjectPath Path;
			if (const FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(Property))
			{
				Path = Soft->GetPropertyValue(Value).ToSoftObjectPath();
			}
			else if (const UObject* Target = Object->GetObjectPropertyValue(Value))
			{
				Path = FSoftObjectPath(Target);
			}
			if (Path.IsNull())
			{
				return MakeShared<FJsonValueNull>();
			}
			return MakeShared<FJsonValueString>(ReferenceName(Path));
		}
		if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property))
		{
			return MakeShared<FJsonValueBoolean>(Bool->GetPropertyValue(Value));
		}
		if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property))
		{
			const int64 Raw = Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value);
			return MakeShared<FJsonValueString>(Enum->GetEnum()->GetNameStringByValue(Raw));
		}
		if (const FByteProperty* Byte = CastField<FByteProperty>(Property); Byte && Byte->Enum)
		{
			return MakeShared<FJsonValueString>(Byte->Enum->GetNameStringByValue(Byte->GetPropertyValue(Value)));
		}
		if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Property))
		{
			return MakeShared<FJsonValueNumber>(Numeric->IsFloatingPoint() ? Numeric->GetFloatingPointPropertyValue(Value) : static_cast<double>(Numeric->GetSignedIntPropertyValue(Value)));
		}
		if (const FArrayProperty* Array = CastField<FArrayProperty>(Property))
		{
			TArray<TSharedPtr<FJsonValue>> Out;
			FScriptArrayHelper Helper(Array, Value);
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				Out.Add(ExportValue(Array->Inner, Helper.GetRawPtr(Index)));
			}
			return MakeShared<FJsonValueArray>(Out);
		}
		if (const FSetProperty* Set = CastField<FSetProperty>(Property))
		{
			TArray<TSharedPtr<FJsonValue>> Out;
			FScriptSetHelper Helper(Set, Value);
			for (FScriptSetHelper::FIterator It(Helper); It; ++It)
			{
				Out.Add(ExportValue(Set->ElementProp, Helper.GetElementPtr(It)));
			}
			return MakeShared<FJsonValueArray>(Out);
		}
		if (const FMapProperty* Map = CastField<FMapProperty>(Property))
		{
			FScriptMapHelper Helper(Map, Value);
			if (CastField<FObjectPropertyBase>(Map->KeyProp) || CastField<FStrProperty>(Map->KeyProp) || CastField<FNameProperty>(Map->KeyProp))
			{
				TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
				for (FScriptMapHelper::FIterator It(Helper); It; ++It)
				{
					const TSharedPtr<FJsonValue> KeyValue = ExportValue(Map->KeyProp, Helper.GetKeyPtr(It));
					Out->SetField(KeyValue->IsNull() ? FString(TEXT("None")) : KeyValue->AsString(), ExportValue(Map->ValueProp, Helper.GetValuePtr(It)));
				}
				return MakeShared<FJsonValueObject>(Out);
			}
			TArray<TSharedPtr<FJsonValue>> Out;
			for (FScriptMapHelper::FIterator It(Helper); It; ++It)
			{
				TSharedRef<FJsonObject> Pair = MakeShared<FJsonObject>();
				Pair->SetField(TEXT("key"), ExportValue(Map->KeyProp, Helper.GetKeyPtr(It)));
				Pair->SetField(TEXT("value"), ExportValue(Map->ValueProp, Helper.GetValuePtr(It)));
				Out.Add(MakeShared<FJsonValueObject>(Pair));
			}
			return MakeShared<FJsonValueArray>(Out);
		}
		if (const FStructProperty* Struct = CastField<FStructProperty>(Property))
		{
			if (Struct->Struct == FVectorND::StaticStruct())
			{
				return MakeShared<FJsonValueArray>(ExportVector(*static_cast<const FVectorND*>(Value)));
			}
			if (Struct->Struct == FGameplayTag::StaticStruct())
			{
				return MakeShared<FJsonValueString>(static_cast<const FGameplayTag*>(Value)->ToString());
			}
			if (Struct->Struct == FGameplayTagContainer::StaticStruct())
			{
				TArray<TSharedPtr<FJsonValue>> Out;
				for (const FGameplayTag& Tag : *static_cast<const FGameplayTagContainer*>(Value))
				{
					Out.Add(MakeShared<FJsonValueString>(Tag.ToString()));
				}
				return MakeShared<FJsonValueArray>(Out);
			}
			if (Struct->Struct == FNarrativeMask::StaticStruct())
			{
				return MakeShared<FJsonValueArray>(ExportMask(*static_cast<const FNarrativeMask*>(Value)));
			}
		}
		// Text, strings, names, colours and anything else: the property's own text form.
		FString Text;
		Property->ExportTextItem_Direct(Text, Value, nullptr, nullptr, PPF_None);
		return MakeShared<FJsonValueString>(Text);
	}

	/** Every editable property an object declares below StopAt (exclusive), by reflection. */
	TSharedRef<FJsonObject> ExportProperties(const UObject& Object, const UClass* StopAt = nullptr)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Object.GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient)
				|| (StopAt && Property->GetOwnerClass() && StopAt->IsChildOf(Property->GetOwnerClass())))
			{
				continue;
			}
			Out->SetField(Property->GetName(), ExportValue(Property, Property->ContainerPtrToValuePtr<void>(&Object)));
		}
		return Out;
	}

	/** Every narrative data asset of a class this module can't link against (e.g. /Script/Narrative.FactionDef). */
	TArray<UObject*> LoadAllByClassPath(IAssetRegistry& Registry, const FTopLevelAssetPath& ClassPath)
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(ClassPath, Assets, /*bSearchSubClasses*/ true);
		Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
		TArray<UObject*> Loaded;
		for (const FAssetData& Asset : Assets)
		{
			if (UObject* Object = Asset.GetAsset())
			{
				Loaded.Add(Object);
			}
		}
		return Loaded;
	}

	TArray<TSharedPtr<FJsonValue>> ExportReflectedAssets(const TArray<UObject*>& Objects)
	{
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (const UObject* Object : Objects)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Object->GetName());
			Row->SetStringField(TEXT("path"), Object->GetPathName());
			Row->SetStringField(TEXT("class"), Object->GetClass()->GetName());
			Row->SetObjectField(TEXT("properties"), ExportProperties(*Object, UNarrativeDataAsset::StaticClass()));
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		return Rows;
	}

	/**
	 * The cast: every actor Blueprint with a default component (inherited ones included) whose class
	 * can reference narrative data, i.e. who carries which entity and factions, and its parent chain.
	 * A component is listed even when its references are empty, so a missing assignment shows.
	 */
	TArray<TSharedPtr<FJsonValue>> ExportCast(IAssetRegistry& Registry)
	{
		TArray<FAssetData> Blueprints;
		Registry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Blueprints, /*bSearchSubClasses*/ true);
		Blueprints.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });

		TArray<TSharedPtr<FJsonValue>> Rows;
		for (const FAssetData& Asset : Blueprints)
		{
			if (!Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
			{
				continue;
			}
			// Characters here are actors, not pawns (ADBCharacter). Skip non-actors from the registry tag when
			// the native parent resolves without loading; otherwise load and decide.
			const FString NativeParent = FPackageName::ExportTextPathToObjectPath(Asset.GetTagValueRef<FString>(FBlueprintTags::NativeParentClassPath));
			const UClass* NativeClass = NativeParent.IsEmpty() ? nullptr : FindObject<UClass>(FTopLevelAssetPath(NativeParent));
			if (NativeClass && !NativeClass->IsChildOf(AActor::StaticClass()))
			{
				continue;
			}
			const UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
			UClass* Generated = Blueprint ? Blueprint->GeneratedClass.Get() : nullptr;
			if (!Generated || !Generated->IsChildOf(AActor::StaticClass()))
			{
				continue;
			}

			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Blueprint->GetName());
			Row->SetStringField(TEXT("path"), Blueprint->GetPathName());
			TArray<TSharedPtr<FJsonValue>> Chain;
			for (const UClass* Parent = Generated->GetSuperClass(); Parent; Parent = Parent->GetSuperClass())
			{
				Chain.Add(MakeShared<FJsonValueString>(Parent->GetName()));
				if (Parent->IsNative())
				{
					break;
				}
			}
			Row->SetArrayField(TEXT("parents"), Chain);

			TArray<const UActorComponent*> Components;
			AActor::GetActorClassDefaultComponents(Generated, UActorComponent::StaticClass(), Components);
			TArray<TSharedPtr<FJsonValue>> ComponentRows;
			for (const UActorComponent* Component : Components)
			{
				bool bNarrative = false;
				for (TFieldIterator<FProperty> It(Component->GetClass()); It && !bNarrative; ++It)
				{
					bNarrative = It->HasAnyPropertyFlags(CPF_Edit) && RefersToNarrative(*It);
				}
				if (!bNarrative)
				{
					continue;
				}
				TSharedRef<FJsonObject> ComponentRow = MakeShared<FJsonObject>();
				FString Name = Component->GetName();
				Name.RemoveFromEnd(UActorComponent::ComponentTemplateNameSuffix);
				ComponentRow->SetStringField(TEXT("name"), Name);
				ComponentRow->SetStringField(TEXT("class"), Component->GetClass()->GetName());
				ComponentRow->SetObjectField(TEXT("properties"), ExportProperties(*Component, UActorComponent::StaticClass()));
				ComponentRows.Add(MakeShared<FJsonValueObject>(ComponentRow));
			}
			if (ComponentRows.IsEmpty())
			{
				continue;
			}
			Row->SetArrayField(TEXT("components"), ComponentRows);
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		return Rows;
	}

	TSharedRef<FJsonObject> ExportLints(const TArray<FNarrativeLintIssue>& Issues)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		int32 Errors = 0;
		TMap<FString, int32> ByRule;
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (const FNarrativeLintIssue& Issue : Issues)
		{
			const bool bError = Issue.Severity == ENarrativeLintSeverity::Error;
			Errors += bError ? 1 : 0;
			ByRule.FindOrAdd(Issue.Rule + (bError ? TEXT(".error") : TEXT(".warning")))++;
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("rule"), Issue.Rule);
			Row->SetStringField(TEXT("severity"), bError ? TEXT("error") : TEXT("warning"));
			Row->SetStringField(TEXT("asset"), Issue.Asset);
			Row->SetStringField(TEXT("message"), Issue.Message);
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		ByRule.KeySort(TLess<FString>());
		TSharedRef<FJsonObject> Counts = MakeShared<FJsonObject>();
		for (const TPair<FString, int32>& Pair : ByRule)
		{
			Counts->SetNumberField(Pair.Key, Pair.Value);
		}
		Out->SetNumberField(TEXT("errors"), Errors);
		Out->SetNumberField(TEXT("warnings"), Issues.Num() - Errors);
		Out->SetObjectField(TEXT("by_rule"), Counts);
		Out->SetArrayField(TEXT("issues"), Rows);
		return Out;
	}

	TArray<FNarrativeLintIssue> RunLints(const TArray<UNarrativeDialogDef*>& Cards, const TArray<UNarrativeEntityDef*>& Entities)
	{
		TArray<FNarrativeLintIssue> Issues;
		for (const UNarrativeDialogDef* Card : Cards)
		{
			NarrativeLints::LintCard(*Card, Issues);
		}
		for (const UNarrativeEntityDef* Entity : Entities)
		{
			NarrativeLints::LintEntity(*Entity, Issues);
		}
		// The catalog lints (P0-N): L5 and L10 need every card at once.
		TArray<const UNarrativeDialogDef*> Catalog(Cards);
		NarrativeLints::LintCatalog(Catalog, Issues);
		return Issues;
	}

	/**
	 * TDD P0-A: the authored state of every narrative asset, as data the oracle (rick.py --authored) reads.
	 * Version 2 adds the factions, faction contexts, the cast (who carries which entity and factions),
	 * entity tell fields and subclass vectors, card labels, and the lints as authored.
	 * Output is ordered by package path and carries no timestamp, so a re-export diffs cleanly.
	 */
	bool Export(const FString& Path, IAssetRegistry& Registry, const TArray<UNarrativeBasisVector*>& Axes, const TArray<UArchetypeDef*>& Archetypes,
		const TArray<UNarrativeEntityDef*>& Entities, const TArray<UNarrativeDialogDef*>& Cards)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("export_version"), 2);

		TArray<TSharedPtr<FJsonValue>> AxisNames;
		for (const UNarrativeBasisVector* Axis : Axes)
		{
			AxisNames.Add(MakeShared<FJsonValueString>(Axis->GetName()));
		}
		Root->SetArrayField(TEXT("axes"), AxisNames);

		TArray<TSharedPtr<FJsonValue>> ArchetypeRows;
		for (const UArchetypeDef* Archetype : Archetypes)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Archetype->GetName());
			Row->SetStringField(TEXT("path"), Archetype->GetPathName());
			Row->SetArrayField(TEXT("location"), ExportVector(Archetype->Location));
			ArchetypeRows.Add(MakeShared<FJsonValueObject>(Row));
		}
		Root->SetArrayField(TEXT("archetypes"), ArchetypeRows);

		TArray<TSharedPtr<FJsonValue>> EntityRows;
		for (const UNarrativeEntityDef* Entity : Entities)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Entity->GetName());
			Row->SetStringField(TEXT("path"), Entity->GetPathName());
			Row->SetArrayField(TEXT("position"), ExportVector(Entity->StartingCoordinates));
			Row->SetArrayField(TEXT("mask"), ExportMask(Entity->Mask));
			Row->SetNumberField(TEXT("mass"), Entity->Mass);
			Row->SetStringField(TEXT("archetype"), Entity->ArchetypeAsset ? Entity->ArchetypeAsset->GetName() : FString());
			Row->SetStringField(TEXT("class"), Entity->GetClass()->GetName());
			Row->SetBoolField(TEXT("individual"), Entity->bIndividual);
			Row->SetObjectField(TEXT("sheet"), ExportSheet(Entity->Mind));
			// Vectors a subclass adds (an agent's Motive), and everything else it declares, by reflection.
			Row->SetObjectField(TEXT("subclass"), ExportProperties(*Entity, UNarrativeEntityDef::StaticClass()));
			EntityRows.Add(MakeShared<FJsonValueObject>(Row));
		}
		Root->SetArrayField(TEXT("entities"), EntityRows);

		TArray<TSharedPtr<FJsonValue>> CardRows;
		for (const UNarrativeDialogDef* Card : Cards)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Card->GetName());
			Row->SetStringField(TEXT("path"), Card->GetPathName());
			Row->SetStringField(TEXT("class"), Card->GetClass()->GetName());
			Row->SetStringField(TEXT("phrase"), Card->Phrase.ToString());
			Row->SetStringField(TEXT("label"), Card->Label.ToString());
			Row->SetArrayField(TEXT("location"), ExportVector(Card->Location));
			Row->SetArrayField(TEXT("mask"), ExportMask(Card->Mask));
			Row->SetNumberField(TEXT("sigma"), Card->SpaceSigma);
			Row->SetNumberField(TEXT("J"), Card->Strength);
			Row->SetNumberField(TEXT("h"), Card->Stance);
			Row->SetStringField(TEXT("family"), Card->Family.ToString());
			Row->SetStringField(TEXT("tier"), NarrativeLints::LexToString(NarrativeLints::TierForReach(Card->SpaceSigma)));
			TArray<TSharedPtr<FJsonValue>> Tags;
			for (const FGameplayTag& Tag : Card->AbilityTags)
			{
				Tags.Add(MakeShared<FJsonValueString>(Tag.ToString()));
			}
			Row->SetArrayField(TEXT("ability_tags"), Tags);
			CardRows.Add(MakeShared<FJsonValueObject>(Row));
		}
		Root->SetArrayField(TEXT("cards"), CardRows);

		// The game module's faction types, by class path: this editor module can't link against them.
		Root->SetArrayField(TEXT("factions"), ExportReflectedAssets(LoadAllByClassPath(Registry, FTopLevelAssetPath(TEXT("/Script/Narrative"), TEXT("FactionDef")))));
		Root->SetArrayField(TEXT("faction_contexts"), ExportReflectedAssets(LoadAllByClassPath(Registry, FTopLevelAssetPath(TEXT("/Script/Narrative"), TEXT("FactionContext")))));
		Root->SetArrayField(TEXT("cast"), ExportCast(Registry));
		Root->SetObjectField(TEXT("lints"), ExportLints(RunLints(Cards, Entities)));

		FString Text;
		const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text);
		return FJsonSerializer::Serialize(Root, Writer) && FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

UNarrativeSchemaCommandlet::UNarrativeSchemaCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UNarrativeSchemaCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> NamedParams;
	ParseCommandLine(*Params, Tokens, Switches, NamedParams);
	const bool bSave = Switches.Contains(TEXT("Save"));
	const bool bLintOnly = Switches.Contains(TEXT("LintOnly"));

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registry.SearchAllAssets(/*bSynchronousSearch*/ true);

	const TArray<UNarrativeBasisVector*> Axes = LoadAll<UNarrativeBasisVector>(Registry);
	const TArray<UNarrativeDialogDef*> Cards = LoadAll<UNarrativeDialogDef>(Registry);
	const TArray<UNarrativeEntityDef*> Entities = LoadAll<UNarrativeEntityDef>(Registry);
	const TArray<UArchetypeDef*> Archetypes = LoadAll<UArchetypeDef>(Registry);

	if (const FString* ExportArg = NamedParams.Find(TEXT("Export")))
	{
		// Export what is authored now, before any migration in this run touches it.
		const FString Path = FPaths::IsRelative(*ExportArg) ? FPaths::Combine(FPaths::ProjectDir(), *ExportArg) : *ExportArg;
		const bool bExported = Export(Path, Registry, Axes, Archetypes, Entities, Cards);
		UE_LOG(LogNarrativeSchema, Display, TEXT("%s %d entities, %d archetypes and %d cards to %s"), bExported ? TEXT("Exported") : TEXT("FAILED to export"),
			Entities.Num(), Archetypes.Num(), Cards.Num(), *Path);
		if (Switches.Contains(TEXT("ExportOnly")))
		{
			return bExported ? 0 : 1;
		}
	}

	TSet<UPackage*> Changed;
	FString Report;
	if (!bLintOnly)
	{
		Report += TEXT("Migration (masks seeded from typed axes, families from folders):\n");
		for (UNarrativeDialogDef* Card : Cards)
		{
			bool bChanged = false;
			if (Card->Mask.IsEmpty() && Card->Location.Num() > 0)
			{
				Card->Mask = FNarrativeMask::FromTypedAxes(Card->Location);
				Report += FString::Printf(TEXT("  %s: mask %s\n"), *Card->GetName(), *DescribeMask(Card->Mask));
				bChanged = true;
			}
			if (!Card->Family.IsValid())
			{
				bChanged |= SeedFamily(*Card, Report);
			}
			if (bChanged)
			{
				Changed.Add(Card->GetOutermost());
			}
		}
		for (UNarrativeEntityDef* Entity : Entities)
		{
			if (Entity->Mask.IsEmpty() && Entity->StartingCoordinates.Num() > 0)
			{
				Entity->Mask = FNarrativeMask::FromTypedAxes(Entity->StartingCoordinates);
				Report += FString::Printf(TEXT("  %s: mask %s\n"), *Entity->GetName(), *DescribeMask(Entity->Mask));
				Changed.Add(Entity->GetOutermost());
			}
		}

		if (const FString* SheetTarget = NamedParams.Find(TEXT("ApplySheet")))
		{
			UNarrativeEntityDef* Entity = FindByName(Entities, *SheetTarget);
			if (!Entity)
			{
				Report += FString::Printf(TEXT("  No entity def named %s.\n"), **SheetTarget);
			}
			else if (ApplySheet(*Entity, NamedParams, Axes, Cards, Report))
			{
				Changed.Add(Entity->GetOutermost());
			}
		}
	}

	const TArray<FNarrativeLintIssue> Issues = RunLints(Cards, Entities);

	int32 Errors = 0;
	for (const FNarrativeLintIssue& Issue : Issues)
	{
		Errors += Issue.Severity == ENarrativeLintSeverity::Error ? 1 : 0;
	}
	Report += FString::Printf(TEXT("Lints: %d cards, %d entities, %d errors, %d warnings%s\n"),
		Cards.Num(), Entities.Num(), Errors, Issues.Num() - Errors, bLintOnly || bSave ? TEXT("") : TEXT(" (as migrated, unsaved)"));
	for (const FNarrativeLintIssue& Issue : Issues)
	{
		Report += TEXT("  ") + Issue.ToString() + TEXT("\n");
	}

	if (bSave && !Changed.IsEmpty())
	{
		TArray<UPackage*> Packages;
		for (UPackage* Package : Changed)
		{
			Package->MarkPackageDirty();
			Packages.Add(Package);
		}
		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, /*bOnlyDirty*/ false);
		Report += FString::Printf(TEXT("%s %d packages.\n"), bSaved ? TEXT("Saved") : TEXT("FAILED to save"), Packages.Num());
	}
	else if (!Changed.IsEmpty())
	{
		Report += FString::Printf(TEXT("%d packages would change; pass -Save to write them.\n"), Changed.Num());
	}

	TArray<FString> Lines;
	Report.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);
	for (const FString& Line : Lines)
	{
		UE_LOG(LogNarrativeSchema, Display, TEXT("%s"), *Line);
	}

	return bLintOnly && Errors > 0 ? 1 : 0;
}
