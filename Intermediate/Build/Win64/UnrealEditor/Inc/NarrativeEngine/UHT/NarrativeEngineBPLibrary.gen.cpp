// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "NarrativeEngineBPLibrary.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeNarrativeEngineBPLibrary() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UBlueprintFunctionLibrary();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEngineBPLibrary();
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEngineBPLibrary_NoRegister();
UPackage* Z_Construct_UPackage__Script_NarrativeEngine();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UNarrativeEngineBPLibrary Function NarrativeEngineSampleFunction *********
struct Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics
{
	struct NarrativeEngineBPLibrary_eventNarrativeEngineSampleFunction_Parms
	{
		float Param;
		float ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "NarrativeEngineTesting" },
		{ "DisplayName", "Execute Sample function" },
		{ "Keywords", "NarrativeEngine sample test testing" },
		{ "ModuleRelativePath", "Public/NarrativeEngineBPLibrary.h" },
	};
#endif // WITH_METADATA

// ********** Begin Function NarrativeEngineSampleFunction constinit property declarations *********
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Param;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function NarrativeEngineSampleFunction constinit property declarations ***********
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function NarrativeEngineSampleFunction Property Definitions ********************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NewProp_Param = { "Param", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(NarrativeEngineBPLibrary_eventNarrativeEngineSampleFunction_Parms, Param), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(NarrativeEngineBPLibrary_eventNarrativeEngineSampleFunction_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NewProp_Param,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::PropPointers) < 2048);
// ********** End Function NarrativeEngineSampleFunction Property Definitions **********************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_UNarrativeEngineBPLibrary, nullptr, "NarrativeEngineSampleFunction", 	Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NarrativeEngineBPLibrary_eventNarrativeEngineSampleFunction_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::Function_MetaDataParams), Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::NarrativeEngineBPLibrary_eventNarrativeEngineSampleFunction_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UNarrativeEngineBPLibrary::execNarrativeEngineSampleFunction)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_Param);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(float*)Z_Param__Result=UNarrativeEngineBPLibrary::NarrativeEngineSampleFunction(Z_Param_Param);
	P_NATIVE_END;
}
// ********** End Class UNarrativeEngineBPLibrary Function NarrativeEngineSampleFunction ***********

// ********** Begin Class UNarrativeEngineBPLibrary ************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UNarrativeEngineBPLibrary;
UClass* UNarrativeEngineBPLibrary::GetPrivateStaticClass()
{
	using TClass = UNarrativeEngineBPLibrary;
	if (!Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("NarrativeEngineBPLibrary"),
			Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.InnerSingleton,
			StaticRegisterNativesUNarrativeEngineBPLibrary,
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
	return Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.InnerSingleton;
}
UClass* Z_Construct_UClass_UNarrativeEngineBPLibrary_NoRegister()
{
	return UNarrativeEngineBPLibrary::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* \n*\x09""Function library class.\n*\x09""Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.\n*\n*\x09When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.\n*\x09""BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.\n*\x09""BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.\n*\x09""DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.\n*\x09\x09\x09\x09Its lets you name the node using characters not allowed in C++ function names.\n*\x09""CompactNodeTitle - the word(s) that appear on the node.\n*\x09Keywords -\x09the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. \n*\x09\x09\x09\x09Good example is \"Print String\" node which you can find also by using keyword \"log\".\n*\x09""Category -\x09the category your node will be under in the Blueprint drop-down menu.\n*\n*\x09""For more info on custom blueprint nodes visit documentation:\n*\x09https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation\n*/" },
#endif
		{ "IncludePath", "NarrativeEngineBPLibrary.h" },
		{ "ModuleRelativePath", "Public/NarrativeEngineBPLibrary.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "*      Function library class.\n*      Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.\n*\n*      When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.\n*      BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.\n*      BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.\n*      DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.\n*                              Its lets you name the node using characters not allowed in C++ function names.\n*      CompactNodeTitle - the word(s) that appear on the node.\n*      Keywords -      the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu.\n*                              Good example is \"Print String\" node which you can find also by using keyword \"log\".\n*      Category -      the category your node will be under in the Blueprint drop-down menu.\n*\n*      For more info on custom blueprint nodes visit documentation:\n*      https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UNarrativeEngineBPLibrary constinit property declarations ****************
// ********** End Class UNarrativeEngineBPLibrary constinit property declarations ******************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("NarrativeEngineSampleFunction"), .Pointer = &UNarrativeEngineBPLibrary::execNarrativeEngineSampleFunction },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UNarrativeEngineBPLibrary_NarrativeEngineSampleFunction, "NarrativeEngineSampleFunction" }, // 709821126
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UNarrativeEngineBPLibrary>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics
UObject* (*const Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UBlueprintFunctionLibrary,
	(UObject* (*)())Z_Construct_UPackage__Script_NarrativeEngine,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::ClassParams = {
	&UNarrativeEngineBPLibrary::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x000000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::Class_MetaDataParams), Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::Class_MetaDataParams)
};
void UNarrativeEngineBPLibrary::StaticRegisterNativesUNarrativeEngineBPLibrary()
{
	UClass* Class = UNarrativeEngineBPLibrary::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::Funcs));
}
UClass* Z_Construct_UClass_UNarrativeEngineBPLibrary()
{
	if (!Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.OuterSingleton, Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UNarrativeEngineBPLibrary.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UNarrativeEngineBPLibrary);
UNarrativeEngineBPLibrary::~UNarrativeEngineBPLibrary() {}
// ********** End Class UNarrativeEngineBPLibrary **************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h__Script_NarrativeEngine_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UNarrativeEngineBPLibrary, UNarrativeEngineBPLibrary::StaticClass, TEXT("UNarrativeEngineBPLibrary"), &Z_Registration_Info_UClass_UNarrativeEngineBPLibrary, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UNarrativeEngineBPLibrary), 2456336376U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h__Script_NarrativeEngine_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h__Script_NarrativeEngine_925535299{
	TEXT("/Script/NarrativeEngine"),
	Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h__Script_NarrativeEngine_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h__Script_NarrativeEngine_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
