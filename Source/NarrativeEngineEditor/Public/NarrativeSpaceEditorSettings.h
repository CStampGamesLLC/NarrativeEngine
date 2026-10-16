#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "NarrativeCoreData.h"
#include "NarrativeSpaceEditorSettings.generated.h"

/** Where Make New files one class of narrative asset. */
USTRUCT()
struct NARRATIVEENGINEEDITOR_API FNarrativeCreationFolder
{
	GENERATED_BODY()

	/** Applies to this class and any class derived from it, unless a nearer ancestor has its own row. */
	UPROPERTY(EditAnywhere, Config, Category = "Narrative Space")
	TSoftClassPtr<UNarrativeDataAsset> Class;

	UPROPERTY(EditAnywhere, Config, Category = "Narrative Space", meta = (ContentDir, LongPackageName))
	FDirectoryPath Folder;
};

/** Project Settings -> Plugins -> Narrative Space. */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Narrative Space"))
class NARRATIVEENGINEEDITOR_API UNarrativeSpaceEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Make New files a new asset in the folder of its nearest listed class. A folder outside the
	 * space's Content Path is skipped, since the asset would vanish from the viewport it was made in.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Narrative Space")
	TArray<FNarrativeCreationFolder> CreationFolders;

	/** The listed folder for Class or its nearest listed ancestor, or empty when none is listed. */
	FString FindCreationFolder(const UClass* Class) const;

	/** Where Make New files a Class asset: its listed folder when under QueryPath, else QueryPath. */
	static FString ResolveCreationFolder(const UClass* Class, const FString& QueryPath);

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
