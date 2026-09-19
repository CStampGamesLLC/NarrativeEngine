#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataMacros.h"
#include "NarrativeVector.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/DataAsset.h"
#include "Engine/Engine.h"
#include "Subsystems/EngineSubsystem.h"

#include "NarrativeCoreData.generated.h"

class UNarrativeDataAsset;

UCLASS()
class UNarrativeDataSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	void RegisterNarrativeAssets(FAssetRegistryModule& AssetRegistryModule);
	void OnAssetRegistryReady();
	void InitializeNarrativeAssetData();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	template<typename T>
	TArray<FAssetData> GetAssetData()
	{
		TArray<FAssetData> CachedAssetData;
		const uint32 ClassHash = GetTypeHash(T::StaticClass());
		NarrativeAssetData.MultiFind(ClassHash, CachedAssetData);
		return CachedAssetData;
	}

	/**
	 * O(1) name lookup.  Returns the asset whose UObject FName matches InName,
	 * cast to T, or nullptr if not found or wrong type.
	 * The cache is populated in PinAsset so it is always in sync with
	 * LoadedNarrativeAssets.
	 */
	template<typename T>
	const T* GetAssetByName(FName InName) const
	{
		const TObjectPtr<UNarrativeDataAsset>* Found = NamedAssetCache.Find(InName);
		if (!Found)
		{
			return nullptr;
		}
		return Cast<T>(*Found);
	}

	/** Load a narrative asset and pin it against GC. Safe to call multiple times for the same asset. */
	void PinAsset(UNarrativeDataAsset* InAsset);

	TMultiMap<uint32, FAssetData> NarrativeAssetData;

	/** Strong-ref cache that prevents GC from collecting loaded narrative assets. */
	UPROPERTY()
	TArray<TObjectPtr<UNarrativeDataAsset>> LoadedNarrativeAssets;

	/** O(1) lookup from UObject FName → asset.  Kept in sync by PinAsset. */
	UPROPERTY()
	TMap<FName, TObjectPtr<UNarrativeDataAsset>> NamedAssetCache;

	static int NumBasisVectors; 
};

/* Common base for all narrative data table records */
UCLASS(Blueprintable, BlueprintType)
class NARRATIVEENGINE_API UNarrativeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	friend uint32 GetTypeHash(const UNarrativeDataAsset& InRecord) 
	{
		// I don't need this lol
		return GetTypeHash(InRecord.GetName());
	}
};

UCLASS(Blueprintable, BlueprintType)
class NARRATIVEENGINE_API UNarrativeBasisVector :  public UNarrativeDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintType)
	FText ArchetypeTitle;

	UPROPERTY(EditAnywhere, BlueprintType)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintType)
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> SubComponents;

#pragma region grammar
	UPROPERTY(EditAnywhere, BlueprintType)
	FText Verb;
	
	UPROPERTY(EditAnywhere, BlueprintType)
	FText Subject;
	
	UPROPERTY(EditAnywhere, BlueprintType)
	TArray<FText> Adjectives;
#pragma endregion grammar

	NARRATIVE_DATA_HELPERS(UNarrativeBasisVector)
};
