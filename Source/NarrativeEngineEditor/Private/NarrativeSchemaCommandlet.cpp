#include "NarrativeSchemaCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
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
		TSharedRef<FJsonObject> Gates = MakeShared<FJsonObject>();
		for (const TPair<FGameplayTag, float>& Gate : Sheet.Gates)
		{
			Gates->SetNumberField(Gate.Key.ToString(), Gate.Value);
		}
		Out->SetObjectField(TEXT("gates"), Gates);
		return Out;
	}

	/** TDD P1's first gate: the serialized values of every narrative asset, as data the oracle can read. */
	bool Export(const FString& Path, const TArray<UNarrativeBasisVector*>& Axes, const TArray<UArchetypeDef*>& Archetypes,
		const TArray<UNarrativeEntityDef*>& Entities, const TArray<UNarrativeDialogDef*>& Cards)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("export_version"), 1);

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
			Row->SetObjectField(TEXT("sheet"), ExportSheet(Entity->Mind));
			EntityRows.Add(MakeShared<FJsonValueObject>(Row));
		}
		Root->SetArrayField(TEXT("entities"), EntityRows);

		TArray<TSharedPtr<FJsonValue>> CardRows;
		for (const UNarrativeDialogDef* Card : Cards)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Card->GetName());
			Row->SetStringField(TEXT("path"), Card->GetPathName());
			Row->SetStringField(TEXT("phrase"), Card->Phrase.ToString());
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
		const bool bExported = Export(Path, Axes, Archetypes, Entities, Cards);
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

	TArray<FNarrativeLintIssue> Issues;
	for (const UNarrativeDialogDef* Card : Cards)
	{
		NarrativeLints::LintCard(*Card, Issues);
	}
	for (const UNarrativeEntityDef* Entity : Entities)
	{
		NarrativeLints::LintEntity(*Entity, Issues);
	}

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
