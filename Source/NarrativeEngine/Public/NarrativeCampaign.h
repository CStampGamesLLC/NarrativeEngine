#pragma once

#include "CoreMinimal.h"
#include "NarrativeClock.h"
#include "NarrativeEvent.h"
#include "NarrativeIds.h"
#include "NarrativeMind.h"

class UNarrativeEntityDef;

NARRATIVEENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogNarrativeTrace, Log, All);

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnNarrativeStanceDecided, const FNarrativeEntityId& /*Observer*/, const FNarrativeEntityId& /*Subject*/,
	ENarrativeStance /*Old*/, ENarrativeStance /*New*/, int64 /*Tick*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNarrativeTell, const FNarrativeEntityId& /*Observer*/, const FNarrativeEntityId& /*Subject*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNarrativeEventCommitted, const FNarrativeEvent&);

/** Where FNarrativeCampaign::ReadBelief's answer came from. */
enum class ENarrativeBeliefSource : uint8
{
	/** Every axis from the relation. */
	Relation,
	/** The relation, with the axes it doesn't hold read as the first impression. */
	RelationAndFirstImpression,
	/** No relation yet: the first impression a meeting would form. */
	FirstImpression,
	/** Observer and subject are one: its own position. */
	Self,
};

NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeBeliefSource Source);

/** What a campaign is, so saves and telemetry can name it (TDD §6 CampaignHeader). */
struct NARRATIVEENGINE_API FNarrativeCampaignHeader
{
	static constexpr int32 ModelVersion = 1;
	static constexpr int32 SaveSchemaVersion = 1;

	uint64 Seed = 20260929;
	FString PresetId;
	uint32 StoreGeneration = 0;
	/* Runtime spawns allocate ids from here. */
	uint32 IdNamespace = 0;
	uint64 NextRuntimeId = 1;
	/* Activations (a delivered line, a swing, a gift) take ids from here, so they never repeat within the
	 * campaign however many worlds come and go: the dedup history lives exactly as long. */
	uint64 LastActivationId = 0;
};

/** Counters the Mind panel and the determinism tests read. */
struct NARRATIVEENGINE_API FNarrativeCampaignStats
{
	/* A fact for a sealed tick: a producer defect. Shipping recovers; tests fail on any. */
	int32 LateFacts = 0;
	int32 DuplicateEvents = 0;
	int32 InvalidFacts = 0;
	/* A completion reported again with the same idempotency key: absorbed at ingress. */
	int32 DuplicateCompletions = 0;
	/* Recipients the store doesn't know: skipped, with a trace line each. */
	int32 UnknownRecipients = 0;
	/* Optional relations refused because the protected budget was exhausted, across all minds. */
	int32 CapacityFailures = 0;
	int64 CommittedEvents = 0;
	int64 Decisions = 0;
	int64 Valuations = 0;
	int64 DuplicateValuations = 0;
	/* Conversation beats advanced (TDD Pattern 27), and beats refused because a producer still held a tick. */
	int64 Beats = 0;
	int64 HeldBeats = 0;
};

/**
 * One speaker's dialog history toward one addressee (TDD Patterns 9, 10; §7 Relation): the opportunity counter that
 * keys every hand, Think, reply and bark, and the novelty memory of the last offers. Authoritative: saved with the
 * relation in P1, hashed now. A read or a hover never advances it.
 */
struct NARRATIVEENGINE_API FNarrativeDialogPairState
{
	/* More than any hand sheet's window W. */
	static constexpr int32 MaxRemembered = 8;

	uint64 Opportunities = 0;

	/* The families shown in each recent offer, oldest first. Verbs and Silence never enter. */
	TArray<TArray<FName>> RecentOffers;
};

/**
 * One campaign's narrative truth (TDD Pattern 21): identities, minds, regard, the clock and the
 * ordered ingress. A plain object so tests run it headless; UNarrativeStoreSubsystem owns the
 * game's, with a game-instance lifetime. Actors, widgets and inspectors only read it.
 *
 * Commit procedure, per sealed tick (TDD §6): facts commit by causal layer, parents first, ordered
 * by (producer, sequence). Within a layer each observer-subject pair gathers its observations with
 * every care read frozen, sums its regard deltas, then caps once. After the last layer, the
 * decisions scheduled at that tick run and read them.
 */
class NARRATIVEENGINE_API FNarrativeCampaign
{
public:
	static constexpr uint64 DefaultSeed = 20260929;
	static constexpr int32 MaxRecentEvents = 64;

	explicit FNarrativeCampaign(uint64 Seed = DefaultSeed);

	/** New game: everything forgotten, the clock at zero. */
	void Reset(uint64 Seed);

