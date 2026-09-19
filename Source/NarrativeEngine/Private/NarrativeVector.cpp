#include "NarrativeVector.h"

#include "NarrativeCoreData.h"
#include "NarrativeVectorLegacy.h"
#include "UObject/PropertyTag.h"
#include "UObject/UnrealType.h"

FVectorND::FVectorND(const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& InBasis, float Value)
	: Basis(InBasis)
{
	Coordinates.Init(Value, Basis.Num());
	check(IsValid());
}

bool FVectorND::IsValid(FString* OutError) const
{
	auto Fail = [OutError](const TCHAR* Error)
	{
		if (OutError) { *OutError = Error; }
		return false;
	};
	if (Basis.Num() != Coordinates.Num()) { return Fail(TEXT("Basis and coordinate counts differ.")); }
	TSet<FSoftObjectPath> Seen;
	for (int32 Index = 0; Index < Num(); ++Index)
	{
		const FSoftObjectPath& Path = Basis[Index].ToSoftObjectPath();
		if (Path.IsNull()) { return Fail(TEXT("A coordinate has no basis asset.")); }
		if (Seen.Contains(Path)) { return Fail(TEXT("A basis asset appears more than once.")); }
		if (!FMath::IsFinite(Coordinates[Index])) { return Fail(TEXT("A coordinate is not finite.")); }
		Seen.Add(Path);
	}
	return true;
}

float FVectorND::GetCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis) const
{
	const int32 Index = Basis.IndexOfByKey(Axis);
	return Coordinates.IsValidIndex(Index) ? Coordinates[Index] : 0.f;
}

float FVectorND::operator[](const UNarrativeBasisVector& Axis) const
{
	return GetCoordinate(TSoftObjectPtr<UNarrativeBasisVector>(const_cast<UNarrativeBasisVector*>(&Axis)));
}

float& FVectorND::operator[](const UNarrativeBasisVector& Axis)
{
	const TSoftObjectPtr<UNarrativeBasisVector> Key(const_cast<UNarrativeBasisVector*>(&Axis));
	int32 Index = Basis.IndexOfByKey(Key);
	if (Index == INDEX_NONE)
	{
		Index = Basis.Add(Key);
		Coordinates.Add(0.f);
	}
	return Coordinates[Index];
}

void FVectorND::SetCoordinate(const TSoftObjectPtr<UNarrativeBasisVector>& Axis, float Value)
{
	check(!Axis.IsNull() && FMath::IsFinite(Value));
	const int32 Index = Basis.IndexOfByKey(Axis);
	if (Index == INDEX_NONE)
	{
		Basis.Add(Axis);
		Coordinates.Add(Value);
	}
	else { Coordinates[Index] = Value; }
}

void FVectorND::RemoveAxis(int32 Index)
{
	Basis.RemoveAt(Index);
	Coordinates.RemoveAt(Index);
}

void FVectorND::Fill(float Value)
{
	for (float& Coordinate : Coordinates) { Coordinate = Value; }
}

FVectorND FVectorND::Rebased(const TArray<TSoftObjectPtr<UNarrativeBasisVector>>& InBasis) const
{
	FVectorND Result(InBasis);
	for (int32 Index = 0; Index < Num(); ++Index)
	{
		Result.SetCoordinate(Basis[Index], Coordinates[Index]);
	}
	return Result;
}

float FVectorND::LengthSquared() const
{
	float Sum = 0.f;
	for (float Value : Coordinates) { Sum += Value * Value; }
	return Sum;
}

FVectorND FVectorND::GetSafeNormal(float Tolerance) const
{
	const float LenSq = LengthSquared();
	FVectorND Result(*this);
	if (LenSq > Tolerance) { Result *= FMath::InvSqrt(LenSq); }
	else { Result.Fill(0.f); }
	return Result;
}

float FVectorND::Distance(const FVectorND& Other) const
{
	return (*this - Other).Length();
}

float FVectorND::Dot(const FVectorND& Other) const
{
	float Sum = 0.f;
	const bool bSameBasis = HasSameBasis(Other);
	for (int32 Index = 0; Index < Num(); ++Index)
	{
		Sum += Coordinates[Index] * (bSameBasis ? Other[Index] : Other.GetCoordinate(Basis[Index]));
	}
	return Sum;
}

FVectorND& FVectorND::AddScaled(const FVectorND& Other, float Scale)
{
	if (HasSameBasis(Other))
	{
		for (int32 Index = 0; Index < Num(); ++Index) { Coordinates[Index] += Other[Index] * Scale; }
	}
	else
	{
		for (int32 Index = 0; Index < Other.Num(); ++Index)
		{
			const auto& Axis = Other.Basis[Index];
			SetCoordinate(Axis, GetCoordinate(Axis) + Other[Index] * Scale);
		}
	}
	return *this;
}

FVectorND FVectorND::operator+(const FVectorND& Other) const { FVectorND Result(*this); return Result += Other; }
FVectorND FVectorND::operator-(const FVectorND& Other) const { FVectorND Result(*this); return Result -= Other; }
FVectorND& FVectorND::operator+=(const FVectorND& Other) { return AddScaled(Other, 1.f); }
FVectorND& FVectorND::operator-=(const FVectorND& Other) { return AddScaled(Other, -1.f); }
FVectorND FVectorND::operator*(float Scalar) const { FVectorND Result(*this); return Result *= Scalar; }
FVectorND FVectorND::operator/(float Scalar) const { FVectorND Result(*this); return Result /= Scalar; }
FVectorND& FVectorND::operator*=(float Scalar)
{
	for (float& Value : Coordinates) { Value *= Scalar; }
	return *this;
}
FVectorND& FVectorND::operator/=(float Scalar)
{
	check(!FMath::IsNearlyZero(Scalar));
	return *this *= 1.f / Scalar;
}

bool FVectorND::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	if (Tag.Type != NAME_MapProperty || Tag.GetType().GetParameterName(0) != NAME_SoftObjectProperty ||
		Tag.GetType().GetParameterName(1) != NAME_FloatProperty)
	{
		return false;
	}

	const FMapProperty* LegacyProperty = FindFProperty<FMapProperty>(
		FNarrativeVectorLegacy::StaticStruct(), GET_MEMBER_NAME_CHECKED(FNarrativeVectorLegacy, Components));
	// The destination already contains its archetype's values. Reconstruct those as the
	// map defaults so inherited unchanged entries and explicit removals survive conversion.
	FNarrativeVectorLegacy Defaults;
	for (int32 Index = 0; Index < Num(); ++Index) { Defaults.Components.Add(Basis[Index], Coordinates[Index]); }
	FNarrativeVectorLegacy Loaded;
	LegacyProperty->SerializeItem(Slot, &Loaded.Components, &Defaults.Components);
	if (Slot.GetUnderlyingArchive().IsError()) { return false; }

	Basis.Reset();
	Coordinates.Reset();
	Loaded.Components.GenerateKeyArray(Basis);
	Basis.Sort([](const auto& A, const auto& B) { return A.ToSoftObjectPath().ToString() < B.ToSoftObjectPath().ToString(); });
	for (const auto& Axis : Basis) { Coordinates.Add(Loaded.Components.FindChecked(Axis)); }
	// Preserve unresolved/null references verbatim; the migration validator reports them.
	bLoadedFromLegacyMap = true;
	return true;
}
