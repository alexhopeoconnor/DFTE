#include <unity.h>
#include <Arduino.h>
#include <TemplateEngine.h>
#include "../templates/simple_templates.h"
#include "../utils/test_utils.h"

// Test TemplateContext initialization
void test_template_context_initialization() {
    Serial.println("[TEST]   Testing TemplateContext initialization...");
    
    TemplateContext ctx;
    
    // Test default constructor
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::TEXT, ctx.state, 
        "Initial state should be TEXT");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.renderingDepth, "Initial renderingDepth should be 0");
    TEST_ASSERT_NULL_MESSAGE(ctx.getCurrentContext(), "Initial context should be null");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.placeholderPos, "Initial placeholderPos should be 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.bufferPos, "Initial bufferPos should be 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.bufferLen, "Initial bufferLen should be 0");
    TEST_ASSERT_NULL_MESSAGE(ctx.registry, "Initial registry should be null");
    
    // Test reset
    ctx.pushContext(RenderingContextType::TEMPLATE, "TEST");
    ctx.renderingDepth = 5;
    ctx.reset();
    
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::TEXT, ctx.state, 
        "Reset state should be TEXT");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.renderingDepth, "Reset renderingDepth should be 0");
    TEST_ASSERT_NULL_MESSAGE(ctx.getCurrentContext(), "Reset context should be null");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.placeholderPos, "Reset placeholderPos should be 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.bufferPos, "Reset bufferPos should be 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.bufferLen, "Reset bufferLen should be 0");
    
    // Test isComplete
    TEST_ASSERT_FALSE_MESSAGE(ctx.isComplete(), "Should not be complete initially");
    ctx.state = TemplateRenderState::COMPLETE;
    TEST_ASSERT_TRUE_MESSAGE(ctx.isComplete(), "Should be complete when state is COMPLETE");
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_TRUE_MESSAGE(ctx.isComplete(), "Should be complete when state is ERROR");
    
    // Test hasError
    ctx.state = TemplateRenderState::TEXT;
    TEST_ASSERT_FALSE_MESSAGE(ctx.hasError(), "Should not have error initially");
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_TRUE_MESSAGE(ctx.hasError(), "Should have error when state is ERROR");
    
    // Test getStateString
    ctx.state = TemplateRenderState::TEXT;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TEXT", ctx.getStateString().c_str(), 
        "Should return correct state string for TEXT");
    ctx.state = TemplateRenderState::RENDERING_CONTEXT;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("RENDERING_CONTEXT", ctx.getStateString().c_str(), 
        "Should return correct state string for RENDERING_CONTEXT");
    ctx.state = TemplateRenderState::COMPLETE;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("COMPLETE", ctx.getStateString().c_str(), 
        "Should return correct state string for COMPLETE");
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("ERROR", ctx.getStateString().c_str(), 
        "Should return correct state string for ERROR");
    
    // Test setRegistry
    PlaceholderRegistry registry(10);
    ctx.setRegistry(&registry);
    TEST_ASSERT_EQUAL_MESSAGE(&registry, ctx.registry, "Should set registry correctly");
    
    Serial.println("[TEST]   TemplateContext initialization tests completed successfully");
}

