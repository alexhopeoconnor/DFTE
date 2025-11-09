#include <Arduino.h>
#include <TemplateEngine.h>

namespace {

static String deviceName = "DFTE Getting Started";
static String buildId    = "v1.0.0";

const char SIMPLE_TEMPLATE[] = R"TPL(
Device: %DEVICE_NAME%
Build: %BUILD_ID%

DFTE lets you reuse this template across Serial, HTTP, or any other transport.
)TPL";

const char* getDeviceName() { return deviceName.c_str(); }
const char* getBuildId()    { return buildId.c_str(); }

}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for USB CDC */ }

  Serial.println();
  Serial.println(F("=== DFTE Hello Placeholder ==="));

  PlaceholderRegistry registry;
  registry.registerRamData("%DEVICE_NAME%", getDeviceName);
  registry.registerRamData("%BUILD_ID%", getBuildId);

  TemplateContext ctx;
  ctx.setRegistry(&registry);
  TemplateRenderer::initializeContext(ctx, SIMPLE_TEMPLATE, false);

  uint8_t buffer[64];
  while (!TemplateRenderer::isComplete(ctx) && !TemplateRenderer::hasError(ctx)) {
    size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
    if (!written) {
      break;
    }
    Serial.write(buffer, written);
  }

  Serial.println(F("\nRendering complete."));
}

void loop() {
  // Nothing else to do in the basic example.
}

