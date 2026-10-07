// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "Subsystems/WorldSubsystem.h"
#include "NarrativeStaticData.h"
#include "NarrativeRuntimeData.h"
#include "NarrativeStoreSubsystem.h"
#include "NarrativeSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEntitySpawned, FNarrativeEntityInstance&)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLocationChangeDelegate, const FVectorND&)

/*
 * Narrative subsystem for keeping score of narrative engagement for all entities from characters to items
 *
 *	RULES:
 *		- Design is centered around the enrichment of basis vectors. It can't matter how many basis vectors are defined, but quality should be prioritized over quantity
 *		- Characters take actions based on reachable entities and normal probability distributions of semi-procedural actions (design defined events between entities).
 *		- Mechanical connections between narrative state and gameplay layer actors should be quality over quantity. They matter both in kind and density of use:
 *			- Changes in hostility / threat. Threat is a scalar that scales negatively into affection

 *			
 *
 * Setting, plot, and character exist to facilitate conflict.
 */

constexpr double GravConstant = 1.0; // Gravitational constant

/**  */
UCLASS()
class NARRATIVEENGINE_API UNarrativeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

#pragma region Convert to config
	
	/* Accumulated distance delta required to trigger a broadcast */
	const float EntityDistanceBroadcastThreshold = 0.067f;
	/* Timer to trigger an immediate broadcast */
	const float EntityDeltaBroadcastInterval = 0.35f;
	/* 60hz tick interval - prevents undefined behaviors from large/small delta times */
	const float FixedStepSize = 0.016f;
	/* Ceiling on substeps per frame for the legacy integrator. Time past it is dropped, not repaid:
	 * the legacy sim slows under load rather than spiralling. Minds never run on it; they run on the
	 * campaign clock, which keeps every tick (TDD Pattern 3). */
	const int32 MaxFixedStepsPerFrame = 3;

#pragma endregion
	
#pragma region EngineMethods
public:
	virtual void Tick(float DeltaTime) override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual TStatId GetStatId() const override;
#pragma endregion
	
	// Actual public API
public:
	void RegisterEntity(const UNarrativeEntityDef& InEntityDef);
	static void ForeachEntity(UWorld* InWorld, TFunction<void(FNarrativeEntityInstance&)> Callback);

	/** The live instance simulating from this definition, or null when the scene holds none. */
	FNarrativeEntityInstance* FindEntity(const UNarrativeEntityDef& InEntityDef);
	const FNarrativeEntityInstance* FindEntity(const UNarrativeEntityDef& InEntityDef) const
	{
		return const_cast<UNarrativeSubsystem*>(this)->FindEntity(InEntityDef);
	}

#pragma region Model
	/**
	 * While the model is active, valuation positions are frozen at their authored values and the
	 * campaign's minds run on game time; while inactive, the legacy integrator runs and no mind
	 * decides (TDD Patterns 2, 24). Set by the game layer when it settles on a mode.
	 */
	void SetModelActive(bool bInModelActive);
	bool IsModelActive() const { return bModelActive; }

	/** The store a world without a game instance reads (automation). Prefer UNarrativeStoreSubsystem::Find. */
	FNarrativeStore& GetLocalStore();
#pragma endregion

#if WITH_EDITOR
	/**
	 * Re-seeds every play world's instance of this definition from its authored coordinates, at
	 * rest, and tells its listeners immediately. This is the editor's live authoring path: moving
	 * a narrative space card or editing the vector while a session runs lands on the simulated
	 * entity rather than only on the asset. Mass, drift, rigidity and damping need no push - the
	 * simulation reads those off the asset every step already.
	 */
	static void SyncEntityFromAsset(const UNarrativeEntityDef& InEntityDef);

	/** Every play world's campaign takes the definition's sheet, mask, mass and position again. */
	static void RefreshCampaignsFromAsset(const UNarrativeEntityDef& InEntityDef);
#endif
	
	TMap<TWeakObjectPtr<const UNarrativeEntityDef>, FOnLocationChangeDelegate> OnLocationChangeDelegates;
	
public:
	// Setup

	FOnEntitySpawned OnEntitySpawned;
	
	// Runtime

	/**
	 * O(1) lookup of a UNarrativeDataAsset subclass by its UObject FName.
	 * Delegates to UNarrativeDataSubsystem::NamedAssetCache which is populated
	 * at asset-registration time via PinAsset.
	 */
	template <typename T>
	const T* GetDataAsset(const FName& RecordName) const
	{
		const UNarrativeDataSubsystem* DataSubsystem = GEngine ? GEngine->GetEngineSubsystem<UNarrativeDataSubsystem>() : nullptr;
		if (!IsValid(DataSubsystem))
		{
			return nullptr;
		}
		return DataSubsystem->GetAssetByName<T>(RecordName);
	}

	template <typename T>
	static void ForeachDataAsset(const TFunction<void(const T&)> Callback = nullptr)
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
	
#pragma region Internal Methods
	
	void InitEntities();
	void CalculateAcceleration(FNarrativeEntityInstance& Entity, float DeltaTime);
	void SimulateEntities(float DeltaTime);
	void VerletIntegrate(FNarrativeEntityInstance& Entity, double DeltaTime);
	void BroadcastEntityDelta(FNarrativeEntityInstance& Entity, const float DeltaTime);
	
#pragma endregion
	
#pragma region Internal State
	FNarrativeScene Scene;
	float FixedStepAccumulator = 0.f;
	bool bModelActive = false;
	TUniquePtr<FNarrativeStore> LocalStore;
#pragma endregion
};