// Test TemplateContext stack
void test_template_context_stack() {
    Serial.println("[TEST]   Testing TemplateContext stack...");
    
    TemplateContext ctx;
    const char* template1 = plain_text_template;
    const char* template2 = single_placeholder_template;
    const char* template3 = multiple_placeholders_template;
    
    size_t len1 = strlen_P(template1);
    size_t len2 = strlen_P(template2);
    size_t len3 = strlen_P(template3);
    
    // Test push context
    ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE1%");
    RenderingContext* ctx1 = ctx.getCurrentContext();
    ctx1->context.templateCtx.templateData = template1;
    ctx1->context.templateCtx.templateLen = len1;
    ctx1->context.templateCtx.isProgmem = true;
    ctx1->context.templateCtx.position = 0;
    
    TEST_ASSERT_EQUAL_MESSAGE(1, ctx.renderingDepth, "Stack depth should be 1 after push");
    TEST_ASSERT_NOT_NULL_MESSAGE(ctx.getCurrentContext(), "Current context should not be null");
    TEST_ASSERT_EQUAL_MESSAGE(template1, ctx.getCurrentContext()->context.templateCtx.templateData, 
        "Current template should be pushed template");
    TEST_ASSERT_EQUAL_MESSAGE(len1, ctx.getCurrentContext()->context.templateCtx.templateLen, "Template length should be set");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.getCurrentContext()->context.templateCtx.position, "Template position should be reset");
    
    // Test push multiple templates
    ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE2%");
    RenderingContext* ctx2 = ctx.getCurrentContext();
    ctx2->context.templateCtx.templateData = template2;
    ctx2->context.templateCtx.templateLen = len2;
    ctx2->context.templateCtx.isProgmem = true;
    ctx2->context.templateCtx.position = 0;
    
    TEST_ASSERT_EQUAL_MESSAGE(2, ctx.renderingDepth, "Stack depth should be 2 after second push");
    TEST_ASSERT_EQUAL_MESSAGE(template2, ctx.getCurrentContext()->context.templateCtx.templateData, 
        "Current template should be second pushed template");
    
    ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE3%");
    RenderingContext* ctx3 = ctx.getCurrentContext();
    ctx3->context.templateCtx.templateData = template3;
    ctx3->context.templateCtx.templateLen = len3;
    ctx3->context.templateCtx.isProgmem = true;
    ctx3->context.templateCtx.position = 0;
    
    TEST_ASSERT_EQUAL_MESSAGE(3, ctx.renderingDepth, "Stack depth should be 3 after third push");
    
    // Test pop context
    ctx.popContext();
    TEST_ASSERT_EQUAL_MESSAGE(2, ctx.renderingDepth, "Stack depth should be 2 after pop");
    TEST_ASSERT_EQUAL_MESSAGE(template2, ctx.getCurrentContext()->context.templateCtx.templateData, 
        "Current template should be previous template");
    
    ctx.popContext();
    TEST_ASSERT_EQUAL_MESSAGE(1, ctx.renderingDepth, "Stack depth should be 1 after second pop");
    TEST_ASSERT_EQUAL_MESSAGE(template1, ctx.getCurrentContext()->context.templateCtx.templateData, 
        "Current template should be first template");
    
    ctx.popContext();
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.renderingDepth, "Stack depth should be 0 after third pop");
    
    // Test stack overflow
    // Push up to MAX_RENDERING_DEPTH
    for (int i = 0; i < TemplateContext::MAX_RENDERING_DEPTH; i++) {
        ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE%");
        RenderingContext* c = ctx.getCurrentContext();
        c->context.templateCtx.templateData = template1;
        c->context.templateCtx.templateLen = len1;
        c->context.templateCtx.isProgmem = true;
        c->context.templateCtx.position = 0;
    }
    TEST_ASSERT_EQUAL_MESSAGE(TemplateContext::MAX_RENDERING_DEPTH, ctx.renderingDepth, 
        "Stack depth should be at MAX_RENDERING_DEPTH");
    
    // Try to push one more (should error)
    ctx.pushContext(RenderingContextType::TEMPLATE, "%OVERFLOW%");
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::ERROR, ctx.state, 
        "State should be ERROR after stack overflow");
    
    // Test stack underflow
    ctx.reset();
    ctx.popContext();
    TEST_ASSERT_EQUAL_MESSAGE(TemplateRenderState::ERROR, ctx.state, 
        "State should be ERROR after stack underflow");
    
    // Test getStackTrace
    ctx.reset();
    ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE1%");
    ctx.getCurrentContext()->context.templateCtx.templateData = template1;
    ctx.getCurrentContext()->context.templateCtx.templateLen = len1;
    ctx.getCurrentContext()->context.templateCtx.isProgmem = true;
    ctx.pushContext(RenderingContextType::TEMPLATE, "%TEMPLATE2%");
    ctx.getCurrentContext()->context.templateCtx.templateData = template2;
    ctx.getCurrentContext()->context.templateCtx.templateLen = len2;
    ctx.getCurrentContext()->context.templateCtx.isProgmem = true;
    String trace = ctx.getStackTrace();
    TEST_ASSERT_TRUE_MESSAGE(trace.length() > 0, "Stack trace should not be empty");
    TEST_ASSERT_TRUE_MESSAGE(trace.indexOf("TEMPLATE1") >= 0, 
        "Stack trace should contain template1");
    TEST_ASSERT_TRUE_MESSAGE(trace.indexOf("TEMPLATE2") >= 0, 
        "Stack trace should contain template2");
    
    Serial.println("[TEST]   TemplateContext stack tests completed successfully");
}

