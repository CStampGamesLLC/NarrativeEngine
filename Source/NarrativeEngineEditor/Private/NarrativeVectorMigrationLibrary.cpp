#include "NarrativeVectorMigrationLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "NarrativeCoreData.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogNarrativeVectorMigration, Log, All);

namespace
{
FString RunMigration(const FString& ContentPath, bool bSave)
{
	if (!GEditor || GEditor->PlayWorld)
	{
		UE_LOG(LogNarrativeVectorMigration, Warning, TEXT("Stop PIE before running vector migration."));
		return TEXT("Vector migration requires the editor with PIE stopped.");
	}
	if (!ContentPath.StartsWith(TEXT("/")) || ContentPath.Contains(TEXT("..")))
	{
		UE_LOG(LogNarrativeVectorMigration, Warning, TEXT("Use a mounted content path, for example /Game."));
		return TEXT("Use a mounted content path, for example /Game or /NarrativeEngine.");
	}
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	Registry.SearchAllAssets(true);
	TSet<FTopLevelAssetPath> NarrativeClasses;
	Registry.GetDerivedClassNames({UNarrativeDataAsset::StaticClass()->GetClassPathName()}, {}, NarrativeClasses);
	NarrativeClasses.Add(UNarrativeDataAsset::StaticClass()->GetClassPathName());
	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*ContentPath));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths = {UNarrativeDataAsset::StaticClass()->GetClassPathName(), UBlueprint::StaticClass()->GetClassPathName()};
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });

	FString Report;
	TSet<UPackage*> Candidates;
	TSet<UPackage*> Blocked;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	int32 ConvertedFields = 0;
	int32 Scanned = 0;
	bool bCancelled = false;
	FScopedSlowTask Progress(Assets.Num(), NSLOCTEXT("NarrativeVectorMigration", "Scan", "Validating narrative vectors"));
	Progress.MakeDialog(true);
	for (const FAssetData& Asset : Assets)
	{
		if (Progress.ShouldCancel()) { bCancelled = true; break; }
		Progress.EnterProgressFrame(1.f, FText::FromName(Asset.AssetName));
		const FString GeneratedClass = Asset.GetTagValueRef<FString>(FBlueprintTags::GeneratedClassPath);
		if (!GeneratedClass.IsEmpty() && !NarrativeClasses.Contains(
			FSoftObjectPath(FPackageName::ExportTextPathToObjectPath(GeneratedClass)).GetAssetPath())) { continue; }
		UObject* Object = Asset.GetAsset();
		if (!Object)
		{
			Report += FString::Printf(TEXT("ERROR: Could not load %s\n"), *Asset.GetObjectPathString());
			continue;
		}
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			Object = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
		}
		if (!Object || !Object->IsA<UNarrativeDataAsset>()) { continue; }
		KeepAlive.Emplace(Object);
		++Scanned;
		UPackage* Package = Object->GetOutermost();
		bool bNeedsSave = false;
		for (TFieldIterator<FStructProperty> It(Object->GetClass()); It; ++It)
		{
			if (It->Struct != FVectorND::StaticStruct()) { continue; }
			FVectorND* Vector = It->ContainerPtrToValuePtr<FVectorND>(Object);
			FString Error;
			if (!Vector->IsValid(&Error))
			{
				Blocked.Add(Package);
				Report += FString::Printf(TEXT("ERROR: %s.%s: %s\n"), *Object->GetPathName(), *It->GetName(), *Error);
			}
			for (const auto& Axis : Vector->GetBasis())
			{
				if (!Axis.LoadSynchronous())
				{
					Blocked.Add(Package);
					Report += FString::Printf(TEXT("ERROR: %s.%s has missing basis %s\n"),
						*Object->GetPathName(), *It->GetName(), *Axis.ToString());
				}
			}
			if (Vector->WasLoadedFromLegacyMap())
			{
				bNeedsSave = true;
				++ConvertedFields;
				Report += FString::Printf(TEXT("CONVERTED: %s.%s (%d axes)\n"), *Object->GetPathName(), *It->GetName(), Vector->Num());
			}
		}
		if (bNeedsSave)
		{
			Candidates.Add(Package);
			if (Package->IsDirty())
			{
				Blocked.Add(Package);
				Report += FString::Printf(TEXT("SKIP: %s is already dirty. Review and save its edits manually.\n"), *Package->GetName());
			}
		}
	}
	TArray<UPackage*> Packages;
	for (UPackage* Package : Candidates)
	{
		if (!Blocked.Contains(Package)) { Packages.Add(Package); }
	}
	Packages.Sort([](const UPackage& A, const UPackage& B) { return A.GetName() < B.GetName(); });
	Report += FString::Printf(TEXT("Scanned %d assets; %d legacy fields; %d packages ready; %d packages blocked.\n"),
		Scanned, ConvertedFields, Packages.Num(), Blocked.Num());
	if (bCancelled) { Report += TEXT("Cancelled. No packages saved.\n"); }
	else if (bSave && !Packages.IsEmpty())
	{
		// Compatibility loading deliberately does not dirty assets. The generic SavePackagesWithDialog
		// helper checks dirty state internally, so explicitly request clean packages here.
		const bool bSaved = FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, true) == FEditorFileUtils::PR_Success;
		Report += bSaved ? TEXT("Migration packages saved.\n") : TEXT("Save cancelled or failed; inspect the Output Log. Some packages may already have saved.\n");
		// Retain provenance until restart, including packages declined in the save dialog.
		// Re-saving a converted value within this session is harmless.
	}
	else { Report += TEXT("No packages saved.\n"); }
	UE_LOG(LogNarrativeVectorMigration, Display, TEXT("%s"), *Report);
	return Report;
}
}

FString UNarrativeVectorMigrationLibrary::PreviewVectorMigration(const FString& ContentPath) { return RunMigration(ContentPath, false); }
FString UNarrativeVectorMigrationLibrary::MigrateVectorAssets(const FString& ContentPath) { return RunMigration(ContentPath, true); }
