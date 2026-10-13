#include "NarrativeCampaign.h"

#include "NarrativeMath.h"
#include "NarrativeRng.h"
#include "NarrativeStaticData.h"
#include "Serialization/BufferArchive.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogNarrativeTrace);

const TCHAR* LexToString(ENarrativeStance Stance)
{
	switch (Stance)
	{
	case ENarrativeStance::Hostile: return TEXT("Hostile");
	case ENarrativeStance::Friendly: return TEXT("Friendly");
	default: return TEXT("None");
	}
}

const TCHAR* LexToString(ENarrativeEventKind Kind)
{
	switch (Kind)
	{
	case ENarrativeEventKind::Line: return TEXT("Line");
	case ENarrativeEventKind::Gift: return TEXT("Gift");
	case ENarrativeEventKind::Attack: return TEXT("Attack");
	case ENarrativeEventKind::AttackAttempt: return TEXT("AttackAttempt");
	case ENarrativeEventKind::Death: return TEXT("Death");
	case ENarrativeEventKind::Encounter: return TEXT("Encounter");
	case ENarrativeEventKind::WarningSeen: return TEXT("WarningSeen");
	default: return TEXT("?");
	}
}

const TCHAR* LexToString(ENarrativeOrigin Origin)
{
	switch (Origin)
	{
	case ENarrativeOrigin::Autonomous: return TEXT("Autonomous");
	case ENarrativeOrigin::PlayerDirected: return TEXT("PlayerDirected");
	case ENarrativeOrigin::Forced: return TEXT("Forced");
	default: return TEXT("?");
	}
}

const TCHAR* LexToString(ENarrativeBeliefSource Source)
{
	switch (Source)
	{
	case ENarrativeBeliefSource::Relation: return TEXT("relation");
	case ENarrativeBeliefSource::RelationAndFirstImpression: return TEXT("relation + first impression");
	case ENarrativeBeliefSource::FirstImpression: return TEXT("first impression");
	case ENarrativeBeliefSource::Self: return TEXT("self");
	default: return TEXT("?");
	}
}

namespace
{
	/* Canonical order: (layer, producer, sequence). */
	bool PrecedesInTick(const FNarrativeEvent& A, const FNarrativeEvent& B)
	{
		if (A.CausalLayer != B.CausalLayer)
		{
			return A.CausalLayer < B.CausalLayer;
		}
		if (A.Producer != B.Producer)
		{
			return A.Producer < B.Producer;
		}
		return A.ProducerSequence < B.ProducerSequence;
	}

	template <typename ValueType>
	TArray<FNarrativeEntityId> SortedKeys(const TMap<FNarrativeEntityId, ValueType>& Map)
	{
		TArray<FNarrativeEntityId> Keys;
		Map.GetKeys(Keys);
		Keys.Sort();
		return Keys;
	}

	int64 ToIntervalTicks(float Seconds)
	{
		return FMath::Max<int64>(1, NarrativeTime::FromSeconds(Seconds));
	}

	void GatherOnMask(const FNarrativeMask& Mask, const FVectorND& Vector, TArray<double, TInlineAllocator<16>>& Out)
	{
		Out.Reset();
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
		{
			Out.Add(Vector.GetCoordinate(Axis));
		}
	}

	/* Every UPROPERTY of a leaf struct, as text: sheets and envelopes, whose fields grow with the model. */
	template <typename StructType>
	void WriteStructText(FArchive& Archive, const StructType& Value)
	{
		FString Text;
		StructType::StaticStruct()->ExportText(Text, &Value, nullptr, nullptr, PPF_None, nullptr);
		Archive << Text;
	}

	/* By axis identity, so a different storage order hashes the same; full precision. */
	void WriteVector(FArchive& Archive, const FVectorND& Vector)
	{
		// Basis and coordinates are parallel arrays; a corrupt vector can disagree on their length.
		TArray<TPair<FString, float>> Coordinates;
		const int32 Count = FMath::Min(Vector.Num(), Vector.GetBasis().Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Coordinates.Emplace(Vector.GetBasis()[Index].ToSoftObjectPath().ToString(), Vector[Index]);
		}
		Coordinates.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) { return A.Key < B.Key; });
		int32 Written = Coordinates.Num();
		Archive << Written;
		for (TPair<FString, float>& Coordinate : Coordinates)
		{
			Archive << Coordinate.Key << Coordinate.Value;
		}
	}

	void WriteMask(FArchive& Archive, const FNarrativeMask& Mask)
	{
		TArray<FString> Paths;
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
		{
			Paths.Add(Axis.ToSoftObjectPath().ToString());
		}
		Paths.Sort();
		Archive << Paths;
	}

	/* An empty axis entry is about nothing (lint L13); dropped so nothing downstream keys a coordinate by it. */
	FNarrativeMask WithoutNullAxes(const FNarrativeMask& Mask)
	{
		FNarrativeMask Out;
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
		{
			if (!Axis.IsNull())
			{
				Out.Axes.Add(Axis);
			}
		}
		return Out;
	}

	template <typename ElementType>
	TArray<ElementType> Sorted(const TSet<ElementType>& Set)
	{
		TArray<ElementType> Out = Set.Array();
		Out.Sort();
		return Out;
	}
}

uint64 FNarrativeEvent::DeriveCompletionKey() const
{
	if (CompletionKey != 0)
	{
		return CompletionKey;
	}
	if (ActivationId == 0)
	{
		return 0;
	}
	uint64 Key = NarrativeRng::Mix((static_cast<uint64>(Producer) << 8) | static_cast<uint64>(Kind));
	Key = NarrativeRng::Mix(Key ^ ActivationId);
	Key = NarrativeRng::Mix(Key ^ NarrativeRng::KeyOf(Instigator));
	Key = NarrativeRng::Mix(Key ^ NarrativeRng::KeyOf(Target));
	return Key == 0 ? 1 : Key;
}

FNarrativeCampaign::FNarrativeCampaign(uint64 Seed)
{
	Reset(Seed);
}

void FNarrativeCampaign::Reset(uint64 Seed)
{
	const uint32 Generation = Header.StoreGeneration + 1;
	Header = FNarrativeCampaignHeader();
	Header.Seed = Seed;
	Header.StoreGeneration = Generation;
	Header.IdNamespace = static_cast<uint32>(NarrativeRng::Mix(Seed));
	Stats = FNarrativeCampaignStats();
	Clock.Reset();
	Entities.Reset();
	Minds.Reset();
	FactionBiases.Reset();
	Ingress.Reset();
	CommittedEventIds.Reset();
	Completions.Reset();
	Reserved.Reset();
	RecentEvents.Reset();
	StanceNotices.Reset();
	TellNotices.Reset();
	CommittedNotices.Reset();
}

#pragma region Entities
FNarrativeEntityId FNarrativeCampaign::AllocateRuntimeId()
{
	return FNarrativeEntityId::FromCounter(Header.IdNamespace, Header.NextRuntimeId++);
}

FNarrativeEntityRecord* FNarrativeCampaign::RegisterEntity(const FNarrativeEntityId& Id, const UNarrativeEntityDef& Def)
{
	if (FNarrativeEntityRecord* Existing = Entities.Find(Id))
	{
		return Existing;
	}

	FNarrativeEntityRecord Record;
	Record.Id = Id;
	Record.TemplateId = FSoftObjectPath(&Def);
	Record.DisplayName = Def.GetName();
	Record.Position = Def.StartingCoordinates;
	Record.Mask = Def.Mask;
	Record.Mass = static_cast<float>(Def.Mass);
	Record.bIndividual = Def.bIndividual;
	if (const UArchetypeDef* Archetype = Def.ArchetypeAsset.Get())
	{
		Record.Archetype = FSoftObjectPath(Archetype);
		Record.ArchetypeLocation = Archetype->Location;
	}
	return RegisterEntity(Record, &Def.Mind);
}

