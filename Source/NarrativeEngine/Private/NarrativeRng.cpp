#include "NarrativeRng.h"

#include "UObject/SoftObjectPath.h"

uint64 NarrativeRng::Mix(uint64 X)
{
	X += 0x9E3779B97F4A7C15ull;
	X = (X ^ (X >> 30)) * 0xBF58476D1CE4E5B9ull;
	X = (X ^ (X >> 27)) * 0x94D049BB133111EBull;
	return X ^ (X >> 31);
}

uint64 NarrativeRng::KeyOf(const FNarrativeEntityId& Id)
{
	return Mix(Id.High() ^ Mix(Id.Low()));
}

uint64 NarrativeRng::AxisKey(const FSoftObjectPath& AxisPath)
{
	// Every decision draws per axis; the path's hash never changes, so remember it.
	thread_local TMap<FSoftObjectPath, uint64> Cache;
	if (const uint64* Known = Cache.Find(AxisPath))
	{
		return *Known;
	}

	const FTCHARToUTF8 Utf8(*AxisPath.ToString().ToLower());
	uint64 Hash = 0xCBF29CE484222325ull;
	for (int32 Index = 0; Index < Utf8.Length(); ++Index)
	{
		Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
		Hash *= 0x100000001B3ull;
	}
	Cache.Add(AxisPath, Hash);
	return Hash;
}

uint64 NarrativeRng::Bits(uint64 Seed, uint64 ObserverKey, uint64 SubjectKey, ENarrativeRngPurpose Purpose, uint64 Index, uint64 Lane)
{
	uint64 H = Mix(Seed);
	H = Mix(H ^ ObserverKey);
	H = Mix(H ^ SubjectKey);
	H = Mix(H ^ static_cast<uint64>(Purpose));
	H = Mix(H ^ Index);
	return Mix(H + Lane);
}

double NarrativeRng::UniformFromBits(uint64 InBits)
{
	// 53 mantissa bits, offset by half a unit so neither 0 nor 1 can come out.
	return (static_cast<double>(InBits >> 11) + 0.5) * (1.0 / 9007199254740992.0);
}

double NarrativeRng::FKey::Normal(uint64 K) const
{
	const double U1 = Uniform(2 * K);
	const double U2 = Uniform(2 * K + 1);
	return FMath::Sqrt(-2.0 * FMath::Loge(U1)) * FMath::Cos(2.0 * UE_DOUBLE_PI * U2);
}
