#ifndef PLACEHOLDER_TEMPLATES_H
#define PLACEHOLDER_TEMPLATES_H

#include <Arduino.h>

// PROGMEM_DATA placeholder
const char PROGMEM progmem_data_template[] = "CSS: %STYLES%";

// PROGMEM_TEMPLATE placeholder
const char PROGMEM nested_template_template[] = "Header: %HEADER%";

// RAM_DATA placeholder
const char PROGMEM ram_data_template[] = "Title: %PAGE_TITLE%";

// Mixed placeholders
const char PROGMEM mixed_placeholders_template[] = 
    "%HEADER%\nTitle: %PAGE_TITLE%\n%STYLES%\n%FOOTER%";

// Multiple PROGMEM_DATA placeholders
const char PROGMEM multiple_progmem_template[] = 
    "%CSS%\n%JS%\n%FAVICON%";

// Multiple RAM_DATA placeholders
const char PROGMEM multiple_ram_template[] = 
    "Title: %TITLE%\nSubtitle: %SUBTITLE%\nDescription: %DESC%";

// Test data for PROGMEM placeholders
const char PROGMEM test_css_data[] = "body { color: red; }";
const char PROGMEM test_js_data[] = "console.log('test');";
const char PROGMEM test_favicon_data[] = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

// Test templates for nested templates
const char PROGMEM test_header_template[] = "<header>%LOGO%</header>";
const char PROGMEM test_footer_template[] = "<footer>Footer</footer>";
const char PROGMEM test_logo_template[] = "<img src=\"%LOGO_URL%\">";

#endif // PLACEHOLDER_TEMPLATES_H