FNarrativeEntityRecord* FNarrativeCampaign::RegisterEntity(const FNarrativeEntityRecord& Record, const FNarrativeMindSheet* Sheet)
{
	if (!ensure(Record.Id.IsValid()))
	{
		return nullptr;
	}
	if (FNarrativeEntityRecord* Existing = Entities.Find(Record.Id))
	{
		return Existing;
	}

	FNarrativeEntityRecord& Added = Entities.Add(Record.Id, Record);
	Added.Mask = WithoutNullAxes(Added.Mask);
	if (Sheet && Sheet->bHasMind)
	{
		AddMind(Record.Id, *Sheet);
	}
	UE_LOG(LogNarrativeTrace, Verbose, TEXT("Entity %s (%s) registered%s."), *Record.Id.ToShortString(), *Record.DisplayName,
		Minds.Contains(Record.Id) ? TEXT(" with a mind") : TEXT(""));
	return &Added;
}

FNarrativeMind& FNarrativeCampaign::AddMind(const FNarrativeEntityId& Observer, const FNarrativeMindSheet& Sheet)
{
	FNarrativeMind& Mind = Minds.Add(Observer);
	Mind.Observer = Observer;
	Mind.Sheet = Sheet;
	Mind.IntervalTicks = ToIntervalTicks(Sheet.DecisionInterval);
	Mind.PhaseTicks = static_cast<int64>(NarrativeRng::KeyOf(Observer) % static_cast<uint64>(Mind.IntervalTicks));
	ScheduleFirstDecision(Mind);
	return Mind;
}

void FNarrativeCampaign::ScheduleFirstDecision(FNarrativeMind& Mind) const
{
	// The first instant on this mind's phase that hasn't committed yet.
	const int64 Earliest = Clock.GetFrontier() + 1;
	const int64 Interval = FMath::Max<int64>(1, Mind.IntervalTicks);
	const int64 Offset = ((Earliest - Mind.PhaseTicks) % Interval + Interval) % Interval;
	Mind.NextDecisionTick = Offset == 0 ? Earliest : Earliest + (Interval - Offset);
}

void FNarrativeCampaign::RefreshFromTemplate(const UNarrativeEntityDef& Def)
{
	const FSoftObjectPath TemplateId(&Def);
	for (TPair<FNarrativeEntityId, FNarrativeEntityRecord>& Pair : Entities)
	{
		FNarrativeEntityRecord& Record = Pair.Value;
		if (Record.TemplateId != TemplateId)
		{
			continue;
		}

		Record.Position = Def.StartingCoordinates;
		Record.Mask = WithoutNullAxes(Def.Mask);
		Record.Mass = static_cast<float>(Def.Mass);
		Record.bIndividual = Def.bIndividual;
		Record.Archetype = Def.ArchetypeAsset ? FSoftObjectPath(Def.ArchetypeAsset.Get()) : FSoftObjectPath();
		Record.ArchetypeLocation = Def.ArchetypeAsset ? Def.ArchetypeAsset->Location : FVectorND();

		if (FNarrativeMind* Mind = Minds.Find(Pair.Key))
		{
			Mind->Sheet = Def.Mind;
			Mind->ArchetypeBeliefs.Reset();
			const int64 Interval = ToIntervalTicks(Def.Mind.DecisionInterval);
			if (Interval != Mind->IntervalTicks)
			{
				Mind->IntervalTicks = Interval;
				Mind->PhaseTicks = static_cast<int64>(NarrativeRng::KeyOf(Pair.Key) % static_cast<uint64>(Interval));
				ScheduleFirstDecision(*Mind);
			}
		}
		else if (Def.Mind.bHasMind)
		{
			AddMind(Pair.Key, Def.Mind);
		}
	}
}

void FNarrativeCampaign::SetFactions(const FNarrativeEntityId& Id, const TArray<FName>& Factions)
{
	if (FNarrativeEntityRecord* Record = Entities.Find(Id))
	{
		Record->Factions = Factions;
	}
}

void FNarrativeCampaign::SetFactionBias(FName ObserverFaction, FName SubjectFaction, double Bias)
{
	FactionBiases.Add(TPair<FName, FName>(ObserverFaction, SubjectFaction), FMath::Clamp(Bias, -1.0, 1.0));
}
#pragma endregion

#pragma region Ingress
uint64 FNarrativeCampaign::FindCompletion(uint64 CompletionKey) const
{
	const uint64* EventId = CompletionKey != 0 ? Completions.Find(CompletionKey) : nullptr;
	return EventId ? *EventId : 0;
}

uint64 FNarrativeCampaign::SubmitFact(FNarrativeEvent Fact, ENarrativeProducer Producer)
{
	if (!FNarrativeClock::IsValidProducer(Producer))
	{
		++Stats.InvalidFacts;
		UE_LOG(LogNarrativeTrace, Warning, TEXT("Fact (%s) names producer %d, which isn't one; refused."), *Fact.Label, static_cast<int32>(Producer));
		return 0;
	}

	// A retry that kept its assigned id is the event it already is. Only the store mints ids, so one it
	// never assigned is a forgery or corruption, refused.
	if (Fact.EventId != 0)
	{
		bool bKnown = CommittedEventIds.Contains(Fact.EventId);
		for (const TPair<int64, TArray<FNarrativeEvent>>& Pair : Ingress)
		{
			bKnown = bKnown || Pair.Value.ContainsByPredicate([&Fact](const FNarrativeEvent& Pending) { return Pending.EventId == Fact.EventId; });
		}
		if (!bKnown)
		{
			++Stats.InvalidFacts;
			UE_LOG(LogNarrativeTrace, Warning, TEXT("Fact (%s) carries event id %llu the store never assigned; refused."), *Fact.Label, Fact.EventId);
			return 0;
		}
		++Stats.DuplicateCompletions;
		UE_LOG(LogNarrativeTrace, Verbose, TEXT("Resubmitted event %llu (%s) absorbed."), Fact.EventId, *Fact.Label);
		return Fact.EventId;
	}

	// Once-only events (TDD Pattern 11): the same completion reported twice is one event.
	Fact.Producer = static_cast<uint8>(Producer);
	const uint64 CompletionKey = Fact.DeriveCompletionKey();
	if (const uint64 Existing = FindCompletion(CompletionKey))
	{
		++Stats.DuplicateCompletions;
		UE_LOG(LogNarrativeTrace, Verbose, TEXT("Repeated completion %llu from %s (%s) absorbed as event %llu."), CompletionKey, LexToString(Producer), *Fact.Label, Existing);
		return Existing;
	}
	Fact.CompletionKey = CompletionKey;

	if (Fact.OccurrenceTick < 0)
	{
		Fact.OccurrenceTick = Clock.GetHorizon();
	}

	if (Fact.OccurrenceTick <= Clock.GetFrontier())
	{
		// The producer vouched for this tick and then owed it anyway. Recover, but never by design.
		++Stats.LateFacts;
		UE_LOG(LogNarrativeTrace, Warning, TEXT("Late fact from %s for sealed tick %lld (frontier %lld): %s. Moved to the next open tick."), LexToString(Producer), Fact.OccurrenceTick, Clock.GetFrontier(), *Fact.Label);
		Fact.OccurrenceTick = Clock.GetFrontier() + 1;
	}

	Fact.ProducerSequence = Clock.NextSequence(Producer);
	Fact.EventId = FNarrativeEvent::MakeEventId(Fact.Producer, Fact.ProducerSequence);
	if (Fact.RootId == 0)
	{
		Fact.RootId = Fact.EventId;
	}
	if (Fact.ValuationUnitId == 0)
	{
		Fact.ValuationUnitId = Fact.EventId;
	}

	const uint64 EventId = Fact.EventId;
	if (CompletionKey != 0)
	{
		Completions.Add(CompletionKey, EventId);
	}
	Ingress.FindOrAdd(Fact.OccurrenceTick).Add(MoveTemp(Fact));
	return EventId;
}

int32 FNarrativeCampaign::GetPendingFactCount() const
{
	int32 Count = 0;
	for (const TPair<int64, TArray<FNarrativeEvent>>& Pair : Ingress)
	{
		Count += Pair.Value.Num();
	}
	return Count;
}

