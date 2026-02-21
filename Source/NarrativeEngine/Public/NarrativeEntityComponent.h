// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	virtual void RegisterWithRuntime();
	virtual void HandleLocationChanged(const FVectorND& NewLocation);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* The static data record that defines this entities properties when initializing the runtime */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<const UNarrativeEntityDef> EntityDef;

	FVectorND GetLocation() const;
	FVectorND GetOldLocation() const;
	
	FOnLocationChangeDelegate OnLocationChanged;

	/*
	 * Alignment a product of goal and progress angles between two entities. ??
	 */
	float GetAlignmentTo(const UNarrativeEntityDef& InEntityDef);
};
