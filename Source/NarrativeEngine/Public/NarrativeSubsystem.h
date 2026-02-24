// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "Subsystems/WorldSubsystem.h"
#include "NarrativeStaticData.h"
#include "NarrativeRuntimeData.h"
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
	const float EntityDistanceBroadcastThreshold = 0.1f;
	/* Timer to trigger an immediate broadcast */
	const float EntityDeltaBroadcastInterval = 0.5f;
	
#pragma endregion
	
#pragma region EngineMethods
public:
	virtual void Tick(float DeltaTime) override;
#pragma endregion
	
	// Actual public API
public:
	void RegisterEntity(const UNarrativeEntityDef& InEntityDef);
	static void ForeachEntity(UWorld* InWorld, TFunction<void(FNarrativeEntityInstance&)> Callback);
	
	TMap<TWeakObjectPtr<const UNarrativeEntityDef>, FOnLocationChangeDelegate> OnLocationChangeDelegates;
	
public:
	// Setup
	virtual TStatId GetStatId() const override;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;


	FOnEntitySpawned OnEntitySpawned;
	void WaveFunctionCollapse();
	
	// Runtime
	template <typename T>
	const T* GetDataAsset(const FName& RecordName);
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
	void BroadcastEntityDelta(const FNarrativeEntityInstance& Entity, const float DeltaTime);
	
#pragma endregion
	
#pragma region Internal State
	FNarrativeScene Scene;
	float EntityDeltaBroadcastTimer = 0.f;
#pragma endregion
};
