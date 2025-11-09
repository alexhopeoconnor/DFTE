# Streaming Async

Small ESPAsyncWebServer demo that exposes a captive portal over a SoftAP and renders a DFTE template directly to the HTTP response stream. It highlights:

- Per-request `TemplateContext` ownership to avoid concurrent request clashes
- PROGMEM template with shared CSS/header/footer snippets
- Runtime data from RAM getters (uptime, connected station count)
- Captive portal DNS redirect so clients automatically receive the dashboard

## Build

```
pio run -d examples/StreamingAsync -e example_esp8266
pio run -d examples/StreamingAsync -e example_esp32
```

Flash the board, connect to the SoftAP announced by the device (`DFTE-Portal-8266` or `DFTE-Portal-ESP32`, password `dfte-demo`), and your browser should automatically present the streamed dashboard. If it does not, navigate manually to `http://192.168.4.1/`.

