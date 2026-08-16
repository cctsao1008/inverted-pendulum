# Rotary Inverted Pendulum

Re-engineered embedded firmware and control software for the existing Forest S1 / Forest D1 rotary inverted pendulum platform.

The project currently targets the original **Forest S1 STM32F103C8T6 controller plus Forest D1 baseboard (2016 revision)** as the hardware baseline. This is not a from-scratch unknown-hardware bring-up: the historical mechanism is a previously working rotary inverted pendulum, while the current firmware/control implementation is being rebuilt and revalidated property by property. A legacy-proven system fact is not automatically treated as proof that the current implementation is correct.

Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

> [!CAUTION]
> The balance controller is not yet connected to physical motor output. Normal `motor test` maintenance commands require explicit arm and remain limited to `20%` and 10 seconds. Characterization may ramp to `30%`; dedicated response/brake characterization commands can use higher explicitly requested duties within their own bounds. These paths automatically stop/disarm on their defined completion or fault conditions. Lift and secure the mechanism, keep clear of the rotating arm, and use a current-limited motor supply.
>
> The firmware maps the pendulum input to **PA7 / ADC1_IN7**, and runtime observations exist, but the complete control-grade calibration is not finished. Zero/reference, sign convention, full physical range/wrap behavior, noise/bias/repeatability, and approved control limits are tracked separately. Do not infer active-control validity from the existence of an ADC reading alone.

## Project truth and validation model

This repository distinguishes several kinds of evidence:

- **LEGACY-PROVEN** — known behavior of the earlier working Forest system.
- **DOC** — supported by schematic, datasheet, vendor material, or another controlled document.
- **CODE** — implemented in the current repository.
- **MEASURED** — observed on the current physical specimen.
- **INFERRED** — engineering inference that still requires confirmation.
- **UNKNOWN** — not yet established.

Hardware and control properties are validated independently. For example, `PA7 / ADC1_IN7` can be a documented and implemented mapping while its zero, direction, noise, and control suitability remain separate validation items.

## Current status

The `main` branch contains these foundations:

