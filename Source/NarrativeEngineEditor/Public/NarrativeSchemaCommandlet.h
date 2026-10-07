#pragma once

#include "Commandlets/Commandlet.h"
#include "NarrativeSchemaCommandlet.generated.h"

/**
 * The P1 schema migration and lints (TDD Patterns 6 and 26), for the editor and for CI.
 *
 *   -run=NarrativeSchema                 Report what the migration would do, then run the lints.
 *   -run=NarrativeSchema -Save           Seed every empty mask from the axes its vector has typed,
 *                                        seed card families from their Dialog/<Family>/ folder, save.
 *   -run=NarrativeSchema -LintOnly       Lints only; exits 1 on any error (for CI).
 *   -run=NarrativeSchema -Export=Docs/NarrativeHeuristics/authored_export.json [-ExportOnly]
 *                                        Writes every axis, archetype, entity sheet and card as JSON:
 *                                        the P1 export the oracle re-calibrates authored content from.
 *
 * Applying a calibrated sheet (the reference implementation's output):
 *   -ApplySheet=<EntityAsset> -Calibration=<numbers.json> -Profile=<profile>
 *   [-Mask=<Axis>,<Axis>,...] [-TellCard=<CardAsset>] [-Loyalty=<n>]
 * reads sheet.<Profile> (w_pos, w_reg, bias, th, tf, delta, tz) and sheet.theta_obey / theta_join,
 * turns the entity's mind on, and replaces its mask and tell card when given. Tuning stays in data.
 *
 * Seeded masks are a starting point, not a decision: every mask wider than three axes is flagged
 * by L13 for an author to narrow.
 */
UCLASS()
class NARRATIVEENGINEEDITOR_API UNarrativeSchemaCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UNarrativeSchemaCommandlet();
	virtual int32 Main(const FString& Params) override;
};
