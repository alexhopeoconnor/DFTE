# Streaming Async

This ESPAsyncWebServer example exposes a captive portal over a SoftAP and streams a DFTE template directly to each HTTP response. It demonstrates:

- one request-scoped `TemplateContext` per response;
- a PROGMEM page with shared CSS/header/footer snippets;
- runtime getters for uptime and connected-station count;
- a captive-portal DNS redirect for browsers that support it.

## Build

```bash
pio run -d examples/StreamingAsync -e example_esp8266
pio run -d examples/StreamingAsync -e example_esp32
```

## Run

1. Flash your target with `pio run -d examples/StreamingAsync -e <env> -t upload --upload-port <port>`.
2. Open the serial monitor at 115200 baud and confirm the SoftAP credentials.
3. Connect to `DFTE-Portal-8266` or `DFTE-Portal-ESP32` using password `dfte-demo`.
4. Browse to `http://192.168.4.1/` if the captive portal does not open automatically.

See the shared [examples guide](../README.md) and [async web guidance](../../docs/ASYNC_WEB.md).
