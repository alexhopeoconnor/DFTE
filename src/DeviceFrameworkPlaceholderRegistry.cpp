#include "DeviceFrameworkPlaceholderRegistry.h"
#include "DeviceFrameworkTemplateEngineDebug.h"
#include <pgmspace.h>
#include <new>

DeviceFrameworkPlaceholderRegistry::DeviceFrameworkPlaceholderRegistry(uint16_t maxPlaceholders) 
    : placeholders(nullptr), maxPlaceholders(maxPlaceholders), count(0) {
    if (maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry size cannot be zero");
        this->maxPlaceholders = 0;
        return;
    }

    placeholders = new (std::nothrow) PlaceholderEntry[maxPlaceholders];
    if (placeholders == nullptr) {
        DFTE_LOG_ERROR("Failed to allocate placeholder registry");
        this->maxPlaceholders = 0;
        return;
    }

    for (uint16_t i = 0; i < maxPlaceholders; ++i) {
        placeholders[i] = PlaceholderEntry();
    }
}

DeviceFrameworkPlaceholderRegistry::~DeviceFrameworkPlaceholderRegistry() {
    if (placeholders) {
        delete[] placeholders;
        placeholders = nullptr;
    }
}

bool DeviceFrameworkPlaceholderRegistry::registerProgmemData(const char* name, const char* progmemData) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }
    
    if (!validatePlaceholderName(name)) {
        return false;
    }
    
    // Check for duplicates
    if (getPlaceholder(name) != nullptr) {
        DFTE_LOG_WARN("Placeholder already registered: " + String(name));
        // Continue anyway - last registration wins
    }
    
    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::PROGMEM_DATA;
    entry.data = progmemData;
    entry.getLength = getProgmemLength;
    entry.cachedLength = getProgmemLength(progmemData);
    entry.hasCachedLength = true;
    
    count++;
    return true;
}

bool DeviceFrameworkPlaceholderRegistry::registerProgmemTemplate(const char* name, const char* progmemTemplate) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }
    
    if (!validatePlaceholderName(name)) {
        return false;
    }
    
    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::PROGMEM_TEMPLATE;
    entry.data = progmemTemplate;
    entry.getLength = getProgmemLength;
    entry.cachedLength = getProgmemLength(progmemTemplate);
    entry.hasCachedLength = true;
    
    count++;
    return true;
}

bool DeviceFrameworkPlaceholderRegistry::registerRamData(const char* name, PlaceholderDataGetter getter) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }
    
    if (!validatePlaceholderName(name)) {
        return false;
    }

    if (getter == nullptr) {
        DFTE_LOG_ERROR("Cannot register RAM_DATA placeholder with null getter: " + String(name));
        return false;
    }
    
    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::RAM_DATA;
    entry.data = (const void*)getter;
    entry.getLength = getRamLength;
    entry.cachedLength = 0;
    entry.hasCachedLength = false;
    
    count++;
    return true;
}

bool DeviceFrameworkPlaceholderRegistry::registerDynamicTemplate(const char* name, const DynamicTemplateDescriptor* descriptor) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }

    if (!validatePlaceholderName(name)) {
        return false;
    }

    if (descriptor == nullptr || descriptor->getter == nullptr) {
        DFTE_LOG_ERROR("Invalid dynamic template descriptor for placeholder: " + String(name));
        return false;
    }

    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::DYNAMIC_TEMPLATE;
    entry.data = descriptor;
    entry.getLength = nullptr;
    entry.cachedLength = 0;
    entry.hasCachedLength = false;

    count++;
    return true;
}

bool DeviceFrameworkPlaceholderRegistry::registerConditional(const char* name, const ConditionalDescriptor* descriptor) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }

    if (!validatePlaceholderName(name)) {
        return false;
    }

    if (descriptor == nullptr || descriptor->evaluate == nullptr) {
        DFTE_LOG_ERROR("Invalid conditional descriptor for placeholder: " + String(name));
        return false;
    }

    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::CONDITIONAL;
    entry.data = descriptor;
    entry.getLength = nullptr;
    entry.cachedLength = 0;
    entry.hasCachedLength = false;

    count++;
    return true;
}

