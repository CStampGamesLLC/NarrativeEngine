#pragma once

#include "CoreMinimal.h"

/** Narrative time in integer ticks of game time (TDD Pattern 3). Timebase 1 ms. */
namespace NarrativeTime
{
	constexpr int64 TicksPerSecond = 1000;

	inline double ToSeconds(int64 Ticks) { return static_cast<double>(Ticks) / TicksPerSecond; }
	inline int64 FromSeconds(double Seconds) { return FMath::RoundToInt64(Seconds * TicksPerSecond); }
}

/**
 * Who submits facts. Each keeps a sequence for the canonical order and a watermark for sealing.
 * The ID is part of the commit order, so append, never reorder.
 */
enum class ENarrativeProducer : uint8
{
	Dialog,
	Gifts,
	Combat,
	Roster,
	Scheduler,
	Workers,
	/* Meetings and witnessed warnings: what a mind perceives without anyone acting on it. */
	Perception,
	/* Gym and test fixtures. */
	Fixture,
	Num
};

NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeProducer Producer);

/**
 * One clock for the narrative, driven by game time (spec §9.1; TDD Pattern 3). Slow motion slows
 * minds with their bodies and pause stops both. Game time advances the horizon; committed work
 * advances the frontier; their difference is lag, which is measured and worked off, never deleted.
 *
 * A tick seals once the horizon has passed it and every producer's watermark has reached it; only
 * sealed ticks commit. A producer with nothing in flight confirms implicitly through the last whole
 * tick; one with work in flight holds its watermark at the oldest occurrence it still owes.
 */
struct NARRATIVEENGINE_API FNarrativeClock
{
	FNarrativeClock();

	/** The tick now in progress: a fact a producer completes now is stamped with it. */
	int64 GetHorizon() const { return Horizon; }

	/** Seconds of game time not yet a whole tick. */
	double GetRemainder() const { return Remainder; }

	/** The last committed tick; -1 before anything committed. */
	int64 GetFrontier() const { return Frontier; }

	/** The newest tick that may commit now: the horizon has passed it and every watermark reached it. */
	int64 GetSealedThrough() const;

	/** Ticks the frontier trails the newest tick game time has finished. */
	int64 GetLagTicks() const { return FMath::Max<int64>(0, (Horizon - 1) - Frontier); }
	int64 GetMaxLagTicks() const { return MaxLagTicks; }
	void ResetLagStats() { MaxLagTicks = GetLagTicks(); }

	/** Advance game time by the world's dilated delta, carrying the sub-tick remainder. */
	void Advance(double GameDeltaSeconds);

	/** Only the commit loop moves the frontier, and only forward over sealed ticks. */
	void CommitThrough(int64 Tick);

	/** A producer with work in flight: nothing it owes from Tick on may commit until it confirms. */
	void HoldWatermark(ENarrativeProducer Producer, int64 OldestOwedTick);

	/** Every fact Producer owes through Tick is submitted. Releases a hold once it passes the horizon. */
	void ConfirmThrough(ENarrativeProducer Producer, int64 Tick);

	/**
	 * A producer that tracks its own oldest debt (open combat units): it owes from OldestOwedTick on, no
	 * more and no less, and keeps holding as game time passes until it releases.
	 */
	void SetHold(ENarrativeProducer Producer, int64 OldestOwedTick);

	/** Nothing in flight any more: back to confirming implicitly through the last whole tick. */
	void ReleaseHold(ENarrativeProducer Producer);

	/** The producer's watermark: its held tick, or the last whole tick when it holds nothing. */
	int64 GetWatermark(ENarrativeProducer Producer) const;
	bool IsHeld(ENarrativeProducer Producer) const;

	/** The next sequence number for Producer's canonical order; 0 for anything that isn't a producer. */
	uint64 NextSequence(ENarrativeProducer Producer);
	uint64 GetSequence(ENarrativeProducer Producer) const;

	/** New game: back to tick zero, nothing held, nothing committed. */
	void Reset();

	static constexpr int32 NumProducers = static_cast<int32>(ENarrativeProducer::Num);
	static bool IsValidProducer(ENarrativeProducer Producer) { return static_cast<int32>(Producer) < NumProducers; }

private:
	/** Producer's slot in the per-producer arrays, or INDEX_NONE (with an ensure) for anything out of range. */
	static int32 ToIndex(ENarrativeProducer Producer);

	int64 Horizon = 0;
	double Remainder = 0.0;
	int64 Frontier = -1;
	int64 MaxLagTicks = 0;

	int64 Watermarks[NumProducers];
	bool Held[NumProducers];
	uint64 Sequences[NumProducers];
};
