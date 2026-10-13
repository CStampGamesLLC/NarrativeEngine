#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "UObject/SoftObjectPtr.h"

class IDetailLayoutBuilder;
class IPropertyUtilities;
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
 *
 * Edits go to the card object, never through property handles: a details refresh destroys the handles, and a
 * field losing focus while its row is torn down still commits. Handlers hold this customization weakly, and a
 * commit that changes nothing does nothing.
 */
class FNarrativeDialogDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void BuildDeed(IDetailLayoutBuilder& DetailBuilder);
	void BuildMeaning(IDetailLayoutBuilder& DetailBuilder);

	/** Opens the one transaction a drag (slider or reach bar) records as. */
	void BeginDrag(const FText& Description);

	/**
	 * One undoable edit of the named properties. A drag's steps write live without notifying, because a details
	 * refresh mid-drag would destroy the widget being dragged; the final step notifies once and closes the drag.
	 * Only BeginDrag opens a drag: an interactive step with none open is an edit of its own. Outside a drag, an
	 * edit HasChanged rejects is cancelled.
	 */
	void EditCard(const FText& Description, TConstArrayView<FName> Properties, bool bInteractive,
		TFunctionRef<void(UNarrativeDialogDef&)> Edit, TFunctionRef<bool(const UNarrativeDialogDef&)> HasChanged);

	/** σ or J. */
	void SetFloat(FName Property, const FText& Description, float Value, bool bInteractive);

	/** Location and Mask together; rebuilds the raw rows under Advanced when the set of axes changes. */
	void EditMeaning(const FText& Description, bool bInteractive, TFunctionRef<void(UNarrativeDialogDef&)> Edit);

	TWeakObjectPtr<UNarrativeDialogDef> Card;
	TWeakPtr<IPropertyUtilities> PropertyUtilities;

	/** Open while a slider or the reach bar is dragged. */
	TSharedPtr<FScopedTransaction> DragTransaction;
};
