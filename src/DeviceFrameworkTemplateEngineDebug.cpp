#include "DeviceFrameworkTemplateEngineDebug.h"

// Global logger pointer - starts as nullptr (disabled by default)
DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineLogger = nullptr;

// ============================================================================
// LOGGING CONFIGURATION IMPLEMENTATIONS
// ============================================================================

bool deviceFrameworkTemplateEngineEnableLogging(DeviceFrameworkTemplateEngineLogger* logger) {
    if (logger == nullptr) {
        return false;
    }
    deviceFrameworkTemplateEngineLogger = logger;
    return true;
}

void deviceFrameworkTemplateEngineDisableLogging() {
    deviceFrameworkTemplateEngineLogger = nullptr;
}

bool deviceFrameworkTemplateEngineIsLoggingEnabled() {
    return deviceFrameworkTemplateEngineLogger != nullptr;
}

DeviceFrameworkTemplateEngineLogger* deviceFrameworkTemplateEngineGetLogger() {
    return deviceFrameworkTemplateEngineLogger;
}