# Device Framework Template Engine (DFTE)

DFTE is a lightweight C++ template engine tailored for Arduino-class hardware (ESP8266, ESP32, RP2040, and friends) that need to render rich HTML dashboards or textual feeds without allocating giant buffers. It streams HTML over chunked HTTP, stitches together deeply nested layouts from PROGMEM, and injects live device data sourced from RAM getters—all while keeping your microcontroller responsive. If you are searching for an “ESP32 streaming HTML template engine” or a way to “render dynamic Arduino web UI without SPIFFS,” DFTE is the solution.


## Quickstart
1. Define your root template in PROGMEM (or RAM if you prefer):

   ```
   static const char ROOT_TEMPLATE_PROGMEM[] PROGMEM = R"DFTE(
   <html>
     <head><title>%APP_TITLE%</title></head>
     <body>
       <h1>%APP_TITLE%</h1>
       <p>Uptime: %UPTIME%</p>
     </body>
   </html>
   )DFTE";
   ```

2. In your sketch, register placeholders and stream them out:

   ```
   #include <TemplateEngine.h>

   PlaceholderRegistry registry;
   TemplateContext ctx;

   void setup() {
     Serial.begin(115200);

     registry.registerProgmemData(PSTR("%APP_TITLE%"), PSTR("DFTE Quickstart"));
     registry.registerRamData(PSTR("%UPTIME%"), [](PlaceholderWriter& w) {
       static char buffer[16];
       snprintf(buffer, sizeof(buffer), "%lus", millis() / 1000);
       w.write(buffer);
     });
     registry.registerProgmemTemplate(PSTR("%ROOT%"), ROOT_TEMPLATE_PROGMEM);

     ctx.setRegistry(&registry);
     TemplateRenderer::initializeContext(ctx, PSTR("%ROOT%"));
   }

   void loop() {
     static uint8_t buffer[128];
     if (!TemplateRenderer::isComplete(ctx) && !TemplateRenderer::hasError(ctx)) {
       size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
       Serial.write(buffer, written);
     }
   }
   ```

3. Open the serial monitor (or send chunks to `AsyncWebServer`) to watch the template stream without ever allocating the full HTML in RAM.

## Why DFTE vs Alternatives?

| Approach | Streaming | Dynamic data | Template reuse | Notes |
|----------|-----------|--------------|---------------|-------|
| DFTE (this library) | ✅ Streams in 128–512 byte chunks | ✅ Registry handles placeholders, conditionals, iterators | ✅ PROGMEM templates shared across requests | Built for async HTTP; keeps RAM usage predictable |
| Static SPIFFS/LittleFS files | ❌ Full file send only | ⚠️ Requires pre-generated HTML | ✅ Stored once on flash filesystem | Great for static assets, not ideal for live telemetry |
| Arduino `String` concatenation | ❌ Concatenates into one RAM buffer | ✅ Manual `String` inserts | ❌ Template duplicated per build | Fast to prototype but fragments heap on longer sketches |
| AsyncWebServer template callback | ⚠️ Streams callback output but copies per placeholder | ✅ Values supplied in callback | ❌ No shared layout or nesting support | Fine for small pages; scales poorly with complex UIs |
| Server-side proxy (external backend) | ✅ Offloaded to external service | ✅ Managed by backend | ❌ Device ships only proxy stub | Requires constant connectivity and extra infrastructure |

## Core Types & API
- `PlaceholderRegistry`
  - `registerProgmemData(const char*, const char*)` – link `%TOKEN%` to flash-resident data.
  - `registerRamData(const char*, PlaceholderDataGetter)` – provide dynamic strings from getters.
  - `registerProgmemTemplate(const char*, const char*)` – nest other templates.
  - `registerDynamicTemplate(const char*, const DynamicTemplateDescriptor*)` – compute template fragments at render time.
  - `registerConditional(const char*, const ConditionalDescriptor*)` – choose between delegates (`TRUE_BRANCH`, `FALSE_BRANCH`, `SKIP`).
  - `registerIterator(const char*, const IteratorDescriptor*)` – stream repeated sections item-by-item.
  - `getPlaceholder`, `getCount`, `clear` – inspection/utilities used throughout the tests.

- `TemplateContext`
  - Holds the render stack, buffers, and statistics.
  - `setRegistry(PlaceholderRegistry*)` – inject the registry you populated.
  - `reset()` – reuse the context without re-allocating buffers.
  - `isComplete()`, `hasError()`, `getStateString()` – status helpers.

- `TemplateRenderer`
  - `initializeContext(TemplateContext&, const char*, bool templateInProgmem = true)` – prime the context with the root template.
  - `renderNextChunk(TemplateContext&, uint8_t* buf, size_t len)` – stream out the next chunk; returns written bytes.
  - `isComplete(const TemplateContext&)`, `hasError(const TemplateContext&)` – convenience checks.

- `DeviceFrameworkTemplateEngineDebug`
  - Optional logging interface; create a `DeviceFrameworkTemplateEngineLogger` subclass and call `deviceFrameworkTemplateEngineEnableLogging(&logger)`.

