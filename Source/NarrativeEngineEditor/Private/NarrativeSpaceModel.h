#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "NarrativeSpaceQuery.h"
#include "Styling/SlateBrush.h"
#include "UObject/GCObject.h"
#include "UObject/FieldPath.h"

class FScopedTransaction;
class FStructProperty;
struct FAssetData;

enum class ENarrativeSpaceView : uint8 { XY, XZ, YZ, Iso };

/** Orthographic camera. Screen Y points down; Up points up in narrative space. */
struct FNarrativeSpaceCamera
{
	FVector Center = FVector::ZeroVector;
	FVector Right = FVector::XAxisVector;
	FVector Up = FVector::YAxisVector;
	double Zoom = 24.0;

	FVector2D Project(const FVector& Position, const FVector2D& Size) const;
	FVector Unproject(const FVector2D& Screen, const FVector2D& Size) const;
	FVector DragDelta(const FVector2D& Pixels, int32 AxisLock = INDEX_NONE) const;
	void SetView(ENarrativeSpaceView View);
	void Orbit(const FVector2D& Pixels);
	void ZoomAt(const FVector2D& Screen, const FVector2D& Size, double WheelDelta);
	void Frame(const TArray<FVector>& Positions, const FVector2D& Size);
	double GridStep() const;
};

struct FNarrativeSpacePoint
{
	TWeakObjectPtr<UNarrativeDataAsset> Asset;
	FVector Position = FVector::ZeroVector;
	uint8 PresentAxes = 0;
	float Radius = 0.f;
	FText Label;
	FLinearColor Color;
	FSlateBrush Icon;
};

/** Owns loaded assets, cached placement fields and the complete edit transaction. */
class FNarrativeSpaceModel : public FSelfRegisteringEditorUndoClient, public FGCObject
{
public:
	FNarrativeSpaceModel();
	virtual ~FNarrativeSpaceModel() override;
	bool SetQuery(const FNarrativeSpaceQuery& InQuery);
	void Refresh();
	void Tick();
	const FNarrativeSpaceQuery& GetQuery() const { return Query; }
	const TArray<FNarrativeSpacePoint>& GetPoints() const { return Points; }
	/** Union of editable vector fields on matching assets, including inherited fields and omitted assets. */
	const TArray<FName>& GetPlacementFields() const { return PlacementFields; }
	const FText& GetStatus() const { return Status; }
	bool IsQueryValid() const { return bQueryValid; }
	bool IsDragging() const { return Transaction.IsValid(); }
	bool CanEdit() const;
	TArray<UObject*> GetSelection() const;
	bool IsSelected(const UNarrativeDataAsset* Asset) const;
	void Select(const TArray<UNarrativeDataAsset*>& Assets, bool bAdd = false);
	bool BeginDrag();
	void UpdateDrag(const FVector& Delta, int32 AxisLock, double SnapStep);
	void EndDrag(bool bCancel = false);
	void Save();
	void OpenSelection();
	FStructProperty* ResolvePlacement(UNarrativeDataAsset* Asset);
	FSimpleMulticastDelegate OnSelectionChanged;

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FNarrativeSpaceModel"); }

	/** Pure authoring operation shared by dragging and tests. Only moved axes are authored. */
	static FVectorND Offset(const FVectorND& Original, const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& Axes,
		const FVector& Delta, int32 AxisLock, double SnapStep);

private:
	struct FDragOriginal
	{
		TWeakObjectPtr<UNarrativeDataAsset> Asset;
		FVectorND Value;
		TFieldPath<FStructProperty> Property;
	};
	FNarrativeSpaceQuery Query;
	TArray<FNarrativeSpacePoint> Points;
	TArray<FName> PlacementFields;
	TArray<TObjectPtr<UNarrativeDataAsset>> LoadedAssets;
	TArray<TObjectPtr<UObject>> LoadedIcons;
	TMap<TWeakObjectPtr<UClass>, TFieldPath<FStructProperty>> PlacementCache;
	TSet<TWeakObjectPtr<UNarrativeDataAsset>> Selection;
	TArray<FDragOriginal> Originals;
	TMap<TWeakObjectPtr<UPackage>, bool> PackageDirtyBefore;
	TUniquePtr<FScopedTransaction> Transaction;
	FText Status;
	bool bQueryValid = false;
	bool bRefreshPending = false;
	/** An in-flight interactive edit: re-read positions, but leave the details panel alone. */
	bool bReadPending = false;
	bool bOwnChange = false;

	void ReadPoints();
	void PropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	void AssetChanged(const FAssetData& Asset);
	void AssetRenamed(const FAssetData& Asset, const FString& OldPath);
	void RegistryReady();
	void ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements);
};
