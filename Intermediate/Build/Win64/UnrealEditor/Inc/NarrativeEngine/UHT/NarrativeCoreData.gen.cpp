// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeCoreData.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeCoreData() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
ENGINE_API UClass* Z_Construct_UClass_UEngineSubsystem();
ENGINE_API UClass* Z_Construct_UClass_UTexture2D_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeBasisVector();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeBasisVector_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDataAsset();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDataAsset_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDataSubsystem();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDataSubsystem_NoRegister();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeDataSubsystem **************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeDataSubsystem;
UClass* UNarrativeDataSubsystem::GetPrivateStaticClass()
{
	using TClass = UNarrativeDataSubsystem;
	if (!Z_Registration_Info_UClass_UNarrativeDataSubsystem.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeDataSubsystem"),
			Z_Registration_Info_UClass_UNarrativeDataSubsystem.InnerSingleton,
			StaticRegisterNativesUNarrativeDataSubsystem,
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
	return Z_Registration_Info_UClass_UNarrativeDataSubsystem.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeDataSubsystem_NoRegister()
{
	return UNarrativeDataSubsystem::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeDataSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "NarrativeCoreData.h" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeDataSubsystem constinit property declarations ******************
// ********** End Class UNarrativeDataSubsystem constinit property declarations ********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeDataSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeDataSubsystem_Statics
UObject* (*const Z_Construct_UClass_UNarrativeDataSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEngineSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDataSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeDataSubsystem_Statics::ClassParams = {
	&UNarrativeDataSubsystem::StaticClass,
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
	0x000000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDataSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeDataSubsystem_Statics::Class_MetaDataParams)
};
void UNarrativeDataSubsystem::StaticRegisterNativesUNarrativeDataSubsystem()
{
}
UClass* Z_Construct_UClass_UNarrativeDataSubsystem()
{
	if (!Z_Registration_Info_UClass_UNarrativeDataSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeDataSubsystem.OuterSingleton, Z_Construct_UClass_UNarrativeDataSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeDataSubsystem.OuterSingleton;
}
UNarrativeDataSubsystem::UNarrativeDataSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeDataSubsystem);
UNarrativeDataSubsystem::~UNarrativeDataSubsystem() {}
// ********** End Class UNarrativeDataSubsystem ****************************************************

// ********** Begin Class UNarrativeDataAsset ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeDataAsset;
UClass* UNarrativeDataAsset::GetPrivateStaticClass()
{
	using TClass = UNarrativeDataAsset;
	if (!Z_Registration_Info_UClass_UNarrativeDataAsset.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeDataAsset"),
			Z_Registration_Info_UClass_UNarrativeDataAsset.InnerSingleton,
			StaticRegisterNativesUNarrativeDataAsset,
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
	return Z_Registration_Info_UClass_UNarrativeDataAsset.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeDataAsset_NoRegister()
{
	return UNarrativeDataAsset::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeDataAsset_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Common base for all narrative data table records */" },
#endif
		{ "IncludePath", "NarrativeCoreData.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Common base for all narrative data table records" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeDataAsset constinit property declarations **********************
// ********** End Class UNarrativeDataAsset constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeDataAsset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeDataAsset_Statics
UObject* (*const Z_Construct_UClass_UNarrativeDataAsset_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDataAsset_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeDataAsset_Statics::ClassParams = {
	&UNarrativeDataAsset::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDataAsset_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeDataAsset_Statics::Class_MetaDataParams)
};
void UNarrativeDataAsset::StaticRegisterNativesUNarrativeDataAsset()
{
}
UClass* Z_Construct_UClass_UNarrativeDataAsset()
{
	if (!Z_Registration_Info_UClass_UNarrativeDataAsset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeDataAsset.OuterSingleton, Z_Construct_UClass_UNarrativeDataAsset_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeDataAsset.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeDataAsset);
UNarrativeDataAsset::~UNarrativeDataAsset() {}
// ********** End Class UNarrativeDataAsset ********************************************************

// ********** Begin Class UNarrativeBasisVector ****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeBasisVector;
UClass* UNarrativeBasisVector::GetPrivateStaticClass()
{
	using TClass = UNarrativeBasisVector;
	if (!Z_Registration_Info_UClass_UNarrativeBasisVector.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeBasisVector"),
			Z_Registration_Info_UClass_UNarrativeBasisVector.InnerSingleton,
			StaticRegisterNativesUNarrativeBasisVector,
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
	return Z_Registration_Info_UClass_UNarrativeBasisVector.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeBasisVector_NoRegister()
{
	return UNarrativeBasisVector::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeBasisVector_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "NarrativeCoreData.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ArchetypeTitle_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Icon_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubComponents_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Verb_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Subject_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Adjectives_MetaData[] = {
		{ "BlueprintType", "true" },
		{ "Category", "NarrativeBasisVector" },
		{ "ModuleRelativePath", "Public/NarrativeCoreData.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeBasisVector constinit property declarations ********************
	static const UECodeGen_Private::FTextPropertyParams NewProp_ArchetypeTitle;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Icon;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_SubComponents_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_SubComponents;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Verb;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Subject;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Adjectives_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Adjectives;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeBasisVector constinit property declarations **********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeBasisVector>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeBasisVector_Statics

// ********** Begin Class UNarrativeBasisVector Property Definitions *******************************
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_ArchetypeTitle = { "ArchetypeTitle", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, ArchetypeTitle), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ArchetypeTitle_MetaData), NewProp_ArchetypeTitle_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Icon = { "Icon", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, Icon), Z_Construct_UClass_UTexture2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Icon_MetaData), NewProp_Icon_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_SubComponents_Inner = { "SubComponents", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_SubComponents = { "SubComponents", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, SubComponents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubComponents_MetaData), NewProp_SubComponents_MetaData) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Verb = { "Verb", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, Verb), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Verb_MetaData), NewProp_Verb_MetaData) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Subject = { "Subject", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, Subject), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Subject_MetaData), NewProp_Subject_MetaData) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Adjectives_Inner = { "Adjectives", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Adjectives = { "Adjectives", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeBasisVector, Adjectives), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Adjectives_MetaData), NewProp_Adjectives_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeBasisVector_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_ArchetypeTitle,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Icon,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_SubComponents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_SubComponents,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Verb,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Subject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Adjectives_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeBasisVector_Statics::NewProp_Adjectives,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeBasisVector_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeBasisVector Property Definitions *********************************
UObject* (*const Z_Construct_UClass_UNarrativeBasisVector_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UNarrativeDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeBasisVector_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeBasisVector_Statics::ClassParams = {
	&UNarrativeBasisVector::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeBasisVector_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeBasisVector_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeBasisVector_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeBasisVector_Statics::Class_MetaDataParams)
};
void UNarrativeBasisVector::StaticRegisterNativesUNarrativeBasisVector()
{
}
UClass* Z_Construct_UClass_UNarrativeBasisVector()
{
	if (!Z_Registration_Info_UClass_UNarrativeBasisVector.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeBasisVector.OuterSingleton, Z_Construct_UClass_UNarrativeBasisVector_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeBasisVector.OuterSingleton;
}
UNarrativeBasisVector::UNarrativeBasisVector() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeBasisVector);
UNarrativeBasisVector::~UNarrativeBasisVector() {}
// ********** End Class UNarrativeBasisVector ******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeCoreData_h__Script_NarrativeEngine_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeDataSubsystem, UNarrativeDataSubsystem::StaticClass, TEXT("UNarrativeDataSubsystem"), &Z_Registration_Info_UClass_UNarrativeDataSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeDataSubsystem), 2922885072U) },
		{ Z_Construct_UClass_UNarrativeDataAsset, UNarrativeDataAsset::StaticClass, TEXT("UNarrativeDataAsset"), &Z_Registration_Info_UClass_UNarrativeDataAsset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeDataAsset), 3435432371U) },
		{ Z_Construct_UClass_UNarrativeBasisVector, UNarrativeBasisVector::StaticClass, TEXT("UNarrativeBasisVector"), &Z_Registration_Info_UClass_UNarrativeBasisVector, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeBasisVector), 2746974645U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeCoreData_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeCoreData_h__Script_NarrativeEngine_1667857055{
	TEXT("/Script/NarrativeEngine"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeCoreData_h__Script_NarrativeEngine_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeCoreData_h__Script_NarrativeEngine_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
