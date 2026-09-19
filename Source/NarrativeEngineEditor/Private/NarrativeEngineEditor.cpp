#include "NarrativeEngineEditor.h"

#include "HAL/IConsoleManager.h"
#include "NarrativeVectorMigrationLibrary.h"
#include "PropertyEditorModule.h"
#include "VectorNDCustomization.h"

#define LOCTEXT_NAMESPACE "FNarrativeEngineEditorModule"

void FNarrativeEngineEditorModule::StartupModule()
{
    FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").RegisterCustomPropertyTypeLayout(
        "VectorND", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVectorNDCustomization::MakeInstance));
    PreviewMigrationCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Narrative.VectorND.PreviewMigration"), TEXT("Validate legacy narrative vectors. Optional content path (default /Game). No saves."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UNarrativeVectorMigrationLibrary::PreviewVectorMigration(Args.IsEmpty() ? TEXT("/Game") : Args[0]);
        }), ECVF_Default);
    MigrateCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Narrative.VectorND.Migrate"), TEXT("Validate and offer to save converted narrative assets. Optional content path (default /Game)."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            UNarrativeVectorMigrationLibrary::MigrateVectorAssets(Args.IsEmpty() ? TEXT("/Game") : Args[0]);
        }), ECVF_Default);
}

void FNarrativeEngineEditorModule::ShutdownModule()
{
    IConsoleManager::Get().UnregisterConsoleObject(PreviewMigrationCommand);
    IConsoleManager::Get().UnregisterConsoleObject(MigrateCommand);
    if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
    {
        PropertyEditor->UnregisterCustomPropertyTypeLayout("VectorND");
    }
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FNarrativeEngineEditorModule, NarrativeEngineEditor)
