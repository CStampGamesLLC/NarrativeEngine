#pragma once

#include "CoreMinimal.h"

/**
 * The choice rule (spec §3.2, §5.1-5.2; TDD Patterns 7, 9, 10): idea first, then wording, with
 * silence as an explicit outcome. Pure numerics over plain arrays, like NarrativeMath; the dialog
 * component gathers the inputs from the store and keeps the trace.
 *
 *   u_y  = -||x_B - l_y||²_M / σ²  +  (1/β) ln E_b[K_y(x_T)^(βη)]  +  λ h_y Ã_B→T
 *          resonance                  relevance under the belief      stance
 *   w̄_g = mean over g of e^(β u_y),   P(g) = w̄_g / (e^(β u_∅) + Σ w̄_g'),   P(y|g) = e^(β u_y) / Σ_g e^(β u_y')
 *
 * Local deviation from the spec: "over g" means over g's phrasings within the speaker's reach
 * (K ≥ FParams::ReachFloor), so an unreachable rewording never drags its idea below silence. A family
 * with nothing in reach is scored over all its phrasings, for the record only.
 *
 * Motive (ν) and echo (ρ) stay 0 until their inputs exist (Pattern 9). Legality and directional
 * repulsion (Patterns 8, 10) are not ported yet: every card counts as legal, and the deal draws
 * families by P(g) alone.
 */
namespace NarrativeChoice
{
	/** β, η, λ, u_∅ (spec Appendix: 1, .5, 1, -3) and the deal's reach floor (§5.2). */
	struct FParams
	{
		double Beta = 1.0;
		double Eta = 0.5;
		double Lambda = 1.0;
		double SilenceUtility = -3.0;

		/** A family is dealable only when one of its phrasings reaches the speaker this well: K ≥ 0.1 (§3.1). */
		double ReachFloor = 0.1;
	};

	/** One phrasing's inputs, every array on the card's own mask axes, in mask order. */
	struct FPhrasing
	{
		/** Index of its idea; phrasings sharing one share a family (Pattern 7). */
		int32 Family = 0;

		/** x_B, the speaker's own position. */
		TArray<double> SpeakerX;

		/** l_y. */
		TArray<double> Location;

		/** μ and diag Σ of the speaker's belief about the addressee: never the addressee's true position. */
		TArray<double> BeliefMean;
		TArray<double> BeliefVariance;

		double Sigma = 1.0;

		/** h_y: +1 for what is done to friends, -1 for what is done to foes. */
		double Stance = 0.0;
	};

	/** u_y term by term, and where it lands. */
	struct FPhrasingTerms
	{
		/** ||x_B - l_y||²_M and the resonance term it makes, -d²/σ². */
		double SpeakerSqDist = 0.0;
		double Resonance = 0.0;

		/** ||μ - l_y||²_M and mean belief variance on the mask, for reading the relevance term. */
		double BeliefSqDist = 0.0;
		double BeliefMeanVariance = 0.0;

		/** (1/β) ln E_b[K^(βη)]. */
		double Relevance = 0.0;

		/** λ h_y Ã. */
		double StanceTerm = 0.0;

		double Utility = 0.0;

		/** ln |g|, the family share every scored phrasing of an idea pays so rephrasings split one idea's weight. 0 when out of reach in a family that has one in reach. */
		double FamilyLogSize = 0.0;

		/** K_y(x_B): the speaker's own reach, gating the deal. */
		double SpeakerReach = 0.0;

		/** P(y | g) and P(g) P(y | g). Both 0 for a phrasing left out of its family's scoring. */
		double ProbabilityInFamily = 0.0;
		double Probability = 0.0;
	};

	enum class EFamilyEligibility : uint8
	{
		Eligible,
		/** No phrasing reaches the speaker to FParams::ReachFloor. */
		OutOfReach,
		/** P(g) ≤ P(∅): saying nothing beats this idea. */
		BelowSilence,
	};

	NARRATIVEENGINE_API const TCHAR* LexToString(EFamilyEligibility Eligibility);

	struct FFamilyScore
	{
		int32 Family = 0;

		/** Indices into the evaluated phrasings, by P(y | g) descending. */
		TArray<int32> Phrasings;