int32 FNarrativeCampaign::Pump(int32 DecisionBudget)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FNarrativeCampaign::Pump)

	if (!ensureMsgf(!bPumping, TEXT("FNarrativeCampaign::Pump re-entered from a listener.")))
	{
		return 0;
	}
	TGuardValue<bool> Guard(bPumping, true);

	int32 Decisions = 0;
	const int64 Sealed = Clock.GetSealedThrough();
	while (Clock.GetFrontier() < Sealed && Decisions < DecisionBudget)
	{
		// Jump straight to the next tick with work: an ingress fact or a scheduled decision.
		int64 NextWork = TNumericLimits<int64>::Max();
		for (const TPair<int64, TArray<FNarrativeEvent>>& Pair : Ingress)
		{
			NextWork = FMath::Min(NextWork, Pair.Key);
		}
		for (const TPair<FNarrativeEntityId, FNarrativeMind>& Pair : Minds)
		{
			NextWork = FMath::Min(NextWork, Pair.Value.NextDecisionTick);
		}

		if (NextWork > Sealed)
		{
			Clock.CommitThrough(Sealed);
			break;
		}

		CommitTick(NextWork, Decisions);
		Clock.CommitThrough(NextWork);
	}

	FlushNotifications();
	return Decisions;
}

void FNarrativeCampaign::CommitTick(int64 Tick, int32& InOutDecisions)
{
	if (TArray<FNarrativeEvent> Facts; Ingress.RemoveAndCopyValue(Tick, Facts))
	{
		Facts.StableSort(&PrecedesInTick);

		// One batch per causal layer: deltas within it sum, then cap; later layers cap separately.
		int32 Begin = 0;
		while (Begin < Facts.Num())
		{
			int32 End = Begin + 1;
			while (End < Facts.Num() && Facts[End].CausalLayer == Facts[Begin].CausalLayer)
			{
				++End;
			}
			CommitLayer(Tick, MakeArrayView(Facts.GetData() + Begin, End - Begin));
			Begin = End;
		}
	}

	RunDecisions(Tick, InOutDecisions);
}

void FNarrativeCampaign::CommitLayer(int64 Tick, TArrayView<FNarrativeEvent> Facts)
{
	struct FPairBatch
	{
		FNarrativeEntityId Observer;
		FNarrativeEntityId Instigator;
		double Sum = 0.0;
		TArray<FNarrativeValuationRecord> Records;
	};
	TArray<FPairBatch> Batches;

	// Every relation this batch reads or writes stays put until the batch is applied: a capacity
	// eviction mid-gather would drop a deed already counted (TDD Pattern 16).
	Reserved.Reset();
	auto Reserve = [this](const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject)
	{
		Reserved.Add(TPair<FNarrativeEntityId, FNarrativeEntityId>(Observer, Subject));
	};

	for (FNarrativeEvent& Fact : Facts)
	{
		// (1) Validate and commit world consequences once.
		if (CommittedEventIds.Contains(Fact.EventId))
		{
			++Stats.DuplicateEvents;
			continue;
		}
		if (!Fact.Instigator.IsValid() || !Entities.Contains(Fact.Instigator))
		{
			++Stats.InvalidFacts;
			UE_LOG(LogNarrativeTrace, Warning, TEXT("Fact %llu (%s) names no known instigator; dropped."), Fact.EventId, *Fact.Label);
			continue;
		}
		if (Fact.Target.IsValid() && !Entities.Contains(Fact.Target))
		{
			++Stats.InvalidFacts;
			UE_LOG(LogNarrativeTrace, Warning, TEXT("Fact %llu (%s) names an unknown target %s; dropped."), Fact.EventId, *Fact.Label, *Fact.Target.ToShortString());
			continue;
		}
		if (Fact.EnvelopeVersion != FNarrativeEvent::SchemaVersion)
		{
			++Stats.InvalidFacts;
			UE_LOG(LogNarrativeTrace, Warning, TEXT("Fact %llu (%s) has envelope version %d, expected %d; dropped."),
				Fact.EventId, *Fact.Label, Fact.EnvelopeVersion, FNarrativeEvent::SchemaVersion);
			continue;
		}
		CommittedEventIds.Add(Fact.EventId);
		++Stats.CommittedEvents;
		ApplyWorldConsequences(Tick, Fact);

		if (!Fact.IsDeed())
		{
			CommittedNotices.Add(Fact);
			continue;
		}

		// (2) Gather each recipient's observation, freezing care at the committed readings.
		TArray<FNarrativeEntityId, TInlineAllocator<8>> Witnesses;
		if (Fact.Recipients.IsEmpty())
		{
			Witnesses.Add(Fact.Instigator);
			if (Fact.Target.IsValid())
			{
				Witnesses.AddUnique(Fact.Target);
			}
		}
		else
		{
			for (const FNarrativeRecipient& Recipient : Fact.Recipients)
			{
				if (!Entities.Contains(Recipient.Id))
				{
					++Stats.UnknownRecipients;
					UE_LOG(LogNarrativeTrace, Verbose, TEXT("Fact %llu (%s): recipient %s is unknown; skipped."), Fact.EventId, *Fact.Label, *Recipient.Id.ToShortString());
					continue;
				}
				Witnesses.AddUnique(Recipient.Id);
			}
		}
		Witnesses.Sort();

		for (const FNarrativeEntityId& Witness : Witnesses)
		{
			FNarrativeMind* Mind = Minds.Find(Witness);
			const FNarrativeEntityRecord* WitnessRecord = Entities.Find(Witness);
			// A mind keeps no ledger of its own deeds, and the dead keep none at all.
			if (!Mind || Witness == Fact.Instigator || !WitnessRecord || WitnessRecord->LifeState == ENarrativeLifeState::Dead)
			{
				continue;
			}

			// Valued already, or gathered already in this batch: once per observer per unit.
			const bool bInBatch = Batches.ContainsByPredicate([&](const FPairBatch& Candidate)
			{
				return Candidate.Observer == Witness && Candidate.Records.ContainsByPredicate([&](const FNarrativeValuationRecord& Record)
				{
					return Record.ValuationUnitId == Fact.ValuationUnitId;
				});
			});
			if (bInBatch || Mind->ValuedUnits.Contains(Fact.ValuationUnitId))
			{
				++Stats.DuplicateValuations;
				continue;
			}

			// The deed's own relation first: the one that must exist for anything to be valued.
			FNarrativeRelation* InstigatorRelation = EnsureRelation(*Mind, Fact.Instigator, Tick);
			if (!InstigatorRelation)
			{
				// Refused for capacity, or not an individual: the unit stays unvalued for this mind, so nothing is consumed silently.
				continue;
			}
			Reserve(Witness, Fact.Instigator);

			double Care = 0.0;
			if (Fact.Target == Witness)
			{
				Care = 1.0;
			}
			else if (Fact.Target.IsValid())
			{
				// Care reads the witness's committed reading of the target, or its first impression if
				// the target is new; never the truth. An optional relation: without room, care is 0.
				if (const FNarrativeRelation* TargetRelation = EnsureRelation(*Mind, Fact.Target, Tick, /*bRequired*/ false))
				{
					Reserve(Witness, Fact.Target);
					Care = FMath::Clamp(TargetRelation->Reading, -1.0, 1.0);
				}
			}

			const double Approval = ApprovalOf(*Mind, Fact);
			const double TargetMass = Fact.Target.IsValid() ? FMath::Max(1.0, static_cast<double>(Fact.TargetMass)) : 1.0;
			const double Delta = NarrativeMath::RegardDelta(Fact.Strength, Fact.Stance, Care, Approval, TargetMass);

			FPairBatch* Batch = Batches.FindByPredicate([&](const FPairBatch& Candidate)
			{
				return Candidate.Observer == Witness && Candidate.Instigator == Fact.Instigator;
			});
			if (!Batch)
			{
				Batch = &Batches.AddDefaulted_GetRef();
				Batch->Observer = Witness;
				Batch->Instigator = Fact.Instigator;
			}
			Batch->Sum += Delta;

			FNarrativeValuationRecord& Record = Batch->Records.AddDefaulted_GetRef();
			Record.EventId = Fact.EventId;
			Record.ValuationUnitId = Fact.ValuationUnitId;
			Record.Tick = Tick;
			Record.Instigator = Fact.Instigator;
			Record.Target = Fact.Target;
			Record.Label = Fact.Label;
			Record.Care = Care;
			Record.Approval = Approval;
			Record.TargetMass = TargetMass;
			Record.RegardDelta = Delta;
		}

		CommittedNotices.Add(Fact);
	}

	// (3) Apply each pair's summed deltas once, then cap: decay to the deed's tick, then add. A unit
	// counts as valued only here, once its effect is committed.
	for (FPairBatch& Batch : Batches)
	{
		FNarrativeMind& Mind = Minds.FindChecked(Batch.Observer);
		FNarrativeRelation* Relation = Mind.Relations.Find(Batch.Instigator);
		if (!ensureMsgf(Relation, TEXT("A reserved relation %s -> %s vanished mid-batch."), *Batch.Observer.ToShortString(), *Batch.Instigator.ToShortString()))
		{
			continue;
		}

		const double Before = NarrativeMath::DecayRegard(Relation->Regard, NarrativeTime::ToSeconds(Tick - Relation->RegardTick), Mind.Sheet.RegardMemory);
		const double After = NarrativeMath::ClipRegard(Before + Batch.Sum, Mind.Sheet.Loyalty);
		Relation->Regard = After;
		Relation->RegardTick = Tick;
		Relation->LastTouchedTick = Tick;

		for (FNarrativeValuationRecord& Record : Batch.Records)
		{
			Mind.ValuedUnits.Add(Record.ValuationUnitId);
			++Stats.Valuations;
			Record.RegardBefore = Before;
			Record.RegardAfter = After;
			UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s values %s's '%s' (event %llu, unit %llu): care %.3f approval %.3f mass %.2f dR %+.3f; R %.3f -> %.3f"),
				Tick, *Batch.Observer.ToShortString(), *Batch.Instigator.ToShortString(), *Record.Label, Record.EventId, Record.ValuationUnitId,
				Record.Care, Record.Approval, Record.TargetMass, Record.RegardDelta, Before, After);
			Mind.RecentValuations.Add(Record);
		}
		if (Mind.RecentValuations.Num() > FNarrativeMind::MaxValuationRecords)
		{
			Mind.RecentValuations.RemoveAt(0, Mind.RecentValuations.Num() - FNarrativeMind::MaxValuationRecords);
		}
	}
	Reserved.Reset();
}

