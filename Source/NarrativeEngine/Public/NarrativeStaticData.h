#pragma once

#include "CoreMinimal.h"
#include "NarrativeCoreData.h"
#include "NarrativeStaticData.generated.h"


UCLASS()
class NARRATIVEENGINE_API UNarrativeEventDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

	NARRATIVE_DATA_HELPERS(UNarrativeEventDef)
};

UCLASS()
class NARRATIVEENGINE_API UNarrativeEntityDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> StartingCoordinates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> PreviousCoordinates;

	/* How resistent this entity is to external forces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Mass = 1.f;

	/* Tendency to naturally progress along the entity's telos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Drift = 0.005f;

	/* Resistence to forces that oppose the entity's telos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Alignment = 1.f;

	/* Affects how quickly the entity comes to rest */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Damping = 2.f;

	NARRATIVE_DATA_HELPERS(UNarrativeEntityDef)
};

UCLASS()
class NARRATIVEENGINE_API UNarrativeActionDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> Force;

	NARRATIVE_DATA_HELPERS(UNarrativeActionDef)
};


/*
 * Emplace some phrase for characters to "say", as if this is one possible incoming thought that's simulated for a character. 
 * The radius is a sort of entropy field for simulating wave function collapse in the narrative state hilbert space 
 */
UCLASS()
class NARRATIVEENGINE_API UNarrativeDialogDef : public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	// todo it'd be cool to break this phrase into a grammer where data fed into the grammar structure is determined via wave function collapse
	/* What's actually said in its entirety */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Phrase;
	/* One word description that indicates the emotion or action associated with delivering this phrase*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

#pragma region Narrative Engine Encounter
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> Location;

	/* todo implment IEncounter interface for items that are selected via wave function collapse */
	// todo make this an array of forces (need an embedded structure)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> Force;

	/* Distribution of information (i.e. analog to energy) for an entity*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShannonEntropyRadius;

	/* Curvature imposed on the state space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RadialFalloffExponent;
#pragma endregion 
	
	NARRATIVE_DATA_HELPERS(UNarrativeDialogDef)
};

