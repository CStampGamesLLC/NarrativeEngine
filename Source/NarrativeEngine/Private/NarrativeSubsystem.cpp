// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeSubsystem.h"

void UNarrativeDataSubsystem::InitializeNarrativeAssetData()
{
	// 1. Get the Asset Registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Make sure the registry is up to date; this is often necessary
	// if you're running this code early in the game.
	AssetRegistryModule.Get().SearchAllAssets(true);

	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeBasisVector)
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeEntityDef)
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeEventDef)
}

void UNarrativeDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	InitializeNarrativeAssetData();
}

template <typename T>
TArray<FAssetData> UNarrativeDataSubsystem::GetAssetData()
{
	TArray<FAssetData> CachedAssetData;
	const uint32 ClassHash = GetClassHash<T>();
	NarrativeAssetData.MultiFind(ClassHash, CachedAssetData);
	return CachedAssetData;
}

#pragma region Setup
void UNarrativeSubsystem::RegisterEntity(const UNarrativeEntityDef& InEntityDef)
{
	if (Scene.Entities.Contains(InEntityDef))
	{
		return;
	}
	
	Scene.Entities.Emplace(InEntityDef);
}

TStatId UNarrativeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UNarrativeSubsystem, STATGROUP_Tickables);
}

void UNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UNarrativeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	InitEntities();
}

void UNarrativeSubsystem::InitEntities()
{
	ForeachDataAsset<UNarrativeEntityDef>([this](const UNarrativeEntityDef& EntityDef)
	{
		// Scene.Entities.Emplace(FNarrativeEntityInstance{EntityDef});
	});
}
#pragma endregion

#pragma region Runtime
void UNarrativeSubsystem::SimulateEntities(float DeltaTime)
{
	for (FNarrativeEntityInstance& EntityA : Scene.Entities)
	{
		for (FNarrativeEntityInstance EntityB : Scene.Entities)
		{
			if (EntityA == EntityB)
			{
				continue;
			}
			
			ApplyGravity(EntityA, EntityB, GravConstant);
		}
	}

	for (FNarrativeEntityInstance& Entity : Scene.Entities)
	{
		for (int Step = 0; Step < 20; ++Step)
		{
			// Simulate movement & forces 
			VerletIntegrate(Entity, DeltaTime);
		}
	}
}

void UNarrativeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Scene.Tick(DeltaTime);

	// Apply gravitational forces between entities
	SimulateEntities(DeltaTime);
	
	WaveFunctionCollapse();
}

void UNarrativeSubsystem::ForeachEntity(UWorld* InWorld, TFunction<void(FNarrativeEntityInstance&)> Callback)
{
	UNarrativeSubsystem* NarrativeSubsystem = InWorld->GetSubsystem<UNarrativeSubsystem>();
	if (!ensure(IsValid(InWorld)))
	{
		return; 
	}
	
	for (FNarrativeEntityInstance& Entity : NarrativeSubsystem->Scene.Entities)
	{
		Callback(Entity);
	}
}

// Simulate curvature-based gravity between two entities
void  UNarrativeSubsystem::ApplyGravity(FNarrativeEntityInstance& A, FNarrativeEntityInstance& B, double G)
{
	FVectorND Direction;
	double Distance = A.Position.Distance(B.Position);
	double ForceMagnitude = G * A.Mass * B.Mass / (Distance * Distance + 0.1); // Regularize for small distances

	// Compute the force vector
	for (int i = 0; i < Direction.Num(); ++i)
	{
		Direction[i] = (B.Position[i] - A.Position[i]) / Distance;
	}

	// Apply the force
	for (int i = 0; i < Direction.Num(); ++i)
	{
		A.Acceleration[i] += ForceMagnitude * Direction[i] / A.Mass;
		B.Acceleration[i] -= ForceMagnitude * Direction[i] / B.Mass; // Equal and opposite
	}
}

// Verlet integration step - in effect, this is the core runtime for the partical physics simulation of narrative entities
void UNarrativeSubsystem::VerletIntegrate(FNarrativeEntityInstance& Entity, double DeltaTime)
{
	//cstamper todo - optimize FVectorND construction so that it's usable
	FVectorND NewPosition;
	
	if (!ensure(NewPosition.Num() == Entity.Position.Num()))
	{
		return;
	}
	
	for (int i = 0; i < Entity.Position.Num(); ++i)
	{
		// Verlet position update
		NewPosition[i] = 2 * Entity.Position[i] - Entity.OldPosition[i] + Entity.Acceleration[i] * DeltaTime * DeltaTime;
	
		// Update previous position for the next iteration
		Entity.OldPosition[i] = Entity.Position[i];
		Entity.Position[i] = NewPosition[i];

		// todo consider implementing a dampening factor 
		// Reset acceleration
		Entity.Acceleration[i] = 0.0;
	}
}

void UNarrativeSubsystem::WaveFunctionCollapse()
{
	for (FNarrativeEntityInstance& Entity : Scene.Entities)
	{
		if (!Entity.Asset.IsValid())
		{
			continue;
		}
		
		// todo this isn't right... needs some work. This needs to select some action, which could be a dialog
		float CollapseProbability = exp(-Entity.Asset->ShannonEntropyRadius);
		if (FMath::RandRange(0.0f, 1.0f) < CollapseProbability)
		{
			// Instantiate entity in the gameplay layer
			OnEntitySpawned.Broadcast(Entity);
		}
	}
}
#pragma endregion

#pragma region Tools

template <typename T>
void UNarrativeSubsystem::ForeachDataAsset(const TFunction<void(const T&)> Callback)
{
	TArray<TSoftObjectPtr<T>> FoundAssets = T::GetLoadedAssets();
	for (TSoftObjectPtr<T>& Asset : FoundAssets)
	{
		if (!Asset.IsValid())
		{
			continue;
		}

		Callback(*Asset);
	}
}

template <typename T>
const T* UNarrativeSubsystem::GetDataAsset(const FName& RecordName)
{
	T* OutRecord = nullptr;
	ForeachRecord<T>([RecordName, &OutRecord](const T& Record)
	{
		if (!Record.Name != RecordName)
		{
			return;
		}

		OutRecord = &Record;
	});

	return OutRecord;
}

#pragma endregion