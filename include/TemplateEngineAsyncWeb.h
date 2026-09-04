#ifndef TEMPLATE_ENGINE_ASYNC_WEB_H
#define TEMPLATE_ENGINE_ASYNC_WEB_H

#include <Arduino.h>
#include <memory>
#include <ESPAsyncWebServer.h>
#include "TemplateEngine.h"

namespace TemplateEngineAsyncWeb {

inline void yieldForChunkRetry() {
#if defined(ARDUINO_ARCH_ESP8266)
    optimistic_yield(1000);
#else
    yield();
#endif
}

template <typename ContextT>
inline bool isTemplateTerminal(const ContextT& context) {
    return TemplateRenderer::isComplete(context) || TemplateRenderer::hasError(context);
}

template <typename ContextT>
inline size_t renderTemplateChunkWithRetries(ContextT& context,
                                             uint8_t* buffer,
                                             size_t maxLen,
                                             unsigned maxNoProgressRetries = 32) {
    if (maxLen == 0) {
        return RESPONSE_TRY_AGAIN;
    }

    for (unsigned attempt = 0; attempt < maxNoProgressRetries; ++attempt) {
        size_t written = TemplateRenderer::renderNextChunk(context, buffer, maxLen);
        if (written > 0 || isTemplateTerminal(context)) {
            return written;
        }

        yieldForChunkRetry();
    }

    return RESPONSE_TRY_AGAIN;
}

template <typename StateT, typename FillFn, typename IsDoneFn>
AsyncWebServerResponse* beginSafeChunkedResponse(AsyncWebServerRequest* request,
                                                 const char* contentType,
                                                 const std::shared_ptr<StateT>& sharedState,
                                                 FillFn fill,
                                                 IsDoneFn isDone) {
    request->onDisconnect([state = sharedState]() mutable {
        state.reset();
    });

    return request->beginChunkedResponse(contentType,
        [state = sharedState, fill, isDone](uint8_t* buffer, size_t maxLen, size_t index) mutable -> size_t {
            if (!state) {
                return 0;
            }

            if (maxLen == 0) {
                return RESPONSE_TRY_AGAIN;
            }

            size_t written = fill(*state, buffer, maxLen, index);
            if (written == RESPONSE_TRY_AGAIN) {
                return RESPONSE_TRY_AGAIN;
            }

            if (!state) {
                return 0;
            }

            if (written > 0) {
                return written;
            }

            if (isDone(*state)) {
                state.reset();
                return 0;
            }

            yieldForChunkRetry();
            return RESPONSE_TRY_AGAIN;
        });
}

// A bounded embedded server can own response state in a fixed slot instead of
// allocating a shared control block for every request. The caller guarantees
// that state remains valid until release() is called. release() must be safe
// to call more than once because a completed request can later disconnect.
template <typename StateT, typename FillFn, typename IsDoneFn, typename ReleaseFn>
AsyncWebServerResponse* beginBorrowedChunkedResponse(AsyncWebServerRequest* request,
                                                     const char* contentType,
                                                     StateT* state,
                                                     FillFn fill,
                                                     IsDoneFn isDone,
                                                     ReleaseFn release) {
    request->onDisconnect([state, release]() mutable {
        if (state != nullptr) {
            release(*state);
        }
    });

    return request->beginChunkedResponse(contentType,
        [state, fill, isDone, release](uint8_t* buffer, size_t maxLen, size_t index) mutable -> size_t {
            if (state == nullptr) {
                return 0;
            }

            if (isDone(*state)) {
                release(*state);
                return 0;
            }

            if (maxLen == 0) {
                return RESPONSE_TRY_AGAIN;
            }

            size_t written = fill(*state, buffer, maxLen, index);
            if (written == RESPONSE_TRY_AGAIN) {
                return RESPONSE_TRY_AGAIN;
            }

            if (written > 0) {
                return written;
            }

            if (isDone(*state)) {
                release(*state);
                return 0;
            }

            yieldForChunkRetry();
            return RESPONSE_TRY_AGAIN;
        });
}

// Variant for dynamically allocated state when the owner also needs an
// idempotent completion notification (for example, returning a bounded web
// response permit). The original overload intentionally remains available for
// callers that only need shared ownership.
template <typename StateT, typename FillFn, typename IsDoneFn, typename ReleaseFn>
AsyncWebServerResponse* beginSafeChunkedResponse(AsyncWebServerRequest* request,
                                                 const char* contentType,
                                                 const std::shared_ptr<StateT>& sharedState,
                                                 FillFn fill,
                                                 IsDoneFn isDone,
                                                 ReleaseFn release) {
    request->onDisconnect([state = sharedState, release]() mutable {
        release();
        state.reset();
    });

    return request->beginChunkedResponse(contentType,
        [state = sharedState, fill, isDone, release](uint8_t* buffer, size_t maxLen, size_t index) mutable -> size_t {
            if (!state) {
                return 0;
            }

            if (maxLen == 0) {
                return RESPONSE_TRY_AGAIN;
            }

            size_t written = fill(*state, buffer, maxLen, index);
            if (written == RESPONSE_TRY_AGAIN) {
                return RESPONSE_TRY_AGAIN;
            }

            if (written > 0) {
                return written;
            }

            if (isDone(*state)) {
                release();
                state.reset();
                return 0;
            }

            yieldForChunkRetry();
            return RESPONSE_TRY_AGAIN;
        });
}

template <typename StateT, typename FillFn, typename IsDoneFn>
AsyncWebServerResponse* beginSafeChunkedResponse(AsyncWebServerRequest* request,
                                                 const String& contentType,
                                                 const std::shared_ptr<StateT>& sharedState,
                                                 FillFn fill,
                                                 IsDoneFn isDone) {
    request->onDisconnect([state = sharedState]() mutable {
        state.reset();
    });

    return request->beginChunkedResponse(contentType,
        [state = sharedState, fill, isDone](uint8_t* buffer, size_t maxLen, size_t index) mutable -> size_t {
            if (!state) {
                return 0;
            }

            if (maxLen == 0) {
                return RESPONSE_TRY_AGAIN;
            }

            size_t written = fill(*state, buffer, maxLen, index);
            if (written == RESPONSE_TRY_AGAIN) {
                return RESPONSE_TRY_AGAIN;
            }

            if (!state) {
                return 0;
            }

            if (written > 0) {
                return written;
            }

            if (isDone(*state)) {
                state.reset();
                return 0;
            }

            yieldForChunkRetry();
            return RESPONSE_TRY_AGAIN;
        });
}

template <typename ContextT, typename ReleaseFn>
AsyncWebServerResponse* beginSafeTemplateResponse(AsyncWebServerRequest* request,
                                                  const char* contentType,
                                                  const std::shared_ptr<ContextT>& sharedContext,
                                                  unsigned maxNoProgressRetries,
                                                  ReleaseFn release) {
    return beginSafeChunkedResponse(
        request,
        contentType,
        sharedContext,
        [maxNoProgressRetries](ContextT& context, uint8_t* buffer, size_t maxLen, size_t /*index*/) -> size_t {
            return renderTemplateChunkWithRetries(context, buffer, maxLen, maxNoProgressRetries);
        },
        [](const ContextT& context) -> bool {
            return isTemplateTerminal(context);
        },
        release);
}

template <typename ContextT>
AsyncWebServerResponse* beginSafeTemplateResponse(AsyncWebServerRequest* request,
                                                  const char* contentType,
                                                  const std::shared_ptr<ContextT>& sharedContext,
                                                  unsigned maxNoProgressRetries = 32) {
    return beginSafeChunkedResponse(
        request,
        contentType,
        sharedContext,
        [maxNoProgressRetries](ContextT& context, uint8_t* buffer, size_t maxLen, size_t /*index*/) -> size_t {
            return renderTemplateChunkWithRetries(context, buffer, maxLen, maxNoProgressRetries);
        },
        [](const ContextT& context) -> bool {
            return isTemplateTerminal(context);
        });
}

template <typename ContextT>
AsyncWebServerResponse* beginSafeTemplateResponse(AsyncWebServerRequest* request,
                                                  const String& contentType,
                                                  const std::shared_ptr<ContextT>& sharedContext,
                                                  unsigned maxNoProgressRetries = 32) {
    return beginSafeChunkedResponse(
        request,
        contentType,
        sharedContext,
        [maxNoProgressRetries](ContextT& context, uint8_t* buffer, size_t maxLen, size_t /*index*/) -> size_t {
            return renderTemplateChunkWithRetries(context, buffer, maxLen, maxNoProgressRetries);
        },
        [](const ContextT& context) -> bool {
            return isTemplateTerminal(context);
        });
}

} // namespace TemplateEngineAsyncWeb

#endif // TEMPLATE_ENGINE_ASYNC_WEB_H
