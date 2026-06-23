// Fill out your copyright notice in the Description page of Project Settings.

#include "NarrativeSubsystem.h"
#include "GameFramework/WorldSettings.h"

static TAutoConsoleVariable<float> CVarNarrativeTelosStrength(
	TEXT("db.Narrative.TelosStrength"),
	0.1f,
	TEXT("Strength multiplier applied to narrative Telos acceleration.\n")
	TEXT("0 = no goal pull, 1 = default, >1 = aggressive convergence."),
	ECVF_Default
);

/* Narrative data boilerplate
 *
 * 1. Scan the asset registry for data asset types and cache FAssetData in `UNarrativeDataSubsystem::NarrativeAssetData` for lazy loading from data asset typename
 * 2. Call either:
 *		a. UNarrativeSubsystem::ForeachDataAsset<UMyAssetType>([&](const UMyAssetType& EntityDef) 
 *			as needed for iterating a class of assets
 *		b. UNarrativeSubsystem::GetDataAsset<UMyAssetType>(const FName& AssetName) 
 *			as needed for a particular asset
 * 3. NARRATIVE_DATA_HELPERS is used by any child of UNarrativeDataAsset to facilitate the following methods provided typename T:
 *		a. static TArray<FAssetData> GetAssetData()
 *		b. static TArray<TSoftObjectPtr<T>> GetLoadedAssets()
 */
void UNarrativeDataSubsystem::RegisterNarrativeAssets(FAssetRegistryModule& AssetRegistryModule)
{
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeBasisVector)
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeEntityDef)
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeEventDef)
	REGISTER_NARRATIVE_ASSET_TYPE(UNarrativeDialogDef)
}

void UNarrativeDataSubsystem::OnAssetRegistryReady()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	RegisterNarrativeAssets(AssetRegistryModule);
}

void UNarrativeDataSubsystem::InitializeNarrativeAssetData()
{
	// 1. Get the Asset Registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	if (AssetRegistry.IsLoadingAssets() || AssetRegistry.IsGathering())
	{
		AssetRegistry.OnFilesLoaded().AddUObject(this, &UNarrativeDataSubsystem::OnAssetRegistryReady);
		// Optionally kick search if needed:
		if (!AssetRegistry.IsSearchAllAssets())
		{
			AssetRegistry.SearchAllAssets(/*bSynchronousSearch=*/false);
		}
		return;
	}

	RegisterNarrativeAssets(AssetRegistryModule);
}

void UNarrativeDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	InitializeNarrativeAssetData();
}

void UNarrativeDataSubsystem::PinAsset(UNarrativeDataAsset* InAsset)
{
	if (IsValid(InAsset))
	{
		LoadedNarrativeAssets.AddUnique(InAsset);
		NamedAssetCache.Emplace(InAsset->GetFName(), InAsset);
	}
}

FPrimaryAssetId UNarrativeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId{"NarrativeData", GetFName()};
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
		FSetElementId EntityId = Scene.Entities.Add(FNarrativeEntityInstance{EntityDef});
		FNarrativeEntityInstance& Entity = Scene.Entities[EntityId];
		
		// Telos is where the entity tends to drift (this is the effective inertial force of the entity)
		// The Telos of an Acorn is to move into its reality vector as a tree; it's stubborness. 
		// The state vectors this tends towards can affect behaviors, gameplay, aesthetics, etc.
		Entity.Telos = Entity.Position;
		// Zero out velocity (which is derived from old - new positions). 
		Entity.OldPosition = Entity.Position;
	});
}

// Acceleration = Force / Mass
void UNarrativeSubsystem::CalculateAcceleration(FNarrativeEntityInstance& Entity, float DeltaTime)
{
	if (!ensure(Entity.Asset.IsValid()))
	{
		return;
	}

	FVectorND ForceSum; // zeroed

	// External forces (again: these are "forces" unless you handle impulse semantics elsewhere)
	for (const FVectorND& F : Entity.QueuedImpulseForces)
	{
		ForceSum += F;
	}
	Entity.QueuedImpulseForces.Reset();

	if (DeltaTime > KINDA_SMALL_NUMBER)
	{
		// Verlet velocity estimate
		const FVectorND Velocity = (Entity.Position - Entity.OldPosition) / DeltaTime;

		// Immutable "telos direction" (preferred direction of motion)
		const FVectorND TelosDir = Entity.Telos.GetSafeNormal();
		if (TelosDir.LengthSquared() > 0.f)
		{
			// Decompose velocity into parallel/perp relative to TelosDir
			const float VParallelMag = Velocity.Dot(TelosDir);
			const FVectorND VParallel = TelosDir * VParallelMag;
			const FVectorND VPerp = Velocity - VParallel;

			// 1) Alignment: damp perpendicular motion (makes it "want" to move along Telos)
			const float Align = FMath::Max(0.f, Entity.Asset->Alignment); // new param
			ForceSum += VPerp * (-Align);

			// 2) Drift: constant push along Telos (the "tends to drift" part)
			const float Drift = Entity.Asset->Drift; // can be +/- to reverse
			ForceSum += TelosDir * Drift;
		}

		// Optional generic damping (overall)
		const float Damping = FMath::Max(0.f, Entity.Asset->Damping);
		ForceSum += Velocity * (-Damping);
	}

	const float Mass = FMath::Max(Entity.Asset->Mass, 1.f);
	Entity.Acceleration = ForceSum / Mass;
}
#pragma endregion

