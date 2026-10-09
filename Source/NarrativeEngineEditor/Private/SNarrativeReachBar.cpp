#include "SNarrativeReachBar.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "NarrativeDialogEditing.h"
#include "NarrativeLints.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SNarrativeReachBar"

void SNarrativeReachBar::Construct(const FArguments& InArgs)
{
	Sigma = InArgs._Sigma;
	OnReachChanged = InArgs._OnReachChanged;
}

float SNarrativeReachBar::XFor(float InSigma, float Width) const
{
	return Width * FMath::Clamp((InSigma - MinSigma) / (MaxSigma - MinSigma), 0.f, 1.f);
}

float SNarrativeReachBar::SigmaAt(float LocalX, float Width) const
{
	const float Alpha = Width > 0.f ? FMath::Clamp(LocalX / Width, 0.f, 1.f) : 0.f;
	return FMath::RoundToFloat(FMath::Lerp(MinSigma, MaxSigma, Alpha) * 10.f) / 10.f;
}

int32 SNarrativeReachBar::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace NarrativeDialogEditing;
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const TOptional<float> Current = Sigma.Get();
	const TOptional<ENarrativeTier> CurrentTier = Current.IsSet() ? NarrativeLints::TierForReach(Current.GetValue()) : TOptional<ENarrativeTier>();
	const FSlateFontInfo Font = FAppStyle::GetFontStyle("SmallFont");
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const TConstArrayView<ENarrativeTier> Tiers = TiersByReach();
	for (int32 Index = 0; Index < Tiers.Num(); ++Index)
	{
		const ENarrativeTier Tier = Tiers[Index];
		const float Left = XFor(static_cast<float>(NarrativeLints::TierReachFloor(Tier)), Size.X);
		const float Right = Index + 1 < Tiers.Num() ? XFor(static_cast<float>(NarrativeLints::TierReachFloor(Tiers[Index + 1])), Size.X) : Size.X;
		const bool bActive = CurrentTier.IsSet() && CurrentTier.GetValue() == Tier;
		FLinearColor Fill = TierColor(Tier);
		Fill.A = bActive ? 1.f : 0.35f;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(FMath::Max(Right - Left - 1.f, 1.f), Size.Y), FSlateLayoutTransform(FVector2D(Left, 0.f))),
			White, ESlateDrawEffect::None, Fill * InWidgetStyle.GetColorAndOpacityTint());

		// The band's best cap, at its narrowest reach, when it fits.
		const TOptional<float> Cap = StrengthCap(static_cast<float>(NarrativeLints::TierReachFloor(Tier)));
		const FText Label = Cap.IsSet() ? FText::Format(LOCTEXT("Cap", "\u2264{0}"), FText::AsNumber(Cap.GetValue(), &FNumberFormattingOptions::DefaultNoGrouping()))
			: LOCTEXT("NoCap", "\u221E");
		const FVector2D LabelSize = Measure->Measure(Label, Font);
		if (LabelSize.X + 4.f < Right - Left)
		{
			const FLinearColor Ink = bActive ? FLinearColor::Black : FLinearColor(1.f, 1.f, 1.f, 0.75f);
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(FVector2D(Left + (Right - Left - LabelSize.X) * 0.5f, (Size.Y - LabelSize.Y) * 0.5f))),
				Label, Font, ESlateDrawEffect::None, Ink * InWidgetStyle.GetColorAndOpacityTint());
		}
	}

	if (Current.IsSet())
	{
		const float X = XFor(Current.GetValue(), Size.X);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(3.f, Size.Y + 4.f), FSlateLayoutTransform(FVector2D(FMath::Clamp(X - 1.5f, 0.f, Size.X - 3.f), -2.f))),
			White, ESlateDrawEffect::None, FLinearColor::White * InWidgetStyle.GetColorAndOpacityTint());
	}
	return LayerId + 3;
}

void SNarrativeReachBar::Report(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bInteractive)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	OnReachChanged.ExecuteIfBound(SigmaAt(Local.X, MyGeometry.GetLocalSize().X), bInteractive);
}

FReply SNarrativeReachBar::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !IsEnabled())
	{
		return FReply::Unhandled();
	}
	bDragging = true;
	Report(MyGeometry, MouseEvent, true);
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SNarrativeReachBar::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDragging)
	{
		return FReply::Unhandled();
	}
	Report(MyGeometry, MouseEvent, true);
	return FReply::Handled();
}

FReply SNarrativeReachBar::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bDragging || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bDragging = false;

	// The release commits once, as one undo step.
	Report(MyGeometry, MouseEvent, false);
	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SNarrativeReachBar::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
}

#undef LOCTEXT_NAMESPACE