// Test TemplateContext buffer management
void test_template_context_buffer() {
    Serial.println("[TEST]   Testing TemplateContext buffer management...");
    
    TemplateContext ctx;
    const char* templateData = plain_text_template;
    size_t templateLen = strlen_P(templateData);
    
    // Push a template context first
    ctx.pushContext(RenderingContextType::TEMPLATE, "TEST");
    RenderingContext* currentCtx = ctx.getCurrentContext();
    currentCtx->context.templateCtx.templateData = templateData;
    currentCtx->context.templateCtx.templateLen = templateLen;
    currentCtx->context.templateCtx.isProgmem = true;
    currentCtx->context.templateCtx.position = 0;
    
    // Test refillBuffer
    bool refilled = ctx.refillBuffer();
    TEST_ASSERT_TRUE_MESSAGE(refilled, "Should refill buffer successfully");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, ctx.bufferLen, "Buffer length should be greater than 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.bufferPos, "Buffer position should be 0 after refill");
    
    // Test getNextChar
    char c1 = ctx.getNextChar();
    TEST_ASSERT_EQUAL_MESSAGE('H', c1, "Should get first character");
    TEST_ASSERT_EQUAL_MESSAGE(1, ctx.bufferPos, "Buffer position should be 1");
    
    // Test getNextChar multiple times
    char c2 = ctx.getNextChar();
    char c3 = ctx.getNextChar();
    TEST_ASSERT_EQUAL_MESSAGE('e', c2, "Should get second character");
    TEST_ASSERT_EQUAL_MESSAGE('l', c3, "Should get third character");
    
    // Test getAvailableBytes
    size_t available = ctx.getAvailableBytes();
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, available, "Should have available bytes");
    
    // Test hasMoreData
    TEST_ASSERT_TRUE_MESSAGE(ctx.hasMoreData(), "Should have more data initially");
    
    // Test getNextChar until end
    ctx.reset();
    ctx.pushContext(RenderingContextType::TEMPLATE, "TEST");
    currentCtx = ctx.getCurrentContext();
    currentCtx->context.templateCtx.templateData = templateData;
    currentCtx->context.templateCtx.templateLen = templateLen;
    currentCtx->context.templateCtx.isProgmem = true;
    currentCtx->context.templateCtx.position = 0;
    
    int charCount = 0;
    while (ctx.hasMoreData()) {
        char c = ctx.getNextChar();
        if (c == '\0') {
            break;
        }
        charCount++;
    }
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, charCount, "Should read all characters");
    
    // Test refillBuffer when at end
    ctx.reset();
    ctx.pushContext(RenderingContextType::TEMPLATE, "TEST");
    currentCtx = ctx.getCurrentContext();
    currentCtx->context.templateCtx.templateData = templateData;
    currentCtx->context.templateCtx.templateLen = templateLen;
    currentCtx->context.templateCtx.isProgmem = true;
    currentCtx->context.templateCtx.position = templateLen;
    
    bool refilled2 = ctx.refillBuffer();
    TEST_ASSERT_FALSE_MESSAGE(refilled2, "Should not refill when at end");
    
    // Test getNextChar when at end
    ctx.reset();
    ctx.pushContext(RenderingContextType::TEMPLATE, "TEST");
    currentCtx = ctx.getCurrentContext();
    currentCtx->context.templateCtx.templateData = templateData;
    currentCtx->context.templateCtx.templateLen = templateLen;
    currentCtx->context.templateCtx.position = templateLen;
    
    char c4 = ctx.getNextChar();
    TEST_ASSERT_EQUAL_MESSAGE('\0', c4, "Should return null character at end");
    
    // Test resetPlaceholder
    ctx.placeholderPos = 10;
    strcpy(ctx.placeholderName, "%TEST%");
    ctx.resetPlaceholder();
    
    TEST_ASSERT_EQUAL_MESSAGE(0, ctx.placeholderPos, "Placeholder position should be reset");
    TEST_ASSERT_EQUAL_MESSAGE(0, strlen(ctx.placeholderName), 
        "Placeholder name should be cleared");
    
    Serial.println("[TEST]   TemplateContext buffer tests completed successfully");
}

