#pragma once

#include "CoreMinimal.h"

/**
 * The model's arithmetic, ported from the Python reference (Docs/NarrativeHeuristics/core.py) on
 * demand, one increment at a time (TDD Pattern 5). Pure functions over plain arrays: no assets, no
 * world, no randomness of their own. Every function here has a parity test against
 * Docs/NarrativeHeuristics/p1_parity.json or a calibrated character's numbers; when the spec
 * changes, regenerate the JSON and the failing tests list what to update.
 *
 * P1 set: masked distance and cosine, the exact OU step, the Schmitt trigger, the regard rule (§8.5,
 * with mass and Loyalty), first impressions (§4.2) and the two-reading alignment (§6.1).
 */
namespace NarrativeMath
{
	/** ||X - Loc||²_M. An empty Mask means every axis counts (core.masked_sqdist with mask=None). */
	NARRATIVEENGINE_API double MaskedSqDist(TConstArrayView<double> X, TConstArrayView<double> Loc, TConstArrayView<double> Mask = {});

	/** cos(M a, M b); 0 when either masked vector vanishes (core.cosine). An empty Mask means every axis. */
	NARRATIVEENGINE_API double MaskedCosine(TConstArrayView<double> A, TConstArrayView<double> B, TConstArrayView<double> Mask = {});

	/**
	 * Exact Ornstein-Uhlenbeck transition in place: z <- a z + sqrt(1 - a²) xi with a = exp(-Dt / T).
	 * The marginal stays N(0, I) for any Dt, so the stance distribution does not depend on how often
	 * it is evaluated (spec §6.2). Xi supplies one standard normal per coordinate.
	 */
	NARRATIVEENGINE_API void OuStep(TArrayView<double> Z, double Dt, double T, TConstArrayView<double> Xi);

	/** Three-state trigger with hysteresis (core.Schmitt). States are -1 Hostile, 0 None, +1 Friendly. */
	struct NARRATIVEENGINE_API FSchmitt
	{
		double HostileThreshold = -0.25;
		double FriendlyThreshold = 0.35;
		double Hysteresis = 0.1;

		/** The bands must not overlap: θH + δ < θF - δ. */
		bool IsValid() const { return HostileThreshold + Hysteresis < FriendlyThreshold - Hysteresis; }

		/** The next state from State given reading A. Direct Friendly <-> Hostile jumps are allowed. */
		int32 Update(int32 State, double A) const;
	};

	/**
	 * Observer O's approval of a deed's meaning: cos over the axes both O and the deed are about, of
	 * O's own position and the deed's location. 0 when the masks share no axis. Arrays are aligned
	 * over one basis; masks are 0/1 per axis.
	 */
	NARRATIVEENGINE_API double Approval(TConstArrayView<double> ObserverX, TConstArrayView<double> ObserverMask,
		TConstArrayView<double> DeedLoc, TConstArrayView<double> DeedMask);

	/**
	 * ΔR = ω m_T J (0.6 h c + 0.4 approval) (spec §8.5 with the TDD's departure, Pattern 16): the deed's
	 * stance h signs only the victim term, so approval of its meaning is unsigned and a hostile act in
	 * a cause the witness rejects costs more, never less. Care is 1 when O is the deed's target, else
	 * clip(Ã_O→T); TargetMass is the target's mass frozen at occurrence (1 when there is no target).
	 */
	NARRATIVEENGINE_API double RegardDelta(double Strength, double Stance, double Care, double ApprovalCos, double TargetMass = 1.0, double Omega = 1.0);

	/** Regard fades with memory TR: R e^(-Dt / TR). */
	NARRATIVEENGINE_API double DecayRegard(double R, double Dt, double TR);

	/** Stored regard is capped by the witness's Loyalty: clip(R, -L, L). */
	NARRATIVEENGINE_API double ClipRegard(double R, double Loyalty);

	/** μ0 = (1 - π) ℓ_arch + π x_A; without an archetype the stereotype is neutral (spec §4.2). */
	NARRATIVEENGINE_API void FirstImpressionMean(TConstArrayView<double> SelfX, const TConstArrayView<double>* ArchetypeLoc,
		double SelfWeight, TArrayView<double> OutMean);

	/**
	 * The P1 alignment reading, D = {pos, reg} (spec §6.1; TDD Pattern 18):
	 * clamp((w_pos a_pos + w_reg tanh R) / (|w_pos| + |w_reg|) + b, -1, 1). An undefined masked cosine
	 * reads 0 and keeps its weight (calibrated fixtures depend on the fixed denominator); all-zero
	 * weights read the clamped bias alone.
	 */
	NARRATIVEENGINE_API double CombineReading(double PositionReading, double Regard, double PositionWeight, double RegardWeight, double Bias);
}
