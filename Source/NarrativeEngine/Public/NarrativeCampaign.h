#pragma once

#include "CoreMinimal.h"
#include "NarrativeClock.h"
#include "NarrativeEvent.h"
#include "NarrativeIds.h"
#include "NarrativeMind.h"
#include "NarrativeParty.h"

class UNarrativeEntityDef;

NARRATIVEENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogNarrativeTrace, Log, All);

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnNarrativeStanceDecided, const FNarrativeEntityId& /*Observer*/, const FNarrativeEntityId& /*Subject*/,
	ENarrativeStance /*Old*/, ENarrativeStance /*New*/, int64 /*Tick*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNarrativeTell, const FNarrativeEntityId& /*Observer*/, const FNarrativeEntityId& /*Subject*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNarrativeEventCommitted, const FNarrativeEvent&);
/* The party record changed: a join (bJoined), a departure (with its reason), or a reorder or whole install (Member unset). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnNarrativePartyChanged, const FNarrativeEntityId& /*Member*/, bool /*bJoined*/, ENarrativeDepartureReason /*Reason*/);
/* A duel began (bStarted) or ended (its End says why). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNarrativeDuelChanged, const FNarrativeDuel& /*Duel*/, bool /*bStarted*/);

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
	/* Bumped by every export: the game save stores it beside its actor half, and a load refuses a pair that disagrees. */
	uint64 CheckpointId = 0;
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
	 * Observer's approval of an act about Location on Mask, by its own values on the axes both are about (-1..1): the
	 * term the commit weighs regard by. Unset when Observer has no mind. Never draws or advances time.
	 */
	TOptional<double> ReadApproval(const FNarrativeEntityId& Observer, const FVectorND& Location, const FNarrativeMask& Mask) const;

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
	bool ExportCheckpoint(TArray<uint8>& Bytes, uint64* OutCheckpointId = nullptr) const;
	/* ExpectedCheckpointId 0 accepts any (a save written before checkpoint ids). */
	bool ImportCheckpoint(const TArray<uint8>& Bytes, FString& Error, uint64 ExpectedCheckpointId = 0);
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
	bool AdvanceBeat(double Seconds, FString* OutHeldBy = nullptr, const FNarrativeBeatContext* Beat = nullptr);
#pragma endregion

#pragma region Game hooks
	/** The party, as the roster now stands: each member with a mind keeps every other member pinned. */
	void SetPartyPins(const TArray<FNarrativeEntityId>& Members);
#pragma endregion

#pragma region Party
	/**
	 * The player's party by entity id (TDD Patterns 20, 21). Actors are views of it: EndPlay, streaming and loading only
	 * detach them, so none of those is a departure.
	 */
	const FNarrativePartyRecord& GetParty() const { return Party; }
	bool IsPartyMember(const FNarrativeEntityId& Id) const { return Id.IsValid() && Party.Members.Contains(Id); }
	bool AreCoMembers(const FNarrativeEntityId& A, const FNarrativeEntityId& B) const { return A != B && IsPartyMember(A) && IsPartyMember(B); }

	/** Member, PendingBetrayal (an active tell toward a fellow member), or NotMember. */
	ENarrativeMemberState GetMemberState(const FNarrativeEntityId& Id) const;

	/** The pending betrayal's tell, toward the member it is actually Hostile toward. Null unless PendingBetrayal. */
	const FNarrativeTell* FindPendingBetrayal(const FNarrativeEntityId& Id, FNarrativeEntityId* OutSubject = nullptr) const;

	/** A roster installed whole, as a load or the starting gather finds it: no join, no departure, no event; pins follow. */
	void InstallParty(const TArray<FNarrativeEntityId>& Members);

	/** A join the game committed (recruitment, a scripted join). Slot below zero appends. False if already a member. */
	bool JoinParty(const FNarrativeEntityId& Id, int32 Slot = INDEX_NONE);

	/** A departure the game committed (dismissal, death). Betrayal is the campaign's own decision; see the grace. */
	bool LeaveParty(const FNarrativeEntityId& Id, ENarrativeDepartureReason Reason);

	/** A reorder. False when Id is not a member or nothing moved. */
	bool MovePartyMember(const FNarrativeEntityId& Id, int32 Slot);

	/** K: Hostile decisions after the seen warning before a betrayal removes the member (TDD Pattern 20; K=1 Candidate). */
	void SetBetrayalGrace(int32 K) { BetrayalGrace = FMath::Max(1, K); }
	int32 GetBetrayalGrace() const { return BetrayalGrace; }

	/** Whether a grace runs and a betrayal can commit at all: the game turns it on with its Party feature (P1), so earlier
	 * presets keep their baseline. Off by default. */
	void SetDeparturesEnabled(bool bEnabled) { bDeparturesEnabled = bEnabled; }
	bool AreDeparturesEnabled() const { return bDeparturesEnabled; }

	/** J of a betrayal's Departure deed (h = -1, the betrayed's mass scales it): what the betrayed and the rest of the party
	 * hold against the betrayer. */
	void SetBetrayalStrength(float J) { BetrayalStrength = FMath::Max(0.f, J); }

	/** The player's own character: never departs by betrayal or death. Invalid clears it. */
	void SetPartyPlayerCharacter(const FNarrativeEntityId& Id) { Party.PlayerCharacter = Id; }

	/** Who perceived a deed the campaign itself commits (a betrayal): the game's perception rules append witnesses to the
	 * fact's recipients. Unset, only the participants value it. Configuration, kept across Reset and loads. */
	void SetWitnessResolver(TFunction<void(FNarrativeEvent&)> Resolver) { WitnessResolver = MoveTemp(Resolver); }

	/** See FNarrativePartyRecord::bProvisional. */
	void SetPartyProvisional(bool bProvisional) { Party.bProvisional = bProvisional; }
