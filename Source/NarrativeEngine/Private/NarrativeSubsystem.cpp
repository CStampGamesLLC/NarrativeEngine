// Fill out your copyright notice in the Description page of Project Settings.

#include "NarrativeSubsystem.h"

static TAutoConsoleVariable<float> CVarNarrativeTelosStrength(
	TEXT("db.Narrative.TelosStrength"),
	1.f,
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
		FSetElementId EntityId = Scene.Entities.Add(FNarrativeEntityInstance{EntityDef});
		FNarrativeEntityInstance& Entity = Scene.Entities[EntityId];
		
		// Telos is where the entity tends to drift (this is the effective inertial force of the entity)
		// The Telos of an Acorn is to move into its reality vector as a tree; it's stubborness. 
		// The state vectors this tends towards can affect behaviors, gameplay, aesthetics, etc.
		Entity.Telos = Entity.Position;
	});
}

void UNarrativeSubsystem::CalculateAcceleration(FNarrativeEntityInstance& Entity, float DeltaTime)
{
	Entity.Acceleration = Entity.Telos;// * CVarNarrativeTelosStrength.GetValueOnGameThread();
	
	// cstamper todo try this instead when it's for real... maybe CVar it
	//const FVectorND x = Entity.Position;
	//const FVectorND goal = Entity.Telos;               // interpret as goal location OR goal direction
	//const FVectorND dir = (goal - x);

	//Entity.Acceleration = dir.GetSafeNormal() * Entity.TelosStrength; // or proportional without normalize
}
#pragma endregion

#pragma region Runtime
void UNarrativeSubsystem::SimulateEntities(float DeltaTime)
{
	for (FNarrativeEntityInstance& Entity : Scene.Entities)
	{
		// Accumulate forces
		CalculateAcceleration(Entity, DeltaTime);
		
		// Simulate movement & forces 
		VerletIntegrate(Entity, DeltaTime);
		
		if (Entity.Position != Entity.OldPosition)
		{
			FOnLocationChangeDelegate* ChangeDelegate = OnLocationChangeDelegates.Find(Entity.Asset.Get());
			if (ChangeDelegate)
			{
				ChangeDelegate->Broadcast(Entity.Position);
			}
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
		const float Velocity = Entity.OldPosition[i] - Entity.Position[i];
		const float Acceleration = Entity.Acceleration[i];
		NewPosition[i] = Entity.Position[i] + Velocity + Acceleration * DeltaTime * DeltaTime;

		// Update previous position for the next iteration
		Entity.OldPosition[i] = Entity.Position[i];
		Entity.Position[i] = NewPosition[i];
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
