using UnrealBuildTool;

public class NarrativeEngineEditor : ModuleRules
{
    public NarrativeEngineEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Blutility", 
                "UMG",
                "CoreUObject",
                "Engine",
                "DeveloperSettings",
                "NarrativeEngine",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "InputCore",
                "ToolMenus",
                "WorkspaceMenuStructure",
                "AssetRegistry",
                "AssetTools",
                "GameplayTags",
                "Json"
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("EditorFramework");    
        }
    }
}
