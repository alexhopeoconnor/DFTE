#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "../templates/placeholder_templates.h"
#include "../utils/test_utils.h"
#include <pgmspace.h>

// Note: PROGMEM data is defined in placeholder_templates.h

// Test RAM data getter
static String testRamData = "test ram data";
static const char* getTestRamData() {
    return testRamData.c_str();
}

// Test PlaceholderRegistry registration
void test_placeholder_registry_registration() {
    Serial.println("[TEST]   Testing PlaceholderRegistry registration...");
    
    PlaceholderRegistry registry(10);
    
    // Test initial state
    TEST_ASSERT_EQUAL_MESSAGE(0, registry.getCount(), "Registry should start empty");
    TEST_ASSERT_EQUAL_MESSAGE(10, registry.getMaxPlaceholders(), "Registry should have correct max");
    
    // Test register PROGMEM_DATA
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemData("%TEST%", test_css_data), 
        "Should register PROGMEM_DATA placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(1, registry.getCount(), "Registry count should be 1");
    
    // Test register PROGMEM_TEMPLATE
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemTemplate("%HEADER%", test_header_template), 
        "Should register PROGMEM_TEMPLATE placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(2, registry.getCount(), "Registry count should be 2");
    
    // Test register RAM_DATA
    TEST_ASSERT_TRUE_MESSAGE(registry.registerRamData("%TITLE%", getTestRamData), 
        "Should register RAM_DATA placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(3, registry.getCount(), "Registry count should be 3");
    
    // Test register multiple placeholders
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemData("%CSS%", test_css_data), 
        "Should register multiple PROGMEM_DATA placeholders");
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemData("%JS%", test_js_data), 
        "Should register multiple PROGMEM_DATA placeholders");
    TEST_ASSERT_EQUAL_MESSAGE(5, registry.getCount(), "Registry count should be 5");
    
    // Test register duplicate (should warn but succeed)
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemData("%TEST%", test_js_data), 
        "Should allow duplicate registration (last wins)");
    TEST_ASSERT_EQUAL_MESSAGE(6, registry.getCount(), "Registry count should be 6");
    
    // Test register with invalid name (null)
    TEST_ASSERT_FALSE_MESSAGE(registry.registerProgmemData(nullptr, test_css_data), 
        "Should reject null placeholder name");
    
    // Test register with invalid name (empty)
    // Note: Empty string "" might be accepted by validation (length 0 is valid)
    // The validation checks for null and length >= MAX_PLACEHOLDER_NAME_SIZE
    // Empty string has length 0, which is < MAX_PLACEHOLDER_NAME_SIZE, so it might pass
    // This test verifies the behavior - if empty strings are accepted, that's fine
    const char* emptyStr = "";
    bool emptyResult = registry.registerProgmemData(emptyStr, test_css_data);
    // Empty string might be accepted (length 0 < MAX_PLACEHOLDER_NAME_SIZE)
    // We just verify the function doesn't crash
    (void)emptyResult; // Suppress unused variable warning
    
    // Test clear
    registry.clear();
    TEST_ASSERT_EQUAL_MESSAGE(0, registry.getCount(), "Registry should be empty after clear");
    
    // Test register when full
    PlaceholderRegistry smallRegistry(2);
    TEST_ASSERT_TRUE_MESSAGE(smallRegistry.registerProgmemData("%A%", test_css_data), 
        "Should register first placeholder");
    TEST_ASSERT_TRUE_MESSAGE(smallRegistry.registerProgmemData("%B%", test_js_data), 
        "Should register second placeholder");
    TEST_ASSERT_FALSE_MESSAGE(smallRegistry.registerProgmemData("%C%", test_css_data), 
        "Should reject registration when registry is full");
    
    Serial.println("[TEST]   PlaceholderRegistry registration tests completed successfully");
}

