// Copyright Timeless Way Interactive, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NarrativeIds.h"
#include "NarrativeParty.generated.h"

/** Why a member left (TDD Pattern 20). Each has its own event policy; only Betrayal emits a deed today. */
UENUM(BlueprintType)
enum class ENarrativeDepartureReason : uint8
{
	/* The player sent it away. */
	Dismissed,
	/* It chose to go, without turning on anyone. */
	Voluntary,
	/* Its grace ran out while it stayed Hostile toward a member. */
	Betrayal,
	Death,
};

NARRATIVEENGINE_API const TCHAR* LexToString(ENarrativeDepartureReason Reason);

/** Where a character stands with the player's party (TDD Pattern 20): Member → PendingBetrayal → out. */
UENUM(BlueprintType)
enum class ENarrativeMemberState : uint8
{
	NotMember,
	Member,
	/* A member whose tell toward a fellow member is active: it takes no orders, and its grace may be running. */
	PendingBetrayal,
};

/**
 * The player's party, as narrative truth (TDD Patterns 20, 21): who is in it by stable entity id, in slot order, and its
 * revision. Actors are views: a member streamed out, unloaded or not yet spawned is still a member. Only a join, a
 * departure or a reorder changes it, and a load installs it whole.
 */
USTRUCT()
struct NARRATIVEENGINE_API FNarrativePartyRecord
{
	GENERATED_BODY()

	/* Slot order. */
	UPROPERTY()
	TArray<FNarrativeEntityId> Members;

	/* Bumped by every change, the install included: a recruitment check revalidates against it at commit. */
	UPROPERTY()
	uint32 Revision = 0;

	/* False until the game's starting gather or a load has installed a roster: an empty unestablished party means "not
	 * known yet", never "nobody". */
	UPROPERTY()
	bool bEstablished = false;

	/* Committed departures over the campaign, by every reason: the exactly-once tests read it. */
	UPROPERTY()
	int32 Departures = 0;

	/* The player's own character (Kaiza): always a member while the player plays, so never betrays or dies out of it. */
	UPROPERTY()
	FNarrativeEntityId PlayerCharacter;

	/* Seeded from a save older than this record while some of its members were still unresolved (their level unloaded):
	 * until they resolve, the save's member paths answer for the roster, so a member streaming in later is added, never
	 * dropped. Saved, so a migration survives another save and load. */
	UPROPERTY()
	bool bProvisional = false;
};

/**
 * Which beat of a paused conversation is advancing narrative time (TDD Pattern 27), so the commit can tell the party's
 * own lines from everything else: a betrayal's grace counts only the former, so a reply or a target's opener never spends
 * the player's window. Speaker and Addressee are the conversation's pair.
 */
struct NARRATIVEENGINE_API FNarrativeBeatContext
{
	FNarrativeEntityId Speaker;
	FNarrativeEntityId Addressee;
	/* A line the player picked for a party member: the only beat a grace counts. */
	bool bPartyLine = false;

	bool Involves(const FNarrativeEntityId& Id) const { return Id.IsValid() && (Id == Speaker || Id == Addressee); }
};
