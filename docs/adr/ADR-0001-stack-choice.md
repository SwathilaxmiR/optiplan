# ADR-0001: Technology Stack Choice

**Status:** Accepted  
**Date:** 2024-10-11  
**Deciders:** Project owner + AI coding assistant

## Context

We need to choose the core technology stack for OptiPlan: an optical network planning
tool with a C++ physics engine, Python ML pipeline, and interactive dashboard.

## Decision

| Component | Choice | Rationale |
|-----------|--------|-----------|
| **Language (core)** | C++17 | Widest compiler support (GCC 7+, Clang 5+, MSVC 19.14+). Provides `std::shared_mutex`, `std::optional`, structured bindings, `if constexpr`. C++20 features (concepts, ranges) not needed and narrow compiler support. |
| **Build system** | CMake ≥ 3.22 + Ninja | Industry standard for C++. Presets (`CMakePresets.json`) provide reproducible build configurations. Ninja is faster than Make for incremental builds. |
| **Dependencies** | FetchContent | Simpler than vcpkg/Conan for a self-contained academic project. Dependencies (GoogleTest, nlohmann-json, pybind11) are small and version-pinned. |
| **Testing** | GoogleTest 1.14 | Industry standard, mature, excellent IDE integration, parameterized tests, death tests. Catch2 is lighter but less commonly seen in industry. |
| **JSON** | nlohmann/json 3.11 | Header-only, intuitive API, dominant C++ JSON library. No runtime dependency. |
| **Python bindings** | pybind11 (Phase 6) | Lightweight, C++17-native, seamless type conversion. Not needed until Phase 6. |
| **ML framework** | scikit-learn + LightGBM + optional PyTorch (Phase 5) | Mature, well-documented. LightGBM for gradient-boosted trees; PyTorch optional for MLP. |
| **Dashboard** | Streamlit (Phase 6) | Minimal boilerplate for data-centric dashboards. Plotly for interactive charts. |
| **Formatting** | clang-format (Google-based) | Consistent code style, enforceable in CI. |
| **Linting** | clang-tidy | Catches bugs, enforces modernize/performance patterns at compile time. |

## Alternatives Considered

- **C++20**: `std::jthread`, concepts would be nice but Apple Clang support is inconsistent.
- **Meson**: Good but CMake has more ecosystem support and better IDE integration.
- **vcpkg**: More robust dependency management but adds complexity for a small project.
- **Catch2**: Lighter header-only framework but less industry recognition.
- **Dash (Plotly)**: More control than Streamlit but more boilerplate.

## Consequences

- All team members must have CMake ≥ 3.22 and a C++17 compiler.
- First build downloads ~50 MB of dependencies via FetchContent (one-time).
- Python 3.11+ recommended for ML phase (system Python 3.9 may work for bindings).
