# Rotary Inverted Pendulum

From-scratch embedded firmware and control software for a rotary inverted pendulum.

The project currently targets the original **Forest S1 STM32F103C8T6 controller plus Forest D1 baseboard (2016 revision)** as the hardware baseline. Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

> [!CAUTION]
> The current firmware application is still **sensor-only**. Motor output is not initialized, and the balance controller is not yet connected to the real-time firmware loop. Keep motor power disconnected during initial bring-up and sensor verification.
>
> The firmware now configures the 2016 Forest D1 schematic input at **PA7 / ADC1_IN7**, but the sensor zero, range, direction, wiring, and physical signal are not yet verified. Do not use the ADC reading for control until that physical validation is complete.

## Current status

The `main` branch contains these foundations:

- STM32F103C8T6 bring-up using [libopencm3](https://github.com/libopencm3/libopencm3)
- 1 kHz firmware timing baseline
- Sensor-acquisition framework with the pendulum ADC mapped to PA7 / ADC1_IN7
- Arm quadrature-encoder acquisition
- M-button-controlled UART telemetry at 115200 baud and 10 Hz when enabled
- Control-loop timing profiling output
- Platform-independent control configuration
- Fail-closed safety state machine
- Filtered four-state estimator
- Four-state balance controller using `u = -Kx`
- Five host-side unit-test suites

The estimator, controller, and safety modules exist in `control/`, but `app/main.c` currently performs sensor acquisition and telemetry only.

### Next milestone

**V0.6 — `control_pipeline`**

Integrate acquisition, state estimation, safety decisions, and controller evaluation through a platform-independent pipeline while keeping physical motor output disabled until the interface and safety behavior are verified.

Before V0.6 is connected to hardware, the PA7 pendulum ADC mapping, sensor zero, range, direction, encoder direction, and motor-interface polarity must be verified.

## Hardware baseline

| Item | Forest S1 + Forest D1 (2016) baseline |
|---|---|
| MCU | STM32F103C8T6 |
| Framework | Bare metal with libopencm3 |
| MCU clock | 8 MHz HSE, 72 MHz system clock |
| Control tick | 1 kHz |
| Pendulum input | **PA7 / ADC1_IN7**; firmware-mapped, physical signal verification pending |
| Battery voltage input | PA6 / ADC1_IN6 through a 10 kΩ / 1 kΩ divider |
| Arm encoder | TIM4 quadrature input on PB6/PB7 |
| Telemetry | USART1 on PA9/PA10, 115200 baud; PA3 M button toggles 10 Hz output; default off |
| Motor B control | PB1 / TIM3_CH4 PWM, PB13 / BIN1, PB12 / BIN2 |
| Motor output | Not initialized in the current application |

See [Forest D1 2016 hardware baseline](docs/hardware/forest-d1-2016-baseline.md) for the schematic-derived pin map, revision boundary, electrical observations, firmware discrepancy, and physical validation checklist.

The STM32F103C8T6 board remains the immediate baseline because it plugs directly into the existing driver board. A **Raspberry Pi Pico 2 / RP2350** port, including native USB HID and CDC, is a planned later platform and is not implemented on `main`.

## Repository layout

```text
app/                    STM32 firmware application
cmake/                  Cross-compilation toolchain
control/                Platform-independent estimator, safety, and control logic
docs/hardware/          Schematic-derived hardware baselines and validation notes
platform/stm32f103/     STM32F103 board support and linker configuration
tests/                  Host-side unit tests
third_party/libopencm3/ Git submodule
```

## Clone

Clone with the libopencm3 submodule:

```bash
git clone --recursive https://github.com/cctsao1008/inverted-pendulum.git
cd inverted-pendulum
```

For an existing clone:

```bash
git submodule update --init --recursive
```

## Run host tests

Requirements:

- CMake 3.16 or newer
- Ninja
- A C11 host compiler

```bash
cmake -S . -B build/host -G Ninja \
  -DBUILD_STM32_FIRMWARE=OFF \
  -DBUILD_HOST_TESTS=ON

cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

The current test executables cover:

- control configuration
- safety manager
- state estimator
- balance controller
- telemetry button debounce and toggle behavior

## Build STM32F103 firmware

Additional requirements:

- Arm GNU Toolchain (`arm-none-eabi-gcc`)
- GNU Make for building libopencm3

```bash
cmake -S . -B build/stm32f103 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DBUILD_STM32_FIRMWARE=ON \
  -DBUILD_HOST_TESTS=OFF

cmake --build build/stm32f103
```

Expected outputs:

```text
build/stm32f103/inverted-pendulum.elf
build/stm32f103/inverted-pendulum.hex
build/stm32f103/inverted-pendulum.bin
build/stm32f103/inverted-pendulum.map
```

## Bring-up sequence

1. Keep motor power disconnected.
2. Flash the STM32F103 firmware.
3. Confirm the status LED and boot messages.
4. Confirm the boot message reports PA7 / ADC1_IN7.
5. Press the PA3 M button once and verify 10 Hz UART sensor telemetry; press it again to stop the output.
6. Check ADC range, encoder direction, zero offsets, and timing.
7. Enable motor-related work only after sensor signs, scales, limits, and fail-closed behavior are confirmed.

## Development principles

- Keep control logic independent of the MCU platform.
- Test deterministic logic on the host.
- Prefer fail-closed behavior when inputs or state are invalid.
- Separate controller computation from physical motor actuation.
- Introduce hardware output only through explicit, reviewable milestones.
