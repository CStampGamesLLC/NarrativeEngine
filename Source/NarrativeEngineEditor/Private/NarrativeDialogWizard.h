#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "NarrativeDialogWizard.generated.h"

class SWidget;
class UNarrativeDialogDef;

/**
 * What the new-card wizard asks for. Shown in a details view, never saved: Create copies it onto the new card.
 * Everything else (name, label, folder, position, mask) follows from these and from where Make New was opened.
 */
UCLASS(Transient)
class UNarrativeDialogWizardInput : public UObject
{
	GENERATED_BODY()

public:
	/* What's actually said in its entirety. Names the asset. */
	UPROPERTY(EditAnywhere, Category = "New Card")
	FText Phrase;

	/** The idea this card is one phrasing of. Its leaf becomes the card's label and the asset name's prefix. */
	UPROPERTY(EditAnywhere, Category = "New Card", meta = (Categories = "Dialog.Family"))
	FGameplayTag Family;

	/** Reach σ: how far along its axes the card still lands. Sets the tier and the strength cap. */
	UPROPERTY(EditAnywhere, Category = "New Card", meta = (DisplayName = "Reach (Sigma)"))
	float SpaceSigma = 5.f;

	/** J: how hard the deed lands on those who witness it. 0 means saying it is not a deed. */
	UPROPERTY(EditAnywhere, Category = "New Card", meta = (ClampMin = "0"))
	float Strength = 0.f;

	/** h: +1 for what is done to friends, -1 for what is done to foes. */
	UPROPERTY(EditAnywhere, Category = "New Card", meta = (ClampMin = "-1", ClampMax = "1"))
	int32 Stance = 1;

	/** Game seconds before the speaker may say this card again. 0: its family's, if any. */
	UPROPERTY(EditAnywhere, Category = "New Card", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 0.f;

	/** The asset name Create will ask for (made unique on creation if taken). */
	FString AssetName() const;

	/** Copies the answers onto a new card, the label inferred from the family and J held to the reach's cap. */
	void Apply(UNarrativeDialogDef& Card) const;
};

namespace NarrativeDialogWizard
{
	/**
	 * Opens the wizard as a modal window over Parent and blocks until it closes. True when the author pressed
	 * Create, with Input holding the answers. Answers other than the phrase carry over to the next card.
	 */
	bool Run(const TSharedPtr<SWidget>& Parent, UNarrativeDialogWizardInput& Input);
}
