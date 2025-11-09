#pragma once

#include <Arduino.h>

namespace DFTEExamples {

// Shared CSS snippet stored in flash to minimise RAM usage.
inline const char PROGMEM SHARED_CSS[] = R"CSS(
body {
  font-family: Arial, sans-serif;
  margin: 0;
  padding: 1.5rem;
  background: #f4f6f9;
  color: #222;
}
h1, h2 {
  color: #0a3d62;
  margin-bottom: 0.5rem;
}
section {
  margin-bottom: 1.5rem;
  padding: 1rem;
  background: #ffffff;
  border-radius: 8px;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.08);
}
)CSS";

// Minimal header/footer fragments that examples can reuse.
inline const char PROGMEM SHARED_HEADER[] = R"HTML(
<header>
  <h1>%PAGE_TITLE%</h1>
  <p>%TAGLINE%</p>
</header>
)HTML";

inline const char PROGMEM SHARED_FOOTER[] = R"HTML(
<footer>
  <small>&copy; 2025 DFTE Examples</small>
</footer>
)HTML";

}  // namespace DFTEExamples

