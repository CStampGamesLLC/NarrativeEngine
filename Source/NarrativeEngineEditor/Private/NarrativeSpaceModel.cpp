#include "NarrativeSpaceModel.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "NarrativeSpace"

bool FNarrativeSpaceQuery::Validate(FText& OutError) const
{
	if (Axes.Num() < 1 || Axes.Num() > 3)
	{
		OutError = LOCTEXT("AxisCount", "Choose one to three basis assets in Axes (X, Y, Z).");
		return false;
	}
	TSet<FSoftObjectPath> Seen;
	for (const auto& Axis : Axes)
	{
		if (Axis.IsNull() || Seen.Contains(Axis.ToSoftObjectPath()) || !Axis.LoadSynchronous())
		{
			OutError = LOCTEXT("AxisInvalid", "Axes must be distinct, existing narrative basis assets.");
			return false;
		}
		Seen.Add(Axis.ToSoftObjectPath());
	}
	if (!ContentPath.IsEmpty() && !FPackageName::IsValidLongPackageName(ContentPath))
	{
		OutError = LOCTEXT("PathInvalid", "Content Path must be a package path, such as /Game or /NarrativeEngine.");
		return false;
	}
	for (const auto& Class : Classes)
	{
		if (!Class || !Class->IsChildOf(UNarrativeDataAsset::StaticClass()))
		{
			OutError = LOCTEXT("ClassInvalid", "Remove empty class entries, or choose a narrative data asset class.");
			return false;
		}
	}
	OutError = FText::GetEmpty();
	return true;
}

FVector2D FNarrativeSpaceCamera::Project(const FVector& Position, const FVector2D& Size) const
{
	const FVector Relative = Position - Center;
	return Size * 0.5 + FVector2D(Relative.Dot(Right), -Relative.Dot(Up)) * Zoom;
}

FVector FNarrativeSpaceCamera::Unproject(const FVector2D& Screen, const FVector2D& Size) const
{
	return Center + DragDelta(Screen - Size * 0.5);
}

FVector FNarrativeSpaceCamera::DragDelta(const FVector2D& Pixels, int32 AxisLock) const
{
	if (AxisLock >= 0 && AxisLock < 3)
	{
		const FVector2D Direction(Right[AxisLock], -Up[AxisLock]);
		const double LengthSquared = Direction.SizeSquared();
		FVector Result = FVector::ZeroVector;
		// An axis viewed end-on cannot be moved reliably in this projection.
		if (LengthSquared > 1.e-6) { Result[AxisLock] = FVector2D::DotProduct(Pixels, Direction) / (Zoom * LengthSquared); }
		return Result;
	}
	return (Right * Pixels.X - Up * Pixels.Y) / Zoom;
}

void FNarrativeSpaceCamera::SetView(ENarrativeSpaceView View)
{
	Right = View == ENarrativeSpaceView::YZ ? FVector::YAxisVector : FVector::XAxisVector;
	Up = View == ENarrativeSpaceView::XY ? FVector::YAxisVector : FVector::ZAxisVector;
	if (View == ENarrativeSpaceView::Iso)
	{
		Right = FVector(1, -1, 0).GetSafeNormal();
		Up = FVector(1, 1, 2).GetSafeNormal();
	}
}

void FNarrativeSpaceCamera::Orbit(const FVector2D& Pixels)
{
	const FQuat Yaw(FVector::ZAxisVector, -Pixels.X * 0.008);
	Right = Yaw.RotateVector(Right).GetSafeNormal();
	Up = FQuat(Right, Pixels.Y * 0.008).RotateVector(Yaw.RotateVector(Up)).GetSafeNormal();
}

void FNarrativeSpaceCamera::ZoomAt(const FVector2D& Screen, const FVector2D& Size, double WheelDelta)
{
	const FVector Before = Unproject(Screen, Size);
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.25, WheelDelta), 0.01, 10000.0);
	Center += Before - Unproject(Screen, Size);
}

