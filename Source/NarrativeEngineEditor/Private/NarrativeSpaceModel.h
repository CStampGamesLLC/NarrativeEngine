#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "NarrativeSpaceQuery.h"
#include "Styling/SlateBrush.h"
#include "UObject/GCObject.h"
#include "UObject/FieldPath.h"

class FScopedTransaction;
class FStructProperty;
class IAssetRegistry;
struct FAssetData;

enum class ENarrativeSpaceView : uint8 { XY, XZ, YZ, Iso };

/** Which set of values the viewport plots. */
enum class ENarrativeSpaceSource : uint8
{
	/** Authored asset values. Editable, including while a play session runs.  */
	Static,
	/** The play session's simulated entity positions. A live view; the simulation owns them. */
	Runtime
};

/** How a query axis presents itself, shared by the viewport and the axis picker. */
namespace NarrativeSpaceAxis
{
	/** Display letter for a query axis index: X, Y, Z in query order. */
	const TCHAR* Label(int32 Axis);

	/** The colour the viewport draws this axis in, and the picker tags its entry with. */
	FLinearColor Color(int32 Axis);
}

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
	/** True when this position came from a simulated entity rather than from the asset. */
	bool bLive = false;
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

	/** Switches between authored and simulated values. Cancels any drag the change would strand. */
	void SetSource(ENarrativeSpaceSource InSource);
	ENarrativeSpaceSource GetSource() const { return Source; }
	void Refresh();
	void Tick();
	const FNarrativeSpaceQuery& GetQuery() const { return Query; }
	const TArray<FNarrativeSpacePoint>& GetPoints() const { return Points; }
	/** Every basis asset in the project, by name, tagged as an axis or not. Drives the axis picker. */
	const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& GetBasisAssets() const { return BasisAssets; }
	/** Union of editable vector fields on matching assets, including inherited fields and omitted assets. */
	const TArray<FName>& GetPlacementFields() const { return PlacementFields; }
	const FText& GetStatus() const { return Status; }

	/** What the current source is showing, and whether a play session is backing it. */
	FText GetSourceStatus() const;
	bool IsQueryValid() const { return bQueryValid; }
	bool IsDragging() const { return Transaction.IsValid(); }

	/** True when authored values can be edited at all. A play session does not stop this. */
	bool CanEdit() const;

	/** True when dragging authors asset values. Runtime values belong to the simulation, not here. */
	bool CanPlace() const;

	/** True when assets may be created, renamed or deleted. Never while a play session holds them. */
	bool CanManageAssets() const;
	TArray<UObject*> GetSelection() const;
	bool IsSelected(const UNarrativeDataAsset* Asset) const;
	void Select(const TArray<UNarrativeDataAsset*>& Assets, bool bAdd = false);
	bool BeginDrag();
	void UpdateDrag(const FVector& Delta, int32 AxisLock, double SnapStep);
	void EndDrag(bool bCancel = false);
	void Save();
	void OpenSelection();

	/** Concrete classes this query can plot, sorted by display name, for the Make New menu. */
	TArray<UClass*> GetCreatableClasses();

	/**
	 * Creates an unsaved data asset of Class under the query's content path, places it at Position
	 * and selects it. Null when the query, the class or its placement field will not allow it.
	 */
	UNarrativeDataAsset* CreateAsset(UClass* Class, const FVector& Position);

	/**
	 * Deletes assets through the editor's own confirmation and reference check. Destructive and
	 * outside the undo buffer, exactly like deleting from the Content Browser. Returns the count.
	 */
	int32 DeleteAssets(const TArray<UNarrativeDataAsset*>& Assets, bool bShowConfirmation = true);

	/**
	 * Renames assets in place through the editor's own rename, so references, redirectors and
	 * source control follow, and its dialog reports anything it cannot do. Outside the undo buffer,
	 * exactly like renaming from the Content Browser. Each asset keeps its own folder. Returns the
	 * number actually renamed; assets already carrying their target name are left alone.
	 */
	int32 RenameAssets(const TArray<UNarrativeDataAsset*>& Assets, const FString& NewName);

	/** True when NewName can name this batch. OutError explains the first problem for the rename box. */
	bool CanRenameAssets(const TArray<UNarrativeDataAsset*>& Assets, const FString& NewName, FText& OutError) const;

	/**
	 * Target names for one rename batch, in the order given. A lone asset takes NewName exactly;
	 * several are numbered from it, zero padded to the batch's width so they sort as they are named.
	 */
	static TArray<FString> RenameNames(const FString& NewName, int32 Count);

	/** Text the rename box opens with: the one asset's name, or the stem several already share. */
	static FString RenameSeed(const TArray<UNarrativeDataAsset*>& Assets);
	FStructProperty* ResolvePlacement(UNarrativeDataAsset* Asset);
	FSimpleMulticastDelegate OnSelectionChanged;
	/** Broadcast when the project's set of basis assets changes, never for a change of axes. */
	FSimpleMulticastDelegate OnBasisAssetsChanged;

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
	ENarrativeSpaceSource Source = ENarrativeSpaceSource::Static;
	TArray<FNarrativeSpacePoint> Points;
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> BasisAssets;
	TArray<FName> PlacementFields;
	TArray<TObjectPtr<UNarrativeDataAsset>> LoadedAssets;
	TArray<TObjectPtr<UObject>> LoadedIcons;
	TMap<TWeakObjectPtr<UClass>, TFieldPath<FStructProperty>> PlacementCache;
	TSet<TWeakObjectPtr<UNarrativeDataAsset>> Selection;
	TArray<FDragOriginal> Originals;
	TMap<TWeakObjectPtr<UPackage>, bool> PackageDirtyBefore;
	TUniquePtr<FScopedTransaction> Transaction;
	TObjectPtr<UNarrativeSpaceCreationRecord> CreationRecord;
	/** Everything Make New produced this session, kept alive so undo and redo can toggle it. */
	TArray<TObjectPtr<UNarrativeDataAsset>> CreatedAssets;
	TSet<TWeakObjectPtr<UNarrativeDataAsset>> LiveCreated;
	FText Status;
	/** How many points the last read took from the simulation rather than from their asset. */
	int32 LiveCount = 0;
	FDelegateHandle PlayStartedHandle;
	FDelegateHandle PlayEndedHandle;
	bool bQueryValid = false;
	bool bRefreshPending = false;
	/** An in-flight interactive edit: re-read positions, but leave the details panel alone. */
	bool bReadPending = false;
	bool bOwnChange = false;

	void ReadPoints();

	/** Re-reads the project's basis assets, broadcasting only when the set itself moved. */
	void ReadBasisAssets(IAssetRegistry& Registry);

	/** Brings created assets' registration back in line with the transacted creation record. */
	void SyncCreatedAssets();
	/** A play session started or ended, so the values behind every point may have just changed. */
	void PlaySessionChanged(bool bSimulating);

	/** Pushes a dragged entity's new authored point into any session already simulating it. */
	void PushDragToPlaySession() const;

	void PropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	void AssetChanged(const FAssetData& Asset);
	void AssetRenamed(const FAssetData& Asset, const FString& OldPath);
	void RegistryReady();
	void ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements);
};
