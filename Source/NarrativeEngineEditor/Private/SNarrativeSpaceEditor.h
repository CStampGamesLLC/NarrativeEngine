#pragma once

#include "CoreMinimal.h"
// Not just the query: the toolbar's mode toggle is declared in terms of ENarrativeSpaceSource.
#include "NarrativeSpaceModel.h"
#include "NarrativeSpaceQuery.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Views/SListView.h"

class IDetailsView;
class ITableRow;
class SNarrativeSpaceSearch;
class SNarrativeSpaceViewport;
class STableViewBase;

/** Toolbar, query details, plot viewport and selection details for a single narrative space. */
class SNarrativeSpaceEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeSpaceEditor) {}
		SLATE_ARGUMENT(TOptional<FNarrativeSpaceQuery>, InitialQuery)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	virtual ~SNarrativeSpaceEditor() override;

	TSharedPtr<FNarrativeSpaceModel> GetModel() const { return Model; }

	/** Plots and drags the named FVectorND field. None restores each asset class's default field. */
	void SetPlacementField(FName Field);

	/**
	 * Tags an untagged basis asset as the next free axis, or untags a tagged one. Untagging closes
	 * the gap, so dropping Y leaves the old Z as the new Y. Ignored once three axes are tagged.
	 */
	void ToggleAxis(const TSoftObjectPtr<UNarrativeBasisVector>& Axis);

	/**
	 * Takes the user to an asset in the space: tags its two most relevant axes (see
	 * NarrativeSpaceSearch::RelevantAxes), widens any class, path or field filter that would leave it
	 * out, then selects and centres it. A basis asset is an axis rather than a point, so going to one
	 * tags it as X, keeping the previous X as Y. Returns false, changing nothing, for an asset with no
	 * position in the space.
	 */
	bool GoToAsset(UNarrativeDataAsset* Asset);

	/** Selects whichever of these assets the current axes plot, and frames them. Returns how many. */
	int32 ShowAssets(const TArray<UNarrativeDataAsset*>& Assets);

	/** Ctrl+F: puts focus in the search bar. */
	void FocusSearch();

	//~ Begin SWidget interface
	virtual FReply OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	//~ End SWidget interface

private:
	/** One row of the axis list: a basis asset in the project, tagged as an axis or not. */
	using FAxisOption = TSharedPtr<TSoftObjectPtr<UNarrativeBasisVector>>;

	TStrongObjectPtr<UNarrativeSpaceSettings> Settings;
	TSharedPtr<FNarrativeSpaceModel> Model;
	TSharedPtr<SNarrativeSpaceViewport> Viewport;
	TSharedPtr<SNarrativeSpaceSearch> Search;
	TSharedPtr<IDetailsView> QueryDetails;
	TSharedPtr<IDetailsView> AssetDetails;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> FieldPicker;
	TArray<TSharedPtr<FName>> FieldOptions;
	TSharedPtr<SListView<FAxisOption>> AxisList;
	TArray<FAxisOption> AxisOptions;
	/** What AssetDetails is currently bound to, so an unchanged selection never rebuilds it. */
	TArray<TWeakObjectPtr<UObject>> DisplayedSelection;

	void CreateDetailViews();
	void CreateSearch();

	/** Tells the user why the search could not take them somewhere. */
	static void Notify(const FText& Message);
	void CreateFieldPicker();
	void CreateAxisList();
	TSharedRef<SWidget> CreateToolbar();

	/**
	 * The editor's own play controls, so a session can be started, paused and stopped without
	 * leaving the tool. Same commands and same buttons the level and blueprint editors put up.
	 */
	static TSharedRef<SWidget> CreatePlayControls();

	/** One half of the Static/Runtime pair: a toggle that reads the model's current source. */
	TSharedRef<SWidget> CreateSourceButton(ENarrativeSpaceSource InSource, const FText& Label, const FText& Tooltip);

	/** Rebuilds the axis list from the project's basis assets, plus any tagged axis they no longer hold. */
	void RefreshAxisOptions();

	TSharedRef<ITableRow> GenerateAxisRow(FAxisOption Option, const TSharedRef<STableViewBase>& Owner);

	/** The axis this basis asset is tagged as, or INDEX_NONE when it is not part of the query. */
	int32 AxisIndex(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const;

	/** The asset's path, and what clicking its entry will do to the query. */
	FText AxisTooltip(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const;

	/** Pushes the edited query to the model, reframing only when the axes themselves moved. */
	void ApplyQuery();

	/** Rebuilds the picker from the fields the current query exposes, keeping the chosen field visible. */
	void RefreshFieldOptions();
	void QueryChanged(const FPropertyChangedEvent& Event);
	void SelectionChanged();
};