void FNarrativeSpaceCamera::Frame(const TArray<FVector>& Positions, const FVector2D& Size)
{
	if (Positions.IsEmpty()) { return; }
	FBox Bounds(ForceInit);
	for (const FVector& Position : Positions) { Bounds += Position; }
	Center = Bounds.GetCenter();
	double Width = 1.0, Height = 1.0;
	for (const FVector& Position : Positions)
	{
		Width = FMath::Max(Width, 2.0 * FMath::Abs((Position - Center).Dot(Right)));
		Height = FMath::Max(Height, 2.0 * FMath::Abs((Position - Center).Dot(Up)));
	}
	Zoom = FMath::Clamp(FMath::Min(FMath::Max(100.0, Size.X - 220.0) / Width,
		FMath::Max(100.0, Size.Y - 160.0) / Height), 0.01, 200.0);
}

double FNarrativeSpaceCamera::GridStep() const
{
	return FMath::Pow(10.0, FMath::FloorToDouble(FMath::LogX(10.0, 90.0 / Zoom)));
}

FNarrativeSpaceModel::FNarrativeSpaceModel()
{
	CreationRecord = NewObject<UNarrativeSpaceCreationRecord>(GetTransientPackage(), NAME_None, RF_Transactional);
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registry.OnAssetAdded().AddRaw(this, &FNarrativeSpaceModel::AssetChanged);
	Registry.OnAssetRemoved().AddRaw(this, &FNarrativeSpaceModel::AssetChanged);
	Registry.OnAssetRenamed().AddRaw(this, &FNarrativeSpaceModel::AssetRenamed);
	Registry.OnFilesLoaded().AddRaw(this, &FNarrativeSpaceModel::RegistryReady);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FNarrativeSpaceModel::PropertyChanged);
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FNarrativeSpaceModel::ObjectsReplaced);
}

FNarrativeSpaceModel::~FNarrativeSpaceModel()
{
	EndDrag(true);
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	if (FAssetRegistryModule* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
	{
		Module->Get().OnAssetAdded().RemoveAll(this);
		Module->Get().OnAssetRemoved().RemoveAll(this);
		Module->Get().OnAssetRenamed().RemoveAll(this);
		Module->Get().OnFilesLoaded().RemoveAll(this);
	}
}

void FNarrativeSpaceModel::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(LoadedAssets);
	Collector.AddReferencedObjects(LoadedIcons);
	Collector.AddReferencedObjects(CreatedAssets);
	Collector.AddReferencedObject(CreationRecord);
	Collector.AddPropertyReferences(FNarrativeSpaceQuery::StaticStruct(), &Query);
}

bool FNarrativeSpaceModel::SetQuery(const FNarrativeSpaceQuery& InQuery)
{
	EndDrag(true);
	Query = InQuery;
	Refresh();
	return bQueryValid;
}

FStructProperty* FNarrativeSpaceModel::ResolvePlacement(UNarrativeDataAsset* Asset)
{
	if (!Asset) { return nullptr; }
	if (auto* Cached = PlacementCache.Find(Asset->GetClass())) { return Cached->Get(); }
	const FName Name = Query.PlacementField.IsNone() ? Asset->GetSpacePlacementProperty() : Query.PlacementField;
	FStructProperty* Property = Name.IsNone() ? nullptr : FindFProperty<FStructProperty>(Asset->GetClass(), Name);
	if (Property && (Property->Struct != FVectorND::StaticStruct() || Property->ArrayDim != 1 || !Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_EditConst))) { Property = nullptr; }
	PlacementCache.Add(Asset->GetClass(), Property);
	return Property;
}

TArray<UClass*> FNarrativeSpaceModel::GetCreatableClasses()
{
	TArray<UClass*> Candidates;
	GetDerivedClasses(UNarrativeDataAsset::StaticClass(), Candidates, true);
	Candidates.Add(UNarrativeDataAsset::StaticClass());

	TArray<UClass*> Result;
	for (UClass* Class : Candidates)
	{
		if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) { continue; }
		// Skeleton and reinstanced classes are blueprint compilation artifacts, not authorable types.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_"))) { continue; }
		if (!Query.Classes.IsEmpty())
		{
			const bool bMatches = Query.Classes.ContainsByPredicate([Class](const TSubclassOf<UNarrativeDataAsset>& Filter)
			{
				return Filter && Class->IsChildOf(Filter);
			});
			if (!bMatches) { continue; }
		}
		// Offer only classes whose placement field resolves, so anything created is actually plotted.
		if (!ResolvePlacement(Class->GetDefaultObject<UNarrativeDataAsset>())) { continue; }
		Result.Add(Class);
	}
	Result.Sort([](const UClass& A, const UClass& B)
	{
		return A.GetDisplayNameText().ToString() < B.GetDisplayNameText().ToString();
	});
	return Result;
}

