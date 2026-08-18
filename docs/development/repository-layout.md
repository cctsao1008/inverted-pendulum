# Repository Layout

This document describes the top-level repository structure.

```text
app/                    STM32 firmware application and system-level adapters
cmake/                  Cross-compilation toolchain
control/                Platform-independent estimator, safety, and control logic
drivers/                 Display and peripheral drivers
docs/architecture/      Source-coupled architecture and contracts
docs/commissioning/     Firmware and plant commissioning procedures
docs/development/       Repository structure and build instructions
docs/hardware/          Schematic-derived hardware baselines and validation notes
docs/process/           Engineering process records
docs/validation/        Evidence and capability-validation model
docs/templates/         Reusable engineering templates
platform/stm32f103/     STM32F103 board support and linker configuration
tests/                  Host-side unit tests
third_party/            External source dependencies
tools/                  Reproducible validation and runtime tooling
```

The control core is intentionally separated from target-specific platform code so deterministic estimator, safety, state-machine, and controller behavior can be exercised independently of the STM32 hardware layer.