#pragma endregion

#pragma region Duels
	/**
	 * Sanctioned combat between two members (see FNarrativeDuel): begins now, at the horizon. False when they aren't both
	 * living members or either is already fighting one. Asking whether they would is the game's (willingness, standing).
	 */
	bool StartDuel(const FNarrativeEntityId& Challenger, const FNarrativeEntityId& Challenged);

	/** Ends the duel Participant is fighting, through the tick in progress. Loser names who went down, for Downed. */
	bool EndDuel(const FNarrativeEntityId& Participant, ENarrativeDuelEnd Reason, const FNarrativeEntityId& Loser = FNarrativeEntityId());

	/** The duel Participant is fighting now, or null. */
	const FNarrativeDuel* FindDuel(const FNarrativeEntityId& Participant) const;

	/** Whether A and B are fighting each other now. */
	bool AreDueling(const FNarrativeEntityId& A, const FNarrativeEntityId& B) const;

	/** Whether a blow between A and B struck at Tick fell inside a duel between them, fought or since ended. */
	bool IsSanctioned(const FNarrativeEntityId& A, const FNarrativeEntityId& B, int64 Tick) const;

	/** How much a sanctioned blow moves regard, against an unsanctioned one: 0 (the default) moves none. Configuration. */
	void SetDuelHarmScale(float Scale) { DuelHarmScale = FMath::Max(0.f, Scale); }
	float GetDuelHarmScale() const { return DuelHarmScale; }
#pragma endregion

#pragma region Game hooks (threat)
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
	FOnNarrativePartyChanged OnPartyChanged;
	FOnNarrativeDuelChanged OnDuelChanged;

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

	/* The grace's bookkeeping for one decision about a fellow member; true when the betrayal must now commit. */
	bool AdvanceGrace(const FNarrativeMind& Mind, FNarrativeRelation& Relation, ENarrativeStance Old, ENarrativeStance New, int64 Tick) const;
	/* Removes Member and emits its betrayal's one Departure fact, keyed by the tell it completes. */
	void CommitBetrayal(const FNarrativeEntityId& Member, const FNarrativeEntityId& Subject, const FNarrativeTell& Tell, int64 Tick);
	/* Party records changed: pins follow, the revision bumps, listeners hear after the pump. */
	void NoteParty(const FNarrativeEntityId& Member, bool bJoined, ENarrativeDepartureReason Reason);
	/* Drops ended duels whose every covered tick has committed. */
	void PruneDuels(int64 CommittedTick);

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
	struct FPartyNotice
	{
		FNarrativeEntityId Member;
		bool bJoined = false;
		ENarrativeDepartureReason Reason = ENarrativeDepartureReason::Dismissed;
	};
	TArray<FPartyNotice> PartyNotices;
	struct FDuelNotice
	{
		FNarrativeDuel Duel;
		bool bStarted = false;
	};
	TArray<FDuelNotice> DuelNotices;
	bool bPumping = false;

	FNarrativePartyRecord Party;
	/* Configuration, not truth: the game sets it from db.Narrative.BetrayalGrace. */
	int32 BetrayalGrace = 1;
	bool bDeparturesEnabled = false;
	float BetrayalStrength = 0.6f;
	TFunction<void(FNarrativeEvent&)> WitnessResolver;
	/* Configuration, not truth: the game sets it from db.Narrative.Duel.HarmScale. */
	float DuelHarmScale = 0.f;
	/* The paused-conversation beat being committed, if any (AdvanceBeat). */
	const FNarrativeBeatContext* CurrentBeat = nullptr;

	TMap<TTuple<FNarrativeEntityId, FNarrativeEntityId>, FNarrativeDialogPairState> DialogPairs;
};
