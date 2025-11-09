#include <Arduino.h>
#include <TemplateEngine.h>
#include <cstring>
#include <memory>
#include <DNSServer.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "../../common/templates/LayoutSnippets.h"

AsyncWebServer server(80);
DNSServer dnsServer;
std::shared_ptr<PlaceholderRegistry> registry;

constexpr byte DNS_PORT = 53;
constexpr const char* AP_PASSWORD = "dfte-dashboard";
#if defined(ESP32)
constexpr const char* AP_SSID = "DFTE-Dashboard-ESP32";
#else
constexpr const char* AP_SSID = "DFTE-Dashboard-8266";
#endif

// ---------------------------------------------------------------------------
// Template definitions (kept in PROGMEM to reduce RAM usage)
// ---------------------------------------------------------------------------
static const char PROGMEM kLayoutTemplate[] = R"HTML(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>%PAGE_TITLE%</title>
    <style>%GLOBAL_CSS%</style>
  </head>
  <body>
    %HEADER%
    <main>
      <section class="meta">
        <h2>Overview</h2>
        <dl>
          <dt>Connected Clients</dt><dd>%CLIENT_COUNT%</dd>
          <dt>Uptime</dt><dd>%UPTIME%</dd>
          <dt>Device Entries</dt><dd>%DEVICE_COUNT%</dd>
        </dl>
      </section>
      <section class="devices">
        <h2>Devices</h2>
        <table>
          <thead>
            <tr><th>Name</th><th>Status</th><th>Last Seen</th></tr>
          </thead>
          <tbody>%DEVICE_ROWS%</tbody>
        </table>
      </section>
    </main>
    %FOOTER%
  </body>
</html>
)HTML";

static const char PROGMEM kDeviceRowTemplate[] = R"HTML(
<tr class="%STATUS_CLASS%">
  <td>%DEVICE_NAME%</td>
  <td>%DEVICE_STATUS%</td>
  <td>%DEVICE_LAST_SEEN%</td>
</tr>
)HTML";

// ---------------------------------------------------------------------------
// Mock data sources (replace with your own business logic)
// ---------------------------------------------------------------------------
struct DeviceInfo {
  const char* name;
  const char* status;
  const char* statusClass;
  const char* lastSeen;
};

static DeviceInfo kDevices[] = {
  {"Living Room Light", "Online", "ok", "5s ago"},
  {"Garage Door", "Warning", "warn", "18s ago"},
  {"Garden Pump", "Offline", "error", "2m ago"}
};
static constexpr size_t kDeviceCount = sizeof(kDevices) / sizeof(DeviceInfo);

static String uptimeBuffer;
static String clientCountBuffer;
static String deviceCountBuffer;

const char* getPageTitle() { return "DFTE Dashboard"; }
const char* getTagline() { return "Rendered chunk-by-chunk from flash + dynamic data"; }

const char* getDeviceCount() {
  deviceCountBuffer = String(kDeviceCount);
  return deviceCountBuffer.c_str();
}

const char* getUptime() {
  uptimeBuffer = String(millis() / 1000) + "s";
  return uptimeBuffer.c_str();
}

const char* getClientCount() {
#if defined(ESP32)
  clientCountBuffer = String(WiFi.softAPgetStationNum());
#else
  clientCountBuffer = String(WiFi.softAPgetStationNum());
#endif
  return clientCountBuffer.c_str();
}

// ---------------------------------------------------------------------------
// Iterator wiring for %DEVICE_ROWS%
// ---------------------------------------------------------------------------
struct DeviceIteratorState {
  size_t index;
};

DeviceIteratorState gIteratorState;
static PlaceholderEntry gDeviceOverrides[kDeviceCount][4];
static bool gOverridesInitialized = false;

void initializeDeviceOverrides() {
  if (gOverridesInitialized) {
    return;
  }

  const char* names[4] = {
    "%DEVICE_NAME%", "%DEVICE_STATUS%", "%STATUS_CLASS%", "%DEVICE_LAST_SEEN%"
  };

  for (size_t i = 0; i < kDeviceCount; ++i) {
    const char* values[4] = {
      kDevices[i].name,
      kDevices[i].status,
      kDevices[i].statusClass,
      kDevices[i].lastSeen
    };

    for (size_t field = 0; field < 4; ++field) {
      PlaceholderEntry& entry = gDeviceOverrides[i][field];
      memset(entry.name, 0, sizeof(entry.name));
      strncpy(entry.name, names[field], sizeof(entry.name) - 1);
      entry.type = PlaceholderType::PROGMEM_DATA;
      entry.data = values[field];
      entry.getLength = DeviceFrameworkPlaceholderRegistry::getProgmemLength;
    }
  }

  gOverridesInitialized = true;
}

