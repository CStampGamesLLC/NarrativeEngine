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

	/** Find over entries held elsewhere (the index keeps them by asset path). */
	TArray<FNarrativeSearchResult> Find(TConstArrayView<const FNarrativeSearchEntry*> Entries, const FString& Query, int32 MaxResults = 100);

	/**
	 * How cleanly Term lands in Text: 1 for the whole field, 0.75 at a word start, 0.4 anywhere else, 0 for a miss.
	 * OutAt is where its best landing starts.
	 */
	float MatchQuality(const FString& Text, const FString& Term, int32& OutAt);

	/**
	 * Whether an edit can change what Index collects. Vectors and masks are never indexed, so moving a card in the
	 * space (the dialog tuner's Apply, a details-panel drag) needs no reindex. An event that names no property
	 * (undo, redo) is assumed to.
	 */
	bool AffectsIndex(const struct FPropertyChangedEvent& Event);

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
 * The search bar's index over every narrative asset in the project, plotted by the current query or not.
 *
 * Kept per asset and filled in bounded slices, never in one blocking pass: an edit re-queues only the asset it
 * touched (and nothing at all when it only moved the asset in the space), asset additions, removals and renames
 * only rescan the registry's list, and Pump indexes queued assets until its time budget runs out. A search runs
 * over whatever is indexed so far; the owner keeps pumping and re-running while IsIndexing().
 */
class FNarrativeSpaceSearchIndex
{
public:
	FNarrativeSpaceSearchIndex();
	~FNarrativeSpaceSearchIndex();

	/** Indexes queued assets for up to BudgetSeconds (at least one), then runs the query over every asset indexed so far. */
	TArray<FNarrativeSearchResult> Find(const FString& Query, int32 MaxResults = 100, double BudgetSeconds = 0.008);

	/** Indexes queued assets for up to BudgetSeconds, always at least one. True when it did any work. */
	bool Pump(double BudgetSeconds);

	/** True while assets wait to be (re)indexed, so a search may be missing some of them. */
	bool IsIndexing() const { return bRescan || !Pending.IsEmpty(); }
	int32 NumPending() const { return Pending.Num(); }

	/** True when the index changed since the last Find, or is still filling in, so open results may be out of date. */
	bool IsStale() const { return bChangedSinceFind || IsIndexing(); }

	/** Re-reads the registry's list and reindexes every asset. */
	void MarkStale();

private:
	TMap<FSoftObjectPath, FNarrativeSearchEntry> Entries;
	/** Assets to index or reindex, oldest first, and the same as a set so queueing stays cheap. */
	TArray<FSoftObjectPath> Pending;
	TSet<FSoftObjectPath> PendingSet;
	/** The registry's list of narrative assets must be re-read: something was added, removed or renamed. */
	bool bRescan = true;
	bool bChangedSinceFind = true;

	void Rescan();
	void Enqueue(const FSoftObjectPath& Path);
	void Reindex(const FSoftObjectPath& Path);
	void PropertyChanged(UObject* Object, struct FPropertyChangedEvent& Event);
	void AssetChanged(const FAssetData& Asset);
	void AssetRenamed(const FAssetData& Asset, const FString& OldPath);
	void ObjectsReplaced(const TMap<UObject*, UObject*>& Replacements);
};
