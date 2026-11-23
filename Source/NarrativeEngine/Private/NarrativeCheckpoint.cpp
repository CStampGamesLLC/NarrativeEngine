// Copyright Timeless Way Interactive, LLC. All Rights Reserved.

#include "NarrativeCampaign.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace
{
	/* 2 (P1): the party record and the checkpoint id. 1 still loads, with the party left for the game to install. */
	constexpr int32 CheckpointVersion = 2;
	constexpr int32 MaxRecords = 1000000;
	struct FCheckpointReader : FMemoryReader
	{
		explicit FCheckpointReader(const TArray<uint8>& Bytes) : FMemoryReader(Bytes)
		{
			ArMaxSerializeSize = Bytes.Num();
		}
	};

	int32 Count(FArchive& Ar, int32 SavingCount)
	{
		int32 Value = SavingCount;
		Ar << Value;
		if (Value < 0 || Value > MaxRecords)
		{
			Ar.SetError();
			return 0;
		}
		return Ar.IsError() ? 0 : Value;
	}

	template <typename T> void Record(FArchive& Ar, T& Value)
	{
		T::StaticStruct()->SerializeItem(Ar, &Value, nullptr);
	}

	template <typename T> void Records(FArchive& Ar, TMap<FNarrativeEntityId, T>& Values)
	{
		const int32 Num = Count(Ar, Values.Num());
		if (Ar.IsLoading())
		{
			Values.Reset();
			for (int32 Index = 0; Index < Num && !Ar.IsError(); ++Index)
			{
				FNarrativeEntityId Key;
				T Value;
				Ar << Key.Guid;
				Record(Ar, Value);
				if (!Key.IsValid() || Values.Contains(Key))
				{
					Ar.SetError();
					break;
				}
				Values.Add(Key, MoveTemp(Value));
			}
		}
		else
		{
			for (auto& Entry : Values)
			{
				Ar << Entry.Key.Guid;
				Record(Ar, Entry.Value);
			}
		}
	}

	void Events(FArchive& Ar, TArray<FNarrativeEvent>& Values)
	{
		const int32 Num = Count(Ar, Values.Num());
		if (Ar.IsLoading())
		{
			Values.SetNum(Num);
		}
		for (auto& Value : Values)
		{
			if (Ar.IsError())
			{
				break;
			}
			Record(Ar, Value);
		}
	}
}

void FNarrativeCampaign::SerializeCheckpoint(FArchive& Ar)
{
	uint32 Magic = 0x44424350; // DBCP
	int32 Version = CheckpointVersion;
	int32 ModelVersion = FNarrativeCampaignHeader::ModelVersion;
	Ar << Magic << Version << ModelVersion;
	if (Magic != 0x44424350 || Version < 1 || Version > CheckpointVersion || ModelVersion != FNarrativeCampaignHeader::ModelVersion)
	{
		Ar.SetError();
		return;
	}
	Ar << Header.Seed << Header.PresetId << Header.StoreGeneration << Header.IdNamespace << Header.NextRuntimeId
	   << Header.LastActivationId;
	if (Version >= 2)
	{
		Ar << Header.CheckpointId;
	}
	Clock.SerializeCheckpoint(Ar);
	Records(Ar, Entities);
	Records(Ar, Minds);

	const int32 BiasCount = Count(Ar, FactionBiases.Num());
	if (Ar.IsLoading())
	{
		FactionBiases.Reset();
		for (int32 I = 0; I < BiasCount && !Ar.IsError(); ++I)
		{
			FName A, B;
			double Bias = 0;
			Ar << A << B << Bias;
			if (!FMath::IsFinite(Bias))
			{
				Ar.SetError();
				break;
			}
			FactionBiases.Add({A, B}, Bias);
		}
	}
	else
	{
		for (auto& Entry : FactionBiases)
		{
			Ar << Entry.Key.Key << Entry.Key.Value << Entry.Value;
		}
	}

	const int32 TickCount = Count(Ar, Ingress.Num());
	if (Ar.IsLoading())
	{
		Ingress.Reset();
		for (int32 I = 0; I < TickCount && !Ar.IsError(); ++I)
		{
			int64 Tick = 0;
			Ar << Tick;
			Events(Ar, Ingress.FindOrAdd(Tick));
		}
	}
	else
	{
		for (auto& Entry : Ingress)
		{
			Ar << Entry.Key;
			Events(Ar, Entry.Value);
		}
	}
	Ar << CommittedEventIds << Completions;
	Events(Ar, RecentEvents);

	const int32 PairCount = Count(Ar, DialogPairs.Num());
	if (Ar.IsLoading())
	{
		DialogPairs.Reset();
		for (int32 I = 0; I < PairCount && !Ar.IsError(); ++I)
		{
			FNarrativeEntityId Speaker, Addressee;
			Ar << Speaker.Guid << Addressee.Guid;
			auto& Pair = DialogPairs.FindOrAdd({Speaker, Addressee});
			Ar << Pair.Opportunities << Pair.RecentOffers;
		}
	}
	else
	{
		for (auto& Entry : DialogPairs)
		{
			Ar << Entry.Key.Key.Guid << Entry.Key.Value.Guid << Entry.Value.Opportunities << Entry.Value.RecentOffers;
		}
	}

	// The roster by entity id (TDD Pattern 21): a load installs it whole, so it never reads as a join or a departure.
	if (Version >= 2)
	{
		Record(Ar, Party);
	}
	else if (Ar.IsLoading())
	{
		Party = FNarrativePartyRecord();
	}
}

