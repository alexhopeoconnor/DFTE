#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include <pgmspace.h>
#include "../templates/simple_templates.h"
#include "../templates/placeholder_templates.h"
#include "../templates/nested_templates.h"
#include "../utils/test_utils.h"

// Test RAM data getters
static String testTitle = "Test Title";
static String testContent = "Test Content";
static String testFooter = "Test Footer";
static String testTheme = "dark";
static String testRssi = "-41";
static String testHeap = "51200";
static String testUptime = "01:23:45";

static const char* getTestTitle() { return testTitle.c_str(); }
static const char* getTestContent() { return testContent.c_str(); }
static const char* getTestFooter() { return testFooter.c_str(); }
static const char* getTestTheme() { return testTheme.c_str(); }
static const char* getTestRssi() { return testRssi.c_str(); }
static const char* getTestHeap() { return testHeap.c_str(); }
static const char* getTestUptime() { return testUptime.c_str(); }

static const char PROGMEM simulated_logo_base64[] = "VEVTVF9MT0dPX0RBVEE=";
static const char ram_root_template[] = "Status: %TITLE%";
static const char ram_root_text_only[] = "RAM root template";

struct DynamicPlaceholderState {
    String value;
};

static DynamicPlaceholderState dynamicGreetingState;

static const char* dynamicGreetingGetter(void* userData) {
    DynamicPlaceholderState* state = static_cast<DynamicPlaceholderState*>(userData);
    static String buffer;
    buffer = state->value;
    return buffer.c_str();
}

static size_t dynamicGreetingLength(const char* data, void* userData) {
    (void)userData;
    return strlen(data);
}

static DynamicTemplateDescriptor dynamicGreetingDescriptor = {
    dynamicGreetingGetter,
    dynamicGreetingLength,
    &dynamicGreetingState
};

struct ConditionalTestState {
    ConditionalBranchResult result;
};

static ConditionalTestState conditionalState;

static ConditionalBranchResult evaluateConditionalState(void* userData) {
    ConditionalTestState* state = static_cast<ConditionalTestState*>(userData);
    return state->result;
}

static ConditionalDescriptor conditionalDescriptor = {
    evaluateConditionalState,
    "%COND_TRUE%",
    "%COND_FALSE%",
    &conditionalState
};

static ConditionalDescriptor skipConditionalDescriptor = {
    evaluateConditionalState,
    "%COND_TRUE%",
    "%COND_FALSE%",
    &conditionalState
};

static ConditionalDescriptor iteratorConditionalDescriptor = {
    evaluateConditionalState,
    "%WIFI_LIST%",
    nullptr,
    &conditionalState
};

struct WifiIteratorState {
    size_t index;
    size_t count;
    bool closeCalled;
    bool useDynamicTemplate;
    bool includeOverrides;
    bool produceError;
    size_t errorAfter;
};

static WifiIteratorState wifiIteratorState;

static void resetWifiIteratorState(size_t count) {
    wifiIteratorState.index = 0;
    wifiIteratorState.count = count;
    wifiIteratorState.closeCalled = false;
    wifiIteratorState.useDynamicTemplate = false;
    wifiIteratorState.includeOverrides = false;
    wifiIteratorState.produceError = false;
    wifiIteratorState.errorAfter = 0;
}

static void* wifiIteratorOpen(void* userData) {
    WifiIteratorState* state = static_cast<WifiIteratorState*>(userData);
    state->index = 0;
    state->closeCalled = false;
    return state;
}

static IteratorStepResult wifiIteratorNext(void* handle, IteratorItemView& view);
static void wifiIteratorClose(void* handle);

static IteratorDescriptor wifiIteratorDescriptor = {
    wifiIteratorOpen,
    wifiIteratorNext,
    wifiIteratorClose,
    &wifiIteratorState
};

static const char PROGMEM wifiItemTemplate[] = "<li>%SSID% (%RSSI%)</li>";
static const char PROGMEM wifiTrueBadgeTemplate[] = "[WiFi ENABLED]";
static const char PROGMEM wifiFalseBadgeTemplate[] = "[WiFi DISABLED]";

static const char* wifiDynamicTemplates[] = {
    "<li>Net-A [-41dBm]</li>",
    "<li>Net-B [-55dBm]</li>",
    "<li>Net-C [-70dBm]</li>"
};

static const char PROGMEM wifiOverrideSsid0[] = "Net-A";
static const char PROGMEM wifiOverrideSsid1[] = "Net-B";
static const char PROGMEM wifiOverrideSsid2[] = "Net-C";
static const char PROGMEM wifiOverrideRssi0[] = "-41";
static const char PROGMEM wifiOverrideRssi1[] = "-55";
static const char PROGMEM wifiOverrideRssi2[] = "-70";

static PlaceholderEntry wifiOverrideEntries[3][2];
static bool wifiOverridesInitialized = false;

