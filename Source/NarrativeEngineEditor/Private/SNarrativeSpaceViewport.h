#pragma once

#include "NarrativeSpaceModel.h"
#include "Widgets/SLeafWidget.h"

class FMenuBuilder;
class IMenu;
class SEditableTextBox;

/** One paint/input surface, with no per-asset widget tree. */
class SNarrativeSpaceViewport : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeSpaceViewport) {}
		SLATE_ARGUMENT(TSharedPtr<FNarrativeSpaceModel>, Model)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual ~SNarrativeSpaceViewport() override;

	/** Fits the camera around every plotted asset, or around the selection only. */
	void FrameAll(bool bSelectionOnly = false);

	/** Switches the view direction. Queries with fewer than three axes stay on XY. */
	void SetView(ENarrativeSpaceView View);

	/** Restores the default camera and reframes once the widget has been given a size. */
	void ResetCamera();

	/**
	 * Moves the selection one entry along reading order: down the current column, then on to the
	 * top of the next column to the right. Step of -1 walks back. Pans if the entry is off screen.
	 */
	void SelectNext(int32 Step);

	/** Chooses how entries that leave one of the plotted axes undefined are drawn. */
	void SetIncompleteDisplay(ENarrativeSpaceIncomplete Mode);

	/** Opens the rename box over the selection. Does nothing without a selection this tool can edit. */
	void BeginRename();

	//~ Begin SWidget interface
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(800, 600); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& OutElements, int32 LayerId, const FWidgetStyle& Style,
		bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply OnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override;
	virtual FReply OnDragOver(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	virtual FReply OnDrop(const FGeometry& Geometry, const FDragDropEvent& Event) override;
	//~ End SWidget interface

private:
	/** One drawn card. Several points collapse into a single item once cards are too small to read. */
	struct FVisibleItem
	{
		FVector2D Screen;
		FVector2D HalfSize;
		TArray<int32> Indices;
		double Depth = 0.0;
		/** True only when every point in this item is missing one of the plotted axes. */
		bool bIncomplete = false;
	};

	TSharedPtr<FNarrativeSpaceModel> Model;
	FNarrativeSpaceCamera Camera;
	FVector2D ViewportSize = FVector2D::ZeroVector;

	// Interaction state, all in local widget space.
	FVector2D MouseDown = FVector2D::ZeroVector;
	FVector2D LastMouse = FVector2D::ZeroVector;
	FVector2D MarqueeEnd = FVector2D::ZeroVector;
	bool bPan = false;
	bool bOrbit = false;
	bool bPendingDrag = false;
	bool bMarquee = false;
	bool bAddSelection = false;
	int32 AxisLock = INDEX_NONE;
	bool bFramePending = false;
	ENarrativeSpaceIncomplete IncompleteDisplay = ENarrativeSpaceIncomplete::Hide;

	// The rename box, while one is open, and the menu holding it.
	TSharedPtr<SEditableTextBox> RenameBox;
	TWeakPtr<IMenu> RenameMenu;

	/** Card edge length in pixels at the current zoom; drives every level-of-detail threshold. */
	double CardSize() const;

	/** Card width on screen, which is also the column width Tab walks down. */
	double ColumnWidth() const;

	/** True when the asset leaves one of the plotted axes undefined. */
	bool IsIncomplete(const FNarrativeSpacePoint& Point) const;

	/** True when the point is kept out of the viewport, framing and hit testing entirely. */
	bool IsHidden(const FNarrativeSpacePoint& Point) const;

	/** Projects and culls the model's points, back to front, clustering them when cards get small. */
	TArray<FVisibleItem> VisibleItems(const FVector2D& Size) const;

	/** Point indices under a local-space position, topmost first. Empty when nothing is hit. */
	TArray<int32> Hit(const FVector2D& At, const FVector2D& Size) const;

	void ApplyDrag(const FVector2D& At, bool bSnap);

	/** Opens the viewport context menu for a right click that did not turn into a pan. */
	void ShowContextMenu(const FVector2D& At, const FVector2D& ScreenPosition);

	/** Fills the Make New submenu with the classes this query can plot. */
	void BuildMakeNewMenu(FMenuBuilder& Builder, FVector2D At);

	/** Creates one asset of Class at the local-space point the context menu was opened on. */
	void CreateAssetAt(TWeakObjectPtr<UClass> Class, FVector2D At);

	/** Deletes the selected assets outright, through the editor's confirmation dialog. */
	void DeleteSelection();

	/** The selection as data assets, in plot order, which is also the order a rename numbers them in. */
	TArray<UNarrativeDataAsset*> SelectedAssets() const;

	/** Renames the selection to the committed text, then closes the box. Anything else just closes it. */
	void CommitRename(const FText& Text, ETextCommit::Type Commit);

	/** Keeps the rename box from committing a name the selection cannot take. */
	bool VerifyRename(const FText& Text, FText& OutError) const;

	/** Closes the rename box, if one is open. */
	void EndRename();

	/** Lets go of a rename box the menu stack closed on its own, so F2 can open the next one. */
	void RenameDismissed(TSharedRef<IMenu> Menu);

	/** Ends any drag, pan, orbit or marquee and clears the axis lock. */
	void FinishInteraction(bool bCancel);
};
