#include "SNarrativeSpaceEditor.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IDetailsView.h"
#include "Misc/PackageName.h"
#include "NarrativeSpaceModel.h"
#include "NarrativeSpaceSearch.h"
#include "PropertyEditorModule.h"
#include "SNarrativeSpaceSearch.h"
#include "SNarrativeSpaceViewport.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Kismet2/DebuggerCommands.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "NarrativeSpaceEditor"

/** Display name for a placement field. None means whatever field the asset class defaults to. */
static FText PlacementFieldText(const FName Field)
{
	return Field.IsNone() ? LOCTEXT("ClassDefault", "Class default") : FText::FromName(Field);
}

void SNarrativeSpaceEditor::Construct(const FArguments& Args)
{
	Settings.Reset(NewObject<UNarrativeSpaceSettings>());
	if (Args._InitialQuery.IsSet()) { Settings->Query = Args._InitialQuery.GetValue(); }

	Model = MakeShared<FNarrativeSpaceModel>();
	Model->SetQuery(Settings->Query);

	CreateDetailViews();
	Model->OnSelectionChanged.AddSP(this, &SNarrativeSpaceEditor::SelectionChanged);

	SAssignNew(Viewport, SNarrativeSpaceViewport).Model(Model);
	Viewport->SetIncompleteDisplay(Settings->Incomplete);
	Viewport->ResetCamera();

	CreateFieldPicker();
	RefreshFieldOptions();

	Model->OnBasisAssetsChanged.AddSP(this, &SNarrativeSpaceEditor::RefreshAxisOptions);
	CreateAxisList();
	RefreshAxisOptions();
	CreateSearch();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 6, 8, 0)
		[
			Search.ToSharedRef()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			CreateToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.20f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 2)
				[
					SNew(STextBlock).Text(LOCTEXT("Axes", "Axes - click to tag X, Y, Z in order"))
				]
				+ SVerticalBox::Slot().FillHeight(0.45f).Padding(10, 2, 10, 8)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						AxisList.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 2)
				[
					SNew(STextBlock).Text(LOCTEXT("PlacementField", "Placement field (plot + drag)"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 2, 10, 8)
				[
					FieldPicker.ToSharedRef()
				]
				+ SVerticalBox::Slot().FillHeight(0.55f)
				[
					QueryDetails.ToSharedRef()
				]
			]
			+ SSplitter::Slot().Value(0.50f)
			[
				Viewport.ToSharedRef()
			]
			+ SSplitter::Slot().Value(0.30f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(10)
				[
					SNew(STextBlock).Text(LOCTEXT("Selection", "Selected assets"))
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					AssetDetails.ToSharedRef()
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
		[
			SNew(STextBlock).Text_Lambda([this]
			{
				return FText::Format(INVTEXT("{0}  |  {1}"), Model->GetStatus(), Model->GetSourceStatus());
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("Help", "Ctrl+F: search (Enter: go to the asset on its two most relevant axes) | Drag: move | Shift-click / marquee: select | Tab / Shift-Tab: step through entries | Hold X/Y/Z: lock | Ctrl: snap | Esc: cancel | RMB drag / MMB: pan | RMB click: menu | Wheel: zoom | Alt+RMB: orbit | F/Home: frame | Double-click: open | F2: rename assets | Delete: delete assets | Ctrl+Z/Y: undo/redo. Static values stay editable during play and are pushed into the running simulation; runtime positions belong to the simulation and are read only. Creating, renaming and deleting assets wait until play ends."))
		]
	];
}

SNarrativeSpaceEditor::~SNarrativeSpaceEditor()
{
	Model->OnSelectionChanged.RemoveAll(this);
	Model->OnBasisAssetsChanged.RemoveAll(this);
	QueryDetails->OnFinishedChangingProperties().RemoveAll(this);
	Model->EndDrag(true);
}

void SNarrativeSpaceEditor::CreateDetailViews()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.bLockable = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	QueryDetails = PropertyEditor.CreateDetailView(DetailsArgs);
	QueryDetails->SetObject(Settings.Get());
	QueryDetails->OnFinishedChangingProperties().AddSP(this, &SNarrativeSpaceEditor::QueryChanged);

	DetailsArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
	AssetDetails = PropertyEditor.CreateDetailView(DetailsArgs);
	// Asset values belong to the model mid-drag. Play does not lock them: editing one here while a
	// session runs is the point of runtime mode.
	AssetDetails->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda(
		[this] { return Model->CanEdit() && !Model->IsDragging(); }));
}

