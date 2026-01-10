#pragma once

#include "CoreMinimal.h"
#include "NarrativeCoreData.h"
#include "NarrativeStaticData.generated.h"


UCLASS()
class NARRATIVEENGINE_API UNarrativeEventDef :  public UNarrativeDataAsset
{
	GENERATED_BODY()

	NARRATIVE_DATA_HELPERS(UNarrativeEventDef)
};

template<>
	inline uint32 GetClassHash<UNarrativeEventDef>()
{
	return GetTypeHash(UNarrativeEventDef::StaticClass());
}

UCLASS()
class NARRATIVEENGINE_API UNarrativeEntityDef :  public UNarrativeDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> StartingCoordinates;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> PreviousCoordinates;
	
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
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Mass;

	NARRATIVE_DATA_HELPERS(UNarrativeEntityDef)
};

template<>
inline uint32 GetClassHash<UNarrativeEntityDef>()
{
	return GetTypeHash(UNarrativeEntityDef::StaticClass());
}

UCLASS()
class NARRATIVEENGINE_API UNarrativeActionDef :  public UNarrativeDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSoftObjectPtr<UNarrativeBasisVector>, float> Force;
	
	NARRATIVE_DATA_HELPERS(UNarrativeActionDef)
};

template<>
inline uint32 GetClassHash<UNarrativeActionDef>()
{
	return GetTypeHash(UNarrativeActionDef::StaticClass());
}


/*
 * Emplace some phrase for characters to "say", as if this is one possible incoming thought that's simulated for a character. 
 * The radius is a sort of entropy field for simulating wave function collapse in the narrative state hilbert space 
 */
UCLASS()
class NARRATIVEENGINE_API UNarrativeDialogDef :  public UNarrativeDataAsset
{
	GENERATED_BODY()

public:

	// todo it'd be cool to break this phrase into a grammer where data fed into the grammar structure is determined via wave function collapse
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Phrase;

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

template<>
inline uint32 GetClassHash<UNarrativeDialogDef>()
{
	return GetTypeHash(UNarrativeActionDef::StaticClass());
}
