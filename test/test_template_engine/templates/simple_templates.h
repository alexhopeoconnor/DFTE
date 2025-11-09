#ifndef SIMPLE_TEMPLATES_H
#define SIMPLE_TEMPLATES_H

#include <Arduino.h>

// Empty template
const char PROGMEM empty_template[] = "";

// Plain text template
const char PROGMEM plain_text_template[] = "Hello, World!";

// Single placeholder
const char PROGMEM single_placeholder_template[] = "Hello, %NAME%!";

// Multiple placeholders
const char PROGMEM multiple_placeholders_template[] = 
    "Title: %TITLE%\nContent: %CONTENT%\nFooter: %FOOTER%";

// Placeholder at start
const char PROGMEM placeholder_start_template[] = "%PLACEHOLDER% text";

// Placeholder at end
const char PROGMEM placeholder_end_template[] = "text %PLACEHOLDER%";

// Consecutive placeholders
const char PROGMEM consecutive_placeholders_template[] = "%A%%B%%C%";

// Long text between placeholders
const char PROGMEM long_text_template[] = 
    "This is a very long text that should test buffer refill logic. "
    "It contains many characters and should span multiple buffer fills. "
    "%PLACEHOLDER% More text here.";

#endif // SIMPLE_TEMPLATES_H

