#ifndef DEVICEFRAMEWORK_TEMPLATE_ENGINE_DEBUG_H
#define DEVICEFRAMEWORK_TEMPLATE_ENGINE_DEBUG_H

#include <Arduino.h>

// ============================================================================
// LOGGING INTERFACE - Framework Agnostic
// ============================================================================

/**
 * Logging interface for the DeviceFramework Template Engine
 * 
 * This interface allows any logging system to be plugged into the template engine.
 * The template engine itself remains completely framework-agnostic.
 * 
 * Usage:
 * 1. Implement this interface with your preferred logging system
 * 2. Call deviceFrameworkTemplateEngineEnableLogging() with your logger instance
 * 3. All DFTE_LOG_* macros will then use your logger
 * 
 * Example:
 *   class MyLogger : public DeviceFrameworkTemplateEngineLogger {
 *       void error(const String& msg) override { // your logging logic }
 *       void warn(const String& msg) override { // your logging logic }
 *       void info(const String& msg) override { // your logging logic }
 *       void debug(const String& msg) override { // your logging logic }
 *   };
 *   
 *   MyLogger myLogger;
 *   deviceFrameworkTemplateEngineEnableLogging(&myLogger);
 */
class DeviceFrameworkTemplateEngineLogger {
public:
    virtual void error(const String& msg) = 0;
    virtual void warn(const String& msg) = 0;
    virtual void info(const String& msg) = 0;
    virtual void debug(const String& msg) = 0;
    virtual ~DeviceFrameworkTemplateEngineLogger() {}
};

// Global logger pointer - set by user or defaults to nullptr (disabled)
extern DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineLogger;

/** Opaque owner tag for the active logger (set on enable). Used to disable without clobbering another host. */
extern const void* deviceFrameworkTemplateEngineLoggerOwner;

// ============================================================================
// LOGGING MACROS - NO-OPS BY DEFAULT
// ============================================================================

/**
 * DeviceFramework Template Engine Logging System
 * 
 * LOGGING IS DISABLED BY DEFAULT - All macros are no-ops unless explicitly enabled
 * 
 * To enable logging:
 * 1. Implement the DeviceFrameworkTemplateEngineLogger interface with your preferred logging system
 * 2. Call deviceFrameworkTemplateEngineEnableLogging() with your logger instance
 * 
 * This design ensures:
 * - Zero logging overhead by default
 * - Complete framework decoupling
 * - Flexible logging system integration
 * - No build flags or conditional compilation needed
 */

// All logging macros are no-ops by default - zero overhead unless explicitly enabled
#define DFTE_LOG_ERROR(msg) do { \
    if (deviceFrameworkTemplateEngineLogger) { \
        deviceFrameworkTemplateEngineLogger->error(msg); \
    } \
} while(0)

#define DFTE_LOG_WARN(msg) do { \
    if (deviceFrameworkTemplateEngineLogger) { \
        deviceFrameworkTemplateEngineLogger->warn(msg); \
    } \
} while(0)

#define DFTE_LOG_INFO(msg) do { \
    if (deviceFrameworkTemplateEngineLogger) { \
        deviceFrameworkTemplateEngineLogger->info(msg); \
    } \
} while(0)

#define DFTE_LOG_DEBUG(msg) do { \
    if (deviceFrameworkTemplateEngineLogger) { \
        deviceFrameworkTemplateEngineLogger->debug(msg); \
    } \
} while(0)

/** Optional verbose trace (compile with -DDFTE_ENABLE_TRACE). Maps to debug() on the active logger. */
#if defined(DFTE_ENABLE_TRACE)
#define DFTE_LOG_TRACE(msg) DFTE_LOG_DEBUG(String(F("[TRACE] ")) + String(msg))
#else
#define DFTE_LOG_TRACE(msg) do { (void)0; } while(0)
#endif

// ============================================================================
// LOGGING CONFIGURATION FUNCTIONS
// ============================================================================

/**
 * Enable logging with a custom logger implementation
 * 
 * @param logger Pointer to custom logger implementation
 * @return true if logging was enabled, false if logger is null
 */
bool deviceFrameworkTemplateEngineEnableLogging(DeviceFrameworkTemplateEngineLogger* logger);

/**
 * Enable logging and record an owner tag (e.g. `this` of WiFiManagerServer).
 * If ownerTag is nullptr, the logger pointer is used as the tag.
 */
bool deviceFrameworkTemplateEngineEnableLogging(DeviceFrameworkTemplateEngineLogger* logger, const void* ownerTag);

/**
 * Disable all logging
 * Sets logger to nullptr, making all DFTE_LOG_* macros no-ops
 */
void deviceFrameworkTemplateEngineDisableLogging();

/** Disable only if the current logger was registered with this owner tag. Returns true if disabled. */
bool deviceFrameworkTemplateEngineDisableLoggingForOwner(const void* ownerTag);

/**
 * Check if logging is currently enabled
 * 
 * @return true if logging is enabled, false if disabled
 */
bool deviceFrameworkTemplateEngineIsLoggingEnabled();

/**
 * Get current logger
 * 
 * @return Pointer to current logger or nullptr if disabled
 */
DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineGetLogger();

const void* deviceFrameworkTemplateEngineGetLoggerOwner();

#endif // DEVICEFRAMEWORK_TEMPLATE_ENGINE_DEBUG_H