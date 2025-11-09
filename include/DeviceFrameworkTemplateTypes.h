#ifndef DEVICEFRAMEWORK_TEMPLATE_TYPES_H
#define DEVICEFRAMEWORK_TEMPLATE_TYPES_H

#include <Arduino.h>

// Fallback defaults when DeviceFrameworkConfig is not available (standalone usage)
// Always use internal macro names (DFTE_*) to avoid conflicts with DeviceFrameworkConfig extern declarations
#ifndef DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT
  #define DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT 24
#endif

// Use DeviceFramework config defaults at compile-time if available, otherwise use internal defaults
// Array sizes must be compile-time constants
#ifdef DEVICEFRAMEWORK_CONFIG_H
  // DeviceFramework is present - use config defaults for array sizing
  // DeviceFrameworkConfig.h must be included before this file to access CONFIG_*_default macros
  #ifdef CONFIG_templatePlaceholderNameSize_default
    #define DFTE_PLACEHOLDER_NAME_SIZE CONFIG_templatePlaceholderNameSize_default
  #else
    #define DFTE_PLACEHOLDER_NAME_SIZE DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT
  #endif
#else
  // Standalone usage - use internal defaults
  #define DFTE_PLACEHOLDER_NAME_SIZE DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT
#endif

/**
 * Placeholder types for template substitution
 */
enum class PlaceholderType {
    PROGMEM_DATA,      // Large PROGMEM data (CSS, JS, base64 images)
    PROGMEM_TEMPLATE,  // Nested template in PROGMEM
    RAM_DATA,           // Dynamic RAM data via getter functions
    DYNAMIC_TEMPLATE,
    CONDITIONAL,
    ITERATOR
};

/**
 * Function pointer types for placeholder data access
 */
typedef const char* (*PlaceholderDataGetter)();
typedef size_t (*PlaceholderLengthGetter)(const void* data);
typedef const char* (*DynamicTemplateGetter)(void* userData);
typedef size_t (*DynamicTemplateLengthGetter)(const char* data, void* userData);

enum class ConditionalBranchResult {
    SKIP,
    TRUE_BRANCH,
    FALSE_BRANCH
};

typedef ConditionalBranchResult (*ConditionalEvaluator)(void* userData);

enum class IteratorStepResult {
    ITEM_READY,
    COMPLETE,
    ERROR
};

struct IteratorItemView;

typedef void* (*IteratorOpenHandler)(void* userData);
typedef IteratorStepResult (*IteratorNextHandler)(void* handle, struct IteratorItemView& view);
typedef void (*IteratorCloseHandler)(void* handle);

struct DynamicTemplateDescriptor {
    DynamicTemplateGetter getter;
    DynamicTemplateLengthGetter getLength;
    void* userData;
};

struct ConditionalDescriptor {
    ConditionalEvaluator evaluate;
    const char* truePlaceholder;
    const char* falsePlaceholder;
    void* userData;
};

/**
 * Placeholder definition entry
 * Represents a single registered placeholder in the registry
 */
struct PlaceholderEntry {
    // Allocated to configured size - matches CONFIG_templatePlaceholderNameSize when DeviceFramework is present
    char name[DFTE_PLACEHOLDER_NAME_SIZE];
    PlaceholderType type;
    const void* data;
    PlaceholderLengthGetter getLength;
    
    PlaceholderEntry() 
        : type(PlaceholderType::RAM_DATA), 
          data(nullptr), 
          getLength(nullptr) {
        name[0] = '\0';
    }
};

struct IteratorItemView {
    const char* templateData;
    size_t templateLength;
    bool templateIsProgmem;
    const PlaceholderEntry* placeholders;
    size_t placeholderCount;
};

struct IteratorDescriptor {
    IteratorOpenHandler open;
    IteratorNextHandler next;
    IteratorCloseHandler close;
    void* userData;
};

/**
 * Rendering context types - what kind of thing are we currently rendering?
 */
enum class RenderingContextType {
    TEMPLATE,              // Rendering a template (contains placeholders)
    PLACEHOLDER_DATA,      // Rendering a data placeholder (PROGMEM_DATA, RAM_DATA)
    PLACEHOLDER_TEMPLATE,   // Rendering a template placeholder (resolved to template)
    PLACEHOLDER_DYNAMIC_TEMPLATE,
    PLACEHOLDER_CONDITIONAL,
    PLACEHOLDER_ITERATOR
};

/**
 * Unified rendering context entry
 * Represents any active rendering context (template or placeholder)
 * Note: Uses PlaceholderEntry which is defined above
 */
struct RenderingContext {
    RenderingContextType type;
    const char* name;  // Placeholder name or template identifier
    
    // Type-specific data (using union to save memory)
    union {
        // TEMPLATE context
        struct {
            const char* templateData;  // Pointer to PROGMEM or RAM template data (not a copy)
            size_t templateLen;        // Length of template in bytes
            bool isProgmem;            // Storage location flag
            size_t position;           // Current position in template
            size_t bufferPos;         // Current position in buffer
            size_t bufferLen;          // Current buffer length
            size_t bufferOffset;       // Offset in template where buffer starts
            const PlaceholderEntry* iteratorPlaceholders;
            size_t iteratorPlaceholderCount;
        } templateCtx;
        
        // PLACEHOLDER_DATA context
        struct {
            const PlaceholderEntry* entry;
            size_t offset;  // Current offset in data
        } data;
        
        // PLACEHOLDER_TEMPLATE context
        struct {
            const PlaceholderEntry* entry;
            // Template state is stored in nested TEMPLATE context
        } templatePlaceholder;

        struct {
            const PlaceholderEntry* entry;
            size_t offset;
            const char* templateData;
            size_t templateLength;
        } dynamicTemplate;

        struct {
            const PlaceholderEntry* entry;
            const ConditionalDescriptor* descriptor;
            bool branchResolved;
            const char* delegateName;
            const PlaceholderEntry* delegateEntry;
        } conditional;

        struct {
            const PlaceholderEntry* entry;
            const IteratorDescriptor* descriptor;
            void* handle;
            bool initialized;
            bool handleOpen;
        } iterator;
    } context;
    
    RenderingContext() 
        : type(RenderingContextType::TEMPLATE), name(nullptr) {
        memset(&context, 0, sizeof(context));
    }
};

/**
 * Template rendering state
 */
enum class TemplateRenderState {
    TEXT,                    // Reading normal text in current context
    BUILDING_PLACEHOLDER,    // Building placeholder name between % and %
    RENDERING_CONTEXT,       // Rendering current context (unified for all context types)
    COMPLETE,                // All rendering complete
    ERROR                    // Error state
};

#endif // DEVICEFRAMEWORK_TEMPLATE_TYPES_H

