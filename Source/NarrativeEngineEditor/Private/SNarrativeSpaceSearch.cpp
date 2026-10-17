#include "SNarrativeSpaceSearch.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "NarrativeSpaceModel.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "NarrativeSpaceSearch"

namespace NarrativeSpaceSearchUI
{
	/** Rows the dropdown lists. Every match still counts toward dimming and Shift+Enter. */
	constexpr int32 MaxRows = 50;

	/** Fastest the results re-run while assets keep changing under an open search, in seconds. */
	constexpr double StaleRefreshInterval = 0.5;

	const FLinearColor DimTextColor(0.5f, 0.6f, 0.7f);
}

void SNarrativeSpaceSearch::Construct(const FArguments& Args)
{
	Model = Args._Model;
	OnPick = Args._OnPick;
	OnPickAll = Args._OnPickAll;
	OnMatchesChanged = Args._OnMatchesChanged;
	OnDismissed = Args._OnDismissed;

	SAssignNew(ResultsList, SListView<FResultPtr>)
		.ListItemsSource(&Results)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SNarrativeSpaceSearch::GenerateRow)
		.OnMouseButtonClick_Lambda([this](FResultPtr Result) { Pick(Result); })
		.OnMouseButtonDoubleClick_Lambda([this](FResultPtr Result) { OpenEditor(Result); });

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[
			SAssignNew(ResultsAnchor, SMenuAnchor)
			.Placement(MenuPlacement_ComboBox)
			.Method(EPopupMethod::UseCurrentWindow)
			.MenuContent(CreateResultsMenu())
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(LOCTEXT("Hint", "Search narrative assets: name, label, phrase, tags... (Ctrl+F)"))
				.ToolTipText(LOCTEXT("Tip", "Finds any narrative asset in the project by name, label, phrase or other text, gameplay tags, referenced assets, type or folder. Every word has to match somewhere.\nEnter: go to the highlighted asset, tagging its two most relevant axes | Shift+Enter: select every match on the current axes | Ctrl+Enter: open its editor | Up/Down: highlight | Esc: clear"))
				.SelectAllTextWhenFocused(true)
				.OnTextChanged(this, &SNarrativeSpaceSearch::TextChanged)
				.OnKeyDownHandler(this, &SNarrativeSpaceSearch::KeyDown)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
		[
			SNew(STextBlock)
			.Text(this, &SNarrativeSpaceSearch::MatchCountText)
			.ColorAndOpacity(NarrativeSpaceSearchUI::DimTextColor)
		]
	];
}

TSharedRef<SWidget> SNarrativeSpaceSearch::CreateResultsMenu()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(2)
		[
			SNew(SBox)
			.MinDesiredWidth(520)
			.MaxDesiredHeight(440)
			[
				ResultsList.ToSharedRef()
			]
		];
}

void SNarrativeSpaceSearch::Activate()
{
	FSlateApplication::Get().SetKeyboardFocus(SearchBox, EFocusCause::SetDirectly);
	if (!Results.IsEmpty()) { ResultsAnchor->SetIsOpen(true, false); }
}

void SNarrativeSpaceSearch::Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
	// An asset renamed, retagged or reworded under an open search should not leave stale rows behind.
	// A details-panel drag marks the index stale every frame, so this paces itself.
	if (IsSearching() && Index.IsStale() && CurrentTime - LastSearchTime >= NarrativeSpaceSearchUI::StaleRefreshInterval)
	{
		RunSearch();
	}
}

void SNarrativeSpaceSearch::TextChanged(const FText& Text)
{
	SearchText = Text.ToString().TrimStartAndEnd();
	RunSearch();
	ResultsAnchor->SetIsOpen(!Results.IsEmpty(), false);
}

