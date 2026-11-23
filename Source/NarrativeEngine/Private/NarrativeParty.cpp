// Copyright Timeless Way Interactive, LLC. All Rights Reserved.

#include "NarrativeParty.h"

#include "NarrativeCampaign.h"

const TCHAR* LexToString(ENarrativeDepartureReason Reason)
{
	switch (Reason)
	{
	case ENarrativeDepartureReason::Dismissed: return TEXT("Dismissed");
	case ENarrativeDepartureReason::Voluntary: return TEXT("Voluntary");
	case ENarrativeDepartureReason::Betrayal: return TEXT("Betrayal");
	case ENarrativeDepartureReason::Death: return TEXT("Death");
	default: return TEXT("?");
	}
}

const TCHAR* LexToString(ENarrativeDuelEnd End)
{
	switch (End)
	{
	case ENarrativeDuelEnd::None: return TEXT("None");
	case ENarrativeDuelEnd::Downed: return TEXT("Downed");
	case ENarrativeDuelEnd::Called: return TEXT("Called");
	case ENarrativeDuelEnd::Departed: return TEXT("Departed");
	case ENarrativeDuelEnd::Died: return TEXT("Died");
	default: return TEXT("?");
	}
}

#pragma region Party
ENarrativeMemberState FNarrativeCampaign::GetMemberState(const FNarrativeEntityId& Id) const
{
	if (!IsPartyMember(Id))
	{
		return ENarrativeMemberState::NotMember;
	}
	return FindPendingBetrayal(Id) ? ENarrativeMemberState::PendingBetrayal : ENarrativeMemberState::Member;
}

const FNarrativeTell* FNarrativeCampaign::FindPendingBetrayal(const FNarrativeEntityId& Id, FNarrativeEntityId* OutSubject) const
{
	const FNarrativeMind* Mind = IsPartyMember(Id) ? Minds.Find(Id) : nullptr;
	if (!Mind)
	{
		return nullptr;
	}

	// The member it is actually Hostile toward. Several at once resolve in slot order, so the answer never depends on hashing.
	for (const FNarrativeEntityId& Other : Party.Members)
	{
		const FNarrativeRelation* Relation = Other != Id ? Mind->Relations.Find(Other) : nullptr;
		if (Relation && Relation->Tell.bActive)
		{
			if (OutSubject)
			{
				*OutSubject = Other;
			}
			return &Relation->Tell;
		}
	}
	return nullptr;
}

void FNarrativeCampaign::InstallParty(const TArray<FNarrativeEntityId>& Members)
{
	TArray<FNarrativeEntityId> Unique;
	for (const FNarrativeEntityId& Id : Members)
	{
		if (Id.IsValid() && !Unique.Contains(Id))
		{
			Unique.Add(Id);
		}
	}

	const bool bChanged = !Party.bEstablished || Party.Members != Unique;
	Party.Members = MoveTemp(Unique);
	Party.bEstablished = true;
	SetPartyPins(Party.Members);
	if (bChanged)
	{
		// An install is no join and no departure: listeners resync, and nothing is valued.
		++Party.Revision;
		PartyNotices.Add({FNarrativeEntityId(), false, ENarrativeDepartureReason::Dismissed});
	}
	if (!bPumping)
	{
		FlushNotifications();
	}
}

bool FNarrativeCampaign::JoinParty(const FNarrativeEntityId& Id, int32 Slot)
{
	if (!Id.IsValid() || IsPartyMember(Id))
	{
		return false;
	}

	Party.Members.Insert(Id, Slot < 0 ? Party.Members.Num() : FMath::Min(Slot, Party.Members.Num()));
	Party.bEstablished = true;
	NoteParty(Id, true, ENarrativeDepartureReason::Dismissed);
	UE_LOG(LogNarrativeTrace, Log, TEXT("[%lld] %s joined the party."), Clock.GetFrontier(), *Id.ToShortString());
	return true;
}

bool FNarrativeCampaign::LeaveParty(const FNarrativeEntityId& Id, ENarrativeDepartureReason Reason)
{
	if (!IsPartyMember(Id))
	{
		return false;
	}

	// A duel is a party's affair: whoever leaves, by any reason, ends theirs first.
	EndDuel(Id, Reason == ENarrativeDepartureReason::Death ? ENarrativeDuelEnd::Died : ENarrativeDuelEnd::Departed);
	Party.Members.Remove(Id);

	++Party.Departures;
	NoteParty(Id, false, Reason);
	UE_LOG(LogNarrativeTrace, Log, TEXT("[%lld] %s left the party (%s)."), Clock.GetFrontier(), *Id.ToShortString(), LexToString(Reason));
	return true;
}

