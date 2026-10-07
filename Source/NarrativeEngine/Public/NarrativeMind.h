#pragma once

#include "CoreMinimal.h"
#include "NarrativeIds.h"
#include "NarrativeMask.h"
#include "NarrativeStaticData.h"
#include "NarrativeVector.h"
#include "NarrativeMind.generated.h"

/** A mind's stance toward someone: the reading after the Schmitt trigger (spec §6.2). */
UENUM()
enum class ENarrativeStance : uint8
{
	None,
	Friendly,
	Hostile,
};

namespace NarrativeStance
{
	/* The trigger's states: -1 Hostile, 0 None, +1 Friendly. */
	inline int32 ToSign(ENarrativeStance Stance) { return Stance == ENarrativeStance::Hostile ? -1 : (Stance == ENarrativeStance::Friendly ? 1 : 0); }
	inline ENarrativeStance FromSign(int32 Sign) { return Sign < 0 ? ENarrativeStance::Hostile : (Sign > 0 ? ENarrativeStance::Friendly : ENarrativeStance::None); }
}

NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeStance Stance);

UENUM()
enum class ENarrativeLifeState : uint8
{
	Alive,
	Dead,
};

/**
 * A campaign entity (TDD §6 Entity): what the store knows of a character whether or not an actor
 * shows it. Positions are frozen at their authored values on the model presets (Pattern 24).
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeEntityRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FNarrativeEntityId Id;

	/* The definition it was made from, as a full asset identity. */
	UPROPERTY()
	FSoftObjectPath TemplateId;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	FVectorND Position;

	UPROPERTY()
	FNarrativeMask Mask;

	UPROPERTY()
	float Mass = 1.f;

	/* Its archetype, and where that archetype is authored to sit. */
	UPROPERTY()
	FSoftObjectPath Archetype;

	UPROPERTY()
	FVectorND ArchetypeLocation;

	/* The factions it wears, as the game names them. Stacked identities sum their biases, then clamp. */
	UPROPERTY()
	TArray<FName> Factions;

	UPROPERTY()
	ENarrativeLifeState LifeState = ENarrativeLifeState::Alive;
};

/**
 * The warning before a social strike (TDD Pattern 17): a narrative record on game time, saved with
 * its relation. Harm through the stance waits until a warning has visibly completed since the turn.
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeTell
{
	GENERATED_BODY()

	UPROPERTY()
	bool bActive = false;

	UPROPERTY()
	int64 StartTick = 0;

	/* The decision that turned Hostile. */
	UPROPERTY()
	int64 StartDecision = 0;

	UPROPERTY()
	bool bWarningSeen = false;

	UPROPERTY()
	int64 WarningSeenTick = 0;

	/* What moved the reading across θH, for the Mind panel: the last valuation before the turn, or 0 for temper alone. */
	UPROPERTY()
	uint64 CauseEventId = 0;

	/* This tell's identity, which a WarningSeen fact must name: a warning played for an earlier turn opens nothing. */
	uint64 GetIdentity() const { return bActive ? static_cast<uint64>(StartTick) + 1 : 0; }
};

/**
 * Why a mind must keep a relation however full it is (TDD Pattern 12): the subject is in its party, or
 * an active threat. The game sets these; they apply whether or not the relation exists yet.
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativePin
{
	GENERATED_BODY()

	UPROPERTY()
	bool bParty = false;

	/* The tick the threat expires after; below zero for none. */
	UPROPERTY()
	int64 ThreatUntilTick = -1;

	bool IsActive(int64 Tick) const { return bParty || ThreatUntilTick >= Tick; }
};

/** What a reading was made of, for the Mind panel (TDD Pattern 25). */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeReadingParts
{
	GENERATED_BODY()

	/* a_pos: likeness to the sampled belief, on the observer's axes. */
	UPROPERTY()
	double Position = 0.0;

	/* tanh R. */
	UPROPERTY()
	double TanhRegard = 0.0;

	/* Own bias plus faction biases, clamped. */
	UPROPERTY()
	double Bias = 0.0;

	UPROPERTY()
	double PositionWeight = 0.0;

	UPROPERTY()
	double RegardWeight = 0.0;
};

/**
 * What one mind holds about one entity (spec §4.1; TDD Pattern 12): a diagonal Gaussian belief, the
 * regard ledger entry, the OU state and decision index, and the cached decision every consumer reads.
 * Until P4 the belief stays at its first impression while regard moves.
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeRelation
{
	GENERATED_BODY()

	UPROPERTY()
	FNarrativeEntityId Subject;

	/* μ and diag Σ, on the observer's axes. */
	UPROPERTY()
	FVectorND Mean;

	UPROPERTY()
	FVectorND Variance;

	/* Where the belief started, so P4's corrections and the Mind panel can compare. */
	UPROPERTY()
	FVectorND PriorMean;

	/* R as of RegardTick; decayed lazily. Clipped to ±Loyalty. */
	UPROPERTY()
	double Regard = 0.0;

	UPROPERTY()
	int64 RegardTick = 0;

	/* z, the OU state carrying the belief sample, as of NoiseTick, keyed by axis identity. */
	UPROPERTY()
	FVectorND Noise;

	UPROPERTY()
	int64 NoiseTick = 0;

	/* Keyed draw index of the last noise draw: 0 is the stationary draw at creation, k the k-th step. */
	UPROPERTY()
	int64 NoiseIndex = 0;

	/* Decisions taken; the next decision's number. */
	UPROPERTY()
	int64 DecisionCount = 0;

	/* The committed decision: reading, stance, when, and a revision every decision bumps. */
	UPROPERTY()
	double Reading = 0.0;

	UPROPERTY()
	FNarrativeReadingParts Parts;

	UPROPERTY()
	ENarrativeStance Stance = ENarrativeStance::None;

	UPROPERTY()
	int64 AsOfTick = 0;

	UPROPERTY()
	uint32 Revision = 0;

	/* False until the first decision; Reading then holds the first impression's reading, for care. */
	UPROPERTY()
	bool bDecided = false;

	UPROPERTY()
	FNarrativeTell Tell;

	UPROPERTY()
	int64 CreatedTick = 0;

	UPROPERTY()
	int64 LastTouchedTick = 0;

	UPROPERTY()
	int32 Flips = 0;
};

