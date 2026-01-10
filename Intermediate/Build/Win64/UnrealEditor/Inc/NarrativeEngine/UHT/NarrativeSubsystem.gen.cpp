// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeSubsystem.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeSubsystem() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UTickableWorldSubsystem();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeSubsystem();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeSubsystem_NoRegister();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeSubsystem ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeSubsystem;
UClass* UNarrativeSubsystem::GetPrivateStaticClass()
{
	using TClass = UNarrativeSubsystem;
	if (!Z_Registration_Info_UClass_UNarrativeSubsystem.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeSubsystem"),
			Z_Registration_Info_UClass_UNarrativeSubsystem.InnerSingleton,
			StaticRegisterNativesUNarrativeSubsystem,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UNarrativeSubsystem.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeSubsystem_NoRegister()
{
	return UNarrativeSubsystem::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**  */" },
#endif
		{ "IncludePath", "NarrativeSubsystem.h" },
		{ "ModuleRelativePath", "Public/NarrativeSubsystem.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeSubsystem constinit property declarations **********************
// ********** End Class UNarrativeSubsystem constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeSubsystem_Statics
UObject* (*const Z_Construct_UClass_UNarrativeSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UTickableWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeSubsystem_Statics::ClassParams = {
	&UNarrativeSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeSubsystem_Statics::Class_MetaDataParams)
};
void UNarrativeSubsystem::StaticRegisterNativesUNarrativeSubsystem()
{
}
UClass* Z_Construct_UClass_UNarrativeSubsystem()
{
	if (!Z_Registration_Info_UClass_UNarrativeSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeSubsystem.OuterSingleton, Z_Construct_UClass_UNarrativeSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeSubsystem.OuterSingleton;
}
UNarrativeSubsystem::UNarrativeSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeSubsystem);
UNarrativeSubsystem::~UNarrativeSubsystem() {}
// ********** End Class UNarrativeSubsystem ********************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h__Script_NarrativeEngine_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeSubsystem, UNarrativeSubsystem::StaticClass, TEXT("UNarrativeSubsystem"), &Z_Registration_Info_UClass_UNarrativeSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeSubsystem), 2358893576U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h__Script_NarrativeEngine_703190443{
	TEXT("/Script/NarrativeEngine"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h__Script_NarrativeEngine_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h__Script_NarrativeEngine_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