All public headers are re-exported from `TemplateEngine.h`, so typical sketches only include that file.

## Usage Overview
1. Include `TemplateEngine.h`.

   ```
   #include <TemplateEngine.h>
   ```

2. Register placeholders on a `PlaceholderRegistry`.

   ```
   PlaceholderRegistry registry;
   registry.registerProgmemData(PSTR("%APP_TITLE%"), PSTR("DFTE Dashboard"));
   registry.registerRamData(PSTR("%UPTIME%"), [](PlaceholderWriter& w) {
     static char buffer[16];
     snprintf(buffer, sizeof(buffer), "%lus", millis() / 1000);
     w.write(buffer);
   });
   registry.registerProgmemTemplate(PSTR("%ROOT%"), ROOT_TEMPLATE_PROGMEM);
   ```

3. Attach the registry to a `TemplateContext`.

   ```
   TemplateContext ctx;
   ctx.setRegistry(&registry);
   ```

4. Feed the root template to `TemplateRenderer::initializeContext`.

   ```
   TemplateRenderer::initializeContext(ctx, PSTR("%ROOT%"));
   ```

5. Loop on `renderNextChunk` until `TemplateRenderer::isComplete(ctx)` is `true`.

   ```
   uint8_t buffer[128];
   while (!TemplateRenderer::isComplete(ctx) && !TemplateRenderer::hasError(ctx)) {
     size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
     Serial.write(buffer, written);
   }
   ```

6. Optionally reuse the same context for additional templates by calling `ctx.reset()`.

   ```
   ctx.reset();
   TemplateRenderer::initializeContext(ctx, PSTR("%DETAIL_PANEL%"));
   ```

### Async Streaming Pattern

When serving requests with ESPAsyncWebServer, give every request its own `TemplateContext` so chunked rendering cannot be corrupted by overlapping clients. Build and cache your `PlaceholderRegistry` once during setup, then share it across handlers. The same pattern powers the DeviceFramework web UI and the DFTE examples:

```cpp
/** Global registry prepared during setup() */
std::shared_ptr<PlaceholderRegistry> registry;

void setupRegistry() {
  registry = std::make_shared<PlaceholderRegistry>();
  registry->registerProgmemData(PSTR("%APP_TITLE%"), PSTR("DFTE Async Portal"));
  registry->registerRamData(PSTR("%UPTIME%"), [](PlaceholderWriter& w) {
    static char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lus", millis() / 1000);
    w.write(buffer);
  });
  registry->registerProgmemTemplate(PSTR("%ROOT%"), ROOT_TEMPLATE_PROGMEM);
}

void streamTemplate(AsyncWebServerRequest* request, const char* rootTemplate) {
  auto ctx = std::make_shared<TemplateContext>();
  ctx->setRegistry(registry.get());
  TemplateRenderer::initializeContext(*ctx, rootTemplate);

  request->onDisconnect([ctx]() mutable { ctx.reset(); });

  AsyncWebServerResponse* response = request->beginChunkedResponse(
      "text/html; charset=utf-8",
      [ctx](uint8_t* buffer, size_t maxLen, size_t) mutable -> size_t {
        if (!ctx) {
          return 0;
        }

        size_t written = TemplateRenderer::renderNextChunk(*ctx, buffer, maxLen);
        if (!written || TemplateRenderer::isComplete(*ctx) || TemplateRenderer::hasError(*ctx)) {
          ctx.reset();              // prevent cross-request pollution
        }
        return written;
      });

  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");

  request->send(response);
}

void setup() {
  setupRegistry();
  server.on("/", [](AsyncWebServerRequest* request) {
    streamTemplate(request, PSTR("%ROOT%"));
  });
  server.begin();
}
```

1. Build and cache the `PlaceholderRegistry` during startup so heavy PROGMEM registration runs once.
2. Allocate a request-scoped `TemplateContext`, initialise it with the shared registry, and render inside the chunked callback.
3. Tear everything down on completion or disconnect to avoid state bleed between clients.

### Template Syntax Reference

- `%PLACEHOLDER%` – Register with `registerProgmemData`, `registerRamData`, or `registerProgmemTemplate`.
- `%DYNAMIC%` – Pair with `registerDynamicTemplate` to supply template text and length at render time.
- `%CONDITIONAL%` – Use `registerConditional` and return `TRUE_BRANCH`, `FALSE_BRANCH`, or `SKIP` from your evaluator.
- `%ITERATOR%` – Register an `IteratorDescriptor` that opens, yields items via `IteratorItemView`, and closes when complete.
- Nest templates freely; DFTE manages stack depth up to `DFTE_MAX_STACK_DEPTH_DEFAULT` by default.

### Buildable Examples

All demos under `examples/` are standalone PlatformIO projects that use the library via `lib_extra_dirs`. Each contains a `platformio.ini` with ready-to-build environments, so you can compile and upload without touching your primary application.

