#pragma once

#include "CoreMinimal.h"
#include "NarrativeCoreData.h"

struct FAssetData;
struct FNarrativeMask;
struct FVectorND;

/** One searchable string pulled off a narrative asset, and the field it came from. */
struct FNarrativeSearchField
{
	/** The details-panel name of the field, or a pseudo field such as Name, Type or Folder. */
	FText Field;
	FString Text;
	/** How much a hit here counts for. A name outranks a phrase, which outranks a folder. */
	float Weight = 1.f;
};

/** Everything the search bar can find one asset by. */
struct FNarrativeSearchEntry
{
	TWeakObjectPtr<UNarrativeDataAsset> Asset;
	FString Name;
	FText Type;
	TArray<FNarrativeSearchField> Fields;
};

/** One asset matching a search, with the field that best explains why. */
struct FNarrativeSearchResult
{
	TWeakObjectPtr<UNarrativeDataAsset> Asset;
	FString Name;
	FText Type;
	float Score = 0.f;
	/** The best match outside the asset name, so a row can say why it is here. Empty when only the name matched. */
	FText Field;
	FString Snippet;
	/** The search term found in Snippet, which the row highlights. */
	FString Term;
};

namespace NarrativeSpaceSearch
{
	/**
	 * Collects every string an asset can be found by: its name, space label, type and folder, then
	 * every edited text, string, name, gameplay tag and asset reference it holds, through structs,
	 * arrays, sets and maps. Narrative vectors and masks are skipped: their basis references are the
	 * space itself, and would match every asset placed on a searched axis.
	 */
	FNarrativeSearchEntry Index(const UNarrativeDataAsset& Asset);

	/**
	 * Matches a query against indexed entries. Whitespace splits the query into terms, and every term
	 * has to match somewhere on the asset, case-insensitively. Results are ranked best first: by the
	 * weight of the fields hit, then by how cleanly each term lands (whole field, word start, anywhere).
	 */
	TArray<FNarrativeSearchResult> Find(const TArray<FNarrativeSearchEntry>& Entries, const FString& Query, int32 MaxResults = 100);

	/** A window of Text around a match, trimmed with ellipses, so a long phrase still fits a row. */
	FString Snippet(const FString& Text, int32 MatchStart, int32 MatchLength, int32 Context = 40);

	/**
	 * The Count axes that best describe a vector: the axes its mask is about first, then whichever
	 * have the largest coordinates, then basis order. Only axes the vector defines are offered, so
	 * the asset is never plotted as missing one.
	 */
	TArray<TSoftObjectPtr<UNarrativeBasisVector>> RelevantAxes(const FVectorND& Value, const FNarrativeMask* Mask, int32 Count = 2);

	/**
	 * The valid vector the space editor plots Asset by: Field when the asset has it, its class
	 * default otherwise. OutField names the one used. Null when the asset has no position at all.
	 */
	const FVectorND* PlacementVector(const UNarrativeDataAsset& Asset, FName Field, FName* OutField = nullptr);

	/** The mask the asset declares through GetSpaceMaskProperty, or null without one. */
	const FNarrativeMask* SpaceMask(const UNarrativeDataAsset& Asset);
}

/**
 * The search bar's index over every narrative asset in the project, plotted by the current query
 * or not. Rebuilt lazily: asset and property changes only mark it stale.
 */
class FNarrativeSpaceSearchIndex
{
public:
	FNarrativeSpaceSearchIndex();
	~FNarrativeSpaceSearchIndex();

	/** Rebuilds the index if anything changed since it was built, then runs the query against it. */
	TArray<FNarrativeSearchResult> Find(const FString& Query, int32 MaxResults = 100);

	/** True when an asset changed since the last build, so open results may be out of date. */
	bool IsStale() const { return bStale; }
	void MarkStale() { bStale = true; }

private:
	TArray<FNarrativeSearchEntry> Entries;
	bool bStale = true;

	void Rebuild();
	void PropertyChanged(UObject* Object, struct FPropertyChangedEvent& Event);
	void AssetChanged(const FAssetData& Asset) { bStale = true; }
	void AssetRenamed(const FAssetData& Asset, const FString& OldPath) { bStale = true; }
	void ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements) { bStale = true; }
};
