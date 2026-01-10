// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeEngineViewTool.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeEngineViewTool() {}

// ********** Begin Cross Module References ********************************************************
BLUTILITY_API UClass* Z_Construct_UClass_UEditorUtilityWidget();
COREUOBJECT_API UClass* Z_Construct_UClass_UClass_NoRegister();
NARRATIVEENGINEEDITOR_API UClass* Z_Construct_UClass_UNarrativeEngineViewTool();
NARRATIVEENGINEEDITOR_API UClass* Z_Construct_UClass_UNarrativeEngineViewTool_NoRegister();
NARRATIVEENGINEEDITOR_API UClass* Z_Construct_UClass_UNarrativeEntityViewWidget();
NARRATIVEENGINEEDITOR_API UClass* Z_Construct_UClass_UNarrativeEntityViewWidget_NoRegister();
UMG_API UClass* Z_Construct_UClass_UUserWidget();
UPackage* Z_Construct_UPackage__Script_NarrativeEngineEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeEntityViewWidget ***********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEntityViewWidget;
UClass* UNarrativeEntityViewWidget::GetPrivateStaticClass()
{
	using TClass = UNarrativeEntityViewWidget;
	if (!Z_Registration_Info_UClass_UNarrativeEntityViewWidget.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEntityViewWidget"),
			Z_Registration_Info_UClass_UNarrativeEntityViewWidget.InnerSingleton,
			StaticRegisterNativesUNarrativeEntityViewWidget,
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
	return Z_Registration_Info_UClass_UNarrativeEntityViewWidget.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEntityViewWidget_NoRegister()
{
	return UNarrativeEntityViewWidget::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEntityViewWidget_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/*\n * Visual representation of an FNarrativeEntityInstance\n */" },
#endif
		{ "IncludePath", "NarrativeEngineViewTool.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/NarrativeEngineViewTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "* Visual representation of an FNarrativeEntityInstance" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEntityViewWidget constinit property declarations ***************
// ********** End Class UNarrativeEntityViewWidget constinit property declarations *****************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEntityViewWidget>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEntityViewWidget_Statics
UObject* (*const Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UUserWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngineEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::ClassParams = {
	&UNarrativeEntityViewWidget::StaticClass,
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
	0x00A010A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::Class_MetaDataParams)
};
void UNarrativeEntityViewWidget::StaticRegisterNativesUNarrativeEntityViewWidget()
{
}
UClass* Z_Construct_UClass_UNarrativeEntityViewWidget()
{
	if (!Z_Registration_Info_UClass_UNarrativeEntityViewWidget.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEntityViewWidget.OuterSingleton, Z_Construct_UClass_UNarrativeEntityViewWidget_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEntityViewWidget.OuterSingleton;
}
UNarrativeEntityViewWidget::UNarrativeEntityViewWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEntityViewWidget);
UNarrativeEntityViewWidget::~UNarrativeEntityViewWidget() {}
// ********** End Class UNarrativeEntityViewWidget *************************************************

// ********** Begin Class UNarrativeEngineViewTool *************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEngineViewTool;
UClass* UNarrativeEngineViewTool::GetPrivateStaticClass()
{
	using TClass = UNarrativeEngineViewTool;
	if (!Z_Registration_Info_UClass_UNarrativeEngineViewTool.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEngineViewTool"),
			Z_Registration_Info_UClass_UNarrativeEngineViewTool.InnerSingleton,
			StaticRegisterNativesUNarrativeEngineViewTool,
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
	return Z_Registration_Info_UClass_UNarrativeEngineViewTool.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEngineViewTool_NoRegister()
{
	return UNarrativeEngineViewTool::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEngineViewTool_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "NarrativeEngineViewTool.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/NarrativeEngineViewTool.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ViewWidgetClass_MetaData[] = {
		{ "Category", "NarrativeEngineViewTool" },
		{ "ModuleRelativePath", "Public/NarrativeEngineViewTool.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EntityViews_MetaData[] = {
		{ "Category", "NarrativeEngineViewTool" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/NarrativeEngineViewTool.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEngineViewTool constinit property declarations *****************
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_ViewWidgetClass;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_EntityViews_ValueProp;
	static const UECodeGen_Private::FNamePropertyParams NewProp_EntityViews_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_EntityViews;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class UNarrativeEngineViewTool constinit property declarations *******************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEngineViewTool>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEngineViewTool_Statics

// ********** Begin Class UNarrativeEngineViewTool Property Definitions ****************************
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_ViewWidgetClass = { "ViewWidgetClass", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEngineViewTool, ViewWidgetClass), Z_Construct_UClass_UNarrativeEntityViewWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ViewWidgetClass_MetaData), NewProp_ViewWidgetClass_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews_ValueProp = { "EntityViews", nullptr, (EPropertyFlags)0x00000000000a0009, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UNarrativeEntityViewWidget_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews_Key_KeyProp = { "EntityViews_Key", nullptr, (EPropertyFlags)0x00000000000a0009, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews = { "EntityViews", nullptr, (EPropertyFlags)0x0010008000020009, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UNarrativeEngineViewTool, EntityViews), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EntityViews_MetaData), NewProp_EntityViews_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UNarrativeEngineViewTool_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_ViewWidgetClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UNarrativeEngineViewTool_Statics::NewProp_EntityViews,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineViewTool_Statics::PropPointers) < 2048);
// ********** End Class UNarrativeEngineViewTool Property Definitions ******************************
UObject* (*const Z_Construct_UClass_UNarrativeEngineViewTool_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEditorUtilityWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngineEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineViewTool_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEngineViewTool_Statics::ClassParams = {
	&UNarrativeEngineViewTool::StaticClass,
	"Editor",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UNarrativeEngineViewTool_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineViewTool_Statics::PropPointers),
	0,
	0x00B010A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineViewTool_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEngineViewTool_Statics::Class_MetaDataParams)
};
void UNarrativeEngineViewTool::StaticRegisterNativesUNarrativeEngineViewTool()
{
}
UClass* Z_Construct_UClass_UNarrativeEngineViewTool()
{
	if (!Z_Registration_Info_UClass_UNarrativeEngineViewTool.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEngineViewTool.OuterSingleton, Z_Construct_UClass_UNarrativeEngineViewTool_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEngineViewTool.OuterSingleton;
}
UNarrativeEngineViewTool::UNarrativeEngineViewTool(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEngineViewTool);
UNarrativeEngineViewTool::~UNarrativeEngineViewTool() {}
// ********** End Class UNarrativeEngineViewTool ***************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngineEditor_Public_NarrativeEngineViewTool_h__Script_NarrativeEngineEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeEntityViewWidget, UNarrativeEntityViewWidget::StaticClass, TEXT("UNarrativeEntityViewWidget"), &Z_Registration_Info_UClass_UNarrativeEntityViewWidget, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEntityViewWidget), 3329742456U) },
		{ Z_Construct_UClass_UNarrativeEngineViewTool, UNarrativeEngineViewTool::StaticClass, TEXT("UNarrativeEngineViewTool"), &Z_Registration_Info_UClass_UNarrativeEngineViewTool, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEngineViewTool), 769565512U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngineEditor_Public_NarrativeEngineViewTool_h__Script_NarrativeEngineEditor_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngineEditor_Public_NarrativeEngineViewTool_h__Script_NarrativeEngineEditor_1379654038{
	TEXT("/Script/NarrativeEngineEditor"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngineEditor_Public_NarrativeEngineViewTool_h__Script_NarrativeEngineEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngineEditor_Public_NarrativeEngineViewTool_h__Script_NarrativeEngineEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