bool DeviceFrameworkPlaceholderRegistry::registerIterator(const char* name, const IteratorDescriptor* descriptor) {
    if (placeholders == nullptr || maxPlaceholders == 0) {
        DFTE_LOG_ERROR("Placeholder registry not initialized");
        return false;
    }

    if (count >= maxPlaceholders) {
        DFTE_LOG_ERROR("Placeholder registry full, cannot register: " + String(name));
        return false;
    }

    if (!validatePlaceholderName(name)) {
        return false;
    }

    if (descriptor == nullptr || descriptor->next == nullptr) {
        DFTE_LOG_ERROR("Invalid iterator descriptor for placeholder: " + String(name));
        return false;
    }

    PlaceholderEntry& entry = placeholders[count];
    strncpy(entry.name, name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    entry.type = PlaceholderType::ITERATOR;
    entry.data = descriptor;
    entry.getLength = nullptr;
    entry.cachedLength = 0;
    entry.hasCachedLength = false;

    count++;
    return true;
}

void DeviceFrameworkPlaceholderRegistry::clear() {
    count = 0;
    if (placeholders == nullptr || maxPlaceholders == 0) {
        return;
    }

    for (uint16_t i = 0; i < maxPlaceholders; ++i) {
        placeholders[i] = PlaceholderEntry();
    }
}

bool DeviceFrameworkPlaceholderRegistry::validatePlaceholderName(const char* name) const {
    if (!name) {
        DFTE_LOG_ERROR("Placeholder name is null");
        return false;
    }
    
    size_t nameLen = strlen(name);
    if (nameLen >= MAX_PLACEHOLDER_NAME_SIZE) {
        DFTE_LOG_ERROR("Placeholder name too long: " + String(name) + " (max: " + String(MAX_PLACEHOLDER_NAME_SIZE) + ")");
        return false;
    }
    
    if (nameLen >= sizeof(PlaceholderEntry::name)) {
        // This shouldn't happen if MAX_PLACEHOLDER_NAME_SIZE is <= sizeof(PlaceholderEntry::name)
        DFTE_LOG_ERROR("Placeholder name exceeds entry buffer size");
        return false;
    }
    
    return true;
}

const PlaceholderEntry* DeviceFrameworkPlaceholderRegistry::getPlaceholder(const char* name) const {
    if (name == nullptr || placeholders == nullptr || count <= 0) return nullptr;
    
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(placeholders[i].name, name) == 0) {
            return &placeholders[i];
        }
    }
    return nullptr;
}

size_t DeviceFrameworkPlaceholderRegistry::renderPlaceholder(const PlaceholderEntry* entry, size_t offset, 
                                             uint8_t* buffer, size_t maxLen) const {
    if (entry == nullptr || buffer == nullptr || maxLen == 0) {
        return 0;
    }
    
    switch (entry->type) {
        case PlaceholderType::PROGMEM_DATA:
        case PlaceholderType::PROGMEM_TEMPLATE: {
            const char* data = static_cast<const char*>(entry->data);
            if (data == nullptr) {
                return 0;
            }

            size_t dataLen = entry->hasCachedLength ? entry->cachedLength : getProgmemLength(entry->data);
            if (offset >= dataLen) {
                return 0;
            }

            size_t remaining = dataLen - offset;
            constexpr size_t MAX_CHUNK = DFTE_PROGMEM_CHUNK_SIZE;
            size_t chunkSize = min(min(maxLen, remaining), MAX_CHUNK);
            memcpy_P(buffer, data + offset, chunkSize);
            return chunkSize;
        }
            
        case PlaceholderType::RAM_DATA:
            return copyRamData((PlaceholderDataGetter)entry->data, offset, buffer, maxLen);

        case PlaceholderType::DYNAMIC_TEMPLATE:
        case PlaceholderType::CONDITIONAL:
        case PlaceholderType::ITERATOR:
            // Dynamic template, conditional, and iterator content are handled directly by the renderer
            return 0;
            
        default:
            return 0;
    }
}

size_t DeviceFrameworkPlaceholderRegistry::getProgmemLength(const void* data) {
    if (data == nullptr) return 0;
    return strlen_P((const char*)data);
}

size_t DeviceFrameworkPlaceholderRegistry::getRamLength(const void* data) {
    if (data == nullptr) return 0;
    
    PlaceholderDataGetter getter = (PlaceholderDataGetter)data;
    const char* str = getter();
    if (str == nullptr) {
        return 0;
    }
    return strlen(str);
}

size_t DeviceFrameworkPlaceholderRegistry::getDynamicTemplateLength(const DynamicTemplateDescriptor* descriptor, const char* templateData) {
    if (descriptor == nullptr || templateData == nullptr) {
        return 0;
    }

    if (descriptor->getLength) {
        return descriptor->getLength(templateData, descriptor->userData);
    }

    return strlen(templateData);
}

size_t DeviceFrameworkPlaceholderRegistry::copyProgmemData(const char* source, size_t offset, uint8_t* dest, size_t maxLen) {
    if (source == nullptr || maxLen == 0) return 0;
    
    size_t dataLen = strlen_P(source);
    if (offset >= dataLen) return 0;
    
    size_t remaining = dataLen - offset;
    // Use configurable chunk size - matches CONFIG_templateProgmemChunkSize when DeviceFramework is present
    constexpr size_t MAX_CHUNK = DFTE_PROGMEM_CHUNK_SIZE;
    size_t chunkSize = min(min(maxLen, remaining), MAX_CHUNK);
    
    memcpy_P(dest, source + offset, chunkSize);
    return chunkSize;
}

size_t DeviceFrameworkPlaceholderRegistry::copyRamData(PlaceholderDataGetter getter, size_t offset, uint8_t* dest, size_t maxLen) {
    if (getter == nullptr || maxLen == 0) return 0;
    
    const char* data = getter();
    if (data == nullptr) {
        return 0;
    }
    size_t dataLen = strlen(data);
    
    if (offset >= dataLen) return 0;
    
    size_t remaining = dataLen - offset;
    // Use configurable chunk size - matches CONFIG_templateRamChunkSize when DeviceFramework is present
    constexpr size_t MAX_CHUNK = DFTE_RAM_CHUNK_SIZE;
    size_t chunkSize = min(min(maxLen, remaining), MAX_CHUNK);
    
    memcpy(dest, data + offset, chunkSize);
    return chunkSize;
}

