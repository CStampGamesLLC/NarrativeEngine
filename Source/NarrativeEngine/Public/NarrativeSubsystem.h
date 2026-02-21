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

	// Actual public API
public:
	void RegisterEntity(const UNarrativeEntityDef& InEntityDef);
	
	TMap<TWeakObjectPtr<const UNarrativeEntityDef>, FOnLocationChangeDelegate> OnLocationChangeDelegates;
	
public:
	// Setup
	virtual TStatId GetStatId() const override;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	void InitEntities();
	void CalculateAcceleration(FNarrativeEntityInstance& Entity, float DeltaTime);
	void SimulateEntities(float DeltaTime);
	void VerletIntegrate(FNarrativeEntityInstance& Entity, double DeltaTime);

	FOnEntitySpawned OnEntitySpawned;
	void WaveFunctionCollapse();
	
	// Runtime
	virtual void Tick(float DeltaTime) override;

	FNarrativeScene Scene;
	
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
	static void ForeachEntity(UWorld* InWorld, TFunction<void(FNarrativeEntityInstance&)> Callback);
};
