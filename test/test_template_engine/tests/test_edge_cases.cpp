#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "../templates/simple_templates.h"
#include "../templates/placeholder_templates.h"
#include "../templates/edge_case_templates.h"
#include "../templates/nested_templates.h"
#include "../utils/test_utils.h"

// Test RAM data getter that returns null
static const char* getNullRamData() {
    return nullptr;
}

// Test RAM data getter that returns empty string
static const char* getEmptyRamData() {
    static String empty = "";
    return empty.c_str();
}

// Test error handling
void test_edge_cases_error_handling() {
    Serial.println("[TEST]   Testing error handling...");
    
    PlaceholderRegistry registry(10);
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test render with null template pointer (should handle gracefully)
    // Note: initializeContext expects valid PROGMEM pointer, so we'll test with empty template
    TemplateRenderer::initializeContext(ctx, empty_template);
    String result1 = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", result1.c_str(), 
        "Should handle empty template");
    
    // Test render with null registry
    ctx.reset();
    ctx.setRegistry(nullptr);
    TemplateRenderer::initializeContext(ctx, single_placeholder_template);
    String result2 = captureRenderedOutput(ctx);
    // Should skip placeholder and render text
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, !", result2.c_str(), 
        "Should handle null registry");
    
    // Test RAM getter returning null
    registry.registerRamData("%NULL%", getNullRamData);
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, single_placeholder_template);
    ctx.setRegistry(&registry);
    String result3 = captureRenderedOutput(ctx);
    // Should handle null return gracefully
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete even with null RAM getter");
    
    // Test RAM getter returning empty string
    registry.registerRamData("%EMPTY%", getEmptyRamData);
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, single_placeholder_template);
    ctx.setRegistry(&registry);
    String result4 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with empty RAM data");
    
    // Test incomplete placeholder (no closing %)
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, incomplete_placeholder_template);
    ctx.setRegistry(&registry);
    String result5 = captureRenderedOutput(ctx);
    // Should handle incomplete placeholder gracefully
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete even with incomplete placeholder");
    
    Serial.println("[TEST]   Error handling tests completed successfully");
}

// Test boundary conditions
void test_edge_cases_boundary_conditions() {
    Serial.println("[TEST]   Testing boundary conditions...");
    
    PlaceholderRegistry registry(10);
    registry.registerProgmemData("%PLACEHOLDER%", test_css_data);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test placeholder at start
    // PLACEHOLDER is already registered above, so we can use it
    Serial.print("[DEBUG] Edge case - %PLACEHOLDER% registered: ");
    Serial.println(registry.getPlaceholder("%PLACEHOLDER%") != nullptr ? "YES" : "NO");
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, placeholder_start_template);
    Serial.print("[DEBUG] Edge case template: [");
    Serial.print(placeholder_start_template);
    Serial.println("]");
    String result1 = captureRenderedOutput(ctx);
    Serial.print("[DEBUG] Edge case result length: ");
    Serial.println(result1.length());
    Serial.print("[DEBUG] Edge case result: [");
    Serial.print(result1);
    Serial.println("]");
    Serial.print("[DEBUG] Edge case - Contains ' text': ");
    Serial.println(result1.indexOf(" text") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Edge case - Contains 'body': ");
    Serial.println(result1.indexOf("body") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Edge case - Final state: ");
    Serial.println(ctx.getStateString());
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.print("[DEBUG] Edge case - Template length: ");
        Serial.println(currentCtx->context.templateCtx.templateLen);
    }
    // Should render placeholder and text
    TEST_ASSERT_TRUE_MESSAGE(result1.length() > 0, "Should handle placeholder at start");
    // Check for either the placeholder content or the text part
    TEST_ASSERT_TRUE_MESSAGE(result1.indexOf(" text") >= 0 || result1.indexOf("body") >= 0, 
        "Should contain placeholder or text");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with placeholder at start");
    
    // Test placeholder at end
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, placeholder_end_template);
    String result2 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result2.length() > 0, "Should handle placeholder at end");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with placeholder at end");
    
    // Test only placeholders (no text)
    registry.registerProgmemData("%A%", test_css_data);
    registry.registerProgmemData("%B%", test_js_data);
    registry.registerProgmemData("%C%", test_favicon_data);
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, only_placeholders_template);
    ctx.setRegistry(&registry);
    String result3 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result3.length() > 0, "Should handle only placeholders");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with only placeholders");
    
    // Test only text (no placeholders)
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, only_text_template);
    ctx.setRegistry(&registry);
    String result4 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result4.length() > 0, "Should handle only text");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with only text");
    
    // Test special characters
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, special_chars_template);
    ctx.setRegistry(&registry);
    String result5 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result5.length() > 0, "Should handle special characters");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with special characters");
    
    // Test newlines and placeholders
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, newlines_template);
    ctx.setRegistry(&registry);
    String result6 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result6.length() > 0, "Should handle newlines");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with newlines");
    
    // Test empty placeholder (%%)
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, empty_placeholder_template);
    ctx.setRegistry(&registry);
    String result7 = captureRenderedOutput(ctx);
    // Should handle empty placeholder gracefully
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with empty placeholder");
    
    Serial.println("[TEST]   Boundary condition tests completed successfully");
}

