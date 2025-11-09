#ifndef NESTED_TEMPLATES_H
#define NESTED_TEMPLATES_H

#include <Arduino.h>

// One level nesting
const char PROGMEM nested_one_level_outer[] = "Outer: %INNER%";
const char PROGMEM nested_one_level_inner[] = "Inner content";

// Two levels nesting
const char PROGMEM nested_two_levels_outer[] = "Level1: %LEVEL2%";
const char PROGMEM nested_two_levels_mid[] = "Level2: %LEVEL3%";
const char PROGMEM nested_two_levels_inner[] = "Level3 content";

// Three levels nesting
const char PROGMEM nested_three_levels_outer[] = "Outer: %MID%";
const char PROGMEM nested_three_levels_mid[] = "Mid: %INNER%";
const char PROGMEM nested_three_levels_inner[] = "Inner: %DEEP%";
const char PROGMEM nested_three_levels_deep[] = "Deep content";

// Four levels nesting
const char PROGMEM nested_four_levels_outer[] = "L1: %L2%";
const char PROGMEM nested_four_levels_level2[] = "L2: %L3%";
const char PROGMEM nested_four_levels_level3[] = "L3: %L4%";
const char PROGMEM nested_four_levels_level4[] = "L4 content";

// Large web-style templates to simulate WebInterface-sized payloads (intentionally fuzzed)
const char PROGMEM simulated_web_outer[] = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>%TITLE%</title>
    <style>%STYLES%</style>
</head>
<body data-theme="%THEME%">
    <section id="app-shell">
        %HEADER%
        <div class="app-layout">
            %SIDEBAR%
            <main class="app-main">
                %CONTENT%
            </main>
        </div>
        %FOOTER%
    </section>
    <script defer>%SCRIPTS%</script>
</body>
</html>
)rawliteral";

const char PROGMEM simulated_web_header[] = R"rawliteral(
<header class="app-header">
    <div class="branding">
        <img src="data:image/webp;base64,%LOGO_BASE64%" alt="Elixir" class="branding-logo">
        <div class="branding-copy">
            <h1>%TITLE%</h1>
            <p class="tagline">Powering ambient devices</p>
        </div>
    </div>
    %NAV%
</header>
)rawliteral";

const char PROGMEM simulated_web_nav[] = R"rawliteral(
<nav class="app-nav">
    <ul>
        <li><a href="#overview" class="nav-link">Overview</a></li>
        <li><a href="#telemetry" class="nav-link">Telemetry</a></li>
        <li><a href="#actions" class="nav-link">Actions</a></li>
    </ul>
</nav>
)rawliteral";

const char PROGMEM simulated_web_sidebar[] = R"rawliteral(
<aside class="app-sidebar">
    <h2>Quick Metrics</h2>
    <ul>
        <li>WiFi RSSI: <span id="metric-rssi">%RSSI%</span>dBm</li>
        <li>Heap Free: <span id="metric-heap">%HEAP%</span> bytes</li>
        <li>Uptime: <span id="metric-uptime">%UPTIME%</span></li>
    </ul>
</aside>
)rawliteral";

const char PROGMEM simulated_web_content[] = R"rawliteral(
<section id="overview" class="panel">
    <h2>Device Overview</h2>
    <p id="device-summary">Preparing device snapshot...</p>
</section>
<section id="telemetry" class="panel">
    <h2>Telemetry</h2>
    <div class="telemetry-grid">
        <div class="telemetry-card">
            <h3>Network</h3>
            <p>SSID: <span id="wifi-ssid">pending</span></p>
            <p>IP: <span id="wifi-ip">pending</span></p>
        </div>
        <div class="telemetry-card">
            <h3>MQTT</h3>
            <p>Status: <span id="mqtt-status">pending</span></p>
            <p>Broker: <span id="mqtt-broker">pending</span></p>
        </div>
        <div class="telemetry-card">
            <h3>Sensors</h3>
            <p>Temperature: <span id="sensor-temp">--</span></p>
            <p>Humidity: <span id="sensor-humidity">--</span></p>
        </div>
    </div>
</section>
<section id="actions" class="panel">
    <h2>Automation Actions</h2>
    <button class="btn" onclick="queueAction('restart')">Restart</button>
    <button class="btn" onclick="queueAction('factory-reset')">Factory Reset</button>
</section>
)rawliteral";

const char PROGMEM simulated_web_footer[] = R"rawliteral(
<footer class="app-footer">
    <small>&copy; 2024 Elixir DeviceFramework Labs</small>
</footer>
)rawliteral";

const char PROGMEM simulated_web_scripts[] = R"rawliteral(
function queueAction(action) {
    window.dispatchEvent(new CustomEvent('device-action', { detail: action }));
}
window.addEventListener('load', () => {
    document.body.dataset.ready = 'true';
});
)rawliteral";

// Nested template with placeholders
const char PROGMEM nested_with_placeholder_outer[] = "Outer: %NESTED%";
const char PROGMEM nested_with_placeholder_inner[] = "Inner: %PLACEHOLDER%";

// Nested template with multiple placeholders
const char PROGMEM nested_multi_outer[] = "%HEADER%\n%CONTENT%\n%FOOTER%";
const char PROGMEM nested_multi_header[] = "<header>%TITLE%</header>";
const char PROGMEM nested_multi_content[] = "<main>%BODY%</main>";
const char PROGMEM nested_multi_footer[] = "<footer>Footer</footer>";

// Deep nesting for stack depth testing
const char PROGMEM deep_nest_level1[] = "L1: %L2%";
const char PROGMEM deep_nest_level2[] = "L2: %L3%";
const char PROGMEM deep_nest_level3[] = "L3: %L4%";
const char PROGMEM deep_nest_level4[] = "L4: %L5%";
const char PROGMEM deep_nest_level5[] = "L5: %L6%";
const char PROGMEM deep_nest_level6[] = "L6: %L7%";
const char PROGMEM deep_nest_level7[] = "L7: %L8%";
const char PROGMEM deep_nest_level8[] = "L8: %L9%";
const char PROGMEM deep_nest_level9[] = "L9: %L10%";
const char PROGMEM deep_nest_level10[] = "L10: %L11%";
const char PROGMEM deep_nest_level11[] = "L11: %L12%";
const char PROGMEM deep_nest_level12[] = "L12: %L13%";
const char PROGMEM deep_nest_level13[] = "L13: %L14%";
const char PROGMEM deep_nest_level14[] = "L14: %L15%";
const char PROGMEM deep_nest_level15[] = "L15: %L16%";
const char PROGMEM deep_nest_level16[] = "L16: content";

#endif // NESTED_TEMPLATES_H

