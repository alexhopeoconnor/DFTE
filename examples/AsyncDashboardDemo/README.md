# Async Dashboard Demo

Full-featured DFTE demo that combines nested templates, iterators, and a captive portal served via ESPAsyncWebServer. The device exposes its own SoftAP so multiple users can connect and view the dashboard without additional infrastructure.

Highlights:
- SoftAP + DNS captive portal (`DFTE-Dashboard-8266` or `DFTE-Dashboard-ESP32`, password `dfte-dashboard`)
- Aggregated metrics section populated from RAM getters (uptime, device count, client count)
- Iterator-driven device table with per-entry overrides
- Streaming HTTP responses to avoid buffering the entire page in RAM

## Build

```
pio run -d examples/AsyncDashboardDemo -e dashboard_esp8266
pio run -d examples/AsyncDashboardDemo -e dashboard_esp32
```

## Run

1. Flash your target board (`pio run -d examples/AsyncDashboardDemo -e <env> -t upload --upload-port <port>`).
2. Open the serial monitor at 115200 baud to confirm the SoftAP credentials.
3. Connect to the advertised network and browse to `http://192.168.4.1/`. The captive portal should automatically redirect; if not, navigate manually.

