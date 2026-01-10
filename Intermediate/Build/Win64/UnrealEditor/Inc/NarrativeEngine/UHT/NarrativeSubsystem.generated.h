// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "NarrativeSubsystem.h"

#ifdef NARRATIVEENGINE_NarrativeSubsystem_generated_h
#error "NarrativeSubsystem.generated.h already included, missing '#pragma once' in NarrativeSubsystem.h"
#endif
#define NARRATIVEENGINE_NarrativeSubsystem_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class UNarrativeSubsystem ******************************************************
struct Z_Construct_UClass_UNarrativeSubsystem_Statics;
NARRATIVEENGINE_API UClass* Z_Construct_UClass_UNarrativeSubsystem_NoRegister();

#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_34_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUNarrativeSubsystem(); \
	friend struct ::Z_Construct_UClass_UNarrativeSubsystem_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend NARRATIVEENGINE_API UClass* ::Z_Construct_UClass_UNarrativeSubsystem_NoRegister(); \
public: \
	DECLARE_CLASS2(UNarrativeSubsystem, UTickableWorldSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/NarrativeEngine"), Z_Construct_UClass_UNarrativeSubsystem_NoRegister) \
	DECLARE_SERIALIZER(UNarrativeSubsystem)


#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_34_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UNarrativeSubsystem(); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UNarrativeSubsystem(UNarrativeSubsystem&&) = delete; \
	UNarrativeSubsystem(const UNarrativeSubsystem&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UNarrativeSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UNarrativeSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UNarrativeSubsystem) \
	NO_API virtual ~UNarrativeSubsystem();


#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_31_PROLOG
#define FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_34_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_34_INCLASS_NO_PURE_DECLS \
	FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h_34_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UNarrativeSubsystem;

// ********** End Class UNarrativeSubsystem ********************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_UnrealProgs2_DungeonBreak_Plugins_NarrativeEngine_Source_NarrativeEngine_Public_NarrativeSubsystem_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
