#include "NarrativeRuntimeData.h"
#include "NarrativeSubsystem.h"

FNarrativeEntityInstance::FNarrativeEntityInstance(const UNarrativeEntityDef& EntityDef)
{
	Name = EntityDef.Name;
	Mass = EntityDef.Mass;


	TArray<TSoftObjectPtr<UNarrativeBasisVector>> BasisVectors = UNarrativeBasisVector::GetLoadedAssets();

	// Position
	Position.SetNum(BasisVectors.Num());
	for (int i = 0; i < Position.Num(); ++i)
	{
		const float* Coord = EntityDef.StartingCoordinates.Find(BasisVectors[i]);
		if (!Coord)
		{
			continue;
		}

		Position[i] = *Coord;
	}

	// Previous Position
	OldPosition.SetNum(BasisVectors.Num());
	for (int i = 0; i < OldPosition.Num(); ++i)
	{
		const float* Coord = EntityDef.PreviousCoordinates.Find(BasisVectors[i]);
		if (!Coord)
		{
			continue;
		}

		OldPosition[i] = *Coord;
	}
}

void FNarrativeScene::Tick(double DeltaTime)
{
	Time += DeltaTime;
}
