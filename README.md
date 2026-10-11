# OptiPlan: QoT-Aware Optical Network Planner

A C++17 / Python project for routing, spectrum, and modulation planning in optical
transport networks, featuring a parallel physical-layer engine and machine-learning
QoT estimator.

---

## Quick Start

### Prerequisites

- C++17 compiler (GCC 7+, Clang 5+, or Apple Clang)
- CMake ≥ 3.22
- Ninja (recommended) or Make
- Python 3.9+ (3.11+ recommended for ML phase)

### Build & Test

```bash
# Configure (downloads GoogleTest + nlohmann-json on first run)
cmake --preset debug

# Build
cmake --build --preset debug

# Run tests
ctest --preset debug
```

### Sanitizer Builds

```bash
# AddressSanitizer + UBSan
cmake --preset asan
cmake --build --preset asan
ctest --preset asan

# ThreadSanitizer
cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan
```

---

## Repository Structure

```
optiplan/
├── core/                  # C++17 library (no Python dependency)
│   ├── include/optiplan/  # Public headers
│   ├── src/               # Implementation
│   ├── tests/             # GoogleTest unit + integration tests
│   └── bench/             # Benchmarks
├── bindings/              # pybind11 module (Phase 6)
├── ml/                    # ML pipeline: dataset gen, training, evaluation (Phase 5)
├── app/                   # Streamlit dashboard (Phase 6)
├── data/topologies/       # Network topologies (NSFNET, toy4, etc.)
├── configs/               # Scenario configuration files
└── docs/                  # Architecture, references, walkthroughs, ADRs
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — Module diagram and design principles
- [References](docs/REFERENCES.md) — Citations for all models and algorithms
- [Assumptions](docs/ASSUMPTIONS.md) — Simplifying assumptions and their impact
- [ADR-0001: Stack Choice](docs/adr/ADR-0001-stack-choice.md)

---

## Project Status

| Phase | Status |
|-------|--------|
| 0. Setup | ✅ Complete |
| 1. Physics | 🔲 Not started |
| 2. Network + Routing | 🔲 Not started |
| 3. Planner + Simulator | 🔲 Not started |
| 4. Concurrency | 🔲 Not started |
| 5. ML | 🔲 Not started |
| 6. Bindings + Dashboard | 🔲 Not started |
| 7. Report | 🔲 Not started |

---

## License

Academic project — license TBD.