static void initializeWifiOverrides() {
    if (wifiOverridesInitialized) {
        return;
    }

    const char* placeholderNames[2] = {"%SSID%", "%RSSI%"};
    const char* const progmemData[3][2] = {
        {wifiOverrideSsid0, wifiOverrideRssi0},
        {wifiOverrideSsid1, wifiOverrideRssi1},
        {wifiOverrideSsid2, wifiOverrideRssi2}
    };

    for (size_t item = 0; item < 3; ++item) {
        for (size_t field = 0; field < 2; ++field) {
            PlaceholderEntry& entry = wifiOverrideEntries[item][field];
            memset(entry.name, 0, sizeof(entry.name));
            strncpy(entry.name, placeholderNames[field], sizeof(entry.name) - 1);
            entry.type = PlaceholderType::PROGMEM_DATA;
            entry.data = progmemData[item][field];
            entry.getLength = DeviceFrameworkPlaceholderRegistry::getProgmemLength;
        }
    }

    wifiOverridesInitialized = true;
}

static IteratorStepResult wifiIteratorNext(void* handle, IteratorItemView& view) {
    WifiIteratorState* state = static_cast<WifiIteratorState*>(handle);
    if (!state || state->index >= state->count) {
        return IteratorStepResult::COMPLETE;
    }

    if (state->produceError && state->index >= state->errorAfter) {
        return IteratorStepResult::ERROR;
    }

    size_t idx = state->index++;

    if (state->useDynamicTemplate) {
        static String dynamicBuffer;
        dynamicBuffer = wifiDynamicTemplates[idx % 3];
        view.templateData = dynamicBuffer.c_str();
        view.templateLength = dynamicBuffer.length();
        view.templateIsProgmem = false;
        view.placeholders = nullptr;
        view.placeholderCount = 0;
    } else {
        view.templateData = wifiItemTemplate;
        view.templateLength = strlen_P(wifiItemTemplate);
        view.templateIsProgmem = true;
        if (state->includeOverrides) {
            initializeWifiOverrides();
            view.placeholders = wifiOverrideEntries[idx % 3];
            view.placeholderCount = 2;
        } else {
            view.placeholders = nullptr;
            view.placeholderCount = 0;
        }
    }

    return IteratorStepResult::ITEM_READY;
}

static void wifiIteratorClose(void* handle) {
    WifiIteratorState* state = static_cast<WifiIteratorState*>(handle);
    if (state) {
        state->closeCalled = true;
    }
}

static const char PROGMEM dynamicWrapperTemplate[] = "Greeting: %DYNAMIC_GREETING%";
static const char PROGMEM dynamicMixedTemplate[] = "Start-%DYNAMIC_GREETING%-End";
static const char PROGMEM conditionalWrapperTemplate[] = "Status:%STATUS_BLOCK%:Done";
static const char PROGMEM conditionalSkipTemplate[] = "Value[%STATUS_BLOCK%]";
static const char PROGMEM iteratorWrapperTemplate[] = "<ul>%WIFI_LIST%</ul>";
static const char PROGMEM iteratorConditionalWrapperTemplate[] = "Badge:<ul>%WIFI_BADGE%</ul>";

// Test TemplateRenderer basic rendering
void test_template_renderer_basic() {
    Serial.println("[TEST:test_template_renderer_basic]   Testing TemplateRenderer basic rendering...");
    
    PlaceholderRegistry registry(10);
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test simple template (no placeholders)
    TemplateRenderer::initializeContext(ctx, plain_text_template);
    String result = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, World!", result.c_str(), 
        "Should render plain text template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should be complete after rendering");
    
    // Test empty template
    ctx.reset();
    TemplateRenderer::initializeContext(ctx, empty_template);
    String result2 = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", result2.c_str(), 
        "Should render empty template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should be complete after rendering empty template");
    
    // Test template with single PROGMEM_DATA placeholder
    // Template uses %STYLES%, so register that
    registry.registerProgmemData("%STYLES%", test_css_data);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    String result3 = captureRenderedOutput(ctx);
    // Expected: "CSS: " + test_css_data content
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("CSS:") >= 0, 
        "Should contain CSS prefix");
    // Verify we got more than just "CSS: " (should have actual CSS data)
    size_t cssPrefixLen = strlen("CSS: ");
    TEST_ASSERT_GREATER_THAN_MESSAGE(cssPrefixLen, result3.length(), 
        "Should contain CSS data");
    
    // Test template with single RAM_DATA placeholder
    // Template uses %PAGE_TITLE%, so register that
    registry.registerRamData("%PAGE_TITLE%", getTestTitle);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, ram_data_template);
    String result4 = captureRenderedOutput(ctx);
    String expected4 = String("Title: ") + testTitle;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected4.c_str(), result4.c_str(), 
        "Should render template with RAM_DATA placeholder");
    
    // Test template with multiple placeholders
    registry.registerRamData("%TITLE%", getTestTitle);
    registry.registerRamData("%CONTENT%", getTestContent);
    registry.registerRamData("%FOOTER%", getTestFooter);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, multiple_placeholders_template);
    String result5 = captureRenderedOutput(ctx);
    String expected5 = String("Title: ") + testTitle + "\nContent: " + testContent + "\nFooter: " + testFooter;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected5.c_str(), result5.c_str(), 
        "Should render template with multiple placeholders");
    
    Serial.println("[TEST:test_template_renderer_basic]   TemplateRenderer basic rendering tests completed successfully");
}

