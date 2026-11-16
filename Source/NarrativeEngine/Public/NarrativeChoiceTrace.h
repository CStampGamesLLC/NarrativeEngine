#pragma once

#include "CoreMinimal.h"
#include "NarrativeCampaign.h"
#include "NarrativeChoice.h"

/** A card as the trace shows it: copied out, so a trace outlives the asset and the world. */
struct NARRATIVEENGINE_API FNarrativeChoiceTraceCard
{
	FString AssetName;
	FString Phrase;
	FString Label;
	double Sigma = 0.0;
	double Stance = 0.0;
	int32 MaskAxes = 0;

	/** The card has no authored mask; the dealer read its typed axes instead (L13 should flag it). */
	bool bMaskInferred = false;

	/** Pattern 8: 0 when legal, else the failing check's number, its name and a detail ("You have no coffee"). */
	uint8 IllegalCheck = 0;
	FString IllegalCheckName;
	FString IllegalDetail;

	/** TierOf(σ): rarity and the Effort it costs. */
	FString Tier;
	int32 Effort = 0;

	/** Where the speaker's repertoire grants it: "default", "family", "card", or why not ("denied", "mechanic not granted"). */
	FString RepertoireSource;

	/** The card's strength J and its outcome policy. */
	double Strength = 0.0;
	FString OutcomePolicy;
};

/**
 * One evaluation of the choice rule and the deal that followed (TDD Patterns 9, 10, 25): every
 * input the rule read, every term it computed, and every roll. Diagnostics only; nothing reads a
 * trace back into the simulation.
 */
struct NARRATIVEENGINE_API FNarrativeChoiceTrace
{
	/** Counts every trace recorded since the log was made; never repeats. */
	uint64 Sequence = 0;

	double WorldSeconds = 0.0;
	int64 Tick = 0;

	FString Speaker;
	FString Addressee;

	/** The hand the player picks from, as opposed to a reply bubble. */
	bool bPlayerHand = false;

	/** Set when the rule couldn't run. Under Dialog the hand is then Silence alone (Pattern 9: never the legacy draw). */
	FString FallbackReason;

	/** The latched preset, the dealer and whether db.Narrative.Dealer overrode it (Pattern 25, P0-H). */
	FString Preset;
	FString Dealer;
	bool bDealerOverridden = false;

	/** "offer", "choice hand" or "choice" (a reply); and the Offer's sheet when one dealt. */
	FString Policy;
	NarrativeChoice::FOfferSheet Sheet;

	/** The pair's opportunity index this deal used (Pattern 9), and the conversation beat it fell in, with that beat's cause. */
	uint64 Opportunity = 0;
	int64 Beat = 0;
	FString BeatCause;

	/** Empty when the four filled; else what kept them short (Pattern 10). */
	FString ShortCause;

	/** r'_g per family rank when the Offer dealt (FEvaluation::Families order). */
	TArray<double> NoveltyScores;

	ENarrativeBeliefSource BeliefSource = ENarrativeBeliefSource::FirstImpression;

	/** Whether the stance term had a committed affinity to read; without one Ã reads 0. */
	bool bHasAffinity = false;

	NarrativeChoice::FParams Params;
	int32 HandSize = 0;

	/** The keyed draw site: (seed, speaker, addressee, Choice, RollIndex), lanes 2k and 2k+1 for slot k. */
	uint64 Seed = 0;
	uint64 RollIndex = 0;

	/** Parallel to Evaluation.Phrasings. */
	TArray<FNarrativeChoiceTraceCard> Cards;

	/** Indexed by family. */
	TArray<FString> FamilyNames;

	NarrativeChoice::FEvaluation Evaluation;
	TArray<NarrativeChoice::FDealSlot> Slots;

	/** Multi-line text for the log (db.Narrative.ChoiceTrace.Dump): the rolls, then every family by rank with its lead phrasing's terms. */
	FString Describe() const;
};

/**
 * The last N choice traces, oldest overwritten first (N from db.Narrative.ChoiceTrace.Capacity).
 * Game thread only. Pausing freezes what's held so a surprising deal can be scrubbed back to.
 */
class NARRATIVEENGINE_API FNarrativeChoiceTraceLog
{
public:
	static FNarrativeChoiceTraceLog& Get();

	/** Keeps Trace and stamps its Sequence. Dropped while paused. */
	void Record(FNarrativeChoiceTrace&& Trace);

	int32 Num() const { return Ring.Num(); }

	/** 0 is the oldest held trace, Num() - 1 the newest. */
	const FNarrativeChoiceTrace& GetOrdered(int32 Index) const;

	/** The held trace with this Sequence, if it hasn't been overwritten. */
	const FNarrativeChoiceTrace* FindBySequence(uint64 Sequence) const;

	/** The Sequence the next recorded trace will take. */
	uint64 GetNextSequence() const { return NextSequence; }

	int32 GetCapacity() const;

	bool IsPaused() const { return bPaused; }
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }

	void Clear();

private:
	/** Oldest first. TArray relocates bitwise, so dropping the front is a memmove of N headers. */
	TArray<FNarrativeChoiceTrace> Ring;

	uint64 NextSequence = 1;
	bool bPaused = false;
};