/** One valuation a mind applied, for the Mind panel's "why does A hate B?" (TDD Pattern 25). */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeValuationRecord
{
	GENERATED_BODY()

	UPROPERTY()
	uint64 EventId = 0;

	UPROPERTY()
	uint64 ValuationUnitId = 0;

	UPROPERTY()
	int64 Tick = 0;

	UPROPERTY()
	FNarrativeEntityId Instigator;

	UPROPERTY()
	FNarrativeEntityId Target;

	UPROPERTY()
	FString Label;

	UPROPERTY()
	double Care = 0.0;

	UPROPERTY()
	double Approval = 0.0;

	UPROPERTY()
	double TargetMass = 1.0;

	/* ΔR before the batch's cap. */
	UPROPERTY()
	double RegardDelta = 0.0;

	/* Stored regard before and after the batch this valuation was summed into. */
	UPROPERTY()
	double RegardBefore = 0.0;

	UPROPERTY()
	double RegardAfter = 0.0;
};

/** A mind (spec §4.1; TDD Pattern 12): one observer's sparse, subjective registry. */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeMind
{
	GENERATED_BODY()

	static constexpr int32 MaxValuationRecords = 20;

	/* |R| at or above this, decayed to now, is a live grudge or debt: forgotten after neutral relations. */
	static constexpr double LiveRegard = 0.05;

	UPROPERTY()
	FNarrativeEntityId Observer;

	/* Copied from the definition when the mind was made; editor tuning re-syncs it. */
	UPROPERTY()
	FNarrativeMindSheet Sheet;

	/* Where this mind believes each archetype sits (TDD Pattern 13). */
	UPROPERTY()
	TMap<FSoftObjectPath, FVectorND> ArchetypeBeliefs;

	UPROPERTY()
	TMap<FNarrativeEntityId, FNarrativeRelation> Relations;

	/* Promises and debts that must outlive capacity. Bounded and separate; empty until P2 gives it entries. */
	UPROPERTY()
	TArray<FNarrativeEntityId> Obligations;

	/* Party members and active threats, by subject: their relations are protected (TDD Pattern 12). */
	UPROPERTY()
	TMap<FNarrativeEntityId, FNarrativePin> Pins;

	/* Its decisions fall at Phase + k·Interval: a stable offset from its id, watched or not (TDD Pattern 18). */
	UPROPERTY()
	int64 PhaseTicks = 0;

	UPROPERTY()
	int64 IntervalTicks = 1000;

	UPROPERTY()
	int64 NextDecisionTick = 0;

	UPROPERTY()
	int64 LastCommittedTick = -1;

	/* Units already valued, so a duplicate completion never doubles a grievance. */
	UPROPERTY()
	TSet<uint64> ValuedUnits;

	/* The last valuations, newest last. */
	UPROPERTY()
	TArray<FNarrativeValuationRecord> RecentValuations;

	/* Optional relations refused because protected ones filled the budget: a capacity failure. */
	UPROPERTY()
	int32 CapacityFailures = 0;

	bool IsPinned(const FNarrativeEntityId& Subject, int64 Tick) const
	{
		const FNarrativePin* Pin = Pins.Find(Subject);
		return Pin && Pin->IsActive(Tick);
	}

	/**
	 * What capacity may never forget (TDD Pattern 12): a party member, an active threat, or a pending
	 * warning. A grudge is not protection; it only makes a relation the last of the unprotected to go.
	 */
	bool IsProtected(const FNarrativeRelation& Relation, int64 Tick) const
	{
		return Relation.Tell.bActive || IsPinned(Relation.Subject, Tick);
	}
};

/**
 * ReadRelation's answer (TDD §6): an immutable snapshot of the last decision. Reads never draw,
 * decide or advance time. A missing relation is explicit, never a silent fallback.
 */
struct NARRATIVEENGINE_API FNarrativeRelationSnapshot
{
	bool bMissing = true;
	int64 AsOfTick = 0;
	double Reading = 0.0;
	ENarrativeStance Stance = ENarrativeStance::None;
	uint32 Revision = 0;
	bool bDecided = false;
	/* Regard decayed to the frontier, never the stored value of some earlier tick. */
	double Regard = 0.0;
	bool bTellActive = false;
	bool bWarningSeen = false;
};