void SNarrativeSpaceEditor::CreateSearch()
{
	auto FocusViewport = [this] { FSlateApplication::Get().SetKeyboardFocus(Viewport, EFocusCause::SetDirectly); };

	SAssignNew(Search, SNarrativeSpaceSearch)
		.Model(Model)
		.OnPick_Lambda([this, FocusViewport](UNarrativeDataAsset* Asset)
		{
			// Into the space rather than the Content Browser, and the viewport keeps the keyboard so
			// F2, Delete and the rest act on what was just found.
			if (GoToAsset(Asset)) { FocusViewport(); }
		})
		.OnPickAll_Lambda([this, FocusViewport](const TArray<UNarrativeDataAsset*>& Assets)
		{
			if (ShowAssets(Assets) > 0) { FocusViewport(); return; }
			Notify(FText::Format(LOCTEXT("NoneShown", "None of the {0} matches are plotted on the current axes. Press Enter on one to tag its axes."),
				FText::AsNumber(Assets.Num())));
		})
		.OnMatchesChanged_Lambda([this]
		{
			Viewport->SetSearchMatches(Search->IsSearching()
				? TOptional<TSet<TWeakObjectPtr<UNarrativeDataAsset>>>(Search->GetMatches())
				: TOptional<TSet<TWeakObjectPtr<UNarrativeDataAsset>>>());
		})
		.OnDismissed_Lambda(FocusViewport);
}

void SNarrativeSpaceEditor::FocusSearch()
{
	Search->Activate();
}

FReply SNarrativeSpaceEditor::OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Previewed rather than bubbled so it works wherever focus sits in the tool, the viewport included.
	if (Event.GetKey() == EKeys::F && Event.IsControlDown() && !Event.IsAltDown() && !Event.IsShiftDown())
	{
		FocusSearch();
		return FReply::Handled();
	}
	return SCompoundWidget::OnPreviewKeyDown(Geometry, Event);
}

void SNarrativeSpaceEditor::Notify(const FText& Message)
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = 5.f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

