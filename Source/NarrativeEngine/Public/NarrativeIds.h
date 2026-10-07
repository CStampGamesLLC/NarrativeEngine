#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "NarrativeIds.generated.h"

/**
 * A campaign entity's durable identity (TDD Pattern 21), separate from its template. Placed
 * characters carry an authored persistent spawn GUID; runtime spawns allocate from the campaign's
 * namespace and a saved counter. Actors, widgets and inspectors bind to it and never stand for it.
 */
USTRUCT(BlueprintType)
struct NARRATIVEENGINE_API FNarrativeEntityId
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Narrative")
	FGuid Guid;

	FNarrativeEntityId() = default;
	explicit FNarrativeEntityId(const FGuid& InGuid) : Guid(InGuid) {}

	bool IsValid() const { return Guid.IsValid(); }

	/** The two 64-bit words every keyed draw and every canonical order reads, high word first. */
	uint64 High() const { return (static_cast<uint64>(Guid.A) << 32) | Guid.B; }
	uint64 Low() const { return (static_cast<uint64>(Guid.C) << 32) | Guid.D; }

	/** Canonical order: by (High, Low). State hashes and same-tick ties walk entities in it. */
	bool operator<(const FNarrativeEntityId& Other) const
	{
		return High() != Other.High() ? High() < Other.High() : Low() < Other.Low();
	}
	bool operator==(const FNarrativeEntityId& Other) const { return Guid == Other.Guid; }
	bool operator!=(const FNarrativeEntityId& Other) const { return Guid != Other.Guid; }

	friend uint32 GetTypeHash(const FNarrativeEntityId& Id) { return GetTypeHash(Id.Guid); }

	/** Short form for logs and the Mind panel. */
	FString ToShortString() const { return Guid.ToString(EGuidFormats::Digits).Left(8); }
	FString ToString() const { return Guid.ToString(EGuidFormats::DigitsWithHyphens); }

	/** A placed actor with no authored GUID: stable across runs because its path is. */
	static FNarrativeEntityId FromStablePath(const FString& Path) { return FNarrativeEntityId(FGuid::NewDeterministicGuid(Path)); }

	/** A runtime spawn: the campaign namespace and its saved counter. */
	static FNarrativeEntityId FromCounter(uint32 Namespace, uint64 Counter)
	{
		return FNarrativeEntityId(FGuid(Namespace, 0x52554E54u /* 'RUNT' */, static_cast<uint32>(Counter >> 32), static_cast<uint32>(Counter)));
	}

	/** Tests and fixtures: a readable id from a small number. */
	static FNarrativeEntityId FromIndex(uint32 Index) { return FNarrativeEntityId(FGuid(0x4E415252u /* 'NARR' */, 0, 0, Index)); }
};
