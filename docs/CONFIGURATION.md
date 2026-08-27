# Configuration and memory limits

DFTE object layouts depend on fixed `DFTE_*` compile-time limits. Define any changes through shared PlatformIO `build_flags` so the library and every consuming translation unit agree on the same layout.

```ini
build_flags =
    -DDFTE_BUFFER_SIZE=768
    -DDFTE_MAX_STACK_DEPTH=24
    -DDFTE_PLACEHOLDER_NAME_SIZE=32
    -DDFTE_MAX_ITERATIONS=80
```

| Flag | Default | Meaning |
| --- | --- | --- |
| `DFTE_BUFFER_SIZE` | 512 | Streaming buffer size in `TemplateContext` |
| `DFTE_MAX_STACK_DEPTH` | 16 | Render stack frames; nested templates usually need two frames each |
| `DFTE_PLACEHOLDER_NAME_SIZE` | 24 | Maximum token length including `%` characters |
| `DFTE_PROGMEM_CHUNK_SIZE` | 512 | PROGMEM source copy window |
| `DFTE_RAM_CHUNK_SIZE` | 128 | RAM source copy window |
| `DFTE_MAX_ITERATIONS` | 50 | Safety cap for one render call |
| `DFTE_MAX_PLACEHOLDERS_DEFAULT` | 16 | Default registry constructor capacity |

`DFTE_MAX_PLACEHOLDERS_DEFAULT` is only a constructor default; pass an explicit registry capacity where a device needs more. DeviceFramework runtime template parameters do not change DFTE’s compile-time object layout.

Back to [documentation](README.md) · [project overview](../README.md).
