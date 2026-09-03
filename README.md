# Device Framework Template Engine

DFTE streams HTML and text from ESP8266 and ESP32 firmware without constructing a complete response in RAM. Store layouts in PROGMEM, resolve changing values through getters, and render fixed-size chunks directly to serial or an asynchronous HTTP response.

## Render a first template

```cpp
#include <TemplateEngine.h>

static const char PAGE[] PROGMEM = "<h1>%TITLE%</h1>";
PlaceholderRegistry registry;
TemplateContext context;

void setup() {
    Serial.begin(115200);
    registry.registerProgmemData(PSTR("%TITLE%"), PSTR("Hello DFTE"));
    registry.registerProgmemTemplate(PSTR("%PAGE%"), PAGE);
    context.setRegistry(&registry);
    TemplateRenderer::initializeContext(context, PSTR("%PAGE%"));
}

void loop() {
    uint8_t chunk[128];
    if (!TemplateRenderer::isComplete(context) && !TemplateRenderer::hasError(context)) {
        Serial.write(chunk, TemplateRenderer::renderNextChunk(context, chunk, sizeof(chunk)));
    }
}
```

Build [Hello Placeholder](examples/HelloPlaceholder/) to see the rendered result over serial.

## Choose an example

| Example | What you will build |
| --- | --- |
| [Hello Placeholder](examples/HelloPlaceholder/) | the smallest registry, context, and serial-rendering flow |
| [Nested Layouts](examples/NestedLayouts/) | reusable partials, conditions, and iterator sections |
| [Streaming Async](examples/StreamingAsync/) | a streamed ESPAsyncWebServer response over a SoftAP |
| [Async Dashboard Demo](examples/AsyncDashboardDemo/) | a live dashboard with iterator rows and captive-portal access |

## Why DFTE

- **Bounded response memory:** render fixed-size chunks instead of allocating one large `String`.
- **Flash-resident layouts:** keep templates, CSS, and shared fragments in PROGMEM.
- **Live values:** resolve dynamic information only as the active response needs it.
- **Async-safe rendering:** each HTTP request owns its rendering context while sharing the prepared registry.

## Install

```ini
lib_deps =
    DeviceFrameworkTemplateEngine=https://github.com/alexhopeoconnor/DFTE.git#v1.1.0
```

PlatformIO checks out the Git ref after `#`; GitHub Release assets are unrelated. DFTE supports ESP8266 and ESP32 Arduino projects.

See [getting started](docs/GETTING_STARTED.md), the [documentation index](docs/README.md), [examples](examples/README.md), [changelog](CHANGELOG.md), and [licence](LICENSE).
