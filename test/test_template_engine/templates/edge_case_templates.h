#ifndef EDGE_CASE_TEMPLATES_H
#define EDGE_CASE_TEMPLATES_H

#include <Arduino.h>

// Incomplete placeholder (no closing %)
const char PROGMEM incomplete_placeholder_template[] = "Text with % incomplete";

// Placeholder with only %
const char PROGMEM empty_placeholder_template[] = "Text with %% text";

// Placeholder with % in text (not a placeholder)
const char PROGMEM percent_in_text_template[] = "Text with 50% discount";

// Special characters
const char PROGMEM special_chars_template[] = "Line1\nLine2\tTabbed\r\nWindows";

// Template with only placeholders (no text)
const char PROGMEM only_placeholders_template[] = "%A%%B%%C%";

// Template with newlines and placeholders
const char PROGMEM newlines_template[] = 
    "Line 1\n"
    "%PLACEHOLDER%\n"
    "Line 3\n";

// Template with tabs and placeholders
const char PROGMEM tabs_template[] = 
    "Column1\tColumn2\t%PLACEHOLDER%\tColumn4";

// Template with mixed whitespace
const char PROGMEM whitespace_template[] = 
    "  Leading spaces\n"
    "%PLACEHOLDER%\n"
    "Trailing spaces  ";

// Template with unicode-like characters (if supported)
const char PROGMEM unicode_template[] = 
    "Test: %PLACEHOLDER% with special chars: ©®™";

// Template with very long placeholder name (at max length)
const char PROGMEM max_length_placeholder_template[] = "%VERY_LONG_PLACEHOLDER_NAME%";

// Template with placeholder name that might exceed max
const char PROGMEM over_length_placeholder_template[] = "%THIS_PLACEHOLDER_NAME_IS_TOO_LONG_FOR_BUFFER%";

// Template with multiple incomplete placeholders
const char PROGMEM multiple_incomplete_template[] = 
    "Text with % incomplete and % another incomplete";

// Template with placeholder at very end
const char PROGMEM placeholder_at_end_template[] = "Text%END%";

// Template with placeholder at very start
const char PROGMEM placeholder_at_start_template[] = "%START%Text";

// Template with only text (no placeholders)
const char PROGMEM only_text_template[] = 
    "This is a template with only text and no placeholders at all.";

// Template with escaped-like patterns (not actual escaping, just patterns)
const char PROGMEM escaped_patterns_template[] = 
    "Text with %% and %PLACEHOLDER% and %%";

#endif // EDGE_CASE_TEMPLATES_H