// Test TemplateRenderer chunked rendering
void test_template_renderer_chunked() {
    Serial.println("[TEST:test_template_renderer_chunked]   Testing TemplateRenderer chunked rendering...");
    
    PlaceholderRegistry registry(10);
    registry.registerProgmemData("%STYLES%", test_css_data);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test rendering with small buffer (1 byte chunks)
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    String result1 = captureRenderedOutput(ctx, 1);
    TEST_ASSERT_TRUE_MESSAGE(result1.indexOf("CSS:") >= 0, 
        "Should render correctly with 1 byte buffer");
    
    // Test rendering with medium buffer (64 bytes)
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    String result2 = captureRenderedOutput(ctx, 64);
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("CSS:") >= 0, 
        "Should render correctly with 64 byte buffer");
    
    // Test rendering with large buffer (512 bytes)
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    String result3 = captureRenderedOutput(ctx, 512);
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("CSS:") >= 0, 
        "Should render correctly with 512 byte buffer");
    
    // Test rendering with buffer larger than template
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, plain_text_template);
    String result4 = captureRenderedOutput(ctx, 1024);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, World!", result4.c_str(), 
        "Should render correctly with buffer larger than template");
    
    // Test that all chunks combined equal full template
    // Register placeholder for long_text_template
    registry.registerProgmemData("%PLACEHOLDER%", test_css_data);
    Serial.print("[DEBUG:test_template_renderer_chunked] Registered placeholders: ");
    Serial.println(registry.getCount());
    const PlaceholderEntry* debugEntry = registry.getPlaceholder("%PLACEHOLDER%");
    Serial.print("[DEBUG:test_template_renderer_chunked] Placeholder %PLACEHOLDER% found: ");
    Serial.println(debugEntry != nullptr ? "YES" : "NO");
    if (debugEntry) {
        Serial.print("[DEBUG:test_template_renderer_chunked] Placeholder type: ");
        Serial.println((int)debugEntry->type);
    }
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, long_text_template);
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (currentCtx) {
        Serial.print("[DEBUG:test_template_renderer_chunked] Template length: ");
        Serial.println(currentCtx->context.templateCtx.templateLen);
    }
    Serial.print("[DEBUG:test_template_renderer_chunked] Initial state: ");
    Serial.println(ctx.getStateString());
    String fullResult = captureRenderedOutput(ctx, 512);
    Serial.print("[DEBUG:test_template_renderer_chunked] Full result length: ");
    Serial.println(fullResult.length());
    Serial.print("[DEBUG:test_template_renderer_chunked] Full result: [");
    Serial.print(fullResult);
    Serial.println("]");
    Serial.print("[DEBUG:test_template_renderer_chunked] Full result final state: ");
    Serial.println(ctx.getStateString());
    currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.print("[DEBUG:test_template_renderer_chunked] Full result template pos: ");
        Serial.println(currentCtx->context.templateCtx.position);
    }
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, long_text_template);
    Serial.print("[DEBUG:test_template_renderer_chunked] Chunked initial state: ");
    Serial.println(ctx.getStateString());
    String chunkedResult = captureRenderedOutput(ctx, 10);
    Serial.print("[DEBUG:test_template_renderer_chunked] Chunked result length: ");
    Serial.println(chunkedResult.length());
    Serial.print("[DEBUG:test_template_renderer_chunked] Chunked result: [");
    Serial.print(chunkedResult);
    Serial.println("]");
    Serial.print("[DEBUG:test_template_renderer_chunked] Chunked result final state: ");
    Serial.println(ctx.getStateString());
    currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.print("[DEBUG:test_template_renderer_chunked] Chunked result template pos: ");
        Serial.println(currentCtx->context.templateCtx.position);
    }
    TEST_ASSERT_EQUAL_STRING_MESSAGE(fullResult.c_str(), chunkedResult.c_str(), 
        "Chunked rendering should match full rendering");
    
    Serial.println("[TEST:test_template_renderer_chunked]   TemplateRenderer chunked rendering tests completed successfully");
}

// Test TemplateRenderer state transitions
void test_template_renderer_state_transitions() {
    Serial.println("[TEST:test_template_renderer_state_transitions]   Testing TemplateRenderer state transitions...");
    
    PlaceholderRegistry registry(10);
    registry.registerProgmemData("%CSS%", test_css_data);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test TEXT -> BUILDING_PLACEHOLDER -> RENDERING_CONTEXT -> TEXT
    TemplateRenderer::initializeContext(ctx, progmem_data_template);
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::TEXT, ctx.state, 
        "Initial state should be TEXT");
    
    // Render a small chunk to trigger state transition
    uint8_t buffer[10];
    size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, written, "Should write some data");
    
    // Continue rendering until complete
    while (!TemplateRenderer::isComplete(ctx)) {
        TemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
    }
    
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::COMPLETE, ctx.state, 
        "Final state should be COMPLETE");
    
    Serial.println("[TEST:test_template_renderer_state_transitions]   TemplateRenderer state transition tests completed successfully");
}

