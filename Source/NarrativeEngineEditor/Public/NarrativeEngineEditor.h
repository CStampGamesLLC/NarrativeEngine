#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNarrativeEngineEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	FDelegateHandle MenuStartupHandle;
	class IConsoleObject* PreviewMigrationCommand = nullptr;
	class IConsoleObject* MigrateCommand = nullptr;
};