- `examples/AsyncDashboardDemo/` – Full SoftAP dashboard with iterators, conditionals, and runtime telemetry.
- `examples/StreamingAsync/` – Minimal captive portal that streams the template directly to the HTTP response.
- `examples/NestedLayouts/` – Demonstrates layout stacking, partials, and conditional fragments.
- `examples/HelloPlaceholder/` – Smallest possible sketch that renders a single placeholder.

Typical workflow (replace the path/env as needed):

```
# Build
pio run -d examples/StreamingAsync -e example_esp32

# Flash
pio run -d examples/StreamingAsync -e example_esp32 -t upload

# Monitor (optional)
pio run -d examples/StreamingAsync -e example_esp32 -t monitor
```

ESP8266 variants use the `example_esp8266` or `dashboard_esp8266` environments, while ESP32 boards use `example_esp32` or `dashboard_esp32`. Update Wi-Fi credentials inside each example’s `src/main.cpp`, then connect to the serial monitor or SoftAP as documented in the per-example README.

Refer to the Unity tests in `test/test_template_engine/tests` for exhaustive combinations of placeholders, nested templates, conditionals, and iterators.

### Memory & Configuration Knobs

Tune DFTE by defining these macros **before** including `TemplateEngine.h` (or via PlatformIO `build_flags = -DNAME=value`). Larger values increase RAM or flash use, so bump them only when necessary.

- `DFTE_BUFFER_SIZE_DEFAULT` (512 bytes) – streaming buffer inside `TemplateContext`.
- `DFTE_MAX_STACK_DEPTH_DEFAULT` (16) – maximum nested placeholder/template depth.
- `DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT` (24) – length limit for placeholder tokens.
- `DFTE_MAX_PLACEHOLDERS_DEFAULT` (16) – default capacity when constructing `PlaceholderRegistry`.
- `DFTE_PROGMEM_CHUNK_SIZE_DEFAULT` (512) – copy window when reading PROGMEM data.
- `DFTE_RAM_CHUNK_SIZE_DEFAULT` (128) – chunk size for RAM-based getters.
- `DFTE_MAX_ITERATIONS_DEFAULT` (50) – safety cap for iterator placeholders.

```
// Increase iterator cap to 100 and expand streaming buffer
#define DFTE_MAX_ITERATIONS_DEFAULT 100
#define DFTE_BUFFER_SIZE_DEFAULT 768
#include <TemplateEngine.h>
```

**Using DFTE inside DeviceFramework**  
DeviceFramework projects generate a `DeviceFrameworkTemplateConfig.h` that is re-exported by `DeviceFrameworkConfig.h`. Define your defaults there and make sure `DeviceFrameworkConfig.h` is included before `TemplateEngine.h`; DFTE detects the `CONFIG_template*` symbols and swaps them in automatically.

```
// DeviceFrameworkTemplateConfig.h
#pragma once

#define CONFIG_templateBufferSize_default       768
#define CONFIG_templateStackDepth_default        24
#define CONFIG_templateMaxTemplatePlaceholders_default 24
#define CONFIG_templateProgmemChunkSize_default 1024
#define CONFIG_templateRamChunkSize_default      256
#define CONFIG_templateMaxIterations_default      80
```

When the core pulls in `DeviceFrameworkConfig.h`, all templates compiled in that project will inherit these values without further changes.

## PlatformIO Usage

### Consume as a Dependency
Add DFTE to your project’s `platformio.ini` using the Git repository URL:

```
lib_deps =
    https://github.com/alexhopeoconnor/DFTE
```

Pin to a specific release tag if you need reproducible builds (for example `https://github.com/alexhopeoconnor/DFTE#v1.0.0`), or keep the `lib_deps` entry as-is to track the latest main branch during development.

### Local Development & Testing
```
pio test -e test_template_engine                         # run full Unity suite
pio test -e test_template_engine -f test_template_renderer   # single test file
pio run -e test_template_engine                          # compile without running tests
```
The default environment targets `d1_mini` (ESP8266) with `test_build_src = yes` so library sources are included during builds.

### Debug Logging

DFTE’s logger is opt-in and costs nothing until you enable it. Implement `DeviceFrameworkTemplateEngineLogger`, register it once, and all internal `DFTE_LOG_*` calls stream through your logger.

```
#include <TemplateEngine.h>
#include <DeviceFrameworkTemplateEngineDebug.h>

class SerialLogger : public DeviceFrameworkTemplateEngineLogger {
public:
  void error(const String& msg) override { Serial.println("[DFTE][E] " + msg); }
  void warn(const String& msg) override  { Serial.println("[DFTE][W] " + msg); }
  void info(const String& msg) override  { Serial.println("[DFTE][I] " + msg); }
  void debug(const String& msg) override { Serial.println("[DFTE][D] " + msg); }
};

SerialLogger logger;

void setup() {
  Serial.begin(115200);
  deviceFrameworkTemplateEngineEnableLogging(&logger);
}
```

Use `deviceFrameworkTemplateEngineDisableLogging()` to silence output or `deviceFrameworkTemplateEngineIsLoggingEnabled()` to inspect the current state.

## License

This project is released under the MIT License. See `LICENSE` for details.
