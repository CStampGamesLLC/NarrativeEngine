// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeEntityComponent.h"

#include "NarrativeEngine.h"
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
}

void UNarrativeEntityComponent::RegisterWithRuntime()
{
	if (!ensureMsgf(EntityDef.IsValid(), TEXT("Actor: %s has a NarrativeEntity component, but it doesn't define and EntityDef binding"), *GetOwner()->GetName()))
	{
		// Actor will be invisible to the narrative sim — won't drift, won't broadcast, won't appear in dialog forces.
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("RegisterWithRuntime: actor '%s' has UNarrativeEntityComponent but no EntityDef bound; actor will not participate in the sim."),
			*GetOwner()->GetName());
		return;
	}

	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		// World subsystem absent — usually means the component ticked before world subsystems initialized.
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("RegisterWithRuntime: NarrativeSubsystem not available for actor '%s' (EntityDef '%s'); registration skipped."),
			*GetOwner()->GetName(), *EntityDef->GetName());
		return;
	}

	NarrativeSubsystem->RegisterEntity(*EntityDef);
	NarrativeSubsystem->OnLocationChangeDelegates.FindOrAdd(EntityDef.Get()).AddUObject(this, &UNarrativeEntityComponent::HandleLocationChanged);

	UE_LOG(LogDB_NarrativeEngine, Log,
		TEXT("RegisterWithRuntime: actor '%s' bound to EntityDef '%s'."),
		*GetOwner()->GetName(), *EntityDef->GetName());
}

void UNarrativeEntityComponent::HandleLocationChanged(const FVectorND& NewLocation)
{
	UE_LOG(LogDB_NarrativeEngine, Verbose,
		TEXT("HandleLocationChanged: '%s' on actor '%s'."),
		EntityDef.IsValid() ? *EntityDef->GetName() : TEXT("<invalid>"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));

	OnLocationChanged.Broadcast(NewLocation);
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
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetLocation: NarrativeSubsystem unavailable for actor '%s'; returning empty vector."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return {};
	}

	if  (!ensure(EntityDef.IsValid()))
	{
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetLocation: EntityDef invalid on actor '%s'; returning empty vector."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return {};
	}

	FNarrativeEntityInstance* EntityInstance = NarrativeSubsystem->Scene.Entities.Find(*EntityDef);
	if (!EntityInstance)
	{
		// EntityDef bound but the sim never registered it — likely an init ordering bug.
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetLocation: no scene entry for EntityDef '%s' (actor '%s'); returning empty vector."),
			*EntityDef->GetName(), GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return {};
	}

	return EntityInstance->Position;
}

FVectorND UNarrativeEntityComponent::GetOldLocation() const
{
	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetOldLocation: NarrativeSubsystem unavailable for actor '%s'; returning empty vector."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return {};
	}

	FNarrativeEntityInstance* EntityInstance = NarrativeSubsystem->Scene.Entities.Find(*EntityDef);
	if (!EntityInstance)
	{
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetOldLocation: no scene entry for EntityDef '%s' (actor '%s'); returning empty vector."),
			EntityDef.IsValid() ? *EntityDef->GetName() : TEXT("<invalid>"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return {};
	}

	return EntityInstance->OldPosition;
}

FNarrativeEntityInstance* UNarrativeEntityComponent::GetInstance()
{
	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetInstance: NarrativeSubsystem unavailable for actor '%s'; returning nullptr."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return nullptr;
	}

	if (!ensure(EntityDef.IsValid()))
	{
		UE_LOG(LogDB_NarrativeEngine, Warning,
			TEXT("GetInstance: EntityDef invalid on actor '%s'; returning nullptr."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<no owner>"));
		return nullptr;
	}

	return NarrativeSubsystem->Scene.Entities.Find(*EntityDef);
}

float UNarrativeEntityComponent::GetAlignmentTo(const UNarrativeEntityDef& InEntityDef)
{
	return 0.f;
}