// Test TemplateRenderer placeholder processing
void test_template_renderer_placeholders() {
    Serial.println("[TEST:test_template_renderer_placeholders]   Testing TemplateRenderer placeholder processing...");
    
    PlaceholderRegistry registry(10);
    registry.registerProgmemData("%CSS%", test_css_data);
    registry.registerRamData("%TITLE%", getTestTitle);
    
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    
    // Test placeholder at start
    // Register placeholder first
    registry.registerProgmemData("%PLACEHOLDER%", test_css_data);
    Serial.print("[DEBUG:test_template_renderer_placeholders] Registered %PLACEHOLDER%: ");
    Serial.println(registry.getPlaceholder("%PLACEHOLDER%") != nullptr ? "YES" : "NO");
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, placeholder_start_template);
    Serial.print("[DEBUG:test_template_renderer_placeholders] Template: [");
    Serial.print(placeholder_start_template);
    Serial.println("]");
    Serial.print("[DEBUG:test_template_renderer_placeholders] Initial state: ");
    Serial.println(ctx.getStateString());
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.print("[DEBUG:test_template_renderer_placeholders] Template length: ");
        Serial.println(currentCtx->context.templateCtx.templateLen);
        Serial.print("[DEBUG:test_template_renderer_placeholders] Template pos: ");
        Serial.println(currentCtx->context.templateCtx.position);
    }
    Serial.print("[DEBUG:test_template_renderer_placeholders] Buffer pos: ");
    Serial.println(ctx.bufferPos);
    Serial.print("[DEBUG:test_template_renderer_placeholders] Buffer len: ");
    Serial.println(ctx.bufferLen);
    String result1 = captureRenderedOutput(ctx);
    Serial.print("[DEBUG:test_template_renderer_placeholders] Final state: ");
    Serial.println(ctx.getStateString());
    currentCtx = ctx.getCurrentContext();
    if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
        Serial.print("[DEBUG:test_template_renderer_placeholders] Final template pos: ");
        Serial.println(currentCtx->context.templateCtx.position);
    }
    Serial.print("[DEBUG:test_template_renderer_placeholders] Result length: ");
    Serial.println(result1.length());
    Serial.print("[DEBUG:test_template_renderer_placeholders] Result: [");
    Serial.print(result1);
    Serial.println("]");
    Serial.print("[DEBUG:test_template_renderer_placeholders] Contains ' text': ");
    Serial.println(result1.indexOf(" text") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG:test_template_renderer_placeholders] Contains 'body': ");
    Serial.println(result1.indexOf("body") >= 0 ? "YES" : "NO");
    // Should render placeholder and text
    // PROGMEM data is "body { color: red; }", so check for that or " text"
    TEST_ASSERT_TRUE_MESSAGE(result1.indexOf(" text") >= 0 || result1.indexOf("body") >= 0, 
        "Should handle placeholder at start");
    
    // Test placeholder at end
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, placeholder_end_template);
    String result2 = captureRenderedOutput(ctx);
    // Placeholder is registered, so it should be rendered
    // Template: "text %PLACEHOLDER%" should render as "text body { color: red; }"
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("text ") >= 0, 
        "Should contain text before placeholder");
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("body") >= 0, 
        "Should render placeholder at end");
    
    // Test consecutive placeholders
    registry.registerProgmemData("%A%", test_css_data);
    registry.registerRamData("%B%", getTestTitle);
    registry.registerRamData("%C%", getTestContent);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, consecutive_placeholders_template);
    String result3 = captureRenderedOutput(ctx);
    // Should render all placeholders
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf(testTitle) >= 0, 
        "Should contain title");
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf(testContent) >= 0, 
        "Should contain content");
    TEST_ASSERT_TRUE_MESSAGE(result3.length() > testTitle.length() + testContent.length(), 
        "Should contain all placeholder data");
    
    // Test unknown placeholder (should be skipped)
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, single_placeholder_template);
    String result4 = captureRenderedOutput(ctx);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Hello, !", result4.c_str(), 
        "Should skip unknown placeholder");
    
    Serial.println("[TEST:test_template_renderer_placeholders]   TemplateRenderer placeholder processing tests completed successfully");
}

