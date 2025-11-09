#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <Arduino.h>
#include "utils/test_utils.h"

// Test function declarations
// Group 1: PlaceholderRegistry Tests
void test_placeholder_registry_registration();
void test_placeholder_registry_lookup();
void test_placeholder_registry_rendering();
void test_placeholder_registry_edge_cases();

// Group 2: TemplateContext Tests
void test_template_context_initialization();
void test_template_context_stack();
void test_template_context_buffer();
void test_template_context_state();

// Group 3: TemplateRenderer Tests
void test_template_renderer_basic();
void test_template_renderer_chunked();
void test_template_renderer_state_transitions();
void test_template_renderer_placeholders();
void test_template_renderer_nested();
void test_template_renderer_nested_four_levels();
void test_template_renderer_nested_chunk_progress();
void test_template_renderer_large_templates();
void test_template_renderer_parallel_contexts();
void test_template_renderer_root_ram_template();
void test_template_renderer_missing_registry();
void test_template_renderer_dynamic_basic();
void test_template_renderer_dynamic_empty();
void test_template_renderer_dynamic_mutable();
void test_template_renderer_conditional_true_branch();
void test_template_renderer_conditional_false_branch();
void test_template_renderer_conditional_skip();
void test_template_renderer_conditional_missing_delegate();
void test_template_renderer_conditional_nested_iterator();
void test_template_renderer_iterator_basic();
void test_template_renderer_iterator_empty();
void test_template_renderer_iterator_dynamic_items();
void test_template_renderer_iterator_error_cleanup();

// Group 4: Integration Tests
void test_integration_full_rendering();
void test_integration_memory_efficiency();
void test_integration_multiple_templates();

// Group 5: Edge Cases
void test_edge_cases_error_handling();
void test_edge_cases_boundary_conditions();
void test_edge_cases_stress();

#endif // TEST_MAIN_H

