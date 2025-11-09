#include "test_utils.h"
#include <unity.h>
#include <pgmspace.h>

// Helper function to capture rendered output
String captureRenderedOutput(TemplateContext& ctx, size_t bufferSize) {
    String output;
    uint8_t* buffer = new uint8_t[bufferSize];
    
    if (!buffer) {
        return output;
    }
    
    while (!TemplateRenderer::isComplete(ctx)) {
        size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, bufferSize);
        if (written > 0) {
            // Arduino String doesn't have (const char*, size_t) constructor
            // Use substring approach or create temporary string
            char* temp = new char[written + 1];
            memcpy(temp, buffer, written);
            temp[written] = '\0';
            output += String(temp);
            delete[] temp;
        } else {
            break;
        }
    }
    
    delete[] buffer;
    return output;
}

// Helper function to compare rendered output
bool compareRenderedOutput(const String& expected, TemplateContext& ctx, size_t bufferSize) {
    String actual = captureRenderedOutput(ctx, bufferSize);
    return actual == expected;
}

// Note: For test RAM getters, define them as regular functions in test files
// Example:
//   static String testData = "test";
//   const char* getTestData() { return testData.c_str(); }

// Helper function to verify template output
void verifyTemplateOutput(const char* templateData, PlaceholderRegistry& registry, const String& expected) {
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, templateData);
    
    String actual = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.c_str(), actual.c_str(), 
        "Template output should match expected");
}

// Helper function to render template to string
String renderTemplateToString(const char* templateData, PlaceholderRegistry& registry, size_t bufferSize) {
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, templateData);
    return captureRenderedOutput(ctx, bufferSize);
}

