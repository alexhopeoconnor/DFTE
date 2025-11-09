#include <Arduino.h>
#include <TemplateEngine.h>
#include <memory>
#include <cstring>

#include "../../common/templates/LayoutSnippets.h"

namespace {

struct SubsystemStatus {
  const char* name;
  const char* detail;
  const char* severityClass;
};

constexpr SubsystemStatus SUBSYSTEMS[] = {
    {"Wi-Fi", "Connected", "ok"},
    {"MQTT", "Disconnected", "warn"},
    {"Storage", "Healthy", "ok"},
    {"OTA", "Idle", "info"},
};
constexpr size_t SUBSYSTEM_COUNT = sizeof(SUBSYSTEMS) / sizeof(SubsystemStatus);

bool maintenanceMode = false;

// Templates
inline const char PROGMEM BASE_TEMPLATE[] = R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>%PAGE_TITLE%</title>
    <style>%CSS%</style>
  </head>
  <body>
    %HEADER%
    <section>
      <h2>Environment</h2>
      <p><strong>Firmware:</strong> %FIRMWARE_VERSION%</p>
      <p><strong>Boot Count:</strong> %BOOT_COUNT%</p>
    </section>
    <section>
      <h2>Maintenance</h2>
      %MAINTENANCE_BANNER%
    </section>
    <section>
      <h2>Subsystems</h2>
      <ul class="subsystems">
        %SUBSYSTEM_LIST%
      </ul>
    </section>
    %FOOTER%
  </body>
</html>
)HTML";

inline const char PROGMEM SUBSYSTEM_ITEM_TEMPLATE[] = R"HTML(
<li class="%SEVERITY%">
  <strong>%NAME%</strong>
  <span>%DETAIL%</span>
</li>
)HTML";

inline const char PROGMEM MAINTENANCE_TRUE_TEMPLATE[] = R"HTML(
<div class="notice warn">System in maintenance mode. Automations disabled.</div>
)HTML";

inline const char PROGMEM MAINTENANCE_FALSE_TEMPLATE[] = R"HTML(
<div class="notice ok">All services operating normally.</div>
)HTML";

// RAM data
static String bootCountBuffer;
static uint32_t bootCount = 42;

const char* getFirmwareVersion() { return "2.3.1"; }

const char* getBootCount() {
  bootCountBuffer = String(bootCount);
  return bootCountBuffer.c_str();
}

// Conditional descriptor
struct MaintenanceState {
  bool enabled;
} maintenanceState{maintenanceMode};

ConditionalBranchResult evaluateMaintenance(void* userData) {
  auto* state = static_cast<MaintenanceState*>(userData);
  return state->enabled ? ConditionalBranchResult::TRUE_BRANCH : ConditionalBranchResult::FALSE_BRANCH;
}

inline const ConditionalDescriptor MAINTENANCE_DESCRIPTOR = {
    evaluateMaintenance,
    "%MAINTENANCE_TRUE%",
    "%MAINTENANCE_FALSE%",
    (void*)&maintenanceState};

// Iterator plumbing
struct SubsystemIteratorState {
  size_t index = 0;
};

SubsystemIteratorState iteratorState;
PlaceholderEntry subsystemOverrides[SUBSYSTEM_COUNT][3];

void initialiseOverrides() {
  for (size_t i = 0; i < SUBSYSTEM_COUNT; ++i) {
    const SubsystemStatus& status = SUBSYSTEMS[i];

    PlaceholderEntry& name = subsystemOverrides[i][0];
    strncpy(name.name, "%NAME%", sizeof(name.name) - 1);
    name.type = PlaceholderType::RAM_DATA;
    name.data = status.name;
    name.getLength = DeviceFrameworkPlaceholderRegistry::getRamLength;

    PlaceholderEntry& detail = subsystemOverrides[i][1];
    strncpy(detail.name, "%DETAIL%", sizeof(detail.name) - 1);
    detail.type = PlaceholderType::RAM_DATA;
    detail.data = status.detail;
    detail.getLength = DeviceFrameworkPlaceholderRegistry::getRamLength;

    PlaceholderEntry& severity = subsystemOverrides[i][2];
    strncpy(severity.name, "%SEVERITY%", sizeof(severity.name) - 1);
    severity.type = PlaceholderType::RAM_DATA;
    severity.data = status.severityClass;
    severity.getLength = DeviceFrameworkPlaceholderRegistry::getRamLength;
  }
}

void* iteratorOpen(void* userData) {
  initialiseOverrides();
  auto* state = static_cast<SubsystemIteratorState*>(userData);
  state->index = 0;
  return state;
}

IteratorStepResult iteratorNext(void* handle, IteratorItemView& view) {
  auto* state = static_cast<SubsystemIteratorState*>(handle);
  if (state->index >= SUBSYSTEM_COUNT) {
    return IteratorStepResult::COMPLETE;
  }

  view.templateData = SUBSYSTEM_ITEM_TEMPLATE;
  view.templateLength = strlen_P(SUBSYSTEM_ITEM_TEMPLATE);
  view.templateIsProgmem = true;
  view.placeholders = subsystemOverrides[state->index];
  view.placeholderCount = 3;
  state->index++;
  return IteratorStepResult::ITEM_READY;
}

void iteratorClose(void*) {}

IteratorDescriptor iteratorDescriptor = {
    iteratorOpen,
    iteratorNext,
    iteratorClose,
    &iteratorState};

std::shared_ptr<PlaceholderRegistry> buildRegistry() {
  auto registry = std::make_shared<PlaceholderRegistry>();
  registry->registerProgmemData("%CSS%", DFTEExamples::SHARED_CSS);
  registry->registerProgmemTemplate("%HEADER%", DFTEExamples::SHARED_HEADER);
  registry->registerProgmemTemplate("%FOOTER%", DFTEExamples::SHARED_FOOTER);

  registry->registerRamData("%PAGE_TITLE%", []() -> const char* { return "DFTE Nested Layouts"; });
  registry->registerRamData("%TAGLINE%", []() -> const char* { return "Composing templates with conditionals and iterators"; });
  registry->registerRamData("%FIRMWARE_VERSION%", getFirmwareVersion);
  registry->registerRamData("%BOOT_COUNT%", getBootCount);

  registry->registerConditional("%MAINTENANCE_BANNER%", &MAINTENANCE_DESCRIPTOR);
  registry->registerProgmemTemplate("%MAINTENANCE_TRUE%", MAINTENANCE_TRUE_TEMPLATE);
  registry->registerProgmemTemplate("%MAINTENANCE_FALSE%", MAINTENANCE_FALSE_TEMPLATE);

  registry->registerIterator("%SUBSYSTEM_LIST%", &iteratorDescriptor);

  return registry;
}

void renderToSerial() {
  auto registry = buildRegistry();
  TemplateContext ctx;
  ctx.setRegistry(registry.get());
  TemplateRenderer::initializeContext(ctx, BASE_TEMPLATE);

  uint8_t buffer[128];
  while (!TemplateRenderer::isComplete(ctx) && !TemplateRenderer::hasError(ctx)) {
    size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
    if (!written) {
      break;
    }
    Serial.write(buffer, written);
  }
  Serial.println();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait */ }

  Serial.println();
  Serial.println(F("=== DFTE Nested Layouts Example ==="));

  renderToSerial();
}

void loop() {
  // Nothing to do in loop for this example.
}