void FNarrativeCampaign::ApplyWorldConsequences(int64 Tick, const FNarrativeEvent& Fact)
{
	RecentEvents.Add(Fact);
	if (RecentEvents.Num() > MaxRecentEvents)
	{
		RecentEvents.RemoveAt(0, RecentEvents.Num() - MaxRecentEvents);
	}

	switch (Fact.Kind)
	{
	case ENarrativeEventKind::Encounter:
		{
			// A meeting: each side with a mind forms its first impression of the other. Optional, unless pinned.
			if (FNarrativeMind* Mind = Minds.Find(Fact.Instigator); Mind && Fact.Target.IsValid())
			{
				EnsureRelation(*Mind, Fact.Target, Tick, Mind->IsPinned(Fact.Target, Tick));
			}
			if (FNarrativeMind* Mind = Minds.Find(Fact.Target); Mind && Fact.Instigator.IsValid())
			{
				EnsureRelation(*Mind, Fact.Instigator, Tick, Mind->IsPinned(Fact.Instigator, Tick));
			}
		}
		break;

	case ENarrativeEventKind::Death:
		// Whoever answers for it, if anyone: the victim died either way.
		if (FNarrativeEntityRecord* Victim = Entities.Find(Fact.Target.IsValid() ? Fact.Target : Fact.Instigator))
		{
			Victim->LifeState = ENarrativeLifeState::Dead;
		}
		break;

	case ENarrativeEventKind::WarningSeen:
		if (FNarrativeMind* Mind = Minds.Find(Fact.Instigator))
		{
			FNarrativeRelation* Relation = Mind->Relations.Find(Fact.Subject);
			// Only the warning for this very turn, finished after it, opens the barrier.
			if (Relation && Relation->Tell.bActive && !Relation->Tell.bWarningSeen && Fact.OccurrenceTick >= Relation->Tell.StartTick
				&& Fact.ActivationId == Relation->Tell.GetIdentity())
			{
				Relation->Tell.bWarningSeen = true;
				Relation->Tell.WarningSeenTick = Tick;
				TellNotices.Add({FTellNotice::EKind::Seen, Fact.Instigator, Fact.Subject});
				UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s's warning to %s was seen."), Tick, *Fact.Instigator.ToShortString(), *Fact.Subject.ToShortString());
			}
		}
		break;

	default:
		break;
	}
}

void FNarrativeCampaign::RunDecisions(int64 Tick, int32& InOutDecisions)
{
	for (const FNarrativeEntityId& Observer : SortedKeys(Minds))
	{
		FNarrativeMind& Mind = Minds.FindChecked(Observer);
		if (Mind.NextDecisionTick != Tick)
		{
			continue;
		}

		const FNarrativeEntityRecord* Self = Entities.Find(Observer);
		if (Self && Self->LifeState == ENarrativeLifeState::Alive)
		{
			// One consistent snapshot: every relation of this mind is decided at this instant.
			for (const FNarrativeEntityId& Subject : SortedKeys(Mind.Relations))
			{
				Decide(Mind, Mind.Relations.FindChecked(Subject), Tick);
				++InOutDecisions;
			}
		}

		Mind.LastCommittedTick = Tick;
		// At least a tick on, or a mind with nothing to decide would hold the commit loop at this tick.
		Mind.NextDecisionTick = Tick + FMath::Max<int64>(1, Mind.IntervalTicks);
	}
}

void FNarrativeCampaign::Decide(FNarrativeMind& Mind, FNarrativeRelation& Relation, int64 Tick)
{
	const FNarrativeMindSheet& Sheet = Mind.Sheet;

	AdvanceNoise(Mind, Relation, Tick);

	const double RegardNow = NarrativeMath::DecayRegard(Relation.Regard, NarrativeTime::ToSeconds(Tick - Relation.RegardTick), Sheet.RegardMemory);
	const double Reading = ComputeReading(Mind, Relation, RegardNow, Relation.Parts);

	NarrativeMath::FSchmitt Trigger;
	Trigger.HostileThreshold = Sheet.HostileThreshold;
	Trigger.FriendlyThreshold = Sheet.FriendlyThreshold;
	Trigger.Hysteresis = Sheet.Hysteresis;
	ensureMsgf(Trigger.IsValid(), TEXT("%s's stance bands overlap: HostileThreshold + Hysteresis must stay below FriendlyThreshold - Hysteresis."),
		*Mind.Observer.ToShortString());

	const ENarrativeStance Old = Relation.Stance;
	const ENarrativeStance New = NarrativeStance::FromSign(Trigger.Update(NarrativeStance::ToSign(Old), Reading));

	const bool bFirst = !Relation.bDecided;
	Relation.Reading = Reading;
	Relation.Stance = New;
	Relation.AsOfTick = Tick;
	Relation.bDecided = true;
	++Relation.DecisionCount;
	++Relation.Revision;
	++Stats.Decisions;

	UE_LOG(LogNarrativeTrace, VeryVerbose, TEXT("[%lld] %s -> %s #%lld: pos %.3f tanhR %.3f bias %.3f reading %.3f %s"),
		Tick, *Mind.Observer.ToShortString(), *Relation.Subject.ToShortString(), Relation.DecisionCount,
		Relation.Parts.Position, Relation.Parts.TanhRegard, Relation.Parts.Bias, Reading, LexToString(New));

	if (New == Old)
	{
		// The first decision is news even when it stays None: views show None explicitly, not their default.
		if (bFirst)
		{
			StanceNotices.Add({Mind.Observer, Relation.Subject, Old, New, Tick});
		}
		return;
	}

	++Relation.Flips;
	StanceNotices.Add({Mind.Observer, Relation.Subject, Old, New, Tick});

	if (New == ENarrativeStance::Hostile)
	{
		// A social turn to Hostile raises the tell: harm waits until its warning has been seen.
		Relation.Tell = FNarrativeTell();
		Relation.Tell.bActive = true;
		Relation.Tell.StartTick = Tick;
		Relation.Tell.StartDecision = Relation.DecisionCount;
		for (int32 Index = Mind.RecentValuations.Num() - 1; Index >= 0; --Index)
		{
			if (Mind.RecentValuations[Index].Instigator == Relation.Subject)
			{
				Relation.Tell.CauseEventId = Mind.RecentValuations[Index].EventId;
				break;
			}
		}
		TellNotices.Add({FTellNotice::EKind::Started, Mind.Observer, Relation.Subject});
	}
	else if (Old == ENarrativeStance::Hostile && Relation.Tell.bActive)
	{
		const bool bWasPending = !Relation.Tell.bWarningSeen;
		Relation.Tell = FNarrativeTell();
		if (bWasPending)
		{
			// A reversal before release cancels the pending attack.
			TellNotices.Add({FTellNotice::EKind::Cancelled, Mind.Observer, Relation.Subject});
		}
	}

	UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s -> %s: %s -> %s (reading %.3f)"), Tick, *Mind.Observer.ToShortString(),
		*Relation.Subject.ToShortString(), LexToString(Old), LexToString(New), Reading);
}

