#pragma once

#define NARRATIVE_DATA_HELPERS(T)																												\
public:																																			\
static TArray<FAssetData> GetAssetData()                                                                                                        \
{                                                                                                                                               \
	UNarrativeDataSubsystem* DataSubsystem = GEngine->GetEngineSubsystem<UNarrativeDataSubsystem>();                                            \
	if (!IsValid(DataSubsystem)){return {};}                                                                                                    \
																																				\
	return DataSubsystem->GetAssetData<T>();																									\
}   																																			\
static TArray<TSoftObjectPtr<T>> GetLoadedAssets()                                                                                              \
{                                                                                                                                               \
	TArray<FAssetData> AssetData = GetAssetData();                                                                                              \
	TArray<TSoftObjectPtr<T>> LoadedAssets;                                                                                 					\
	for (const FAssetData& Data : AssetData)                                                                                                    \
	{                                                                                                                                           \
		T* LoadedObject = Cast<T>(Data.GetSoftObjectPath().TryLoad());                                                       \
		TSoftObjectPtr<T> LoadedAsset(LoadedObject);                                                                          \
		if (!LoadedAsset.IsValid())                                                                                                             \
		{                                                                                                                                       \
			continue;                                                                                                                           \
		}                                                                                                                                       \
				                                                                                                                                \
		LoadedAssets.Add(LoadedAsset);                                                                                                          \
	}                                                                                                                                           \
return LoadedAssets;																															\
}                                                                                                                                               \
																																				\
static int Num()                                                                                                                                \
{                                                                                                                                               \
	TArray<FAssetData> FoundAssets = GetAssetData();                                                                                            \
	return FoundAssets.Num();                                                                                                                   \
}                                                                                                                                               \
																																				\
operator int() const                                                                                                                            \
{                                                                                                                                               \
	TArray<FAssetData> FoundAssets = GetAssetData();                                                                                            \
	int i = 0;                                                                                                                                  \
	for (const FAssetData& FoundAsset : GetAssetData())                                                                                         \
	{                                                                                                                                           \
		TSoftObjectPtr<T> FoundT = Cast<T>(FoundAsset.GetSoftObjectPath().TryLoad());															\
		if (this == FoundT.Get())																												\
		{                                                                                                                                       \
			return i;                                                                                                                           \
		}                                                                                                                                       \
	}                                                                                                                                           \
																																				\
	return INDEX_NONE;																															\
}

#define REGISTER_NARRATIVE_ASSET_TYPE(T)										\
 /*2. Query for all assets of class UMyDataAsset*/								\
	TArray<FAssetData> T##FoundAssets;											\
 /*Using GetAssetsByClass directly*/											\
	AssetRegistryModule.Get().GetAssetsByClass(									\
	T::StaticClass()->GetClassPathName(),										\
	T##FoundAssets,																\
	true /* bSearchSubClasses */												\
);																				\
																				\
for (FAssetData FoundAsset : T##FoundAssets)									\
{																				\
	NarrativeAssetData.Add(GetTypeHash(T::StaticClass()), FoundAsset);			\
	/* Eagerly load and pin: TSoftObjectPtr returned by TryLoad has no other   */\
	/* owner, so we must root it here or GC will collect the asset.            */\
	PinAsset(Cast<UNarrativeDataAsset>(FoundAsset.GetSoftObjectPath().TryLoad()));\
}