bool FNarrativeCampaign::MovePartyMember(const FNarrativeEntityId& Id, int32 Slot)
{
	const int32 Old = Party.Members.IndexOfByKey(Id);
	if (Old == INDEX_NONE)
	{
		return false;
	}

	const int32 New = FMath::Clamp(Slot, 0, Party.Members.Num() - 1);
	if (New == Old)
	{
		return false;
	}
	Party.Members.RemoveAt(Old);
	Party.Members.Insert(Id, New);
	NoteParty(FNarrativeEntityId(), false, ENarrativeDepartureReason::Dismissed);
	return true;
}

void FNarrativeCampaign::NoteParty(const FNarrativeEntityId& Member, bool bJoined, ENarrativeDepartureReason Reason)
{
	++Party.Revision;
	SetPartyPins(Party.Members);
	PartyNotices.Add({Member, bJoined, Reason});
	// Outside a commit, listeners hear at once; inside one, after the pump, never mid-commit.
	if (!bPumping)
	{
		FlushNotifications();
	}
}

bool FNarrativeCampaign::AdvanceGrace(const FNarrativeMind& Mind, FNarrativeRelation& Relation, ENarrativeStance Old, ENarrativeStance New, int64 Tick) const
{
	// Only a decision that stays Hostile, about a fellow member, after the warning was seen; and in a paused conversation
	// only one on a beat of the party's own line, so a reply never spends the player's window.
	FNarrativeTell& Tell = Relation.Tell;
	// The player's own character never leaves its own party, however it feels about a member.
	if (!bDeparturesEnabled || Mind.Observer == Party.PlayerCharacter)
	{
		return false;
	}
	if (!Tell.bActive || Old != ENarrativeStance::Hostile || New != ENarrativeStance::Hostile || !AreCoMembers(Mind.Observer, Relation.Subject))
	{
		return false;
	}
	if (!Tell.bGraceStarted || Tick <= Tell.GraceStartTick || (CurrentBeat && !CurrentBeat->bPartyLine))
	{
		return false;
	}

	++Tell.GraceDecisions;
	UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s's grace toward %s: %d of %d."), Tick, *Mind.Observer.ToShortString(), *Relation.Subject.ToShortString(),
		Tell.GraceDecisions, BetrayalGrace);
	return Tell.GraceDecisions >= BetrayalGrace;
}

void FNarrativeCampaign::CommitBetrayal(const FNarrativeEntityId& Member, const FNarrativeEntityId& Subject, const FNarrativeTell& Tell, int64 Tick)
{
	const uint64 TellIdentity = Tell.GetIdentity();
	if (!LeaveParty(Member, ENarrativeDepartureReason::Betrayal))
	{
		return;
	}

	// The betrayal's one valuation unit, keyed by the tell it completes: a repeat of the same completion is absorbed.
	// It lands on the next tick, so this tick's decisions never read their own consequence. A harm to the betrayed
	// (h = -1, its mass frozen now), valued by it and by the members it left: the party learns what it did.
	FNarrativeEvent Fact;
	Fact.OccurrenceTick = Tick + 1;
	Fact.Kind = ENarrativeEventKind::Departure;
	Fact.Instigator = Member;
	Fact.Target = Subject;
	Fact.Subject = Subject;
	Fact.ActivationId = TellIdentity;
	Fact.Strength = BetrayalStrength;
	Fact.Stance = -1;
	const FNarrativeEntityRecord* Betrayed = Entities.Find(Subject);
	Fact.TargetMass = Betrayed ? Betrayed->Mass : 1.f;
	// The betrayed is a participant wherever it is; everyone else values it only if the game's perception rules say they
	// saw it, member or not: an ally in an unloaded region learns nothing, a bystander in sight does.
	if (WitnessResolver)
	{
		WitnessResolver(Fact);
	}
	Fact.Recipients.RemoveAll([&Subject](const FNarrativeRecipient& Recipient) { return Recipient.Id == Subject; });
	FNarrativeRecipient Victim;
	Victim.Id = Subject;
	Victim.Channel = ENarrativeChannel::Participant;
	Fact.Recipients.Insert(Victim, 0);
	Fact.Label = FString::Printf(TEXT("%s leaves the party, turning on %s"), *Member.ToShortString(), *Subject.ToShortString());
	SubmitFact(MoveTemp(Fact), ENarrativeProducer::Roster);
}
#pragma endregion

