#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataMacros.h"

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

	/** Load a narrative asset and pin it against GC. Safe to call multiple times for the same asset. */
	void PinAsset(UNarrativeDataAsset* InAsset);

	TMultiMap<uint32, FAssetData> NarrativeAssetData;

	/** Strong-ref cache that prevents GC from collecting loaded narrative assets. */
	UPROPERTY()
	TArray<TObjectPtr<UNarrativeDataAsset>> LoadedNarrativeAssets;

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

// cstamper todod - replace template specialization with macro'd static function overloads

/* N Dimensional vector */
struct FVectorND : public TArray<float>
{
	// Match FVector::GetSafeNormal default-ish tolerance (but in squared-space).
	static constexpr float DefaultSafeNormalTolerance = 1.e-8f;

	FVectorND()
	{
		SetNumZeroed(UNarrativeBasisVector::Num());
	}

	explicit FVectorND(int32 InNum)
	{
		SetNumZeroed(InNum);
	}

	explicit FVectorND(float InMagnitude) : FVectorND()
	{
		float* RESTRICT Data = GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			Data[i] = InMagnitude;
		}
	}

	// cstamper todo - replace this with something better... maybe a UHT plugin. 
	/* These component maps are how FVectorND's are serialized to editor. */
	explicit FVectorND(const TMap<TSoftObjectPtr<UNarrativeBasisVector>, float>& ComponentMap) : FVectorND()
	{
		for (const TTuple<TSoftObjectPtr<UNarrativeBasisVector>, float>& ComponentPair : ComponentMap)
		{
			UNarrativeBasisVector* BasisVector = ComponentPair.Key.Get();
			if (!IsValid(BasisVector))
			{
				continue;
			}
			
			(*this)[*BasisVector] = ComponentPair.Value;
		}
	}

	// --- Indexing ------------------------------------------------------------

	const float& operator[](const UNarrativeBasisVector& BasisVector) const
	{
		const int32 Index = UNarrativeBasisVector::GetLoadedAssets().IndexOfByKey(&BasisVector);
		RangeCheck(Index);
		return GetData()[Index];
	}

	float& operator[](const UNarrativeBasisVector& BasisVector)
	{
		const int32 Index = UNarrativeBasisVector::GetLoadedAssets().IndexOfByKey(&BasisVector);
		RangeCheck(Index);
		return GetData()[Index];
	}

	const float& operator[](int32 Index) const
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	float& operator[](int32 Index)
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	// --- Metrics -------------------------------------------------------------

	float LengthSquared() const
	{
		float Sum = 0.0f;
		const float* RESTRICT Data = GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			Sum += Data[i] * Data[i];
		}
		return Sum;
	}

	float Length() const
	{
		return FMath::Sqrt(LengthSquared());
	}

	FVectorND GetSafeNormal(float Tolerance = DefaultSafeNormalTolerance) const
	{
		const float LenSq = LengthSquared();
		if (LenSq > Tolerance)
		{
			const float InvLen = FMath::InvSqrt(LenSq);
			return (*this) * InvLen;
		}

		FVectorND Zero(Num());
		// already zeroed by ctor
		return Zero;
	}

	// --- Distance / Dot ------------------------------------------------------

	float Distance(const FVectorND& Other) const
	{
		check(Num() == Other.Num());

		float Sum = 0.0f;
		const float* RESTRICT A = GetData();
		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			const float D = A[i] - B[i];
			Sum += D * D;
		}
		return FMath::Sqrt(Sum);
	}

	float Dot(const FVectorND& Other) const
	{
		check(Num() == Other.Num());

		float Result = 0.0f;
		const float* RESTRICT A = GetData();
		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			Result += A[i] * B[i];
		}
		return Result;
	}

	// --- Arithmetic: vector-vector ------------------------------------------

	FVectorND operator+(const FVectorND& Other) const
	{
		check(Num() == Other.Num());

		FVectorND Result(*this);
		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			Result[i] += B[i];
		}
		return Result;
	}

	FVectorND operator-(const FVectorND& Other) const
	{
		check(Num() == Other.Num());

		FVectorND Result(*this);
		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			Result[i] -= B[i];
		}
		return Result;
	}

	FVectorND& operator+=(const FVectorND& Other)
	{
		check(Num() == Other.Num());

		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			GetData()[i] += B[i];
		}
		return *this;
	}

	FVectorND& operator-=(const FVectorND& Other)
	{
		check(Num() == Other.Num());

		const float* RESTRICT B = Other.GetData();
		for (int32 i = 0; i < Num(); ++i)
		{
			GetData()[i] -= B[i];
		}
		return *this;
	}

	FVectorND operator-() const
	{
		FVectorND Result(*this);
		for (int32 i = 0; i < Num(); ++i)
		{
			Result[i] = -Result[i];
		}
		return Result;
	}

	// --- Arithmetic: vector-scalar ------------------------------------------

	FVectorND operator*(float Scalar) const
	{
		FVectorND Result(*this);
		for (int32 i = 0; i < Num(); ++i)
		{
			Result[i] *= Scalar;
		}
		return Result;
	}

	FVectorND operator/(float Scalar) const
	{
		check(!FMath::IsNearlyZero(Scalar));

		const float Inv = 1.0f / Scalar;
		return (*this) * Inv;
	}

	FVectorND& operator*=(float Scalar)
	{
		for (int32 i = 0; i < Num(); ++i)
		{
			GetData()[i] *= Scalar;
		}
		return *this;
	}

	FVectorND& operator/=(float Scalar)
	{
		check(!FMath::IsNearlyZero(Scalar));

		const float Inv = 1.0f / Scalar;
		return (*this) *= Inv;
	}

	friend FVectorND operator*(float Scalar, const FVectorND& V)
	{
		return V * Scalar;
	}
};
