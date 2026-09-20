#include "NarrativeEngineEditor.h"

#include "HAL/IConsoleManager.h"
#include "NarrativeVectorMigrationLibrary.h"
#include "PropertyEditorModule.h"
#include "VectorNDCustomization.h"
#include "Framework/Docking/TabManager.h"
#include "SNarrativeSpaceEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FNarrativeEngineEditorModule"

void FNarrativeEngineEditorModule::StartupModule()
{
	if (!IsRunningCommandlet())
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("NarrativeSpace", FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
		{
			return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SNarrativeSpaceEditor)];
		}))
		.SetDisplayName(LOCTEXT("NarrativeSpace", "Narrative Space"))
		.SetTooltipText(LOCTEXT("NarrativeSpaceTip", "Visually author FVectorND data asset coordinates."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
		MenuStartupHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]
		{
			FToolMenuOwnerScoped Owner(this);
			UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
			Menu->FindOrAddSection("NarrativeEngine").AddMenuEntry("NarrativeSpace",
				LOCTEXT("OpenNarrativeSpace", "Narrative Space"), LOCTEXT("OpenNarrativeSpaceTip", "Open the narrative space asset editor."),
				FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([] { FGlobalTabmanager::Get()->TryInvokeTab(FName("NarrativeSpace")); })));
		}));
	}
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
	if (!IsRunningCommandlet())
	{
		UToolMenus::UnRegisterStartupCallback(MenuStartupHandle);
		UToolMenus::UnregisterOwner(this);
		if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FName("NarrativeSpace"))) { Tab->RequestCloseTab(); }
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("NarrativeSpace");
	}
	IConsoleManager::Get().UnregisterConsoleObject(PreviewMigrationCommand);
	IConsoleManager::Get().UnregisterConsoleObject(MigrateCommand);
	if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyEditor->UnregisterCustomPropertyTypeLayout("VectorND");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNarrativeEngineEditorModule, NarrativeEngineEditor)
