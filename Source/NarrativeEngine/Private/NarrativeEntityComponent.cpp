// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeEntityComponent.h"

#include "NarrativeSubsystem.h"


// Sets default values for this component's properties
UNarrativeEntityComponent::UNarrativeEntityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UNarrativeEntityComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterWithRuntime();
	// ...
	
}

void UNarrativeEntityComponent::RegisterWithRuntime()
{
	if (!ensureMsgf(EntityDef.IsValid(), TEXT("Actor: %s has a NarrativeEntity component, but it doesn't define and EntityDef binding"), *GetOwner()->GetName()))
	{
		return;
	}

	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		return;
	}

	NarrativeSubsystem->RegisterEntity(*EntityDef);
}


// Called every frame
void UNarrativeEntityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FVectorND UNarrativeEntityComponent::GetLocation() const
{
	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		return {};
	}

	FNarrativeEntityInstance* EntityInstance = NarrativeSubsystem->Scene.Entities.Find(*EntityDef);
	if (!EntityInstance)
	{
		return {};
	}
	
	return EntityInstance->Position;
}

float UNarrativeEntityComponent::GetAlignmentTo(const UNarrativeEntityDef& InEntityDef)
{
	return 0.f;
}

