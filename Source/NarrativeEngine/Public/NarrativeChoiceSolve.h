#pragma once

#include "CoreMinimal.h"
#include "NarrativeChoice.h"

/**
 * The choice rule run backwards (TDD Pattern 25's tuning column): given what a designer wants one card to
 * score, where must its location l_y sit? Everything else the rule read (the speaker, the belief, σ, the
 * other cards) is held as it was, so the answer is the smallest move of this one card that hits the target.
 *
 * Every target is a function of l_y alone once the rest is held:
 *
 *   resonance  -||x_B - l||² / σ²                    level sets: spheres about the speaker x_B
 *   relevance  C - Σ_i w_i (μ_i - l_i)²               level sets: axis-aligned ellipsoids about the belief μ,
 *              w_i = η / (σ² (1 + 2 β η v_i / σ²))     tighter on axes the speaker is surer of
 *   utility    their sum plus the stance term         a concave quadratic, peaking at l* (MaxUtilityLocation)
 *   P(y), P(g) monotone in u_y while the card's family keeps the same phrasings in reach
 *
 * Resonance and relevance alone are solved as the nearest point on their level set. Holding the other term too
 * means landing on the meeting of a sphere and an ellipsoid: a min-norm Newton projection from the card, and when
 * that stalls, a path along the ellipsoid between its nearest and farthest points from the speaker. The extremes
 * also decide feasibility exactly, so "can't hold both" is a proof, not a failure to converge.
 *
 * Utility and the probabilities are searched on lines through the card: toward l*, and toward the best spot still
 * in the speaker's reach. A probability jumps where the card crosses the reach floor (an out-of-reach wording
 * leaves its family's weight), so each line is cut at those crossings and at its utility peak; between cuts the
 * target is monotone, and each piece is bisected on its own. The nearest crossing wins; with none, the closest value.
 * Probabilities are probed incrementally: only the moved card's family is rescored per step.
 *
 * Probabilities here are the Choice rule's baseline P(y) and P(g). The Offer (verbs, reserved slots, novelty,
 * repulsion) deals from them but is not re-run.
 *
 * Pure numerics like NarrativeChoice; the Dialog Choice Debugger draws and applies the answer.
 */
namespace NarrativeChoice
{
	enum class ESolveTarget : uint8
	{
		Resonance,
		Relevance,
		Utility,
		/** P(y) = P(g) P(y | g): the chance this card is the one rolled. */
		CardProbability,
		/** P(g): the chance its idea is rolled, any wording. */
		FamilyProbability,
	};

	NARRATIVEENGINE_API const TCHAR* LexToString(ESolveTarget Target);

	struct FSolveRequest
	{
		/** Index into the phrasings handed to the solver. */
		int32 Phrasing = INDEX_NONE;

		ESolveTarget Target = ESolveTarget::Resonance;
		double Value = 0.0;

		/** Resonance or relevance only: keep the other of the two at its current value while moving this one. */
		bool bHoldOther = false;

		/** On the card's mask, in mask order: true keeps that axis where it is. Empty locks nothing. */
		TArray<bool> LockedAxes;
	};

	struct FSolveResult
	{
		/** The target was met to tolerance. False means Location is the closest the free axes allow, and Note says why. */
		bool bSolved = false;

		/** Empty when solved cleanly; else a sentence for the designer ("resonance can't exceed 0", "every axis is locked"). */
		FString Note;

		/** l_y', on the card's mask in mask order. */
		TArray<double> Location;

		/** The target's measure at Location, and at the original location. */
		double Achieved = 0.0;
		double Before = 0.0;

		/** The whole rule re-run with the card at Location, so every other card's shift can be shown. */
		FEvaluation Evaluation;
	};

	/** The target's value for one phrasing in an evaluation. */
	NARRATIVEENGINE_API double MeasureTarget(ESolveTarget Target, const FEvaluation& Evaluation, int32 Phrasing);

	/** The location that maximizes u_y with the locked axes held: the precision-weighted blend of x_B and μ on each free axis. */
	NARRATIVEENGINE_API TArray<double> MaxUtilityLocation(const FParams& Params, const FPhrasing& Phrasing, TConstArrayView<bool> LockedAxes = {});

	/**
	 * The location that maximizes u_y among those the speaker reaches (K ≥ ReachFloor, a ball about x_B), locked axes held.
	 * MaxUtilityLocation when that is already in reach. False (and MaxUtilityLocation) when the locked axes alone put the
	 * card out of reach.
	 */
	NARRATIVEENGINE_API bool MaxUtilityLocationInReach(const FParams& Params, const FPhrasing& Phrasing, TConstArrayView<bool> LockedAxes, TArray<double>& OutLocation);

	/** The rule re-run with one phrasing moved to Location. */
	NARRATIVEENGINE_API FEvaluation EvaluateMoved(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity, int32 Phrasing,
		TConstArrayView<double> Location);

	/** Where the phrasing must sit to meet the request; see the header comment for how each target is solved. */
	NARRATIVEENGINE_API FSolveResult SolveLocation(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity, const FSolveRequest& Request);
}