bool FNarrativeCampaign::ExportCheckpoint(TArray<uint8>& Bytes, uint64* OutCheckpointId) const
{
	if (bPumping || !Reserved.IsEmpty())
	{
		return false;
	}
	Bytes.Reset();
	FMemoryWriter Writer(Bytes);
	FObjectAndNameAsStringProxyArchive Archive(Writer, false);
	FNarrativeCampaign* Mutable = const_cast<FNarrativeCampaign*>(this);
	// A fresh id per export, unique across save branches (two saves played on from one load never share one): the game
	// save keeps it beside its actor half, and a load refuses a pair that disagrees. Not narrative truth, so never hashed.
	const FGuid Unique = FGuid::NewGuid();
	Mutable->Header.CheckpointId = ((static_cast<uint64>(Unique.A) << 32) | Unique.B) ^ ((static_cast<uint64>(Unique.C) << 32) | Unique.D);
	if (Mutable->Header.CheckpointId == 0)
	{
		Mutable->Header.CheckpointId = 1;
	}
	if (OutCheckpointId)
	{
		*OutCheckpointId = Header.CheckpointId;
	}
	Mutable->SerializeCheckpoint(Archive);
	return !Archive.IsError() && Bytes.Num() <= 64 * 1024 * 1024;
}

bool FNarrativeCampaign::ImportCheckpoint(const TArray<uint8>& Bytes, FString& Error, uint64 ExpectedCheckpointId)
{
	if (bPumping || Bytes.IsEmpty() || Bytes.Num() > 64 * 1024 * 1024)
	{
		Error = TEXT("Campaign checkpoint is missing, too large, or the campaign is committing.");
		return false;
	}
	FNarrativeCampaign Candidate;
	FCheckpointReader Reader(Bytes);
	FObjectAndNameAsStringProxyArchive Archive(Reader, false);
	Candidate.SerializeCheckpoint(Archive);
	if (Archive.IsError() || Reader.Tell() != Bytes.Num())
	{
		Error = TEXT("Campaign checkpoint is invalid or has an unsupported version.");
		return false;
	}
	for (const auto& Pair : Candidate.Entities)
	{
		if (Pair.Key != Pair.Value.Id || !Pair.Value.Position.IsValid() || !Pair.Value.ArchetypeLocation.IsValid())
		{
			Error = TEXT("Campaign checkpoint contains an invalid entity.");
			return false;
		}
	}
	for (const auto& Pair : Candidate.Minds)
	{
		if (!Candidate.Entities.Contains(Pair.Key) || Pair.Key != Pair.Value.Observer || Pair.Value.IntervalTicks <= 0)
		{
			Error = TEXT("Campaign checkpoint contains an invalid mind.");
			return false;
		}
		for (const auto& Relation : Pair.Value.Relations)
		{
			const auto& Value = Relation.Value;
			if (Relation.Key != Value.Subject || !Value.Mean.IsValid() || !Value.Variance.IsValid() ||
				!Value.Noise.IsValid() || !Value.PriorMean.IsValid() || !FMath::IsFinite(Value.Regard) ||
				!FMath::IsFinite(Value.Affinity))
			{
				Error = TEXT("Campaign checkpoint contains an invalid relation.");
				return false;
			}
		}
	}
	if (ExpectedCheckpointId != 0 && Candidate.Header.CheckpointId != ExpectedCheckpointId)
	{
		Error = TEXT("Campaign checkpoint belongs to a different save than its actor state.");
		return false;
	}
	for (const FNarrativeEntityId& Member : Candidate.Party.Members)
	{
		if (!Candidate.Entities.Contains(Member) || Candidate.Party.Members.FilterByPredicate([&Member](const FNarrativeEntityId& Id) { return Id == Member; }).Num() != 1)
		{
			Error = TEXT("Campaign checkpoint's party names an unknown or repeated member.");
			return false;
		}
	}
	for (const auto& Tick : Candidate.Ingress)
	{
		for (const auto& Event : Tick.Value)
		{
			if (Tick.Key <= Candidate.Clock.GetFrontier() || Event.OccurrenceTick != Tick.Key ||
				Event.EnvelopeVersion != FNarrativeEvent::SchemaVersion ||
				Event.Producer >= FNarrativeClock::NumProducers || !Event.Location.IsValid())
			{
				Error = TEXT("Campaign checkpoint contains invalid pending work.");
				return false;
			}
		}
	}
	Header = MoveTemp(Candidate.Header);
	Clock = MoveTemp(Candidate.Clock);
	Entities = MoveTemp(Candidate.Entities);
	Minds = MoveTemp(Candidate.Minds);
	FactionBiases = MoveTemp(Candidate.FactionBiases);
	Ingress = MoveTemp(Candidate.Ingress);
	CommittedEventIds = MoveTemp(Candidate.CommittedEventIds);
	Completions = MoveTemp(Candidate.Completions);
	RecentEvents = MoveTemp(Candidate.RecentEvents);
	DialogPairs = MoveTemp(Candidate.DialogPairs);
	Party = MoveTemp(Candidate.Party);
	PartyNotices.Reset();
	// A tell against a member whose world warning was seen before the save plays again before anyone strikes (Pattern 21).
	for (TPair<FNarrativeEntityId, FNarrativeMind>& Mind : Minds)
	{
		for (TPair<FNarrativeEntityId, FNarrativeRelation>& Relation : Mind.Value.Relations)
		{
			FNarrativeTell& Tell = Relation.Value.Tell;
			Tell.bWarningReplayDue = Tell.bActive && Tell.bWarningSeen && IsPartyMember(Relation.Key);
		}
	}
	CurrentBeat = nullptr;
	Reserved.Reset();
	StanceNotices.Reset();
	TellNotices.Reset();
	CommittedNotices.Reset();
	Stats = FNarrativeCampaignStats(); // Telemetry belongs to this run, not narrative truth.
	return true;
}