// Test TemplateRenderer nested templates
void test_template_renderer_nested() {
    Serial.println("[TEST:test_template_renderer_nested]   Testing TemplateRenderer nested templates...");
    
    PlaceholderRegistry registry(10);
    
    // Test one level nesting
    registry.registerProgmemTemplate("%INNER%", nested_one_level_inner);
    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_one_level_outer);
    String result1 = captureRenderedOutput(ctx);
    // Expected: "Outer: Inner content"
    TEST_ASSERT_TRUE_MESSAGE(result1.indexOf("Outer:") >= 0, 
        "Should contain outer text");
    TEST_ASSERT_TRUE_MESSAGE(result1.indexOf("Inner content") >= 0, 
        "Should contain inner template content");
    
    // Test two levels nesting
    registry.registerProgmemTemplate("%LEVEL2%", nested_two_levels_mid);
    registry.registerProgmemTemplate("%LEVEL3%", nested_two_levels_inner);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_two_levels_outer);
    String result2 = captureRenderedOutput(ctx);
    // Expected: "Level1: Level2: Level3 content"
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("Level1:") >= 0, 
        "Should contain level1 text");
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("Level2:") >= 0, 
        "Should contain level2 text");
    TEST_ASSERT_TRUE_MESSAGE(result2.indexOf("Level3 content") >= 0, 
        "Should contain level3 content");
    
    // Test three levels nesting
    registry.registerProgmemTemplate("%MID%", nested_three_levels_mid);
    registry.registerProgmemTemplate("%INNER%", nested_three_levels_inner);
    registry.registerProgmemTemplate("%DEEP%", nested_three_levels_deep);
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_three_levels_outer);
    String result3 = captureRenderedOutput(ctx);
    // Expected: "Outer: Mid: Inner: Deep content"
    Serial.println("\n[TEST:test_template_renderer_nested] ========== THREE-LEVEL NESTED TEST ==========");
    Serial.print("[TEST:test_template_renderer_nested] Result: [");
    Serial.print(result3);
    Serial.println("]");
    Serial.print("[TEST:test_template_renderer_nested] Length: ");
    Serial.println(result3.length());
    Serial.print("[TEST:test_template_renderer_nested] Contains 'Outer:': ");
    Serial.println(result3.indexOf("Outer:") >= 0 ? "YES" : "NO");
    Serial.print("[TEST:test_template_renderer_nested] Contains 'Mid:': ");
    Serial.println(result3.indexOf("Mid:") >= 0 ? "YES" : "NO");
    Serial.print("[TEST:test_template_renderer_nested] Contains 'Inner:': ");
    Serial.println(result3.indexOf("Inner:") >= 0 ? "YES" : "NO");
    Serial.print("[TEST:test_template_renderer_nested] Contains 'Deep content': ");
    Serial.println(result3.indexOf("Deep content") >= 0 ? "YES" : "NO");
    Serial.println("[TEST:test_template_renderer_nested] ============================================\n");
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("Outer:") >= 0, 
        "Should contain outer text");
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("Mid:") >= 0, 
        "Should contain mid text");
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("Inner:") >= 0, 
        "Should contain inner text");
    TEST_ASSERT_TRUE_MESSAGE(result3.indexOf("Deep content") >= 0, 
        "Should contain deep content");
    
    // Test nested template with placeholders
    // nested_with_placeholder_outer uses %NESTED%
    // nested_with_placeholder_inner uses %PLACEHOLDER%
    registry.registerRamData("%PLACEHOLDER%", getTestTitle);
    registry.registerProgmemTemplate("%NESTED%", nested_with_placeholder_inner);
    Serial.print("[DEBUG:test_template_renderer_nested] Registered %PLACEHOLDER%: ");
    Serial.println(registry.getPlaceholder("%PLACEHOLDER%") != nullptr ? "YES" : "NO");
    Serial.print("[DEBUG:test_template_renderer_nested] Registered %NESTED%: ");
    Serial.println(registry.getPlaceholder("%NESTED%") != nullptr ? "YES" : "NO");
    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_with_placeholder_outer);
    String result4 = captureRenderedOutput(ctx);
    Serial.print("[DEBUG:test_template_renderer_nested] Nested result length: ");
    Serial.println(result4.length());
    Serial.print("[DEBUG:test_template_renderer_nested] Nested result: [");
    Serial.print(result4);
    Serial.println("]");
    Serial.print("[DEBUG:test_template_renderer_nested] Contains 'Outer:': ");
    Serial.println(result4.indexOf("Outer:") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG:test_template_renderer_nested] Contains 'Inner:': ");
    Serial.println(result4.indexOf("Inner:") >= 0 ? "YES" : "NO");
    Serial.print("[DEBUG:test_template_renderer_nested] Contains testTitle '");
    Serial.print(testTitle);
    Serial.print("': ");
    Serial.println(result4.indexOf(testTitle) >= 0 ? "YES" : "NO");
    // Expected: "Outer: Inner: " + testTitle
    TEST_ASSERT_TRUE_MESSAGE(result4.indexOf("Outer:") >= 0, 
        "Should contain outer text");
    TEST_ASSERT_TRUE_MESSAGE(result4.indexOf("Inner:") >= 0, 
        "Should contain inner text");
    TEST_ASSERT_TRUE_MESSAGE(result4.indexOf(testTitle) >= 0, 
        "Should contain placeholder value");
    
    // Test deep nesting (up to MAX_RENDERING_DEPTH)
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
    String result5 = captureRenderedOutput(ctx);
    TEST_ASSERT_TRUE_MESSAGE(result5.length() > 0, "Should render deep nested template");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), 
        "Should complete deep nested template");
    
    Serial.println("[TEST:test_template_renderer_nested]   TemplateRenderer nested template tests completed successfully");
}

void test_template_renderer_nested_four_levels() {
    Serial.println("[TEST:test_template_renderer_nested_four_levels]   Ensuring four-level nesting with chunked output...");

    PlaceholderRegistry registry(8);
    registry.registerProgmemTemplate("%L2%", nested_four_levels_level2);
    registry.registerProgmemTemplate("%L3%", nested_four_levels_level3);
    registry.registerProgmemTemplate("%L4%", nested_four_levels_level4);

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_four_levels_outer);

    const size_t chunkSize = 4;
    uint8_t buffer[chunkSize];
    String rendered;
    size_t iterations = 0;

    while (!TemplateRenderer::isComplete(ctx) && !ctx.hasError()) {
        size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, chunkSize);
        iterations++;
        TEST_ASSERT_FALSE_MESSAGE(ctx.hasError(), "Four-level nested rendering should not error");
        TEST_ASSERT_TRUE_MESSAGE(written > 0 || TemplateRenderer::isComplete(ctx), "Renderer stalled before completion");

        for (size_t i = 0; i < written; ++i) {
            rendered += static_cast<char>(buffer[i]);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), "Four-level nested render should complete");
    TEST_ASSERT_GREATER_THAN_MESSAGE((unsigned int)1, iterations, "Four-level nested render should require multiple chunks");

    const String expected = "L1: L2: L3: L4 content";
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.c_str(), rendered.c_str(), "Four-level nested render should match expected output");
}

