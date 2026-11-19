#include "SNarrativeReachField.h"

#include "DetailLayoutBuilder.h"
#include "NarrativeDialogEditing.h"
#include "SNarrativeReachBar.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "NarrativeDialogDetails"

namespace
{
	const FLinearColor ReachErrorInk(1.f, 0.32f, 0.26f);
}

FText SNarrativeReachField::Number(float Value)
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 2;
	Options.MaximumFractionalDigits = 2;
	Options.UseGrouping = false;
	return FText::AsNumber(Value, &Options);
}

TSharedRef<SWidget> SNarrativeReachField::TierBadge(TAttribute<ENarrativeTier> Tier)
{
	using namespace NarrativeDialogEditing;
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

void SNarrativeReachField::Construct(const FArguments& InArgs)
{
	using namespace NarrativeDialogEditing;

	const TAttribute<float> SigmaAttribute = InArgs._Sigma;
	const FSimpleDelegate BeginDragDelegate = InArgs._OnBeginDrag;
	const FOnReachChanged ChangedDelegate = InArgs._OnReachChanged;

	auto Sigma = [SigmaAttribute]() { return SigmaAttribute.Get(); };
	auto Tier = [Sigma]() { return NarrativeLints::TierForReach(Sigma()); };
	auto SetSigma = [ChangedDelegate](float Value, bool bInteractive) { ChangedDelegate.ExecuteIfBound(Value, bInteractive); };
	auto BeginDrag = [BeginDragDelegate]() { BeginDragDelegate.ExecuteIfBound(); };

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

	ChildSlot
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
					.OnBeginSliderMovement_Lambda(BeginDrag)
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
				.ColorAndOpacity_Lambda([Sigma]() { return Sigma() < NarrativeLints::MinReach ? FSlateColor(ReachErrorInk) : FSlateColor::UseSubduedForeground(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SNarrativeReachBar)
			.Sigma_Lambda([Sigma]() { return TOptional<float>(Sigma()); })
			.OnReachChanged_Lambda([BeginDrag, SetSigma](float Value, bool bInteractive)
			{
				if (bInteractive) { BeginDrag(); }
				SetSigma(Value, bInteractive);
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			Chips
		]
	];
}

#undef LOCTEXT_NAMESPACE
