#pragma once

#include "CoreMinimal.h"
#include "NarrativeLints.h"

class UNarrativeBasisVector;
class UNarrativeDialogDef;

/**
 * The card editor's rules, kept apart from Slate so tests can drive them. A card's Location and Mask stay
 * separate fields (the runtime reads the authored mask, spec §2), but the editor gives them one front end:
 * an axis is either about the card, with a coordinate (0 meaning deliberately neutral), or absent from both.
 */
namespace NarrativeDialogEditing
{
	enum class EAxisState : uint8
	{
		/** Neither typed nor masked. */
		Absent,
		/** Masked with a coordinate: the card is about this axis. */
		About,
		/** Typed but outside the mask, so the model ignores the coordinate (lint L1). */
		Ignored,
		/** Masked with no coordinate, so the model reads 0 without the author saying so. */
		Blank,
	};

	EAxisState GetAxisState(const UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis);

	/** Typed coordinate, unset when the axis is untyped. */
	TOptional<float> GetCoordinate(const UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis);

	/** On masks the axis and keeps its coordinate (0 if it had none); off removes it from both fields. */
	void SetAbout(UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis, bool bAbout);

	/** Typing a coordinate makes the card about that axis. */
	void SetCoordinate(UNarrativeDialogDef& Card, const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value);

	/** Drops ignored coordinates and types blank axes at 0, so Location and Mask name the same axes. */
	void Tidy(UNarrativeDialogDef& Card);

	/** Any axis Ignored or Blank. */
	bool NeedsTidy(const UNarrativeDialogDef& Card);

	/** Every basis asset in the registry plus any axis the card names, sorted by display name. */
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> GatherAxes(const UNarrativeDialogDef& Card);
	FText AxisLabel(const TSoftObjectPtr<UNarrativeBasisVector>& Axis);

	/** Strength cap at the card's reach; unset for Legendary, which the lint exempts. */
	TOptional<float> StrengthCap(float Sigma);

	FLinearColor TierColor(ENarrativeTier Tier);
	FText TierName(ENarrativeTier Tier);

	/** The tiers from narrowest reach to widest, the order the reach bar draws them. */
	TConstArrayView<ENarrativeTier> TiersByReach();
}
