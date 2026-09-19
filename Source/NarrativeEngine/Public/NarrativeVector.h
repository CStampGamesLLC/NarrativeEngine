#pragma once

#include "CoreMinimal.h"
#include "Serialization/StructuredArchive.h"
#include "NarrativeVector.generated.h"

class UNarrativeBasisVector;
struct FPropertyTag;

/** Dense coordinates with a persistent identity for each axis. Missing axes mean zero.
 * Construction/serialization never queries the asset registry or loads basis assets.
 */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FVectorND
{
	GENERATED_BODY()

	static constexpr float DefaultSafeNormalTolerance = 1.e-8f;

	FVectorND() = default;
	explicit FVectorND(const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& InBasis, float Fill = 0.f);

	int32 Num() const { return Coordinates.Num(); }
	const TArray<float>& GetCoordinates() const { return Coordinates; }
	const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& GetBasis() const { return Basis; }
	float* GetData() { return Coordinates.GetData(); }
	const float* GetData() const { return Coordinates.GetData(); }
	float& operator[](int32 Index) { return Coordinates[Index]; }
	const float& operator[](int32 Index) const { return Coordinates[Index]; }
	float operator[](const UNarrativeBasisVector& Axis) const;
	float& operator[](const UNarrativeBasisVector& Axis);
	float GetCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const;
	void SetCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value);
	void RemoveAxis(int32 Index);
	void Fill(float Value);
	bool HasSameBasis(const FVectorND& Other) const { return Basis == Other.Basis; }
	bool IsValid(FString* OutError = nullptr) const;

	/** Reorder/zero-extend to the requested basis, retaining any additional authored axes. */
	FVectorND Rebased(const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& InBasis) const;
	float LengthSquared() const;
	float Length() const { return FMath::Sqrt(LengthSquared()); }
	FVectorND GetSafeNormal(float Tolerance = DefaultSafeNormalTolerance) const;
	float Distance(const FVectorND& Other) const;
	float Dot(const FVectorND& Other) const;
	FVectorND operator+(const FVectorND& Other) const;
	FVectorND operator-(const FVectorND& Other) const;
	FVectorND& operator+=(const FVectorND& Other);
	FVectorND& operator-=(const FVectorND& Other);
	FVectorND operator-() const { return *this * -1.f; }
	FVectorND operator*(float Scalar) const;
	FVectorND operator/(float Scalar) const;
	FVectorND& operator*=(float Scalar);
	FVectorND& operator/=(float Scalar);
	friend FVectorND operator*(float Scalar, const FVectorND& Vector) { return Vector * Scalar; }

	/** Compatibility reader only. New saves use the reflected dense arrays below. */
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
	bool WasLoadedFromLegacyMap() const { return bLoadedFromLegacyMap; }

	static FName CoordinatesPropertyName() { return GET_MEMBER_NAME_CHECKED(FVectorND, Coordinates); }

private:
	// These arrays are one representation, not a map plus a runtime cache.
	UPROPERTY(EditAnywhere, Category = "Vector", meta = (AllowPrivateAccess = "true"))
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> Basis;

	UPROPERTY(EditAnywhere, Category = "Vector", meta = (AllowPrivateAccess = "true"))
	TArray<float> Coordinates;

	// Session-only provenance for the explicit resave command. Not authored data.
	bool bLoadedFromLegacyMap = false;

	FVectorND& AddScaled(const FVectorND& Other, float Scale);
};

template<> struct TStructOpsTypeTraits<FVectorND> : TStructOpsTypeTraitsBase2<FVectorND>
{
	enum { WithStructuredSerializeFromMismatchedTag = true };
};