- STM32F103C8T6 bring-up using [libopencm3](https://github.com/libopencm3/libopencm3)
- 1 kHz firmware timing baseline
- Sensor-acquisition framework with the pendulum ADC mapped to PA7 / ADC1_IN7
- Arm quadrature-encoder acquisition
- Five-key local input service for M/X/+/-/USER with debounce, long-press, and repeat events
- **SSD1315-class** 128x64 local status UI using bounded software-SPI updates
- USER-key-controlled UART telemetry at 115200 baud and 10 Hz when enabled
- Text UART command interface and RAM-only runtime parameter registry
- Wrap-safe pendulum ADC conversion to `[-pi, pi)`
- Bounded Motor B maintenance test on PB1/PB12/PB13
- VBUS monitoring on PA6 / ADC1_IN6 (nominal 11:1 conversion; calibration still required before protection use)
- Control-loop timing profiling output
- Platform-independent control configuration
- Fail-closed state-safety path
- Filtered four-state estimator
- Four-state balance-controller path using `u = -Kx`
- Platform-independent `control_pipeline` with dedicated runtime status
- Closed-loop admission gate and explicit operator enable request
- Legitimate closed-loop mode-transition helper for `DISABLED -> IDLE -> BALANCE` admission flow
- Motor Authority Arbiter with NONE / MAINTENANCE / CONTROL / FAULT ownership semantics
- 5 ms stale closed-loop output watchdog contract
- STM32 observe-only control runtime with wrap-safe continuous arm position and control trace
- Host-side tests for control contracts, safety boundaries, runtime configuration, and application adapters

`app/main.c` feeds real STM32 sensor samples into `control_pipeline_step()` at the real 1 kHz cadence without replaying missed control cycles. The automatic-control motor sink remains intentionally **unbound**, so the bounded UART maintenance path remains the only current path that can write the physical motor.

The communication architecture keeps text mode for maintenance and reserves Micro XRCE-DDS for a measured feasibility milestone. COBS is intentionally not implemented. See [Communication and Parameter Architecture](docs/architecture/communications.md).

### Current development position

The active closed-loop commissioning sequence is intentionally staged:

```text
hardware / I/O bring-up                     DONE
maintenance motor path                      DONE
control computation architecture            DONE
observe-only runtime                        DONE
explicit observe-only state safety          DONE
dedicated pipeline status                   DONE
closed-loop admission gate                  DONE
legitimate mode-transition implementation   DONE / runtime validation in progress
admission vs continuous run-permit split    NEXT
operator control interface                  PLANNED
magnitude / slew output limits              PLANNED
independent emergency stop                  PLANNED
full observe-only admission proof           PLANNED
CONTROL motor-sink binding                  BLOCKED
plant identification                        PLANNED
PID / LQR commissioning                     PLANNED
restrained physical balance commissioning   BLOCKED
```

The immediate technical focus is the legitimate closed-loop mode transition and its runtime validation while the automatic motor sink remains unbound. Physical actuator binding is deliberately later work.

## Hardware baseline

| Item | Forest S1 + Forest D1 (2016) baseline |
|---|---|
| MCU | STM32F103C8T6 |
| Framework | Bare metal with libopencm3 |
| MCU clock | 8 MHz HSE, 72 MHz system clock |
| Control tick | 1 kHz |
| Pendulum input | **PA7 / ADC1_IN7**; mapping implemented; full control-grade calibration tracked separately |
| Battery voltage input | PA6 / ADC1_IN6 through a 10 kΩ / 1 kΩ divider; meter calibration required before protection use |
| Arm encoder | TIM2 quadrature input on PA0/PA1 (A0/A1 connector signals); physical sign/zero/scale validation tracked separately |
| Maintenance interface | USART1 on PA9/PA10, 115200 baud; text commands and telemetry |
| Local keys | PA3 M, PA2 X, PA11 +, PA12 -, PA5 USER; active-low |
| Telemetry control | PA5 USER button or `telem on/off`; default off; runtime rate 1–20 Hz |
| OLED | **SSD1315-class** 128x64 software SPI: PB5 CLK, PB4 DATA, PB3 RESET, PA15 D/C; SWD retained |
| Motor B control | PB1 / TIM3_CH4 PWM, PB13 / BIN1, PB12 / BIN2 |
| Motor output | Maintenance test only: 20 kHz PWM, `±20%` maximum for ordinary tests; specialized characterization commands have their own explicit bounds |

### Pendulum-sensor validation status

The pendulum sensor must not be represented by one coarse VERIFIED / NOT VERIFIED flag. Current engineering status is property-specific:

| Property | Current status |
|---|---|
| Forest D1 schematic / MCU mapping | DOC + CODE: PA7 / ADC1_IN7 |
| Current specimen produces a runtime signal | MEASURED |
| Upright reference | Partial measured evidence; must remain traceable to the current specimen and test record |
| Sign convention | Requires explicit final validation against the defined control coordinates |
| Full physical range and wrap behavior | Not fully validated |
| Noise, bias, repeatability | Not fully characterized |
| Approved active-control calibration / safety limits | Not established |

Motor/encoder polarity commissioning is available after the basic D2 and PA0/PA1 checks pass:

```text
motor channel d2
motor arm
motor identify
```

`motor identify` applies a `+5%` pulse for 250 ms, stops for 250 ms, and only retries once at `+8%` when fewer than three encoder counts are observed. It then stops and disarms. The completion message reports encoder delta, inferred motor/encoder sign, and peak observed encoder velocity. This command does not enable position control or automatic swing-up.

The dead-zone characterization supersedes fixed-pulse guessing:

```text
motor arm
motor characterize right

motor arm
motor characterize left
```

It ramps from 5% to 30% in 2% steps, detects two consecutive 250 ms encoder motion windows, confirms the detected encoder polarity, and then ramps down in 1% steps. Each descending step lasts 1.5 seconds, and only the final consecutive motion windows determine whether that duty can sustain rotation. The result reports `breakaway_pct`, `minimum_sustain_pct`, `dropout_pct`, encoder sign, and windowed peak velocity. The encoder scale currently used by the firmware is 1040 quadrature counts per output shaft revolution; this remains subject to the physical validation tracked for the installed mechanism. The rated-speed reference is 9516 counts/s and 15000 counts/s is the initial plausibility ceiling. This command does not enable position control or automatic swing-up.

Stopping-response measurement uses a configurable operating point so the geared output can be tested above the dead-zone-only range:

```text
motor arm
motor response right 50 5000

motor arm
motor response left 50 5000
```

The accepted response range is 30% to 80% and 1000 to 10000 ms. PWM is then set to zero while the encoder is observed for up to 5 seconds. Three consecutive 100 ms windows at no more than one count per window declare the shaft stopped. The result reports signed drive displacement, velocity in the last complete window before cutoff, stopping time, signed coast displacement, and peak windowed velocity. Use 50%/5000 ms first in both directions, then 70%/5000 ms; extend a point to 10000 ms only if its cutoff velocity shows that the geared output was still accelerating. The ordinary `motor test` command remains limited to 20%.

Reverse-braking response uses the same drive point followed by a 1 ms neutral guard, bounded opposite PWM, and a 300 ms output-off settling observation. Start at 10%; do not try 15% or 20% until the 10% result and electrical traces have been reviewed:

```text
motor arm
motor brake-response right 50 5000 10
```

Encoder position is accumulated wrap-safely every 1 ms. A 10 ms sliding velocity estimate releases reverse PWM before estimated zero speed, while a 40 ms estimate is used only to classify the settling result. The initial release threshold is 600 counts/s, reverse PWM is limited to 300 ms, and one opposite encoder count cannot terminate the measurement. The report separates drive, neutral, reverse-brake, and settling displacement and reports `stop_reason=stable|reversal`. Completion and every fault path stop and disarm automatically. Successful or reversal-classified runs also emit a compact `[MOTOR_CSV]` line:

```text
[MOTOR_CSV] brake_response,stop_reason,direction,drive_pct,drive_ms,brake_pct,drive_delta,cutoff_velocity_counts_s,neutral_delta,brake_entry_velocity_counts_s,brake_time_ms,braking_delta,release_velocity_counts_s,settling_delta,final_velocity_counts_s,peak_velocity_counts_s,vbus_mV
```

D2 remains the only enabled channel for the current baseline; the measured encoder sign at brake entry is used instead of assuming a fixed encoder polarity.

Repeated commissioning sequences can be stored in a RAM-only script buffer and run after a single manual arm. Recording only stores whitelisted lines; it does not start the motor:

```text
script load brake-sweep 50 5000
script list
motor arm
script run
```

`script load brake-sweep <drive_pct> <drive_ms> [brake_pct]` expands to three right/left `motor brake-response` pairs with 5 second waits between tests. When `brake_pct` is omitted, it defaults to 10%. The accepted ranges remain the same as `motor brake-response`: 30% to 80%, 1000 to 10000 ms, and 10% to 20%. Manual recording is still available for custom sequences:

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

The first script implementation accepts only `motor brake-response ...` and `wait <ms>`. It refuses `motor arm` inside the script, keeps PWM at zero during waits, aborts on any motor failure or script timeout, and clears on reset.

See [Forest D1 2016 hardware baseline](docs/hardware/forest-d1-2016-baseline.md) for the schematic-derived pin map, revision boundary, electrical observations, firmware discrepancy, and physical validation checklist.

The STM32F103C8T6 board remains the immediate baseline because it plugs directly into the existing driver board. A **Raspberry Pi Pico 2 / RP2350** port, including native USB HID and CDC, is a planned later platform and is not implemented on `main`.

## Repository layout

```text
app/                    STM32 firmware application and system-level adapters
cmake/                  Cross-compilation toolchain
control/                Platform-independent estimator, safety, and control logic
drivers/ssd1315/        Active SSD1315-class display driver for the Forest target
drivers/ssd1306/        Legacy/reference SSD1306 implementation; not the active Forest target driver
docs/architecture/      Source-coupled architecture and contracts
docs/hardware/          Schematic-derived hardware baselines and validation notes
docs/process/           AI-assisted restart / engineering process records
docs/templates/         Reusable engineering templates
platform/stm32f103/     STM32F103 board support and linker configuration
tests/                  Host-side unit tests
third_party/libopencm3/ Git submodule
tools/                  Reproducible validation and Windows runtime tooling
```

## Architecture boundaries

The project separates **control computation** from **physical actuation authority**.

Control-computation path:

```text
Sensor Acquisition
    -> State Estimation
    -> State Safety
    -> Control State Machine
    -> Selected Controller
    -> Actuator Mapping
    -> Output Safety
    -> computed actuator command
```

System-level actuation/safety path:

```text
Operator Intent
    -> Admission / Mode Management
    -> Continuous Run Permit (planned next split)
    -> Closed-loop command limiting
    -> Motor Authority Arbiter
    -> watchdog / emergency-stop boundary
    -> board_motor
    -> rotary-arm motor
```

The Control State Machine owns **controller-selection and mode-transition authority**. The Motor Authority Arbiter owns **physical actuator command authority**. These responsibilities must not be conflated.

The current automatic path intentionally ends at an **unbound motor sink**. The maintenance path reaches the motor only through the Motor Authority Arbiter.

See [Control Architecture](docs/architecture/control_architecture.md).

## Capability status terminology

A source file or architecture block is not automatically a commissioned capability. Documentation should use these states where relevant:

- **TARGET** — architectural capability intended by the design.
- **STUB** — interface/topology exists but behavior is intentionally incomplete or safe-zero/invalid.
- **IMPLEMENTED** — code exists in the current implementation.
- **HOST-VALIDATED** — deterministic host tests support the implementation.
- **RUNTIME-VALIDATED** — exercised on the current embedded target with recorded evidence.
- **PHYSICALLY-COMMISSIONED** — validated with the real plant and actuator under the required safety procedure.

For example, the presence of LQI, Kalman-estimator, swing-up, or capture source files must not be interpreted as proof that those paths are runtime-validated or physically commissioned.

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

The host suite covers control configuration, safety behavior, estimation, control-state contracts, gate/runtime adapters, motor authority, application adapters, display drivers, and other deterministic logic included by the current CMake configuration.

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
5. Use the current local-key / telemetry behavior to enable the required sensor observation.
6. Run `status`, `param list`, and `transport status` from the UART terminal.
7. Check ADC range, encoder direction, zero offsets, wrapped angle, and timing against the relevant validation issue; do not collapse separate properties into a single pass/fail claim.
8. Lift and mechanically secure the unit so the arm can rotate without contact. Use a current-limited motor supply and keep an immediate power disconnect within reach.
9. With motor power still disconnected, run `motor status`, `motor arm`, and a minimum bounded maintenance test; verify the command automatically returns to stopped/disarmed.
10. Connect motor power only for the explicitly approved maintenance/commissioning procedure. `motor stop` is available through the normal command path; closed-loop commissioning additionally requires the independent emergency-stop work tracked by the roadmap.
11. Increase duty or duration only within the firmware command limits and the specific experiment procedure.

Motor maintenance commands include:

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

`motor arm` remains valid for 30 seconds and authorizes only bounded maintenance operations. `motor status`, test-start responses, telemetry, and automatic-stop messages report nominal `vbus_mV`. The conversion assumes 3.300 V VDDA and the schematic 10 kΩ / 1 kΩ divider, so calibrate it against a trusted meter before using it for protection decisions.

`motor channel d1|d2` selects the output connector used by the next test. Changing the channel first stops both PWM outputs and disarms the test. D1 uses PB0/TIM3_CH3 with PB14/PB15; D2 uses PB1/TIM3_CH4 with PB13/PB12. The default remains D2 to preserve the current maintenance-firmware baseline.

A normal test accepts `-20..-1` or `1..20` percent and `50..10000 ms`. Every completion and explicit stop forces both PWM outputs to zero, sets all four direction pins low, and disarms the interface. This is a software safety layer, not a substitute for current limiting, physical guarding, or a hardware emergency disconnect.

## Development principles

- Keep control logic independent of the MCU platform.
- Test deterministic logic on the host.
- Prefer fail-closed behavior when inputs, configuration, status, or state are invalid.
- Separate controller computation from physical motor actuation authority.
- Preserve explicit provenance for hardware facts and safety parameters.
- Do not promote assumptions, source-file existence, or historical behavior into current validation without evidence.
- Introduce physical automatic output only through explicit, reviewable milestones.
- Keep GitHub as the source of truth for source-coupled code, tests, tools, and architecture; use Google Drive for long-lived plans, evidence, experiments, reports, decision records, dashboards, releases, and datasets.