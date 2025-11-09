#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "test_main.h"
#include "utils/test_utils.h"
#include "DeviceFrameworkTemplateEngineDebug.h"

// Logger implementation for tests - uses Serial.println
class TestTemplateEngineLogger : public DeviceFrameworkTemplateEngineLogger {
public:
    void error(const String& msg) override {
        Serial.print("[DFTE ERROR] ");
        Serial.println(msg);
    }
    
    void warn(const String& msg) override {
        Serial.print("[DFTE WARN] ");
        Serial.println(msg);
    }
    
    void info(const String& msg) override {
        Serial.print("[DFTE INFO] ");
        Serial.println(msg);
    }
    
    void debug(const String& msg) override {
        Serial.print("[DFTE DEBUG] ");
        Serial.println(msg);
    }
};

// Global logger instance
TestTemplateEngineLogger testLogger;

// Test case array
TestCase tests[] = {
    // Group 1: PlaceholderRegistry Tests
    TEST_ENTRY(test_placeholder_registry_registration),
    TEST_ENTRY(test_placeholder_registry_lookup),
    TEST_ENTRY(test_placeholder_registry_rendering),
    TEST_ENTRY(test_placeholder_registry_edge_cases),
    
    // Group 2: TemplateContext Tests
    TEST_ENTRY(test_template_context_initialization),
    TEST_ENTRY(test_template_context_stack),
    TEST_ENTRY(test_template_context_buffer),
    TEST_ENTRY(test_template_context_state),
    
    // Group 3: TemplateRenderer Tests
    TEST_ENTRY(test_template_renderer_basic),
    TEST_ENTRY(test_template_renderer_chunked),
    TEST_ENTRY(test_template_renderer_state_transitions),
    TEST_ENTRY(test_template_renderer_placeholders),
    TEST_ENTRY(test_template_renderer_nested),
    TEST_ENTRY(test_template_renderer_nested_four_levels),
    TEST_ENTRY(test_template_renderer_nested_chunk_progress),
    TEST_ENTRY(test_template_renderer_large_templates),
    TEST_ENTRY(test_template_renderer_parallel_contexts),
    TEST_ENTRY(test_template_renderer_root_ram_template),
    TEST_ENTRY(test_template_renderer_missing_registry),
    TEST_ENTRY(test_template_renderer_dynamic_basic),
    TEST_ENTRY(test_template_renderer_dynamic_empty),
    TEST_ENTRY(test_template_renderer_dynamic_mutable),
    TEST_ENTRY(test_template_renderer_conditional_true_branch),
    TEST_ENTRY(test_template_renderer_conditional_false_branch),
    TEST_ENTRY(test_template_renderer_conditional_skip),
    TEST_ENTRY(test_template_renderer_conditional_missing_delegate),
    TEST_ENTRY(test_template_renderer_conditional_nested_iterator),
    TEST_ENTRY(test_template_renderer_iterator_basic),
    TEST_ENTRY(test_template_renderer_iterator_empty),
    TEST_ENTRY(test_template_renderer_iterator_dynamic_items),
    TEST_ENTRY(test_template_renderer_iterator_error_cleanup),
    
    // Group 4: Integration Tests
    TEST_ENTRY(test_integration_full_rendering),
    TEST_ENTRY(test_integration_memory_efficiency),
    TEST_ENTRY(test_integration_multiple_templates),
    
    // Group 5: Edge Cases
    TEST_ENTRY(test_edge_cases_error_handling),
    TEST_ENTRY(test_edge_cases_boundary_conditions),
    TEST_ENTRY(test_edge_cases_stress),
};

const size_t TEST_COUNT = sizeof(tests) / sizeof(TestCase);

// Test state variables
size_t next_index = 0;
bool begun = false;

// Test setup and teardown
void setUp(void) {
    // No per-test setup needed
}

void tearDown(void) {
    // No per-test teardown needed
}

// Arduino setup and loop functions (required by framework)
void setup() {
    Serial.begin(115200);
    delay(2000); // Give time for serial to initialize
    
    Serial.println("\n[TEST] =============================================");
    Serial.println("[TEST] === DeviceFrameworkTemplateEngine Test Setup ===");
    Serial.println("[TEST] Starting template engine tests...");
    Serial.println("[TEST] =============================================");
    
    // Enable template engine debug logging
    deviceFrameworkTemplateEngineEnableLogging(&testLogger);
    Serial.println("[TEST] Template engine debug logging enabled");
    
    UNITY_BEGIN(); // Start Unity test framework
    begun = true; // Start tests immediately
}

void loop() {
    // Run one test and return immediately
    if (begun && next_index < TEST_COUNT) {
        TestCase& t = tests[next_index];
        Serial.print("\n[TEST] ==== Running test: ");
        Serial.print(t.name);
        Serial.println(" ====");
        UnityDefaultTestRun(t.fn, t.name, t.line);
        next_index++;
        return; // yield quickly
    }
    
    // All tests completed
    if (begun && next_index >= TEST_COUNT) {
        Serial.println("\n[TEST] =============================================");
        Serial.println("[TEST] === All tests completed ===");
        Serial.println("[TEST] =============================================");
        UNITY_END(); // prints Unity summary
        begun = false; // avoid repeating
    }
}

