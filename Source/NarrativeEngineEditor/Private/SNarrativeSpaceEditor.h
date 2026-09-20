#pragma once

#include "CoreMinimal.h"
#include "NarrativeSpaceQuery.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

class FNarrativeSpaceModel;
class IDetailsView;
class SNarrativeSpaceViewport;

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

private:
	TStrongObjectPtr<UNarrativeSpaceSettings> Settings;
	TSharedPtr<FNarrativeSpaceModel> Model;
	TSharedPtr<SNarrativeSpaceViewport> Viewport;
	TSharedPtr<IDetailsView> QueryDetails;
	TSharedPtr<IDetailsView> AssetDetails;
	TSharedPtr<SComboBox<TSharedPtr<FName>>> FieldPicker;
	TArray<TSharedPtr<FName>> FieldOptions;

	void CreateDetailViews();
	void CreateFieldPicker();
	TSharedRef<SWidget> CreateToolbar();

	/** Rebuilds the picker from the fields the current query exposes, keeping the chosen field visible. */
	void RefreshFieldOptions();
	void QueryChanged(const FPropertyChangedEvent& Event);
	void SelectionChanged();
};
