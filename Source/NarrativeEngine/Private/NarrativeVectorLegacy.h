#pragma once

#include "CoreMinimal.h"
#include "NarrativeVectorLegacy.generated.h"

class UNarrativeBasisVector;

/** Supplies Unreal's real map property serializer for old assets, including map deltas.
 * Never stored as part of FVectorND. Keep until all legacy packages have been resaved.
 */
USTRUCT()
struct FNarrativeVectorLegacy
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> Components;
};
