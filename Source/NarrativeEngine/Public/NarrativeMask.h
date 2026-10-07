#pragma once

#include "CoreMinimal.h"
#include "NarrativeVector.h"
#include "NarrativeMask.generated.h"

class UNarrativeBasisVector;

/**
 * The axes a card or character is about (spec §2). Everything outside the mask is irrelevant, not
 * neutral: a typed 0 inside it means "deliberately neutral", a blank means "not about this".
 *
 * The mask is its own authored field and is never inferred from how a vector happens to be stored.
 * Axes are held by identity, like FVectorND's basis, so reordering or adding basis assets never
 * reinterprets a mask; compiled bit order arrives with the versioned schema in P5.
 */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FNarrativeMask
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mask")
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> Axes;

	bool IsEmpty() const { return Axes.IsEmpty(); }
	int32 Num() const { return Axes.Num(); }
	bool Contains(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const { return Axes.Contains(Axis); }
	void Add(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) { Axes.AddUnique(Axis); }

	/** The axes both masks are about (spec §2: cos_{MM'} is taken on M ⊙ M'). */
	FNarrativeMask Intersect(const FNarrativeMask& Other) const;

	/** The migration's seed: every axis the author typed into the vector, zeros included. */
	static FNarrativeMask FromTypedAxes(const FVectorND& Vector);

	/** Coordinates of Vector on this mask's axes, in mask order. Missing axes read 0. */
	void Gather(const FVectorND& Vector, TArray<double>& Out) const;
};
