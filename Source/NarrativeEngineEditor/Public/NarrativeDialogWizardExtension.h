#pragma once

#include "CoreMinimal.h"

class IDetailCategoryBuilder;
class UNarrativeDialogDef;
class UPackage;

/**
 * Lets a module the plugin cannot see (the game's) add questions to the new dialog card wizard and act on the answers.
 * One instance serves one wizard run, so it can hold that run's answers.
 */
class NARRATIVEENGINEEDITOR_API INarrativeDialogWizardExtension : public TSharedFromThis<INarrativeDialogWizardExtension>
{
public:
	virtual ~INarrativeDialogWizardExtension() = default;

	/** Adds rows under the wizard's own. Called once, when the wizard opens. */
	virtual void AddRows(IDetailCategoryBuilder& Category) = 0;

	/**
	 * The card was created and authored from the wizard. Runs inside the create's undo step, so anything this
	 * Modify()s is undone together with the card.
	 */
	virtual void OnCardCreated(UNarrativeDialogDef& Card) = 0;

	/**
	 * Packages OnCardCreated changed besides the card's own (a catalog it added a row to). Narrative Space's Save
	 * offers them with the card, so the two never go to disk apart.
	 */
	virtual void GatherModifiedPackages(TArray<UPackage*>& OutPackages) const
	{
	}
};

namespace NarrativeDialogWizardExtensions
{
	using FFactory = TFunction<TSharedRef<INarrativeDialogWizardExtension>()>;

	/** Registers a factory under Name, replacing any earlier one. Call from a module's StartupModule. */
	NARRATIVEENGINEEDITOR_API void Register(FName Name, FFactory Factory);

	/** Call from the registering module's ShutdownModule. */
	NARRATIVEENGINEEDITOR_API void Unregister(FName Name);

	/** One fresh instance from each registered factory, in name order so the wizard's rows keep their place. */
	NARRATIVEENGINEEDITOR_API TArray<TSharedRef<INarrativeDialogWizardExtension>> MakeAll();
}
