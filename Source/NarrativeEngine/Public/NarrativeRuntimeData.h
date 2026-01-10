#pragma once
#include "NarrativeStaticData.h"
#include "NarrativeRuntimeData.generated.h"

static const FString AssetPath = TEXT("/Game/ENarrativeBasisVectors.ENarrativeBasisVectors");

USTRUCT()
struct FNarrativeComponent
{
	GENERATED_BODY()

	
};

// Entity struct representing a particle in the space
/*
 * Entities tend to simulate towards the tensor product of their motive, mortality, conviction, and telos.
 * - Boids implement a constant perterbation of particles
 * - Verlet Integration kinematically simulates the flow of entities within the abstract state space
 * - A* enables pathfinding straight along their telos vector or naturally seeking low entropy states towards their motive.
 * - Every entity exists and simulates simultaneously, regardless of whether or not they're being observed/interacted through through the gameplay layer
 */
struct FNarrativeEntityInstance
{
	TSoftObjectPtr<UNarrativeEntityDef> Asset;
	
	FVectorND Position;  // Current position
	FVectorND OldPosition; // Previous position for Verlet integration
	FVectorND Acceleration; // Accumulated acceleration
	FName Name; // For thematic association
	double Mass;  // Mass of the entity, effectively their "conviction" or potential impact on peoples' inherited will

	/* Normal vector for a simulated entity. All entities simulate strait along this vector */
	FVectorND Telos;
	
	/* Where entities path to (when pathable). Motives can become unreachable, but new motives can be acquired */
	FVectorND Motive;

	FNarrativeEntityInstance(const UNarrativeEntityDef& EntityDef);

	bool operator==(const FNarrativeEntityInstance& Other) const
	{
		return Name == Other.Name;
	}

	friend uint32 GetTypeHash(const FNarrativeEntityInstance& Entity)
	{
		return GetTypeHash(Entity.Name);
	}
};

// Event as a thematic force acting between entities
struct FNarrativeEventInstance
{
	FString Description; // Thematic description of the force
	TFunction<void(FNarrativeEntityInstance&, FNarrativeEntityInstance&)> ApplyForce; // Force function
};

/* Root object for storing all entities
 *	- N-dimensional Hilbert Space, where N = ENarrativeBasisVector::Num
 *  - The norm of a state vector is always 1(∥ψ∥=1), providing geometric structure to the state space
 *  - Inner products describe how two states relate to each other; orthogonality (i.e. zero inner product) indicates two entirely distinct, parallel outcomes of the state space
 *  
 */
struct FNarrativeScene 
{
	double Time = 0.f;
	TSet<FNarrativeEntityInstance> Entities;

	void Tick(double DeltaTime);
};