bool SNarrativeSpaceEditor::GoToAsset(UNarrativeDataAsset* Asset)
{
	if (!Asset) { return false; }
	FNarrativeSpaceQuery& Query = Settings->Query;

	FName Field;
	const FVectorND* Vector = NarrativeSpaceSearch::PlacementVector(*Asset, Query.PlacementField, &Field);
	const TArray<TSoftObjectPtr<UNarrativeBasisVector>> Axes = Vector
		? NarrativeSpaceSearch::RelevantAxes(*Vector, NarrativeSpaceSearch::SpaceMask(*Asset), 2)
		: TArray<TSoftObjectPtr<UNarrativeBasisVector>>();

	if (UNarrativeBasisVector* Basis = Cast<UNarrativeBasisVector>(Asset); Basis && Axes.IsEmpty())
	{
		// An axis rather than a point. Tag it as X and keep the old X, or the old Y when it already
		// was X, so there is still a plane to look at; the axis list shows where it landed.
		TArray<TSoftObjectPtr<UNarrativeBasisVector>> NewAxes = {Basis};
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Query.Axes)
		{
			if (Axis != NewAxes[0]) { NewAxes.Add(Axis); break; }
		}
		Query.Axes = MoveTemp(NewAxes);
		ApplyQuery();
		Model->Select({});
		return true;
	}
	if (Axes.IsEmpty())
	{
		Notify(FText::Format(LOCTEXT("NoPosition", "{0} has no position in the narrative space to go to. Ctrl+Enter in the search bar opens its editor."),
			FText::FromString(Asset->GetName())));
		return false;
	}

	// Widen whatever in the query would leave the asset out, and nothing more. An explicit placement
	// field the asset lacks gives way to each class's default, which is the vector the axes came from.
	if (Field != Query.PlacementField) { Query.PlacementField = NAME_None; }
	const bool bClassMatches = Query.Classes.IsEmpty() || Query.Classes.ContainsByPredicate(
		[Asset](const TSubclassOf<UNarrativeDataAsset>& Class) { return Class && Asset->IsA(Class); });
	if (!bClassMatches) { Query.Classes.Add(Asset->GetClass()); }

	const FString PackageName = Asset->GetPackage()->GetName();
	FString Root = Query.ContentPath.IsEmpty() ? TEXT("/Game") : Query.ContentPath;
	Root.RemoveFromEnd(TEXT("/"));
	if (!PackageName.StartsWith(Root + TEXT("/")))
	{
		// Out to the asset's mount point: /Game, or the plugin it lives in.
		Query.ContentPath = TEXT("/") + FPackageName::GetPackageMountPoint(PackageName).ToString();
	}

	Query.Axes = Axes;
	QueryDetails->ForceRefresh();
	ApplyQuery();
	RefreshFieldOptions();

	Model->Select({Asset});
	Viewport->FocusSelection();
	return Model->IsSelected(Asset);
}

int32 SNarrativeSpaceEditor::ShowAssets(const TArray<UNarrativeDataAsset*>& Assets)
{
	// Only what the current axes actually draw, so the axes stay as they are and nothing hidden for
	// missing one of them is selected out of sight.
	const uint8 AllAxes = uint8((1 << Model->GetQuery().Axes.Num()) - 1);
	TArray<UNarrativeDataAsset*> Shown;
	for (const FNarrativeSpacePoint& Point : Model->GetPoints())
	{
		UNarrativeDataAsset* Asset = Point.Asset.Get();
		const bool bHidden = Settings->Incomplete == ENarrativeSpaceIncomplete::Hide && Point.PresentAxes != AllAxes;
		if (Asset && !bHidden && Assets.Contains(Asset)) { Shown.Add(Asset); }
	}
	Model->Select(Shown);
	if (!Shown.IsEmpty()) { Viewport->FocusSelection(); }
	return Shown.Num();
}

void SNarrativeSpaceEditor::CreateFieldPicker()
{
	SAssignNew(FieldPicker, SComboBox<TSharedPtr<FName>>)
		.OptionsSource(&FieldOptions)
		.OnComboBoxOpening(this, &SNarrativeSpaceEditor::RefreshFieldOptions)
		.ToolTipText(LOCTEXT("FieldTip", "Choose the FVectorND field to plot and drag. Includes inherited fields. Assets without this field are omitted; no other field is modified. Class default uses each asset type's default."))
		.OnGenerateWidget_Lambda([](TSharedPtr<FName> Field)
		{
			return SNew(STextBlock).Text(PlacementFieldText(*Field));
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<FName> Field, ESelectInfo::Type Reason)
		{
			if (Field.IsValid() && Reason != ESelectInfo::Direct) { SetPlacementField(*Field); }
		})
		[
			SNew(STextBlock).Text_Lambda([this] { return PlacementFieldText(Settings->Query.PlacementField); })
		];
}

void SNarrativeSpaceEditor::CreateAxisList()
{
	// Selection here is the query's own X/Y/Z tagging, drawn by each row, so the list keeps none of its own.
	SAssignNew(AxisList, SListView<FAxisOption>)
		.ListItemsSource(&AxisOptions)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SNarrativeSpaceEditor::GenerateAxisRow);
}

