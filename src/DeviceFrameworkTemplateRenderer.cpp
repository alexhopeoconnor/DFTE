#include "DeviceFrameworkTemplateRenderer.h"
#include "DeviceFrameworkTemplateEngineDebug.h"
#include <pgmspace.h>
#include <cstring>

namespace {

static bool pushPlaceholderEntry(DeviceFrameworkTemplateContext& ctx, const PlaceholderEntry* entry, const char* nameOverride = nullptr) {
    if (entry == nullptr) {
        DFTE_LOG_ERROR("Attempted to push null placeholder entry");
        return false;
    }

    const char* name = nameOverride ? nameOverride : entry->name;

    switch (entry->type) {
        case PlaceholderType::PROGMEM_DATA:
        case PlaceholderType::RAM_DATA: {
            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_DATA, name)) {
                return false;
            }
            RenderingContext* dataCtx = ctx.getCurrentContext();
            dataCtx->context.data.entry = entry;
            dataCtx->context.data.offset = 0;
            return true;
        }
        case PlaceholderType::PROGMEM_TEMPLATE: {
            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_TEMPLATE, name)) {
                return false;
            }
            RenderingContext* placeholderCtx = ctx.getCurrentContext();
            placeholderCtx->context.templatePlaceholder.entry = entry;

            if (!ctx.pushContext(RenderingContextType::TEMPLATE, name)) {
                ctx.popContext();
                return false;
            }

            RenderingContext* templateCtx = ctx.getCurrentContext();
            templateCtx->context.templateCtx.templateData = static_cast<const char*>(entry->data);
            templateCtx->context.templateCtx.templateLen = entry->getLength(entry->data);
            templateCtx->context.templateCtx.isProgmem = true;
            templateCtx->context.templateCtx.position = 0;
            templateCtx->context.templateCtx.iteratorPlaceholders = nullptr;
            templateCtx->context.templateCtx.iteratorPlaceholderCount = 0;
            return true;
        }
        case PlaceholderType::DYNAMIC_TEMPLATE: {
            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_DYNAMIC_TEMPLATE, name)) {
                return false;
            }

            RenderingContext* dynamicCtx = ctx.getCurrentContext();
            dynamicCtx->context.dynamicTemplate.entry = entry;
            dynamicCtx->context.dynamicTemplate.offset = 0;
            dynamicCtx->context.dynamicTemplate.templateData = nullptr;
            dynamicCtx->context.dynamicTemplate.templateLength = 0;

            const DynamicTemplateDescriptor* descriptor = static_cast<const DynamicTemplateDescriptor*>(entry->data);
            if (descriptor == nullptr || descriptor->getter == nullptr) {
                DFTE_LOG_ERROR("Dynamic template placeholder missing descriptor: " + String(name));
                ctx.popContext();
                return false;
            }

            const char* templateData = descriptor->getter(descriptor->userData);
            if (templateData == nullptr) {
                DFTE_LOG_WARN("Dynamic template getter returned null for placeholder: " + String(name));
                templateData = "";
            }

            size_t templateLen = DeviceFrameworkPlaceholderRegistry::getDynamicTemplateLength(descriptor, templateData);
            dynamicCtx->context.dynamicTemplate.templateData = templateData;
            dynamicCtx->context.dynamicTemplate.templateLength = templateLen;

            if (!ctx.pushContext(RenderingContextType::TEMPLATE, name)) {
                ctx.popContext();
                return false;
            }

            RenderingContext* templateCtx = ctx.getCurrentContext();
            templateCtx->context.templateCtx.templateData = templateData;
            templateCtx->context.templateCtx.templateLen = templateLen;
            templateCtx->context.templateCtx.isProgmem = false;
            templateCtx->context.templateCtx.position = 0;
            templateCtx->context.templateCtx.iteratorPlaceholders = nullptr;
            templateCtx->context.templateCtx.iteratorPlaceholderCount = 0;
            return true;
        }
        case PlaceholderType::CONDITIONAL: {
            const ConditionalDescriptor* descriptor = static_cast<const ConditionalDescriptor*>(entry->data);
            if (descriptor == nullptr || descriptor->evaluate == nullptr) {
                DFTE_LOG_ERROR("Conditional placeholder missing descriptor: " + String(name));
                return false;
            }

            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_CONDITIONAL, name)) {
                return false;
            }

            RenderingContext* conditionalCtx = ctx.getCurrentContext();
            conditionalCtx->context.conditional.entry = entry;
            conditionalCtx->context.conditional.descriptor = descriptor;
            conditionalCtx->context.conditional.branchResolved = false;
            conditionalCtx->context.conditional.delegateName = nullptr;
            conditionalCtx->context.conditional.delegateEntry = nullptr;

            if (!ctx.registry) {
                conditionalCtx->context.conditional.branchResolved = true;
                return true;
            }

            ConditionalBranchResult branch = descriptor->evaluate(descriptor->userData);
            const char* delegateName = nullptr;
            switch (branch) {
                case ConditionalBranchResult::TRUE_BRANCH:
                    delegateName = descriptor->truePlaceholder;
                    break;
                case ConditionalBranchResult::FALSE_BRANCH:
                    delegateName = descriptor->falsePlaceholder;
                    break;
                case ConditionalBranchResult::SKIP:
                default:
                    delegateName = nullptr;
                    break;
            }

            conditionalCtx->context.conditional.branchResolved = true;
            conditionalCtx->context.conditional.delegateName = delegateName;

            if (delegateName == nullptr) {
                return true;
            }

            const PlaceholderEntry* delegateEntry = ctx.registry->getPlaceholder(delegateName);
            if (!delegateEntry) {
                DFTE_LOG_WARN("Conditional placeholder '" + String(name) + "' referenced unknown placeholder: " + String(delegateName));
                return true;
            }

            conditionalCtx->context.conditional.delegateEntry = delegateEntry;

            if (!pushPlaceholderEntry(ctx, delegateEntry, delegateName)) {
                ctx.popContext();
                return false;
            }

            return true;
        }
        case PlaceholderType::ITERATOR: {
            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_ITERATOR, name)) {
                return false;
            }

            RenderingContext* iteratorCtx = ctx.getCurrentContext();
            iteratorCtx->context.iterator.entry = entry;
            iteratorCtx->context.iterator.descriptor = static_cast<const IteratorDescriptor*>(entry->data);
            iteratorCtx->context.iterator.handle = nullptr;
            iteratorCtx->context.iterator.initialized = false;
            iteratorCtx->context.iterator.handleOpen = false;

            const IteratorDescriptor* descriptor = iteratorCtx->context.iterator.descriptor;
            if (!descriptor || !descriptor->next) {
                DFTE_LOG_ERROR("Iterator placeholder missing descriptor: " + String(name));
                ctx.popContext();
                return false;
            }

            return true;
        }
        default:
            DFTE_LOG_WARN("pushPlaceholderEntry does not support placeholder type for: " + String(name));
            return false;
    }
}