void test_template_renderer_nested_chunk_progress() {
    Serial.println("[TEST:test_template_renderer_nested_chunk_progress]   Verifying nested templates with tiny chunks...");

    PlaceholderRegistry registry(10);
    registry.registerProgmemTemplate("%MID%", nested_three_levels_mid);
    registry.registerProgmemTemplate("%INNER%", nested_three_levels_inner);
    registry.registerProgmemTemplate("%DEEP%", nested_three_levels_deep);

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, nested_three_levels_outer);

    const size_t chunkSize = 3;
    uint8_t buffer[chunkSize];
    String rendered;

    while (!TemplateRenderer::isComplete(ctx) && !ctx.hasError()) {
        size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, chunkSize);
        TEST_ASSERT_FALSE_MESSAGE(ctx.hasError(), "Context should not enter error state");
        TEST_ASSERT_MESSAGE(written > 0 || ctx.hasError() || TemplateRenderer::isComplete(ctx), "Renderer stalled before completion");

        for (size_t i = 0; i < written; ++i) {
            rendered += static_cast<char>(buffer[i]);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), "Nested render should complete");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Outer:") >= 0, "Rendered string should include outer text");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Deep content") >= 0, "Rendered string should include deepest content");
}

void test_template_renderer_large_templates() {
    Serial.println("[TEST:test_template_renderer_large_templates]   Rendering large web-style templates with small chunks...");

    PlaceholderRegistry registry(20);
    registry.registerProgmemTemplate("%HEADER%", simulated_web_header);
    registry.registerProgmemTemplate("%CONTENT%", simulated_web_content);
    registry.registerProgmemTemplate("%FOOTER%", simulated_web_footer);
    registry.registerProgmemTemplate("%NAV%", simulated_web_nav);
    registry.registerProgmemTemplate("%SIDEBAR%", simulated_web_sidebar);
    registry.registerRamData("%TITLE%", getTestTitle);
    registry.registerRamData("%THEME%", getTestTheme);
    registry.registerRamData("%RSSI%", getTestRssi);
    registry.registerRamData("%HEAP%", getTestHeap);
    registry.registerRamData("%UPTIME%", getTestUptime);
    registry.registerProgmemData("%STYLES%", test_css_data);
    registry.registerProgmemData("%SCRIPTS%", simulated_web_scripts);
    registry.registerProgmemData("%LOGO_BASE64%", simulated_logo_base64);

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, simulated_web_outer);

    const size_t chunkSize = 32;
    uint8_t buffer[chunkSize];
    String rendered;
    size_t iterations = 0;

    while (!TemplateRenderer::isComplete(ctx) && !ctx.hasError()) {
        size_t written = TemplateRenderer::renderNextChunk(ctx, buffer, chunkSize);
        iterations++;
        TEST_ASSERT_FALSE_MESSAGE(ctx.hasError(), "Large template render should not error");
        TEST_ASSERT_TRUE_MESSAGE(written > 0 || TemplateRenderer::isComplete(ctx), "Renderer stalled while processing large template");

        for (size_t i = 0; i < written; ++i) {
            rendered += static_cast<char>(buffer[i]);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), "Large template render should complete");
    TEST_ASSERT_GREATER_THAN_MESSAGE((unsigned int)1, iterations, "Large template should require multiple render iterations");
    TEST_ASSERT_TRUE_MESSAGE(rendered.length() > chunkSize, "Rendered output should be larger than a single chunk");

    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("<header class=\"app-header\">") >= 0, "Rendered output should include header section");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("<aside class=\"app-sidebar\">") >= 0, "Rendered output should include sidebar");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("queueAction('factory-reset')") >= 0, "Rendered output should include automation actions");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("data-theme=\"dark\"") >= 0, "Rendered output should include theme value");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf(testTitle.c_str()) >= 0, "Rendered output should include page title");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf(testRssi.c_str()) >= 0, "Rendered output should include RSSI metric");

    TEST_ASSERT_EQUAL_MESSAGE(-1, (int)rendered.indexOf("%HEADER%"), "Rendered output should not contain unresolved %HEADER% placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(-1, (int)rendered.indexOf("%NAV%"), "Rendered output should not contain unresolved %NAV% placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(-1, (int)rendered.indexOf("%CONTENT%"), "Rendered output should not contain unresolved %CONTENT% placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(-1, (int)rendered.indexOf("%SIDEBAR%"), "Rendered output should not contain unresolved %SIDEBAR% placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(-1, (int)rendered.indexOf("%RSSI%"), "Rendered output should not contain unresolved %RSSI% placeholder");
}

void test_template_renderer_parallel_contexts() {
    Serial.println("[TEST:test_template_renderer_parallel_contexts]   Verifying independent contexts...");

    PlaceholderRegistry registry(10);
    registry.registerProgmemTemplate("%NESTED%", nested_with_placeholder_inner);
    registry.registerRamData("%PLACEHOLDER%", getTestTitle);

    TemplateContext ctxA;
    TemplateContext ctxB;
    ctxA.setRegistry(&registry);
    ctxB.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctxA, nested_with_placeholder_outer);
    TemplateRenderer::initializeContext(ctxB, nested_with_placeholder_outer);

    const size_t chunkSize = 5;
    uint8_t bufferA[chunkSize];
    uint8_t bufferB[chunkSize];
    String outputA;
    String outputB;

    bool running = true;
    while (running) {
        running = false;

        if (!TemplateRenderer::isComplete(ctxA) && !ctxA.hasError()) {
            size_t writtenA = TemplateRenderer::renderNextChunk(ctxA, bufferA, chunkSize);
            TEST_ASSERT_FALSE_MESSAGE(ctxA.hasError(), "Context A should not enter error state");
            TEST_ASSERT_MESSAGE(writtenA > 0 || ctxA.hasError() || TemplateRenderer::isComplete(ctxA), "Context A stalled");
            for (size_t i = 0; i < writtenA; ++i) {
                outputA += static_cast<char>(bufferA[i]);
            }
        }

        if (!TemplateRenderer::isComplete(ctxB) && !ctxB.hasError()) {
            size_t writtenB = TemplateRenderer::renderNextChunk(ctxB, bufferB, chunkSize);
            TEST_ASSERT_FALSE_MESSAGE(ctxB.hasError(), "Context B should not enter error state");
            TEST_ASSERT_MESSAGE(writtenB > 0 || ctxB.hasError() || TemplateRenderer::isComplete(ctxB), "Context B stalled");
            for (size_t i = 0; i < writtenB; ++i) {
                outputB += static_cast<char>(bufferB[i]);
            }
        }

        running = !TemplateRenderer::isComplete(ctxA) || !TemplateRenderer::isComplete(ctxB);
    }

    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctxA), "Context A should complete");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctxB), "Context B should complete");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(outputA.c_str(), outputB.c_str(), "Interleaved contexts should produce identical output");
}