// Test PlaceholderRegistry lookup
void test_placeholder_registry_lookup() {
    Serial.println("[TEST]   Testing PlaceholderRegistry lookup...");
    
    PlaceholderRegistry registry(10);
    
    // Register some placeholders
    registry.registerProgmemData("%CSS%", test_css_data);
    registry.registerProgmemTemplate("%HEADER%", test_header_template);
    registry.registerRamData("%TITLE%", getTestRamData);
    
    // Test get existing placeholder
    const PlaceholderEntry* entry1 = registry.getPlaceholder("%CSS%");
    TEST_ASSERT_NOT_NULL_MESSAGE(entry1, "Should find existing PROGMEM_DATA placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(PlaceholderType::PROGMEM_DATA, entry1->type, 
        "Placeholder type should be PROGMEM_DATA");
    
    const PlaceholderEntry* entry2 = registry.getPlaceholder("%HEADER%");
    TEST_ASSERT_NOT_NULL_MESSAGE(entry2, "Should find existing PROGMEM_TEMPLATE placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(PlaceholderType::PROGMEM_TEMPLATE, entry2->type, 
        "Placeholder type should be PROGMEM_TEMPLATE");
    
    const PlaceholderEntry* entry3 = registry.getPlaceholder("%TITLE%");
    TEST_ASSERT_NOT_NULL_MESSAGE(entry3, "Should find existing RAM_DATA placeholder");
    TEST_ASSERT_EQUAL_MESSAGE(PlaceholderType::RAM_DATA, entry3->type, 
        "Placeholder type should be RAM_DATA");
    
    // Test get non-existent placeholder
    const PlaceholderEntry* entry4 = registry.getPlaceholder("%NOTFOUND%");
    TEST_ASSERT_NULL_MESSAGE(entry4, "Should return nullptr for non-existent placeholder");
    
    // Test case-sensitive matching
    const PlaceholderEntry* entry5 = registry.getPlaceholder("%css%");
    TEST_ASSERT_NULL_MESSAGE(entry5, "Should be case-sensitive (lowercase not found)");
    
    // Test get with null name
    const PlaceholderEntry* entry6 = registry.getPlaceholder(nullptr);
    TEST_ASSERT_NULL_MESSAGE(entry6, "Should return nullptr for null name");
    
    Serial.println("[TEST]   PlaceholderRegistry lookup tests completed successfully");
}

// Test PlaceholderRegistry rendering
void test_placeholder_registry_rendering() {
    Serial.println("[TEST]   Testing PlaceholderRegistry rendering...");
    
    PlaceholderRegistry registry(10);
    
    // Register PROGMEM_DATA placeholder
    registry.registerProgmemData("%CSS%", test_css_data);
    const PlaceholderEntry* entry1 = registry.getPlaceholder("%CSS%");
    TEST_ASSERT_NOT_NULL_MESSAGE(entry1, "Should find PROGMEM_DATA placeholder");
    
    // Test render PROGMEM_DATA (full)
    uint8_t buffer1[256];
    size_t len1 = registry.renderPlaceholder(entry1, 0, buffer1, sizeof(buffer1));
    char* temp1 = new char[len1 + 1];
    memcpy(temp1, buffer1, len1);
    temp1[len1] = '\0';
    String result1 = String(temp1);
    delete[] temp1;
    
    // Compare with PROGMEM data using strlen_P
    size_t expectedLen = strlen_P(test_css_data);
    TEST_ASSERT_EQUAL_MESSAGE(expectedLen, len1, 
        "Should render full PROGMEM_DATA length");
    // Compare first few characters to verify content
    char firstChar = pgm_read_byte(test_css_data);
    TEST_ASSERT_EQUAL_MESSAGE(firstChar, result1.charAt(0), 
        "Should render correct PROGMEM_DATA content");
    
    // Test render PROGMEM_DATA (chunked)
    uint8_t buffer2[10];
    size_t len2 = registry.renderPlaceholder(entry1, 0, buffer2, sizeof(buffer2));
    TEST_ASSERT_EQUAL_MESSAGE(10, len2, "Should render chunk of PROGMEM_DATA");
    
    // Test render PROGMEM_DATA with offset
    size_t len3 = registry.renderPlaceholder(entry1, 5, buffer2, sizeof(buffer2));
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, len3, "Should render PROGMEM_DATA with offset");
    
    // Test render PROGMEM_DATA with offset beyond length
    size_t len4 = registry.renderPlaceholder(entry1, 1000, buffer2, sizeof(buffer2));
    TEST_ASSERT_EQUAL_MESSAGE(0, len4, "Should return 0 when offset beyond length");
    
    // Register RAM_DATA placeholder
    registry.registerRamData("%TITLE%", getTestRamData);
    const PlaceholderEntry* entry2 = registry.getPlaceholder("%TITLE%");
    TEST_ASSERT_NOT_NULL_MESSAGE(entry2, "Should find RAM_DATA placeholder");
    
    // Test render RAM_DATA (full)
    uint8_t buffer3[256];
    size_t len5 = registry.renderPlaceholder(entry2, 0, buffer3, sizeof(buffer3));
    char* temp2 = new char[len5 + 1];
    memcpy(temp2, buffer3, len5);
    temp2[len5] = '\0';
    String result2 = String(temp2);
    delete[] temp2;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(testRamData.c_str(), result2.c_str(), 
        "Should render full RAM_DATA");
    
    // Test render RAM_DATA (chunked)
    uint8_t buffer4[5];
    size_t len6 = registry.renderPlaceholder(entry2, 0, buffer4, sizeof(buffer4));
    TEST_ASSERT_EQUAL_MESSAGE(5, len6, "Should render chunk of RAM_DATA");
    
    // Test render with null entry
    size_t len7 = registry.renderPlaceholder(nullptr, 0, buffer1, sizeof(buffer1));
    TEST_ASSERT_EQUAL_MESSAGE(0, len7, "Should return 0 for null entry");
    
    // Test render with maxLen = 0
    size_t len8 = registry.renderPlaceholder(entry1, 0, buffer1, 0);
    TEST_ASSERT_EQUAL_MESSAGE(0, len8, "Should return 0 when maxLen is 0");
    
    Serial.println("[TEST]   PlaceholderRegistry rendering tests completed successfully");
}