UNarrativeDataAsset* FNarrativeSpaceModel::CreateAsset(UClass* Class, const FVector& Position)
{
	if (!CanEdit() || !Class || !Class->IsChildOf(UNarrativeDataAsset::StaticClass())) { return nullptr; }
	if (!CreationRecord || GIsTransacting || GEditor->IsTransactionActive()) { return nullptr; }
	EndDrag(true);

	// The record is what actually gets transacted; see UNarrativeSpaceCreationRecord.
	const FScopedTransaction CreateTransaction(LOCTEXT("MakeNew", "Create narrative asset"));
	CreationRecord->Modify();

	FString BaseName = Class->GetName();
	BaseName.RemoveFromEnd(TEXT("_C"));
	const FString Path = Query.ContentPath.IsEmpty() ? TEXT("/Game") : Query.ContentPath;

	FString PackageName;
	FString AssetName;
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get()
		.CreateUniqueAssetName(Path / (TEXT("New") + BaseName), TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package) { return nullptr; }
	UNarrativeDataAsset* Asset = NewObject<UNarrativeDataAsset>(Package, Class, *AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!Asset) { return nullptr; }

	// Author the placement before Refresh reads positions back out of the asset.
	if (FStructProperty* Property = ResolvePlacement(Asset))
	{
		FVectorND& Vector = *Property->ContainerPtrToValuePtr<FVectorND>(Asset);
		for (int32 Axis = 0; Axis < Query.Axes.Num(); ++Axis)
		{
			Vector.SetCoordinate(Query.Axes[Axis], float(Position[Axis]));
		}
	}

	FAssetRegistryModule::AssetCreated(Asset);
	Package->MarkPackageDirty();

	// The new package stays dirty until an explicit save, undone or not.
	CreationRecord->Created.Add(Asset);
	CreatedAssets.Add(Asset);
	LiveCreated.Add(Asset);

	Refresh();
	Select({Asset});
	return Asset;
}

int32 FNarrativeSpaceModel::DeleteAssets(const TArray<UNarrativeDataAsset*>& Assets, bool bShowConfirmation)
{
	if (!CanEdit() || Assets.IsEmpty()) { return 0; }
	EndDrag(true);

	TArray<UObject*> Objects;
	for (UNarrativeDataAsset* Asset : Assets)
	{
		if (Asset && IsValid(Asset)) { Objects.AddUnique(Asset); }
	}
	if (Objects.IsEmpty()) { return 0; }

	/** What we were tracking about a doomed asset, so a declined delete can be put back. */
	struct FDoomed
	{
		TWeakObjectPtr<UNarrativeDataAsset> Asset;
		bool bTracked = false;
		bool bRecorded = false;
		bool bLive = false;
	};
	TArray<FDoomed> Doomed;
	Doomed.Reserve(Objects.Num());
	for (UObject* Object : Objects)
	{
		UNarrativeDataAsset* Asset = CastChecked<UNarrativeDataAsset>(Object);
		FDoomed& Entry = Doomed.AddDefaulted_GetRef();
		Entry.Asset = Asset;
		// Our own strong references send ObjectTools down its force-delete path, which frees the
		// object outright and leaves every raw pointer to it dangling. Let go of them first.
		Entry.bTracked = CreatedAssets.Remove(Asset) > 0;
		Entry.bRecorded = CreationRecord && CreationRecord->Created.Remove(Asset) > 0;
		Entry.bLive = LiveCreated.Remove(Entry.Asset) > 0;
		Selection.Remove(Entry.Asset);
		LoadedAssets.Remove(Asset);
	}

	// ObjectTools runs the editor's own confirmation and reference check. This is not undoable.
	const int32 Deleted = ObjectTools::DeleteObjects(Objects, bShowConfirmation);

	// Objects is off limits from here: those pointers may name freed memory, and even building a
	// TWeakObjectPtr from one would index the object array with -1. The weak handles are safe.
	for (const FDoomed& Entry : Doomed)
	{
		UNarrativeDataAsset* Asset = Entry.Asset.Get();
		// Survived, so the user declined. Put it back on the books; its create is still undoable.
		if (!Asset) { continue; }
		if (Entry.bTracked) { CreatedAssets.Add(Asset); }
		if (Entry.bRecorded && CreationRecord) { CreationRecord->Created.Add(Asset); }
		if (Entry.bLive) { LiveCreated.Add(Entry.Asset); }
	}

	Refresh();
	return Deleted;
}

