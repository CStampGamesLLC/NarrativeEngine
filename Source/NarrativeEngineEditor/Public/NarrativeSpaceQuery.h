#pragma once

#include "CoreMinimal.h"
#include "NarrativeCoreData.h"
#include "NarrativeSpaceQuery.generated.h"

/** What the viewport does with assets that leave one of the plotted axes undefined. */
UENUM()
enum class ENarrativeSpaceIncomplete : uint8
{
	/** Drawn like any other entry; the hollow axis markers still flag the missing axis. */
	Show,
	/** Drawn faded, but still selectable and draggable so the missing axis can be authored. */
	Dim,
	/** Left out of the viewport, and out of framing and hit testing with it. */
	Hide
};

/** Editor-only authoring filter; axis order is X, Y, Z. */
USTRUCT(BlueprintType)
struct NARRATIVEENGINEEDITOR_API FNarrativeSpaceQuery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Space")
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> Axes;

	/** Empty means all narrative data asset classes. Derived classes are included. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Space")
	TArray<TSubclassOf<UNarrativeDataAsset>> Classes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative Space")
	FString ContentPath = TEXT("/Game");

	/** None uses each class's default. An explicit field never falls back to another field. */
	UPROPERTY(BlueprintReadWrite, Category = "Narrative Space")
	FName PlacementField = NAME_None;

	bool Validate(FText& OutError) const;
};

/**
 * Transacted stand-in for package contents, so Make New can be undone. UE can restore an object's
 * saved state but cannot un-create one, and a package has no transacted contents array the way a
 * level has ULevel::Actors. Undo therefore drops the asset from this array and the model
 * unregisters it; redo puts it back and the model registers it again.
 */
UCLASS(Transient)
class NARRATIVEENGINEEDITOR_API UNarrativeSpaceCreationRecord : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<TObjectPtr<UNarrativeDataAsset>> Created;
};

/** Transient details-panel adapter, never saved into game content. */
UCLASS(Transient)
class NARRATIVEENGINEEDITOR_API UNarrativeSpaceSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Narrative Space", meta = (ShowOnlyInnerProperties))
	FNarrativeSpaceQuery Query;

	/** Display only: does not change which assets the query matches, or any asset value. */
	UPROPERTY(EditAnywhere, Category = "Narrative Space", meta = (DisplayName = "Missing An Axis"))
	ENarrativeSpaceIncomplete Incomplete = ENarrativeSpaceIncomplete::Dim;
};