FNarrativeRelation* FNarrativeCampaign::EnsureRelation(FNarrativeMind& Mind, const FNarrativeEntityId& Subject, int64 Tick, bool bRequired)
{
	if (FNarrativeRelation* Existing = Mind.Relations.Find(Subject))
	{
		Existing->LastTouchedTick = Tick;
		return Existing;
	}

	// Minds relate only to individuals: objects and shared kinds stay on the faction tables.
	const FNarrativeEntityRecord* SubjectRecord = Entities.Find(Subject);
	if (!SubjectRecord || Subject == Mind.Observer || !SubjectRecord->bIndividual)
	{
		return nullptr;
	}

	// A pinned subject (party, threat) is never optional.
	bRequired = bRequired || Mind.IsPinned(Subject, Tick);

	const FNarrativeMindSheet& Budget = Mind.Sheet;
	int32 Protected = 0;
	for (const TPair<FNarrativeEntityId, FNarrativeRelation>& Pair : Mind.Relations)
	{
		Protected += Mind.IsProtected(Pair.Value, Tick) ? 1 : 0;
	}

	// An exhausted protected budget is a capacity failure: it blocks new optional relations (Pattern 12).
	if (!bRequired && Protected >= FMath::Max(1, Budget.ProtectedCapacity))
	{
		++Mind.CapacityFailures;
		++Stats.CapacityFailures;
		UE_LOG(LogNarrativeTrace, Warning, TEXT("%s's protected budget is exhausted (%d of %d); refused an optional relation with %s."),
			*Mind.Observer.ToShortString(), Protected, Budget.ProtectedCapacity, *Subject.ToShortString());
		return nullptr;
	}

	if (Mind.Relations.Num() >= FMath::Max(1, Budget.RelationCapacity))
	{
		// Forget what matters least: never a protected relation or one the commit in progress holds; a
		// neutral one before a live grudge or debt; then the least recently touched; then by id.
		FNarrativeEntityId Forget;
		bool bForgetHasRegard = true;
		int64 Oldest = TNumericLimits<int64>::Max();
		for (const FNarrativeEntityId& Key : SortedKeys(Mind.Relations))
		{
			const FNarrativeRelation& Candidate = Mind.Relations.FindChecked(Key);
			if (Mind.IsProtected(Candidate, Tick) || Reserved.Contains(TPair<FNarrativeEntityId, FNarrativeEntityId>(Mind.Observer, Key)))
			{
				continue;
			}

			const double RegardNow = NarrativeMath::DecayRegard(Candidate.Regard, NarrativeTime::ToSeconds(Tick - Candidate.RegardTick), Budget.RegardMemory);
			const bool bHasRegard = FMath::Abs(RegardNow) >= FNarrativeMind::LiveRegard;
			const bool bBetter = !Forget.IsValid() || (!bHasRegard && bForgetHasRegard) || (bHasRegard == bForgetHasRegard && Candidate.LastTouchedTick < Oldest);
			if (bBetter)
			{
				Forget = Key;
				bForgetHasRegard = bHasRegard;
				Oldest = Candidate.LastTouchedTick;
			}
		}
		if (!Forget.IsValid())
		{
			++Mind.CapacityFailures;
			++Stats.CapacityFailures;
			UE_LOG(LogNarrativeTrace, Warning, TEXT("%s's relations are all protected or in use (%d protected); refused a relation with %s."),
				*Mind.Observer.ToShortString(), Protected, *Subject.ToShortString());
			return nullptr;
		}
		UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s forgets %s to make room for %s."), Tick, *Mind.Observer.ToShortString(),
			*Forget.ToShortString(), *Subject.ToShortString());
		Mind.Relations.Remove(Forget);
	}

	const FNarrativeMindSheet& Sheet = Mind.Sheet;
	const FNarrativeEntityRecord* Self = Entities.Find(Mind.Observer);
	const FVectorND SelfX = Self ? Self->Position : FVectorND();
	const FNarrativeMask& Mask = Self ? Self->Mask : FNarrativeMask();
	const FVectorND& Stereotype = GetArchetypeBelief(Mind, *SubjectRecord);

	FNarrativeRelation Relation;
	Relation.Subject = Subject;
	Relation.CreatedTick = Tick;
	Relation.LastTouchedTick = Tick;
	Relation.RegardTick = Tick;
	Relation.NoiseTick = Tick;
	Relation.AsOfTick = Tick;

	// First impression (§4.2): the believer's idea of the subject's archetype, leaning toward itself by
	// π. The subject's true position is never read. One stationary draw starts the noise.
	const NarrativeRng::FKey Draw(Header.Seed, Mind.Observer, Subject, ENarrativeRngPurpose::StanceNoise, 0);
	const double Variance = FMath::Square(static_cast<double>(Sheet.PriorSpread));
	TArray<double, TInlineAllocator<16>> SelfOnMask;
	TArray<double, TInlineAllocator<16>> StereotypeOnMask;
	GatherOnMask(Mask, SelfX, SelfOnMask);
	GatherOnMask(Mask, Stereotype, StereotypeOnMask);
	TArray<double, TInlineAllocator<16>> Means;
	Means.SetNumZeroed(Mask.Axes.Num());
	const TConstArrayView<double> StereotypeView(StereotypeOnMask);
	NarrativeMath::FirstImpressionMean(SelfOnMask, &StereotypeView, Sheet.PriorSelfWeight, Means);
	for (int32 Index = 0; Index < Mask.Axes.Num(); ++Index)
	{
		const TSoftObjectPtr<UNarrativeBasisVector>& Axis = Mask.Axes[Index];
		Relation.Mean.SetCoordinate(Axis, static_cast<float>(Means[Index]));
		Relation.Variance.SetCoordinate(Axis, static_cast<float>(Variance));
		Relation.Noise.SetCoordinate(Axis, static_cast<float>(Draw.Normal(NarrativeRng::AxisKey(Axis.ToSoftObjectPath()))));
	}
	Relation.PriorMean = Relation.Mean;
	Relation.NoiseIndex = 0;

	// Until its first decision, the first impression's reading serves as care.
	Relation.Reading = ComputeReading(Mind, Relation, 0.0, Relation.Parts);

	UE_LOG(LogNarrativeTrace, Verbose, TEXT("[%lld] %s meets %s (%s): first reading %.3f"), Tick, *Mind.Observer.ToShortString(),
		*Subject.ToShortString(), *SubjectRecord->DisplayName, Relation.Reading);
	return &Mind.Relations.Add(Subject, MoveTemp(Relation));
}

