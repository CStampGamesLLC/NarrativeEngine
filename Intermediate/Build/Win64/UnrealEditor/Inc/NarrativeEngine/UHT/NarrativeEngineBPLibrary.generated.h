// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "NarrativeEngineBPLibrary.h"

#ifdef NARRATIVEENGINE_NarrativeEngineBPLibrary_generated_h
#error "NarrativeEngineBPLibrary.generated.h already included, missing '#pragma once' in NarrativeEngineBPLibrary.h"
#endif
#define NARRATIVEENGINE_NarrativeEngineBPLibrary_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UNarrativeEngineBPLibrary ************************************************
#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_RPC_WRAPPERS \
	DECLARE_FUNCTION(execNarrativeEngineSampleFunction);


struct Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics;
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeEngineBPLibrary_NoRegister();

#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_INCLASS \
private: \
	static void StaticRegisterNativesUNarrativeEngineBPLibrary(); \
	friend struct ::Z_Construct_UClass_UNarrativeEngineBPLibrary_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend NARRATIVEENGINE_API UClass* ::Z_Construct_UClass_UNarrativeEngineBPLibrary_NoRegister(); \
public: \
	DECLARE_CLASS2(UNarrativeEngineBPLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/NarrativeEngine"), Z_Construct_UClass_UNarrativeEngineBPLibrary_NoRegister) \
	DECLARE_SERIALIZER(UNarrativeEngineBPLibrary)


#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UNarrativeEngineBPLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UNarrativeEngineBPLibrary) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UNarrativeEngineBPLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UNarrativeEngineBPLibrary); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UNarrativeEngineBPLibrary(UNarrativeEngineBPLibrary&&) = delete; \
	UNarrativeEngineBPLibrary(const UNarrativeEngineBPLibrary&) = delete; \
	NO_API virtual ~UNarrativeEngineBPLibrary();


#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_25_PROLOG
#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_RPC_WRAPPERS \
	FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_INCLASS \
	FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h_28_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UNarrativeEngineBPLibrary;

// ********** End Class UNarrativeEngineBPLibrary **************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeEngineBPLibrary_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
