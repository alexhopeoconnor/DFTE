#include "DeviceFrameworkTemplateContext.h"
#include "DeviceFrameworkTemplateEngineDebug.h"

DeviceFrameworkTemplateContext::DeviceFrameworkTemplateContext() 
    : state(TemplateRenderState::TEXT), renderingDepth(0), placeholderPos(0),
      bufferPos(0), bufferLen(0), bufferOffset(0),
      registry(nullptr),
      totalBytesProcessed(0), startTime(0) {
    memset(placeholderName, 0, sizeof(placeholderName));
    memset(renderingStack, 0, sizeof(renderingStack));
}

void DeviceFrameworkTemplateContext::reset() {
    state = TemplateRenderState::TEXT;
    renderingDepth = 0;
    placeholderPos = 0;
    bufferPos = 0;
    bufferLen = 0;
    bufferOffset = 0;
    totalBytesProcessed = 0;
    startTime = millis();
    memset(placeholderName, 0, sizeof(placeholderName));
    memset(renderingStack, 0, sizeof(renderingStack));
}

// Unified stack management methods
bool DeviceFrameworkTemplateContext::pushContext(RenderingContextType type, const char* name) {
    if (renderingDepth >= MAX_RENDERING_DEPTH) {
        DFTE_LOG_ERROR("Rendering stack overflow! Depth=" + String(renderingDepth));
        state = TemplateRenderState::ERROR;
        return false;
    }
    
    // Save current buffer state if we're currently in a template context
    if (renderingDepth > 0) {
        RenderingContext* currentCtx = getCurrentContext();
        if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
            // Save buffer state to current template context
            currentCtx->context.templateCtx.bufferPos = bufferPos;
            currentCtx->context.templateCtx.bufferLen = bufferLen;
            currentCtx->context.templateCtx.bufferOffset = bufferOffset;
        }
    }
    
    RenderingContext& ctx = renderingStack[renderingDepth];
    ctx.type = type;
    ctx.name = name;
    
    // Initialize buffer state for new template context
    if (type == RenderingContextType::TEMPLATE) {
        ctx.context.templateCtx.bufferPos = 0;
        ctx.context.templateCtx.bufferLen = 0;
        ctx.context.templateCtx.bufferOffset = 0;
        ctx.context.templateCtx.iteratorPlaceholders = nullptr;
        ctx.context.templateCtx.iteratorPlaceholderCount = 0;
        bufferPos = 0;
        bufferLen = 0;
        bufferOffset = 0;
    }
    
    renderingDepth++;
    return true;
}

void DeviceFrameworkTemplateContext::popContext() {
    if (renderingDepth <= 0) {
        DFTE_LOG_ERROR("Rendering stack underflow!");
        state = TemplateRenderState::ERROR;
        return;
    }
    
    renderingDepth--;
    RenderingContext& ctx = renderingStack[renderingDepth];

    if (ctx.type == RenderingContextType::PLACEHOLDER_ITERATOR) {
        const IteratorDescriptor* descriptor = ctx.context.iterator.descriptor;
        if (descriptor && descriptor->close && ctx.context.iterator.handleOpen) {
            descriptor->close(ctx.context.iterator.handle);
        }
    }
    
    // Restore buffer state from parent template context if it exists
    if (renderingDepth > 0) {
        RenderingContext* parentCtx = getCurrentContext();
        if (parentCtx && parentCtx->type == RenderingContextType::TEMPLATE) {
            auto& parentTemplateCtx = parentCtx->context.templateCtx;
            // Invalidate shared buffer so parent refill starts fresh
            bufferPos = 0;
            bufferLen = 0;
            bufferOffset = parentTemplateCtx.position;

            // Clear cached buffer state stored on the parent template context
            parentTemplateCtx.bufferPos = 0;
            parentTemplateCtx.bufferLen = 0;
            parentTemplateCtx.bufferOffset = parentTemplateCtx.position;
        } else {
            // No parent template context, reset buffer state
            bufferPos = 0;
            bufferLen = 0;
            bufferOffset = 0;
        }
    } else {
        // No more contexts, reset buffer state
        bufferPos = 0;
        bufferLen = 0;
        bufferOffset = 0;
    }
    
    // Clear the popped context
    memset(&ctx, 0, sizeof(RenderingContext));
}

RenderingContext* DeviceFrameworkTemplateContext::getCurrentContext() {
    if (renderingDepth == 0) return nullptr;
    return &renderingStack[renderingDepth - 1];
}

RenderingContext* DeviceFrameworkTemplateContext::getContext(int depth) {
    if (depth < 0 || depth >= renderingDepth) return nullptr;
    return &renderingStack[depth];
}

bool DeviceFrameworkTemplateContext::isRenderingTemplate() const {
    if (renderingDepth == 0) return false;
    return renderingStack[renderingDepth - 1].type == RenderingContextType::TEMPLATE;
}

bool DeviceFrameworkTemplateContext::isRenderingPlaceholder() const {
    if (renderingDepth == 0) return false;
    RenderingContextType type = renderingStack[renderingDepth - 1].type;
    return type == RenderingContextType::PLACEHOLDER_DATA || 
           type == RenderingContextType::PLACEHOLDER_TEMPLATE;
}

