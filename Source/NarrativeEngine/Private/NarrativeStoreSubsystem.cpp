#include "NarrativeStoreSubsystem.h"

#include "NarrativeEntityComponent.h"
#include "NarrativeStaticData.h"
#include "NarrativeSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarNarrativeSeed(
		TEXT("db.Narrative.Seed"),
		static_cast<int32>(FNarrativeCampaign::DefaultSeed),
		TEXT("Seed for keyed narrative randomness, read when a campaign starts. The same seed and inputs replay the same stances."),
		ECVF_Default);
}

#pragma region Views
bool FNarrativeViewRegistry::Bind(const FNarrativeEntityId& Id, AActor& View)
{
	if (const TWeakObjectPtr<AActor>* Bound = ByEntity.Find(Id); Bound && Bound->IsValid() && Bound->Get() != &View)
	{
		return false;
	}

	if (const FNarrativeEntityId* Previous = ByActor.Find(&View); Previous && *Previous != Id)
	{
		ByEntity.Remove(*Previous);
	}

	ByEntity.Add(Id, &View);
	ByActor.Add(&View, Id);
	++Generation;
	return true;
}

void FNarrativeViewRegistry::Unbind(const FNarrativeEntityId& Id, const AActor& View)
{
	if (const TWeakObjectPtr<AActor>* Bound = ByEntity.Find(Id); Bound && Bound->Get() == &View)
	{
		ByEntity.Remove(Id);
	}
	ByActor.Remove(&View);
	++Generation;
}

AActor* FNarrativeViewRegistry::FindView(const FNarrativeEntityId& Id) const
{
	const TWeakObjectPtr<AActor>* Bound = ByEntity.Find(Id);
	return Bound ? Bound->Get() : nullptr;
}

FNarrativeEntityId FNarrativeViewRegistry::FindEntity(const AActor* View) const
{
	const FNarrativeEntityId* Id = View ? ByActor.Find(View) : nullptr;
	return Id ? *Id : FNarrativeEntityId();
}

void FNarrativeViewRegistry::Reset()
{
	ByEntity.Reset();
	ByActor.Reset();
	++Generation;
}
#pragma endregion

void FNarrativeStore::Reset(uint64 Seed)
{
	Campaign.Reset(Seed);

	// In id order, so the same views make the same campaign however the registry was filled.
	TArray<FNarrativeEntityId> Bound;
	Views.GetViews().GetKeys(Bound);
	Bound.Sort();
	for (const FNarrativeEntityId& Id : Bound)
	{
		const AActor* View = Views.FindView(Id);
		const UNarrativeEntityComponent* Entity = IsValid(View) ? View->FindComponentByClass<UNarrativeEntityComponent>() : nullptr;
		if (const UNarrativeEntityDef* Def = IsValid(Entity) ? Entity->EntityDef.Get() : nullptr)
		{
			Campaign.RegisterEntity(Id, *Def);
		}
	}
	UE_LOG(LogNarrativeTrace, Log, TEXT("Campaign reset from seed %llu; %d bound views registered again."), Seed, Bound.Num());
	OnReset.Broadcast();
}

void UNarrativeStoreSubsystem::ResetStore(const UObject* WorldContext, uint64 Seed)
{
	if (FNarrativeStore* Store = Find(WorldContext))
	{
		Store->Reset(Seed != 0 ? Seed : GetConfiguredSeed());
	}
}

FNarrativeStore* UNarrativeStoreSubsystem::Find(const UObject* WorldContext)
{
	return FindHandle(WorldContext).Get();
}

FNarrativeStoreHandle UNarrativeStoreSubsystem::FindHandle(const UObject* WorldContext)
{
	FNarrativeStoreHandle Handle;
	UWorld* World = IsValid(GEngine) ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!IsValid(World))
	{
		return Handle;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UNarrativeStoreSubsystem* Subsystem = GameInstance->GetSubsystem<UNarrativeStoreSubsystem>())
		{
			Handle.Owner = Subsystem;
			Handle.Store = &Subsystem->Store;
			return Handle;
		}
	}

	if (UNarrativeSubsystem* Narrative = World->GetSubsystem<UNarrativeSubsystem>(); IsValid(Narrative))
	{
		Handle.Owner = Narrative;
		Handle.Store = &Narrative->GetLocalStore();
	}
	return Handle;
}

uint64 UNarrativeStoreSubsystem::GetConfiguredSeed()
{
	return static_cast<uint64>(static_cast<uint32>(CVarNarrativeSeed.GetValueOnGameThread()));
}

void UNarrativeStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Store.Campaign.Reset(GetConfiguredSeed());
}

void UNarrativeStoreSubsystem::ResetCampaign(uint64 Seed)
{
	Store.Reset(Seed);
}
