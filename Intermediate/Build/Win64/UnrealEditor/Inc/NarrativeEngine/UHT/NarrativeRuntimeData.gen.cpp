// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeRuntimeData.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeRuntimeData() {}

// ********** Begin Cross Module References ********************************************************
NARRATIVEENGINE_API UScriptStruct* Z_Construct_UScriptStruct_FNarrativeComponent();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FNarrativeComponent ***********************************************
struct Z_Construct_UScriptStruct_FNarrativeComponent_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FNarrativeComponent); }
	static inline consteval int16 GetStructAlignment() { return alignof(FNarrativeComponent); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/NarrativeRuntimeData.h" },
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FNarrativeComponent constinit property declarations ***************
// ********** End ScriptStruct FNarrativeComponent constinit property declarations *****************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FNarrativeComponent>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FNarrativeComponent_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FNarrativeComponent;
class UScriptStruct* FNarrativeComponent::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FNarrativeComponent.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FNarrativeComponent.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FNarrativeComponent, (UObject*)Z_Construct_UPackage__Script_NarrativeEngine(), TEXT("NarrativeComponent"));
	}
	return Z_Registration_Info_UScriptStruct_FNarrativeComponent.OuterSingleton;
	}
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FNarrativeComponent_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
	nullptr,
	&NewStructOps,
	"NarrativeComponent",
	nullptr,
	0,
	sizeof(FNarrativeComponent),
	alignof(FNarrativeComponent),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000001),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FNarrativeComponent_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FNarrativeComponent_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FNarrativeComponent()
{
	if (!Z_Registration_Info_UScriptStruct_FNarrativeComponent.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FNarrativeComponent.InnerSingleton, Z_Construct_UScriptStruct_FNarrativeComponent_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FNarrativeComponent.InnerSingleton);
}
// ********** End ScriptStruct FNarrativeComponent *************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeRuntimeData_h__Script_NarrativeEngine_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FNarrativeComponent::StaticStruct, Z_Construct_UScriptStruct_FNarrativeComponent_Statics::NewStructOps, TEXT("NarrativeComponent"),&Z_Registration_Info_UScriptStruct_FNarrativeComponent, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FNarrativeComponent), 2519244563U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeRuntimeData_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeRuntimeData_h__Script_NarrativeEngine_253285682{
	TEXT("/Script/NarrativeEngine"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeRuntimeData_h__Script_NarrativeEngine_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeRuntimeData_h__Script_NarrativeEngine_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
