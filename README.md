# Rotary Inverted Pendulum

From-scratch embedded firmware and control software for a rotary inverted pendulum.

The project currently targets the original **Forest S1 STM32F103C8T6 controller plus Forest D1 baseboard (2016 revision)** as the hardware baseline. Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

> [!CAUTION]
> The balance controller is not yet connected to physical motor output. Normal maintenance tests require explicit arm and remain limited to `20%` and 10 seconds. The separate characterization command may ramp to `30%`; it automatically stops and disarms on completion, timeout, direction reversal, or implausible encoder speed. Lift and secure the mechanism, keep clear of the rotating arm, and use a current-limited motor supply.
>
> The firmware now configures the 2016 Forest D1 schematic input at **PA7 / ADC1_IN7**, but the sensor zero, range, direction, wiring, and physical signal are not yet verified. Do not use the ADC reading for control until that physical validation is complete.

## Current status

The `main` branch contains these foundations:

- STM32F103C8T6 bring-up using [libopencm3](https://github.com/libopencm3/libopencm3)
- 1 kHz firmware timing baseline
- Sensor-acquisition framework with the pendulum ADC mapped to PA7 / ADC1_IN7
- Arm quadrature-encoder acquisition
- M-button-controlled UART telemetry at 115200 baud and 10 Hz when enabled
- Text UART command interface and RAM-only runtime parameter registry
- Wrap-safe pendulum ADC conversion to `[-pi, pi)`
- Bounded Motor B maintenance test on PB1/PB12/PB13
- VBUS monitoring on PA6 / ADC1_IN6 (nominal 11:1 conversion)
- Control-loop timing profiling output
- Platform-independent control configuration
- Fail-closed safety state machine
- Filtered four-state estimator
- Four-state balance controller using `u = -Kx`
- Five host-side unit-test suites

The estimator, controller, and safety modules exist in `control/`, but `app/main.c` does not yet connect them to motor output. The motor interface is available only through the bounded UART maintenance command.

The communication architecture keeps text mode for maintenance and reserves Micro XRCE-DDS for a measured feasibility milestone. COBS is intentionally not implemented. See [Communication and Parameter Architecture](docs/architecture/communications.md).

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
| Arm encoder | TIM2 quadrature input on PA0/PA1 (A0/A1 connector signals) |
| Maintenance interface | USART1 on PA9/PA10, 115200 baud; text commands and telemetry |
| Telemetry control | PA3 M button or `telem on/off`; default off; runtime rate 1–20 Hz |
| Motor B control | PB1 / TIM3_CH4 PWM, PB13 / BIN1, PB12 / BIN2 |
| Motor output | Maintenance test only: 20 kHz PWM, `±20%` maximum, 10 s maximum |

Motor/encoder polarity commissioning is available after the basic D2 and
PA0/PA1 checks pass:

```text
motor channel d2
motor arm
motor identify
```

`motor identify` applies a `+5%` pulse for 250 ms, stops for 250 ms, and only
retries once at `+8%` when fewer than three encoder counts are observed. It
then stops and disarms. The completion message reports encoder delta, inferred
motor/encoder sign, and peak observed encoder velocity. This command does not
enable position control or automatic swing-up.

The dead-zone characterization supersedes fixed-pulse guessing:

```text
motor arm
motor characterize right

motor arm
motor characterize left
```

It ramps from 5% to 30% in 2% steps, detects two consecutive 250 ms encoder
motion windows, confirms the detected encoder polarity, and then ramps down in
1% steps. Each descending step lasts 1.5 seconds, and only the final consecutive
motion windows determine whether that duty can sustain rotation. The result
reports `breakaway_pct`, `minimum_sustain_pct`, `dropout_pct`, encoder sign, and
windowed peak velocity. The encoder scale is 1040 quadrature counts per output
shaft revolution; 9516 counts/s is the rated-speed reference and 15000 counts/s
is the initial plausibility ceiling. This command does not enable position
control or automatic swing-up.

Stopping-response measurement uses a configurable operating point so the
geared output can be tested above the dead-zone-only range:

```text
motor arm
motor response right 50 5000

motor arm
motor response left 50 5000
```

The accepted response range is 30% to 80% and 1000 to 10000 ms. PWM is then
set to zero while the encoder is observed for up to 5 seconds. Three
consecutive 100 ms windows at no more than one count per window declare the
shaft stopped. The result reports signed drive displacement, velocity in the
last complete window before cutoff, stopping time, signed coast displacement,
and peak windowed velocity. Use 50%/5000 ms first in both directions, then
70%/5000 ms; extend a point to 10000 ms only if its cutoff velocity shows that
the geared output was still accelerating. The ordinary `motor test` command
remains limited to 20%.

Reverse-braking response uses the same drive point followed by a 1 ms neutral
guard, bounded opposite PWM, and a 300 ms output-off settling observation.
Start at 10%; do not try 15% or 20% until the 10% result and electrical traces
have been reviewed:

```text
motor arm
motor brake-response right 50 5000 10
```

Encoder position is accumulated wrap-safely every 1 ms. A 10 ms sliding
velocity estimate releases reverse PWM before estimated zero speed, while a
40 ms estimate is used only to classify the settling result. The initial
release threshold is 600 counts/s, reverse PWM is limited to 300 ms, and one
opposite encoder count cannot terminate the measurement. The report separates
drive, neutral, reverse-brake, and settling displacement and reports
`stop_reason=stable|reversal`. Completion and every fault path stop and disarm
automatically. D2 remains the only enabled channel; the measured encoder sign
at brake entry is used instead of assuming a fixed encoder polarity.

Repeated commissioning sequences can be stored in a RAM-only script buffer and
run after a single manual arm. Recording only stores whitelisted lines; it does
not start the motor:

```text
script load brake-sweep 50 5000
script list
motor arm
script run
```

`script load brake-sweep <drive_pct> <drive_ms> [brake_pct]` expands to three
right/left `motor brake-response` pairs with 5 second waits between tests.
When `brake_pct` is omitted, it defaults to 10%. The accepted ranges remain the
same as `motor brake-response`: 30% to 80%, 1000 to 10000 ms, and 10% to 20%.
Manual recording is still available for custom sequences:

```text
script clear
script begin
motor brake-response right 50 5000 10
wait 5000
motor brake-response left 50 5000 10
wait 5000
motor brake-response right 50 5000 10
wait 5000
motor brake-response left 50 5000 10
wait 5000
motor brake-response right 50 5000 10
wait 5000
motor brake-response left 50 5000 10
script end
script list
motor arm
script run
```

The first script implementation accepts only `motor brake-response ...` and
`wait <ms>`. It refuses `motor arm` inside the script, keeps PWM at zero during
waits, aborts on any motor failure or script timeout, and clears on reset.

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

1. Keep motor power disconnected for the initial boot and sensor checks.
2. Flash the STM32F103 firmware.
3. Confirm the status LED and boot messages.
4. Confirm the boot message reports PA7 / ADC1_IN7.
5. Press the PA3 M button once and verify 10 Hz UART sensor telemetry; press it again to stop the output.
6. Run `status`, `param list`, and `transport status` from the UART terminal.
7. Check ADC range, encoder direction, zero offsets, wrapped angle, and timing.
8. Lift and mechanically secure the unit so the arm can rotate without contact. Use a current-limited motor supply and keep an immediate power disconnect within reach.
9. With motor power still disconnected, run `motor status`, `motor arm`, and `motor test 5 100`; verify the command automatically returns to stopped/disarmed.
10. Connect motor power and repeat the minimum test. `motor stop` is available at any time. Re-arm before every test.
11. Test the opposite direction with `motor arm` followed by `motor test -5 100`. Increase duty or duration only if required, never beyond the firmware limits.

Motor maintenance commands:

```text
motor status
motor channel d1
motor channel d2
motor arm
motor identify
motor characterize right
motor characterize left
motor test <signed_percent> <duration_ms>
motor stop
motor disarm
```

`motor arm` remains valid for 30 seconds and still authorizes only one
bounded test. `motor status`, the test-start response, 10 Hz telemetry, and
the automatic-stop message report nominal `vbus_mV`. The conversion assumes
3.300 V VDDA and the schematic 10 kΩ / 1 kΩ divider, so calibrate it against a
trusted meter before using it for protection decisions.

`motor channel d1|d2` selects the output connector used by the next test.
Changing the channel first stops both PWM outputs and disarms the test. D1
uses PB0/TIM3_CH3 with PB14/PB15; D2 uses PB1/TIM3_CH4 with PB13/PB12. The
default remains D2 to preserve the previous maintenance-firmware behavior.

`motor arm` expires after 30 seconds if no test starts. A normal test accepts `-20..-1` or `1..20` percent and `50..10000 ms`. Every completion and explicit stop forces both PWM outputs to zero, sets all four direction pins low, and disarms the interface. This is a software safety layer, not a substitute for current limiting, physical guarding, or a hardware emergency disconnect.

## Development principles

- Keep control logic independent of the MCU platform.
- Test deterministic logic on the host.
- Prefer fail-closed behavior when inputs or state are invalid.
- Separate controller computation from physical motor actuation.
- Introduce hardware output only through explicit, reviewable milestones.
