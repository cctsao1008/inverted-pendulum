# Rotary Inverted Pendulum

From-scratch embedded firmware and control software for a rotary inverted pendulum.

The project currently targets the original **Forest S1 STM32F103C8T6 controller plus Forest D1 baseboard (2016 revision)** as the hardware baseline. Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

> [!CAUTION]
> The balance controller is not yet connected to physical motor output. Normal `motor test` maintenance commands require explicit arm and remain limited to `20%` and 10 seconds. Characterization may ramp to `30%`; dedicated response/brake characterization commands can use higher explicitly requested duties within their own bounds. These paths automatically stop/disarm on their defined completion or fault conditions. Lift and secure the mechanism, keep clear of the rotating arm, and use a current-limited motor supply.
>
> The firmware now configures the 2016 Forest D1 schematic input at **PA7 / ADC1_IN7**, but the sensor zero, range, direction, wiring, and physical signal are not yet verified. Do not use the ADC reading for control until that physical validation is complete.

## Current status

The `main` branch contains these foundations:

- STM32F103C8T6 bring-up using [libopencm3](https://github.com/libopencm3/libopencm3)
- 1 kHz firmware timing baseline
- Sensor-acquisition framework with the pendulum ADC mapped to PA7 / ADC1_IN7
- Arm quadrature-encoder acquisition
- Five-key local input service for M/X/+/-/USER with debounce, long-press, and repeat events
- SSD1306 128x64 local status UI using bounded software-SPI updates
- USER-key-controlled UART telemetry at 115200 baud and 10 Hz when enabled
- Text UART command interface and RAM-only runtime parameter registry
- Wrap-safe pendulum ADC conversion to `[-pi, pi)`
- Bounded Motor B maintenance test on PB1/PB12/PB13
- VBUS monitoring on PA6 / ADC1_IN6 (nominal 11:1 conversion)
- Control-loop timing profiling output
- Platform-independent control configuration
- Fail-closed safety state machine
- Filtered four-state estimator
- Four-state balance controller using `u = -Kx`
- Platform-independent `control_pipeline` with fail-closed runtime capability validation
- STM32 observe-only control runtime with wrap-safe continuous arm position and control trace
- Host-side tests for control contracts, safety boundaries, runtime configuration, and application adapters

`app/main.c` now feeds real STM32 sensor samples into `control_pipeline_step()` at the real 1 kHz cadence without replaying missed control cycles. The automatic-control motor sink remains intentionally unbound, so the bounded UART maintenance path is still the only code path that can write the physical motor.

The communication architecture keeps text mode for maintenance and reserves Micro XRCE-DDS for a measured feasibility milestone. COBS is intentionally not implemented. See [Communication and Parameter Architecture](docs/architecture/communications.md).

### Next milestone

**V0.7 — runtime observation and motor-authority preparation**

Verify the live 1 kHz control trace, pendulum calibration, continuous arm coordinate, missed-cycle behavior, and sensor timing on hardware. Then define validated state/output safety limits and add an explicit maintenance/control motor-authority boundary before automatic control is connected to the physical motor sink.

The PA7 pendulum ADC signal, sensor zero/range/direction, encoder direction, and motor-interface polarity still require physical verification before closed-loop control is enabled.

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
| Local keys | PA3 M, PA2 X, PA11 +, PA11 +, PA12 -, PA5 USER; active-low |
| Telemetry control | PA5 USER button or `telem on/off`; default off; runtime rate 1–20 Hz |
| OLED | SSD1306 128x64 software SPI: PB5 CLK, PB4 DATA, PB3 RESET, PA15 D/C; SWD retained |
| Motor B control | PB1 / TIM3_CH4 PWM, PB13 / BIN1, PB12 / BIN2 |
| Motor output | Maintenance test only: 20 kHz PWM, `±20%` maximum, 10 s maximum |