#pragma region Duels
const FNarrativeDuel* FNarrativeCampaign::FindDuel(const FNarrativeEntityId& Participant) const
{
	return Party.Duels.FindByPredicate([&Participant](const FNarrativeDuel& Duel) { return Duel.IsActive() && Duel.Involves(Participant); });
}

bool FNarrativeCampaign::AreDueling(const FNarrativeEntityId& A, const FNarrativeEntityId& B) const
{
	const FNarrativeDuel* Duel = FindDuel(A);
	return Duel && Duel->Pairs(A, B);
}

bool FNarrativeCampaign::IsSanctioned(const FNarrativeEntityId& A, const FNarrativeEntityId& B, int64 Tick) const
{
	return Party.Duels.ContainsByPredicate([&](const FNarrativeDuel& Duel) { return Duel.Pairs(A, B) && Duel.Covers(Tick); });
}

bool FNarrativeCampaign::StartDuel(const FNarrativeEntityId& Challenger, const FNarrativeEntityId& Challenged)
{
	// Two living members, neither already fighting one.
	if (!AreCoMembers(Challenger, Challenged) || FindDuel(Challenger) || FindDuel(Challenged))
	{
		return false;
	}
	for (const FNarrativeEntityId& Id : {Challenger, Challenged})
	{
		const FNarrativeEntityRecord* Record = Entities.Find(Id);
		if (!Record || Record->LifeState != ENarrativeLifeState::Alive)
		{
			return false;
		}
	}

	FNarrativeDuel& Duel = Party.Duels.AddDefaulted_GetRef();
	Duel.Challenger = Challenger;
	Duel.Challenged = Challenged;
	// The tick a blow struck now is stamped with.
	Duel.StartTick = Clock.GetHorizon();
	DuelNotices.Add({Duel, true});
	UE_LOG(LogNarrativeTrace, Log, TEXT("[%lld] %s and %s begin a duel."), Duel.StartTick, *Challenger.ToShortString(), *Challenged.ToShortString());
	if (!bPumping)
	{
		FlushNotifications();
	}
	return true;
}

bool FNarrativeCampaign::EndDuel(const FNarrativeEntityId& Participant, ENarrativeDuelEnd Reason, const FNarrativeEntityId& Loser)
{
	FNarrativeDuel* Duel = Party.Duels.FindByPredicate([&Participant](const FNarrativeDuel& Candidate) { return Candidate.IsActive() && Candidate.Involves(Participant); });
	if (!Duel || Reason == ENarrativeDuelEnd::None)
	{
		return false;
	}

	// Through the tick in progress: the blow that ended it was struck now, so it is still sanctioned.
	Duel->EndTick = FMath::Max(Duel->StartTick, Clock.GetHorizon());
	Duel->End = Reason;
	Duel->Loser = Reason == ENarrativeDuelEnd::Downed && Duel->Involves(Loser) ? Loser : FNarrativeEntityId();
	DuelNotices.Add({*Duel, false});
	UE_LOG(LogNarrativeTrace, Log, TEXT("[%lld] The duel between %s and %s ends (%s%s%s)."), Duel->EndTick, *Duel->Challenger.ToShortString(),
		*Duel->Challenged.ToShortString(), LexToString(Reason), Duel->Loser.IsValid() ? TEXT(": ") : TEXT(""),
		Duel->Loser.IsValid() ? *Duel->Loser.ToShortString() : TEXT(""));
	if (!bPumping)
	{
		FlushNotifications();
	}
	return true;
}

void FNarrativeCampaign::PruneDuels(int64 CommittedTick)
{
	// Every blow an ended duel covered has committed by now: nothing left to judge by it.
	Party.Duels.RemoveAll([CommittedTick](const FNarrativeDuel& Duel) { return !Duel.IsActive() && Duel.EndTick <= CommittedTick; });
}
#pragma endregion