static DeviceFrameworkTemplateRenderer::RenderOutcome processIteratorContext(DeviceFrameworkTemplateContext& ctx, RenderingContext* iteratorCtx) {
    if (!iteratorCtx) {
        return DeviceFrameworkTemplateRenderer::makeError();
    }

    const IteratorDescriptor* descriptor = iteratorCtx->context.iterator.descriptor;
    if (!descriptor || !descriptor->next) {
        DFTE_LOG_ERROR("Iterator placeholder missing descriptor or next handler");
        return DeviceFrameworkTemplateRenderer::makeError();
    }

    auto& iteratorState = iteratorCtx->context.iterator;

    if (!iteratorState.initialized) {
        iteratorState.handle = descriptor->open ? descriptor->open(descriptor->userData) : descriptor->userData;
        iteratorState.initialized = true;
        iteratorState.handleOpen = descriptor->close != nullptr && iteratorState.handle != nullptr && descriptor->open != nullptr;
    }

    IteratorItemView view = {};
    IteratorStepResult step = descriptor->next(iteratorState.handle, view);

    switch (step) {
        case IteratorStepResult::ITEM_READY: {
            if (!view.templateData && view.templateLength == 0) {
                return DeviceFrameworkTemplateRenderer::makeState(TemplateRenderState::RENDERING_CONTEXT, true);
            }

            const char* templatePtr = view.templateData;
            if (!templatePtr) {
                return DeviceFrameworkTemplateRenderer::makeState(TemplateRenderState::RENDERING_CONTEXT, true);
            }

            size_t templateLen = view.templateLength;
            if (templateLen == 0) {
                templateLen = view.templateIsProgmem ? strlen_P(templatePtr) : strlen(templatePtr);
            }

            if (!ctx.pushContext(RenderingContextType::TEMPLATE, iteratorCtx->name)) {
                return DeviceFrameworkTemplateRenderer::makeError();
            }

            RenderingContext* templateCtx = ctx.getCurrentContext();
            templateCtx->context.templateCtx.templateData = templatePtr;
            templateCtx->context.templateCtx.templateLen = templateLen;
            templateCtx->context.templateCtx.isProgmem = view.templateIsProgmem;
            templateCtx->context.templateCtx.position = 0;
            templateCtx->context.templateCtx.iteratorPlaceholders = view.placeholders;
            templateCtx->context.templateCtx.iteratorPlaceholderCount = view.placeholderCount;

            return DeviceFrameworkTemplateRenderer::makeState(TemplateRenderState::TEXT, true);
        }
        case IteratorStepResult::COMPLETE: {
            if (iteratorState.handleOpen && descriptor->close) {
                descriptor->close(iteratorState.handle);
                iteratorState.handleOpen = false;
            }
            iteratorState.handle = nullptr;

            auto outcome = DeviceFrameworkTemplateRenderer::makeState(TemplateRenderState::RENDERING_CONTEXT, true);
            outcome.popCount = 1;

            RenderingContext* parent = (ctx.renderingDepth > 1) ? ctx.getContext(ctx.renderingDepth - 2) : nullptr;
            if (!parent) {
                outcome.nextState = TemplateRenderState::COMPLETE;
                outcome.repeat = false;
                outcome.finished = true;
            } else if (parent->type == RenderingContextType::TEMPLATE) {
                outcome.nextState = TemplateRenderState::TEXT;
            }

            return outcome;
        }
        case IteratorStepResult::ERROR:
        default:
            DFTE_LOG_ERROR("Iterator placeholder reported error");
            if (descriptor->close && (iteratorState.handleOpen || iteratorState.handle != nullptr)) {
                descriptor->close(iteratorState.handle);
            }
            iteratorState.handleOpen = false;
            iteratorState.handle = nullptr;
            return DeviceFrameworkTemplateRenderer::makeError();
    }
}

} // namespace

