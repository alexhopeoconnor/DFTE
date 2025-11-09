#ifndef TEMPLATE_ENGINE_H
#define TEMPLATE_ENGINE_H

/**
 * DeviceFramework Template Engine
 * Main public API header - include this in your projects
 * 
 * Memory-efficient streaming template renderer for ESP8266/ESP32
 * Supports chunked rendering with PROGMEM templates and nested template support
 * 
 * Usage:
 *   1. Create a PlaceholderRegistry and register your placeholders
 *   2. Create a TemplateContext
 *   3. Set the registry on the context
 *   4. Initialize with your template
 *   5. Call renderNextChunk() repeatedly until complete
 * 
 * Example:
 *   DeviceFrameworkPlaceholderRegistry registry;
 *   registry.registerProgmemData("%CSS%", my_css);
 *   
 *   DeviceFrameworkTemplateContext ctx;
 *   ctx.setRegistry(&registry);
 *   DeviceFrameworkTemplateRenderer::initializeContext(ctx, my_template);
 *   
 *   uint8_t buffer[512];
 *   while (!DeviceFrameworkTemplateRenderer::isComplete(ctx)) {
 *       size_t written = DeviceFrameworkTemplateRenderer::renderNextChunk(ctx, buffer, sizeof(buffer));
 *       server->sendContent((const char*)buffer, written);
 *   }
 */

// Type definitions first
#include "DeviceFrameworkTemplateTypes.h"

// Core components
#include "DeviceFrameworkTemplateRenderer.h"
#include "DeviceFrameworkTemplateContext.h"
#include "DeviceFrameworkPlaceholderRegistry.h"
#include "DeviceFrameworkTemplateEngineDebug.h"

// Type aliases for convenience
using TemplateRenderer = DeviceFrameworkTemplateRenderer;
using TemplateContext = DeviceFrameworkTemplateContext;
using PlaceholderRegistry = DeviceFrameworkPlaceholderRegistry;

// Use new type names
using PlaceholderType = PlaceholderType;
using PlaceholderEntry = PlaceholderEntry;
using RenderingContextType = RenderingContextType;
using RenderingContext = RenderingContext;
using TemplateRenderState = TemplateRenderState;

#endif // TEMPLATE_ENGINE_H