// Test stress conditions
void test_edge_cases_stress() {
    Serial.println("[TEST]   Testing stress conditions...");
    
    PlaceholderRegistry registry(50);
    
    // Register many placeholders
    for (int i = 0; i < 20; i++) {
        char name[10];
        sprintf(name, "%%P%d%%", i);
        registry.registerProgmemData(name, test_css_data);
    }
    
    TEST_ASSERT_EQUAL_MESSAGE(20, registry.getCount(), 
        "Should register many placeholders");
    
    // Test template with many placeholders
    // Note: PROGMEM templates must be defined at file scope
    const char many_placeholders_template[] = 
        "%P0%%P1%%P2%%P3%%P4%%P5%%P6%%P7%%P8%%P9%"
        "%P10%%P11%%P12%%P13%%P14%%P15%%P16%%P17%%P18%%P19%";
    
    // All placeholders are already registered above
    Serial.print("[DEBUG] Stress test - Registry count: ");
    Serial.println(registry.getCount());
    Serial.print("[DEBUG] Stress test - %P0% registered: ");
    Serial.println(registry.getPlaceholder("%P0%") != nullptr ? "YES" : "NO");
    Serial.print("[DEBUG] Stress test - %P19% registered: ");
    Serial.println(registry.getPlaceholder("%P19%") != nullptr ? "YES" : "NO");
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, many_placeholders_template);
    Serial.print("[DEBUG] Stress test template length: ");
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.println(currentCtx->context.templateCtx.templateLen);
    } else {
        Serial.println(0);
    }
    String result = captureRenderedOutput(ctx);
    Serial.print("[DEBUG] Stress test result length: ");
    Serial.println(result.length());
    Serial.print("[DEBUG] Stress test result (first 200 chars): [");
    if (result.length() > 200) {
        Serial.print(result.substring(0, 200));
        Serial.println("...]");
    } else {
        Serial.print(result);
        Serial.println("]");
    }
    Serial.print("[DEBUG] Stress test - Contains 'body': ");
    Serial.println(result.indexOf("body") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG] Stress test - Final state: ");
    Serial.println(ctx.getStateString());
    Serial.print("[DEBUG] Stress test - Is complete: ");
    Serial.println(TemplateRenderer::isComplete(ctx) ? "YES" : "NO");
    // All placeholders are registered with test_css_data, so result should contain "body" multiple times
    TEST_ASSERT_TRUE_MESSAGE(result.length() > 0, "Should render template with many placeholders");
    // Verify we got some content (all placeholders render the same CSS data)
    TEST_ASSERT_TRUE_MESSAGE(result.indexOf("body") >= 0 || result.length() > 20, 
        "Should contain rendered placeholder data");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete with many placeholders");
    
    // Test very long template
    // Register placeholder for long_text_template (if not already registered)
    if (registry.getPlaceholder("%PLACEHOLDER%") == nullptr) {
        registry.registerProgmemData("%PLACEHOLDER%", test_css_data);
    }
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, long_text_template);
    String result2 = captureRenderedOutput(ctx);
    
    TEST_ASSERT_TRUE_MESSAGE(result2.length() > 0, "Should render very long template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete very long template");
    
    // Test rendering with very small buffer repeatedly
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, long_text_template);
    
    uint8_t tinyBuffer[1];
    int iterations = 0;
    while (!TemplateRenderer::isComplete(ctx) && iterations < 10000) {
        TemplateRenderer::renderNextChunk(ctx, tinyBuffer, sizeof(tinyBuffer));
        iterations++;
    }
    
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete even with tiny buffer");
    TEST_ASSERT_LESS_THAN_MESSAGE(10000, iterations, 
        "Should complete in reasonable iterations");
    
    // Test stack depth near limit
    registry.clear();
    registry.registerProgmemTemplate("%L2%", deep_nest_level2);
    registry.registerProgmemTemplate("%L3%", deep_nest_level3);
    registry.registerProgmemTemplate("%L4%", deep_nest_level4);
    registry.registerProgmemTemplate("%L5%", deep_nest_level5);
    registry.registerProgmemTemplate("%L6%", deep_nest_level6);
    registry.registerProgmemTemplate("%L7%", deep_nest_level7);
    registry.registerProgmemTemplate("%L8%", deep_nest_level8);
    registry.registerProgmemTemplate("%L9%", deep_nest_level9);
    registry.registerProgmemTemplate("%L10%", deep_nest_level10);
    registry.registerProgmemTemplate("%L11%", deep_nest_level11);
    registry.registerProgmemTemplate("%L12%", deep_nest_level12);
    registry.registerProgmemTemplate("%L13%", deep_nest_level13);
    registry.registerProgmemTemplate("%L14%", deep_nest_level14);
    registry.registerProgmemTemplate("%L15%", deep_nest_level15);
    registry.registerProgmemTemplate("%L16%", deep_nest_level16);
    
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, deep_nest_level1);
    String result3 = captureRenderedOutput(ctx);
    
    TEST_ASSERT_TRUE_MESSAGE(result3.length() > 0, "Should render deep nested template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete deep nested template");
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(TemplateContext::MAX_RENDERING_DEPTH, ctx.renderingDepth, 
        "Rendering depth should not exceed MAX_RENDERING_DEPTH");
    
    Serial.println("[TEST]   Stress condition tests completed successfully");
}