TArray<FString> FNarrativeSpaceModel::RenameNames(const FString& NewName, const int32 Count)
{
	TArray<FString> Result;
	const FString Base = NewName.TrimStartAndEnd();
	if (Base.IsEmpty() || Count <= 0) { return Result; }
	if (Count == 1) { Result.Add(Base); return Result; }

	const int32 Width = FString::FromInt(Count).Len();
	for (int32 Number = 1; Number <= Count; ++Number)
	{
		FString Suffix = FString::FromInt(Number);
		while (Suffix.Len() < Width) { Suffix.InsertAt(0, TEXT('0')); }
		Result.Add(Base + TEXT("_") + Suffix);
	}
	return Result;
}

FString FNarrativeSpaceModel::RenameSeed(const TArray<UNarrativeDataAsset*>& Assets)
{
	TArray<FString> Names;
	for (const UNarrativeDataAsset* Asset : Assets) { if (Asset) { Names.Add(Asset->GetName()); } }
	if (Names.IsEmpty()) { return FString(); }
	if (Names.Num() == 1) { return Names[0]; }

	// The stem the batch already shares, so re-numbering one usually only needs Enter. Trailing
	// digits and separators come off it, because RenameNames puts its own numbering back on.
	FString Stem = Names[0];
	for (const FString& Name : Names)
	{
		int32 Shared = 0;
		while (Shared < Stem.Len() && Shared < Name.Len() && Stem[Shared] == Name[Shared]) { ++Shared; }
		Stem.LeftInline(Shared);
	}
	while (!Stem.IsEmpty() && (FChar::IsDigit(Stem[Stem.Len() - 1]) || Stem[Stem.Len() - 1] == TEXT('_')))
	{
		Stem.LeftChopInline(1);
	}
	return Stem;
}

bool FNarrativeSpaceModel::CanRenameAssets(const TArray<UNarrativeDataAsset*>& Assets, const FString& NewName, FText& OutError) const
{
	OutError = FText::GetEmpty();
	if (!CanEdit() || IsDragging() || GIsTransacting || GEditor->IsTransactionActive())
	{
		OutError = LOCTEXT("RenameBusy", "Renaming is not available right now.");
		return false;
	}
	const TArray<FString> Names = RenameNames(NewName, Assets.Num());
	if (Names.IsEmpty())
	{
		OutError = LOCTEXT("RenameEmpty", "Enter a name.");
		return false;
	}

	for (int32 Index = 0; Index < Assets.Num(); ++Index)
	{
		const UNarrativeDataAsset* Asset = Assets[Index];
		if (!Asset || !IsValid(Asset)) { continue; }
		// A batch numbers past whatever is already in the folder, so only a lone rename reports a
		// collision -- and renaming an asset to the name it already has is a no-op, not a clash.
		const bool bUnique = Assets.Num() == 1 && Asset->GetName() != Names[Index];
		const FString Folder = FPackageName::GetLongPackagePath(Asset->GetPackage()->GetName());
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *Folder, *Names[Index], *Names[Index]);
		if (!AssetViewUtils::IsValidObjectPathForCreate(ObjectPath, bUnique ? Asset->GetClass() : nullptr, OutError, !bUnique))
		{
			return false;
		}
	}
	return true;
}