	const FNarrativeCampaignHeader& GetHeader() const { return Header; }
	void SetPresetId(const FString& InPresetId) { Header.PresetId = InPresetId; }
	const FNarrativeCampaignStats& GetStats() const { return Stats; }

#pragma region Entities
	/** A fresh id for a runtime spawn, from the campaign namespace and its counter. */
	FNarrativeEntityId AllocateRuntimeId();

	/** A fresh activation id: never repeats within this campaign, whichever world asks. */
	uint64 AllocateActivationId() { return ++Header.LastActivationId; }

	/**
	 * Registers an entity from its definition, and its mind if the sheet has one. An id already
	 * known keeps its state: that is a view rebinding after travel or streaming, not a new entity.
	 */
	FNarrativeEntityRecord* RegisterEntity(const FNarrativeEntityId& Id, const UNarrativeEntityDef& Def);

	/** The same from plain data, for tests and fixtures. A null Sheet, or one without a mind, makes none. */
	FNarrativeEntityRecord* RegisterEntity(const FNarrativeEntityRecord& Record, const FNarrativeMindSheet* Sheet);

	/** Editor tuning: every entity made from Def takes its authored sheet, mask, mass and position again. */
	void RefreshFromTemplate(const UNarrativeEntityDef& Def);

	const FNarrativeEntityRecord* FindEntity(const FNarrativeEntityId& Id) const { return Entities.Find(Id); }

	/** An entity minds may relate to. False for unknown ids. */
	bool IsIndividual(const FNarrativeEntityId& Id) const
	{
		const FNarrativeEntityRecord* Record = Entities.Find(Id);
		return Record && Record->bIndividual;
	}
	const TMap<FNarrativeEntityId, FNarrativeEntityRecord>& GetEntities() const { return Entities; }

	void SetFactions(const FNarrativeEntityId& Id, const TArray<FName>& Factions);

	/** b between two factions, as the game defines them. Unset pairs add nothing. */
	void SetFactionBias(FName ObserverFaction, FName SubjectFaction, double Bias);
	void ClearFactionBiases() { FactionBiases.Reset(); }
#pragma endregion

#pragma region Ingress
	/**
	 * Takes a producer's fact with its occurrence tick and snapshot, assigns its id and sequence, and
	 * enqueues it. An OccurrenceTick below zero means "now" (the horizon). A fact for a tick already
	 * committed is late: a producer defect, logged and moved to the next open tick.
	 *
	 * A completion reported again (same completion key) is the same event: nothing is enqueued and the
	 * first report's EventId comes back. So is a fact that arrives carrying an EventId already assigned.
	 */
	uint64 SubmitFact(FNarrativeEvent Fact, ENarrativeProducer Producer);

	/** Whether this completion key was already submitted, and its EventId if so. */
	uint64 FindCompletion(uint64 CompletionKey) const;

	/** Game time passed: the world's dilated delta. */
	void Advance(double GameDeltaSeconds) { Clock.Advance(GameDeltaSeconds); }

	/**
	 * Commits every sealed tick, up to a budget of decisions. A budget may delay work, never skip it:
	 * what's left stays as lag. Returns the decisions made. Listeners hear about stance changes, tells
	 * and committed events after the pump, never mid-commit.
	 */
	int32 Pump(int32 DecisionBudget = TNumericLimits<int32>::Max());

	FNarrativeClock& GetClock() { return Clock; }
	const FNarrativeClock& GetClock() const { return Clock; }

