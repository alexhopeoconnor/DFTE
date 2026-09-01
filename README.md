# Device Framework Template Engine

DFTE streams HTML and text for ESP8266 and ESP32 without constructing a giant response in RAM. It combines PROGMEM templates, live getters, nested layouts, conditions, and iterators with request-safe asynchronous HTTP rendering.

## Why use it

- **Predictable memory:** render fixed-size chunks instead of one large `String`.
- **Reusable templates:** keep layouts, partials, CSS, and HTML in flash.
- **Live device data:** resolve values from getters only when a chunk is rendered.
- **Safe async responses:** use one render context per HTTP request while sharing one prepared registry.

## Try it

```cpp
#include <TemplateEngine.h>

static const char PAGE[] PROGMEM = "<h1>%TITLE%</h1>";
    Serial.begin(115200);
PlaceholderRegistry registry;
TemplateContext context;

void setup() {
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

## Featured examples

| Goal | Example |
| --- | --- |
| Smallest placeholder render | [HelloPlaceholder](examples/HelloPlaceholder/) |
| Layouts, partials, and conditions | [NestedLayouts](examples/NestedLayouts/) |
| Async HTTP streaming | [StreamingAsync](examples/StreamingAsync/) |
| Dashboard, iterators, and telemetry | [AsyncDashboardDemo](examples/AsyncDashboardDemo/) |

## Install

```ini
lib_deps =
    DeviceFrameworkTemplateEngine=https://github.com/alexhopeoconnor/DFTE.git#v1.1.0
```

PlatformIO checks out the Git ref after `#`; GitHub Release assets are unrelated. DFTE’s supported release targets are ESP8266 and ESP32.

## Documentation

Read the [documentation index](docs/README.md) for template syntax, async web responses, per-context memory sizing, examples, tests, and releases.

## Development and releases

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/check-docs.sh
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Tagging repeats the board-free compile checks, validates the package, and creates a GitHub Release from the matching changelog section. It does not publish to the PlatformIO Registry or deploy firmware.

See the [changelog](CHANGELOG.md) and [licence](LICENSE).