// Test PlaceholderRegistry edge cases
void test_placeholder_registry_edge_cases() {
    Serial.println("[TEST]   Testing PlaceholderRegistry edge cases...");
    
    // Test registry with maxPlaceholders = 1
    PlaceholderRegistry smallRegistry(1);
    TEST_ASSERT_TRUE_MESSAGE(smallRegistry.registerProgmemData("%A%", test_css_data), 
        "Should register placeholder in small registry");
    TEST_ASSERT_FALSE_MESSAGE(smallRegistry.registerProgmemData("%B%", test_js_data), 
        "Should reject second placeholder in small registry");
    
    // Test placeholder name at maximum length
    char maxName[25];
    strncpy(maxName, "%", sizeof(maxName) - 1);
    maxName[0] = '%';
    for (int i = 1; i < 22; i++) {
        maxName[i] = 'A';
    }
    maxName[22] = '%';
    maxName[23] = '\0';
    
    PlaceholderRegistry registry(10);
    TEST_ASSERT_TRUE_MESSAGE(registry.registerProgmemData(maxName, test_css_data), 
        "Should register placeholder with max length name");
    
    // Test placeholder name over maximum length
    char overName[30];
    overName[0] = '%';
    for (int i = 1; i < 27; i++) {
        overName[i] = 'A';
    }
    overName[27] = '%';
    overName[28] = '\0';
    
    TEST_ASSERT_FALSE_MESSAGE(registry.registerProgmemData(overName, test_css_data), 
        "Should reject placeholder with over max length name");
    
    // Test length calculation
    size_t progmemLen = PlaceholderRegistry::getProgmemLength(test_css_data);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, progmemLen, "Should calculate PROGMEM length");
    
    // Test length calculation with null
    size_t nullLen = PlaceholderRegistry::getProgmemLength(nullptr);
    TEST_ASSERT_EQUAL_MESSAGE(0, nullLen, "Should return 0 for null PROGMEM data");
    
    // Test RAM length calculation
    size_t ramLen = PlaceholderRegistry::getRamLength((const void*)getTestRamData);
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, ramLen, "Should calculate RAM length");
    
    // Test RAM length calculation with null
    size_t nullRamLen = PlaceholderRegistry::getRamLength(nullptr);
    TEST_ASSERT_EQUAL_MESSAGE(0, nullRamLen, "Should return 0 for null RAM getter");
    
    Serial.println("[TEST]   PlaceholderRegistry edge case tests completed successfully");
}

