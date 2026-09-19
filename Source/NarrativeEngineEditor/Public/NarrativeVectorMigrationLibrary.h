#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "NarrativeVectorMigrationLibrary.generated.h"

/** Callable from an Editor Utility Widget, or the Narrative.VectorND console commands. */
UCLASS()
class NARRATIVEENGINEEDITOR_API UNarrativeVectorMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Load and validate narrative assets below ContentPath; log a report without saving. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Migration", meta = (DevelopmentOnly))
	static FString PreviewVectorMigration(const FString& ContentPath = TEXT("/Game"));

	/** Validate, then offer to save converted packages. Dirty or invalid packages are skipped. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Migration", meta = (DevelopmentOnly))
	static FString MigrateVectorAssets(const FString& ContentPath = TEXT("/Game"));
};
