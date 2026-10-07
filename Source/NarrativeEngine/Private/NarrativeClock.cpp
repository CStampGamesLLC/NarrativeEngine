#include "NarrativeClock.h"

const TCHAR* LexToString(ENarrativeProducer Producer)
{
	switch (Producer)
	{
	case ENarrativeProducer::Dialog: return TEXT("Dialog");
	case ENarrativeProducer::Gifts: return TEXT("Gifts");
	case ENarrativeProducer::Combat: return TEXT("Combat");
	case ENarrativeProducer::Roster: return TEXT("Roster");
	case ENarrativeProducer::Scheduler: return TEXT("Scheduler");
	case ENarrativeProducer::Workers: return TEXT("Workers");
	case ENarrativeProducer::Perception: return TEXT("Perception");
	case ENarrativeProducer::Fixture: return TEXT("Fixture");
	default: return TEXT("?");
	}
}

FNarrativeClock::FNarrativeClock()
{
	Reset();
}

int32 FNarrativeClock::ToIndex(ENarrativeProducer Producer)
{
	return ensureMsgf(IsValidProducer(Producer), TEXT("Invalid narrative producer %d."), static_cast<int32>(Producer))
		? static_cast<int32>(Producer) : INDEX_NONE;
}

void FNarrativeClock::Reset()
{
	Horizon = 0;
	Remainder = 0.0;
	Frontier = -1;
	MaxLagTicks = 0;
	for (int32 Index = 0; Index < NumProducers; ++Index)
	{
		Watermarks[Index] = -1;
		Held[Index] = false;
		Sequences[Index] = 0;
	}
}

void FNarrativeClock::Advance(double GameDeltaSeconds)
{
	if (!(GameDeltaSeconds > 0.0))
	{
		return;
	}

	// Whole ticks only; the remainder carries, so a run of small frames loses no time.
	Remainder += GameDeltaSeconds;
	const int64 Whole = static_cast<int64>(FMath::FloorToDouble(Remainder * NarrativeTime::TicksPerSecond + 1e-9));
	if (Whole > 0)
	{
		Horizon += Whole;
		Remainder = FMath::Max(0.0, Remainder - NarrativeTime::ToSeconds(Whole));
	}
	MaxLagTicks = FMath::Max(MaxLagTicks, GetLagTicks());
}

int64 FNarrativeClock::GetWatermark(ENarrativeProducer Producer) const
{
	const int32 Index = ToIndex(Producer);
	return Index != INDEX_NONE && Held[Index] ? Watermarks[Index] : Horizon - 1;
}

bool FNarrativeClock::IsHeld(ENarrativeProducer Producer) const
{
	const int32 Index = ToIndex(Producer);
	return Index != INDEX_NONE && Held[Index];
}

uint64 FNarrativeClock::NextSequence(ENarrativeProducer Producer)
{
	const int32 Index = ToIndex(Producer);
	return Index != INDEX_NONE ? ++Sequences[Index] : 0;
}

uint64 FNarrativeClock::GetSequence(ENarrativeProducer Producer) const
{
	const int32 Index = ToIndex(Producer);
	return Index != INDEX_NONE ? Sequences[Index] : 0;
}

int64 FNarrativeClock::GetSealedThrough() const
{
	int64 Sealed = Horizon - 1;
	for (int32 Index = 0; Index < NumProducers; ++Index)
	{
		if (Held[Index])
		{
			Sealed = FMath::Min(Sealed, Watermarks[Index]);
		}
	}
	return Sealed;
}

void FNarrativeClock::CommitThrough(int64 Tick)
{
	if (ensureMsgf(Tick <= GetSealedThrough(), TEXT("Committing tick %lld past the sealed tick %lld."), Tick, GetSealedThrough()))
	{
		Frontier = FMath::Max(Frontier, Tick);
	}
}

void FNarrativeClock::HoldWatermark(ENarrativeProducer Producer, int64 OldestOwedTick)
{
	const int32 Index = ToIndex(Producer);
	if (Index == INDEX_NONE)
	{
		return;
	}

	// What it owes from OldestOwedTick on isn't submitted yet, so it vouches only for the tick before.
	const int64 Through = OldestOwedTick - 1;
	Watermarks[Index] = Held[Index] ? FMath::Min(Watermarks[Index], Through) : Through;
	Held[Index] = true;
}

void FNarrativeClock::SetHold(ENarrativeProducer Producer, int64 OldestOwedTick)
{
	const int32 Index = ToIndex(Producer);
	if (Index == INDEX_NONE)
	{
		return;
	}

	// Never behind what already committed: a hold can delay sealing, not reopen a sealed tick.
	Watermarks[Index] = FMath::Max(OldestOwedTick - 1, Frontier);
	Held[Index] = true;
}

void FNarrativeClock::ReleaseHold(ENarrativeProducer Producer)
{
	const int32 Index = ToIndex(Producer);
	if (Index != INDEX_NONE)
	{
		Held[Index] = false;
	}
}

void FNarrativeClock::ConfirmThrough(ENarrativeProducer Producer, int64 Tick)
{
	const int32 Index = ToIndex(Producer);
	if (Index == INDEX_NONE || !Held[Index])
	{
		return;
	}

	Watermarks[Index] = FMath::Max(Watermarks[Index], Tick);
	if (Watermarks[Index] >= Horizon - 1)
	{
		// Caught up with game time: back to confirming implicitly.
		Held[Index] = false;
	}
}
