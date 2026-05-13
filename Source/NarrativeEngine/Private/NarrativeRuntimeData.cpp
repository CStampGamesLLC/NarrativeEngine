#include "NarrativeRuntimeData.h"

#include "NarrativeEngine.h"
#include "NarrativeSubsystem.h"

FNarrativeEntityInstance::FNarrativeEntityInstance(const UNarrativeEntityDef& EntityDef)
{
	Asset = &EntityDef;
	Name = EntityDef.GetFName();
	Mass = EntityDef.Mass;

	// Use the cached basis-vector list. The previous form did a synchronous
	// TryLoad sweep on every entity registration; cached form is O(B).
	const TArray<TObjectPtr<UNarrativeBasisVector>>& BasisVectors = UNarrativeBasisVector::GetCachedAssets();

	if (BasisVectors.Num() == 0)
	{
		// Constructed before basis vectors loaded — Position will stay zero-length and indexing asserts later.
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("FNarrativeEntityInstance('%s'): 0 basis vectors at construction; entity will have zero-length state vectors."),
			*Name.ToString());
	}

	int32 MissingStartingCoords = 0;
	int32 MissingPreviousCoords = 0;

	// Position
	Position.SetNum(BasisVectors.Num());
	for (int i = 0; i < Position.Num(); ++i)
	{
		const UNarrativeBasisVector* BasisVector = BasisVectors[i];
		if (!IsValid(BasisVector))
		{
			Position[i] = 0.0f;
			++MissingStartingCoords;
			continue;
		}

		// StartingCoordinates is keyed on TSoftObjectPtr; construct one from the
		// cached object to look up. Soft-pointer hash is path-based and matches
		// the editor-authored key.
		const float* Coord = EntityDef.StartingCoordinates.Find(TSoftObjectPtr<UNarrativeBasisVector>(BasisVector));
		Position[i] = Coord ? *Coord : 0.0f;
	}

	// Previous Position
	OldPosition.SetNum(BasisVectors.Num());
	for (int i = 0; i < OldPosition.Num(); ++i)
	{
		const UNarrativeBasisVector* BasisVector = BasisVectors[i];
		if (!IsValid(BasisVector))
		{
			continue;
		}

		const float* Coord = EntityDef.PreviousCoordinates.Find(TSoftObjectPtr<UNarrativeBasisVector>(BasisVector));
		if (!Coord)
		{
			++MissingPreviousCoords;
			continue;
		}

		OldPosition[i] = *Coord;
	}

	if (MissingStartingCoords > 0 || MissingPreviousCoords > 0)
	{
		// Authoring gap: a basis vector exists in the registry that the EntityDef doesn't have a coordinate for.
		// Defaulting to 0 is non-obvious to designers and a frequent source of "the NPC starts at the origin" bugs.
		UE_LOG(LogDB_NarrativeEngine, Verbose,
			TEXT("FNarrativeEntityInstance('%s'): defaulted %d/%d StartingCoordinates and %d/%d PreviousCoordinates to 0."),
			*Name.ToString(),
			MissingStartingCoords, BasisVectors.Num(),
			MissingPreviousCoords, BasisVectors.Num());
	}
}

void FNarrativeScene::Tick(double DeltaTime)
{
	Time += DeltaTime;
}
