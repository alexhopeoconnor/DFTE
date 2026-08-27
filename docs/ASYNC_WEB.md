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

Use [StreamingAsync](../examples/StreamingAsync/) for a complete SoftAP/captive-portal project.

Back to [documentation](README.md) · [project overview](../README.md).
