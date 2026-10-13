#include "NarrativeChoiceTrace.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarChoiceTraceCapacity(
		TEXT("db.Narrative.ChoiceTrace.Capacity"),
		32,
		TEXT("How many choice-rule traces (dealt hands and replies) the Dialog Choice admin panel can scrub back through."),
		ECVF_Default);

	FAutoConsoleCommand DumpChoiceTraces(
		TEXT("db.Narrative.ChoiceTrace.Dump"),
		TEXT("Logs the newest choice-rule traces, newest last. Optional argument: how many (default 1)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const FNarrativeChoiceTraceLog& Log = FNarrativeChoiceTraceLog::Get();
			const int32 Count = FMath::Clamp(Args.IsEmpty() ? 1 : FCString::Atoi(*Args[0]), 1, FMath::Max(1, Log.Num()));
			if (Log.Num() == 0)
			{
				UE_LOG(LogNarrativeTrace, Display, TEXT("No choice traces held. Deal one with db.Narrative.Dealer 1."));
			}
			for (int32 Index = FMath::Max(0, Log.Num() - Count); Index < Log.Num(); ++Index)
			{
				TArray<FString> Lines;
				Log.GetOrdered(Index).Describe().ParseIntoArrayLines(Lines, false);
				for (const FString& Line : Lines)
				{
					UE_LOG(LogNarrativeTrace, Display, TEXT("%s"), *Line);
				}
			}
		}));
}

FString FNarrativeChoiceTrace::Describe() const
{
	FString Out = FString::Printf(TEXT("Choice trace #%llu at %.2f s: %s -> %s (%s)\n"), static_cast<unsigned long long>(Sequence), WorldSeconds,
		*Speaker, *Addressee, bPlayerHand ? TEXT("player hand") : TEXT("reply"));
	if (!FallbackReason.IsEmpty())
	{
		return Out + FString::Printf(TEXT("  legacy draw dealt: %s\n"), *FallbackReason);
	}

	Out += FString::Printf(TEXT("  belief %s, A~ %s, P(silence) %.3f, %d cards over %d families\n"), LexToString(BeliefSource),
		bHasReading ? *FString::Printf(TEXT("%+.3f"), Evaluation.Reading) : TEXT("none"), Evaluation.SilenceProbability, Cards.Num(), Evaluation.Families.Num());
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		const NarrativeChoice::FDealSlot& Dealt = Slots[Slot];
		if (Dealt.IsSilence())
		{
			Out += FString::Printf(TEXT("  reply: roll %.3f -> silence (P %.3f against %d families)\n"), Dealt.FamilyRoll, Dealt.SilenceWeight, Dealt.Candidates.Num());
			continue;
		}
		Out +=FString::Printf(TEXT("  card %d: roll %.3f -> #%d %s (%d of %d left)%s; wording roll %.3f -> \"%s\"\n"), Slot + 1, Dealt.FamilyRoll,
			Dealt.FamilyRank + 1, *FamilyNames[Evaluation.Families[Dealt.FamilyRank].Family], Dealt.CandidateIndex + 1, Dealt.Candidates.Num(),
			Dealt.CandidateIndex > 0 ? *FString::Printf(TEXT(", over #%d"), Dealt.Candidates[0] + 1) : TEXT(""), Dealt.PhrasingRoll, *Cards[Dealt.Phrasing].Phrase);
	}
	for (int32 Rank = 0; Rank < Evaluation.Families.Num(); ++Rank)
	{
		const NarrativeChoice::FFamilyScore& Family = Evaluation.Families[Rank];
		const NarrativeChoice::FPhrasingTerms& Lead = Evaluation.Phrasings[Family.Phrasings[0]];
		Out += FString::Printf(TEXT("  #%-2d P %.4f %-13s %s x%d: u %+.2f = res %+.2f rel %+.2f stance %+.2f, K %.2f\n"), Rank + 1, Family.Probability,
			NarrativeChoice::LexToString(Family.Eligibility), *FamilyNames[Family.Family], Family.Phrasings.Num(), Lead.Utility, Lead.Resonance, Lead.Relevance,
			Lead.StanceTerm, Lead.SpeakerReach);
	}
	return Out;
}

FNarrativeChoiceTraceLog& FNarrativeChoiceTraceLog::Get()
{
	static FNarrativeChoiceTraceLog Log;
	return Log;
}

int32 FNarrativeChoiceTraceLog::GetCapacity() const
{
	return FMath::Max(1, CVarChoiceTraceCapacity.GetValueOnGameThread());
}

void FNarrativeChoiceTraceLog::Record(FNarrativeChoiceTrace&& Trace)
{
	check(IsInGameThread());
	if (bPaused)
	{
		return;
	}

	Trace.Sequence = NextSequence++;
	Ring.Add(MoveTemp(Trace));
	const int32 Excess = Ring.Num() - GetCapacity();
	if (Excess > 0)
	{
		Ring.RemoveAt(0, Excess, EAllowShrinking::No);
	}
}

const FNarrativeChoiceTrace& FNarrativeChoiceTraceLog::GetOrdered(int32 Index) const
{
	return Ring[Index];
}

const FNarrativeChoiceTrace* FNarrativeChoiceTraceLog::FindBySequence(uint64 Sequence) const
{
	return Ring.FindByPredicate([Sequence](const FNarrativeChoiceTrace& Trace) { return Trace.Sequence == Sequence; });
}

void FNarrativeChoiceTraceLog::Clear()
{
	Ring.Reset();
}
