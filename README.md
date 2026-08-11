# Rotary Inverted Pendulum

From-scratch embedded firmware and control software for a rotary inverted pendulum.

The project currently uses the original **STM32F103C8T6** control-board form factor as the hardware baseline. Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

> [!CAUTION]
> The current firmware application is still **sensor-only**. Motor output is not initialized, and the balance controller is not yet connected to the real-time firmware loop. Keep motor power disconnected during initial bring-up and sensor verification.

## Current status

The `main` branch contains these foundations:

- STM32F103C8T6 bring-up using [libopencm3](https://github.com/libopencm3/libopencm3)
- 1 kHz firmware timing baseline
- Pendulum ADC and arm quadrature-encoder acquisition
- UART telemetry at 115200 baud
- Control-loop timing profiling output
- Platform-independent control configuration
- Fail-closed safety state machine
- Filtered four-state estimator
- Four-state balance controller using `u = -Kx`
- Four host-side unit-test suites

The estimator, controller, and safety modules exist in `control/`, but `app/main.c` currently performs sensor acquisition and telemetry only.

### Next milestone

**V0.6 — `control_pipeline`**

Integrate acquisition, state estimation, safety decisions, and controller evaluation through a platform-independent pipeline while keeping physical motor output disabled until the interface and safety behavior are verified.

## Hardware baseline

| Item | Current baseline |
|---|---|
| MCU | STM32F103C8T6 |
| Framework | Bare metal with libopencm3 |
| Control tick | 1 kHz |
| Pendulum input | ADC1 channel 3 on PA3 |
| Arm encoder | TIM4 quadrature input on PB6/PB7 |
| Telemetry | USART1, 115200 baud, 100 Hz |
| Motor output | Not initialized in the current application |

The STM32F103C8T6 board remains the immediate baseline because it plugs directly into the existing driver board. A **Raspberry Pi Pico 2 / RP2350** port, including native USB HID and CDC, is a planned later platform and is not implemented on `main`.

## Repository layout

```text
app/                    STM32 firmware application
cmake/                  Cross-compilation toolchain
control/                Platform-independent estimator, safety, and control logic
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
4. Verify 100 Hz UART sensor telemetry.
5. Check ADC range, encoder direction, zero offsets, and timing.
6. Enable motor-related work only after sensor signs, scales, limits, and fail-closed behavior are confirmed.

## Development principles

- Keep control logic independent of the MCU platform.
- Test deterministic logic on the host.
- Prefer fail-closed behavior when inputs or state are invalid.
- Separate controller computation from physical motor actuation.
- Introduce hardware output only through explicit, reviewable milestones.