int32 FNarrativeSpaceModel::RenameAssets(const TArray<UNarrativeDataAsset*>& Assets, const FString& NewName)
{
	FText Error;
	if (!CanRenameAssets(Assets, NewName, Error)) { return 0; }
	EndDrag(true);

	const TArray<FString> Names = RenameNames(NewName, Assets.Num());
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	TArray<FAssetRenameData> Renames;
	for (int32 Index = 0; Index < Assets.Num(); ++Index)
	{
		UNarrativeDataAsset* Asset = Assets[Index];
		if (!Asset || !IsValid(Asset) || Asset->GetName() == Names[Index]) { continue; }
		const FString Folder = FPackageName::GetLongPackagePath(Asset->GetPackage()->GetName());
		FString PackageName;
		FString AssetName;
		AssetTools.CreateUniqueAssetName(Folder / Names[Index], TEXT(""), PackageName, AssetName);
		// The folder is kept, so this only ever renames; localized variants follow the asset.
		Renames.Emplace(Asset, Folder, AssetName, false, true);
	}
	if (Renames.IsEmpty()) { return 0; }

	// AssetTools carries references, redirectors and source control across, and puts up its own
	// dialog for anything it cannot rename. Like a Content Browser rename, this is not undoable.
	const bool bRenamed = AssetTools.RenameAssetsWithDialog(Renames) == EAssetRenameResult::Success;
	bRefreshPending = true;
	return bRenamed ? Renames.Num() : 0;
}

void FNarrativeSpaceModel::Refresh()
{
	if (IsDragging()) { bRefreshPending = true; return; }
	bRefreshPending = false;
	bReadPending = false;
	LoadedAssets.Reset();
	LoadedIcons.Reset();
	Points.Reset();
	PlacementFields.Reset();
	PlacementCache.Reset();
	bQueryValid = Query.Validate(Status);
	int32 Skipped = 0;
	if (bQueryValid)
	{
		FARFilter Filter;
		Filter.PackagePaths.Add(FName(Query.ContentPath.IsEmpty() ? TEXT("/Game") : *Query.ContentPath));
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;
		if (Query.Classes.IsEmpty()) { Filter.ClassPaths.Add(UNarrativeDataAsset::StaticClass()->GetClassPathName()); }
		for (const auto& Class : Query.Classes) { Filter.ClassPaths.Add(Class->GetClassPathName()); }
		TArray<FAssetData> Assets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssets(Filter, Assets);
		Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString(); });
		TSet<UClass*> InspectedClasses;
		for (const FAssetData& Data : Assets)
		{
			UNarrativeDataAsset* Asset = Cast<UNarrativeDataAsset>(Data.GetAsset());
			if (Asset && !InspectedClasses.Contains(Asset->GetClass()))
			{
				InspectedClasses.Add(Asset->GetClass());
				for (TFieldIterator<FStructProperty> It(Asset->GetClass()); It; ++It)
				{
					if (It->Struct == FVectorND::StaticStruct() && It->ArrayDim == 1 && It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_EditConst))
					{
						PlacementFields.AddUnique(It->GetFName());
					}
				}
			}
			FStructProperty* Property = ResolvePlacement(Asset);
			if (!Property || !Property->ContainerPtrToValuePtr<FVectorND>(Asset)->IsValid()) { ++Skipped; continue; }
			LoadedAssets.Add(Asset);
			FNarrativeSpacePoint& Point = Points.AddDefaulted_GetRef();
			Point.Asset = Asset;
			Point.Label = Asset->GetSpaceLabel();
			Point.Color = FLinearColor::MakeFromHSV8(uint8(GetTypeHash(Asset->GetClass()->GetPathName()) % 255), 130, 230);
			if (UTexture2D* Texture = Asset->GetSpaceIcon().LoadSynchronous())
			{
				LoadedIcons.Add(Texture);
				Point.Icon.SetResourceObject(Texture);
				Point.Icon.DrawAs = ESlateBrushDrawType::Image;
			}
		}
		PlacementFields.Sort(FNameLexicalLess());
		Status = FText::Format(LOCTEXT("FieldCount", "{0} assets | Field: {1} | {2} without a valid matching vector | Hollow: missing an axis"),
			FText::AsNumber(Points.Num()), Query.PlacementField.IsNone() ? LOCTEXT("DefaultField", "Class default") : FText::FromName(Query.PlacementField), FText::AsNumber(Skipped));
	}
	for (auto It = Selection.CreateIterator(); It; ++It)
	{
		if (!LoadedAssets.Contains(It->Get())) { It.RemoveCurrent(); }
	}
	ReadPoints();
	OnSelectionChanged.Broadcast();
}

