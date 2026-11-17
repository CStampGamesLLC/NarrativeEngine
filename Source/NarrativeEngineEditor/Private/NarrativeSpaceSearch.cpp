#include "NarrativeSpaceSearch.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "GameplayTagContainer.h"
#include "Misc/PackageName.h"
#include "NarrativeMask.h"
#include "NarrativeVector.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "NarrativeSpaceSearch"

namespace NarrativeSpaceSearch
{
	/** How much a hit in each kind of field counts for. */
	namespace Weight
	{
		constexpr float Name = 10.f;
		constexpr float Label = 8.f;
		constexpr float Text = 5.f;
		constexpr float Tag = 4.f;
		constexpr float String = 3.f;
		constexpr float Type = 2.f;
		constexpr float Reference = 2.f;
		constexpr float Folder = 1.f;
	}

	/** How cleanly a term lands in a field, which scales that field's weight. */
	namespace Quality
	{
		constexpr float Whole = 1.f;
		constexpr float WordStart = 0.75f;
		constexpr float Anywhere = 0.4f;
	}

	/** Nesting is shallow in practice; this only stops a self-referencing subobject graph. */
	constexpr int32 MaxDepth = 8;

	void AddField(TArray<FNarrativeSearchField>& Out, const FText& Field, FString Text, const float FieldWeight)
	{
		Text.TrimStartAndEndInline();
		if (!Text.IsEmpty()) { Out.Add({Field, MoveTemp(Text), FieldWeight}); }
	}

	bool IsSearchable(const FProperty& Property)
	{
		return !Property.HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated);
	}

	void CollectObject(const UObject& Object, const UObject& Root, const FText* Field, TArray<FNarrativeSearchField>& Out, int32 Depth);

	/** Pulls every searchable string out of one property value, under the top-level field's name. */
	void CollectValue(const FProperty* Property, const void* Value, const UObject& Root, const FText& Field,
		TArray<FNarrativeSearchField>& Out, const int32 Depth)
	{
		if (!Property || !Value || Depth > MaxDepth) { return; }

		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			AddField(Out, Field, TextProperty->GetPropertyValue(Value).ToString(), Weight::Text);
		}
		else if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			AddField(Out, Field, StrProperty->GetPropertyValue(Value), Weight::String);
		}
		else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			const FName Name = NameProperty->GetPropertyValue(Value);
			if (!Name.IsNone()) { AddField(Out, Field, Name.ToString(), Weight::String); }
		}
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			const UScriptStruct* Struct = StructProperty->Struct;
			if (Struct == FGameplayTag::StaticStruct())
			{
				const FGameplayTag& Tag = *static_cast<const FGameplayTag*>(Value);
				if (Tag.IsValid()) { AddField(Out, Field, Tag.ToString(), Weight::Tag); }
			}
			else if (Struct == FGameplayTagContainer::StaticStruct())
			{
				for (const FGameplayTag& Tag : *static_cast<const FGameplayTagContainer*>(Value))
				{
					AddField(Out, Field, Tag.ToString(), Weight::Tag);
				}
			}
			// A vector's basis and a mask's axes are positions in the space, not descriptions of the
			// asset. Indexing them would match every asset placed on whatever axis was searched for.
			else if (Struct != FVectorND::StaticStruct() && Struct != FNarrativeMask::StaticStruct())
			{
				for (TFieldIterator<FProperty> It(Struct); It; ++It)
				{
					if (!IsSearchable(**It)) { continue; }
					for (int32 Index = 0; Index < It->ArrayDim; ++Index)
					{
						CollectValue(*It, It->ContainerPtrToValuePtr<void>(Value, Index), Root, Field, Out, Depth + 1);
					}
				}
			}
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Helper(ArrayProperty, Value);
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				CollectValue(ArrayProperty->Inner, Helper.GetRawPtr(Index), Root, Field, Out, Depth + 1);
			}
		}
		else if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			FScriptSetHelper Helper(SetProperty, Value);
			for (FScriptSetHelper::FIterator It(Helper); It; ++It)
			{
				CollectValue(SetProperty->ElementProp, Helper.GetElementPtr(It), Root, Field, Out, Depth + 1);
			}
		}
		else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			FScriptMapHelper Helper(MapProperty, Value);
			for (FScriptMapHelper::FIterator It(Helper); It; ++It)
			{
				CollectValue(MapProperty->KeyProp, Helper.GetKeyPtr(It), Root, Field, Out, Depth + 1);
				CollectValue(MapProperty->ValueProp, Helper.GetValuePtr(It), Root, Field, Out, Depth + 1);
			}
		}
		else if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
		{
			// By path, so indexing never loads what the asset only refers to.
			AddField(Out, Field, SoftProperty->GetPropertyValue(Value).ToSoftObjectPath().GetAssetName(), Weight::Reference);
		}
		else if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			const UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);
			if (!Object) { return; }
			// An instanced subobject is part of the asset, so what it says is searchable too. Anything
			// else is only a reference, and is found by its name.
			if (Object->IsIn(&Root)) { CollectObject(*Object, Root, &Field, Out, Depth + 1); }
			else { AddField(Out, Field, Object->GetName(), Weight::Reference); }
		}
	}

	/**
	 * Indexes an object's edited properties. The asset itself files each under its own display name;
	 * a subobject files everything under the field that holds it, which is where the user will look.
	 */
	void CollectObject(const UObject& Object, const UObject& Root, const FText* Field, TArray<FNarrativeSearchField>& Out, const int32 Depth)
	{
		for (TFieldIterator<FProperty> It(Object.GetClass()); It; ++It)
		{
			// Only what a designer authors. Bare UPROPERTY() fields are runtime plumbing.
			if (!IsSearchable(**It) || !It->HasAnyPropertyFlags(CPF_Edit)) { continue; }
			const FText ItField = Field ? *Field : It->GetDisplayNameText();
			for (int32 Index = 0; Index < It->ArrayDim; ++Index)
			{
				CollectValue(*It, It->ContainerPtrToValuePtr<void>(&Object, Index), Root, ItField, Out, Depth);
			}
		}
	}

	bool IsWordStart(const FString& Text, const int32 At)
	{
		if (At <= 0) { return true; }
		const TCHAR Previous = Text[At - 1];
		const TCHAR Here = Text[At];
		// Separators, tag dots and path slashes start words, and so does a camel-case hump: DA_BackOff.
		return !FChar::IsAlnum(Previous)
			|| (FChar::IsLower(Previous) && FChar::IsUpper(Here))
			|| (FChar::IsDigit(Previous) != FChar::IsDigit(Here));
	}

	/** Never indexed (see CollectValue): an edit confined to one of these changes no searchable text. */
	bool IsPlacementType(const FProperty* Property)
	{
		if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) { Property = Array->Inner; }
		const FStructProperty* Struct = CastField<FStructProperty>(Property);
		return Struct && (Struct->Struct == FVectorND::StaticStruct() || Struct->Struct == FNarrativeMask::StaticStruct());
	}
}