RenderingContextType DeviceFrameworkTemplateContext::getCurrentContextType() const {
    if (renderingDepth == 0) return RenderingContextType::TEMPLATE; // Default
    return renderingStack[renderingDepth - 1].type;
}

bool DeviceFrameworkTemplateContext::isComplete() const {
    return state == TemplateRenderState::COMPLETE || state == TemplateRenderState::ERROR;
}

bool DeviceFrameworkTemplateContext::hasError() const {
    return state == TemplateRenderState::ERROR;
}

String DeviceFrameworkTemplateContext::getStateString() const {
    switch (state) {
        case TemplateRenderState::TEXT: return "TEXT";
        case TemplateRenderState::BUILDING_PLACEHOLDER: return "BUILDING_PLACEHOLDER";
        case TemplateRenderState::RENDERING_CONTEXT: return "RENDERING_CONTEXT";
        case TemplateRenderState::COMPLETE: return "COMPLETE";
        case TemplateRenderState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

String DeviceFrameworkTemplateContext::getStackTrace() const {
    String trace = "Stack trace (depth=" + String(renderingDepth) + "):\n";
    for (int i = 0; i < renderingDepth; i++) {
        const RenderingContext& ctx = renderingStack[i];
        String typeStr;
        switch (ctx.type) {
            case RenderingContextType::TEMPLATE: typeStr = "TEMPLATE"; break;
            case RenderingContextType::PLACEHOLDER_DATA: typeStr = "PLACEHOLDER_DATA"; break;
            case RenderingContextType::PLACEHOLDER_TEMPLATE: typeStr = "PLACEHOLDER_TEMPLATE"; break;
            default: typeStr = "UNKNOWN"; break;
        }
        trace += "  [" + String(i) + "] " + String(ctx.name) + " (type=" + typeStr + ")";
        if (ctx.type == RenderingContextType::TEMPLATE) {
            trace += " at pos " + String(ctx.context.templateCtx.position);
        } else if (ctx.type == RenderingContextType::PLACEHOLDER_DATA) {
            trace += " at offset " + String(ctx.context.data.offset);
        }
        trace += "\n";
    }
    return trace;
}

bool DeviceFrameworkTemplateContext::refillBuffer() {
    RenderingContext* currentCtx = getCurrentContext();
    if (!currentCtx || currentCtx->type != RenderingContextType::TEMPLATE) {
        return false;
    }
    
    auto& templateCtx = currentCtx->context.templateCtx;
    if (templateCtx.position >= templateCtx.templateLen) {
        return false;
    }
    
    bufferLen = min(BUFFER_SIZE, templateCtx.templateLen - templateCtx.position);
    
    if (bufferLen > 0) {
        if (templateCtx.isProgmem) {
            memcpy_P(readBuffer, templateCtx.templateData + templateCtx.position, bufferLen);
        } else {
            memcpy(readBuffer, templateCtx.templateData + templateCtx.position, bufferLen);
        }
        bufferPos = 0;
        bufferOffset = templateCtx.position;
        // Save buffer state to template context
        templateCtx.bufferPos = bufferPos;
        templateCtx.bufferLen = bufferLen;
        templateCtx.bufferOffset = bufferOffset;
        // Don't advance templateCtx.position here - it will be updated as we read characters
        return true;
    }
    
    return false;
}

char DeviceFrameworkTemplateContext::getNextChar() {
    RenderingContext* currentCtx = getCurrentContext();
    if (!currentCtx || currentCtx->type != RenderingContextType::TEMPLATE) {
        return '\0';
    }
    
    // Restore buffer state from template context if needed
    auto& templateCtx = currentCtx->context.templateCtx;
    if (bufferPos == 0 && bufferLen == 0 && templateCtx.bufferLen > 0) {
        // Buffer state might be stale, restore from context
        bufferPos = templateCtx.bufferPos;
        bufferLen = templateCtx.bufferLen;
        bufferOffset = templateCtx.bufferOffset;
    }
    
    if (bufferPos >= bufferLen) {
        if (!refillBuffer()) {
            return '\0';  // End of template
        }
    }
    char c = readBuffer[bufferPos++];
    // Update template position to reflect current position
    templateCtx.position = bufferOffset + bufferPos;
    // Save buffer state to template context
    templateCtx.bufferPos = bufferPos;
    templateCtx.bufferLen = bufferLen;
    templateCtx.bufferOffset = bufferOffset;
    return c;
}

size_t DeviceFrameworkTemplateContext::getAvailableBytes() const {
    return bufferLen - bufferPos;
}

bool DeviceFrameworkTemplateContext::hasMoreData() const {
    RenderingContext* currentCtx = const_cast<DeviceFrameworkTemplateContext*>(this)->getCurrentContext();
    if (!currentCtx || currentCtx->type != RenderingContextType::TEMPLATE) {
        return false;
    }
    const auto& templateCtx = currentCtx->context.templateCtx;
    return templateCtx.position < templateCtx.templateLen || bufferPos < bufferLen;
}

void DeviceFrameworkTemplateContext::resetPlaceholder() {
    placeholderPos = 0;
    memset(placeholderName, 0, sizeof(placeholderName));
}

