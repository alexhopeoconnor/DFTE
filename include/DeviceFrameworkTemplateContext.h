#ifndef DEVICEFRAMEWORK_TEMPLATE_CONTEXT_H
#define DEVICEFRAMEWORK_TEMPLATE_CONTEXT_H

#include <Arduino.h>
#include "DeviceFrameworkTemplateTypes.h"

// Fallback defaults when DeviceFrameworkConfig is not available (standalone usage)
// Always use internal macro names (DFTE_*) to avoid conflicts with DeviceFrameworkConfig extern declarations
// When DeviceFrameworkConfig is included, extern variables are declared with CONFIG_* names
// We use DFTE_* internal names for compile-time constants (constexpr) and array sizes
#ifndef DFTE_MAX_STACK_DEPTH_DEFAULT
  #define DFTE_MAX_STACK_DEPTH_DEFAULT 16
#endif

#ifndef DFTE_BUFFER_SIZE_DEFAULT
  #define DFTE_BUFFER_SIZE_DEFAULT 512
#endif

#ifndef DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT
  #define DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT 24
#endif

// Use DeviceFramework config defaults at compile-time if available, otherwise use internal defaults
// Array sizes must be compile-time constants
#ifdef DEVICEFRAMEWORK_CONFIG_H
  // DeviceFramework is present - use config defaults for array sizing
  // DeviceFrameworkConfig.h must be included before this file to access CONFIG_*_default macros
  #ifdef CONFIG_templateStackDepth_default
    #define DFTE_MAX_STACK_DEPTH CONFIG_templateStackDepth_default
  #else
    #define DFTE_MAX_STACK_DEPTH DFTE_MAX_STACK_DEPTH_DEFAULT
  #endif
  #ifdef CONFIG_templateBufferSize_default
    #define DFTE_BUFFER_SIZE CONFIG_templateBufferSize_default
  #else
    #define DFTE_BUFFER_SIZE DFTE_BUFFER_SIZE_DEFAULT
  #endif
  // DFTE_PLACEHOLDER_NAME_SIZE is already defined in DeviceFrameworkTemplateTypes.h
#else
  // Standalone usage - use internal defaults
  #define DFTE_MAX_STACK_DEPTH DFTE_MAX_STACK_DEPTH_DEFAULT
  #define DFTE_BUFFER_SIZE DFTE_BUFFER_SIZE_DEFAULT
  // DFTE_PLACEHOLDER_NAME_SIZE is already defined in DeviceFrameworkTemplateTypes.h
#endif

// Forward declaration
class DeviceFrameworkPlaceholderRegistry;

/**
 * Template rendering context
 * Maintains state for chunked streaming rendering with nested template support
 */
class DeviceFrameworkTemplateContext {
public:
    using State = TemplateRenderState;
    
    // Context state
    State state;
    
    // Unified rendering stack
    static constexpr int MAX_RENDERING_DEPTH = DFTE_MAX_STACK_DEPTH;
    RenderingContext renderingStack[MAX_RENDERING_DEPTH];
    int renderingDepth;
    
    // Current placeholder being built (only valid during BUILDING_PLACEHOLDER state)
    // Allocated to configured size - matches CONFIG_templatePlaceholderNameSize when DeviceFramework is present
    char placeholderName[DFTE_PLACEHOLDER_NAME_SIZE];
    size_t placeholderPos;
    
    // Centralized buffer management
    // Allocated to configured size - matches CONFIG_templateBufferSize when DeviceFramework is present
    static const size_t BUFFER_SIZE = DFTE_BUFFER_SIZE;
    uint8_t readBuffer[BUFFER_SIZE];
    size_t bufferPos;
    size_t bufferLen;
    size_t bufferOffset;
    
    // Placeholder registry (injected, not owned)
    DeviceFrameworkPlaceholderRegistry* registry;
    
    // Statistics
    size_t totalBytesProcessed;
    unsigned long startTime;
    
    DeviceFrameworkTemplateContext();
    void reset();
    
    // Unified stack management methods
    bool pushContext(RenderingContextType type, const char* name);
    void popContext();
    RenderingContext* getCurrentContext();
    RenderingContext* getContext(int depth);
    bool isRenderingTemplate() const;
    bool isRenderingPlaceholder() const;
    RenderingContextType getCurrentContextType() const;
    
    bool isComplete() const;
    bool hasError() const;
    String getStateString() const;
    String getStackTrace() const;
    
    // Set the registry to use for placeholder lookups
    void setRegistry(DeviceFrameworkPlaceholderRegistry* reg) { registry = reg; }
    
    // Unified buffer management
    bool refillBuffer();
    char getNextChar();
    size_t getAvailableBytes() const;
    bool hasMoreData() const;
    void resetPlaceholder();
};

#endif // DEVICEFRAMEWORK_TEMPLATE_CONTEXT_H

