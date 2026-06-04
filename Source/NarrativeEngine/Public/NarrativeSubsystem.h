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
	const float EntityDistanceBroadcastThreshold = 0.067f;
	/* Timer to trigger an immediate broadcast */
	const float EntityDeltaBroadcastInterval = 0.35f;
	/* 60hz tick interval - prevents undefined behaviors from large/small delta times */
	const float FixedStepSize = 0.016f;
	
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
#pragma endregion
};
