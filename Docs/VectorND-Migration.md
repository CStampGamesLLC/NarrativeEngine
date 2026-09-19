# Dense narrative vectors and asset migration

`FVectorND` is now a reflected value type. Its contiguous `Coordinates` array is authoritative;
the parallel `Basis` array identifies each coordinate by a soft asset reference. There is no
serialized map and no derived coordinate cache. Both arrays are saved in editor and cooked
assets. Construction, normal serialization, and arithmetic do not load assets or query the registry.

The following properties retain their names but change from map to vector:

- `UNarrativeEntityDef.StartingCoordinates` and `PreviousCoordinates`
- `UArchetypeDef.Location`
- `UNarrativeActionDef.Force`
- `UNarrativeDialogDef.Location`
- The game's `UNarrativeAgentDef.Motive`

## Run the migration

1. Save your current work, close the editor, and build `DungeonBreakEditor` (Development/Win64).
   Reopen the project after the build. This reflection/layout change needs a full restart;
   do not apply it with Live Coding or Hot Reload.
2. In **Output Log**, run `Narrative.VectorND.PreviewMigration /Game`.
   An optional narrower folder, such as `/Game/Narrative`, is useful for an initial review.
   The scan loads assets and converts legacy maps **in memory only**. It reports converted
   fields, missing/null basis references, invalid vectors, and already-dirty packages.
3. Inspect representative entity, archetype, action, and dialog assets in Details.
   Check both nonzero and zero coordinates. The vector header displays axis count and magnitude.
   Each axis has a numeric field and Remove button. Add one basis asset with the picker or
   use **Add all basis axes** to append missing dimensions at zero.
4. Run `Narrative.VectorND.Migrate /Game`. It repeats validation, then presents Unreal's
   checkout/save dialog for eligible converted packages. You can deselect packages or cancel.
   Missing basis assets and malformed data block their entire package from this batch.
   Dirty packages are skipped so unrelated unsaved work is not swept into the migration.
   Review and save those manually, or restart after resolving them and run again.
5. Restart and preview again. Resaved properties should no longer report as legacy.
   Review binary asset changes in source control, then run a representative PIE encounter.

For plugin content, run the commands with its mount root, such as `/NarrativeEngine`.
The default `/Game` scan does not cover other mount roots.

The same operations are exposed as **Preview Vector Migration** and **Migrate Vector Assets**
Blueprint nodes on `UNarrativeVectorMigrationLibrary`, so an Editor Utility Widget button
can call them directly. Both return the report text; console commands write it under
`LogNarrativeVectorMigration`. PIE must be stopped. No migration command runs automatically.

## Compatibility and boundaries

- Old map property tags are accepted through `WithStructuredSerializeFromMismatchedTag`.
  `FNarrativeVectorLegacy` supplies the original reflected map serializer, including inherited
  defaults and map removals. It is a temporary decoding type, never a member of the new vector.
- Soft references are preserved even if their assets are unloaded. Conversion does not
  depend on whether a reference is currently loaded. The explicit migration scan loads references to
  validate them before offering to save. Missing references remain available for diagnosis
  and can be removed/re-added in Details.
- Asset identity, not registry enumeration order, determines the meaning of saved values.
  Reordered bases and missing axes are supported by arithmetic (missing means zero).
  Runtime entity initialization expands authored vectors to the registered basis; extra
  authored axes are retained. Matching layouts use contiguous coordinate arithmetic.
- The empty vector is a valid zero vector. Explicit zero coordinates are retained. A newly
  added basis does not rewrite every asset; it is zero until authored. Axis removal changes
  the vector's defined basis and removes the corresponding coordinate.
- Editing multiple objects is supported when their basis layouts match. Different layouts
  require individual editing, to avoid writing one axis's value into another axis's slot.
- Existing map-specific Blueprint operations/pins are **not** rewritten by this migration.
  Update those graphs to the vector API as needed and recompile them. The scan covers
  narrative data assets and narrative-data-asset Blueprint class defaults, including
  inherited native vector properties. It is not a generic conversion of arbitrary map
  variables, embedded actor instances, or unrelated Blueprint graphs.
- Existing save-game `TArray<float>` fields retain their legacy registry-index format.
  Their capture/restore adapters are explicit and validate coordinate counts. This change
  does not migrate player save files or solve the pre-existing ordering limitation of that
  format; a separately versioned save-game migration would be needed for that.
- Conversion provenance is session-only. Repeating migration in one editor session can
  offer previously saved converted packages again; the conversion is idempotent. Restarting
  reads the new format and clears that provenance naturally. Keep the compatibility reader
  until all relevant content (including other branches) has been resaved.

## Tests

Run **Narrative.VectorND** in Session Frontend's Automation tab, or execute:

```text
Automation RunTests Narrative.VectorND
```

The tests exercise identity-aware arithmetic and rebasing, reflected binary and text
round trips, legacy map decoding, inherited unchanged values, overrides, removals, empty
maps, explicit zero values, and preservation/validation of a null legacy basis. Legacy
tests use actual tagged property streams to exercise the property system's conversion hook.

The Development/Win64 editor build and all four `Narrative.VectorND` automation tests
passed locally. The report is in `Game/Saved/Automation/VectorND`. The test run did not
invoke the asset-resave command.

Also check editor save/reload, undo/redo for numeric and structural edits, copy/paste,
reset to default, and multi-selection on representative assets before bulk resaving.