void SNarrativeSpaceSearch::RunSearch()
{
	LastSearchTime = FSlateApplication::Get().GetCurrentTime();

	// Keep the highlighted asset highlighted across a re-run, so typing on does not lose the user's place.
	const FResultPtr Highlighted = ResultsList->GetSelectedItems().IsEmpty() ? nullptr : ResultsList->GetSelectedItems()[0];
	const TWeakObjectPtr<UNarrativeDataAsset> HighlightedAsset = Highlighted ? Highlighted->Asset : nullptr;

	Results.Reset();
	Matches.Reset();
	const TArray<FNarrativeSearchResult> Found = SearchText.IsEmpty() ? TArray<FNarrativeSearchResult>() : Index.Find(SearchText, MAX_int32);
	MatchCount = Found.Num();
	for (const FNarrativeSearchResult& Result : Found)
	{
		Matches.Add(Result.Asset);
		if (Results.Num() < NarrativeSpaceSearchUI::MaxRows) { Results.Add(MakeShared<FNarrativeSearchResult>(Result)); }
	}

	ResultsList->RequestListRefresh();
	ResultsList->ClearSelection();
	const FResultPtr* Keep = Results.FindByPredicate([&HighlightedAsset](const FResultPtr& Result)
	{
		return HighlightedAsset.IsValid() && Result->Asset == HighlightedAsset;
	});
	if (Keep) { ResultsList->SetSelection(*Keep, ESelectInfo::Direct); }
	else if (!Results.IsEmpty()) { ResultsList->SetSelection(Results[0], ESelectInfo::Direct); }

	if (Results.IsEmpty()) { ResultsAnchor->SetIsOpen(false); }
	OnMatchesChanged.ExecuteIfBound();
}

