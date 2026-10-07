#pragma once

#include "CoreMinimal.h"

class UNarrativeDialogDef;
class UNarrativeEntityDef;
struct FNarrativeMindSheet;

enum class ENarrativeLintSeverity : uint8
{
	Warning,
	Error,
};

/** One finding against one asset. Rule is the spec's lint number (§13.5), e.g. "L6". */
struct NARRATIVEENGINE_API FNarrativeLintIssue
{
	FString Rule;
	ENarrativeLintSeverity Severity = ENarrativeLintSeverity::Warning;
	FString Asset;
	FString Message;

	FString ToString() const;
};

/** The pricing tier a card's reach puts it in (spec §13.4). */
	enum class ENarrativeTier : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary,
};

/**
 * The schema lints that land with masks in P1 (TDD Pattern 26): L1 neutral semantics, L6 strength
 * cap, L7 mass floor, L8 weights and fallback, L11 reach floor, L13 explicit masks (including null
 * and repeated axes), L14 ability tags consistent with meaning, plus a sanity check that a mind's
 * stance bands and capacities make sense. The model's failure modes are authoring mistakes; these
 * catch them when the asset is saved rather than weeks later as vague playtest feel. The catalog
 * lints (L2, L3, L5, L10) arrive with the choice rule in P3.
 *
 * L14 reads the project's tag convention: a card carrying Dialog.Mechanic.X runs mechanic X, so it
 * belongs to Dialog.Family.X. A tag copied with a card (Appendix A: the intimidation cards carrying
 * the coffee's GiveItem) shows up as a family mismatch.
 */
namespace NarrativeLints
{
	/** σ_min (L11): evidence below this reach is finer than the sampler can resolve. */
	constexpr double MinReach = 1.0;

	/** L13 flags masks wider than this: they bring back the uncertainty penalty. */
	constexpr int32 MaxMaskAxes = 3;

	NARRATIVEENGINE_API ENarrativeTier TierForReach(double Sigma);
	NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeTier Tier);

	/** J ≤ min(tier cap, 1.8/σ). Legendary cards are exempt (they canonize instead) and return +inf. */
	NARRATIVEENGINE_API double StrengthCap(double Sigma);

	NARRATIVEENGINE_API void LintCard(const UNarrativeDialogDef& Card, TArray<FNarrativeLintIssue>& OutIssues);
	NARRATIVEENGINE_API void LintEntity(const UNarrativeEntityDef& Entity, TArray<FNarrativeLintIssue>& OutIssues);

	/** L14 alone, for callers that lint a card's mechanics without its geometry. */
	NARRATIVEENGINE_API void LintMechanics(const UNarrativeDialogDef& Card, TArray<FNarrativeLintIssue>& OutIssues);

	/** The stance bands must leave room between them: θH + δ < θF - δ. */
	NARRATIVEENGINE_API bool AreStanceBandsValid(const FNarrativeMindSheet& Sheet);
}