float NarrativeSpaceSearch::MatchQuality(const FString& Text, const FString& Term, int32& OutAt)
{
	if (Term.IsEmpty()) { return 0.f; }
	if (Text.Equals(Term, ESearchCase::IgnoreCase)) { OutAt = 0; return Quality::Whole; }
	float Best = 0.f;
	for (int32 At = Text.Find(Term, ESearchCase::IgnoreCase); At != INDEX_NONE;)
	{
		const float Here = IsWordStart(Text, At) ? Quality::WordStart : Quality::Anywhere;
		if (Here > Best) { Best = Here; OutAt = At; }
		if (Best >= Quality::WordStart) { break; }
		// FString::Find clamps a start past the end back onto the last character, so a one-letter term that
		// matched there would be found again for ever. Nothing can start where the term no longer fits: stop.
		const int32 Next = At + 1;
		if (Next + Term.Len() > Text.Len()) { break; }
		At = Text.Find(Term, ESearchCase::IgnoreCase, ESearchDir::FromStart, Next);
	}
	return Best;
}

bool NarrativeSpaceSearch::AffectsIndex(const FPropertyChangedEvent& Event)
{
	// MemberProperty is the property on the edited object that holds the change, however deep the edit went.
	const FProperty* Member = Event.MemberProperty ? Event.MemberProperty : Event.Property;
	return !Member || !IsPlacementType(Member);
}

FNarrativeSearchEntry NarrativeSpaceSearch::Index(const UNarrativeDataAsset& Asset)
{
	FNarrativeSearchEntry Entry;
	Entry.Asset = const_cast<UNarrativeDataAsset*>(&Asset);
	Entry.Name = Asset.GetName();
	Entry.Type = Asset.GetClass()->GetDisplayNameText();

	// Name has to stay first: Find tells a name hit from an explaining hit by it.
	AddField(Entry.Fields, LOCTEXT("NameField", "Name"), Entry.Name, Weight::Name);
	const FString Label = Asset.GetSpaceLabel().ToString();
	if (Label != Entry.Name) { AddField(Entry.Fields, LOCTEXT("LabelField", "Label"), Label, Weight::Label); }
	AddField(Entry.Fields, LOCTEXT("TypeField", "Type"), Entry.Type.ToString(), Weight::Type);
	AddField(Entry.Fields, LOCTEXT("FolderField", "Folder"), FPackageName::GetLongPackagePath(Asset.GetPackage()->GetName()), Weight::Folder);

	CollectObject(Asset, Asset, nullptr, Entry.Fields, 0);
	return Entry;
}

