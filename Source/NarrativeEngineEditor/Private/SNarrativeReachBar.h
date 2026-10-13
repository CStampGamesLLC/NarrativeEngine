#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * Reach σ drawn as the pricing tiers it falls in, narrowest (Legendary) on the left. Each band is filled in
 * its rarity color and labeled with its strength cap; the current tier is bright and the marker shows σ.
 * Click or drag to set σ.
 */
class SNarrativeReachBar : public SLeafWidget
{
public:
	DECLARE_DELEGATE_TwoParams(FOnReachChanged, float /*Sigma*/, bool /*bInteractive*/);

	SLATE_BEGIN_ARGS(SNarrativeReachBar) {}
		SLATE_ATTRIBUTE(TOptional<float>, Sigma)
		SLATE_EVENT(FOnReachChanged, OnReachChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** The bar's span; reaches beyond it pin the marker to the edge. */
	static constexpr float MinSigma = 1.f;
	static constexpr float MaxSigma = 10.f;

	/** σ under a local x position, rounded to 0.1. */
	float SigmaAt(float LocalX, float Width) const;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(240.f, 22.f); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	float XFor(float Sigma, float Width) const;
	void Report(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bInteractive);

	TAttribute<TOptional<float>> Sigma;
	FOnReachChanged OnReachChanged;
	bool bDragging = false;
};
