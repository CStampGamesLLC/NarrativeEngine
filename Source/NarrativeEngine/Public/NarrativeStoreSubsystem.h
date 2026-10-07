#pragma once

#include "CoreMinimal.h"
#include "NarrativeCampaign.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/ObjectKey.h"
#include "NarrativeStoreSubsystem.generated.h"

class AActor;

/**
 * Weak bindings between entities and the actors that show them (TDD Pattern 21). Detaching a view
 * never changes narrative state; streaming, travel and EndPlay only detach.
 */
struct NARRATIVEENGINE_API FNarrativeViewRegistry
{
	/** False when another live actor already shows Id: a duplicate placement, rejected rather than merged. */
	bool Bind(const FNarrativeEntityId& Id, AActor& View);
	void Unbind(const FNarrativeEntityId& Id, const AActor& View);

	AActor* FindView(const FNarrativeEntityId& Id) const;
	FNarrativeEntityId FindEntity(const AActor* View) const;

	const TMap<FNarrativeEntityId, TWeakObjectPtr<AActor>>& GetViews() const { return ByEntity; }
	void Reset();

	/** Bumped by every bind and unbind, so presentation can tell a view came or went. */
	uint32 GetGeneration() const { return Generation; }

private:
	uint32 Generation = 0;
	TMap<FNarrativeEntityId, TWeakObjectPtr<AActor>> ByEntity;
	TMap<TObjectKey<AActor>, FNarrativeEntityId> ByActor;
};

/** A campaign and the views bound to it: what the game reads narrative truth from. */
struct NARRATIVEENGINE_API FNarrativeStore
{
	FNarrativeCampaign Campaign;
	FNarrativeViewRegistry Views;

	/* After a reset: whatever the game mirrors into the campaign (factions, pins) must say it again. */
	FSimpleMulticastDelegate OnReset;

	/** The entity Actor shows, or an invalid id. */
	FNarrativeEntityId FindEntity(const AActor* Actor) const { return Views.FindEntity(Actor); }

	/**
	 * A fresh campaign from Seed (new game, or a gym fixture's boundary): the clock at zero, every mind
	 * forgotten, runtime ids counting again from the seed's namespace. Views stay bound, and what they
	 * show registers again from its definition, as it did at BeginPlay.
	 */
	void Reset(uint64 Seed);
};

/**
 * A store and the object that owns it, for holding a store across frames: the store lives exactly as
 * long as its owner, so the handle yields nothing once the owner is gone.
 */
struct FNarrativeStoreHandle
{
	TWeakObjectPtr<const UObject> Owner;
	FNarrativeStore* Store = nullptr;

	/** The store, or null once its owner is gone. */
	FNarrativeStore* Get() const { return Owner.IsValid() ? Store : nullptr; }
	void Reset() { Owner.Reset(); Store = nullptr; }
};

/**
 * The campaign store (TDD Pattern 21): narrative truth with a game-instance lifetime, so travel
 * keeps it and a new game resets it. Worlds without a game instance (automation) get their own
 * store from UNarrativeSubsystem, through the same Find.
 */
UCLASS()
class NARRATIVEENGINE_API UNarrativeStoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The store the world of WorldContext reads: its game instance's, else the world's own. */
	static FNarrativeStore* Find(const UObject* WorldContext);

	/** As Find, with the store's owner: what to keep when the store is held past the current call. */
	static FNarrativeStoreHandle FindHandle(const UObject* WorldContext);

	/** Resets the store WorldContext reads, from Seed, or from db.Narrative.Seed when Seed is 0. */
	static void ResetStore(const UObject* WorldContext, uint64 Seed = 0);

	/** The seed new campaigns start from: db.Narrative.Seed. */
	static uint64 GetConfiguredSeed();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** New game: a fresh campaign. Views stay bound; their entities register again as they play. */
	void ResetCampaign(uint64 Seed);

	FNarrativeStore& GetStore() { return Store; }

private:
	FNarrativeStore Store;
};
