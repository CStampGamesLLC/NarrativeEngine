#pragma once

#include "CoreMinimal.h"
#include "NarrativeIds.h"
#include "NarrativeMask.h"
#include "NarrativeVector.h"
#include "NarrativeEvent.generated.h"

/** Who is responsible for an act (spec §3.3; TDD Pattern 15). Orders are providence and never acts. */
UENUM()
enum class ENarrativeOrigin : uint8
{
	Autonomous,
	/* What a character does under the player's order: its own deed, judged from the selected event alone. */
	PlayerDirected,
	/* No positional evidence by default; material harm still counts. */
	Forced,
};

/** What a fact records. Kinds past Death land with their increments. */
UENUM()
enum class ENarrativeEventKind : uint8
{
	/* A delivered line. */
	Line,
	/* An item that changed hands. */
	Gift,
	/* One attack's finalized harm to one target: its valuation unit. */
	Attack,
	/* An attack that completed without harm, perceived: the weaker attempt record. */
	AttackAttempt,
	Death,
	/* Two entities came within reach of each other's minds: relations form, nothing is valued. */
	Encounter,
	/* The warning before a social strike finished playing where it could be seen (TDD Pattern 17). */
	WarningSeen,
};

UENUM()
enum class ENarrativeOutcome : uint8
{
	Completed,
	/* Tried, and failed in a way others could see. */
	Attempted,
};

/** How a recipient perceived the act (TDD Pattern 11). Evidence weight scales beliefs (P4), never regard. */
UENUM()
enum class ENarrativeChannel : uint8
{
	Participant,
	Addressed,
	Overheard,
	Seen,
};

USTRUCT()
struct NARRATIVEENGINE_API FNarrativeRecipient
{
	GENERATED_BODY()

	UPROPERTY()
	FNarrativeEntityId Id;

	UPROPERTY()
	ENarrativeChannel Channel = ENarrativeChannel::Seen;

	/* Addressed 1, overheard 0.5 (candidate). */
	UPROPERTY()
	float EvidenceWeight = 1.f;
};

/**
 * The record of one completed act (spec §3.3; TDD Pattern 11): immutable once committed, and valued
 * once per observer through its valuation unit. Producers stamp the occurrence tick and the snapshot
 * at completion; the store assigns identity and order.
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativeEvent
{
	GENERATED_BODY()

	static constexpr int32 SchemaVersion = 1;

	/* The envelope layout this record was written in; the commit refuses any other (TDD §6). */
	UPROPERTY()
	int32 EnvelopeVersion = SchemaVersion;

#pragma region Identity
	/* (producer << 56) | producer sequence: the same inputs always get the same ids. */
	UPROPERTY()
	uint64 EventId = 0;

	/* The original deed a retelling descends from; its own id for a first-hand act. */
	UPROPERTY()
	uint64 RootId = 0;

	/* The act this one follows from, e.g. the attack behind a death. */
	UPROPERTY()
	uint64 ParentId = 0;

	/* The activation it completes: one delivered line, one swing. */
	UPROPERTY()
	uint64 ActivationId = 0;

	/* What is valued once per observer. Several events may share one (multi-hit, damage over time). */
	UPROPERTY()
	uint64 ValuationUnitId = 0;

	/*
	 * The producer's idempotency key for this completion: a report repeating one is the same event, and
	 * the store absorbs it at ingress. 0 derives it from (producer, kind, activation, instigator, target) when the
	 * fact names an activation; a fact with neither is always new.
	 */
	UPROPERTY()
	uint64 CompletionKey = 0;

	/* The horizon when the act completed. Deferred processing keeps it. */
	UPROPERTY()
	int64 OccurrenceTick = 0;

	UPROPERTY()
	uint8 Producer = 0;

	UPROPERTY()
	uint64 ProducerSequence = 0;

	/* Parents commit before children within one tick. */
	UPROPERTY()
	int32 CausalLayer = 0;
#pragma endregion

#pragma region Attribution
	/*
	 * Who did it and answers for it: the speaker, giver, attacker, killer. Never the perceiver (see
	 * Recipients). Non-deeds bend it: an unblamed Death names the victim; WarningSeen names the one warning.
	 */
	UPROPERTY()
	FNarrativeEntityId Instigator;

	/* The body that did it on the instigator's behalf: a summon, a projectile. Unset when they're one. */
	UPROPERTY()
	FNarrativeEntityId Causer;

	UPROPERTY()
	FNarrativeEntityId Target;

	/* What the act is about, when that isn't the target: the subject of a warning. */
	UPROPERTY()
	FNarrativeEntityId Subject;

	UPROPERTY()
	ENarrativeOrigin Origin = ENarrativeOrigin::Autonomous;

	UPROPERTY()
	ENarrativeEventKind Kind = ENarrativeEventKind::Line;

	UPROPERTY()
	ENarrativeOutcome Outcome = ENarrativeOutcome::Completed;
#pragma endregion

#pragma region Meaning
	/* The card or outcome policy, as a full asset identity. */
	UPROPERTY()
	FName DefinitionId;

	UPROPERTY()
	int32 DefinitionVersion = 0;

	/* ℓ, M and σ of what the act means. */
	UPROPERTY()
	FVectorND Location;

	UPROPERTY()
	FNarrativeMask Mask;

	UPROPERTY()
	float Sigma = 0.f;

	/* J: how hard it lands. 0 moves no one's regard. */
	UPROPERTY()
	float Strength = 0.f;

	/* h: +1 done to friends, -1 done to foes. */
	UPROPERTY()
	int32 Stance = 0;

	/* m_T, frozen at occurrence; 1 when there is no target. */
	UPROPERTY()
	float TargetMass = 1.f;
#pragma endregion

#pragma region Perception
	UPROPERTY()
	FName Place;

	/* Who perceived it. Empty means the participants alone. */
	UPROPERTY()
	TArray<FNarrativeRecipient> Recipients;
#pragma endregion

#pragma region Evidence
	/* For the Mind panel and trace. The private hand never enters an event. */
	UPROPERTY()
	FString Label;
#pragma endregion

	/* A deed moves regard; a meeting, a warning or a zero-strength line doesn't. */
	bool IsDeed() const { return Strength > 0.f && Kind != ENarrativeEventKind::Encounter && Kind != ENarrativeEventKind::WarningSeen; }

	static uint64 MakeEventId(uint8 InProducer, uint64 Sequence) { return (static_cast<uint64>(InProducer) << 56) | (Sequence & 0x00FFFFFFFFFFFFFFull); }

	/* (producer, kind, activation, instigator, target): what makes two reports the same completion. */
	uint64 DeriveCompletionKey() const;

	/* One activation's valuation unit: kept apart from event ids by bit 55, so the two never collide. */
	static uint64 MakeActivationUnitId(uint8 InProducer, uint64 ActivationId) { return (static_cast<uint64>(InProducer) << 56) | (1ull << 55) | (ActivationId & 0x007FFFFFFFFFFFFFull); }
};

NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeEventKind Kind);
NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeOrigin Origin);
