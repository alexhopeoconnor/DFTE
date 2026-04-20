#include "DeviceFrameworkTemplateEngineDebug.h"

// Global logger pointer - starts as nullptr (disabled by default)
DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineLogger = nullptr;
const void* deviceFrameworkTemplateEngineLoggerOwner = nullptr;

// ============================================================================
// LOGGING CONFIGURATION IMPLEMENTATIONS
// ============================================================================

bool deviceFrameworkTemplateEngineEnableLogging(DeviceFrameworkTemplateEngineLogger* logger) {
    return deviceFrameworkTemplateEngineEnableLogging(logger, nullptr);
}

bool deviceFrameworkTemplateEngineEnableLogging(DeviceFrameworkTemplateEngineLogger* logger, const void* ownerTag) {
    if (logger == nullptr) {
        return false;
    }
    deviceFrameworkTemplateEngineLogger = logger;
    deviceFrameworkTemplateEngineLoggerOwner = ownerTag ? ownerTag : static_cast<const void*>(logger);
    return true;
}

void deviceFrameworkTemplateEngineDisableLogging() {
    deviceFrameworkTemplateEngineLogger = nullptr;
    deviceFrameworkTemplateEngineLoggerOwner = nullptr;
}

bool deviceFrameworkTemplateEngineDisableLoggingForOwner(const void* ownerTag) {
    if (deviceFrameworkTemplateEngineLogger == nullptr || ownerTag == nullptr) {
        return false;
    }
    if (deviceFrameworkTemplateEngineLoggerOwner != ownerTag) {
        return false;
    }
    deviceFrameworkTemplateEngineDisableLogging();
    return true;
}

bool deviceFrameworkTemplateEngineIsLoggingEnabled() {
    return deviceFrameworkTemplateEngineLogger != nullptr;
}

DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineGetLogger() {
    return deviceFrameworkTemplateEngineLogger;
}

const void* deviceFrameworkTemplateEngineGetLoggerOwner() {
    return deviceFrameworkTemplateEngineLoggerOwner;
}