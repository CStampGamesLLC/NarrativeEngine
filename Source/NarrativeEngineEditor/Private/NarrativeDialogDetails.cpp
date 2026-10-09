#include "NarrativeDialogDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "NarrativeCoreData.h"
#include "NarrativeDialogEditing.h"
#include "NarrativeLints.h"
#include "NarrativeStaticData.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "SNarrativeReachBar.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "NarrativeDialogDetails"

namespace
{
	using namespace NarrativeDialogEditing;

	const FLinearColor WarningInk(1.f, 0.62f, 0.12f);
	const FLinearColor ErrorInk(1.f, 0.32f, 0.26f);
	const FLinearColor OkInk(0.35f, 0.82f, 0.45f);

	FText Number(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 2;
		Options.MaximumFractionalDigits = 2;
		Options.UseGrouping = false;
		return FText::AsNumber(Value, &Options);
	}

	/** A pill in the tier's rarity color. */
	TSharedRef<SWidget> TierBadge(TAttribute<ENarrativeTier> Tier)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([Tier]() { return FSlateColor(TierColor(Tier.Get())); })
			.Padding(FMargin(6.f, 1.f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("SmallFontBold"))
				.ColorAndOpacity(FLinearColor::Black)
				.Text_Lambda([Tier]() { return TierName(Tier.Get()); })
			];
	}
}

TSharedRef<IDetailCustomization> FNarrativeDialogDetails::MakeInstance()
{
	return MakeShared<FNarrativeDialogDetails>();
}

void FNarrativeDialogDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1 || !Cast<UNarrativeDialogDef>(Objects[0].Get()))
	{
		return;
	}
	Card = Cast<UNarrativeDialogDef>(Objects[0].Get());

	SigmaHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, SpaceSigma));
	StrengthHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Strength));
	LocationHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Location));
	MaskHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Mask));

	IDetailCategoryBuilder& CardCategory = DetailBuilder.EditCategory("Card", LOCTEXT("Card", "Card"), ECategoryPriority::Important);
	CardCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Phrase)));
	CardCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Label)));
	CardCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Family)));
	CardCategory.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, AbilityTags)));

	BuildDeed(DetailBuilder);
	BuildMeaning(DetailBuilder);
}