const FVectorND& FNarrativeCampaign::GetArchetypeBelief(FNarrativeMind& Mind, const FNarrativeEntityRecord& Subject)
{
	static const FVectorND Neutral;
	if (!Subject.Archetype.IsValid())
	{
		// Without an archetype the stereotype is neutral.
		return Neutral;
	}

	if (const FVectorND* Known = Mind.ArchetypeBeliefs.Find(Subject.Archetype))
	{
		return *Known;
	}

	// Each mind's archetype belief starts where the archetype is authored, unless its worldview says otherwise.
	const TSoftObjectPtr<UArchetypeDef> Key{Subject.Archetype};
	const FVectorND* Override = Mind.Sheet.WorldviewOverrides.Find(Key);
	return Mind.ArchetypeBeliefs.Add(Subject.Archetype, Override ? *Override : Subject.ArchetypeLocation);
}

void FNarrativeCampaign::AdvanceNoise(const FNarrativeMind& Mind, FNarrativeRelation& Relation, int64 Tick) const
{
	const FNarrativeEntityRecord* Self = Entities.Find(Mind.Observer);
	if (!Self)
	{
		return;
	}

	const int64 DeltaTicks = Tick - Relation.NoiseTick;
	const FNarrativeMask& Mask = Self->Mask;

	// An axis the mask gained after the relation was made starts at its stationary draw.
	const NarrativeRng::FKey Initial(Header.Seed, Mind.Observer, Relation.Subject, ENarrativeRngPurpose::StanceNoise, 0);
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
	{
		if (!Relation.Noise.GetBasis().Contains(Axis))
		{
			Relation.Noise.SetCoordinate(Axis, static_cast<float>(Initial.Normal(NarrativeRng::AxisKey(Axis.ToSoftObjectPath()))));
		}
	}

	if (DeltaTicks <= 0)
	{
		return;
	}

	// The exact OU transition, keyed by this relation's step number: the stance's distribution at
	// any instant is independent of when anyone looked (§6.2).
	++Relation.NoiseIndex;
	const NarrativeRng::FKey Step(Header.Seed, Mind.Observer, Relation.Subject, ENarrativeRngPurpose::StanceNoise, static_cast<uint64>(Relation.NoiseIndex));
	TArray<double, TInlineAllocator<16>> Z;
	TArray<double, TInlineAllocator<16>> Xi;
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Mask.Axes)
	{
		Z.Add(Relation.Noise.GetCoordinate(Axis));
		Xi.Add(Step.Normal(NarrativeRng::AxisKey(Axis.ToSoftObjectPath())));
	}
	NarrativeMath::OuStep(Z, NarrativeTime::ToSeconds(DeltaTicks), FMath::Max(0.01, static_cast<double>(Mind.Sheet.NoiseMemory)), Xi);
	for (int32 Index = 0; Index < Mask.Axes.Num(); ++Index)
	{
		Relation.Noise.SetCoordinate(Mask.Axes[Index], static_cast<float>(Z[Index]));
	}
	Relation.NoiseTick = Tick;
}

double FNarrativeCampaign::ComputeReading(const FNarrativeMind& Mind, const FNarrativeRelation& Relation, double RegardNow, FNarrativeReadingParts& OutParts) const
{
	const FNarrativeMindSheet& Sheet = Mind.Sheet;
	const FNarrativeEntityRecord* Self = Entities.Find(Mind.Observer);

	// a_pos: am I like the one I believe you to be? On my axes only, through the noise sample (§6.1).
	double Position = 0.0;
	if (Self && !Self->Mask.IsEmpty())
	{
		TArray<double, TInlineAllocator<16>> SelfX;
		TArray<double, TInlineAllocator<16>> Sample;
		GatherOnMask(Self->Mask, Self->Position, SelfX);
		for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Self->Mask.Axes)
		{
			const double Sd = FMath::Sqrt(FMath::Max(0.0, static_cast<double>(Relation.Variance.GetCoordinate(Axis))));
			Sample.Add(Relation.Mean.GetCoordinate(Axis) + Sd * Relation.Noise.GetCoordinate(Axis));
		}
		Position = NarrativeMath::MaskedCosine(SelfX, Sample);
	}

	OutParts.Position = Position;
	OutParts.TanhRegard = FMath::Tanh(RegardNow);
	OutParts.Bias = FMath::Clamp(static_cast<double>(Sheet.Bias) + GetFactionBias(Mind.Observer, Relation.Subject), -1.0, 1.0);
	OutParts.PositionWeight = Sheet.PositionWeight;
	OutParts.RegardWeight = Sheet.RegardWeight;
	return NarrativeMath::CombineReading(Position, RegardNow, Sheet.PositionWeight, Sheet.RegardWeight, OutParts.Bias);
}

