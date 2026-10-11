# Phase 0 Walkthrough: Repository & Infrastructure Setup

## 1. What It Does
Phase 0 establishes the engineering foundation of **OptiPlan**:
- Initializes the C++17 build system using CMake presets (`debug`, `release`, `asan`, `tsan`) with Ninja.
- Sets up unit testing with GoogleTest (v1.14.0) and JSON serialization with `nlohmann::json` (v3.11.3) via CMake's `FetchContent`.
- Integrates static configuration loading (`optiplan::Config`) supporting parameter defaults (§2.1 of project specification) and JSON overrides.
- Provides canonical optical network topologies: `toy4` (for verification & hand-computation) and `nsfnet` (14 nodes, 21 bidirectional links).
- Enforces strict code quality: `-Wall -Wextra -Wpedantic -Werror`, `clang-format` (Google style), and AddressSanitizer/ThreadSanitizer validation.

## 2. Why It Is Designed This Way
- **CMake Presets**: Standardizes build configurations across developer machines and CI runners without memorizing complex command-line flags.
- **FetchContent vs. System Package Managers**: Keeps the project self-contained and reproducible without requiring users or CI to pre-install specific C++ library versions.
- **Hierarchical Config Object**: Decouples physical constants from algorithmic code. Physical layer properties (dispersion, attenuation, noise figure) can be modified without recompilation.
- **Sanitizers from Day One**: Running ASan (AddressSanitizer/UBSan) and TSan (ThreadSanitizer) from the very first commit prevents memory bugs and data races before concurrency (Phase 4) is implemented.

## 3. Key Function / Code to Read First
- `core/include/optiplan/config.hpp` and `core/src/config.cpp`:
  - Look at `optiplan::Config::loadFromJson`: See how physical default values are preserved unless explicitly overridden by incoming JSON objects.
- `core/tests/test_config.cpp`:
  - Illustrates how unit tests assert default parameters and partial/full JSON overrides.

---

## 4. Likely Interview Questions & Answers

### Q1: Why did you choose C++17 over C++20 or C++14 for this high-performance optical networking engine?
**Answer:**
C++17 was chosen because it provides the exact language features needed for high-concurrency simulation—such as `std::shared_mutex` (reader-writer locks), `std::optional`, structured bindings, and `if constexpr`—while maintaining universal, mature compiler support across Linux, macOS (Apple Clang), and CI environments. C++14 lacks native reader-writer locks in `<shared_mutex>`, while C++20 features (like coroutines or modules) add compiler variability without substantial benefits for our event-driven simulation model.

### Q2: How do you ensure that parameters like fiber attenuation and amplifier noise figure are not hardcoded or magically tuned?
**Answer:**
Every parameter is represented in explicit structs (`FiberParams`, `AmplifierParams`, `ChannelParams`) initialized to standardized SSMF / C-band values from published ITU-T / literature standards. The `Config::loadFromJson` loader reads from `configs/default.json` or scenario files. If a parameter is absent, the documented standard default is kept. Any deviations or assumptions are tracked explicitly in `docs/ASSUMPTIONS.md` and cited in `docs/REFERENCES.md`.

### Q3: What is the purpose of configuring AddressSanitizer (ASan) and ThreadSanitizer (TSan) in Phase 0 rather than later?
**Answer:**
Retrofitting sanitizers onto an existing codebase often uncovers dozens of subtle undefined behavior and memory leaks, making triage expensive. In an optical network simulator where we later implement multi-threaded candidate evaluation (Phase 4) with `std::shared_mutex` and custom thread pools, having TSan and ASan active in CI from Day 0 ensures zero data races and zero memory leaks at each incremental step.