void FNarrativeDialogDetails::BuildDeed(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Deed = DetailBuilder.EditCategory("Deed", LOCTEXT("Deed", "Deed"), ECategoryPriority::Important);
	DetailBuilder.HideProperty(SigmaHandle);
	DetailBuilder.HideProperty(StrengthHandle);

	auto Sigma = [this]() { return Card.IsValid() ? Card->SpaceSigma : static_cast<float>(NarrativeLints::MinReach); };
	auto Tier = [Sigma]() { return NarrativeLints::TierForReach(Sigma()); };

	// Reach: the number, its tier, the band bar, and chips that snap to each tier's narrowest reach.
	TSharedRef<SHorizontalBox> Chips = SNew(SHorizontalBox);
	for (const ENarrativeTier ChipTier : TiersByReach())
	{
		const float Floor = static_cast<float>(NarrativeLints::TierReachFloor(ChipTier));
		const TOptional<float> Cap = StrengthCap(Floor);
		Chips->AddSlot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(0.f))
			.ToolTipText(Cap.IsSet()
				? FText::Format(LOCTEXT("ChipTip", "Snap reach to \u03C3 {0}, where {1} cards may reach J \u2264 {2}."), Number(Floor), TierName(ChipTier), Number(Cap.GetValue()))
				: FText::Format(LOCTEXT("ChipTipLegendary", "Snap reach to \u03C3 {0}. Legendary cards have no strength cap: they canonize instead."), Number(Floor)))
			.OnClicked_Lambda([this, Floor]()
			{
				SetFloat(SigmaHandle, Floor, false);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.Padding(FMargin(5.f, 1.f))
				.BorderBackgroundColor_Lambda([Tier, ChipTier]()
				{
					FLinearColor Color = TierColor(ChipTier);
					Color.A = Tier() == ChipTier ? 1.f : 0.4f;
					return FSlateColor(Color);
				})
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity(FLinearColor::Black)
					.Text(TierName(ChipTier))
				]
			]
		];
	}

	Deed.AddCustomRow(LOCTEXT("Reach", "Reach"))
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(LOCTEXT("ReachName", "Reach (\u03C3)"))
		.ToolTipText(SigmaHandle->GetToolTipText())
	]
	.ValueContent()
	.MinDesiredWidth(280.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(72.f)
				[
					SNew(SNumericEntryBox<float>)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.AllowSpin(true)
					.MinValue(TOptional<float>(0.1f))
					.MinSliderValue(TOptional<float>(SNarrativeReachBar::MinSigma))
					.MaxSliderValue(TOptional<float>(SNarrativeReachBar::MaxSigma))
					.Value_Lambda([Sigma]() { return TOptional<float>(Sigma()); })
					.OnBeginSliderMovement_Lambda([this]() { DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("SetReach", "Set Reach")); })
					.OnValueChanged_Lambda([this](float Value) { if (DragTransaction) { SetFloat(SigmaHandle, Value, true); } })
					.OnEndSliderMovement_Lambda([this](float Value) { SetFloat(SigmaHandle, Value, false); })
					.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { if (!DragTransaction) { SetFloat(SigmaHandle, Value, false); } })
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f).VAlign(VAlign_Center)
			[
				TierBadge(TAttribute<ENarrativeTier>::CreateLambda(Tier))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([Sigma]()
				{
					if (Sigma() < NarrativeLints::MinReach)
					{
						return FText::Format(LOCTEXT("BelowFloor", "Below \u03C3 {0}: finer than the sampler resolves (L11)."), Number(static_cast<float>(NarrativeLints::MinReach)));
					}
					const TOptional<float> Cap = StrengthCap(Sigma());
					return Cap.IsSet() ? FText::Format(LOCTEXT("CapHere", "J \u2264 {0} here"), Number(Cap.GetValue())) : LOCTEXT("NoCapHere", "J uncapped");
				})
				.ColorAndOpacity_Lambda([Sigma]() { return Sigma() < NarrativeLints::MinReach ? FSlateColor(ErrorInk) : FSlateColor::UseSubduedForeground(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SNarrativeReachBar)
			.Sigma_Lambda([Sigma]() { return TOptional<float>(Sigma()); })
			.OnReachChanged_Lambda([this](float Value, bool bInteractive)
			{
				if (bInteractive && !DragTransaction)
				{
					DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("SetReach", "Set Reach"));
				}
				SetFloat(SigmaHandle, Value, bInteractive);
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			Chips
		]
	];

	// Strength: capped by the reach above; over the cap (a wider reach, an old card) shows red with a fix.
	auto Strength = [this]() { return Card.IsValid() ? Card->Strength : 0.f; };
	auto Cap = [Sigma]() { return StrengthCap(Sigma()); };
	auto IsOverCap = [Strength, Cap]() { return Cap().IsSet() && Strength() > Cap().GetValue() + UE_KINDA_SMALL_NUMBER; };

	Deed.AddCustomRow(LOCTEXT("Strength", "Strength"))
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(LOCTEXT("StrengthName", "Strength (J)"))
		.ToolTipText(StrengthHandle->GetToolTipText())
	]
	.ValueContent()
	.MinDesiredWidth(280.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(72.f)
			[
				SNew(SNumericEntryBox<float>)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AllowSpin(true)
				.MinValue(TOptional<float>(0.f))
				.MaxValue_Lambda(Cap)
				.MinSliderValue(TOptional<float>(0.f))
				.MaxSliderValue_Lambda([Cap]() { return TOptional<float>(Cap().Get(2.f)); })
				.Value_Lambda([Strength]() { return TOptional<float>(Strength()); })
				.OnBeginSliderMovement_Lambda([this]() { DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("SetStrength", "Set Strength")); })
				.OnValueChanged_Lambda([this](float Value) { if (DragTransaction) { SetFloat(StrengthHandle, Value, true); } })
				.OnEndSliderMovement_Lambda([this](float Value) { SetFloat(StrengthHandle, Value, false); })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type) { if (!DragTransaction) { SetFloat(StrengthHandle, Value, false); } })
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.AutoWrapText(true)
			.Text_Lambda([Strength, Cap, IsOverCap, Tier]()
			{
				if (IsOverCap())
				{
					return FText::Format(LOCTEXT("OverCap", "Over the {0} cap of {1} (L6)."), TierName(Tier()), Number(Cap().GetValue()));
				}
				if (Strength() <= 0.f)
				{
					return LOCTEXT("NotADeed", "0: saying it is not a deed.");
				}
				return Cap().IsSet()
					? FText::Format(LOCTEXT("WithinCap", "of {0} at {1}"), Number(Cap().GetValue()), TierName(Tier()))
					: LOCTEXT("Uncapped", "Legendary: uncapped");
			})
			.ColorAndOpacity_Lambda([IsOverCap]() { return IsOverCap() ? FSlateColor(ErrorInk) : FSlateColor::UseSubduedForeground(); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Visibility_Lambda([IsOverCap]() { return IsOverCap() ? EVisibility::Visible : EVisibility::Collapsed; })
			.Text(LOCTEXT("Clamp", "Clamp"))
			.ToolTipText(LOCTEXT("ClampTip", "Lower J to the cap at this reach."))
			.OnClicked_Lambda([this, Cap]()
			{
				if (Cap().IsSet())
				{
					SetFloat(StrengthHandle, Cap().GetValue(), false);
				}
				return FReply::Handled();
			})
		]
	];

	Deed.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Stance)));

	// The card's lint findings, live, so a fix shows its effect at once.
	auto Findings = [this]()
	{
		TArray<FNarrativeLintIssue> Issues;
		if (Card.IsValid())
		{
			NarrativeLints::LintCard(*Card, Issues);
		}
		return Issues;
	};
	Deed.AddCustomRow(LOCTEXT("Checks", "Checks"))
	.NameContent()[SNew(STextBlock).Font(IDetailLayoutBuilder::GetDetailFont()).Text(LOCTEXT("ChecksName", "Checks"))]
	.ValueContent()
	.MinDesiredWidth(280.f)
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.AutoWrapText(true)
		.Text_Lambda([Findings]()
		{
			const TArray<FNarrativeLintIssue> Issues = Findings();
			if (Issues.IsEmpty())
			{
				return LOCTEXT("Clean", "No lint findings.");
			}
			TArray<FString> Lines;
			for (const FNarrativeLintIssue& Issue : Issues)
			{
				Lines.Add(FString::Printf(TEXT("%s %s: %s"), Issue.Severity == ENarrativeLintSeverity::Error ? TEXT("Error") : TEXT("Warning"), *Issue.Rule, *Issue.Message));
			}
			return FText::FromString(FString::Join(Lines, TEXT("\n")));
		})
		.ColorAndOpacity_Lambda([Findings]()
		{
			const TArray<FNarrativeLintIssue> Issues = Findings();
			if (Issues.IsEmpty()) { return FSlateColor(OkInk); }
			const bool bError = Issues.ContainsByPredicate([](const FNarrativeLintIssue& Issue) { return Issue.Severity == ENarrativeLintSeverity::Error; });
			return FSlateColor(bError ? ErrorInk : WarningInk);
		})
	];
}

