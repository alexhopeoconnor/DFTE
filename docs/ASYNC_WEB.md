# Async web responses

Build a `PlaceholderRegistry` once during setup, but allocate a fresh `TemplateContext` for each request. A shared context would mix rendering state when clients overlap.

```cpp
#include <TemplateEngine.h>
#include <TemplateEngineAsyncWeb.h>

std::shared_ptr<PlaceholderRegistry> registry;

void sendTemplate(AsyncWebServerRequest* request, const char* root) {
    auto context = std::make_shared<TemplateContext>();
    context->setRegistry(registry.get());
    TemplateRenderer::initializeContext(*context, root);
    request->onDisconnect([context]() mutable { context.reset(); });

    AsyncWebServerResponse* response =
        TemplateEngineAsyncWeb::beginSafeTemplateResponse(
            request, "text/html; charset=utf-8", context, 128
        );
    request->send(response);
}
```

The response retains the context while it streams. Releasing the request-owned `shared_ptr` on disconnect prevents state from leaking into later requests.

## Bounded fixed slots

Use `beginBorrowedChunkedResponse()` when the application already owns a small, fixed response pool. It does not allocate a `shared_ptr` control block for the request. The release callback must be idempotent because it can run once when rendering finishes and again if the connection later disconnects.

```cpp
struct ResponseSlot {
    bool busy = false;
    TemplateContext context;
};

ResponseSlot slots[2];

void releaseSlot(ResponseSlot& slot) {
    slot.context.reset();
    slot.busy = false;
}

void sendBoundedTemplate(AsyncWebServerRequest* request, const char* root) {
    ResponseSlot* slot = nullptr;
    for (auto& candidate : slots) {
        if (!candidate.busy) {
            slot = &candidate;
            break;
        }
    }
    if (slot == nullptr) {
        request->send(503, "text/plain", "Busy");
        return;
    }

    slot->busy = true;
    slot->context.setRegistry(registry.get());
    TemplateRenderer::initializeContext(slot->context, root);
    request->send(TemplateEngineAsyncWeb::beginBorrowedChunkedResponse(
        request, "text/html; charset=utf-8", slot,
        [](ResponseSlot& state, uint8_t* out, size_t size, size_t) {
            return TemplateEngineAsyncWeb::renderTemplateChunkWithRetries(
                state.context, out, size, 128);
        },
        [](const ResponseSlot& state) {
            return TemplateEngineAsyncWeb::isTemplateTerminal(state.context);
        },
        releaseSlot));
}
```

Use this form only while the slot itself has static or otherwise guaranteed lifetime. Use the `shared_ptr` form above for a request-owned dynamic context.

Use [StreamingAsync](../examples/StreamingAsync/) for a complete SoftAP/captive-portal project.

Back to [documentation](README.md) · [project overview](../README.md).
