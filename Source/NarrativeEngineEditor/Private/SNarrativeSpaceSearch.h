#pragma once

#include "CoreMinimal.h"
#include "NarrativeSpaceSearch.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FNarrativeSpaceModel;
class ITableRow;
class SMenuAnchor;
class SSearchBox;
class STableViewBase;

DECLARE_DELEGATE_OneParam(FOnNarrativeSearchPick, UNarrativeDataAsset*);
DECLARE_DELEGATE_OneParam(FOnNarrativeSearchPickAll, const TArray<UNarrativeDataAsset*>&);

/**
 * The search bar over the narrative space: finds any narrative asset in the project by name, label,
 * text, tags or references, and lists the matches in a dropdown under the box. Picking one is left to
 * the owner, which takes the user to it in the space rather than to the Content Browser.
 */
class SNarrativeSpaceSearch : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNarrativeSpaceSearch) {}
		SLATE_ARGUMENT(TSharedPtr<FNarrativeSpaceModel>, Model)
		/** Enter or a click on a row: go to that one asset. */
		SLATE_EVENT(FOnNarrativeSearchPick, OnPick)
		/** Shift+Enter: every match the current query plots. */
		SLATE_EVENT(FOnNarrativeSearchPickAll, OnPickAll)
		/** The set of matching assets moved, including to nothing when the search is cleared. */
		SLATE_EVENT(FSimpleDelegate, OnMatchesChanged)
		/** Escape: the search is over, and focus belongs back with the viewport. */
		SLATE_EVENT(FSimpleDelegate, OnDismissed)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	/** Ctrl+F: focuses the box with its text selected, and reopens the results for any text it holds. */
	void Activate();

	/** True while the box holds a query, which is when the viewport dims everything it does not match. */
	bool IsSearching() const { return !SearchText.IsEmpty(); }

	/** Every asset the current text matches, not just the rows the dropdown has room for. */
	const TSet<TWeakObjectPtr<UNarrativeDataAsset>>& GetMatches() const { return Matches; }

	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;

private:
	using FResultPtr = TSharedPtr<FNarrativeSearchResult>;

	FNarrativeSpaceSearchIndex Index;
	TSharedPtr<FNarrativeSpaceModel> Model;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SMenuAnchor> ResultsAnchor;
	TSharedPtr<SListView<FResultPtr>> ResultsList;
	/** The rows on show: the best matches, capped so the dropdown stays responsive. */
	TArray<FResultPtr> Results;
	TSet<TWeakObjectPtr<UNarrativeDataAsset>> Matches;
	int32 MatchCount = 0;
	FString SearchText;
	/** When the results were last worked out, so a stream of edits re-runs the search at a steady pace. */
	double LastSearchTime = 0.0;

	FOnNarrativeSearchPick OnPick;
	FOnNarrativeSearchPickAll OnPickAll;
	FSimpleDelegate OnMatchesChanged;
	FSimpleDelegate OnDismissed;

	void TextChanged(const FText& Text);
	void RunSearch();
	FReply KeyDown(const FGeometry& Geometry, const FKeyEvent& Event);
	TSharedRef<ITableRow> GenerateRow(FResultPtr Result, const TSharedRef<STableViewBase>& Owner);
	TSharedRef<SWidget> CreateResultsMenu();

	/** Moves the highlighted row, wrapping at either end. */
	void MoveHighlight(int32 Step);

	/** The highlighted row, or the best one when nothing is highlighted. */
	FResultPtr ChosenResult() const;

	void Pick(FResultPtr Result);
	void PickAll();

	/** Ctrl+Enter or a double click: the asset's own editor, for assets the space cannot show. */
	void OpenEditor(FResultPtr Result);

	/** True when the asset has a position the space can take the user to. */
	bool IsPlaceable(const UNarrativeDataAsset* Asset) const;

	FText MatchCountText() const;
};
