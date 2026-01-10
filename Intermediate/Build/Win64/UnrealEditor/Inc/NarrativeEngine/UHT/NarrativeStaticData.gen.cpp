// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeStaticData.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeStaticData() {}

// ********** Begin Cross Module References ********************************************************
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeActionDef();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeActionDef_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeBasisVector_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDataAsset();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDialogDef();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeDialogDef_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEntityDef();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEntityDef_NoRegister();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEventDef();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEventDef_NoRegister();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeEventDef *******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEventDef;
UClass* UNarrativeEventDef::GetPrivateStaticClass()
{
	using TClass = UNarrativeEventDef;
	if (!Z_Registration_Info_UClass_UNarrativeEventDef.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEventDef"),
			Z_Registration_Info_UClass_UNarrativeEventDef.InnerSingleton,
			StaticRegisterNativesUNarrativeEventDef,
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
	return Z_Registration_Info_UClass_UNarrativeEventDef.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEventDef_NoRegister()
{
	return UNarrativeEventDef::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEventDef_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "NarrativeStaticData.h" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEventDef constinit property declarations ***********************
// ********** End Class UNarrativeEventDef constinit property declarations *************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEventDef>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEventDef_Statics
UObject* (*const Z_Construct_UClass_UNarrativeEventDef_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UNarrativeDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEventDef_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEventDef_Statics::ClassParams = {
	&UNarrativeEventDef::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEventDef_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEventDef_Statics::Class_MetaDataParams)
};
void UNarrativeEventDef::StaticRegisterNativesUNarrativeEventDef()
{
}
UClass* Z_Construct_UClass_UNarrativeEventDef()
{
	if (!Z_Registration_Info_UClass_UNarrativeEventDef.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEventDef.OuterSingleton, Z_Construct_UClass_UNarrativeEventDef_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEventDef.OuterSingleton;
}
UNarrativeEventDef::UNarrativeEventDef() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEventDef);
UNarrativeEventDef::~UNarrativeEventDef() {}
// ********** End Class UNarrativeEventDef *********************************************************

// ********** Begin Class UNarrativeEntityDef ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEntityDef;
UClass* UNarrativeEntityDef::GetPrivateStaticClass()
{
	using TClass = UNarrativeEntityDef;
	if (!Z_Registration_Info_UClass_UNarrativeEntityDef.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEntityDef"),
			Z_Registration_Info_UClass_UNarrativeEntityDef.InnerSingleton,
			StaticRegisterNativesUNarrativeEntityDef,
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
	return Z_Registration_Info_UClass_UNarrativeEntityDef.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEntityDef_NoRegister()
{
	return UNarrativeEntityDef::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEntityDef_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "NarrativeStaticData.h" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_StartingCoordinates_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PreviousCoordinates_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Location_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Force_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* todo implment IEncounter interface for items that are selected via wave function collapse */// todo make this an array of forces (need an embedded structure)\n" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "todo implment IEncounter interface for items that are selected via wave function collapse // todo make this an array of forces (need an embedded structure)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ShannonEntropyRadius_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Distribution of information (i.e. analog to energy) for an entity*/" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Distribution of information (i.e. analog to energy) for an entity" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RadialFalloffExponent_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Curvature imposed on the state space */" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Curvature imposed on the state space" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Mass_MetaData[] = {
		{ "Category", "NarrativeEntityDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEntityDef constinit property declarations **********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_StartingCoordinates_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_StartingCoordinates_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_StartingCoordinates;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_PreviousCoordinates_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_PreviousCoordinates_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_PreviousCoordinates;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Location_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Location_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Location;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Force_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Force_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Force;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ShannonEntropyRadius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_RadialFalloffExponent;
	static const UECodeGen_Private::FDoublePropertyParams NewProp_Mass;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeEntityDef constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEntityDef>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEntityDef_Statics

// ********** Begin Class UNarrativeEntityDef Property Definitions *********************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates_ValueProp = { "StartingCoordinates", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates_Key_KeyProp = { "StartingCoordinates_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates = { "StartingCoordinates", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, StartingCoordinates), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_StartingCoordinates_MetaData), NewProp_StartingCoordinates_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates_ValueProp = { "PreviousCoordinates", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates_Key_KeyProp = { "PreviousCoordinates_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates = { "PreviousCoordinates", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, PreviousCoordinates), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PreviousCoordinates_MetaData), NewProp_PreviousCoordinates_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location_ValueProp = { "Location", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location_Key_KeyProp = { "Location_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location = { "Location", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, Location), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Location_MetaData), NewProp_Location_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force_ValueProp = { "Force", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force_Key_KeyProp = { "Force_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force = { "Force", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, Force), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Force_MetaData), NewProp_Force_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_ShannonEntropyRadius = { "ShannonEntropyRadius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, ShannonEntropyRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ShannonEntropyRadius_MetaData), NewProp_ShannonEntropyRadius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_RadialFalloffExponent = { "RadialFalloffExponent", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, RadialFalloffExponent), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RadialFalloffExponent_MetaData), NewProp_RadialFalloffExponent_MetaData) };
const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Mass = { "Mass", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEntityDef, Mass), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Mass_MetaData), NewProp_Mass_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeEntityDef_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_StartingCoordinates,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_PreviousCoordinates,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Location,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Force,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_ShannonEntropyRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_RadialFalloffExponent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEntityDef_Statics::NewProp_Mass,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityDef_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeEntityDef Property Definitions ***********************************
UObject* (*const Z_Construct_UClass_UNarrativeEntityDef_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UNarrativeDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityDef_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEntityDef_Statics::ClassParams = {
	&UNarrativeEntityDef::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeEntityDef_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityDef_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityDef_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEntityDef_Statics::Class_MetaDataParams)
};
void UNarrativeEntityDef::StaticRegisterNativesUNarrativeEntityDef()
{
}
UClass* Z_Construct_UClass_UNarrativeEntityDef()
{
	if (!Z_Registration_Info_UClass_UNarrativeEntityDef.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEntityDef.OuterSingleton, Z_Construct_UClass_UNarrativeEntityDef_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEntityDef.OuterSingleton;
}
UNarrativeEntityDef::UNarrativeEntityDef() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEntityDef);
UNarrativeEntityDef::~UNarrativeEntityDef() {}
// ********** End Class UNarrativeEntityDef ********************************************************

// ********** Begin Class UNarrativeActionDef ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeActionDef;
UClass* UNarrativeActionDef::GetPrivateStaticClass()
{
	using TClass = UNarrativeActionDef;
	if (!Z_Registration_Info_UClass_UNarrativeActionDef.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeActionDef"),
			Z_Registration_Info_UClass_UNarrativeActionDef.InnerSingleton,
			StaticRegisterNativesUNarrativeActionDef,
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
	return Z_Registration_Info_UClass_UNarrativeActionDef.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeActionDef_NoRegister()
{
	return UNarrativeActionDef::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeActionDef_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "NarrativeStaticData.h" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Force_MetaData[] = {
		{ "Category", "NarrativeActionDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeActionDef constinit property declarations **********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Force_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Force_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Force;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeActionDef constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeActionDef>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeActionDef_Statics

// ********** Begin Class UNarrativeActionDef Property Definitions *********************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force_ValueProp = { "Force", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force_Key_KeyProp = { "Force_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force = { "Force", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeActionDef, Force), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Force_MetaData), NewProp_Force_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeActionDef_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeActionDef_Statics::NewProp_Force,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeActionDef_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeActionDef Property Definitions ***********************************
UObject* (*const Z_Construct_UClass_UNarrativeActionDef_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UNarrativeDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeActionDef_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeActionDef_Statics::ClassParams = {
	&UNarrativeActionDef::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeActionDef_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeActionDef_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeActionDef_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeActionDef_Statics::Class_MetaDataParams)
};
void UNarrativeActionDef::StaticRegisterNativesUNarrativeActionDef()
{
}
UClass* Z_Construct_UClass_UNarrativeActionDef()
{
	if (!Z_Registration_Info_UClass_UNarrativeActionDef.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeActionDef.OuterSingleton, Z_Construct_UClass_UNarrativeActionDef_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeActionDef.OuterSingleton;
}
UNarrativeActionDef::UNarrativeActionDef() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeActionDef);
UNarrativeActionDef::~UNarrativeActionDef() {}
// ********** End Class UNarrativeActionDef ********************************************************

// ********** Begin Class UNarrativeDialogDef ******************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeDialogDef;
UClass* UNarrativeDialogDef::GetPrivateStaticClass()
{
	using TClass = UNarrativeDialogDef;
	if (!Z_Registration_Info_UClass_UNarrativeDialogDef.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeDialogDef"),
			Z_Registration_Info_UClass_UNarrativeDialogDef.InnerSingleton,
			StaticRegisterNativesUNarrativeDialogDef,
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
	return Z_Registration_Info_UClass_UNarrativeDialogDef.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeDialogDef_NoRegister()
{
	return UNarrativeDialogDef::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeDialogDef_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/*\n * Emplace some phrase for characters to \"say\", as if this is one possible incoming thought that's simulated for a character. \n * The radius is a sort of entropy field for simulating wave function collapse in the narrative state hilbert space \n */" },
#endif
		{ "IncludePath", "NarrativeStaticData.h" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "* Emplace some phrase for characters to \"say\", as if this is one possible incoming thought that's simulated for a character.\n* The radius is a sort of entropy field for simulating wave function collapse in the narrative state hilbert space" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Phrase_MetaData[] = {
		{ "Category", "NarrativeDialogDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// todo it'd be cool to break this phrase into a grammer where data fed into the grammar structure is determined via wave function collapse\n" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "todo it'd be cool to break this phrase into a grammer where data fed into the grammar structure is determined via wave function collapse" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Location_MetaData[] = {
		{ "Category", "NarrativeDialogDef" },
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Force_MetaData[] = {
		{ "Category", "NarrativeDialogDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* todo implment IEncounter interface for items that are selected via wave function collapse */// todo make this an array of forces (need an embedded structure)\n" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "todo implment IEncounter interface for items that are selected via wave function collapse // todo make this an array of forces (need an embedded structure)" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ShannonEntropyRadius_MetaData[] = {
		{ "Category", "NarrativeDialogDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Distribution of information (i.e. analog to energy) for an entity*/" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Distribution of information (i.e. analog to energy) for an entity" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RadialFalloffExponent_MetaData[] = {
		{ "Category", "NarrativeDialogDef" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Curvature imposed on the state space */" },
#endif
		{ "ModuleRelativePath", "Public/NarrativeStaticData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Curvature imposed on the state space" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeDialogDef constinit property declarations **********************
	static const UECodeGen_Private::FTextPropertyParams NewProp_Phrase;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Location_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Location_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Location;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Force_ValueProp;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Force_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Force;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ShannonEntropyRadius;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_RadialFalloffExponent;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeDialogDef constinit property declarations ************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeDialogDef>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeDialogDef_Statics

// ********** Begin Class UNarrativeDialogDef Property Definitions *********************************
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Phrase = { "Phrase", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeDialogDef, Phrase), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Phrase_MetaData), NewProp_Phrase_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location_ValueProp = { "Location", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location_Key_KeyProp = { "Location_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location = { "Location", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeDialogDef, Location), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Location_MetaData), NewProp_Location_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force_ValueProp = { "Force", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force_Key_KeyProp = { "Force_Key", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UNarrativeBasisVector_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force = { "Force", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeDialogDef, Force), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Force_MetaData), NewProp_Force_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_ShannonEntropyRadius = { "ShannonEntropyRadius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeDialogDef, ShannonEntropyRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ShannonEntropyRadius_MetaData), NewProp_ShannonEntropyRadius_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_RadialFalloffExponent = { "RadialFalloffExponent", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeDialogDef, RadialFalloffExponent), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RadialFalloffExponent_MetaData), NewProp_RadialFalloffExponent_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeDialogDef_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Phrase,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Location,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_Force,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_ShannonEntropyRadius,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeDialogDef_Statics::NewProp_RadialFalloffExponent,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDialogDef_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeDialogDef Property Definitions ***********************************
UObject* (*const Z_Construct_UClass_UNarrativeDialogDef_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UNarrativeDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDialogDef_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeDialogDef_Statics::ClassParams = {
	&UNarrativeDialogDef::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeDialogDef_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDialogDef_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeDialogDef_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeDialogDef_Statics::Class_MetaDataParams)
};
void UNarrativeDialogDef::StaticRegisterNativesUNarrativeDialogDef()
{
}
UClass* Z_Construct_UClass_UNarrativeDialogDef()
{
	if (!Z_Registration_Info_UClass_UNarrativeDialogDef.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeDialogDef.OuterSingleton, Z_Construct_UClass_UNarrativeDialogDef_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeDialogDef.OuterSingleton;
}
UNarrativeDialogDef::UNarrativeDialogDef() {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeDialogDef);
UNarrativeDialogDef::~UNarrativeDialogDef() {}
// ********** End Class UNarrativeDialogDef ********************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeStaticData_h__Script_NarrativeEngine_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeEventDef, UNarrativeEventDef::StaticClass, TEXT("UNarrativeEventDef"), &Z_Registration_Info_UClass_UNarrativeEventDef, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEventDef), 1451263916U) },
		{ Z_Construct_UClass_UNarrativeEntityDef, UNarrativeEntityDef::StaticClass, TEXT("UNarrativeEntityDef"), &Z_Registration_Info_UClass_UNarrativeEntityDef, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEntityDef), 786253226U) },
		{ Z_Construct_UClass_UNarrativeActionDef, UNarrativeActionDef::StaticClass, TEXT("UNarrativeActionDef"), &Z_Registration_Info_UClass_UNarrativeActionDef, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeActionDef), 219156060U) },
		{ Z_Construct_UClass_UNarrativeDialogDef, UNarrativeDialogDef::StaticClass, TEXT("UNarrativeDialogDef"), &Z_Registration_Info_UClass_UNarrativeDialogDef, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeDialogDef), 2793736321U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeStaticData_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeStaticData_h__Script_NarrativeEngine_551861686{
	TEXT("/Script/NarrativeEngine"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeStaticData_h__Script_NarrativeEngine_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeStaticData_h__Script_NarrativeEngine_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
