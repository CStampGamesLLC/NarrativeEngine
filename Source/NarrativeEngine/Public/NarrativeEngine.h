// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * Narrative Engine simulation (UNarrativeDataSubsystem asset registry → UNarrativeSubsystem
 * scene tick → UNarrativeEntityComponent broadcasts → game-side UNarrativeLocationComponent).
 * Always-on category — playtest builds are Dev and ensures hit the crash reporter, but the
 * silent paths in this system (asset registry returning zero, EntityDef unset, subsystem not
 * yet alive, weak-pointer stale) are exactly what QA needs visibility into when an NPC stops
 * "behaving" or a save round-trip lands an entity in the wrong place.
 *
 * Verbosity convention:
 *  - Verbose: per-tick / per-broadcast spam; gated on for isolated repro
 *  - Log:     scene/entity lifecycle and save-path control flow
 *  - Warning: a single entity/feature silently dropped out of the sim
 *  - Error:   the simulation as a whole can no longer function (registry empty, subsystem dead)
 */
DECLARE_LOG_CATEGORY_EXTERN(LogDB_NarrativeEngine, Log, All);

class FNarrativeEngineModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
