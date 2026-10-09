#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "UObject/SoftObjectPtr.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class FScopedTransaction;
class UNarrativeBasisVector;
class UNarrativeDialogDef;

/**
 * Card details, laid out the way a card is authored:
 * - Deed: reach σ with its rarity tier (badge, colored band bar, snap chips), then strength J capped by that
 *   reach, then stance h, then the card's live lint findings.
 * - Meaning: one row per axis. Ticking an axis or typing its coordinate makes the card about it; unticking
 *   removes it from both Location and Mask, so the two never disagree. The raw fields stay under Advanced.
 * A multi-card selection keeps the default layout.
 */
class FNarrativeDialogDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void BuildDeed(IDetailLayoutBuilder& DetailBuilder);
	void BuildMeaning(IDetailLayoutBuilder& DetailBuilder);

	/** Sets σ or J through its handle; an interactive run (a drag) becomes one undo step. */
	void SetFloat(const TSharedPtr<IPropertyHandle>& Handle, float Value, bool bInteractive);

	/** One undoable edit of Location and Mask together. */
	void EditMeaning(const FText& Description, TFunctionRef<void(UNarrativeDialogDef&)> Edit);
	void SetAxisCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value, bool bInteractive);

	TWeakObjectPtr<UNarrativeDialogDef> Card;
	TSharedPtr<IPropertyHandle> SigmaHandle;
	TSharedPtr<IPropertyHandle> StrengthHandle;
	TSharedPtr<IPropertyHandle> LocationHandle;
	TSharedPtr<IPropertyHandle> MaskHandle;

	/** Open while a slider or the reach bar is dragged. */
	TSharedPtr<FScopedTransaction> DragTransaction;
};
