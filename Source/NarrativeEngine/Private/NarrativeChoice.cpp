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
		case EFamilyEligibility::Illegal: return TEXT("illegal");
		default: return TEXT("eligible");
		}
	}

	const TCHAR* LexToString(EDealSlotKind Kind)
	{
		switch (Kind)
		{
		case EDealSlotKind::Reply: return TEXT("reply");
		case EDealSlotKind::Verb: return TEXT("verb");
		case EDealSlotKind::ReservedWarm: return TEXT("reserved warm");
		case EDealSlotKind::ReservedProvoke: return TEXT("reserved provoke");
		default: return TEXT("draw");
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

	FEvaluation Evaluate(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity)
	{
		FEvaluation Out;
		Out.Affinity = Affinity;
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

			Terms.StanceTerm = Params.Lambda * In.Stance * Affinity;
			Terms.Utility = Terms.Resonance + Terms.Relevance + Terms.StanceTerm;
			Logits.Add(Beta * Terms.Utility);
		}

		// Families: w̄_g is the mean Gibbs weight of the legal phrasings in the speaker's reach (MD8), so an idea's
		// weight never grows by rewording it, and a wording the speaker can't reach or may not say never dilutes the
		// ones it can. A family with nothing legal in reach is scored on all its legal phrasings, for the record; it
		// is never dealt by Choice. A family with no legal phrasing is scored on all of them for the record only, and
		// stays out of the partition: what can't be said takes no probability from what can (Pattern 8).
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

			TArray<int32> Legal;
			for (const int32 Member : Family.Phrasings)
			{
				if (Phrasings[Member].IsLegal())
				{
					Legal.Add(Member);
				}
				else if (Family.IllegalCheck == 0 || Phrasings[Member].IllegalCheck < Family.IllegalCheck)
				{
					Family.IllegalCheck = Phrasings[Member].IllegalCheck;
				}
			}
			Family.LegalPhrasings = Legal.Num();
			const bool bIllegal = Legal.IsEmpty();
			if (bIllegal)
			{
				Family.Eligibility = EFamilyEligibility::Illegal;
				Legal = Family.Phrasings;
			}
			else
			{
				Family.IllegalCheck = 0;
			}

			TArray<int32> Scored;
			for (const int32 Member : Legal)
			{
				Family.MaxSpeakerReach = FMath::Max(Family.MaxSpeakerReach, Out.Phrasings[Member].SpeakerReach);
				if (Out.Phrasings[Member].SpeakerReach >= Params.ReachFloor)
				{
					Scored.Add(Member);
				}
			}
			if (Scored.IsEmpty())
			{
				Scored = Legal;
			}

			TArray<double> FamilyLogits;
			for (const int32 Member : Scored)
			{
				FamilyLogits.Add(Logits[Member]);
			}
			const double LogSum = LogSumExp(FamilyLogits);
			const double LogSize = FMath::Loge(static_cast<double>(Scored.Num()));
			Family.LogMeanWeight = LogSum - LogSize;
			if (bIllegal)
			{
				// For the record: its terms are kept, but it takes no share and nothing of it is ever rolled.
				continue;
			}
			LogWeights.Add(Family.LogMeanWeight);

			// A wording outside the scored set pays no share and is never rolled.
			for (const int32 Member : Scored)
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
			if (Family.Eligibility == EFamilyEligibility::Illegal)
			{
				Family.Probability = 0.0;
				continue;
			}
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
			Dealt.Kind = EDealSlotKind::Draw;
			Dealt.LaneSlot = Slot;
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
		return Family.Eligibility != EFamilyEligibility::OutOfReach && Family.Eligibility != EFamilyEligibility::Illegal;
	}

	FDealSlot Reply(const FEvaluation& Evaluation, TFunctionRef<double(uint64 Lane)> Uniform)
	{
		FDealSlot Said;
		Said.Kind = EDealSlotKind::Reply;
		Said.LaneSlot = 0;
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

	TArray<double> MaskedDirection(TConstArrayView<int32> AxisSlots, TConstArrayView<double> Location, TConstArrayView<double> Belief, int32 NumAxes)
	{
		TArray<double> Out;
		Out.Init(0.0, FMath::Max(0, NumAxes));
		const int32 Count = FMath::Min3(AxisSlots.Num(), Location.Num(), Belief.Num());
		for (int32 Axis = 0; Axis < Count; ++Axis)
		{
			if (Out.IsValidIndex(AxisSlots[Axis]))
			{
				Out[AxisSlots[Axis]] = Location[Axis] - Belief[Axis];
			}
		}
		return Out;
	}

	double Repulsion(TConstArrayView<double> Direction, TConstArrayView<TArray<double>> Hand)
	{
		double Norm = 0.0;
		for (const double Value : Direction)
		{
			Norm += Value * Value;
		}
		if (Norm <= UE_DOUBLE_SMALL_NUMBER)
		{
			return 1.0;
		}

		double Factor = 1.0;
		for (const TArray<double>& Other : Hand)
		{
			if (Other.Num() != Direction.Num())
			{
				continue;
			}
			double Dot = 0.0;
			double OtherNorm = 0.0;
			for (int32 Axis = 0; Axis < Direction.Num(); ++Axis)
			{
				Dot += Direction[Axis] * Other[Axis];
				OtherNorm += Other[Axis] * Other[Axis];
			}
			if (OtherNorm <= UE_DOUBLE_SMALL_NUMBER)
			{
				continue;
			}
			const double Cosine = Dot / FMath::Sqrt(Norm * OtherNorm);
			Factor *= 1.0 - FMath::Square(FMath::Max(0.0, Cosine));
		}
		return Factor;
	}

	namespace
	{
		/* The family's favourite legal wording: its phrasings run by P(y | g), and illegal ones hold 0 there. */
		int32 LeadPhrasing(TConstArrayView<FPhrasing> Phrasings, const FFamilyScore& Family, int32& OutIndex)
		{
			for (int32 Index = 0; Index < Family.Phrasings.Num(); ++Index)
			{
				if (Phrasings[Family.Phrasings[Index]].IsLegal())
				{
					OutIndex = Index;
					return Family.Phrasings[Index];
				}
			}
			OutIndex = 0;
			return Family.Phrasings.IsEmpty() ? INDEX_NONE : Family.Phrasings[0];
		}
	}

	FOfferResult Offer(const FParams& Params, const FOfferSheet& Sheet, const FEvaluation& Evaluation, TConstArrayView<FPhrasing> Phrasings,
		const TMap<int32, FOfferFamily>& Families, TFunctionRef<double(uint64 Lane)> Uniform)
	{
		FOfferResult Out;
		const double Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);
		const double Tau = Sheet.HandBeta / Beta;
		const double LogNu = FMath::Loge(FMath::Max(Sheet.NoveltyDiscount, 1e-12));
		Out.NoveltyScores.Init(-TNumericLimits<double>::Max(), Evaluation.Families.Num());

		auto Meta = [&Evaluation, &Families](int32 Rank) -> const FOfferFamily*
		{
			return Families.Find(Evaluation.Families[Rank].Family);
		};
		static const TArray<double> NoDirection;
		auto DirectionOf = [&Phrasings](int32 Phrasing) -> const TArray<double>&
		{
			return Phrasings.IsValidIndex(Phrasing) ? Phrasings[Phrasing].Direction : NoDirection;
		};

		// 1. Score: every family with a legal phrasing is in the pool, r_g = ln w̄_g at β.
		TArray<int32> Pool;
		for (int32 Rank = 0; Rank < Evaluation.Families.Num(); ++Rank)
		{
			if (Evaluation.Families[Rank].Eligibility != EFamilyEligibility::Illegal)
			{
				Pool.Add(Rank);
			}
		}

		TArray<TArray<double>> HandDirections;
		TArray<FDealSlot> Verbs;

		// 2. Verbs: each legal mechanic family, shown with its favourite wording. No lanes; they leave the pool (D7).
		if (Sheet.bVerbs)
		{
			for (int32 Index = 0; Index < Pool.Num();)
			{
				const int32 Rank = Pool[Index];
				const FOfferFamily* Family = Meta(Rank);
				if (!Family || !Family->bVerb)
				{
					++Index;
					continue;
				}
				FDealSlot& Verb = Verbs.AddDefaulted_GetRef();
				Verb.Kind = EDealSlotKind::Verb;
				Verb.FamilyRank = Rank;
				Verb.Phrasing = LeadPhrasing(Phrasings, Evaluation.Families[Rank], Verb.PhrasingIndex);
				HandDirections.Add(DirectionOf(Verb.Phrasing));
				Pool.RemoveAt(Index);
			}
		}

		// 3. Novelty: r'_g = r_g + n_g ln ν, for reserved slots and draws alike. With ν = 1 it adds an exact 0.
		for (const int32 Rank : Pool)
		{
			const FOfferFamily* Family = Meta(Rank);
			const int32 Recent = Family ? FMath::Clamp(Family->Novelty, 0, FMath::Max(0, Sheet.NoveltyWindow)) : 0;
			Out.NoveltyScores[Rank] = Evaluation.Families[Rank].LogMeanWeight + (Recent > 0 ? Recent * LogNu : 0.0);
		}

		TArray<FDealSlot> Cards;

		// 4. Reserved slots: the best legal family of each sign by r'_g, ties by family id. No roll picks the family.
		auto Reserve = [&](int32 Sign, EDealSlotKind Kind, int32 LaneSlot)
		{
			int32 BestIndex = INDEX_NONE;
			for (int32 Index = 0; Index < Pool.Num(); ++Index)
			{
				const FOfferFamily* Family = Meta(Pool[Index]);
				if (!Family || Family->Sign != Sign)
				{
					continue;
				}
				if (BestIndex == INDEX_NONE)
				{
					BestIndex = Index;
					continue;
				}
				const double Score = Out.NoveltyScores[Pool[Index]];
				const double Best = Out.NoveltyScores[Pool[BestIndex]];
				if (Score > Best || (Score == Best && Family->Id < Meta(Pool[BestIndex])->Id))
				{
					BestIndex = Index;
				}
			}
			if (BestIndex == INDEX_NONE)
			{
				return;
			}

			const int32 Rank = Pool[BestIndex];
			const FFamilyScore& Family = Evaluation.Families[Rank];
			FDealSlot& Slot = Cards.AddDefaulted_GetRef();
			Slot.Kind = Kind;
			Slot.LaneSlot = LaneSlot;
			Slot.FamilyRank = Rank;
			Slot.Candidates = {Rank};
			Slot.CandidateWeights = {1.0};
			Slot.CandidateIndex = 0;
			if (Family.MaxSpeakerReach >= Params.ReachFloor)
			{
				PickPhrasing(Evaluation, Slot, Uniform(2 * static_cast<uint64>(LaneSlot) + 1));
			}
			else
			{
				// Nothing legal in reach: the highest-utility legal wording, which P(y | g) over all legal ones ranks first.
				Slot.Phrasing = LeadPhrasing(Phrasings, Family, Slot.PhrasingIndex);
			}
			HandDirections.Add(DirectionOf(Slot.Phrasing));
			Pool.RemoveAt(BestIndex);
		};
		if (Sheet.bReservedSlots)
		{
			Reserve(+1, EDealSlotKind::ReservedWarm, 0);
			Reserve(-1, EDealSlotKind::ReservedProvoke, 1);
		}
		const int32 Reserved = Cards.Num();

		// 5. Draws: eligible families at β, weighted exp(τ r'_g) ρ_g over the remaining candidates; Silence takes no share.
		TArray<int32> Candidates = Pool.FilterByPredicate([&Evaluation](int32 Rank)
		{
			return Evaluation.Families[Rank].Eligibility == EFamilyEligibility::Eligible;
		});
		const int32 FirstDraw = Sheet.bReservedSlots ? 2 : 0;
		for (int32 Draw = 0; Reserved + Draw < Sheet.HandSize && !Candidates.IsEmpty(); ++Draw)
		{
			const int32 LaneSlot = FirstDraw + Draw;
			FDealSlot Slot;
			Slot.Kind = EDealSlotKind::Draw;
			Slot.LaneSlot = LaneSlot;
			Slot.Candidates = Candidates;
			for (const int32 Rank : Candidates)
			{
				// Relative to the partition, so the reduction case weighs exactly P(g), as Deal does.
				double Weight = FMath::Exp(Tau * (Out.NoveltyScores[Rank] - Evaluation.LogPartition));
				if (Sheet.bRepulsion)
				{
					int32 Unused = 0;
					Weight *= Repulsion(DirectionOf(LeadPhrasing(Phrasings, Evaluation.Families[Rank], Unused)), HandDirections);
				}
				Slot.CandidateWeights.Add(Weight);
			}
			Slot.FamilyRoll = Uniform(2 * static_cast<uint64>(LaneSlot));
			Slot.CandidateIndex = PickByRoll(Slot.CandidateWeights, Slot.FamilyRoll);
			if (Slot.CandidateIndex == INDEX_NONE)
			{
				break;
			}
			Slot.FamilyRank = Candidates[Slot.CandidateIndex];
			PickPhrasing(Evaluation, Slot, Uniform(2 * static_cast<uint64>(LaneSlot) + 1));
			HandDirections.Add(DirectionOf(Slot.Phrasing));
			Candidates.RemoveAt(Slot.CandidateIndex);
			Pool.Remove(Slot.FamilyRank);
			Cards.Add(MoveTemp(Slot));
		}

		// A short hand names what kept the four from filling (Pattern 10, H06).
		if (Cards.Num() < Sheet.HandSize)
		{
			int32 Repertoire = 0;
			int32 Legality = 0;
			int32 Reach = 0;
			int32 BelowSilence = 0;
			for (const FFamilyScore& Family : Evaluation.Families)
			{
				const FOfferFamily* FamilyMeta = Families.Find(Family.Family);
				if (Sheet.bVerbs && FamilyMeta && FamilyMeta->bVerb)
				{
					continue;
				}
				switch (Family.Eligibility)
				{
				case EFamilyEligibility::Illegal: (Family.IllegalCheck == RepertoireCheck ? Repertoire : Legality)++; break;
				case EFamilyEligibility::OutOfReach: ++Reach; break;
				case EFamilyEligibility::BelowSilence: ++BelowSilence; break;
				default: break;
				}
			}
			const TPair<const TCHAR*, int32> Causes[] = {{TEXT("legality"), Legality}, {TEXT("reach"), Reach}, {TEXT("below silence"), BelowSilence}, {TEXT("repertoire"), Repertoire}};
			const TPair<const TCHAR*, int32>* Worst = nullptr;
			for (const TPair<const TCHAR*, int32>& Cause : Causes)
			{
				if (Cause.Value > 0 && (!Worst || Cause.Value > Worst->Value))
				{
					Worst = &Cause;
				}
			}
			Out.ShortCause = Worst ? Worst->Key : TEXT("pool exhausted");
		}

		Out.Slots = MoveTemp(Verbs);
		Out.Slots.Append(MoveTemp(Cards));
		return Out;
	}

	FSubjectPick PickSubject(const FParams& Params, TConstArrayView<FPhrasing> Candidates, double U)
	{
		FSubjectPick Out;
		Out.Roll = U;
		if (Candidates.IsEmpty())
		{
			return Out;
		}

		// Each subject is its own idea with no stance, so u is resonance + relevance alone.
		TArray<FPhrasing> Scored(Candidates.GetData(), Candidates.Num());
		for (int32 Index = 0; Index < Scored.Num(); ++Index)
		{
			Scored[Index].Family = Index;
			Scored[Index].Stance = 0.0;
		}
		Out.Evaluation = Evaluate(Params, Scored, 0.0);

		const double Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);
		double MaxLogit = -TNumericLimits<double>::Max();
		for (const FPhrasingTerms& Terms : Out.Evaluation.Phrasings)
		{
			Out.bInReach |= Terms.SpeakerReach >= Params.ReachFloor;
		}
		for (const FPhrasingTerms& Terms : Out.Evaluation.Phrasings)
		{
			if (!Out.bInReach || Terms.SpeakerReach >= Params.ReachFloor)
			{
				MaxLogit = FMath::Max(MaxLogit, Beta * Terms.Utility);
			}
		}

		// Relative to the best, so a far-off field of candidates doesn't underflow to nothing.
		Out.Weights.Reserve(Scored.Num());
		for (const FPhrasingTerms& Terms : Out.Evaluation.Phrasings)
		{
			const bool bCounted = !Out.bInReach || Terms.SpeakerReach >= Params.ReachFloor;
			Out.Weights.Add(bCounted ? FMath::Exp(Beta * Terms.Utility - MaxLogit) : 0.0);
		}
		Out.Index = PickByRoll(Out.Weights, U);
		return Out;
	}
}
