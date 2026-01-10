// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeNarrativeEngine_init() {}
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_NarrativeEngine;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_NarrativeEngine()
	{
		if (!Z_Registration_Info_UPackage__Script_NarrativeEngine.OuterSingleton)
		{
		static const UECodeGen_Private::FPackageParams PackageParams = {
			"/Script/NarrativeEngine",
			nullptr,
			0,
			PKG_CompiledIn | 0x00000000,
			0x6E1B6089,
			0xDD85F88C,
			METADATA_PARAMS(0, nullptr)
		};
		UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_NarrativeEngine.OuterSingleton, PackageParams);
	}
	return Z_Registration_Info_UPackage__Script_NarrativeEngine.OuterSingleton;
}
static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_NarrativeEngine(Z_Construct_UPackage__Script_NarrativeEngine, TEXT("/Script/NarrativeEngine"), Z_Registration_Info_UPackage__Script_NarrativeEngine, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x6E1B6089, 0xDD85F88C));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