// Test TemplateContext state management
void test_template_context_state() {
    Serial.println("[TEST]   Testing TemplateContext state management...");
    
    TemplateContext ctx;
    
    // Test all state strings
    ctx.state = TemplateRenderState::TEXT;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TEXT", ctx.getStateString().c_str(), 
        "Should return TEXT for TEXT state");
    
    ctx.state = TemplateRenderState::BUILDING_PLACEHOLDER;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("BUILDING_PLACEHOLDER", ctx.getStateString().c_str(), 
        "Should return BUILDING_PLACEHOLDER for BUILDING_PLACEHOLDER state");
    
    ctx.state = TemplateRenderState::RENDERING_CONTEXT;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("RENDERING_CONTEXT", ctx.getStateString().c_str(), 
        "Should return RENDERING_CONTEXT for RENDERING_CONTEXT state");
    
    ctx.state = TemplateRenderState::COMPLETE;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("COMPLETE", ctx.getStateString().c_str(), 
        "Should return COMPLETE for COMPLETE state");
    
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_EQUAL_STRING_MESSAGE("ERROR", ctx.getStateString().c_str(), 
        "Should return ERROR for ERROR state");
    
    // Test isComplete for all states
    ctx.state = TemplateRenderState::TEXT;
    TEST_ASSERT_FALSE_MESSAGE(ctx.isComplete(), "TEXT should not be complete");
    
    ctx.state = TemplateRenderState::BUILDING_PLACEHOLDER;
    TEST_ASSERT_FALSE_MESSAGE(ctx.isComplete(), "BUILDING_PLACEHOLDER should not be complete");
    
    ctx.state = TemplateRenderState::RENDERING_CONTEXT;
    TEST_ASSERT_FALSE_MESSAGE(ctx.isComplete(), "RENDERING_CONTEXT should not be complete");
    
    ctx.state = TemplateRenderState::COMPLETE;
    TEST_ASSERT_TRUE_MESSAGE(ctx.isComplete(), "COMPLETE should be complete");
    
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_TRUE_MESSAGE(ctx.isComplete(), "ERROR should be complete");
    
    // Test hasError for all states
    ctx.state = TemplateRenderState::TEXT;
    TEST_ASSERT_FALSE_MESSAGE(ctx.hasError(), "TEXT should not have error");
    
    ctx.state = TemplateRenderState::ERROR;
    TEST_ASSERT_TRUE_MESSAGE(ctx.hasError(), "ERROR should have error");
    
    Serial.println("[TEST]   TemplateContext state tests completed successfully");
}