void FNarrativeSpaceModel::ReadPoints()
{
	for (auto& Point : Points)
	{
		UNarrativeDataAsset* Asset = Point.Asset.Get();
		FStructProperty* Property = ResolvePlacement(Asset);
		if (!Property) { continue; }
		const FVectorND& Value = *Property->ContainerPtrToValuePtr<FVectorND>(Asset);
		Point.Position = FVector::ZeroVector;
		Point.PresentAxes = 0;
		for (int32 Axis = 0; Axis < Query.Axes.Num(); ++Axis)
		{
			Point.Position[Axis] = Value.GetCoordinate(Query.Axes[Axis]);
			if (Value.GetBasis().Contains(Query.Axes[Axis])) { Point.PresentAxes |= 1 << Axis; }
		}
		Point.Radius = FMath::IsFinite(Asset->GetSpaceRadius()) ? FMath::Max(0.f, Asset->GetSpaceRadius()) : 0.f;
	}
}

void FNarrativeSpaceModel::Tick()
{
	if (IsDragging() && !CanEdit()) { EndDrag(true); }
	if (bRefreshPending && !IsDragging()) { Refresh(); }
	else if (bReadPending && !IsDragging()) { bReadPending = false; ReadPoints(); }
}

bool FNarrativeSpaceModel::CanEdit() const { return bQueryValid && GEditor && !GEditor->IsPlaySessionInProgress(); }

TArray<UObject*> FNarrativeSpaceModel::GetSelection() const
{
	TArray<UObject*> Result;
	for (const auto& Point : Points) { if (IsSelected(Point.Asset.Get())) { Result.Add(Point.Asset.Get()); } }
	return Result;
}

bool FNarrativeSpaceModel::IsSelected(const UNarrativeDataAsset* Asset) const { return Asset && Selection.Contains(Asset); }

void FNarrativeSpaceModel::Select(const TArray<UNarrativeDataAsset*>& Assets, bool bAdd)
{
	if (!bAdd) { Selection.Reset(); }
	for (UNarrativeDataAsset* Asset : Assets) { if (LoadedAssets.Contains(Asset)) { Selection.Add(Asset); } }
	OnSelectionChanged.Broadcast();
}

FVectorND FNarrativeSpaceModel::Offset(const FVectorND& Original, const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& Axes,
	const FVector& Delta, int32 AxisLock, double SnapStep)
{
	FVectorND Result = Original;
	for (int32 Axis = 0; Axis < FMath::Min(3, Axes.Num()); ++Axis)
	{
		if ((AxisLock != INDEX_NONE && AxisLock != Axis) || FMath::IsNearlyZero(Delta[Axis], 1.e-8)) { continue; }
		double Value = Original.GetCoordinate(Axes[Axis]) + Delta[Axis];
		if (SnapStep > 0.0) { Value = FMath::GridSnap(Value, SnapStep); }
		if (FMath::IsFinite(Value) && FMath::Abs(Value) <= MAX_flt) { Result.SetCoordinate(Axes[Axis], float(Value)); }
	}
	return Result;
}

bool FNarrativeSpaceModel::BeginDrag()
{
	if (!CanEdit() || IsDragging() || Selection.IsEmpty() || GIsTransacting || GEditor->IsTransactionActive()) { return false; }
	Transaction = MakeUnique<FScopedTransaction>(LOCTEXT("Move", "Move narrative assets"));
	for (UObject* Object : GetSelection())
	{
		UNarrativeDataAsset* Asset = CastChecked<UNarrativeDataAsset>(Object);
		if (FStructProperty* Property = ResolvePlacement(Asset))
		{
			Originals.Add({Asset, *Property->ContainerPtrToValuePtr<FVectorND>(Asset), Property});
			PackageDirtyBefore.FindOrAdd(Asset->GetPackage(), Asset->GetPackage()->IsDirty());
			Asset->SetFlags(RF_Transactional);
			Asset->Modify();
			Asset->PreEditChange(Property);
		}
	}
	return true;
}

void FNarrativeSpaceModel::UpdateDrag(const FVector& Delta, int32 AxisLock, double SnapStep)
{
	if (!IsDragging()) { return; }
	if (!CanEdit()) { EndDrag(true); return; }
	for (const FDragOriginal& Original : Originals)
	{
		if (UNarrativeDataAsset* Asset = Original.Asset.Get())
		{
			if (FStructProperty* Property = Original.Property.Get())
			{
				*Property->ContainerPtrToValuePtr<FVectorND>(Asset) = Offset(Original.Value, Query.Axes, Delta, AxisLock, SnapStep);
			}
		}
	}
	ReadPoints();
}