	/** Newest last. */
	const TArray<FNarrativeEvent>& GetRecentEvents() const { return RecentEvents; }
	int32 GetPendingFactCount() const;
#pragma endregion

#pragma region Reads
	/** The committed decision of Observer about Subject. Never draws, decides or advances time. */
	FNarrativeRelationSnapshot ReadRelation(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const;

	bool HasMind(const FNarrativeEntityId& Observer) const { return Minds.Contains(Observer); }
	const FNarrativeMind* FindMind(const FNarrativeEntityId& Observer) const { return Minds.Find(Observer); }
	const TMap<FNarrativeEntityId, FNarrativeMind>& GetMinds() const { return Minds; }
	const FNarrativeRelation* FindRelation(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const;

	/** R decayed to the frontier. */
	double GetRegardAtFrontier(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const;

	/**
	 * What Observer believes of Subject on Axes, in Axes order, for choosing what to say (TDD Pattern
	 * 10): the relation's μ and diag Σ where it holds the axis, else the first impression a meeting
	 * would form (§4.2). Never the subject's true position, and nothing is created or drawn. Talking
	 * to oneself reads one's own position, exactly. False when either entity is unknown.
	 */
	bool ReadBelief(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, const FNarrativeMask& Axes,
		TArray<double>& OutMean, TArray<double>& OutVariance, ENarrativeBeliefSource* OutSource = nullptr) const;

	/**
	 * What a line spoken to Observer by Speaker would do, by the commit's own rule (§8.5; care is 1 because Observer is
	 * its target), run through the last decision's affinity parts. Never draws, decides or advances time. Invalid
	 * without a relation.
	 */
	FNarrativeAffinityForecast ForecastLine(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Speaker, double Strength, int32 Stance,
		const FVectorND& Location, const FNarrativeMask& Mask) const;

	/**
	 * ForecastLine's regard -> affinity -> stance chain for a regard change the caller valued itself: a player-facing
	 * preview values the line by what the speaker believes of Observer, never by Observer's true position as ForecastLine does.
	 */
	FNarrativeAffinityForecast ForecastRegardDelta(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Speaker, double RegardDelta) const;

	/**
	 * How many lines of strength J, each landing fully on Observer's values (ΔR = ±m J), move its affinity toward Speaker
	 * to Goal. Direction +1 ends at or above Goal; -1 ends strictly below it, as the Schmitt trigger's θH does. 0 when
	 * already there; INDEX_NONE only when regard saturates at the Loyalty cap short of it. An estimate for that J: a line
	 * off those values moves regard less (0.6 J), and a weaker or stronger card takes more or fewer lines.
	 */
	int32 CountLinesToAffinity(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Speaker, double Strength, int32 Direction, double Goal) const;

	/**
	 * Over every authoritative record in canonical order (TDD §7): header and counters, the frontier,
	 * producer sequences and holds, pending facts, committed and completed ids, entities, sheets,
	 * archetype beliefs, pins, relations with their tells, and every valued unit's identity. Inspection
	 * never changes it.
	 *
	 * Clock debt (horizon, remainder) is what differs between frame schedules that reach the same
	 * frontier, so it is left out unless asked for.
	 */
	uint32 ComputeStateHash(bool bIncludeClockDebt = false) const;
	/* Versioned domain checkpoint. Import validates a temporary campaign before replacing truth;
	 * view bindings and subscribers are owned outside this payload and remain intact. */
	bool ExportCheckpoint(TArray<uint8>& Bytes) const;
	bool ImportCheckpoint(const TArray<uint8>& Bytes, FString& Error);
	bool IsCommitting() const
	{
		return bPumping;
	}
#pragma endregion

#pragma region Dialog
	/** The pair's next opportunity index, advancing the counter: once per hand, Think, reply or bark (TDD Pattern 9). */
	uint64 TakeOpportunity(const FNarrativeEntityId& Speaker, const FNarrativeEntityId& Addressee);

	/** The index the next TakeOpportunity returns. Never advances anything. */
	uint64 PeekOpportunity(const FNarrativeEntityId& Speaker, const FNarrativeEntityId& Addressee) const;

	/** n_g: how many of the pair's last Window offers held Family (TDD Pattern 10). */
	int32 CountRecentOffers(const FNarrativeEntityId& Speaker, const FNarrativeEntityId& Addressee, FName Family, int32 Window) const;

	/** A hand was shown: its non-verb families enter the pair's novelty memory. A Think is a new offer and enters again. */
	void RecordOffer(const FNarrativeEntityId& Speaker, const FNarrativeEntityId& Addressee, const TArray<FName>& Families);

	const TMap<TTuple<FNarrativeEntityId, FNarrativeEntityId>, FNarrativeDialogPairState>& GetDialogPairs() const { return DialogPairs; }

	/**
	 * A conversation beat (TDD Pattern 27, D1): while a conversation holds the world paused, a committed line or reply
	 * advances narrative time by exactly Seconds (one decision interval) and seals it, so the addressee judges each line
	 * before it answers. False, advancing nothing, while any producer holds a tick; OutHeldBy names it.
	 */
	bool AdvanceBeat(double Seconds, FString* OutHeldBy = nullptr);
#pragma endregion

#pragma region Game hooks
	/** The party, as the roster now stands: each member with a mind keeps every other member pinned. */
	void SetPartyPins(const TArray<FNarrativeEntityId>& Members);

	/** Source threatens Observer until UntilTick: pinned so long. Every new threat fact extends it. */
	void PinThreat(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Source, int64 UntilTick);
	void UnpinThreat(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Source);
#pragma endregion

#pragma region Fixtures
	/**
	 * A fixture's acquaintance (gyms and tests only): Observer believes Subject is at N(Mean, Spread² I)
	 * from now on. Makes the relation if needed. The noise state is kept; nothing is redrawn.
	 */
	bool FixtureSetBelief(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, const FVectorND& Mean, double Spread);

	/** A fixture's meeting at the next open tick, without waiting for perception. */
	bool FixtureMeet(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject);

	/**
	 * A fixture's sheet (gyms and tests only): Id's mind runs Sheet from now on, made if it had none,
	 * and its mask becomes Mask when given. The asset is untouched; tuning still belongs there.
	 */
	bool FixtureSetSheet(const FNarrativeEntityId& Id, const FNarrativeMindSheet& Sheet, const FNarrativeMask* Mask = nullptr);

	/** A fixture's history (tests only): Observer's stored regard for Subject is R as of the next open tick. */
	bool FixtureSetRegard(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, double R);
#pragma endregion

	FOnNarrativeStanceDecided OnStanceDecided;
	FOnNarrativeTell OnTellStarted;
	/* The turn reversed before its warning was seen: any pending strike is off. */
	FOnNarrativeTell OnTellCancelled;
	FOnNarrativeTell OnWarningSeen;
	FOnNarrativeEventCommitted OnEventCommitted;

private:
	void SerializeCheckpoint(FArchive& Archive);
	FNarrativeMind& AddMind(const FNarrativeEntityId& Observer, const FNarrativeMindSheet& Sheet);
	void ScheduleFirstDecision(FNarrativeMind& Mind) const;

	void CommitTick(int64 Tick, int32& InOutDecisions);
	void CommitLayer(int64 Tick, TArrayView<FNarrativeEvent> Facts);
	void ApplyWorldConsequences(int64 Tick, const FNarrativeEvent& Fact);
	void RunDecisions(int64 Tick, int32& InOutDecisions);
	void Decide(FNarrativeMind& Mind, FNarrativeRelation& Relation, int64 Tick);

	/**
	 * The relation, made now from a first impression if this is the meeting. Null when refused.
	 *
	 * Capacity (TDD Pattern 12): a full registry forgets the unprotected relation that matters least, a
	 * neutral one before a grudge and the least recently touched among equals, never one the commit in
	 * progress has reserved. An optional relation (a meeting) is refused once protected relations fill
	 * their budget; a required one (a deed's witness, a pin) never is while anything can be forgotten.
	 */
	FNarrativeRelation* EnsureRelation(FNarrativeMind& Mind, const FNarrativeEntityId& Subject, int64 Tick,
									   bool bRequired = true);
	const FVectorND& GetArchetypeBelief(FNarrativeMind& Mind, const FNarrativeEntityRecord& Subject);
	void AdvanceNoise(const FNarrativeMind& Mind, FNarrativeRelation& Relation, int64 Tick) const;
	double ComputeAffinity(const FNarrativeMind& Mind, const FNarrativeRelation& Relation, double RegardNow,
						   FNarrativeAffinityParts& OutParts) const;
	double GetFactionBias(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const;
	double ApprovalOf(const FNarrativeMind& Mind, const FNarrativeEvent& Fact) const;

	void FlushNotifications();

	FNarrativeCampaignHeader Header;
	FNarrativeCampaignStats Stats;
	FNarrativeClock Clock;

	TMap<FNarrativeEntityId, FNarrativeEntityRecord> Entities;
	TMap<FNarrativeEntityId, FNarrativeMind> Minds;
	TMap<TPair<FName, FName>, double> FactionBiases;

	/* Facts waiting for their tick to seal, by occurrence tick. */
	TMap<int64, TArray<FNarrativeEvent>> Ingress;
	TSet<uint64> CommittedEventIds;
	/* Every completion key submitted, with the EventId it got: the once-only ledger for events. */
	TMap<uint64, uint64> Completions;

	/* Relations a commit in progress has touched, as (observer, subject): eviction never takes them mid-batch. */
	TSet<TPair<FNarrativeEntityId, FNarrativeEntityId>> Reserved;
	TArray<FNarrativeEvent> RecentEvents;

	struct FStanceNotice
	{
		FNarrativeEntityId Observer;
		FNarrativeEntityId Subject;
		ENarrativeStance Old;
		ENarrativeStance New;
		int64 Tick;
	};
	struct FTellNotice
	{
		enum class EKind : uint8 { Started, Cancelled, Seen } Kind;
		FNarrativeEntityId Observer;
		FNarrativeEntityId Subject;
	};
	TArray<FStanceNotice> StanceNotices;
	TArray<FTellNotice> TellNotices;
	TArray<FNarrativeEvent> CommittedNotices;
	bool bPumping = false;

	TMap<TTuple<FNarrativeEntityId, FNarrativeEntityId>, FNarrativeDialogPairState> DialogPairs;
};
