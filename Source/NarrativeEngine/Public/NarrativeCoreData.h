#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataMacros.h"

#include "NarrativeCoreData.generated.h"

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
	TArray<FAssetData> GetAssetData();
	TMultiMap<uint32, FAssetData> NarrativeAssetData;

	static int NumBasisVectors; 
};

template<typename T>
uint32 GetClassHash(){return 0;};

/* Common base for all narrative data table records */
UCLASS(Blueprintable, BlueprintType)
class NARRATIVEENGINE_API UNarrativeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	FName Name;

	UNarrativeDataAsset() : Super()
	{
		Name = FName{GetName()};
	}

	friend uint32 GetTypeHash(const UNarrativeDataAsset& InRecord) 
	{
		// I don't need this lol
		return GetTypeHash(InRecord.Name);
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

// cstamper todod - replace template specialization with macro'd static function overloads
template<>
inline uint32 GetClassHash<UNarrativeBasisVector>()
{
	return GetTypeHash(UNarrativeBasisVector::StaticClass());
}

/* N Dimensional vector*/
struct FVectorND : public TArray<float>
{
	FVectorND()
	{
		SetNum(UNarrativeBasisVector::Num());
	}
	
	const float& operator[](const UNarrativeBasisVector& BasisVector) const
	{
		const int Index = UNarrativeBasisVector::GetLoadedAssets().IndexOfByKey(&BasisVector);
		RangeCheck(Index);
		return GetData()[Index];
	}

	const float& operator[](const int& Index) const
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	float& operator[](const int& Index)
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	// Helper function to calculate the distance in n-dimensional space
	float Distance(const FVectorND& Other) const
	{
		float Sum = 0.0;
		for (int i = 0; i < Num(); ++i)
		{
			Sum += FMath::Pow(GetData()[i] - Other[i], 2);
		}

		return FMath::Sqrt(Sum);
	}

	float Dot(const FVectorND& Other) const
	{
	    float Result = 0.0;
	    for(int i = 0; i < Num(); ++i)
	    {
	        Result += GetData()[i] * Other[i];
	    }
	    return Result;
	}

	FVectorND operator*(double X) const
	{
		FVectorND Result = *this;
		for (int i = 0; i < Num(); ++i)
		{
			Result[i] *= X;
		}
		return Result;
	}
};