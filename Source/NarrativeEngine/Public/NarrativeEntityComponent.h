// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NarrativeIds.h"
#include "NarrativeStaticData.h"
#include "NarrativeSubsystem.h"
#include "Components/ActorComponent.h"
#include "NarrativeEntityComponent.generated.h"


/* Registers an actor to a narrative "proxy entity", which gives them a narrative state-space vector representation */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NARRATIVEENGINE_API UNarrativeEntityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNarrativeEntityComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void RegisterWithRuntime();
#if WITH_EDITOR
	/* A new placement, or a pasted copy, gets its own spawn GUID; PIE duplication keeps it. */
	virtual void OnComponentCreated() override;
	virtual void PostEditImport() override;
#endif
	virtual void HandleLocationChanged(const FVectorND& NewLocation);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* The static data record that defines this entities properties when initializing the runtime */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<const UNarrativeEntityDef> EntityDef;

	/* The authored persistent spawn GUID of a placed character (TDD Pattern 21). Copies get a new one.
	 * A placement without one derives a stable id from its level path; a runtime spawn draws one
	 * from the campaign. */
	UPROPERTY(VisibleAnywhere, NonPIEDuplicateTransient, Category = "Narrative")
	FGuid PersistentSpawnId;

	/* This actor's campaign entity, once bound; invalid when the store refused it as a duplicate. */
	const FNarrativeEntityId& GetEntityId() const { return EntityId; }

	FVectorND GetLocation() const;
	FVectorND GetOldLocation() const;
	FNarrativeEntityInstance* GetInstance();
	
	FOnLocationChangeDelegate OnLocationChanged;

	/*
	 * Alignment a product of goal and progress angles between two entities. ??
	 */
	float GetAlignmentTo(const UNarrativeEntityDef& InEntityDef);

private:
	FNarrativeEntityId ResolveEntityId(struct FNarrativeStore& Store) const;

	FNarrativeEntityId EntityId;
};
