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

// These are default capacities for a general-purpose standalone context. A
// caller can select smaller per-context capacities when its template topology
// is known, without changing the public object layout across translation units.
#ifndef DFTE_MAX_STACK_DEPTH
  #define DFTE_MAX_STACK_DEPTH DFTE_MAX_STACK_DEPTH_DEFAULT
#endif

#ifndef DFTE_BUFFER_SIZE
  #define DFTE_BUFFER_SIZE DFTE_BUFFER_SIZE_DEFAULT
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
    
    // Standalone defaults retained for callers that use the no-argument
    // constructor and for tests that exercise the maximum supported depth.
    static constexpr int MAX_RENDERING_DEPTH = DFTE_MAX_STACK_DEPTH;
    static const size_t BUFFER_SIZE = DFTE_BUFFER_SIZE;

    // Per-context storage avoids layout-affecting build flags. This also lets
    // constrained applications request only the stack and read buffer they
    // actually need for a specific response.
    RenderingContext* renderingStack;
    size_t maxRenderingDepth;
    int renderingDepth;
    
    // Current placeholder being built (only valid during BUILDING_PLACEHOLDER state)
    // Fixed ABI-stable placeholder token storage.
    char placeholderName[DFTE_PLACEHOLDER_NAME_CAPACITY];
    size_t placeholderPos;
    
    // Centralized buffer management
    uint8_t* readBuffer;
    size_t readBufferSize;
    size_t bufferPos;
    size_t bufferLen;
    size_t bufferOffset;
    
    // Placeholder registry (injected, not owned)
    DeviceFrameworkPlaceholderRegistry* registry;
    
    // Statistics
    size_t totalBytesProcessed;
    unsigned long startTime;
    
    explicit DeviceFrameworkTemplateContext(
        size_t maxDepth = MAX_RENDERING_DEPTH,
        size_t bufferSize = BUFFER_SIZE);
    ~DeviceFrameworkTemplateContext();
    DeviceFrameworkTemplateContext(const DeviceFrameworkTemplateContext&) = delete;
    DeviceFrameworkTemplateContext& operator=(const DeviceFrameworkTemplateContext&) = delete;
    void reset();

    bool isReady() const { return renderingStack != nullptr && readBuffer != nullptr; }
    size_t getMaxRenderingDepth() const { return maxRenderingDepth; }
    size_t getBufferSize() const { return readBufferSize; }
    
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

