#ifndef DEVICEFRAMEWORK_PLACEHOLDER_REGISTRY_H
#define DEVICEFRAMEWORK_PLACEHOLDER_REGISTRY_H

#include <Arduino.h>
#include <stdio.h>
#include "DeviceFrameworkTemplateTypes.h"

// Fallback defaults when DeviceFrameworkConfig is not available (standalone usage)
// Always use internal macro names (DFTE_*) to avoid conflicts with DeviceFrameworkConfig extern declarations
// When DeviceFrameworkConfig is included, extern variables are declared with CONFIG_* names
// We use DFTE_* internal names for compile-time constants (constexpr) and default parameter values
// NEVER define CONFIG_* macros here to avoid conflicts with extern declarations in DeviceFrameworkConfig
#ifndef DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT
  #define DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT 24
#endif

#ifndef DFTE_MAX_PLACEHOLDERS_DEFAULT
  #define DFTE_MAX_PLACEHOLDERS_DEFAULT 16
#endif

#ifndef DFTE_PROGMEM_CHUNK_SIZE_DEFAULT
  #define DFTE_PROGMEM_CHUNK_SIZE_DEFAULT 512
#endif

#ifndef DFTE_RAM_CHUNK_SIZE_DEFAULT
  #define DFTE_RAM_CHUNK_SIZE_DEFAULT 128
#endif

// Use DeviceFramework config defaults at compile-time if available, otherwise use internal defaults
#ifdef DEVICEFRAMEWORK_CONFIG_H
  // DeviceFramework is present - use config defaults
  // DeviceFrameworkConfig.h must be included before this file to access CONFIG_*_default macros
  #ifdef CONFIG_templateProgmemChunkSize_default
    #define DFTE_PROGMEM_CHUNK_SIZE CONFIG_templateProgmemChunkSize_default
  #else
    #define DFTE_PROGMEM_CHUNK_SIZE DFTE_PROGMEM_CHUNK_SIZE_DEFAULT
  #endif
  #ifdef CONFIG_templateRamChunkSize_default
    #define DFTE_RAM_CHUNK_SIZE CONFIG_templateRamChunkSize_default
  #else
    #define DFTE_RAM_CHUNK_SIZE DFTE_RAM_CHUNK_SIZE_DEFAULT
  #endif
#else
  // Standalone usage - use internal defaults
  #define DFTE_PROGMEM_CHUNK_SIZE DFTE_PROGMEM_CHUNK_SIZE_DEFAULT
  #define DFTE_RAM_CHUNK_SIZE DFTE_RAM_CHUNK_SIZE_DEFAULT
#endif

/**
 * DeviceFramework Placeholder Registry
 * Manages runtime registration of template placeholders
 * Registry starts empty - users must register their placeholders
 */
class DeviceFrameworkPlaceholderRegistry {
public:
    /**
     * Constructor with optional max placeholders parameter
     * @param maxPlaceholders Maximum number of placeholders (defaults to DFTE_MAX_PLACEHOLDERS_DEFAULT)
     *                        Allows creating multiple registries with different sizes
     *                        When using with DeviceFramework, pass getConfigMaxTemplatePlaceholders() for runtime config
     */
    explicit DeviceFrameworkPlaceholderRegistry(uint16_t maxPlaceholders = DFTE_MAX_PLACEHOLDERS_DEFAULT);
    
    ~DeviceFrameworkPlaceholderRegistry();
    
    // Prevent copying
    DeviceFrameworkPlaceholderRegistry(const DeviceFrameworkPlaceholderRegistry&) = delete;
    DeviceFrameworkPlaceholderRegistry& operator=(const DeviceFrameworkPlaceholderRegistry&) = delete;
    
    /**
     * Register a PROGMEM data placeholder
     * @param name Placeholder name (e.g., "%CSS%")
     * @param progmemData Pointer to PROGMEM data
     * @return true if registered successfully, false if registry full
     */
    bool registerProgmemData(const char* name, const char* progmemData);
    
    /**
     * Register a PROGMEM template placeholder (nested template)
     * @param name Placeholder name (e.g., "%HEADER%")
     * @param progmemTemplate Pointer to PROGMEM template
     * @return true if registered successfully
     */
    bool registerProgmemTemplate(const char* name, const char* progmemTemplate);
    
    /**
     * Register a RAM data placeholder with getter function
     * @param name Placeholder name (e.g., "%PAGE_TITLE%")
     * @param getter Function that returns current value
     * @return true if registered successfully
     */
    bool registerRamData(const char* name, PlaceholderDataGetter getter);
    bool registerDynamicTemplate(const char* name, const DynamicTemplateDescriptor* descriptor);
    bool registerConditional(const char* name, const ConditionalDescriptor* descriptor);
    bool registerIterator(const char* name, const IteratorDescriptor* descriptor);
    
    /**
     * Clear all registered placeholders
     */
    void clear();
    
    /**
     * Get number of registered placeholders
     */
    int getCount() const { return count; }
    
    /**
     * Get maximum number of placeholders
     */
    uint16_t getMaxPlaceholders() const { return maxPlaceholders; }
    
    /**
     * Find placeholder entry by name
     * @return PlaceholderEntry pointer or nullptr if not found
     */
    const PlaceholderEntry* getPlaceholder(const char* name) const;
    
    /**
     * Render placeholder content at given offset
     */
    size_t renderPlaceholder(const PlaceholderEntry* entry, size_t offset, 
                            uint8_t* buffer, size_t maxLen) const;
    
    // Static helper functions for length calculation
    static size_t getProgmemLength(const void* data);
    static size_t getRamLength(const void* data);
    static size_t getDynamicTemplateLength(const DynamicTemplateDescriptor* descriptor, const char* templateData);
    
private:
    static constexpr uint16_t MAX_PLACEHOLDER_NAME_SIZE = DFTE_PLACEHOLDER_NAME_SIZE_DEFAULT;
    
    PlaceholderEntry* placeholders;  // Dynamically allocated array
    uint16_t maxPlaceholders;        // Configurable size
    int count;
    
    bool validatePlaceholderName(const char* name) const;
    static size_t copyProgmemData(const char* source, size_t offset, 
                                 uint8_t* dest, size_t maxLen);
    static size_t copyRamData(PlaceholderDataGetter getter, size_t offset, 
                             uint8_t* dest, size_t maxLen);
};

#endif // DEVICEFRAMEWORK_PLACEHOLDER_REGISTRY_H

