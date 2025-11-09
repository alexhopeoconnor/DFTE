# Hello Placeholder

This introductory sketch demonstrates the minimum DFTE setup:

- Create a `PlaceholderRegistry` and register simple RAM getters
- Initialise a `TemplateContext` with a RAM template
- Stream the rendered output into a Serial buffer

## Build

```
pio run -d examples/HelloPlaceholder -e example_esp8266
pio run -d examples/HelloPlaceholder -e example_esp32
```

Flash the firmware and open the serial monitor at 115200 baud to see the rendered text.