TArray<FNarrativeSearchResult> NarrativeSpaceSearch::Find(const TArray<FNarrativeSearchEntry>& Entries, const FString& Query, const int32 MaxResults)
{
	TArray<const FNarrativeSearchEntry*> Pointers;
	Pointers.Reserve(Entries.Num());
	for (const FNarrativeSearchEntry& Entry : Entries) { Pointers.Add(&Entry); }
	return Find(Pointers, Query, MaxResults);
}

TArray<FNarrativeSearchResult> NarrativeSpaceSearch::Find(TConstArrayView<const FNarrativeSearchEntry*> Entries, const FString& Query, const int32 MaxResults)
{
	TArray<FNarrativeSearchResult> Results;
	TArray<FString> Terms;
	Query.ParseIntoArrayWS(Terms);
	if (Terms.IsEmpty() || MaxResults <= 0) { return Results; }

	for (const FNarrativeSearchEntry* EntryPtr : Entries)
	{
		if (!EntryPtr || !EntryPtr->Asset.IsValid()) { continue; }
		const FNarrativeSearchEntry& Entry = *EntryPtr;

		float Score = 0.f;
		bool bAllMatched = true;
		// The strongest term whose best hit is not the name, which is what the row shows as the reason
		// it matched. A term the name already explains needs no other field to justify it.
		float ExplainScore = 0.f;
		int32 ExplainField = INDEX_NONE, ExplainAt = 0, ExplainTerm = 0;

		for (int32 TermIndex = 0; TermIndex < Terms.Num() && bAllMatched; ++TermIndex)
		{
			const FString& Term = Terms[TermIndex];
			float TermScore = 0.f;
			int32 TermField = INDEX_NONE, TermAt = 0;
			for (int32 FieldIndex = 0; FieldIndex < Entry.Fields.Num(); ++FieldIndex)
			{
				const FNarrativeSearchField& Field = Entry.Fields[FieldIndex];
				int32 At = 0;
				const float Hit = MatchQuality(Field.Text, Term, At) * Field.Weight;
				if (Hit > TermScore)
				{
					TermScore = Hit;
					TermField = FieldIndex;
					TermAt = At;
				}
			}
			bAllMatched = TermScore > 0.f;
			Score += TermScore;
			if (TermField > 0 && TermScore > ExplainScore)
			{
				ExplainScore = TermScore;
				ExplainField = TermField;
				ExplainAt = TermAt;
				ExplainTerm = TermIndex;
			}
		}
		if (!bAllMatched) { continue; }

		FNarrativeSearchResult& Result = Results.AddDefaulted_GetRef();
		Result.Asset = Entry.Asset;
		Result.Name = Entry.Name;
		Result.Type = Entry.Type;
		Result.Score = Score;
		if (ExplainField != INDEX_NONE)
		{
			const FNarrativeSearchField& Field = Entry.Fields[ExplainField];
			Result.Field = Field.Field;
			Result.Snippet = Snippet(Field.Text, ExplainAt, Terms[ExplainTerm].Len());
			Result.Term = Terms[ExplainTerm];
		}
	}

	Results.Sort([](const FNarrativeSearchResult& A, const FNarrativeSearchResult& B)
	{
		return A.Score != B.Score ? A.Score > B.Score : A.Name < B.Name;
	});
	if (Results.Num() > MaxResults) { Results.SetNum(MaxResults); }
	return Results;
}

FString NarrativeSpaceSearch::Snippet(const FString& Text, const int32 MatchStart, const int32 MatchLength, const int32 Context)
{
	// One line per row: line breaks become spaces, which keeps every index where it was.
	FString Flat = Text.Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" "));
	const int32 From = FMath::Clamp(MatchStart - Context, 0, Flat.Len());
	const int32 To = FMath::Clamp(MatchStart + MatchLength + Context, From, Flat.Len());
	FString Result = Flat.Mid(From, To - From);
	if (From > 0) { Result.InsertAt(0, TEXT("...")); }
	if (To < Flat.Len()) { Result += TEXT("..."); }
	return Result;
}