void* deviceIteratorOpen(void* userData) {
  auto* state = static_cast<DeviceIteratorState*>(userData);
  state->index = 0;
  initializeDeviceOverrides();
  return state;
}

IteratorStepResult deviceIteratorNext(void* handle, IteratorItemView& view) {
  auto* state = static_cast<DeviceIteratorState*>(handle);
  if (state->index >= kDeviceCount) {
    return IteratorStepResult::COMPLETE;
  }

  view.templateData = kDeviceRowTemplate;
  view.templateLength = strlen_P(kDeviceRowTemplate);
  view.templateIsProgmem = true;
  view.placeholders = gDeviceOverrides[state->index];
  view.placeholderCount = 4;

  state->index++;
  return IteratorStepResult::ITEM_READY;
}

void deviceIteratorClose(void*) {}

IteratorDescriptor gDeviceIterator = {
  .open = deviceIteratorOpen,
  .next = deviceIteratorNext,
  .close = deviceIteratorClose,
  .userData = &gIteratorState
};

// ---------------------------------------------------------------------------
// Registry + rendering helpers
// ---------------------------------------------------------------------------
void initialiseRegistry(PlaceholderRegistry& registryRef) {
  registryRef.clear();

  // Static assets and nested templates
  registryRef.registerProgmemData("%GLOBAL_CSS%", DFTEExamples::SHARED_CSS);
  registryRef.registerProgmemTemplate("%HEADER%", DFTEExamples::SHARED_HEADER);
  registryRef.registerProgmemTemplate("%FOOTER%", DFTEExamples::SHARED_FOOTER);

  // Header content
  registryRef.registerRamData("%PAGE_TITLE%", getPageTitle);
  registryRef.registerRamData("%TAGLINE%", getTagline);

  // Overview metrics
  registryRef.registerRamData("%CLIENT_COUNT%", getClientCount);
  registryRef.registerRamData("%UPTIME%", getUptime);
  registryRef.registerRamData("%DEVICE_COUNT%", getDeviceCount);

  // Iterator to populate the device table
  registryRef.registerIterator("%DEVICE_ROWS%", &gDeviceIterator);
}

void streamTemplate(AsyncWebServerRequest* request,
                    std::shared_ptr<PlaceholderRegistry> registryPtr,
                    const char* rootTemplate) {
  auto ctx = std::make_shared<TemplateContext>();
  ctx->setRegistry(registryPtr.get());
  TemplateRenderer::initializeContext(*ctx, rootTemplate);

  request->onDisconnect([ctx]() mutable {
    ctx.reset();
  });

  AsyncWebServerResponse* response = request->beginChunkedResponse(
      "text/html; charset=utf-8",
      [ctx](uint8_t* buffer, size_t maxLen, size_t /*index*/) mutable -> size_t {
        if (!ctx) {
          return 0;
        }

        size_t written = TemplateRenderer::renderNextChunk(*ctx, buffer, maxLen);

        if (!written || TemplateRenderer::isComplete(*ctx) || TemplateRenderer::hasError(*ctx)) {
          ctx.reset();
        }

        return written;
      });

  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");

  request->send(response);
}

void handleDashboard(AsyncWebServerRequest* request) {
  if (!registry) {
    registry = std::make_shared<PlaceholderRegistry>(24);
    initialiseRegistry(*registry);
  }
  streamTemplate(request, registry, kLayoutTemplate);
}

void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netmask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netmask);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.println(F("Captive portal active"));
  Serial.print(F("SSID: "));
  Serial.println(AP_SSID);
  Serial.print(F("Password: "));
  Serial.println(AP_PASSWORD);
  Serial.print(F("Portal IP: "));
  Serial.println(apIP);
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait */ }

  Serial.println();
  Serial.println(F("=== DFTE Async Dashboard Demo ==="));

  registry = std::make_shared<PlaceholderRegistry>(24);
  initialiseRegistry(*registry);
  startCaptivePortal();

  server.on("/", HTTP_GET, handleDashboard);
  server.onNotFound(handleDashboard);
  server.begin();

  Serial.println(F("Dashboard available at http://192.168.4.1/"));
}

void loop() {
  dnsServer.processNextRequest();
}

