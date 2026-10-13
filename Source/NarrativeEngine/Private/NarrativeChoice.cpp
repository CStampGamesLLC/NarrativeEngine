#include "NarrativeChoice.h"

#include "NarrativeMath.h"

namespace NarrativeChoice
{
	namespace
	{
		/* The smallest reach σ the rule divides by: an authored 0 means "this exact spot", not a crash. */
		constexpr double MinSigma = 1e-3;

		double LogSumExp(TConstArrayView<double> Values)
		{
			double Max = -TNumericLimits<double>::Max();
			for (const double Value : Values)
			{
				Max = FMath::Max(Max, Value);
			}
			if (Values.IsEmpty() || Max == -TNumericLimits<double>::Max())
			{
				return -TNumericLimits<double>::Max();
			}
			double Sum = 0.0;
			for (const double Value : Values)
			{
				Sum += FMath::Exp(Value - Max);
			}
			return Max + FMath::Loge(Sum);
		}

		/* The wording roll, once the slot has its family: by P(y | g), the favourite if every weight underflowed. */
		void PickPhrasing(const FEvaluation& Evaluation, FDealSlot& Slot, double Roll)
		{
			const FFamilyScore& Family = Evaluation.Families[Slot.FamilyRank];
			TArray<double> Wordings;
			for (const int32 Member : Family.Phrasings)
			{
				Wordings.Add(Evaluation.Phrasings[Member].ProbabilityInFamily);
			}
			Slot.PhrasingRoll = Roll;
			Slot.PhrasingIndex = FMath::Max(0, PickByRoll(Wordings, Roll));
			Slot.Phrasing = Family.Phrasings[Slot.PhrasingIndex];
		}
	}

	const TCHAR* LexToString(EFamilyEligibility Eligibility)
	{
		switch (Eligibility)
		{
		case EFamilyEligibility::OutOfReach: return TEXT("out of reach");
		case EFamilyEligibility::BelowSilence: return TEXT("below silence");
		default: return TEXT("eligible");
		}
	}

	double LogExpectedReachPow(TConstArrayView<double> Mean, TConstArrayView<double> Variance, TConstArrayView<double> Location, double Sigma, double Power)
	{
		if (!ensureMsgf(Mean.Num() == Location.Num() && Variance.Num() == Location.Num(), TEXT("LogExpectedReachPow: misaligned arrays.")))
		{
			return 0.0;
		}

		// With Q = a I on the mask and diagonal Σ, det(I + 2QΣ) and the quadratic form factor per axis.
		const double A = Power / FMath::Square(FMath::Max(Sigma, MinSigma));
		double LogDet = 0.0;
		double Quadratic = 0.0;
		for (int32 Index = 0; Index < Location.Num(); ++Index)
		{
			const double Spread = 1.0 + 2.0 * A * FMath::Max(0.0, Variance[Index]);
			const double D = Mean[Index] - Location[Index];
			LogDet += FMath::Loge(Spread);
			Quadratic += A * D * D / Spread;
		}
		return -0.5 * LogDet - Quadratic;
	}

	FEvaluation Evaluate(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Reading)
	{
		FEvaluation Out;
		Out.Reading = Reading;
		const double Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);

		// Utilities, term by term.
		TArray<double> Logits;
		Logits.Reserve(Phrasings.Num());
		Out.Phrasings.SetNum(Phrasings.Num());
		for (int32 Index = 0; Index < Phrasings.Num(); ++Index)
		{
			const FPhrasing& In = Phrasings[Index];
			FPhrasingTerms& Terms = Out.Phrasings[Index];
			const double Sigma2 = FMath::Square(FMath::Max(In.Sigma, MinSigma));

			Terms.SpeakerSqDist = NarrativeMath::MaskedSqDist(In.SpeakerX, In.Location);
			Terms.Resonance = -Terms.SpeakerSqDist / Sigma2;
			Terms.SpeakerReach = FMath::Exp(Terms.Resonance);

			Terms.BeliefSqDist = NarrativeMath::MaskedSqDist(In.BeliefMean, In.Location);
			double VarianceSum = 0.0;
			for (const double Variance : In.BeliefVariance)
			{
				VarianceSum += Variance;
			}
			Terms.BeliefMeanVariance = In.BeliefVariance.IsEmpty() ? 0.0 : VarianceSum / In.BeliefVariance.Num();
			Terms.Relevance = LogExpectedReachPow(In.BeliefMean, In.BeliefVariance, In.Location, In.Sigma, Beta * Params.Eta) / Beta;

			Terms.StanceTerm = Params.Lambda * In.Stance * Reading;
			Terms.Utility = Terms.Resonance + Terms.Relevance + Terms.StanceTerm;
			Logits.Add(Beta * Terms.Utility);
		}

		// Families: w̄_g is the mean Gibbs weight of the phrasings, so an idea's weight never grows by rewording it.
		TMap<int32, TArray<int32>> Members;
		for (int32 Index = 0; Index < Phrasings.Num(); ++Index)
		{
			Members.FindOrAdd(Phrasings[Index].Family).Add(Index);
		}

