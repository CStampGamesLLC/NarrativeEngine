#pragma once

#include "CoreMinimal.h"
#include "NarrativeLints.h"
#include "Widgets/SCompoundWidget.h"

/**
 * The whole reach editor, shared by the card details and the new-card wizard: σ as a number with its tier badge
 * and the strength cap it allows, the rarity band bar, and chips that snap to each tier's narrowest reach.
 * It holds no value of its own; it reads Sigma and reports changes.
 */
class SNarrativeReachField : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_TwoParams(FOnReachChanged, float /*Sigma*/, bool /*bInteractive*/);

	SLATE_BEGIN_ARGS(SNarrativeReachField) {}
		SLATE_ATTRIBUTE(float, Sigma)
		/** A slider or bar drag starts: open one transaction for its steps. */
		SLATE_EVENT(FSimpleDelegate, OnBeginDrag)
		/** Interactive steps come from a drag; the final step and every click or commit are not. */
		SLATE_EVENT(FOnReachChanged, OnReachChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** σ to two places, the way every reach and strength number reads. */
	static FText Number(float Value);

	/** A pill in the tier's rarity color. */
	static TSharedRef<SWidget> TierBadge(TAttribute<ENarrativeTier> Tier);
};
