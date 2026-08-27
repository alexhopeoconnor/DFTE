# Template language and lifecycle

Every token has the form `%NAME%`. DFTE resolves it from a `PlaceholderRegistry` according to its registered type.

| Registered type | Purpose |
| --- | --- |
| `registerProgmemData()` | Static flash-resident text |
| `registerRamData()` | Getter returning current `const char*` data |
| `registerProgmemTemplate()` | Nested PROGMEM template |
| `registerDynamicTemplate()` | Template fragment supplied at render time |
| `registerConditional()` | Choose a true, false, or skipped delegate |
| `registerIterator()` | Stream repeated item templates through an iterator handle |

## Context lifecycle

1. Populate a registry during setup.
2. Attach it to a `TemplateContext`.
3. Initialise the context with the root placeholder.
4. Call `renderNextChunk()` until complete or error.
5. Call `reset()` before reusing a completed context for another root.

Do not treat `isComplete()` as a success result by itself; check `hasError()` as well.

## Iterators

An iterator opens a handle, produces `IteratorItemView` values, then closes the handle. Handles are closed when rendering finishes, resets, stalls, or fails. The `open`, `next`, and `close` callbacks must therefore tolerate early termination.

Use [AsyncDashboardDemo](../examples/AsyncDashboardDemo/) for a complete iterator example. Use [NestedLayouts](../examples/NestedLayouts/) for partials and conditions.

Back to [documentation](README.md) · [project overview](../README.md).
