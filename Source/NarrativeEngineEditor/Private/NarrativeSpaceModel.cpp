#include "NarrativeSpaceModel.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
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

void FNarrativeSpaceModel::Refresh()
{
	if (IsDragging()) { bRefreshPending = true; return; }
	bRefreshPending = false;
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

void FNarrativeSpaceModel::PostUndo(bool bSuccess) { if (bSuccess) { bRefreshPending = true; } }
void FNarrativeSpaceModel::PropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (!bOwnChange && Object && Object->IsA<UNarrativeDataAsset>()) { bRefreshPending = true; }
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
