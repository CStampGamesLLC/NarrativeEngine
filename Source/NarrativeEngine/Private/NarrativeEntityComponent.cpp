// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeEntityComponent.h"

#include "NarrativeStoreSubsystem.h"
#include "NarrativeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"


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
		return;
	}

	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		return;
	}

	NarrativeSubsystem->RegisterEntity(*EntityDef);
	NarrativeSubsystem->OnLocationChangeDelegates.FindOrAdd(EntityDef.Get()).AddUObject(this, &UNarrativeEntityComponent::HandleLocationChanged);

	// The campaign entity this actor shows. Travel and streaming rebind the same id; they never make a new one.
	FNarrativeStore* Store = UNarrativeStoreSubsystem::Find(this);
	if (!Store)
	{
		return;
	}

	const FNarrativeEntityId Id = ResolveEntityId(*Store);
	if (!Store->Views.Bind(Id, *GetOwner()))
	{
		UE_LOG(LogNarrativeTrace, Error, TEXT("%s shares narrative entity %s with %s: duplicate placements need their own spawn GUID. Not bound."), *GetOwner()->GetPathName(), *Id.ToString(), *GetPathNameSafe(Store->Views.FindView(Id)));
		return;
	}

	EntityId = Id;
	Store->Campaign.RegisterEntity(Id, *EntityDef);
}

FNarrativeEntityId UNarrativeEntityComponent::ResolveEntityId(FNarrativeStore& Store) const
{
	if (PersistentSpawnId.IsValid())
	{
		return FNarrativeEntityId(PersistentSpawnId);
	}

	const AActor* Owner = GetOwner();
	if (IsValid(Owner) && Owner->IsNetStartupActor())
	{
		// Placed before spawn GUIDs existed: its level path is stable from run to run.
		return FNarrativeEntityId::FromStablePath(UWorld::RemovePIEPrefix(Owner->GetPathName()));
	}

	return Store.Campaign.AllocateRuntimeId();
}

void UNarrativeEntityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Only the view detaches: the entity, its mind and everything it remembers stay in the store.
	if (EntityId.IsValid())
	{
		if (FNarrativeStore* Store = UNarrativeStoreSubsystem::Find(this))
		{
			Store->Views.Unbind(EntityId, *GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UNarrativeEntityComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	const AActor* Owner = GetOwner();
	if (!PersistentSpawnId.IsValid() && Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
		&& Owner->GetWorld() && Owner->GetWorld()->WorldType == EWorldType::Editor)
	{
		PersistentSpawnId = FGuid::NewGuid();
	}
}

void UNarrativeEntityComponent::PostEditImport()
{
	Super::PostEditImport();

	// A paste is a new placement.
	PersistentSpawnId = FGuid::NewGuid();
}
#endif

void UNarrativeEntityComponent::HandleLocationChanged(const FVectorND& NewLocation)
{
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
		return {};
	}
	
	if  (!ensure(EntityDef.IsValid()))
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

FVectorND UNarrativeEntityComponent::GetOldLocation() const
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
	
	return EntityInstance->OldPosition;
}

FNarrativeEntityInstance* UNarrativeEntityComponent::GetInstance()
{
	UNarrativeSubsystem* NarrativeSubsystem = GetWorld()->GetSubsystem<UNarrativeSubsystem>();
	if (!IsValid(NarrativeSubsystem))
	{
		return nullptr;
	}
	
	if (!ensure(EntityDef.IsValid()))
	{
		return nullptr;
	}
	
	return NarrativeSubsystem->Scene.Entities.Find(*EntityDef);
}

float UNarrativeEntityComponent::GetAlignmentTo(const UNarrativeEntityDef& InEntityDef)
{
	return 0.f;
}