#pragma region Runtime
void UNarrativeSubsystem::SimulateEntities(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UNarrativeSubsystem::SimulateEntities)
	
	// Tick at a steady 60hz to prevent weird jumps/artifacts from odd DT values
	FixedStepAccumulator += DeltaTime;
	while (FixedStepAccumulator > FixedStepSize)
	{
		FixedStepAccumulator -= FixedStepSize;
	
		for (FNarrativeEntityInstance& Entity : Scene.Entities)
		{
			// Accumulate forces
			CalculateAcceleration(Entity, FixedStepSize);
		
			// Simulate movement & forces 
			VerletIntegrate(Entity, FixedStepSize);
		
			// Broadcast to listeners
			BroadcastEntityDelta(Entity, FixedStepSize);
		}
	}
}

void UNarrativeSubsystem::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UNarrativeSubsystem::Tick)
	
	Super::Tick(DeltaTime);

	// The narrative sim is the world's "broad phase": it runs in real time regardless of global time
	// dilation, so a local soft-pause (focus) slows the player's view without slowing the simulated
	// world. Un-dilate the delta so e.g. a 0.2 dilation still advances the sim at the true rate —
	// this is the "time injected back into the narrative simulation" the local-focus concept needs.
	float RealDeltaTime = DeltaTime;
	if (const UWorld* World = GetWorld())
	{
		if (const AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			const float Dilation = WorldSettings->GetEffectiveTimeDilation();
			if (Dilation > KINDA_SMALL_NUMBER)
			{
				RealDeltaTime = DeltaTime / Dilation;
			}
		}
	}

	Scene.Tick(RealDeltaTime);

	// Apply gravitational forces between entities
	SimulateEntities(RealDeltaTime);
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

// Verlet integration step - in effect, this is the core runtime for the partical physics simulation of narrative entities
void UNarrativeSubsystem::VerletIntegrate(FNarrativeEntityInstance& Entity, double DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UNarrativeSubsystem::VerletIntegrate)
	
	//cstamper todo - optimize FVectorND construction so that it's usable
	FVectorND NewPosition;

	if (!ensure(NewPosition.Num() == Entity.Position.Num()))
	{
		return;
	}

	for (int i = 0; i < Entity.Position.Num(); ++i)
	{
		// Verlet position update
		const float Velocity = Entity.Position[i] - Entity.OldPosition[i];
		const float Acceleration = Entity.Acceleration[i];
		NewPosition[i] = Entity.Position[i] + Velocity + Acceleration * DeltaTime * DeltaTime;

		// Update previous position for the next iteration
		Entity.OldPosition[i] = Entity.Position[i];
		Entity.Position[i] = NewPosition[i];
		
		// Reset acceleration
		Entity.Acceleration[i] = 0.f;
	}
}

void UNarrativeSubsystem::BroadcastEntityDelta(FNarrativeEntityInstance& Entity, const float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UNarrativeSubsystem::BroadcastEntityDelta)
	
	FOnLocationChangeDelegate* ChangeDelegate = OnLocationChangeDelegates.Find(Entity.Asset.Get());
	if (!ChangeDelegate)
	{
		return;
	}
	
	// Delta check time
	Entity.EntityDeltaBroadcastTimer += DeltaTime;
	const bool bTimerIntervalElapsed = Entity.EntityDeltaBroadcastTimer >= EntityDeltaBroadcastInterval;
		
	// Delta check distance
	const float DistanceDelta = Entity.Position.Distance(Entity.LastBroadcastPosition);
	const bool bSufficientDistance =  DistanceDelta > EntityDistanceBroadcastThreshold;
		
	// Broadcast
	if (bTimerIntervalElapsed || bSufficientDistance)
	{
		Entity.LastBroadcastPosition = Entity.Position;
		Entity.EntityDeltaBroadcastTimer = 0.f;
		ChangeDelegate->Broadcast(Entity.Position);
	}
}
#pragma endregion

