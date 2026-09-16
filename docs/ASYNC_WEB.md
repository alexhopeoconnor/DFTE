# Async web responses

Build a `PlaceholderRegistry` once during setup, but allocate a fresh `TemplateContext` for each request. A shared context would mix rendering state when clients overlap. The handler below assumes setup has populated the long-lived `registry`; see [StreamingAsync](../examples/StreamingAsync/) for the complete project.

| Need | Use |
| --- | --- |
| A request-owned dynamic render context | `beginSafeTemplateResponse()` |
| A fixed response-slot pool owned by the application | `beginBorrowedChunkedResponse()` |

```cpp
#include <TemplateEngine.h>
#include <TemplateEngineAsyncWeb.h>

std::shared_ptr<PlaceholderRegistry> registry;

void sendTemplate(AsyncWebServerRequest* request, const char* root) {
    auto context = std::make_shared<TemplateContext>();
    context->setRegistry(registry.get());
    TemplateRenderer::initializeContext(*context, root);

    // The helper retains the request-owned context until the response completes or disconnects.
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
    uint32_t lease = 0;
    TemplateContext context;
};

ResponseSlot slots[2];

void releaseSlot(ResponseSlot& slot, uint32_t lease) {
    // A completed response can disconnect after this slot has been reused.
    if (!slot.busy || slot.lease != lease) return;
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
    const uint32_t lease = ++slot->lease;
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
        [lease](ResponseSlot& state) { releaseSlot(state, lease); }));
}
```

Use this form only while the slot itself has static or otherwise guaranteed lifetime. The lease makes a late disconnect from an old response a no-op after the slot has been reused. Use the `shared_ptr` form above for a request-owned dynamic context.

Use [StreamingAsync](../examples/StreamingAsync/) for a complete SoftAP/captive-portal project.

Back to [documentation](README.md) · [project overview](../README.md).