TSharedRef<ITableRow> SNarrativeSpaceEditor::GenerateAxisRow(FAxisOption Option, const TSharedRef<STableViewBase>& Owner)
{
	const TSoftObjectPtr<UNarrativeBasisVector> Axis = Option.IsValid() ? *Option : nullptr;
	return SNew(STableRow<FAxisOption>, Owner)
		.Padding(FMargin(2, 1))
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0))
			.IsEnabled_Lambda([this, Axis] { return AxisIndex(Axis) != INDEX_NONE || Settings->Query.Axes.Num() < 3; })
			.ToolTipText_Lambda([this, Axis] { return AxisTooltip(Axis); })
			.OnClicked_Lambda([this, Axis] { ToggleAxis(Axis); return FReply::Handled(); })
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this, Axis]
				{
					// A tagged entry carries its own axis colour from the viewport, dimmed enough to
					// read a label on. Untagged entries stay the colour of the viewport's cards.
					const int32 Index = AxisIndex(Axis);
					return Index == INDEX_NONE
						? FLinearColor(0.05f, 0.06f, 0.075f)
						: (NarrativeSpaceAxis::Color(Index) * 0.45f).CopyWithNewOpacity(1.f);
				})
				.Padding(FMargin(6, 3))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(14.f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, Axis]
							{
								const int32 Index = AxisIndex(Axis);
								return Index == INDEX_NONE ? FText::GetEmpty() : FText::FromString(NarrativeSpaceAxis::Label(Index));
							})
							.ColorAndOpacity_Lambda([this, Axis]
							{
								return FSlateColor(NarrativeSpaceAxis::Color(FMath::Max(0, AxisIndex(Axis))));
							})
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(Axis.GetAssetName()))
					]
				]
			]
		];
}

FText SNarrativeSpaceEditor::AxisTooltip(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const
{
	const int32 Index = AxisIndex(Axis);
	const FText Action = Index != INDEX_NONE
		? FText::Format(LOCTEXT("AxisUntag", "Tagged {0}. Click to untag it; anything tagged after it moves up."),
			FText::FromString(NarrativeSpaceAxis::Label(Index)))
		: Settings->Query.Axes.Num() >= 3
			? LOCTEXT("AxisFull", "Three axes are already tagged. Untag one to plot this basis instead.")
			: FText::Format(LOCTEXT("AxisTag", "Click to tag as {0}."),
				FText::FromString(NarrativeSpaceAxis::Label(Settings->Query.Axes.Num())));
	return FText::Format(INVTEXT("{0}\n{1}"), FText::FromString(Axis.ToString()), Action);
}

void SNarrativeSpaceEditor::RefreshAxisOptions()
{
	AxisOptions.Reset();
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Basis : Model->GetBasisAssets())
	{
		AxisOptions.Add(MakeShared<TSoftObjectPtr<UNarrativeBasisVector>>(Basis));
	}
	// A tagged axis the project no longer holds still belongs here: this list is the only way to untag it.
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Settings->Query.Axes)
	{
		if (!Axis.IsNull() && !Model->GetBasisAssets().Contains(Axis))
		{
			AxisOptions.Add(MakeShared<TSoftObjectPtr<UNarrativeBasisVector>>(Axis));
		}
	}
	AxisList->RequestListRefresh();
}