void test_template_renderer_root_ram_template() {
    PlaceholderRegistry registry(4);
    registry.registerRamData("%TITLE%", getTestTitle);

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, ram_root_template, false);
    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Status: Test Title", rendered.c_str(), "RAM root template should render correctly");
}

void test_template_renderer_missing_registry() {
    TemplateContext ctx;
    TemplateRenderer::initializeContext(ctx, ram_root_template, false);

    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Status: ", rendered.c_str(), "Renderer should skip placeholder when registry missing");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), "Renderer should complete without registry");

    ctx.reset();
    TemplateRenderer::initializeContext(ctx, ram_root_text_only, false);
    String textOnly = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("RAM root template", textOnly.c_str(), "RAM-only root template should render without registry");
}

void test_template_renderer_dynamic_basic() {
    PlaceholderRegistry registry(6);
    dynamicGreetingState.value = "Hello Device";
    TEST_ASSERT_TRUE_MESSAGE(registry.registerDynamicTemplate("%DYNAMIC_GREETING%", &dynamicGreetingDescriptor), "Dynamic placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, dynamicWrapperTemplate);
    String full = captureRenderedOutput(ctx, 64);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Greeting: Hello Device", full.c_str(), "Dynamic placeholder should render inline");

    ctx.reset();
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, dynamicWrapperTemplate);
    String chunked = captureRenderedOutput(ctx, 5);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(full.c_str(), chunked.c_str(), "Dynamic placeholder should render identically with small chunks");
}

void test_template_renderer_dynamic_empty() {
    PlaceholderRegistry registry(4);
    dynamicGreetingState.value = "";
    TEST_ASSERT_TRUE_MESSAGE(registry.registerDynamicTemplate("%DYNAMIC_GREETING%", &dynamicGreetingDescriptor), "Dynamic placeholder should register when empty");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, dynamicWrapperTemplate);
    String result = captureRenderedOutput(ctx, 16);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Greeting: ", result.c_str(), "Empty dynamic placeholder should yield empty string");
    TEST_ASSERT_TRUE_MESSAGE(TemplateRenderer::isComplete(ctx), "Renderer should complete for empty dynamic placeholder");
}

void test_template_renderer_dynamic_mutable() {
    PlaceholderRegistry registry(4);
    TEST_ASSERT_TRUE_MESSAGE(registry.registerDynamicTemplate("%DYNAMIC_GREETING%", &dynamicGreetingDescriptor), "Dynamic placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);

    dynamicGreetingState.value = "First";
    TemplateRenderer::initializeContext(ctx, dynamicMixedTemplate);
    String first = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Start-First-End", first.c_str(), "Dynamic placeholder should reflect initial value");

    ctx.reset();
    ctx.setRegistry(&registry);
    dynamicGreetingState.value = "Second";
    TemplateRenderer::initializeContext(ctx, dynamicMixedTemplate);
    String second = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Start-Second-End", second.c_str(), "Dynamic placeholder should reflect updated value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(first.c_str(), second.c_str(), "Dynamic placeholder outputs should differ");
}

void test_template_renderer_conditional_true_branch() {
    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%COND_TRUE%", wifiTrueBadgeTemplate);
    registry.registerProgmemData("%COND_FALSE%", wifiFalseBadgeTemplate);

    conditionalState.result = ConditionalBranchResult::TRUE_BRANCH;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerConditional("%STATUS_BLOCK%", &conditionalDescriptor), "Conditional placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, conditionalWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Status:[WiFi ENABLED]:Done", rendered.c_str(), "Conditional true branch should render badge");
}