		/** ln w̄_g, over the phrasings in reach (all of them when none is). */
		double LogMeanWeight = 0.0;

		double Probability = 0.0;
		double MaxSpeakerReach = 0.0;
		EFamilyEligibility Eligibility = EFamilyEligibility::Eligible;
	};

	struct FEvaluation
	{
		/** Parallel to the input phrasings. */
		TArray<FPhrasingTerms> Phrasings;

		/** By P(g) descending, ties by family index: a family's rank is its position here, plus one. */
		TArray<FFamilyScore> Families;

		/** Ã_B→T as the stance term read it. */
		double Reading = 0.0;

		/** β u_∅, and ln(e^(β u_∅) + Σ w̄_g). */
		double SilenceLogWeight = 0.0;
		double LogPartition = 0.0;
		double SilenceProbability = 0.0;

		/** The family's position in Families, or INDEX_NONE. */
		int32 FindRank(int32 Family) const
		{
			return Families.IndexOfByPredicate([Family](const FFamilyScore& Score) { return Score.Family == Family; });
		}
	};

	/** ln E_{x~N(μ, diag v)}[K(x)^p] in closed form (spec §5.1, core.log_expected_reach_pow, diagonal Σ on the mask). */
	NARRATIVEENGINE_API double LogExpectedReachPow(TConstArrayView<double> Mean, TConstArrayView<double> Variance, TConstArrayView<double> Location,
		double Sigma, double Power);

	/** Scores every phrasing and family. Reading is Ã_B→T, the speaker's committed reading of the addressee (Pattern 18). */
	NARRATIVEENGINE_API FEvaluation Evaluate(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Reading);

	/** Inverse CDF in the given order: the first index whose running sum exceeds U Σw. INDEX_NONE when nothing weighs anything. */
	NARRATIVEENGINE_API int32 PickByRoll(TConstArrayView<double> Weights, double U);

	/** One card of a dealt hand, or a reply: which idea and wording, and the two rolls that chose them. */
	struct FDealSlot
	{
		/** Index into FEvaluation::Families; Families[FamilyRank].Family names the idea. INDEX_NONE when a reply rolled silence. */
		int32 FamilyRank = INDEX_NONE;
		int32 Phrasing = INDEX_NONE;

		/** Uniforms in (0, 1). */
		double FamilyRoll = 0.0;
		double PhrasingRoll = 0.0;

		/** The families still in the draw at this slot, best first, and the weights the roll ran over. */
		TArray<int32> Candidates;
		TArray<double> CandidateWeights;

		/** P(∅) when silence was in the roll, ahead of Candidates: a reply's. A hand offers Silence apart, so its cards hold 0. */
		double SilenceWeight = 0.0;

		/** Where the roll landed among Candidates (INDEX_NONE on silence), and among the phrasings of the picked family (0 = favourite). */
		int32 CandidateIndex = INDEX_NONE;
		int32 PhrasingIndex = INDEX_NONE;

		bool IsSilence() const { return FamilyRank == INDEX_NONE; }
	};

	/**
	 * Deals up to HandSize eligible families without replacement (§5.2), each by one roll over the
	 * remaining families ranked by P(g), then a phrasing by a second roll over P(y | g). Stops on
	 * exhaustion and never pads. Uniform(Lane) supplies the draws: lanes 2k and 2k+1 for slot k.
	 */
	NARRATIVEENGINE_API TArray<FDealSlot> Deal(const FEvaluation& Evaluation, int32 HandSize, TFunctionRef<double(uint64 Lane)> Uniform);

	/** Whether a reply can say this idea: it is in the speaker's reach. Beating silence is the hand's bar, not the speaker's. */
	NARRATIVEENGINE_API bool CanReply(const FFamilyScore& Family);

	/**
	 * A speaker's own choice (Pattern 9), as a reply makes it: one roll over silence and every family
	 * it can say, by P(∅) and P(g), then a phrasing by P(y | g). Silence wins its true share, so a
	 * family that barely beats it is said only about half the time. Lanes 0 and 1, as for a hand's first card.
	 */
	NARRATIVEENGINE_API FDealSlot Reply(const FEvaluation& Evaluation, TFunctionRef<double(uint64 Lane)> Uniform);
}