FReply SNarrativeSpaceSearch::KeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		if (!ResultsAnchor->IsOpen() && !Results.IsEmpty()) { ResultsAnchor->SetIsOpen(true, false); }
		else { MoveHighlight(Key == EKeys::Down ? 1 : -1); }
		return FReply::Handled();
	}
	if (Key == EKeys::Enter)
	{
		if (Event.IsShiftDown()) { PickAll(); }
		else if (Event.IsControlDown()) { OpenEditor(ChosenResult()); }
		else { Pick(ChosenResult()); }
		return FReply::Handled();
	}
	if (Key == EKeys::Escape)
	{
		// The first Escape clears the search and stops the dimming; either way focus returns to the space.
		ResultsAnchor->SetIsOpen(false);
		if (!SearchText.IsEmpty())
		{
			SearchBox->SetText(FText::GetEmpty());
			// Whether or not SetText announced itself, the matches and the dimming have to go.
			TextChanged(FText::GetEmpty());
		}
		OnDismissed.ExecuteIfBound();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SNarrativeSpaceSearch::MoveHighlight(const int32 Step)
{
	if (Results.IsEmpty()) { return; }
	const TArray<FResultPtr> Selected = ResultsList->GetSelectedItems();
	const int32 Current = Selected.IsEmpty() ? INDEX_NONE : Results.IndexOfByKey(Selected[0]);
	const int32 Next = Current == INDEX_NONE
		? (Step > 0 ? 0 : Results.Num() - 1)
		: ((Current + Step) % Results.Num() + Results.Num()) % Results.Num();
	ResultsList->SetSelection(Results[Next], ESelectInfo::OnNavigation);
	ResultsList->RequestScrollIntoView(Results[Next]);
}

SNarrativeSpaceSearch::FResultPtr SNarrativeSpaceSearch::ChosenResult() const
{
	const TArray<FResultPtr> Selected = ResultsList->GetSelectedItems();
	if (!Selected.IsEmpty()) { return Selected[0]; }
	return Results.IsEmpty() ? nullptr : Results[0];
}

void SNarrativeSpaceSearch::Pick(const FResultPtr Result)
{
	UNarrativeDataAsset* Asset = Result ? Result->Asset.Get() : nullptr;
	if (!Asset) { return; }
	ResultsAnchor->SetIsOpen(false);
	OnPick.ExecuteIfBound(Asset);
}

void SNarrativeSpaceSearch::PickAll()
{
	TArray<UNarrativeDataAsset*> Assets;
	for (const TWeakObjectPtr<UNarrativeDataAsset>& Match : Matches)
	{
		if (UNarrativeDataAsset* Asset = Match.Get()) { Assets.Add(Asset); }
	}
	if (Assets.IsEmpty()) { return; }
	ResultsAnchor->SetIsOpen(false);
	OnPickAll.ExecuteIfBound(Assets);
}

void SNarrativeSpaceSearch::OpenEditor(const FResultPtr Result)
{
	UNarrativeDataAsset* Asset = Result ? Result->Asset.Get() : nullptr;
	if (!Asset || !GEditor) { return; }
	ResultsAnchor->SetIsOpen(false);
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
}

bool SNarrativeSpaceSearch::IsPlaceable(const UNarrativeDataAsset* Asset) const
{
	if (!Asset) { return false; }
	// A basis asset is an axis of the space rather than a point in it; going to one tags it.
	if (Asset->IsA<UNarrativeBasisVector>()) { return true; }
	return NarrativeSpaceSearch::PlacementVector(*Asset, Model->GetQuery().PlacementField) != nullptr;
}

TSharedRef<ITableRow> SNarrativeSpaceSearch::GenerateRow(const FResultPtr Result, const TSharedRef<STableViewBase>& Owner)
{
	const UNarrativeDataAsset* Asset = Result->Asset.Get();
	const bool bPlaceable = IsPlaceable(Asset);

	// Second line: why this asset matched, or what it says when its name alone was enough.
	FText Detail;
	FString Highlight;
	if (!Result->Field.IsEmpty())
	{
		Detail = FText::Format(INVTEXT("{0}: {1}"), Result->Field, FText::FromString(Result->Snippet));
		Highlight = Result->Term;
	}
	else if (Asset && Asset->GetSpaceLabel().ToString() != Result->Name)
	{
		Detail = Asset->GetSpaceLabel();
	}
	if (!bPlaceable)
	{
		const FText NoPosition = LOCTEXT("NoPosition", "No position in the narrative space; Ctrl+Enter opens its editor");
		Detail = Detail.IsEmpty() ? NoPosition : FText::Format(INVTEXT("{0}  ({1})"), Detail, NoPosition);
	}

	TSharedRef<SVerticalBox> Text = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Result->Name))
				.HighlightText(FText::FromString(SearchText.Contains(TEXT(" ")) ? Highlight : SearchText))
				.ColorAndOpacity(bPlaceable ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(Result->Type)
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(NarrativeSpaceSearchUI::DimTextColor)
			]
		];
	if (!Detail.IsEmpty())
	{
		Text->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(Detail)
			.HighlightText(FText::FromString(Highlight))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(NarrativeSpaceSearchUI::DimTextColor)
		];
	}

	return SNew(STableRow<FResultPtr>, Owner)
		.Padding(FMargin(4, 3))
		.ToolTipText(Asset ? FText::FromString(Asset->GetPathName()) : FText::GetEmpty())
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[
				// The same colour the viewport draws this type's cards in.
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Asset ? FNarrativeSpaceModel::ClassColor(Asset->GetClass()) : FLinearColor::Gray)
				.Padding(FMargin(4, 9))
			]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[
				Text
			]
		];
}

FText SNarrativeSpaceSearch::MatchCountText() const
{
	if (!IsSearching()) { return FText::GetEmpty(); }
	if (MatchCount == 0) { return LOCTEXT("NoMatches", "No matches"); }
	return MatchCount > Results.Num()
		? FText::Format(LOCTEXT("SomeMatches", "{0} matches, best {1} listed"), FText::AsNumber(MatchCount), FText::AsNumber(Results.Num()))
		: FText::Format(LOCTEXT("Matches", "{0} {0}|plural(one=match,other=matches)"), MatchCount);
}

#undef LOCTEXT_NAMESPACE