void FNarrativeSpaceModel::EndDrag(bool bCancel)
{
	if (!IsDragging()) { return; }
	TGuardValue<bool> Guard(bOwnChange, true);
	bool bChanged = false;
	for (const FDragOriginal& Original : Originals)
	{
		if (UNarrativeDataAsset* Asset = Original.Asset.Get())
		{
			if (FStructProperty* Property = Original.Property.Get())
			{
				FVectorND& Current = *Property->ContainerPtrToValuePtr<FVectorND>(Asset);
				bChanged |= Current.GetBasis() != Original.Value.GetBasis() || Current.GetCoordinates() != Original.Value.GetCoordinates();
				if (bCancel) { Current = Original.Value; }
				FPropertyChangedEvent Event(Property, EPropertyChangeType::ValueSet);
				Asset->PostEditChangeProperty(Event);
			}
		}
	}
	if (bCancel || !bChanged)
	{
		Transaction->Cancel();
		for (const auto& Pair : PackageDirtyBefore) { if (UPackage* Package = Pair.Key.Get()) { Package->SetDirtyFlag(Pair.Value); } }
	}
	Transaction.Reset();
	Originals.Reset();
	PackageDirtyBefore.Reset();
	ReadPoints();
}

void FNarrativeSpaceModel::Save()
{
	if (!CanEdit() || IsDragging()) { return; }
	TArray<UPackage*> Packages;
	for (UNarrativeDataAsset* Asset : LoadedAssets) { if (Asset->GetPackage()->IsDirty()) { Packages.AddUnique(Asset->GetPackage()); } }
	if (!Packages.IsEmpty()) { FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true); }
}

void FNarrativeSpaceModel::OpenSelection()
{
	if (GEditor) { GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(GetSelection()); }
}

void FNarrativeSpaceModel::PostUndo(bool bSuccess)
{
	if (!bSuccess) { return; }
	SyncCreatedAssets();
	bRefreshPending = true;
}

void FNarrativeSpaceModel::SyncCreatedAssets()
{
	if (!CreationRecord) { return; }
	for (const TObjectPtr<UNarrativeDataAsset>& Created : CreatedAssets)
	{
		UNarrativeDataAsset* Asset = Created.Get();
		// A destructive delete can retire an asset that undo would otherwise try to resurrect.
		if (!Asset || !IsValid(Asset)) { continue; }

		const bool bShouldBeLive = CreationRecord->Created.Contains(Asset);
		if (bShouldBeLive == LiveCreated.Contains(Asset)) { continue; }
		if (bShouldBeLive)
		{
			Asset->SetFlags(RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Asset);
			Asset->GetPackage()->MarkPackageDirty();
			LiveCreated.Add(Asset);
		}
		else
		{
			// The asset is unsaved either way, so dropping it from the registry undoes the create.
			FAssetRegistryModule::AssetDeleted(Asset);
			Asset->ClearFlags(RF_Public | RF_Standalone);
			Asset->GetPackage()->SetDirtyFlag(false);
			LiveCreated.Remove(Asset);
		}
	}
}
void FNarrativeSpaceModel::PropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (bOwnChange || !Object || !Object->IsA<UNarrativeDataAsset>()) { return; }
	// Dragging a coordinate spinbox broadcasts an interactive change on every mouse move, and a
	// full refresh rebuilds the details tree out from under the cursor, ending the drag. Re-read
	// the plotted positions instead so the viewport tracks the drag live; the ValueSet that closes
	// the interaction still refreshes.
	if ((Event.ChangeType & EPropertyChangeType::Interactive) != 0) { bReadPending = true; }
	else { bRefreshPending = true; }
}
void FNarrativeSpaceModel::AssetChanged(const FAssetData& Asset) { bRefreshPending = true; }
void FNarrativeSpaceModel::AssetRenamed(const FAssetData& Asset, const FString& OldPath) { bRefreshPending = true; }
void FNarrativeSpaceModel::RegistryReady() { bRefreshPending = true; }
void FNarrativeSpaceModel::ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements)
{
	EndDrag(true);
	PlacementCache.Reset();
	bRefreshPending = true;
}

#undef LOCTEXT_NAMESPACE