int32 SNarrativeSpaceEditor::AxisIndex(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const
{
	return Settings->Query.Axes.IndexOfByKey(Axis);
}

void SNarrativeSpaceEditor::ToggleAxis(const TSoftObjectPtr<UNarrativeBasisVector>& Axis)
{
	if (Axis.IsNull()) { return; }
	TArray<TSoftObjectPtr<UNarrativeBasisVector>>& Axes = Settings->Query.Axes;
	const int32 Index = AxisIndex(Axis);
	if (Index != INDEX_NONE)
	{
		// Untagging closes the gap it leaves, so dropping Y leaves whatever was Z as the new Y.
		Axes.RemoveAt(Index);
	}
	else
	{
		// Three axes is all a query can plot; a fourth entry stays untagged until one is freed.
		if (Axes.Num() >= 3) { return; }
		Axes.Add(Axis);
	}
	ApplyQuery();
}

TSharedRef<SWidget> SNarrativeSpaceEditor::CreatePlayControls()
{
	// The commands are bound once by the level editor at startup; without them there is nothing to
	// drive the buttons, so put up no controls rather than dead ones.
	if (!FPlayWorldCommands::GlobalPlayWorldActions.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	static const FName ToolbarName("NarrativeSpace.PlayToolBar");
	if (!UToolMenus::Get()->IsMenuRegistered(ToolbarName))
	{
		UToolMenu* Toolbar = UToolMenus::Get()->RegisterMenu(ToolbarName, NAME_None, EMultiBoxType::SlimHorizontalToolBar);
		FPlayWorldCommands::BuildToolbar(Toolbar->AddSection("Play"));
	}
	return UToolMenus::Get()->GenerateWidget(ToolbarName, FToolMenuContext(FPlayWorldCommands::GlobalPlayWorldActions));
}

TSharedRef<SWidget> SNarrativeSpaceEditor::CreateSourceButton(const ENarrativeSpaceSource InSource, const FText& Label, const FText& Tooltip)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
		.Padding(FMargin(10, 2))
		.ToolTipText(Tooltip)
		.IsChecked_Lambda([this, InSource]
		{
			return Model->GetSource() == InSource ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this, InSource](ECheckBoxState)
		{
			// Clicking the active half is a no-op rather than a way to leave neither selected.
			Model->SetSource(InSource);
		})
		[
			SNew(STextBlock).Text(Label)
		];
}

TSharedRef<SWidget> SNarrativeSpaceEditor::CreateToolbar()
{
	const TSharedRef<SHorizontalBox> Toolbar = SNew(SHorizontalBox);

	Toolbar->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		CreatePlayControls()
	];
	Toolbar->AddSlot().AutoWidth().Padding(8, 2, 2, 2)
	[
		CreateSourceButton(ENarrativeSpaceSource::Static, LOCTEXT("StaticMode", "Static"),
			LOCTEXT("StaticModeTip", "Plot and drag the values authored on the assets. Editing stays live during play: a move is pushed straight into the running simulation."))
	];
	Toolbar->AddSlot().AutoWidth().Padding(2)
	[
		CreateSourceButton(ENarrativeSpaceSource::Runtime, LOCTEXT("RuntimeMode", "Runtime"),
			LOCTEXT("RuntimeModeTip", "Plot where the play session's entities actually are. The simulation owns those positions, so cards cannot be dragged; edit the asset in the details panel and watch it land. Assets with no simulated counterpart keep showing their authored value."))
	];

	auto AddButton = [&Toolbar](const FText& Label, const FText& Tooltip, TFunction<void()> Action)
	{
		Toolbar->AddSlot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(Label)
			.ToolTipText(Tooltip)
			.OnClicked_Lambda([Action] { Action(); return FReply::Handled(); })
		];
	};

	AddButton(LOCTEXT("Refresh", "Refresh"),
		LOCTEXT("RefreshTip", "Reload assets matching this query."),
		[this] { Model->Refresh(); });
	AddButton(LOCTEXT("FrameAll", "Frame all"),
		LOCTEXT("FrameAllTip", "Frame all matching assets (Home)."),
		[this] { Viewport->FrameAll(); });
	AddButton(LOCTEXT("FrameSelection", "Frame selection"),
		LOCTEXT("FrameSelectionTip", "Frame the selection (F)."),
		[this] { Viewport->FrameAll(true); });
	AddButton(LOCTEXT("XY", "XY"),
		LOCTEXT("XYTip", "Look along Z."),
		[this] { Viewport->SetView(ENarrativeSpaceView::XY); });
	AddButton(LOCTEXT("XZ", "XZ"),
		LOCTEXT("XZTip", "Look along Y (three-axis query)."),
		[this] { Viewport->SetView(ENarrativeSpaceView::XZ); });
	AddButton(LOCTEXT("YZ", "YZ"),
		LOCTEXT("YZTip", "Look along X (three-axis query)."),
		[this] { Viewport->SetView(ENarrativeSpaceView::YZ); });
	AddButton(LOCTEXT("Iso", "Iso"),
		LOCTEXT("IsoTip", "Isometric view (three-axis query)."),
		[this] { Viewport->SetView(ENarrativeSpaceView::Iso); });

	Toolbar->AddSlot().FillWidth(1)
	[
		SNew(SSpacer)
	];
	Toolbar->AddSlot().AutoWidth().Padding(2)
	[
		SNew(SButton)
		.Text(LOCTEXT("Save", "Save matching assets"))
		.ToolTipText(LOCTEXT("SaveTip", "Offer to check out and save dirty packages in the current query. Other content is not saved."))
		.IsEnabled_Lambda([this] { return Model->CanEdit() && !Model->IsDragging(); })
		.OnClicked_Lambda([this] { Model->Save(); return FReply::Handled(); })
	];

	return Toolbar;
}

