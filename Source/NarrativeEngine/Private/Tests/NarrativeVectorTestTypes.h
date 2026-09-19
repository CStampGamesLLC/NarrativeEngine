#pragma once

#include "NarrativeVector.h"
#include "NarrativeVectorTestTypes.generated.h"

/** Same property name as FNarrativeVectorLegacy, different type, for tagged-load coverage. */
USTRUCT()
struct FNarrativeVectorTestContainer
{
	GENERATED_BODY()

	UPROPERTY()
	FVectorND Components;
};
