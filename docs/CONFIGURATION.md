# Configuration and memory limits

DFTE has a stable public object layout. `TemplateContext` owns its rendering
stack and read buffer at runtime, so choosing capacities cannot make a consumer
and the compiled library disagree about object size.

```cpp
TemplateContext standard;          // default: 16 stack frames, 512-byte buffer
TemplateContext pageContext(6, 128); // known shallow response, smaller allocation
if (!pageContext.isReady()) {
    // Allocation failed; do not start a response with this context.
}
```

The context allocates its stack and read buffer when it is constructed. Its
approximate heap use is `maxDepth * sizeof(RenderingContext) + bufferSize`, plus
allocator overhead; its fixed 24-byte placeholder-token storage is part of the
stable context object. Allocate one context per concurrently streaming request; use explicit
small capacities for bounded pages rather than changing global definitions.
Nested template expansion normally consumes two stack frames per level.

| Setting | Default | Meaning |
| --- | --- | --- |
| `DFTE_MAX_STACK_DEPTH` | 16 | Default depth passed by the no-argument `TemplateContext` constructor |
| `DFTE_BUFFER_SIZE` | 512 | Default read-buffer size passed by the no-argument constructor |
| `DFTE_MAX_ITERATIONS` | 50 | Safety cap for one renderer call |
| `DFTE_PROGMEM_CHUNK_SIZE` | 512 | Source-copy window for flash data |
| `DFTE_RAM_CHUNK_SIZE` | 128 | Source-copy window for RAM data |
| `DFTE_MAX_PLACEHOLDERS_DEFAULT` | 16 | Default `PlaceholderRegistry` capacity |

The first two are constructor defaults, not layout controls: a consuming
translation unit can choose them without an ABI mismatch. The source-copy and
iteration settings change renderer behaviour, so set them consistently for a
whole PlatformIO build. `DFTE_PLACEHOLDER_NAME_SIZE` is no longer a supported
setting; placeholder tokens have a fixed ABI-stable capacity of 23 characters
plus the terminator.

`DFTE_MAX_PLACEHOLDERS_DEFAULT` is only a constructor default; pass an explicit
registry capacity where a device needs more.

Back to [documentation](README.md) · [project overview](../README.md).
