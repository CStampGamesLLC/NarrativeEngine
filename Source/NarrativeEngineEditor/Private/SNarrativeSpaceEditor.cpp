#include "SNarrativeSpaceEditor.h"

#include "IDetailsView.h"
#include "NarrativeSpaceModel.h"
#include "PropertyEditorModule.h"
#include "SNarrativeSpaceViewport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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

	ChildSlot
	[
		SNew(SVerticalBox)
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
				+ SVerticalBox::Slot().AutoHeight().Padding(10)
				[
					SNew(STextBlock).Text(LOCTEXT("Query", "Query - axes in X, Y, Z order"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 2)
				[
					SNew(STextBlock).Text(LOCTEXT("PlacementField", "Placement field (plot + drag)"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(10, 2, 10, 8)
				[
					FieldPicker.ToSharedRef()
				]
				+ SVerticalBox::Slot().FillHeight(1)
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
			SNew(STextBlock).Text_Lambda([this] { return Model->GetStatus(); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("Help", "Drag: move | Shift-click / marquee: select | Tab / Shift-Tab: step through entries | Hold X/Y/Z: lock | Ctrl: snap | Esc: cancel | RMB drag / MMB: pan | RMB click: menu | Wheel: zoom | Alt+RMB: orbit | F/Home: frame | Double-click: open | Delete: delete assets | Ctrl+Z/Y: undo/redo. Editing is disabled during PIE."))
		]
	];
}

SNarrativeSpaceEditor::~SNarrativeSpaceEditor()
{
	Model->OnSelectionChanged.RemoveAll(this);
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
	// Asset values belong to the model mid-drag, and to the PIE world during play.
	AssetDetails->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda(
		[this] { return Model->CanEdit() && !Model->IsDragging(); }));
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

TSharedRef<SWidget> SNarrativeSpaceEditor::CreateToolbar()
{
	const TSharedRef<SHorizontalBox> Toolbar = SNew(SHorizontalBox);

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
