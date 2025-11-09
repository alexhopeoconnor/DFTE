# Nested Layouts

Demonstrates nested templates, conditional placeholders, and iterator-driven sections without a web server dependency. Ideal for learning how DFTE composes complex layouts on memory-constrained boards.

Highlights:
- Shared header/footer partials referenced via `%HEADER%` and `%FOOTER%`
- Conditional placeholder toggling a "maintenance" banner
- Iterator rendering a list of subsystem status entries with per-item overrides

## Build

```
pio run -d examples/NestedLayouts -e example_esp8266
pio run -d examples/NestedLayouts -e example_esp32
```

Open the serial monitor at 115200 baud to inspect the rendered output.

