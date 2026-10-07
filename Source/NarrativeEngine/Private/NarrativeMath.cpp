#include "NarrativeMath.h"

namespace NarrativeMath
{
	namespace
	{
		double MaskAt(TConstArrayView<double> Mask, int32 Index)
		{
			return Mask.IsEmpty() ? 1.0 : Mask[Index];
		}
	}

	double MaskedSqDist(TConstArrayView<double> X, TConstArrayView<double> Loc, TConstArrayView<double> Mask)
	{
		if (!ensureMsgf(X.Num() == Loc.Num() && (Mask.IsEmpty() || Mask.Num() == X.Num()), TEXT("MaskedSqDist: misaligned arrays.")))
		{
			return 0.0;
		}
		double Sum = 0.0;
		for (int32 i = 0; i < X.Num(); ++i)
		{
			const double D = X[i] - Loc[i];
			Sum += MaskAt(Mask, i) * D * D;
		}
		return Sum;
	}

	double MaskedCosine(TConstArrayView<double> A, TConstArrayView<double> B, TConstArrayView<double> Mask)
	{
		if (!ensureMsgf(A.Num() == B.Num() && (Mask.IsEmpty() || Mask.Num() == A.Num()), TEXT("MaskedCosine: misaligned arrays.")))
		{
			return 0.0;
		}
		double Dot = 0.0, NormA = 0.0, NormB = 0.0;
		for (int32 i = 0; i < A.Num(); ++i)
		{
			const double M = MaskAt(Mask, i);
			const double Ai = M * A[i];
			const double Bi = M * B[i];
			Dot += Ai * Bi;
			NormA += Ai * Ai;
			NormB += Bi * Bi;
		}
		// The reference floors the denominator rather than branching, so a vanishing side reads 0.
		return Dot / FMath::Max(FMath::Sqrt(NormA) * FMath::Sqrt(NormB), 1e-12);
	}

	void OuStep(TArrayView<double> Z, double Dt, double T, TConstArrayView<double> Xi)
	{
		if (!ensureMsgf(Z.Num() == Xi.Num() && T > 0.0, TEXT("OuStep: misaligned arrays or non-positive memory.")))
		{
			return;
		}
		const double Decay = FMath::Exp(-Dt / T);
		const double Kick = FMath::Sqrt(FMath::Max(0.0, 1.0 - Decay * Decay));
		for (int32 i = 0; i < Z.Num(); ++i)
		{
			Z[i] = Decay * Z[i] + Kick * Xi[i];
		}
	}

	int32 FSchmitt::Update(int32 State, double A) const
	{
		if (A < HostileThreshold)
		{
			return -1;
		}
		if (A > FriendlyThreshold)
		{
			return 1;
		}
		if (State == -1 && A < HostileThreshold + Hysteresis)
		{
			return -1;
		}
		if (State == 1 && A > FriendlyThreshold - Hysteresis)
		{
			return 1;
		}
		return 0;
	}

	double Approval(TConstArrayView<double> ObserverX, TConstArrayView<double> ObserverMask,
		TConstArrayView<double> DeedLoc, TConstArrayView<double> DeedMask)
	{
		if (!ensureMsgf(ObserverX.Num() == DeedLoc.Num() && ObserverMask.Num() == ObserverX.Num() && DeedMask.Num() == ObserverX.Num(), TEXT("Approval: misaligned arrays.")))
		{
			return 0.0;
		}
		TArray<double, TInlineAllocator<16>> Shared;
		Shared.SetNumUninitialized(ObserverX.Num());
		bool bAnyShared = false;
		for (int32 i = 0; i < ObserverX.Num(); ++i)
		{
			Shared[i] = ObserverMask[i] * DeedMask[i];
			bAnyShared |= Shared[i] != 0.0;
		}
		return bAnyShared ? MaskedCosine(ObserverX, DeedLoc, Shared) : 0.0;
	}

	double RegardDelta(double Strength, double Stance, double Care, double ApprovalCos, double TargetMass, double Omega)
	{
		return Omega * TargetMass * Strength * (0.6 * Stance * Care + 0.4 * ApprovalCos);
	}

	double DecayRegard(double R, double Dt, double TR)
	{
		return TR > 0.0 ? R * FMath::Exp(-Dt / TR) : 0.0;
	}

	double ClipRegard(double R, double Loyalty)
	{
		const double Cap = FMath::Max(0.0, Loyalty);
		return FMath::Clamp(R, -Cap, Cap);
	}

	void FirstImpressionMean(TConstArrayView<double> SelfX, const TConstArrayView<double>* ArchetypeLoc,
		double SelfWeight, TArrayView<double> OutMean)
	{
		if (!ensureMsgf(OutMean.Num() == SelfX.Num() && (!ArchetypeLoc || ArchetypeLoc->Num() == SelfX.Num()), TEXT("FirstImpressionMean: misaligned arrays.")))
		{
			// Neutral rather than whatever the caller's buffer held.
			for (double& Out : OutMean)
			{
				Out = 0.0;
			}
			return;
		}
		for (int32 i = 0; i < SelfX.Num(); ++i)
		{
			const double Stereotype = ArchetypeLoc ? (*ArchetypeLoc)[i] : 0.0;
			OutMean[i] = (1.0 - SelfWeight) * Stereotype + SelfWeight * SelfX[i];
		}
	}

	double CombineReading(double PositionReading, double Regard, double PositionWeight, double RegardWeight, double Bias)
	{
		const double Denominator = FMath::Abs(PositionWeight) + FMath::Abs(RegardWeight);
		const double Raw = Denominator > 0.0
			? (PositionWeight * PositionReading + RegardWeight * FMath::Tanh(Regard)) / Denominator + Bias
			: Bias;
		return FMath::Clamp(Raw, -1.0, 1.0);
	}
}