void FNarrativeDialogDetails::BuildMeaning(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Meaning = DetailBuilder.EditCategory("Meaning", LOCTEXT("Meaning", "Meaning"), ECategoryPriority::Important);

	Meaning.AddCustomRow(LOCTEXT("MeaningSummary", "About"))
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.AutoWrapText(true)
			.Text_Lambda([this]()
			{
				const int32 Count = Card.IsValid() ? Card->Mask.Num() : 0;
				if (Count == 0)
				{
					return LOCTEXT("AboutNothing", "Not about anything yet: tick an axis or type its coordinate (L13).");
				}
				if (Count > NarrativeLints::MaxMaskAxes)
				{
					return FText::Format(LOCTEXT("AboutTooMany", "About {0} axes. More than {1} brings back the uncertainty penalty (L13)."), Count, NarrativeLints::MaxMaskAxes);
				}
				return FText::Format(LOCTEXT("AboutCount", "About {0} {0}|plural(one=axis,other=axes). A coordinate of 0 means deliberately neutral."), Count);
			})
			.ColorAndOpacity_Lambda([this]()
			{
				const int32 Count = Card.IsValid() ? Card->Mask.Num() : 0;
				return Count == 0 ? FSlateColor(ErrorInk) : Count > NarrativeLints::MaxMaskAxes ? FSlateColor(WarningInk) : FSlateColor::UseSubduedForeground();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Visibility_Lambda([this]() { return Card.IsValid() && NeedsTidy(*Card) ? EVisibility::Visible : EVisibility::Collapsed; })
			.Text(LOCTEXT("Tidy", "Tidy"))
			.ToolTipText(LOCTEXT("TidyTip", "Drop coordinates outside the mask (the model ignores them) and type blank masked axes at 0."))
			.OnClicked_Lambda([this]()
			{
				EditMeaning(LOCTEXT("TidyMeaning", "Tidy Card Meaning"), [](UNarrativeDialogDef& Def) { Tidy(Def); });
				return FReply::Handled();
			})
		]
	];

	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : GatherAxes(*Card))
	{
		auto State = [this, Axis]() { return Card.IsValid() ? GetAxisState(*Card, Axis) : EAxisState::Absent; };
		const FText Label = AxisLabel(Axis);

		Meaning.AddCustomRow(Label)
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SCheckBox)
				.ToolTipText(LOCTEXT("AboutTip", "Whether the card is about this axis. Unticking clears its coordinate too."))
				.IsChecked_Lambda([State]()
				{
					switch (State())
					{
					case EAxisState::About:
					case EAxisState::Blank: return ECheckBoxState::Checked;
					case EAxisState::Ignored: return ECheckBoxState::Undetermined;
					default: return ECheckBoxState::Unchecked;
					}
				})
				.OnCheckStateChanged_Lambda([this, Axis](ECheckBoxState NewState)
				{
					const bool bAbout = NewState == ECheckBoxState::Checked;
					EditMeaning(bAbout ? LOCTEXT("AddAxis", "Add Axis to Card") : LOCTEXT("RemoveAxis", "Remove Axis from Card"),
						[&Axis, bAbout](UNarrativeDialogDef& Def) { SetAbout(Def, Axis, bAbout); });
				})
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(Label)
				.ToolTipText(FText::FromString(Axis.ToString()))
				.ColorAndOpacity_Lambda([State]() { return State() == EAxisState::Absent ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground(); })
			]
		]
		.ValueContent()
		.MinDesiredWidth(280.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(72.f)
				[
					SNew(SNumericEntryBox<float>)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.AllowSpin(true)
					.MinSliderValue(TOptional<float>(-30.f))
					.MaxSliderValue(TOptional<float>(30.f))
					.UndeterminedString(LOCTEXT("Untyped", "-"))
					.ToolTipText(LOCTEXT("CoordinateTip", "Typing a coordinate makes the card about this axis."))
					.Value_Lambda([this, Axis]() { return Card.IsValid() ? GetCoordinate(*Card, Axis) : TOptional<float>(); })
					.OnBeginSliderMovement_Lambda([this]()
					{
						DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("SetCoordinate", "Set Card Coordinate"));
						if (Card.IsValid()) { Card->Modify(); }
					})
					.OnValueChanged_Lambda([this, Axis](float Value) { if (DragTransaction) { SetAxisCoordinate(Axis, Value, true); } })
					.OnEndSliderMovement_Lambda([this, Axis](float Value) { SetAxisCoordinate(Axis, Value, false); })
					.OnValueCommitted_Lambda([this, Axis](float Value, ETextCommit::Type)
					{
						if (!DragTransaction) { SetAxisCoordinate(Axis, Value, false); }
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text_Lambda([State]()
				{
					switch (State())
					{
					case EAxisState::Ignored: return LOCTEXT("Ignored", "Ignored: outside the mask (L1). Tick to keep it.");
					case EAxisState::Blank: return LOCTEXT("Blank", "Blank: masked with no coordinate.");
					default: return FText::GetEmpty();
					}
				})
				.ColorAndOpacity_Lambda([State]() { return State() == EAxisState::Blank ? FSlateColor(ErrorInk) : FSlateColor(WarningInk); })
			]
		];
	}

	// The raw fields, for anything the rows above don't cover.
	Meaning.AddProperty(LocationHandle, EPropertyLocation::Advanced);
	Meaning.AddProperty(MaskHandle, EPropertyLocation::Advanced);
}

