#pragma once

#include "CoreMinimal.h"
#include "NarrativeIds.h"

/** Why a draw is made. Part of every key, so two uses of one relationship never share numbers. */
enum class ENarrativeRngPurpose : uint8
{
	StanceNoise = 1,
	Choice = 2,
	Witness = 3,
	Opportunity = 4,
	Sample = 5,
};

/**
 * Keyed chance (spec §9.3, TDD Pattern 4). Every narrative draw is a pure function of
 * (campaign seed, observer, subject, purpose, semantic index, lane): a counter-based hash, never a
 * shared stream. A's stance noise toward B then depends only on that relationship and its
 * decision number, so scheduling, cameras and inspectors can't change what anyone draws.
 *
 * Version 1, mirrored by Docs/NarrativeHeuristics/narrative_rng.py with golden vectors:
 *  - Mix(x) is SplitMix64's step: x += 0x9E3779B97F4A7C15, then the two xor-shift-multiplies and a
 *    final xor-shift, all modulo 2^64.
 *  - An id's key is Mix(High ^ Mix(Low)), where High = A<<32|B and Low = C<<32|D of its GUID. A
 *    missing subject keys as 0.
 *  - Bits = Mix(Mix(Mix(Mix(Mix(Mix(Seed) ^ ObserverKey) ^ SubjectKey) ^ Purpose) ^ Index) + Lane).
 *  - Uniform = ((Bits >> 11) + 0.5) / 2^53, on the open interval (0, 1).
 *  - Normal(Lane pair k) = sqrt(-2 ln U(2k)) cos(2 pi U(2k+1)): Box-Muller's cosine half only.
 */
namespace NarrativeRng
{
	constexpr int32 Version = 1;

	NARRATIVEENGINE_API uint64 Mix(uint64 X);
	NARRATIVEENGINE_API uint64 KeyOf(const FNarrativeEntityId& Id);

	/**
	 * An axis's lane key: FNV-1a 64 over the UTF-8 bytes of its lower-cased object path. Noise is drawn
	 * per axis identity, so reordering a mask or a schema permutes nothing (M01).
	 */
	NARRATIVEENGINE_API uint64 AxisKey(const FSoftObjectPath& AxisPath);
	NARRATIVEENGINE_API uint64 Bits(uint64 Seed, uint64 ObserverKey, uint64 SubjectKey, ENarrativeRngPurpose Purpose, uint64 Index, uint64 Lane);
	NARRATIVEENGINE_API double UniformFromBits(uint64 InBits);

	/** A keyed draw site: one (seed, observer, subject, purpose, index). Lanes number the draws within it. */
	struct NARRATIVEENGINE_API FKey
	{
		uint64 Seed = 0;
		uint64 ObserverKey = 0;
		uint64 SubjectKey = 0;
		ENarrativeRngPurpose Purpose = ENarrativeRngPurpose::StanceNoise;
		uint64 Index = 0;

		FKey() = default;
		FKey(uint64 InSeed, const FNarrativeEntityId& Observer, const FNarrativeEntityId& Subject, ENarrativeRngPurpose InPurpose, uint64 InIndex)
			: Seed(InSeed), ObserverKey(KeyOf(Observer)), SubjectKey(Subject.IsValid() ? KeyOf(Subject) : 0), Purpose(InPurpose), Index(InIndex) {}

		double Uniform(uint64 Lane) const { return UniformFromBits(Bits(Seed, ObserverKey, SubjectKey, Purpose, Index, Lane)); }

		/** The k-th standard normal at this site, from lanes 2k and 2k+1. */
		double Normal(uint64 K) const;
	};
}
