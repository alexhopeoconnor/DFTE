# Getting started

DFTE renders a named root template through a `TemplateContext`. Register templates and placeholder values once, then ask the renderer for fixed-size chunks until completion.

```cpp
#include <Arduino.h>
#include <TemplateEngine.h>

static const char ROOT[] PROGMEM = R"DFTE(
<h1>%TITLE%</h1><p>Uptime: %UPTIME%</p>
)DFTE";

PlaceholderRegistry registry;
TemplateContext context;

void setup() {
    Serial.begin(115200);
    registry.registerProgmemData(PSTR("%TITLE%"), PSTR("DFTE Quickstart"));
    registry.registerRamData(PSTR("%UPTIME%"), []() -> const char* {
        static char value[16];
        snprintf(value, sizeof(value), "%lus", millis() / 1000);
        return value;
    });
    registry.registerProgmemTemplate(PSTR("%ROOT%"), ROOT);
    context.setRegistry(&registry);
    TemplateRenderer::initializeContext(context, PSTR("%ROOT%"));
}

void loop() {
    uint8_t chunk[128];
    if (!TemplateRenderer::isComplete(context) && !TemplateRenderer::hasError(context)) {
        const size_t written = TemplateRenderer::renderNextChunk(context, chunk, sizeof(chunk));
        Serial.write(chunk, written);
    }
}
```

`registerProgmemData()` is for static flash data. `registerRamData()` takes a getter for a value that can change as chunks are rendered. `registerProgmemTemplate()` lets a placeholder expand to another template.

For a buildable project, start with [HelloPlaceholder](../examples/HelloPlaceholder/). Next: [template language](TEMPLATE_LANGUAGE.md).

Back to [documentation](README.md) · [project overview](../README.md).