void FNarrativeDialogDetails::SetFloat(const TSharedPtr<IPropertyHandle>& Handle, float Value, bool bInteractive)
{
	if (!Handle.IsValid())
	{
		return;
	}

	if (bInteractive)
	{
		Handle->SetValue(Value, EPropertyValueSetFlags::InteractiveChange);
		return;
	}

	// Inside an open drag transaction this nests, so the whole drag is one undo step.
	Handle->SetValue(Value);
	DragTransaction.Reset();
}

void FNarrativeDialogDetails::EditMeaning(const FText& Description, TFunctionRef<void(UNarrativeDialogDef&)> Edit)
{
	UNarrativeDialogDef* Def = Card.Get();
	if (!Def || !LocationHandle.IsValid() || !MaskHandle.IsValid())
	{
		return;
	}

	FScopedTransaction Transaction(Description);
	Def->Modify();
	LocationHandle->NotifyPreChange();
	MaskHandle->NotifyPreChange();
	Edit(*Def);
	LocationHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	MaskHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

void FNarrativeDialogDetails::SetAxisCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value, bool bInteractive)
{
	UNarrativeDialogDef* Def = Card.Get();
	if (!Def)
	{
		return;
	}

	if (bInteractive)
	{
		// Live while dragging; the drag's transaction captured the card before the first step. No notification,
		// because a details refresh mid-drag would destroy the slider being dragged.
		NarrativeDialogEditing::SetCoordinate(*Def, Axis, Value);
		return;
	}

	EditMeaning(LOCTEXT("SetCoordinateFinal", "Set Card Coordinate"), [&Axis, Value](UNarrativeDialogDef& Edited)
	{
		NarrativeDialogEditing::SetCoordinate(Edited, Axis, Value);
	});
	DragTransaction.Reset();
}

#undef LOCTEXT_NAMESPACE
