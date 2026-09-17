# DFTE examples

Every example is a standalone PlatformIO project. Build, upload, and monitor from the repository root:

```bash
pio run -d examples/HelloPlaceholder -e example_esp8266
pio run -d examples/HelloPlaceholder -e example_esp8266 -t upload
pio device monitor -d examples/HelloPlaceholder -e example_esp8266
```

Choose the corresponding ESP32 environment where provided. The examples use the checked-out DFTE source, so they double as practical integration checks for this repository.

The normal `example_esp32` / `dashboard_esp32` environments retain the
Arduino-ESP32 3.0.5 compatibility lane. Repository CI also builds the matching
`*_esp32_3_3_11` environments against the current Arduino-ESP32 3.3.11
validation baseline. A consuming application should select and pin its own
complete PlatformIO platform stack.

| Example | Start here when you want to… |
| --- | --- |
| [HelloPlaceholder](HelloPlaceholder/) | understand the smallest registry/context/chunk flow over serial |
| [NestedLayouts](NestedLayouts/) | compose a page with partials, conditions, and repeating data |
| [StreamingAsync](StreamingAsync/) | serve one streamed page through an asynchronous web server |
| [AsyncDashboardDemo](AsyncDashboardDemo/) | inspect a richer browser-facing dashboard without buffering the response |

The SoftAP examples print their network names and passwords to serial. They are self-contained browser demonstrations, not Wi-Fi provisioning implementations.

Back to [DFTE documentation](../docs/README.md) · [project overview](../README.md).
