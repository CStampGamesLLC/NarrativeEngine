# Narrative Space editor

Open **Tools > Narrative Space** (also available in the Window tools category).
In the Query panel, add one to three `UNarrativeBasisVector` assets to **Axes**,
in X, Y, Z order. Choose any number of narrative data asset **Classes**, or leave
the list empty for all classes, including subclasses. **Content Path** is a
recursive package folder such as `/Game` or `/NarrativeEngine`, not a disk path.
Empty content path means `/Game`. Invalid/duplicate axes and invalid class/path
entries show an explanation and disable editing instead of retaining a stale view.

Click an asset to inspect/edit its properties in the right-hand Details panel.
Assets without a valid editable `FVectorND` placement are omitted and counted in
the status line. Hollow axis markers mean an axis is absent, distinct from an
explicit zero. Hover for the full label and asset path.

Use **Placement field (plot + drag)** above the Query panel to switch fields on
the fly, for example `StartingCoordinates`, `Motive`, or `PreviousCoordinates`.
The list includes inherited editable `FVectorND` fields from assets matching the
class/path filters. **Class default** uses each type's default field; agents now
inherit `StartingCoordinates`, just like other entities. An explicit field choice
omits assets without that field, rather than silently editing a different vector.
The status line identifies the chosen field and counts omitted assets. Switching
reframes the view, retains compatible selection, and does not change any values.
An in-progress drag is canceled before switching. Undo/redo still targets the
field that was edited, even after choosing another field.

## Controls

| Action | Input |
| --- | --- |
| Select / add to selection | Click / Shift-click |
| Select a region | Drag empty space; Shift adds |
| Move selected assets | Drag a selected card |
| Restrict movement | Hold X, Y, or Z during the drag |
| Snap moved coordinates | Hold Ctrl; grid step follows zoom |
| Cancel a move | Esc; losing mouse capture also cancels |
| Pan | Right or middle mouse drag |
| Zoom around cursor | Mouse wheel |
| Frame selection / all | F / Home, or toolbar buttons |
| 3D orientation | XY, XZ, YZ, Iso; Alt+right mouse drag orbits |
| Open asset editor | Double-click |
| Undo / redo | Ctrl+Z / Ctrl+Y while viewport has focus |
| Save | Save matching assets, or Ctrl+S in viewport |
| Place Content Browser assets | Drop assets matching the current query onto the viewport |

A one-axis query is a number line. Overlapping cards stack vertically for
selection; this vertical displacement is display-only. Two axes use an XY plane.
Three axes use orthographic projection and depth sorting. Unlocked dragging moves
in the view plane. A locked axis viewed end-on cannot move until the view changes.
Dropping multiple assets preserves their relative offsets and hidden coordinates.

Zoom changes the representation from dots to icons, labels, then full coordinate
cards with radius rings. Small overlapping items combine into count badges;
clicking a badge selects and frames its contents. Offscreen cards are culled.

## Editing and persistence

Dragging starts a single transaction after the mouse crosses the movement
threshold. Deltas are always computed from the original values. Only moved axes
in the current query are written; missing moved axes are appended through
`FVectorND::SetCoordinate`. Other coordinates and other vector properties retain
their values and basis ordering. One undo restores the entire multi-asset move,
including removing axes introduced by the move. Esc restores both vectors and
the packages' previous dirty state. A click/no-op move does not create an undo
entry or leave a previously clean package dirty.

Completed edits send `PostEditChangeProperty` for the resolved vector field.
External Details edits, undo/redo, registry add/remove/rename, initial registry
scan completion, and object replacement refresh the model. The model pins loaded
assets and icon textures against GC. Editing and saving are disabled during PIE.

Saving is explicit: **Save matching assets** offers Unreal's checkout-and-save
dialog for dirty packages currently represented by the query, including changes
made in the Details panel. Changing the filter does not save packages removed
from the view; use the normal editor Save All workflow for those packages. The
tool does not automatically save, migrate, submit, or commit content.

## Architecture and choices

The initial host is a native dockable Slate tab, instead of a binary Editor Utility
Widget Blueprint. It works immediately after compiling, exposes the query through
a standard Details panel, and keeps the whole change reviewable as source.
`SNarrativeSpaceViewport` is a reusable leaf widget, with no per-card child widget
tree. `FNarrativeSpaceModel` owns gathering, selection, GC references, field caches,
and editing. `FNarrativeSpaceCamera` owns projection and navigation math. These can
be hosted in a future `UWidget`/EUW adapter without moving editing logic into UMG.
The existing PIE viewer is unchanged. Query/camera settings currently last for
the lifetime of the tab; no view preset asset or config is written.

Runtime assets expose four C++ virtuals only under `WITH_EDITOR`:
`GetSpacePlacementProperty`, `GetSpaceLabel`, `GetSpaceIcon`, and `GetSpaceRadius`.
The default placement is the first editable scalar `FVectorND` field, including
inherited fields. The selected field is cached per class, so overrides must choose
the same field for every instance of that class. A native override can return
`NAME_None` to opt out. Nested vectors/arrays are not automatically traversed.

| Asset type | Placement | Presentation |
| --- | --- | --- |
| Entity | `StartingCoordinates` | Asset name |
| Game Agent | `StartingCoordinates` (select `Motive` in the picker when needed) | `DisplayName`, `Portrait` |
| Dialog | `Location` | `Label`, `SpaceSigma` radius |
| Archetype | `Location` | `DisplayName`, `Icon` |
| Action | `Force` | Asset name |

Matching assets and their icons are loaded on query refresh. This is appropriate
for the proposed dozens-to-hundreds scale. For much larger libraries, the next
architectural step is coordinate/presentation registry tags plus lazy loading on
edit. Saved named queries and a UMG host can be added independently if needed.

## Validation

Run `Narrative.Space` from Session Frontend's Automation tab or:

```text
Automation RunTests Narrative.Space
```

Tests cover projection and zoom invariants, orbit, locks, reordered/sparse bases,
hidden coordinate preservation, query validation, registry/property refresh,
multi-asset undo/redo, cancellation, no-op dirty-state preservation, and native
widget construction. `Narrative.Space.Render` requires a rendering RHI and writes
`Saved/Automation/NarrativeSpace/Editor.png`; it reports a skip under NullRHI.
Fixtures are temporary in-memory assets; these tests do not save project assets.

Local validation on 2026-09-19: the Win64 Development editor build passed, all
18 `Narrative.*` tests passed, and the six `Narrative.Space` tests passed again
after visual fixes. The final inspector-width change was rebuilt and checked with
`Narrative.Space.Render`, including initial framing, hit selection, and an inspected
screenshot. The later render runs captured an unrelated HTTP retry warning from
the editor's background services, with zero test failures. Reports are under
`Saved/Automation/NarrativeSpace`, `NarrativeSpaceFinal`, and `NarrativeSpaceRender`.

Before adopting in an authoring session, review a representative real asset in
each class, check the proposed Entity/Agent placement choices, and exercise the
checkout-and-save dialog with your source-control setup.