void SNarrativeSpaceEditor::QueryChanged(const FPropertyChangedEvent& Event)
{
	Viewport->SetIncompleteDisplay(Settings->Incomplete);
	ApplyQuery();
}

void SNarrativeSpaceEditor::ApplyQuery()
{
	// Incomplete is display only, and SetQuery always reloads every matching asset. Only re-run
	// the query when the query itself moved, so toggling the display option stays cheap.
	const FNarrativeSpaceQuery& Current = Model->GetQuery();
	const bool bAxesChanged = Current.Axes != Settings->Query.Axes;
	const bool bQueryChanged = bAxesChanged
		|| Current.Classes != Settings->Query.Classes
		|| Current.ContentPath != Settings->Query.ContentPath
		|| Current.PlacementField != Settings->Query.PlacementField;
	if (!bQueryChanged) { return; }

	Model->SetQuery(Settings->Query);
	if (bAxesChanged) { Viewport->ResetCamera(); }
}

void SNarrativeSpaceEditor::SelectionChanged()
{
	// Refresh broadcasts whether or not the selection actually moved, and SetObjects rebuilds the
	// whole tree, destroying any widget the user is mid-interaction with. Only rebind on a change.
	const TArray<UObject*> Selection = Model->GetSelection();
	bool bChanged = Selection.Num() != DisplayedSelection.Num();
	for (int32 Index = 0; !bChanged && Index < Selection.Num(); ++Index)
	{
		bChanged = DisplayedSelection[Index].Get() != Selection[Index];
	}
	if (!bChanged) { return; }

	DisplayedSelection.Reset(Selection.Num());
	for (UObject* Object : Selection) { DisplayedSelection.Emplace(Object); }
	AssetDetails->SetObjects(Selection, true);
}

void SNarrativeSpaceEditor::RefreshFieldOptions()
{
	FieldOptions.Reset();
	FieldOptions.Add(MakeShared<FName>(NAME_None));
	for (const FName Field : Model->GetPlacementFields()) { FieldOptions.Add(MakeShared<FName>(Field)); }

	// Keep an explicit choice visible when changing filters leaves no matching assets.
	if (!Settings->Query.PlacementField.IsNone() && !Model->GetPlacementFields().Contains(Settings->Query.PlacementField))
	{
		FieldOptions.Add(MakeShared<FName>(Settings->Query.PlacementField));
	}

	FieldPicker->RefreshOptions();
	for (const TSharedPtr<FName>& Field : FieldOptions)
	{
		if (*Field == Settings->Query.PlacementField)
		{
			FieldPicker->SetSelectedItem(Field);
			break;
		}
	}
}

void SNarrativeSpaceEditor::SetPlacementField(const FName Field)
{
	if (Settings->Query.PlacementField == Field) { return; }
	Settings->Query.PlacementField = Field;
	// SetQuery cancels any in-flight edit before changing the resolved property cache.
	Model->SetQuery(Settings->Query);
	Viewport->FrameAll();
}

#undef LOCTEXT_NAMESPACE