TArray<TSoftObjectPtr<UNarrativeBasisVector>> NarrativeSpaceSearch::RelevantAxes(const FVectorND& Value, const FNarrativeMask* Mask, const int32 Count)
{
	const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& Basis = Value.GetBasis();
	const TArray<float>& Coordinates = Value.GetCoordinates();

	TArray<int32> Order;
	for (int32 Index = 0; Index < Basis.Num(); ++Index)
	{
		if (!Basis[Index].IsNull()) { Order.Add(Index); }
	}
	auto Magnitude = [&Coordinates](const int32 Index)
	{
		return Coordinates.IsValidIndex(Index) && FMath::IsFinite(Coordinates[Index]) ? FMath::Abs(Coordinates[Index]) : 0.f;
	};
	Order.StableSort([&](const int32 A, const int32 B)
	{
		const bool bMaskA = Mask && Mask->Contains(Basis[A]);
		const bool bMaskB = Mask && Mask->Contains(Basis[B]);
		if (bMaskA != bMaskB) { return bMaskA; }
		return Magnitude(A) > Magnitude(B);
	});

	TArray<TSoftObjectPtr<UNarrativeBasisVector>> Result;
	for (const int32 Index : Order)
	{
		if (Result.Num() >= Count) { break; }
		Result.AddUnique(Basis[Index]);
	}
	return Result;
}

const FVectorND* NarrativeSpaceSearch::PlacementVector(const UNarrativeDataAsset& Asset, const FName Field, FName* OutField)
{
	// The same rules FNarrativeSpaceModel::ResolvePlacement plots by, so a vector found here is one it will draw.
	auto Resolve = [&Asset](const FName Name) -> const FVectorND*
	{
		const FStructProperty* Property = Name.IsNone() ? nullptr : FindFProperty<FStructProperty>(Asset.GetClass(), Name);
		if (!Property || Property->Struct != FVectorND::StaticStruct() || Property->ArrayDim != 1
			|| !Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_EditConst))
		{
			return nullptr;
		}
		const FVectorND* Vector = Property->ContainerPtrToValuePtr<FVectorND>(&Asset);
		return Vector->IsValid() ? Vector : nullptr;
	};

	FName Used = Field;
	const FVectorND* Vector = Resolve(Field);
	if (!Vector)
	{
		Used = Asset.GetSpacePlacementProperty();
		Vector = Resolve(Used);
	}
	if (OutField) { *OutField = Vector ? Used : NAME_None; }
	return Vector;
}

const FNarrativeMask* NarrativeSpaceSearch::SpaceMask(const UNarrativeDataAsset& Asset)
{
	const FName Name = Asset.GetSpaceMaskProperty();
	const FStructProperty* Property = Name.IsNone() ? nullptr : FindFProperty<FStructProperty>(Asset.GetClass(), Name);
	if (!Property || Property->Struct != FNarrativeMask::StaticStruct() || Property->ArrayDim != 1) { return nullptr; }
	return Property->ContainerPtrToValuePtr<FNarrativeMask>(&Asset);
}

FNarrativeSpaceSearchIndex::FNarrativeSpaceSearchIndex()
{
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registry.OnAssetAdded().AddRaw(this, &FNarrativeSpaceSearchIndex::AssetChanged);
	Registry.OnAssetRemoved().AddRaw(this, &FNarrativeSpaceSearchIndex::AssetChanged);
	Registry.OnAssetRenamed().AddRaw(this, &FNarrativeSpaceSearchIndex::AssetRenamed);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FNarrativeSpaceSearchIndex::PropertyChanged);
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FNarrativeSpaceSearchIndex::ObjectsReplaced);
}

FNarrativeSpaceSearchIndex::~FNarrativeSpaceSearchIndex()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	if (FAssetRegistryModule* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
	{
		Module->Get().OnAssetAdded().RemoveAll(this);
		Module->Get().OnAssetRemoved().RemoveAll(this);
		Module->Get().OnAssetRenamed().RemoveAll(this);
	}
}

void FNarrativeSpaceSearchIndex::PropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (!Object) { return; }
	const UNarrativeDataAsset* Asset = Cast<UNarrativeDataAsset>(Object);
	if (!Asset) { Asset = Object->GetTypedOuter<UNarrativeDataAsset>(); }
	// Undo and redo arrive here too, through PostEditUndo, naming no property: that reindexes the one asset.
	// Moving an asset in the space changes nothing searchable, so it queues nothing.
	if (Asset && NarrativeSpaceSearch::AffectsIndex(Event)) { Enqueue(FSoftObjectPath(Asset)); }
}

