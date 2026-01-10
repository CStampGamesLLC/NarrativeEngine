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
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "NarrativeEngine"
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("EditorFramework");    
        }
    }
}