double FNarrativeCampaign::GetFactionBias(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const
{
	const FNarrativeEntityRecord* A = Entities.Find(Observer);
	const FNarrativeEntityRecord* B = Entities.Find(Subject);
	if (!A || !B || FactionBiases.IsEmpty())
	{
		return 0.0;
	}

	// Stacked identities sum; the caller clamps.
	double Sum = 0.0;
	for (const FName& FactionA : A->Factions)
	{
		for (const FName& FactionB : B->Factions)
		{
			if (const double* Bias = FactionBiases.Find(TPair<FName, FName>(FactionA, FactionB)))
			{
				Sum += *Bias;
			}
		}
	}
	return Sum;
}

double FNarrativeCampaign::ApprovalOf(const FNarrativeMind& Mind, const FNarrativeEvent& Fact) const
{
	// Approval of what the act means, by the witness's own values, on the axes both are about.
	const FNarrativeEntityRecord* Self = Entities.Find(Mind.Observer);
	if (!Self)
	{
		return 0.0;
	}
	const FNarrativeMask Shared = Self->Mask.Intersect(Fact.Mask);
	if (Shared.IsEmpty())
	{
		return 0.0;
	}

	TArray<double> SelfX;
	TArray<double> Location;
	Shared.Gather(Self->Position, SelfX);
	Shared.Gather(Fact.Location, Location);
	return NarrativeMath::MaskedCosine(SelfX, Location);
}

void FNarrativeCampaign::FlushNotifications()
{
	TArray<FNarrativeEvent> Events = MoveTemp(CommittedNotices);
	TArray<FStanceNotice> Stances = MoveTemp(StanceNotices);
	TArray<FTellNotice> Tells = MoveTemp(TellNotices);
	CommittedNotices.Reset();
	StanceNotices.Reset();
	TellNotices.Reset();

	for (const FNarrativeEvent& Event : Events)
	{
		OnEventCommitted.Broadcast(Event);
	}
	for (const FStanceNotice& Notice : Stances)
	{
		OnStanceDecided.Broadcast(Notice.Observer, Notice.Subject, Notice.Old, Notice.New, Notice.Tick);
	}
	for (const FTellNotice& Notice : Tells)
	{
		switch (Notice.Kind)
		{
		case FTellNotice::EKind::Started: OnTellStarted.Broadcast(Notice.Observer, Notice.Subject); break;
		case FTellNotice::EKind::Cancelled: OnTellCancelled.Broadcast(Notice.Observer, Notice.Subject); break;
		case FTellNotice::EKind::Seen: OnWarningSeen.Broadcast(Notice.Observer, Notice.Subject); break;
		}
	}
}
#pragma endregion

#pragma region Reads
FNarrativeRelationSnapshot FNarrativeCampaign::ReadRelation(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const
{
	FNarrativeRelationSnapshot Snapshot;
	const FNarrativeRelation* Relation = FindRelation(Observer, Subject);
	if (!Relation)
	{
		return Snapshot;
	}

	Snapshot.bMissing = false;
	Snapshot.AsOfTick = Relation->AsOfTick;
	Snapshot.Reading = Relation->Reading;
	Snapshot.Stance = Relation->Stance;
	Snapshot.Revision = Relation->Revision;
	Snapshot.bDecided = Relation->bDecided;
	Snapshot.Regard = GetRegardAtFrontier(Observer, Subject);
	Snapshot.bTellActive = Relation->Tell.bActive;
	Snapshot.bWarningSeen = Relation->Tell.bWarningSeen;
	return Snapshot;
}

const FNarrativeRelation* FNarrativeCampaign::FindRelation(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const
{
	const FNarrativeMind* Mind = Minds.Find(Observer);
	return Mind ? Mind->Relations.Find(Subject) : nullptr;
}

bool FNarrativeCampaign::ReadBelief(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, const FNarrativeMask& Axes,
	TArray<double>& OutMean, TArray<double>& OutVariance, ENarrativeBeliefSource* OutSource) const
{
	OutMean.Reset();
	OutVariance.Reset();
	const FNarrativeEntityRecord* Self = Entities.Find(Observer);
	const FNarrativeEntityRecord* SubjectRecord = Entities.Find(Subject);
	if (!Self || !SubjectRecord)
	{
		return false;
	}

	if (Observer == Subject)
	{
		Axes.Gather(Self->Position, OutMean);
		OutVariance.SetNumZeroed(OutMean.Num());
		if (OutSource)
		{
			*OutSource = ENarrativeBeliefSource::Self;
		}
		return true;
	}

	// The first impression EnsureRelation would form, read without caching the archetype belief:
	// the believer's idea of the subject's archetype, leaning toward itself by π (§4.2).
	const FNarrativeMind* Mind = Minds.Find(Observer);
	const FNarrativeMindSheet Sheet = Mind ? Mind->Sheet : FNarrativeMindSheet();
	const FVectorND* Stereotype = nullptr;
	if (SubjectRecord->Archetype.IsValid())
	{
		Stereotype = Mind ? Mind->ArchetypeBeliefs.Find(SubjectRecord->Archetype) : nullptr;
		if (!Stereotype)
		{
			Stereotype = Sheet.WorldviewOverrides.Find(TSoftObjectPtr<UArchetypeDef>(SubjectRecord->Archetype));
		}
		if (!Stereotype)
		{
			Stereotype = &SubjectRecord->ArchetypeLocation;
		}
	}

	TArray<double> SelfOnAxes;
	TArray<double> StereotypeOnAxes;
	Axes.Gather(Self->Position, SelfOnAxes);
	if (Stereotype)
	{
		Axes.Gather(*Stereotype, StereotypeOnAxes);
	}
	OutMean.SetNumZeroed(Axes.Num());
	const TConstArrayView<double> StereotypeView(StereotypeOnAxes);
	NarrativeMath::FirstImpressionMean(SelfOnAxes, Stereotype ? &StereotypeView : nullptr, Sheet.PriorSelfWeight, OutMean);
	OutVariance.Init(FMath::Square(static_cast<double>(Sheet.PriorSpread)), Axes.Num());

	// What the relation holds overrides it, axis by axis.
	const FNarrativeRelation* Relation = Mind ? Mind->Relations.Find(Subject) : nullptr;
	int32 Held = 0;
	if (Relation)
	{
		for (int32 Index = 0; Index < Axes.Num(); ++Index)
		{
			const TSoftObjectPtr<UNarrativeBasisVector>& Axis = Axes.Axes[Index];
			if (Relation->Mean.GetBasis().Contains(Axis))
			{
				OutMean[Index] = Relation->Mean.GetCoordinate(Axis);
				OutVariance[Index] = FMath::Max(0.0, static_cast<double>(Relation->Variance.GetCoordinate(Axis)));
				++Held;
			}
		}
	}

	if (OutSource)
	{
		*OutSource = !Relation ? ENarrativeBeliefSource::FirstImpression
			: (Held == Axes.Num() ? ENarrativeBeliefSource::Relation : ENarrativeBeliefSource::RelationAndFirstImpression);
	}
	return true;
}

double FNarrativeCampaign::GetRegardAtFrontier(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject) const
{
	const FNarrativeMind* Mind = Minds.Find(Observer);
	const FNarrativeRelation* Relation = Mind ? Mind->Relations.Find(Subject) : nullptr;
	if (!Relation)
	{
		return 0.0;
	}
	const int64 Now = FMath::Max(Clock.GetFrontier(), Relation->RegardTick);
	return NarrativeMath::DecayRegard(Relation->Regard, NarrativeTime::ToSeconds(Now - Relation->RegardTick), Mind->Sheet.RegardMemory);
}

uint32 FNarrativeCampaign::ComputeStateHash(bool bIncludeClockDebt) const
{
	FBufferArchive Archive;

	// Header and counters: the seed changes every future draw; the id counters every future identity.
	{
		uint64 Seed = Header.Seed;
		FString Preset = Header.PresetId;
		uint32 Namespace = Header.IdNamespace;
		uint64 NextRuntime = Header.NextRuntimeId;
		uint64 Activation = Header.LastActivationId;
		int32 Model = FNarrativeCampaignHeader::ModelVersion;
		Archive << Seed << Preset << Namespace << NextRuntime << Activation << Model;
	}

	// The clock: the frontier, every producer's sequence and hold. Debt only when asked.
	{
		int64 Frontier = Clock.GetFrontier();
		Archive << Frontier;
		for (int32 Index = 0; Index < FNarrativeClock::NumProducers; ++Index)
		{
			const ENarrativeProducer Producer = static_cast<ENarrativeProducer>(Index);
			uint64 Sequence = Clock.GetSequence(Producer);
			bool bHeld = Clock.IsHeld(Producer);
			int64 Watermark = bHeld ? Clock.GetWatermark(Producer) : -1;
			Archive << Sequence << bHeld << Watermark;
		}
		if (bIncludeClockDebt)
		{
			int64 Horizon = Clock.GetHorizon();
			double Remainder = Clock.GetRemainder();
			Archive << Horizon << Remainder;
		}
	}

	// Pending work, in commit order, and the once-only ledgers by identity.
	{
		TArray<int64> Ticks;
		Ingress.GetKeys(Ticks);
		Ticks.Sort();
		for (int64 Tick : Ticks)
		{
			TArray<FNarrativeEvent> Pending = Ingress.FindChecked(Tick);
			Pending.StableSort(&PrecedesInTick);
			Archive << Tick;
			for (const FNarrativeEvent& Fact : Pending)
			{
				WriteStructText(Archive, Fact);
			}
		}

		TArray<uint64> Committed = Sorted(CommittedEventIds);
		Archive << Committed;

		TArray<uint64> Keys;
		Completions.GetKeys(Keys);
		Keys.Sort();
		for (uint64 Key : Keys)
		{
			uint64 EventId = Completions.FindChecked(Key);
			Archive << Key << EventId;
		}
	}

	{
		TArray<TPair<FName, FName>> BiasKeys;
		FactionBiases.GetKeys(BiasKeys);
		BiasKeys.Sort([](const TPair<FName, FName>& A, const TPair<FName, FName>& B)
		{
			return A.Key != B.Key ? A.Key.LexicalLess(B.Key) : A.Value.LexicalLess(B.Value);
		});
		for (const TPair<FName, FName>& Key : BiasKeys)
		{
			FString A = Key.Key.ToString();
			FString B = Key.Value.ToString();
			double Bias = FactionBiases.FindChecked(Key);
			Archive << A << B << Bias;
		}
	}

	for (const FNarrativeEntityId& Id : SortedKeys(Entities))
	{
		const FNarrativeEntityRecord& Record = Entities.FindChecked(Id);
		FGuid Guid = Id.Guid;
		FString Template = Record.TemplateId.ToString();
		FString Archetype = Record.Archetype.ToString();
		float Mass = Record.Mass;
		uint8 Life = static_cast<uint8>(Record.LifeState);
		bool bIndividual = Record.bIndividual;
		TArray<FString> Factions;
		for (const FName& Faction : Record.Factions)
		{
			Factions.Add(Faction.ToString());
		}
		Archive << Guid << Template << Archetype << Mass << Life << bIndividual << Factions;
		WriteVector(Archive, Record.Position);
		WriteVector(Archive, Record.ArchetypeLocation);
		WriteMask(Archive, Record.Mask);
	}

	for (const FNarrativeEntityId& Observer : SortedKeys(Minds))
	{
		const FNarrativeMind& Mind = Minds.FindChecked(Observer);
		FGuid Guid = Observer.Guid;
		int64 Phase = Mind.PhaseTicks;
		int64 Interval = Mind.IntervalTicks;
		int64 Next = Mind.NextDecisionTick;
		int64 Last = Mind.LastCommittedTick;
		int32 Failures = Mind.CapacityFailures;
		Archive << Guid << Phase << Interval << Next << Last << Failures;
		WriteStructText(Archive, Mind.Sheet);

		TArray<FSoftObjectPath> Archetypes;
		Mind.ArchetypeBeliefs.GetKeys(Archetypes);
		Archetypes.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B) { return A.ToString() < B.ToString(); });
		for (const FSoftObjectPath& Archetype : Archetypes)
		{
			FString Path = Archetype.ToString();
			Archive << Path;
			WriteVector(Archive, Mind.ArchetypeBeliefs.FindChecked(Archetype));
		}

		for (const FNarrativeEntityId& Subject : SortedKeys(Mind.Pins))
		{
			const FNarrativePin& Pin = Mind.Pins.FindChecked(Subject);
			FGuid SubjectGuid = Subject.Guid;
			bool bParty = Pin.bParty;
			int64 Until = Pin.ThreatUntilTick;
			Archive << SubjectGuid << bParty << Until;
		}

		TArray<FNarrativeEntityId> Obligations = Mind.Obligations;
		Obligations.Sort();
		for (const FNarrativeEntityId& Obligation : Obligations)
		{
			FGuid ObligationGuid = Obligation.Guid;
			Archive << ObligationGuid;
		}

		// Which units, not how many: two minds with the same count can disagree about the next completion.
		TArray<uint64> Valued = Sorted(Mind.ValuedUnits);
		Archive << Valued;

		for (const FNarrativeEntityId& Subject : SortedKeys(Mind.Relations))
		{
			const FNarrativeRelation& Relation = Mind.Relations.FindChecked(Subject);
			FGuid SubjectGuid = Subject.Guid;
			double Regard = Relation.Regard;
			int64 RegardTick = Relation.RegardTick;
			int64 NoiseTick = Relation.NoiseTick;
			int64 NoiseIndex = Relation.NoiseIndex;
			int64 Decisions = Relation.DecisionCount;
			double Reading = Relation.Reading;
			uint8 Stance = static_cast<uint8>(Relation.Stance);
			int64 AsOf = Relation.AsOfTick;
			uint32 Revision = Relation.Revision;
			bool bDecided = Relation.bDecided;
			int64 Created = Relation.CreatedTick;
			int64 Touched = Relation.LastTouchedTick;
			int32 Flips = Relation.Flips;
			Archive << SubjectGuid << Regard << RegardTick << NoiseTick << NoiseIndex << Decisions << Reading << Stance << AsOf << Revision
				<< bDecided << Created << Touched << Flips;
			WriteStructText(Archive, Relation.Tell);
			WriteVector(Archive, Relation.Mean);
			WriteVector(Archive, Relation.Variance);
			WriteVector(Archive, Relation.PriorMean);
			WriteVector(Archive, Relation.Noise);
		}
	}

	return FCrc::MemCrc32(Archive.GetData(), Archive.Num());
}
#pragma endregion

#pragma region Game hooks
void FNarrativeCampaign::SetPartyPins(const TArray<FNarrativeEntityId>& Members)
{
	for (TPair<FNarrativeEntityId, FNarrativeMind>& Pair : Minds)
	{
		FNarrativeMind& Mind = Pair.Value;
		const bool bMember = Members.Contains(Pair.Key);
		for (auto It = Mind.Pins.CreateIterator(); It; ++It)
		{
			It.Value().bParty = bMember && Members.Contains(It.Key());
			if (!It.Value().bParty && It.Value().ThreatUntilTick < 0)
			{
				It.RemoveCurrent();
			}
		}
		if (!bMember)
		{
			continue;
		}
		for (const FNarrativeEntityId& Other : Members)
		{
			if (Other != Pair.Key && Other.IsValid())
			{
				Mind.Pins.FindOrAdd(Other).bParty = true;
			}
		}
	}
}

void FNarrativeCampaign::PinThreat(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Source, int64 UntilTick)
{
	if (FNarrativeMind* Mind = Minds.Find(Observer); Mind && Source.IsValid() && Source != Observer)
	{
		FNarrativePin& Pin = Mind->Pins.FindOrAdd(Source);
		Pin.ThreatUntilTick = FMath::Max(Pin.ThreatUntilTick, UntilTick);
	}
}

void FNarrativeCampaign::UnpinThreat(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Source)
{
	FNarrativeMind* Mind = Minds.Find(Observer);
	if (FNarrativePin* Pin = Mind ? Mind->Pins.Find(Source) : nullptr)
	{
		Pin->ThreatUntilTick = -1;
		if (!Pin->bParty)
		{
			Mind->Pins.Remove(Source);
		}
	}
}
#pragma endregion

#pragma region Fixtures
bool FNarrativeCampaign::FixtureSetBelief(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, const FVectorND& Mean, double Spread)
{
	FNarrativeMind* Mind = Minds.Find(Observer);
	const FNarrativeEntityRecord* Self = Entities.Find(Observer);
	if (!Mind || !Self)
	{
		return false;
	}

	FNarrativeRelation* Relation = EnsureRelation(*Mind, Subject, Clock.GetFrontier() + 1);
	if (!Relation)
	{
		return false;
	}

	const double Variance = FMath::Square(Spread);
	for (const TSoftObjectPtr<UNarrativeBasisVector>& Axis : Self->Mask.Axes)
	{
		Relation->Mean.SetCoordinate(Axis, Mean.GetCoordinate(Axis));
		Relation->Variance.SetCoordinate(Axis, static_cast<float>(Variance));
	}
	Relation->PriorMean = Relation->Mean;
	if (!Relation->bDecided)
	{
		Relation->Reading = ComputeReading(*Mind, *Relation, 0.0, Relation->Parts);
	}
	return true;
}

bool FNarrativeCampaign::FixtureSetSheet(const FNarrativeEntityId& Id, const FNarrativeMindSheet& Sheet, const FNarrativeMask* Mask)
{
	FNarrativeEntityRecord* Record = Entities.Find(Id);
	if (!Record)
	{
		return false;
	}

	if (Mask)
	{
		Record->Mask = WithoutNullAxes(*Mask);
	}

	FNarrativeMindSheet Running = Sheet;
	Running.bHasMind = true;
	if (FNarrativeMind* Mind = Minds.Find(Id))
	{
		Mind->Sheet = Running;
		Mind->ArchetypeBeliefs.Reset();
		const int64 Interval = ToIntervalTicks(Running.DecisionInterval);
		if (Interval != Mind->IntervalTicks)
		{
			Mind->IntervalTicks = Interval;
			Mind->PhaseTicks = static_cast<int64>(NarrativeRng::KeyOf(Id) % static_cast<uint64>(Interval));
			ScheduleFirstDecision(*Mind);
		}
	}
	else
	{
		AddMind(Id, Running);
	}
	return true;
}

bool FNarrativeCampaign::FixtureSetRegard(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, double R)
{
	FNarrativeMind* Mind = Minds.Find(Observer);
	FNarrativeRelation* Relation = Mind ? EnsureRelation(*Mind, Subject, Clock.GetFrontier() + 1) : nullptr;
	if (!Relation)
	{
		return false;
	}
	Relation->Regard = NarrativeMath::ClipRegard(R, Mind->Sheet.Loyalty);
	Relation->RegardTick = Clock.GetFrontier() + 1;
	return true;
}

bool FNarrativeCampaign::FixtureMeet(const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject)
{
	FNarrativeMind* Mind = Minds.Find(Observer);
	return Mind && EnsureRelation(*Mind, Subject, Clock.GetFrontier() + 1) != nullptr;
}
#pragma endregion
