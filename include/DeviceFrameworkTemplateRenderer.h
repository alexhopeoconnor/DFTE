#ifndef DEVICEFRAMEWORK_TEMPLATE_RENDERER_H
#define DEVICEFRAMEWORK_TEMPLATE_RENDERER_H

#include <Arduino.h>
#include "DeviceFrameworkTemplateContext.h"
#include "DeviceFrameworkPlaceholderRegistry.h"

// Fallback defaults when DeviceFrameworkConfig is not available (standalone usage)
#ifndef DFTE_MAX_ITERATIONS_DEFAULT
  #define DFTE_MAX_ITERATIONS_DEFAULT 50
#endif

// Use DeviceFramework config defaults at compile-time if available, otherwise use internal defaults
#ifdef DEVICEFRAMEWORK_CONFIG_H
  // DeviceFramework is present - use config defaults
  // DeviceFrameworkConfig.h must be included before this file to access CONFIG_*_default macros
  #ifdef CONFIG_templateMaxIterations_default
    #define DFTE_MAX_ITERATIONS CONFIG_templateMaxIterations_default
  #else
    #define DFTE_MAX_ITERATIONS DFTE_MAX_ITERATIONS_DEFAULT
  #endif
#else
  // Standalone usage - use internal defaults
  #define DFTE_MAX_ITERATIONS DFTE_MAX_ITERATIONS_DEFAULT
#endif

/**
 * DeviceFramework Template Renderer
 * Core rendering engine for streaming template output with chunked processing
 */
class DeviceFrameworkTemplateRenderer {
public:
    struct RenderOutcome {
        size_t bytesWritten;
        TemplateRenderState nextState;
        bool repeat;
        bool finished;
        bool errored;
        uint8_t popCount;
        struct {
            bool active;
            RenderingContextType type;
            const PlaceholderEntry* entry;
        } pushContext;
    };

    /**
     * Render next chunk of template
     * Call repeatedly until returns 0
     *
     * @param ctx Rendering context (maintains state between calls)
     * @param buffer Output buffer to write to
     * @param maxLen Maximum bytes to write to buffer
     * @return Number of bytes written (0 = complete or error)
     */
    static size_t renderNextChunk(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen);

    /**
     * Initialize rendering context with template stored in PROGMEM
     * Call once before rendering begins
     *
     * @param ctx Context to initialize
     * @param templateData Pointer to PROGMEM template string
     */
    static void initializeContext(DeviceFrameworkTemplateContext& ctx, const char* templateData);

    /**
     * Initialize rendering context with template stored in RAM or PROGMEM
     *
     * @param ctx Context to initialize
     * @param templateData Pointer to template string
     * @param templateInProgmem True if template is stored in PROGMEM, false if in RAM
     */
    static void initializeContext(DeviceFrameworkTemplateContext& ctx, const char* templateData, bool templateInProgmem);

    /**
     * Check if rendering is complete
     */
    static bool isComplete(const DeviceFrameworkTemplateContext& ctx);

    /**
     * Check if rendering encountered an error
     */
    static bool hasError(const DeviceFrameworkTemplateContext& ctx);

    /**
     * Helper constructors for RenderOutcome
     */
    static RenderOutcome makeWritten(size_t bytes, TemplateRenderState state, bool repeat = false);
    static RenderOutcome makeState(TemplateRenderState nextState, bool repeat = true);
    static RenderOutcome makeComplete();
    static RenderOutcome makeError();

private:
    static RenderOutcome renderChunk(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen);
    static bool applyStackCommands(DeviceFrameworkTemplateContext& ctx, const RenderOutcome& outcome);

    // Core processing methods
    static RenderOutcome consumeTemplateText(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen);
    static RenderOutcome buildPlaceholderToken(DeviceFrameworkTemplateContext& ctx);
    static RenderOutcome resolvePlaceholder(DeviceFrameworkTemplateContext& ctx);
    static RenderOutcome emitActiveContext(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen);
    static RenderOutcome streamPlaceholderData(DeviceFrameworkTemplateContext& ctx, RenderingContext* context, uint8_t* buffer, size_t maxLen);
    static RenderOutcome handleTemplateCompletion(DeviceFrameworkTemplateContext& ctx);

    // Constants
    static constexpr size_t MAX_ITERATIONS = DFTE_MAX_ITERATIONS;
};

#endif // DEVICEFRAMEWORK_TEMPLATE_RENDERER_H