// Helper function to convert RenderingContextType to string
static String getContextTypeString(RenderingContextType type) {
    switch (type) {
        case RenderingContextType::TEMPLATE: return "TEMPLATE";
        case RenderingContextType::PLACEHOLDER_DATA: return "PLACEHOLDER_DATA";
        case RenderingContextType::PLACEHOLDER_TEMPLATE: return "PLACEHOLDER_TEMPLATE";
        default: return "UNKNOWN";
    }
}

// Helper function to log state transitions with stack state
static void logStateTransition(DeviceFrameworkTemplateContext& ctx, const String& fromState, const String& toState, const String& reason = "") {
    if (!deviceFrameworkTemplateEngineLogger) {
        return;
    }

    String msg = "State: " + fromState + " -> " + toState;
    if (reason.length() > 0) {
        msg += " (" + reason + ")";
    }
    msg += " | Stack depth: " + String(ctx.renderingDepth);
    if (ctx.renderingDepth > 0) {
        RenderingContext* currentCtx = ctx.getCurrentContext();
        msg += " | Current: " + String(currentCtx->name) + " (type=" + getContextTypeString(currentCtx->type) + ")";
    }
    deviceFrameworkTemplateEngineLogger->debug(msg);
}

static String getStateName(TemplateRenderState state) {
    switch (state) {
        case TemplateRenderState::TEXT: return "TEXT";
        case TemplateRenderState::BUILDING_PLACEHOLDER: return "BUILDING_PLACEHOLDER";
        case TemplateRenderState::RENDERING_CONTEXT: return "RENDERING_CONTEXT";
        case TemplateRenderState::COMPLETE: return "COMPLETE";
        case TemplateRenderState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::makeWritten(size_t bytes, TemplateRenderState state, bool repeat) {
    return {bytes, state, repeat, false, false, 0, {false, RenderingContextType::TEMPLATE, nullptr}};
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::makeState(TemplateRenderState nextState, bool repeat) {
    return {0, nextState, repeat, false, false, 0, {false, RenderingContextType::TEMPLATE, nullptr}};
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::makeComplete() {
    return {0, TemplateRenderState::COMPLETE, false, true, false, 0, {false, RenderingContextType::TEMPLATE, nullptr}};
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::makeError() {
    return {0, TemplateRenderState::ERROR, false, false, true, 0, {false, RenderingContextType::TEMPLATE, nullptr}};
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::renderChunk(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen) {
    RenderOutcome outcome = makeState(ctx.state, false);

    switch (ctx.state) {
        case TemplateRenderState::TEXT:
            outcome = consumeTemplateText(ctx, buffer, maxLen);
            break;
        case TemplateRenderState::BUILDING_PLACEHOLDER:
            outcome = buildPlaceholderToken(ctx);
            break;
        case TemplateRenderState::RENDERING_CONTEXT:
            outcome = emitActiveContext(ctx, buffer, maxLen);
            break;
        case TemplateRenderState::COMPLETE:
            return makeComplete();
        case TemplateRenderState::ERROR:
        default:
            return makeError();
    }

    if (!applyStackCommands(ctx, outcome)) {
        ctx.state = TemplateRenderState::ERROR;
        return makeError();
    }

    TemplateRenderState previousState = ctx.state;
    if (outcome.nextState != previousState) {
        String fromState = getStateName(previousState);
        ctx.state = outcome.nextState;
        String toState = getStateName(ctx.state);
        logStateTransition(ctx, fromState, toState);
    } else {
        ctx.state = outcome.nextState;
    }

    outcome.finished = (ctx.state == TemplateRenderState::COMPLETE);
    outcome.errored = (ctx.state == TemplateRenderState::ERROR);

    return outcome;
}

bool DeviceFrameworkTemplateRenderer::applyStackCommands(DeviceFrameworkTemplateContext& ctx, const RenderOutcome& outcome) {
    for (uint8_t i = 0; i < outcome.popCount; ++i) {
        if (ctx.renderingDepth == 0) {
            DFTE_LOG_ERROR("Rendering stack underflow during pop");
            return false;
        }
        ctx.popContext();
    }

    if (!outcome.pushContext.active) {
        return true;
    }

    const PlaceholderEntry* entry = outcome.pushContext.entry;
    if (entry == nullptr) {
        DFTE_LOG_ERROR("Missing placeholder entry for pushContext");
        return false;
    }

    const char* name = entry->name;

    switch (outcome.pushContext.type) {
        case RenderingContextType::PLACEHOLDER_DATA: {
            if (!pushPlaceholderEntry(ctx, entry, name)) {
                return false;
            }
            return true;
        }
        case RenderingContextType::PLACEHOLDER_TEMPLATE: {
            if (!pushPlaceholderEntry(ctx, entry, name)) {
                return false;
            }
            return true;
        }
        case RenderingContextType::PLACEHOLDER_DYNAMIC_TEMPLATE: {
            if (!pushPlaceholderEntry(ctx, entry, name)) {
                return false;
            }
            return true;
        }
        case RenderingContextType::PLACEHOLDER_CONDITIONAL: {
            const ConditionalDescriptor* descriptor = static_cast<const ConditionalDescriptor*>(entry->data);
            if (descriptor == nullptr || descriptor->evaluate == nullptr) {
                DFTE_LOG_ERROR("Conditional placeholder missing descriptor: " + String(name));
                return false;
            }

            ConditionalBranchResult branch = descriptor->evaluate(descriptor->userData);
            const char* delegateName = nullptr;
            switch (branch) {
                case ConditionalBranchResult::TRUE_BRANCH:
                    delegateName = descriptor->truePlaceholder;
                    break;
                case ConditionalBranchResult::FALSE_BRANCH:
                    delegateName = descriptor->falsePlaceholder;
                    break;
                case ConditionalBranchResult::SKIP:
                default:
                    delegateName = nullptr;
                    break;
            }

            if (delegateName == nullptr) {
                return true; // Nothing to render
            }

            const PlaceholderEntry* delegateEntry = ctx.registry ? ctx.registry->getPlaceholder(delegateName) : nullptr;
            if (!delegateEntry) {
                DFTE_LOG_WARN("Conditional placeholder '" + String(name) + "' referenced unknown placeholder: " + String(delegateName));
                return true;
            }

            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_CONDITIONAL, name)) {
                return false;
            }

            RenderingContext* conditionalCtx = ctx.getCurrentContext();
            conditionalCtx->context.conditional.entry = entry;
            conditionalCtx->context.conditional.descriptor = descriptor;
            conditionalCtx->context.conditional.branchResolved = true;
            conditionalCtx->context.conditional.delegateName = delegateName;
            conditionalCtx->context.conditional.delegateEntry = delegateEntry;

            if (!pushPlaceholderEntry(ctx, delegateEntry, delegateName)) {
                ctx.popContext();
                return false;
            }

            return true;
        }
        case RenderingContextType::PLACEHOLDER_ITERATOR: {
            if (!ctx.pushContext(RenderingContextType::PLACEHOLDER_ITERATOR, name)) {
                return false;
            }

            RenderingContext* iteratorCtx = ctx.getCurrentContext();
            iteratorCtx->context.iterator.entry = entry;
            iteratorCtx->context.iterator.descriptor = static_cast<const IteratorDescriptor*>(entry->data);
            iteratorCtx->context.iterator.handle = nullptr;
            iteratorCtx->context.iterator.initialized = false;
            iteratorCtx->context.iterator.handleOpen = false;

            if (!iteratorCtx->context.iterator.descriptor || !iteratorCtx->context.iterator.descriptor->next) {
                DFTE_LOG_ERROR("Iterator placeholder missing next handler: " + String(name));
                ctx.popContext();
                return false;
            }

            return true;
        }
        case RenderingContextType::TEMPLATE: {
            if (!ctx.pushContext(RenderingContextType::TEMPLATE, name)) {
                return false;
            }
            RenderingContext* templateCtx = ctx.getCurrentContext();
            templateCtx->context.templateCtx.templateData = static_cast<const char*>(entry->data);
            templateCtx->context.templateCtx.templateLen = entry->getLength(entry->data);
            templateCtx->context.templateCtx.isProgmem = true;
            templateCtx->context.templateCtx.position = 0;
            templateCtx->context.templateCtx.iteratorPlaceholders = nullptr;
            templateCtx->context.templateCtx.iteratorPlaceholderCount = 0;
            return true;
        }
        default:
            return true;
    }
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::handleTemplateCompletion(DeviceFrameworkTemplateContext& ctx) {
    RenderOutcome outcome = makeState(TemplateRenderState::TEXT, true);
    outcome.popCount = 1;

    if (ctx.renderingDepth <= 1) {
        outcome.nextState = TemplateRenderState::COMPLETE;
        outcome.repeat = false;
        outcome.finished = true;
        return outcome;
    }

    RenderingContext* parentCtx = ctx.getContext(ctx.renderingDepth - 2);
    if (parentCtx && (parentCtx->type == RenderingContextType::PLACEHOLDER_TEMPLATE ||
                      parentCtx->type == RenderingContextType::PLACEHOLDER_DYNAMIC_TEMPLATE ||
                      parentCtx->type == RenderingContextType::PLACEHOLDER_CONDITIONAL)) {
        outcome.popCount += 1;
        if (ctx.renderingDepth > 2) {
            parentCtx = ctx.getContext(ctx.renderingDepth - 3);
        } else {
            parentCtx = nullptr;
        }
    }

    if (!parentCtx) {
        outcome.nextState = TemplateRenderState::COMPLETE;
        outcome.repeat = false;
        outcome.finished = true;
        return outcome;
    }

    if (parentCtx->type == RenderingContextType::TEMPLATE) {
        outcome.nextState = TemplateRenderState::TEXT;
    } else {
        outcome.nextState = TemplateRenderState::RENDERING_CONTEXT;
    }

    outcome.repeat = true;
    return outcome;
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::consumeTemplateText(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen) {
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (!currentCtx || currentCtx->type != RenderingContextType::TEMPLATE) {
        DFTE_LOG_ERROR("consumeTemplateText called without TEMPLATE context");
        return makeError();
    }

    auto& templateCtx = currentCtx->context.templateCtx;

    // Empty template completes immediately
    if (templateCtx.templateLen == 0) {
        return handleTemplateCompletion(ctx);
    }

    size_t written = 0;
    while (written < maxLen) {
        if (templateCtx.position >= templateCtx.templateLen) {
            break;
        }

        char c = ctx.getNextChar();
        if (c == '\0') {
            break;
        }

        if (c == '%') {
            ctx.placeholderPos = 0;
            ctx.placeholderName[ctx.placeholderPos++] = '%';
            RenderOutcome outcome = makeState(TemplateRenderState::BUILDING_PLACEHOLDER, true);
            outcome.bytesWritten = written;
            return outcome;
        }

        buffer[written++] = c;
    }

    if (written > 0) {
        RenderOutcome outcome = makeWritten(written, TemplateRenderState::TEXT, written < maxLen);
        if (written < maxLen) {
            outcome.repeat = true;
        }
        return outcome;
    }

    // No bytes written, check if template finished
    if (templateCtx.position >= templateCtx.templateLen || !ctx.hasMoreData()) {
        return handleTemplateCompletion(ctx);
    }

    // Need more data
    return makeState(TemplateRenderState::TEXT, false);
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::buildPlaceholderToken(DeviceFrameworkTemplateContext& ctx) {
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (!currentCtx || currentCtx->type != RenderingContextType::TEMPLATE) {
        DFTE_LOG_ERROR("buildPlaceholderToken called without TEMPLATE context");
        return makeError();
    }

    bool madeProgress = false;
    while (ctx.placeholderPos < sizeof(ctx.placeholderName) - 1) {
        if (!ctx.hasMoreData()) {
            break;
        }

        char c = ctx.getNextChar();
        if (c == '\0') {
            break;
        }

        madeProgress = true;
        ctx.placeholderName[ctx.placeholderPos++] = c;

        if (c == '%') {
            ctx.placeholderName[ctx.placeholderPos] = '\0';
            return resolvePlaceholder(ctx);
        }
    }

    if (ctx.placeholderPos >= sizeof(ctx.placeholderName) - 1) {
        DFTE_LOG_WARN("Placeholder name too long: " + String(ctx.placeholderName));
        ctx.resetPlaceholder();
        return makeState(TemplateRenderState::TEXT, true);
    }

    if (!ctx.hasMoreData()) {
        DFTE_LOG_WARN("Incomplete placeholder at end of template");
        ctx.resetPlaceholder();
        return handleTemplateCompletion(ctx);
    }

    // We consumed data but did not finish; stay in BUILDING_PLACEHOLDER
    RenderOutcome outcome = makeState(TemplateRenderState::BUILDING_PLACEHOLDER, false);
    outcome.repeat = madeProgress;
    return outcome;
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::resolvePlaceholder(DeviceFrameworkTemplateContext& ctx) {
    const PlaceholderEntry* entry = ctx.registry ? ctx.registry->getPlaceholder(ctx.placeholderName) : nullptr;

    if (!entry) {
        RenderingContext* currentCtx = ctx.getCurrentContext();
        if (currentCtx && currentCtx->type == RenderingContextType::TEMPLATE) {
            const PlaceholderEntry* overrides = currentCtx->context.templateCtx.iteratorPlaceholders;
            size_t overrideCount = currentCtx->context.templateCtx.iteratorPlaceholderCount;
            for (size_t i = 0; i < overrideCount; ++i) {
                if (strcmp(overrides[i].name, ctx.placeholderName) == 0) {
                    entry = &overrides[i];
                    break;
                }
            }
        }
    }

    if (!entry) {
        DFTE_LOG_WARN("Unknown placeholder: " + String(ctx.placeholderName));
        ctx.resetPlaceholder();
        return makeState(TemplateRenderState::TEXT, true);
    }

    RenderOutcome outcome = makeState(TemplateRenderState::RENDERING_CONTEXT, true);
    outcome.pushContext.active = true;

    switch (entry->type) {
        case PlaceholderType::PROGMEM_DATA:
        case PlaceholderType::RAM_DATA:
            outcome.pushContext.type = RenderingContextType::PLACEHOLDER_DATA;
            outcome.pushContext.entry = entry;
            break;
        case PlaceholderType::PROGMEM_TEMPLATE:
            outcome.pushContext.type = RenderingContextType::PLACEHOLDER_TEMPLATE;
            outcome.pushContext.entry = entry;
            outcome.nextState = TemplateRenderState::TEXT;
            break;
        case PlaceholderType::DYNAMIC_TEMPLATE:
            outcome.pushContext.type = RenderingContextType::PLACEHOLDER_DYNAMIC_TEMPLATE;
            outcome.pushContext.entry = entry;
            outcome.nextState = TemplateRenderState::TEXT;
            break;
        case PlaceholderType::CONDITIONAL:
            outcome.pushContext.type = RenderingContextType::PLACEHOLDER_CONDITIONAL;
            outcome.pushContext.entry = entry;
            outcome.nextState = TemplateRenderState::RENDERING_CONTEXT;
            break;
        case PlaceholderType::ITERATOR:
            outcome.pushContext.type = RenderingContextType::PLACEHOLDER_ITERATOR;
            outcome.pushContext.entry = entry;
            outcome.nextState = TemplateRenderState::RENDERING_CONTEXT;
            break;
        default:
            DFTE_LOG_WARN("Unsupported placeholder type");
            ctx.resetPlaceholder();
            return makeState(TemplateRenderState::TEXT, true);
    }

    ctx.resetPlaceholder();
    return outcome;
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::emitActiveContext(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen) {
    RenderingContext* currentCtx = ctx.getCurrentContext();
    if (!currentCtx) {
        return makeComplete();
    }

    switch (currentCtx->type) {
        case RenderingContextType::TEMPLATE:
            return consumeTemplateText(ctx, buffer, maxLen);

        case RenderingContextType::PLACEHOLDER_DATA:
            return streamPlaceholderData(ctx, currentCtx, buffer, maxLen);

        case RenderingContextType::PLACEHOLDER_TEMPLATE:
            return makeState(TemplateRenderState::TEXT, true);

        case RenderingContextType::PLACEHOLDER_DYNAMIC_TEMPLATE:
            return makeState(TemplateRenderState::TEXT, true);

        case RenderingContextType::PLACEHOLDER_CONDITIONAL:
            return makeState(TemplateRenderState::RENDERING_CONTEXT, true);

        case RenderingContextType::PLACEHOLDER_ITERATOR:
            return processIteratorContext(ctx, currentCtx);

        default:
            DFTE_LOG_ERROR("emitActiveContext encountered unknown context type");
            return makeError();
    }
}

DeviceFrameworkTemplateRenderer::RenderOutcome DeviceFrameworkTemplateRenderer::streamPlaceholderData(DeviceFrameworkTemplateContext& ctx,
                                                                                                     RenderingContext* context,
                                                                                                     uint8_t* buffer,
                                                                                                     size_t maxLen) {
    auto& dataCtx = context->context.data;
    const PlaceholderEntry* entry = dataCtx.entry;

    if (!entry) {
        DFTE_LOG_ERROR("Placeholder data context missing entry");
        RenderOutcome outcome = makeState(TemplateRenderState::RENDERING_CONTEXT, true);
        outcome.popCount = 1;
        return outcome;
    }

    if (ctx.registry == nullptr) {
        DFTE_LOG_ERROR("Placeholder registry not set; cannot render placeholder: " + String(entry->name));
        RenderOutcome outcome = makeState(TemplateRenderState::RENDERING_CONTEXT, true);
        outcome.popCount = 1;

        RenderingContext* parent = (ctx.renderingDepth > 1) ? ctx.getContext(ctx.renderingDepth - 2) : nullptr;
        if (!parent) {
            outcome.nextState = TemplateRenderState::COMPLETE;
            outcome.repeat = false;
            outcome.finished = true;
        } else if (parent->type == RenderingContextType::TEMPLATE) {
            outcome.nextState = TemplateRenderState::TEXT;
        }

        return outcome;
    }

    size_t totalLength = entry->getLength(entry->data);
    if (dataCtx.offset >= totalLength) {
        RenderOutcome outcome = makeState(TemplateRenderState::RENDERING_CONTEXT, true);
        outcome.popCount = 1;

        RenderingContext* parent = (ctx.renderingDepth > 1) ? ctx.getContext(ctx.renderingDepth - 2) : nullptr;
        if (parent && parent->type == RenderingContextType::PLACEHOLDER_CONDITIONAL) {
            outcome.popCount += 1;
            parent = (ctx.renderingDepth > 2) ? ctx.getContext(ctx.renderingDepth - 3) : nullptr;
        }
        if (!parent) {
            outcome.nextState = TemplateRenderState::COMPLETE;
            outcome.repeat = false;
            outcome.finished = true;
        } else if (parent->type == RenderingContextType::TEMPLATE) {
            outcome.nextState = TemplateRenderState::TEXT;
        }
        return outcome;
    }

    size_t written = ctx.registry->renderPlaceholder(entry, dataCtx.offset, buffer, maxLen);
    if (written > 0) {
        dataCtx.offset += written;
        return makeWritten(written, TemplateRenderState::RENDERING_CONTEXT, written < maxLen);
    }

    // No bytes written; treat as completion and pop context
    RenderOutcome outcome = makeState(TemplateRenderState::RENDERING_CONTEXT, true);
    outcome.popCount = 1;

    RenderingContext* parent = (ctx.renderingDepth > 1) ? ctx.getContext(ctx.renderingDepth - 2) : nullptr;
    if (parent && parent->type == RenderingContextType::PLACEHOLDER_CONDITIONAL) {
        outcome.popCount += 1;
        parent = (ctx.renderingDepth > 2) ? ctx.getContext(ctx.renderingDepth - 3) : nullptr;
    }
    if (!parent) {
        outcome.nextState = TemplateRenderState::COMPLETE;
        outcome.repeat = false;
        outcome.finished = true;
    } else if (parent->type == RenderingContextType::TEMPLATE) {
        outcome.nextState = TemplateRenderState::TEXT;
    }

    return outcome;
}
size_t DeviceFrameworkTemplateRenderer::renderNextChunk(DeviceFrameworkTemplateContext& ctx, uint8_t* buffer, size_t maxLen) {
    if (ctx.isComplete() || ctx.hasError()) {
        return 0;
    }

    size_t written = 0;
    size_t iterations = 0;
    uint8_t* writePtr = buffer;
    size_t remaining = maxLen;

    while (remaining > 0 && !ctx.isComplete() && !ctx.hasError() && iterations < MAX_ITERATIONS) {
        RenderOutcome outcome = renderChunk(ctx, writePtr, remaining);

        written += outcome.bytesWritten;
        ctx.totalBytesProcessed += outcome.bytesWritten;
        writePtr += outcome.bytesWritten;
        remaining -= outcome.bytesWritten;
        iterations++;

        if (outcome.finished || outcome.errored) {
            break;
        }

        if (!outcome.repeat && outcome.bytesWritten == 0) {
            break;
        }
    }

    if (iterations >= MAX_ITERATIONS) {
        DFTE_LOG_WARN("Maximum iterations reached in renderNextChunk");
    }

    return written;
}

void DeviceFrameworkTemplateRenderer::initializeContext(DeviceFrameworkTemplateContext& ctx, const char* templateData) {
    initializeContext(ctx, templateData, true);
}

void DeviceFrameworkTemplateRenderer::initializeContext(DeviceFrameworkTemplateContext& ctx, const char* templateData, bool templateInProgmem) {
    ctx.reset();
    
    // Push initial template context
    if (!ctx.pushContext(RenderingContextType::TEMPLATE, "ROOT")) {
        ctx.state = TemplateRenderState::ERROR;
        return;
    }
    
    RenderingContext* rootCtx = ctx.getCurrentContext();
    rootCtx->context.templateCtx.templateData = templateData;
    rootCtx->context.templateCtx.templateLen = templateInProgmem ? strlen_P(templateData) : strlen(templateData);
    rootCtx->context.templateCtx.isProgmem = templateInProgmem;
    rootCtx->context.templateCtx.position = 0;
    
    ctx.state = TemplateRenderState::TEXT;
    logStateTransition(ctx, "INIT", "TEXT", "Initialized template context");
}

bool DeviceFrameworkTemplateRenderer::isComplete(const DeviceFrameworkTemplateContext& ctx) {
    return ctx.isComplete();
}

bool DeviceFrameworkTemplateRenderer::hasError(const DeviceFrameworkTemplateContext& ctx) {
    return ctx.hasError();
}

