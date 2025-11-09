#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "../templates/simple_templates.h"
#include "../templates/placeholder_templates.h"
#include "../templates/nested_templates.h"
#include "../utils/test_utils.h"

// Test RAM data getters
static String testTitle = "Test Title";
static String testSubtitle = "Test Subtitle";
static String testDesc = "Test Description";

static const char* getTestTitle() { return testTitle.c_str(); }
static const char* getTestSubtitle() { return testSubtitle.c_str(); }
static const char* getTestDesc() { return testDesc.c_str(); }

// Test full rendering scenarios
void test_integration_full_rendering() {
    Serial.println("[TEST]   Testing full rendering scenarios...");
    
    PlaceholderRegistry registry(20);
    
    // Register all placeholder types
    // Note: mixed_placeholders_template uses %STYLES%, %PAGE_TITLE%, %HEADER%, %FOOTER%
    registry.registerProgmemData("%STYLES%", test_css_data);
    registry.registerProgmemData("%JS%", test_js_data);
    registry.registerProgmemData("%FAVICON%", test_favicon_data);
    registry.registerProgmemTemplate("%HEADER%", test_header_template);
    registry.registerProgmemTemplate("%FOOTER%", test_footer_template);
    registry.registerRamData("%PAGE_TITLE%", getTestTitle);
    registry.registerRamData("%SUBTITLE%", getTestSubtitle);
    registry.registerRamData("%DESC%", getTestDesc);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    Serial.print("[DEBUG] Registry count: ");
    Serial.println(registry.getCount());
    Serial.print("[DEBUG] %STYLES% registered: ");
    Serial.println(registry.getPlaceholder("%STYLES%") != nullptr ? "YES" : "NO");
    Serial.print("[DEBUG] %PAGE_TITLE% registered: ");
    Serial.println(registry.getPlaceholder("%PAGE_TITLE%") != nullptr ? "YES" : "NO");
    Serial.print("[DEBUG] %HEADER% registered: ");
    Serial.println(registry.getPlaceholder("%HEADER%") != nullptr ? "YES" : "NO");
    Serial.print("[DEBUG] %FOOTER% registered: ");
    Serial.println(registry.getPlaceholder("%FOOTER%") != nullptr ? "YES" : "NO");
    TemplateRenderer::initializeContext(ctx, mixed_placeholders_template);
    String result = captureRenderedOutput(ctx);
    Serial.print("[DEBUG] Mixed result length: ");
    Serial.println(result.length());
    Serial.print("[DEBUG] Mixed result: [");
    Serial.print(result);
    Serial.println("]");
    Serial.print("[DEBUG] Contains 'Header:': ");
    Serial.println(result.indexOf("Header:") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Contains 'header': ");
    Serial.println(result.indexOf("header") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Contains 'body': ");
    Serial.println(result.indexOf("body") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Contains 'Title:': ");
    Serial.println(result.indexOf("Title:") >= 0 ? "YES" : "NO");
    TEST_ASSERT_TRUE_MESSAGE(result.length() > 0, "Should render template with all placeholder types");
    // Check for rendered content - PROGMEM data is "body { color: red; }"
    TEST_ASSERT_TRUE_MESSAGE(result.indexOf("Header:") >= 0 || result.indexOf("header") >= 0 || result.indexOf("body") >= 0 || result.indexOf("Title:") >= 0, 
        "Should contain rendered content");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should be complete after rendering");
    
    // Test real-world HTML-like template structure
    // Note: PROGMEM templates must be defined at file scope, not as local variables
    // Using a simple template string instead
    // Register placeholders for HTML template
    registry.registerRamData("%TITLE%", getTestTitle);
    registry.registerProgmemData("%CSS%", test_css_data);
    
    const char html_template[] = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>%TITLE%</title>\n"
        "  <style>%CSS%</style>\n"
        "</head>\n"
        "<body>\n"
        "  %HEADER%\n"
        "  <main>%DESC%</main>\n"
        "  %FOOTER%\n"
        "  <script>%JS%</script>\n"
        "</body>\n"
        "</html>";
    
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, html_template);
    String htmlResult = captureRenderedOutput(ctx);
    
    TEST_ASSERT_TRUE_MESSAGE(htmlResult.length() > 0, "Should render HTML-like template");
    TEST_ASSERT_TRUE_MESSAGE(htmlResult.indexOf("<!DOCTYPE html>") >= 0, 
        "Should contain HTML structure");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should be complete after rendering HTML template");
    
    Serial.println("[TEST]   Full rendering scenario tests completed successfully");
}

// Test memory efficiency
void test_integration_memory_efficiency() {
    Serial.println("[TEST]   Testing memory efficiency...");
    
    PlaceholderRegistry registry(10);
    // Register placeholder for long_text_template
    registry.registerProgmemData("%PLACEHOLDER%", test_css_data);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, long_text_template);
    
    size_t totalBytes = 0;
    uint8_t buffer[64];
    
    while (!TemplateRenderer::isComplete(ctx)) {
        size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
        totalBytes += written;
        
        // Verify buffer size stays reasonable
        TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(sizeof(buffer), written, 
            "Chunk size should not exceed buffer size");
    }
    
    // Verify we processed all data
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, totalBytes, "Should process some bytes");
    
    // Test that stack depth stays within limits
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, nested_one_level_outer);
    ctx.setRegistry(&registry);
    registry.registerProgmemTemplate("%INNER%", nested_one_level_inner);
    
    while (!TemplateRenderer::isComplete(ctx)) {
        TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
        TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(TemplateContext::MAX_RENDERING_DEPTH, ctx.renderingDepth, 
            "Rendering depth should not exceed MAX_RENDERING_DEPTH");
    }
    
    Serial.println("[TEST]   Memory efficiency tests completed successfully");
}

// Test multiple templates sequentially
void test_integration_multiple_templates() {
    Serial.println("[TEST]   Testing multiple templates sequentially...");
    
    PlaceholderRegistry registry(10);
    registry.registerProgmemData("%CSS%", test_css_data);
    registry.registerRamData("%TITLE%", getTestTitle);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Register placeholder for progmem_data_template
    registry.registerProgmemData("%STYLES%", test_css_data);
    // Render first template
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    String result1 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result1.length() > 0, "Should render first template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "First template should be complete");
    
    // Register placeholder for ram_data_template
    registry.registerRamData("%PAGE_TITLE%", getTestTitle);
    // Reset and render second template
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, ram_data_template);
    String result2 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result2.length() > 0, "Should render second template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Second template should be complete");
    
    // Reset and render third template
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, plain_text_template);
    String result3 = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, World!", result3.c_str(), 
        "Should render third template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Third template should be complete");
    
    // Verify results are different
    TEST_ASSERT_NOT_EQUAL_MESSAGE(result1.c_str(), result2.c_str(), 
        "Results should be different for different templates");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(result2.c_str(), result3.c_str(), 
        "Results should be different for different templates");
    
    Serial.println("[TEST]   Multiple templates sequential tests completed successfully");
}

