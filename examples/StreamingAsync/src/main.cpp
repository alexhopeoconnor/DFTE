#include <Arduino.h>
#include <TemplateEngine.h>
#include <memory>
#include <cstring>
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

namespace {

constexpr const char* AP_SSID =
#if defined(ESP32)
    "DFTE-Portal-ESP32";
#else
    "DFTE-Portal-8266";
#endif
constexpr const char* AP_PASSWORD = "dfte-demo";
constexpr byte DNS_PORT = 53;

DNSServer dnsServer;

AsyncWebServer server(80);
std::shared_ptr<PlaceholderRegistry> registry;

inline const char PROGMEM PAGE_TEMPLATE[] = R"HTML(
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
      <h2>Device Snapshot</h2>
      <p><strong>Uptime:</strong> %UPTIME%</p>
      <p><strong>Connected Clients:</strong> %CLIENT_COUNT%</p>
    </section>
    %FOOTER%
  </body>
</html>
)HTML";

String uptimeBuffer;
String clientBuffer;

const char* getUptime() {
  uptimeBuffer = String(millis() / 1000) + "s";
  return uptimeBuffer.c_str();
}

const char* getClientCount() {
#if defined(ESP32)
  clientBuffer = String(WiFi.softAPgetStationNum());
#else
  clientBuffer = String(WiFi.softAPgetStationNum());
#endif
  return clientBuffer.c_str();
}

void initialiseRegistry(PlaceholderRegistry& registryRef) {
  registryRef.clear();
  registryRef.registerProgmemData("%CSS%", DFTEExamples::SHARED_CSS);
  registryRef.registerProgmemTemplate("%HEADER%", DFTEExamples::SHARED_HEADER);
  registryRef.registerProgmemTemplate("%FOOTER%", DFTEExamples::SHARED_FOOTER);
  registryRef.registerRamData("%PAGE_TITLE%", []() -> const char* { return "DFTE Streaming Async"; });
  registryRef.registerRamData("%TAGLINE%", []() -> const char* { return "Chunked rendering with ESPAsyncWebServer"; });
  registryRef.registerRamData("%UPTIME%", getUptime);
  registryRef.registerRamData("%CLIENT_COUNT%", getClientCount);
}

void streamTemplate(AsyncWebServerRequest* request,
                    std::shared_ptr<PlaceholderRegistry> registryPtr,
                    const char* tpl) {
  auto ctx = std::make_shared<TemplateContext>();
  ctx->setRegistry(registryPtr.get());
  TemplateRenderer::initializeContext(*ctx, tpl);

  request->onDisconnect([ctx]() mutable {
    ctx.reset();
  });

  AsyncWebServerResponse* response = request->beginChunkedResponse(
      "text/html; charset=utf-8",
      [ctx](uint8_t* buffer, size_t maxLen, size_t) mutable -> size_t {
        if (!ctx) {
          return 0;
        }

        size_t written = TemplateRenderer::renderNextChunk(*ctx, buffer, maxLen);
        if (!written || TemplateRenderer::isComplete(*ctx) || TemplateRenderer::hasError(*ctx)) {
          ctx.reset();
        }
        return written;
      });

  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

void handleRoot(AsyncWebServerRequest* request) {
  if (!registry) {
    registry = std::make_shared<PlaceholderRegistry>();
    initialiseRegistry(*registry);
  }
  streamTemplate(request, registry, PAGE_TEMPLATE);
}

void startCaptivePortal() {
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMask);
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

}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait */ }

  Serial.println();
  Serial.println(F("=== DFTE Streaming Async Example ==="));

  registry = std::make_shared<PlaceholderRegistry>();
  initialiseRegistry(*registry);
  startCaptivePortal();

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleRoot);
  server.begin();
  Serial.println(F("HTTP server listening on port 80"));
}

void loop() {
  dnsServer.processNextRequest();
}

