// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeEntityComponent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeEntityComponent() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEntityComponent();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEntityComponent_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEntityDef_NoRegister();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeEntityComponent ************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEntityComponent;
UClass* UNarrativeEntityComponent::GetPrivateStaticClass()
{
	using TClass = UNarrativeEntityComponent;
	if (!Z_Registration_Info_UClass_UNarrativeEntityComponent.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEntityComponent"),
			Z_Registration_Info_UClass_UNarrativeEntityComponent.InnerSingleton,
			StaticRegisterNativesUNarrativeEntityComponent,
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
	return Z_Registration_Info_UClass_UNarrativeEntityComponent.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEntityComponent_NoRegister()
{
	return UNarrativeEntityComponent::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEntityComponent_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Custom" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Registers an actor to a narrative \"proxy entity\", which gives them a narrative state-space vector representation */" },
#endif
		{ "IncludePath", "NarrativeEntityComponent.h" },
		{ "ModuleRelativePath", "Public/NarrativeEntityComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Registers an actor to a narrative \"proxy entity\", which gives them a narrative state-space vector representation" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EntityDef_MetaData[] = {
		{ "Category", "NarrativeEntityComponent" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* The static data record that defines this entities properties when initializing the runtime */" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeEntityComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The static data record that defines this entities properties when initializing the runtime" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEntityComponent constinit property declarations ****************
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_EntityDef;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeEntityComponent constinit property declarations ******************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEntityComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEntityComponent_Statics

// ********** Begin Class UNarrativeEntityComponent Property Definitions ***************************
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeEntityComponent_Statics::NewProp_EntityDef = { "EntityDef", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityComponent, EntityDef), Z_Construct_UClass_UNarrativeEntityDef_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EntityDef_MetaData), NewProp_EntityDef_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeEntityComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityComponent_Statics::NewProp_EntityDef,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityComponent_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeEntityComponent Property Definitions *****************************
UObject* (*const Z_Construct_UClass_UNarrativeEntityComponent_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UActorComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityComponent_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEntityComponent_Statics::ClassParams = {
	&UNarrativeEntityComponent::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeEntityComponent_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityComponent_Statics::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEntityComponent_Statics::Class_MetaDataParams)
};
void UNarrativeEntityComponent::StaticRegisterNativesUNarrativeEntityComponent()
{
}
UClass* Z_Construct_UClass_UNarrativeEntityComponent()
{
	if (!Z_Registration_Info_UClass_UNarrativeEntityComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEntityComponent.OuterSingleton, Z_Construct_UClass_UNarrativeEntityComponent_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEntityComponent.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEntityComponent);
UNarrativeEntityComponent::~UNarrativeEntityComponent() {}
// ********** End Class UNarrativeEntityComponent **************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEntityComponent_h__Script_NarrativeEngine_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeEntityComponent, UNarrativeEntityComponent::StaticClass, TEXT("UNarrativeEntityComponent"), &Z_Registration_Info_UClass_UNarrativeEntityComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEntityComponent), 3327041420U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEntityComponent_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEntityComponent_h__Script_NarrativeEngine_1936033326{
	TEXT("/Script/NarrativeEngine"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEntityComponent_h__Script_NarrativeEngine_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEntityComponent_h__Script_NarrativeEngine_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
