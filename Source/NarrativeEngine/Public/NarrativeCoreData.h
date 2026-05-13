#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataMacros.h"

#include "NarrativeCoreData.generated.h"

class UNarrativeDataAsset;
class UNarrativeBasisVector;

UCLASS()
class UNarrativeDataSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
	friend class UNarrativeBasisVector;

public:
	void RegisterNarrativeAssets(FAssetRegistryModule& AssetRegistryModule);
	void OnAssetRegistryReady();
	void InitializeNarrativeAssetData();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void RegisterAsset(UNarrativeDataAsset* Asset);

	template<typename T>
	TArray<FAssetData> GetAssetData()
	{
		TArray<FAssetData> CachedAssetData;
		const uint32 ClassHash = GetTypeHash(T::StaticClass());
		NarrativeAssetData.MultiFind(ClassHash, CachedAssetData);
		return CachedAssetData;
	}
	
	/*
	 * Hot-path-safe basis-vector cache.
	 *
	 * Built once RegisterNarrativeAssets has populated NarrativeAssetData, and
	 * rebuilt any time it re-runs. The macro-generated UNarrativeBasisVector::Num()
	 * and GetLoadedAssets() do a synchronous TryLoad sweep on every call, which
	 * makes them unsafe inside any per-tick code path (FVectorND ctor, operator[],
	 * VerletIntegrate, CalculateAcceleration). These caches give those paths an
	 * O(1) Num() and an O(1) basis-vector -> dimension index lookup.
	 *
	 * Order matches the dimension layout of every FVectorND in the simulation:
	 * stable for the lifetime of an asset-registry snapshot.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UNarrativeBasisVector>> CachedBasisVectors;
	
protected:
	TMultiMap<uint32, FAssetData> NarrativeAssetData;
	
	UPROPERTY()
	TSet<UNarrativeDataAsset*> RegisteredAssets;

	/*
	 * Reverse index: basis-vector pointer -> dimension index. O(1).
	 *
	 * Raw pointer keys are intentional - GC-tracking is already handled by the
	 * parallel CachedBasisVectors UPROPERTY array. Both are rebuilt together by
	 * RebuildBasisVectorCache, so the keys never outlive their objects.
	 */
	TMap<const UNarrativeBasisVector*, int32> CachedBasisVectorIndices;

	/* Snapshot CachedBasisVectors / CachedBasisVectorIndices from registered basis-vector assets. */
	void RebuildBasisVectorCache();
};

/* Common base for all narrative data table records */
UCLASS(Blueprintable, BlueprintType)
class NARRATIVEENGINE_API UNarrativeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual void PostLoad() override;
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

	/*
	 * Hot-path-safe cached accessors. Prefer these over Num() / GetLoadedAssets()
	 * inside any per-tick code: they read from UNarrativeDataSubsystem's snapshot
	 * (built when the asset registry settles) instead of doing a synchronous
	 * TryLoad sweep every call.
	 *
	 * They return 0 / empty / INDEX_NONE when the cache hasn't been built yet -
	 * callers must tolerate that case (the simulation simply has no dimensions
	 * until the cache is alive).
	 */
	static int32 CachedNum();
	static const TArray<TObjectPtr<UNarrativeBasisVector>>& GetCachedAssets();
	static int32 GetCachedIndex(const UNarrativeBasisVector* InBasisVector);

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
		// Hot-path: use cached basis-vector count instead of UNarrativeBasisVector::Num(),
		// which would do an asset-registry scan + multi-map find on every default ctor.
		SetNumZeroed(UNarrativeBasisVector::CachedNum());
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
		// Hot-path: O(1) cached lookup. The previous form did a TryLoad sweep and
		// O(N) IndexOfByKey on every index, which made even FVectorND[X] expensive.
		const int32 Index = UNarrativeBasisVector::GetCachedIndex(&BasisVector);
		RangeCheck(Index);
		return GetData()[Index];
	}

	float& operator[](const UNarrativeBasisVector& BasisVector)
	{
		const int32 Index = UNarrativeBasisVector::GetCachedIndex(&BasisVector);
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
