#include "NarrativeDialogDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
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

	const FName SigmaName = GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, SpaceSigma);
	const FName StrengthName = GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Strength);
	const FName LocationName = GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Location);
	const FName MaskName = GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Mask);

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
	PropertyUtilities = DetailBuilder.GetPropertyUtilities();

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
	const TSharedRef<IPropertyHandle> SigmaHandle = DetailBuilder.GetProperty(SigmaName);
	const TSharedRef<IPropertyHandle> StrengthHandle = DetailBuilder.GetProperty(StrengthName);
	DetailBuilder.HideProperty(SigmaHandle);
	DetailBuilder.HideProperty(StrengthHandle);

	// Widgets may outlive this customization by a frame (a row torn down mid-commit), so they hold it weakly.
	const TWeakPtr<FNarrativeDialogDetails> Weak = StaticCastSharedRef<FNarrativeDialogDetails>(AsShared());
	const TWeakObjectPtr<UNarrativeDialogDef> WeakCard = Card;
	auto SetSigma = [Weak](float Value, bool bInteractive)
	{
		if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin()) { Self->SetFloat(SigmaName, LOCTEXT("SetReach", "Set Reach"), Value, bInteractive); }
	};
	auto SetStrength = [Weak](float Value, bool bInteractive)
	{
		if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin()) { Self->SetFloat(StrengthName, LOCTEXT("SetStrength", "Set Strength"), Value, bInteractive); }
	};
	auto BeginDrag = [Weak](const FText& Description)
	{
		if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin()) { Self->BeginDrag(Description); }
	};

	auto Sigma = [WeakCard]() { return WeakCard.IsValid() ? WeakCard->SpaceSigma : static_cast<float>(NarrativeLints::MinReach); };
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
			.OnClicked_Lambda([SetSigma, Floor]()
			{
				SetSigma(Floor, false);
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
					.OnBeginSliderMovement_Lambda([BeginDrag]() { BeginDrag(LOCTEXT("SetReach", "Set Reach")); })
					.OnValueChanged_Lambda([SetSigma](float Value) { SetSigma(Value, true); })
					.OnEndSliderMovement_Lambda([SetSigma](float Value) { SetSigma(Value, false); })
					.OnValueCommitted_Lambda([SetSigma](float Value, ETextCommit::Type) { SetSigma(Value, false); })
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
			.OnReachChanged_Lambda([BeginDrag, SetSigma](float Value, bool bInteractive)
			{
				if (bInteractive) { BeginDrag(LOCTEXT("SetReach", "Set Reach")); }
				SetSigma(Value, bInteractive);
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			Chips
		]
	];

	// Strength: capped by the reach above; over the cap (a wider reach, an old card) shows red with a fix.
	auto Strength = [WeakCard]() { return WeakCard.IsValid() ? WeakCard->Strength : 0.f; };
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
				.OnBeginSliderMovement_Lambda([BeginDrag]() { BeginDrag(LOCTEXT("SetStrength", "Set Strength")); })
				.OnValueChanged_Lambda([SetStrength](float Value) { SetStrength(Value, true); })
				.OnEndSliderMovement_Lambda([SetStrength](float Value) { SetStrength(Value, false); })
				.OnValueCommitted_Lambda([SetStrength](float Value, ETextCommit::Type) { SetStrength(Value, false); })
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
			.OnClicked_Lambda([SetStrength, Cap]()
			{
				if (Cap().IsSet())
				{
					SetStrength(Cap().GetValue(), false);
				}
				return FReply::Handled();
			})
		]
	];

	Deed.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNarrativeDialogDef, Stance)));

	// The card's lint findings, live, so a fix shows its effect at once.
	auto Findings = [WeakCard]()
	{
		TArray<FNarrativeLintIssue> Issues;
		if (WeakCard.IsValid())
		{
			NarrativeLints::LintCard(*WeakCard, Issues);
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
	const TWeakPtr<FNarrativeDialogDetails> Weak = StaticCastSharedRef<FNarrativeDialogDetails>(AsShared());
	const TWeakObjectPtr<UNarrativeDialogDef> WeakCard = Card;

	Meaning.AddCustomRow(LOCTEXT("MeaningSummary", "About"))
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.AutoWrapText(true)
			.Text_Lambda([WeakCard]()
			{
				const int32 Count = WeakCard.IsValid() ? WeakCard->Mask.Num() : 0;
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
			.ColorAndOpacity_Lambda([WeakCard]()
			{
				const int32 Count = WeakCard.IsValid() ? WeakCard->Mask.Num() : 0;
				return Count == 0 ? FSlateColor(ErrorInk) : Count > NarrativeLints::MaxMaskAxes ? FSlateColor(WarningInk) : FSlateColor::UseSubduedForeground();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Visibility_Lambda([WeakCard]() { return WeakCard.IsValid() && NeedsTidy(*WeakCard) ? EVisibility::Visible : EVisibility::Collapsed; })
			.Text(LOCTEXT("Tidy", "Tidy"))
			.ToolTipText(LOCTEXT("TidyTip", "Drop coordinates outside the mask (the model ignores them) and type blank masked axes at 0."))
			.OnClicked_Lambda([Weak]()
			{
				if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin())
				{
					Self->EditMeaning(LOCTEXT("TidyMeaning", "Tidy Card Meaning"), false, [](UNarrativeDialogDef& Def) { Tidy(Def); });
				}
				return FReply::Handled();
			})
		]
	];

	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : GatherAxes(*Card))
	{
		auto State = [WeakCard, Axis]() { return WeakCard.IsValid() ? GetAxisState(*WeakCard, Axis) : EAxisState::Absent; };
		auto SetValue = [Weak, Axis](float Value, bool bInteractive)
		{
			if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin())
			{
				Self->EditMeaning(LOCTEXT("SetCoordinate", "Set Card Coordinate"), bInteractive,
					[&Axis, Value](UNarrativeDialogDef& Def) { SetCoordinate(Def, Axis, Value); });
			}
		};
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
				.OnCheckStateChanged_Lambda([Weak, Axis](ECheckBoxState NewState)
				{
					const bool bAbout = NewState == ECheckBoxState::Checked;
					if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin())
					{
						Self->EditMeaning(bAbout ? LOCTEXT("AddAxis", "Add Axis to Card") : LOCTEXT("RemoveAxis", "Remove Axis from Card"), false,
							[&Axis, bAbout](UNarrativeDialogDef& Def) { SetAbout(Def, Axis, bAbout); });
					}
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
					.Value_Lambda([WeakCard, Axis]() { return WeakCard.IsValid() ? GetCoordinate(*WeakCard, Axis) : TOptional<float>(); })
					.OnBeginSliderMovement_Lambda([Weak]()
					{
						if (const TSharedPtr<FNarrativeDialogDetails> Self = Weak.Pin()) { Self->BeginDrag(LOCTEXT("SetCoordinate", "Set Card Coordinate")); }
					})
					.OnValueChanged_Lambda([SetValue](float Value) { SetValue(Value, true); })
					.OnEndSliderMovement_Lambda([SetValue](float Value) { SetValue(Value, false); })
					.OnValueCommitted_Lambda([SetValue](float Value, ETextCommit::Type) { SetValue(Value, false); })
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
	Meaning.AddProperty(DetailBuilder.GetProperty(LocationName), EPropertyLocation::Advanced);
	Meaning.AddProperty(DetailBuilder.GetProperty(MaskName), EPropertyLocation::Advanced);
}

void FNarrativeDialogDetails::BeginDrag(const FText& Description)
{
	UNarrativeDialogDef* Def = Card.Get();
	if (!Def || DragTransaction)
	{
		return;
	}

	// The card is captured once, before the first step, so the whole drag undoes as one.
	DragTransaction = MakeShared<FScopedTransaction>(Description);
	Def->Modify();
}

void FNarrativeDialogDetails::EditCard(const FText& Description, TConstArrayView<FName> Properties, bool bInteractive,
	TFunctionRef<void(UNarrativeDialogDef&)> Edit, TFunctionRef<bool(const UNarrativeDialogDef&)> HasChanged)
{
	UNarrativeDialogDef* Def = Card.Get();
	if (!Def)
	{
		DragTransaction.Reset();
		return;
	}

	// Only a drag that was opened explicitly writes live. A spin box also reports typed values, wheel and arrow
	// steps as changes with no drag around them, and opening one there would leave it open for good. Those are
	// edits of their own, and a typed value's echo after its commit changes nothing, so it cancels.
	if (bInteractive && DragTransaction.IsValid())
	{
		Edit(*Def);
		return;
	}

	{
		// Nests into an open drag, so a drag and its final value are one undo step.
		const bool bClosesDrag = DragTransaction.IsValid();
		FScopedTransaction Transaction(Description);
		Def->Modify();
		Edit(*Def);

		// A commit that changes nothing does nothing: a field losing focus while its row is torn down commits again.
		if (!bClosesDrag && !HasChanged(*Def))
		{
			Transaction.Cancel();
			return;
		}

		for (const FName Name : Properties)
		{
			FPropertyChangedEvent Event(FindFProperty<FProperty>(UNarrativeDialogDef::StaticClass(), Name), EPropertyChangeType::ValueSet);
			Def->PostEditChangeProperty(Event);
		}
	}
	DragTransaction.Reset();
}

void FNarrativeDialogDetails::SetFloat(FName Property, const FText& Description, float Value, bool bInteractive)
{
	const UNarrativeDialogDef* Def = Card.Get();
	const FFloatProperty* Field = FindFProperty<FFloatProperty>(UNarrativeDialogDef::StaticClass(), Property);
	if (!Def || !Field)
	{
		DragTransaction.Reset();
		return;
	}

	const float Before = Field->GetPropertyValue_InContainer(Def);
	EditCard(Description, {Property}, bInteractive,
		[Field, Value](UNarrativeDialogDef& Edited) { Field->SetPropertyValue_InContainer(&Edited, Value); },
		[Field, Before](const UNarrativeDialogDef& Edited) { return Field->GetPropertyValue_InContainer(&Edited) != Before; });
}

void FNarrativeDialogDetails::EditMeaning(const FText& Description, bool bInteractive, TFunctionRef<void(UNarrativeDialogDef&)> Edit)
{
	UNarrativeDialogDef* Def = Card.Get();
	if (!Def)
	{
		DragTransaction.Reset();
		return;
	}

	const FVectorND LocationBefore = Def->Location;
	const FNarrativeMask MaskBefore = Def->Mask;
	auto AxesChanged = [&LocationBefore, &MaskBefore](const UNarrativeDialogDef& Edited)
	{
		return Edited.Location.GetBasis() != LocationBefore.GetBasis() || Edited.Mask.Axes != MaskBefore.Axes;
	};

	EditCard(Description, {LocationName, MaskName}, bInteractive, Edit, [&LocationBefore, &AxesChanged](const UNarrativeDialogDef& Edited)
	{
		return AxesChanged(Edited) || Edited.Location.GetCoordinates() != LocationBefore.GetCoordinates();
	});

	// The raw rows under Advanced index the vector by position, so a new set of axes needs new rows. Coordinate
	// edits keep the layout, and with it the focus of the field being typed in.
	if (!bInteractive && AxesChanged(*Def))
	{
		if (const TSharedPtr<IPropertyUtilities> Utilities = PropertyUtilities.Pin())
		{
			Utilities->RequestForceRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE
