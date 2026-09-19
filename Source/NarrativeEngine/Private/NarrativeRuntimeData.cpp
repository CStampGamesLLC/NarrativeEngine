#include "NarrativeRuntimeData.h"
#include "NarrativeSubsystem.h"

FNarrativeEntityInstance::FNarrativeEntityInstance(const UNarrativeEntityDef& EntityDef)
{
	Asset = &EntityDef;
	Name = EntityDef.GetFName();
	Mass = EntityDef.Mass;

	TArray<TSoftObjectPtr<UNarrativeBasisVector>> BasisVectors = UNarrativeBasisVector::GetLoadedAssets();

	if (!ensure(EntityDef.StartingCoordinates.IsValid() && EntityDef.PreviousCoordinates.IsValid()))
	{
		return;
	}
	// Include unresolved authored axes as well as all registered axes; never reinterpret indices.
	Position = EntityDef.StartingCoordinates.Rebased(BasisVectors);
	OldPosition = EntityDef.PreviousCoordinates.Rebased(Position.GetBasis());
	Position = Position.Rebased(OldPosition.GetBasis());
	LastBroadcastPosition = Position;
	Acceleration = FVectorND(Position.GetBasis());
	Telos = Position;
}

void FNarrativeScene::Tick(double DeltaTime)
{
	Time += DeltaTime;
}