void test_template_renderer_conditional_false_branch() {
    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%COND_TRUE%", wifiTrueBadgeTemplate);
    registry.registerProgmemData("%COND_FALSE%", wifiFalseBadgeTemplate);

    conditionalState.result = ConditionalBranchResult::FALSE_BRANCH;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerConditional("%STATUS_BLOCK%", &conditionalDescriptor), "Conditional placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, conditionalWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Status:[WiFi DISABLED]:Done", rendered.c_str(), "Conditional false branch should render alternate badge");
}

void test_template_renderer_conditional_skip() {
    PlaceholderRegistry registry(8);
    registry.registerProgmemData("%COND_TRUE%", wifiTrueBadgeTemplate);
    registry.registerProgmemData("%COND_FALSE%", wifiFalseBadgeTemplate);

    conditionalState.result = ConditionalBranchResult::SKIP;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerConditional("%STATUS_BLOCK%", &skipConditionalDescriptor), "Conditional placeholder should register for skip case");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, conditionalSkipTemplate);
    String rendered = captureRenderedOutput(ctx, 16);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Value[]", rendered.c_str(), "Conditional skip should omit content");
}

void test_template_renderer_conditional_missing_delegate() {
    PlaceholderRegistry registry(6);
    registry.registerProgmemData("%COND_TRUE%", wifiTrueBadgeTemplate);

    conditionalState.result = ConditionalBranchResult::FALSE_BRANCH;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerConditional("%STATUS_BLOCK%", &conditionalDescriptor), "Conditional placeholder should register even if delegate missing");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, conditionalWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Status::Done", rendered.c_str(), "Missing delegate should skip rendering without leaving token");
}

void test_template_renderer_conditional_nested_iterator() {
    PlaceholderRegistry registry(12);
    registry.registerProgmemData("%COND_TRUE%", wifiTrueBadgeTemplate);
    registry.registerProgmemData("%COND_FALSE%", wifiFalseBadgeTemplate);

    resetWifiIteratorState(3);
    wifiIteratorState.includeOverrides = true;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerIterator("%WIFI_LIST%", &wifiIteratorDescriptor), "Iterator placeholder should register");

    conditionalState.result = ConditionalBranchResult::TRUE_BRANCH;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerConditional("%WIFI_BADGE%", &iteratorConditionalDescriptor), "Conditional placeholder should delegate to iterator");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, iteratorConditionalWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 64);
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-A") >= 0, "Iterator content should appear inside conditional");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-B") >= 0, "Iterator should render second item");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-C") >= 0, "Iterator should render third item");
}

void test_template_renderer_iterator_basic() {
    PlaceholderRegistry registry(8);
    resetWifiIteratorState(3);
    wifiIteratorState.includeOverrides = true;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerIterator("%WIFI_LIST%", &wifiIteratorDescriptor), "Iterator placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, iteratorWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 32);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("<ul><li>Net-A (-41)</li><li>Net-B (-55)</li><li>Net-C (-70)</li></ul>", rendered.c_str(), "Iterator should render all items with overrides");
    TEST_ASSERT_TRUE_MESSAGE(wifiIteratorState.closeCalled, "Iterator close handler should run");
}

void test_template_renderer_iterator_empty() {
    PlaceholderRegistry registry(8);
    resetWifiIteratorState(0);
    TEST_ASSERT_TRUE_MESSAGE(registry.registerIterator("%WIFI_LIST%", &wifiIteratorDescriptor), "Iterator placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, iteratorWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 16);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("<ul></ul>", rendered.c_str(), "Iterator with zero items should render empty list");
    TEST_ASSERT_TRUE_MESSAGE(wifiIteratorState.closeCalled, "Iterator close handler should run for empty list");
}

void test_template_renderer_iterator_dynamic_items() {
    PlaceholderRegistry registry(8);
    resetWifiIteratorState(3);
    wifiIteratorState.useDynamicTemplate = true;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerIterator("%WIFI_LIST%", &wifiIteratorDescriptor), "Iterator placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, iteratorWrapperTemplate);
    String rendered = captureRenderedOutput(ctx, 16);
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-A [-41dBm]") >= 0, "Dynamic iterator item should include Net-A");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-B [-55dBm]") >= 0, "Dynamic iterator item should include Net-B");
    TEST_ASSERT_TRUE_MESSAGE(rendered.indexOf("Net-C [-70dBm]") >= 0, "Dynamic iterator item should include Net-C");
}

void test_template_renderer_iterator_error_cleanup() {
    PlaceholderRegistry registry(8);
    resetWifiIteratorState(2);
    wifiIteratorState.includeOverrides = true;
    wifiIteratorState.produceError = true;
    wifiIteratorState.errorAfter = 1;
    TEST_ASSERT_TRUE_MESSAGE(registry.registerIterator("%WIFI_LIST%", &wifiIteratorDescriptor), "Iterator placeholder should register");

    TemplateContext ctx;
    ctx.setRegistry(&registry);
    TemplateRenderer::initializeContext(ctx, iteratorWrapperTemplate);

    const size_t chunkSize = 16;
    uint8_t buffer[chunkSize];
    while (!TemplateRenderer::isComplete(ctx) && !ctx.hasError()) {
        TemplateRenderer::renderNextChunk(ctx, buffer, chunkSize);
    }

    TEST_ASSERT_TRUE_MESSAGE(ctx.hasError(), "Iterator error should propagate to context");
    TEST_ASSERT_TRUE_MESSAGE(wifiIteratorState.closeCalled, "Iterator close should run on error");
}

