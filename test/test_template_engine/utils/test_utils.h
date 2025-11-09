#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <Arduino.h>
#include <TemplateEngine.h>

// Test structure and state management
using TestFn = void(*)();

struct TestCase {
    const char* name;
    TestFn fn;
    uint16_t line;
};

#define TEST_ENTRY(fn) { #fn, fn, __LINE__ }

// Helper function to capture rendered output
String captureRenderedOutput(TemplateContext& ctx, size_t bufferSize = 512);

// Helper function to compare rendered output
bool compareRenderedOutput(const String& expected, TemplateContext& ctx, size_t bufferSize = 512);

// Helper function to create test RAM getter
// Note: This is a simple wrapper - for actual tests, define getter functions directly

// Helper function to verify template output
void verifyTemplateOutput(const char* templateData, PlaceholderRegistry& registry, const String& expected);

// Helper function to render template to string
String renderTemplateToString(const char* templateData, PlaceholderRegistry& registry, size_t bufferSize = 512);

#endif // TEST_UTILS_H