void FNarrativeSpaceSearchIndex::AssetChanged(const FAssetData& Asset)
{
	// The registry's list is re-read when the index next works, and only the difference is (re)indexed.
	if (Asset.IsInstanceOf(UNarrativeDataAsset::StaticClass())) { bRescan = true; }
}

void FNarrativeSpaceSearchIndex::AssetRenamed(const FAssetData& Asset, const FString& OldPath)
{
	if (!Asset.IsInstanceOf(UNarrativeDataAsset::StaticClass())) { return; }
	if (Entries.Remove(FSoftObjectPath(OldPath)) > 0) { bChangedSinceFind = true; }
	bRescan = true;
}

void FNarrativeSpaceSearchIndex::ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements)
{
	// Reinstancing can swap the objects entries point at, or the class they were reflected through.
	for (const TPair<UObject*, UObject*>& Pair : Replacements)
	{
		if ((Pair.Key && Pair.Key->IsA<UNarrativeDataAsset>()) || (Pair.Value && Pair.Value->IsA<UNarrativeDataAsset>()))
		{
			MarkStale();
			return;
		}
	}
}

void FNarrativeSpaceSearchIndex::MarkStale()
{
	bRescan = true;
	for (const TPair<FSoftObjectPath, FNarrativeSearchEntry>& Pair : Entries) { Enqueue(Pair.Key); }
}

void FNarrativeSpaceSearchIndex::Enqueue(const FSoftObjectPath& Path)
{
	bool bQueued = false;
	PendingSet.Add(Path, &bQueued);
	if (!bQueued) { Pending.Add(Path); }
}

TArray<FNarrativeSearchResult> FNarrativeSpaceSearchIndex::Find(const FString& Query, const int32 MaxResults, const double BudgetSeconds)
{
	Pump(BudgetSeconds);
	bChangedSinceFind = false;
	TArray<const FNarrativeSearchEntry*> Indexed;
	Indexed.Reserve(Entries.Num());
	for (const TPair<FSoftObjectPath, FNarrativeSearchEntry>& Pair : Entries) { Indexed.Add(&Pair.Value); }
	return NarrativeSpaceSearch::Find(Indexed, Query, MaxResults);
}

bool FNarrativeSpaceSearchIndex::Pump(const double BudgetSeconds)
{
	if (bRescan) { Rescan(); }
	if (Pending.IsEmpty()) { return false; }

	// At least one per call, so a slow load can't stall indexing for good; then as many as the budget allows.
	const double Start = FPlatformTime::Seconds();
	int32 Done = 0;
	while (Done < Pending.Num() && (Done == 0 || FPlatformTime::Seconds() - Start < BudgetSeconds))
	{
		PendingSet.Remove(Pending[Done]);
		Reindex(Pending[Done++]);
	}
	Pending.RemoveAt(0, Done);
	bChangedSinceFind = true;
	return true;
}

void FNarrativeSpaceSearchIndex::Rescan()
{
	bRescan = false;
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	FARFilter Filter;
	Filter.ClassPaths.Add(UNarrativeDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);

	// Every narrative asset, not only the query's: picking one outside it retargets the query to it.
	TSet<FSoftObjectPath> Present;
	Present.Reserve(Assets.Num());
	for (const FAssetData& Data : Assets)
	{
		const FSoftObjectPath Path = Data.GetSoftObjectPath();
		Present.Add(Path);
		if (!Entries.Contains(Path)) { Enqueue(Path); }
	}
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		if (!Present.Contains(It.Key()))
		{
			It.RemoveCurrent();
			bChangedSinceFind = true;
		}
	}
	Pending.RemoveAll([this, &Present](const FSoftObjectPath& Path)
	{
		const bool bGone = !Present.Contains(Path);
		if (bGone) { PendingSet.Remove(Path); }
		return bGone;
	});
}

void FNarrativeSpaceSearchIndex::Reindex(const FSoftObjectPath& Path)
{
	// The data catalog pins narrative assets at startup, so this is mostly a lookup, not a load; the budget bounds the rest.
	UObject* Object = Path.ResolveObject();
	if (!Object) { Object = Path.TryLoad(); }
	if (const UNarrativeDataAsset* Asset = Cast<UNarrativeDataAsset>(Object))
	{
		Entries.Add(Path, NarrativeSpaceSearch::Index(*Asset));
	}
	else
	{
		Entries.Remove(Path);
	}
}

#undef LOCTEXT_NAMESPACE