		TArray<double> LogWeights;
		Out.SilenceLogWeight = Beta * Params.SilenceUtility;
		LogWeights.Add(Out.SilenceLogWeight);
		for (TPair<int32, TArray<int32>>& Pair : Members)
		{
			FFamilyScore& Family = Out.Families.AddDefaulted_GetRef();
			Family.Family = Pair.Key;
			Family.Phrasings = MoveTemp(Pair.Value);

			TArray<double> FamilyLogits;
			for (const int32 Member : Family.Phrasings)
			{
				FamilyLogits.Add(Logits[Member]);
				Family.MaxSpeakerReach = FMath::Max(Family.MaxSpeakerReach, Out.Phrasings[Member].SpeakerReach);
			}
			const double LogSum = LogSumExp(FamilyLogits);
			const double LogSize = FMath::Loge(static_cast<double>(Family.Phrasings.Num()));
			Family.LogMeanWeight = LogSum - LogSize;
			LogWeights.Add(Family.LogMeanWeight);

			for (const int32 Member : Family.Phrasings)
			{
				Out.Phrasings[Member].FamilyLogSize = LogSize;
				Out.Phrasings[Member].ProbabilityInFamily = FMath::Exp(Logits[Member] - LogSum);
			}
			Family.Phrasings.StableSort([&Out](int32 A, int32 B)
			{
				return Out.Phrasings[A].ProbabilityInFamily > Out.Phrasings[B].ProbabilityInFamily;
			});
		}

		Out.LogPartition = LogSumExp(LogWeights);
		Out.SilenceProbability = FMath::Exp(Out.SilenceLogWeight - Out.LogPartition);
		for (FFamilyScore& Family : Out.Families)
		{
			Family.Probability = FMath::Exp(Family.LogMeanWeight - Out.LogPartition);
			for (const int32 Member : Family.Phrasings)
			{
				Out.Phrasings[Member].Probability = Family.Probability * Out.Phrasings[Member].ProbabilityInFamily;
			}

			// Eligible (§5.2): within the speaker's reach, and better than saying nothing.
			if (Family.MaxSpeakerReach < Params.ReachFloor)
			{
				Family.Eligibility = EFamilyEligibility::OutOfReach;
			}
			else if (Family.Probability <= Out.SilenceProbability)
			{
				Family.Eligibility = EFamilyEligibility::BelowSilence;
			}
		}

		Out.Families.Sort([](const FFamilyScore& A, const FFamilyScore& B)
		{
			return A.Probability != B.Probability ? A.Probability > B.Probability : A.Family < B.Family;
		});
		return Out;
	}

	int32 PickByRoll(TConstArrayView<double> Weights, double U)
	{
		double Total = 0.0;
		int32 LastPositive = INDEX_NONE;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			if (Weights[Index] > 0.0)
			{
				Total += Weights[Index];
				LastPositive = Index;
			}
		}
		if (Total <= 0.0)
		{
			return INDEX_NONE;
		}

		const double Target = FMath::Clamp(U, 0.0, 1.0) * Total;
		double Running = 0.0;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			Running += FMath::Max(0.0, Weights[Index]);
			if (Weights[Index] > 0.0 && Running > Target)
			{
				return Index;
			}
		}
		// Rounding left the target past the last running sum: it belongs to the last real weight.
		return LastPositive;
	}

	TArray<FDealSlot> Deal(const FEvaluation& Evaluation, int32 HandSize, TFunctionRef<double(uint64 Lane)> Uniform)
	{
		TArray<int32> Remaining;
		for (int32 Rank = 0; Rank < Evaluation.Families.Num(); ++Rank)
		{
			if (Evaluation.Families[Rank].Eligibility == EFamilyEligibility::Eligible)
			{
				Remaining.Add(Rank);
			}
		}

		TArray<FDealSlot> Hand;
		for (int32 Slot = 0; Slot < HandSize && !Remaining.IsEmpty(); ++Slot)
		{
			FDealSlot& Dealt = Hand.AddDefaulted_GetRef();
			Dealt.Candidates = Remaining;
			for (const int32 Rank : Remaining)
			{
				Dealt.CandidateWeights.Add(Evaluation.Families[Rank].Probability);
			}

			Dealt.FamilyRoll = Uniform(2 * static_cast<uint64>(Slot));
			Dealt.CandidateIndex = PickByRoll(Dealt.CandidateWeights, Dealt.FamilyRoll);
			if (Dealt.CandidateIndex == INDEX_NONE)
			{
				// Every remaining idea underflowed to nothing: a short hand, never a dead card.
				Hand.Pop();
				break;
			}
			Dealt.FamilyRank = Remaining[Dealt.CandidateIndex];
			PickPhrasing(Evaluation, Dealt, Uniform(2 * static_cast<uint64>(Slot) + 1));

			Remaining.RemoveAt(Dealt.CandidateIndex);
		}
		return Hand;
	}

	bool CanReply(const FFamilyScore& Family)
	{
		return Family.Eligibility != EFamilyEligibility::OutOfReach;
	}

	FDealSlot Reply(const FEvaluation& Evaluation, TFunctionRef<double(uint64 Lane)> Uniform)
	{
		FDealSlot Said;
		Said.SilenceWeight = Evaluation.SilenceProbability;
		for (int32 Rank = 0; Rank < Evaluation.Families.Num(); ++Rank)
		{
			if (CanReply(Evaluation.Families[Rank]))
			{
				Said.Candidates.Add(Rank);
				Said.CandidateWeights.Add(Evaluation.Families[Rank].Probability);
			}
		}

		// Silence sits first in the roll, so the inverse CDF is stable however the families rank.
		TArray<double> Outcomes = {Said.SilenceWeight};
		Outcomes.Append(Said.CandidateWeights);
		Said.FamilyRoll = Uniform(0);
		const int32 Outcome = PickByRoll(Outcomes, Said.FamilyRoll);
		if (Outcome <= 0)
		{
			// Silence, or nothing weighed anything: saying nothing is always legal.
			return Said;
		}

		Said.CandidateIndex = Outcome - 1;
		Said.FamilyRank = Said.Candidates[Said.CandidateIndex];
		PickPhrasing(Evaluation, Said, Uniform(1));
		return Said;
	}
}
