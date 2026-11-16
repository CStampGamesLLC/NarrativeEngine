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
 * Motive (ν) and echo (ρ) stay 0 until their inputs exist (Pattern 9). Legality (Pattern 8) is an
 * input: an illegal phrasing is scored for the record but never enters its family's weight, and a
 * family with no legal phrasing is out of the partition altogether. Two policies deal from one
 * evaluation (Pattern 10): Choice (Reply, and Deal for the Choice hand) and the Offer (a party
 * member's hand: verbs, reserved slots, novelty and directional repulsion).
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

		/** 0 when the speaker may say it (Pattern 8); otherwise the 1-based number of the legality check that failed. */
		uint8 IllegalCheck = 0;

		/** ℓ_y - x̂_L on every axis of the space, for the Offer's directional repulsion (spec §5.2). Empty: no repulsion. */
		TArray<double> Direction;

		bool IsLegal() const { return IllegalCheck == 0; }
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
		/** No phrasing of it is legal for this speaker and addressee (Pattern 8): out of the partition. */
		Illegal,
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

		/** For an Illegal family, the earliest check that removed its phrasings (the chain's order); 0 otherwise. */
		uint8 IllegalCheck = 0;

		/** Its phrasings that are legal, in reach or not. */
		int32 LegalPhrasings = 0;
	};

	struct FEvaluation
	{
		/** Parallel to the input phrasings. */
		TArray<FPhrasingTerms> Phrasings;

		/** By P(g) descending, ties by family index: a family's rank is its position here, plus one. */
		TArray<FFamilyScore> Families;

		/** Ã_B→T as the stance term read it. */
		double Affinity = 0.0;

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

	/** Scores every phrasing and family. Affinity is Ã_B→T, the speaker's committed affinity of the addressee (Pattern 18). */
	NARRATIVEENGINE_API FEvaluation Evaluate(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity);

	/** Inverse CDF in the given order: the first index whose running sum exceeds U Σw. INDEX_NONE when nothing weighs anything. */
	NARRATIVEENGINE_API int32 PickByRoll(TConstArrayView<double> Weights, double U);

	/** What a dealt card is in its hand (Pattern 10; the trace's slot kind). */
	enum class EDealSlotKind : uint8
	{
		/** Drawn by the rule: a Choice hand's card, or one of the Offer's draws. */
		Draw,
		/** A speaker's own choice: a reply or a bark. */
		Reply,
		/** A legal mechanic family, always offered outside the four (D7). */
		Verb,
		/** The best legal warming family (h = +1, J > 0). */
		ReservedWarm,
		/** The best legal provoking family (h = -1, J > 0). */
		ReservedProvoke,
	};

	NARRATIVEENGINE_API const TCHAR* LexToString(EDealSlotKind Kind);

	/** One card of a dealt hand, or a reply: which idea and wording, and the two rolls that chose them. */
	struct FDealSlot
	{
		EDealSlotKind Kind = EDealSlotKind::Draw;

		/** The fill-order slot whose lanes (2s, 2s + 1) its rolls used; INDEX_NONE for a verb, which rolls nothing. */
		int32 LaneSlot = INDEX_NONE;

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

	/**
	 * The Offer's sheet (Pattern 10's hand sheet): data versioned with the preset, never code. ChoiceHand() is the
	 * reduction case of §4.4: with it the Offer deals exactly what Deal deals.
	 */
	struct FOfferSheet
	{
		int32 HandSize = 4;
		bool bVerbs = true;
		bool bReservedSlots = true;
		/** Spec §5.2's directional repulsion between the cards of one hand. */
		bool bRepulsion = true;
		/** β_hand; τ = β_hand / β is derived. */
		double HandBeta = 1.0;
		/** W: how many of the pair's last offers novelty remembers. */
		int32 NoveltyWindow = 3;
		/** ν: each recent offer of a family multiplies its draw weight by this. 1 turns novelty off. */
		double NoveltyDiscount = 0.5;

		/** τ = ν = 1, no verbs, no reserved slots, no repulsion: today's Choice hand. */
		static FOfferSheet ChoiceHand()
		{
			FOfferSheet Sheet;
			Sheet.bVerbs = false;
			Sheet.bReservedSlots = false;
			Sheet.bRepulsion = false;
			Sheet.NoveltyDiscount = 1.0;
			return Sheet;
		}
	};

	/** What the Offer needs to know of each family beyond the evaluation, indexed by FPhrasing::Family. */
	struct FOfferFamily
	{
		/** A mechanic family: dealt as a verb when the sheet has verbs. */
		bool bVerb = false;

		/** +1 warming, -1 provoking: every legal phrasing has J > 0 and this h. 0 for neither or mixed (never reserved). */
		int32 Sign = 0;

		/** n_g: how many of the pair's last W offers held this family. */
		int32 Novelty = 0;

		/** Ties among reserved candidates break by this, ascending. */
		FString Id;
	};

	/** One Offer, slot by slot, with what kept it short. */
	struct FOfferResult
	{
		/** Verbs first, then the four in fill order (warm, provoke, draws). */
		TArray<FDealSlot> Slots;

		/** r'_g per family rank (FEvaluation::Families order): ln w̄_g + n_g ln ν. -inf for a family out of the pool. */
		TArray<double> NoveltyScores;

		/** Empty when the four filled; else what kept them short: "legality", "repertoire", "reach", "below silence" or "pool exhausted". */
		FString ShortCause;

		int32 NumVerbs() const { return Slots.FilterByPredicate([](const FDealSlot& Slot) { return Slot.Kind == EDealSlotKind::Verb; }).Num(); }
	};

	/** The legality check numbered 1 in Pattern 8's chain: a family the speaker's repertoire doesn't grant. */
	constexpr uint8 RepertoireCheck = 1;

	/**
	 * A party member's hand (Pattern 10), in the contract's order: score, verbs, novelty, reserved slots, draws.
	 * Families indexes FOfferFamily by FPhrasing::Family. Phrasings supplies the repulsion directions. Uniform(Lane)
	 * draws as Deal's does: lanes 2s and 2s + 1 for fill-order slot s, where the warm slot is 0, the provoke slot 1
	 * and draws start at 2 (at 0 when the sheet has no reserved slots, so the reduction case keeps Deal's lanes).
	 */
	NARRATIVEENGINE_API FOfferResult Offer(const FParams& Params, const FOfferSheet& Sheet, const FEvaluation& Evaluation, TConstArrayView<FPhrasing> Phrasings,
		const TMap<int32, FOfferFamily>& Families, TFunctionRef<double(uint64 Lane)> Uniform);

	/**
	 * A phrasing's repulsion direction d_y = M_y (ℓ_y - x̂_L) (spec §5.2): from the speaker's belief about the listener
	 * (x̂_L, never the listener's true position nor the speaker's own) to the card, on the card's own mask and zero
	 * elsewhere, so cards about different axes never repel. Location and Belief are gathered on the mask, in mask order;
	 * AxisSlots places each mask axis in a shared axis table of NumAxes.
	 */
	NARRATIVEENGINE_API TArray<double> MaskedDirection(TConstArrayView<int32> AxisSlots, TConstArrayView<double> Location, TConstArrayView<double> Belief, int32 NumAxes);

	/** ρ_g against the cards already in a hand: Π (1 - max(0, cos(d_g, d_g'))²) over the hand's directions (spec §5.2). */
	NARRATIVEENGINE_API double Repulsion(TConstArrayView<double> Direction, TConstArrayView<TArray<double>> Hand);

	/** Whether a reply can say this idea: it is in the speaker's reach. Beating silence is the hand's bar, not the speaker's. */
	NARRATIVEENGINE_API bool CanReply(const FFamilyScore& Family);

	/**
	 * A speaker's own choice (Pattern 9), as a reply makes it: one roll over silence and every family
	 * it can say, by P(∅) and P(g), then a phrasing by P(y | g). Silence wins its true share, so a
	 * family that barely beats it is said only about half the time. Lanes 0 and 1, as for a hand's first card.
	 */
	NARRATIVEENGINE_API FDealSlot Reply(const FEvaluation& Evaluation, TFunctionRef<double(uint64 Lane)> Uniform);

	/** A wild card's draw: which subject filled it, and the weights the roll ran over. */
	struct FSubjectPick
	{
		/** Index into the candidates, or INDEX_NONE when there were none. */
		int32 Index = INDEX_NONE;

		/** e^(β u) per candidate, 0 for one left out because others were in reach. */
		TArray<double> Weights;

		/** Whether the roll ran over candidates in the speaker's reach, rather than over all of them because none was. */
		bool bInReach = false;

		double Roll = 0.0;

		/** Each candidate scored as its own idea with no stance, for the terms behind the weights. */
		FEvaluation Evaluation;
	};

	/**
	 * Fills a wild card on a card already dealt (Pattern 9, applied to what the card names rather than
	 * to the card): u = resonance with the speaker + relevance under the speaker's belief about the
	 * addressee. Stance and silence take no part; the card already carries one and ruled out the other.
	 * Candidates' Family and Stance are ignored.
	 *
	 * The roll runs over e^(β u) of the candidates in the speaker's reach (K ≥ ReachFloor), or over all of
	 * them when none is: the card is already said, so its blank is always filled.
	 */
	NARRATIVEENGINE_API FSubjectPick PickSubject(const FParams& Params, TConstArrayView<FPhrasing> Candidates, double U);
}
