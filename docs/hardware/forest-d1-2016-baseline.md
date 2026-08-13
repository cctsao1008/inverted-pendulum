# Forest D1 2016 Hardware Baseline

## Scope

This document records the hardware baseline for the original rotary inverted-pendulum kit owned for this project:

- **Forest S1 STM32 minimum-system controller**
- **Forest D1 inverted-pendulum baseboard**
- STM32F103C8T6 controller
- Schematic documents dated March and April 2016

This revision is electrically different from later controller/baseboard versions that use USB Type-C and a CH9102F. Later-version pin maps must not be applied to this board without schematic and physical verification.

## Evidence and confidence

The pin map below is derived from the original Forest S1 and Forest D1 schematics supplied with the purchased kit.

Terms used in this document:

- **Schematic-confirmed**: directly shown by the 2016 schematic.
- **Firmware-observed**: directly present in the current repository source.
- **Pending physical verification**: requires measurement on the actual assembled unit.

A schematic establishes intended connectivity, but it does not establish sensor polarity, harness wire order, assembly substitutions, calibration values, or the condition of the physical board.

## Schematic-confirmed pin map

| Function | STM32F103 pin / peripheral | Notes |
|---|---|---|
| Pendulum-angle sensor | **PA7 / ADC1_IN7** | WDD35D4 potentiometer wiper input |
| Battery-voltage sense | PA6 / ADC1_IN6 | 10 kΩ / 1 kΩ divider; nominal scale ratio 11:1 |
| Arm encoder phase A | PA0 / TIM2_CH1 | A0 on the assembled six-pin connector |
| Arm encoder phase B | PA1 / TIM2_CH2 | A1 on the assembled six-pin connector |
| Motor B PWM | PB1 / TIM3_CH4 | TB6612 PWMB |
| Motor B direction 1 | PB13 | TB6612 BIN1 |
| Motor B direction 2 | PB12 | TB6612 BIN2 |
| USART1 TX | PA9 | Connected through the controller-board USB-UART path |
| USART1 RX | PA10 | Connected through the controller-board USB-UART path |
| Blue user LED | PA4 | Active-low |
| Forest S1 USER key | PA5 | Active-low; toggles UART telemetry in the local UI baseline |
| Forest D1 M key | **PA3** | Active-low; next local UI page |
| Forest D1 X key | PA2 | Active-low; previous/back local UI page |
| Forest D1 + key | PA11 | Active-low; local UI increase/repeat input |
| Forest D1 - key | PA12 | Active-low; local UI decrease/repeat input |
| OLED clock | PB5 | 0.96-inch 6-pin SSD1306 module serial clock |
| OLED data | PB4 | SSD1306 serial data; JTAG function reclaimed |
| OLED reset | PB3 | SSD1306 reset; JTAG function reclaimed |
| OLED D/C | PA15 | SSD1306 data/command; JTAG function reclaimed |
| SWDIO | PA13 | Kept available for debug |
| SWCLK | PA14 | Kept available for debug |

## Clock and USB-UART boundary

The two crystals serve different devices:

| Crystal | Device | Purpose |
|---|---|---|
| 8 MHz | STM32F103C8T6 | MCU HSE; current firmware derives a 72 MHz system clock |
| 12 MHz | CH340G | USB-UART clock |

The 12 MHz crystal must not be used as the STM32 HSE value.

The 2016 Forest S1 controller uses Micro-USB and CH340G. The schematic also shows a DTR/RTS transistor network associated with BOOT0 and reset for automatic download behavior. This differs from the later Type-C / CH9102F revision.

## Pendulum sensor

The supplied WDD35D4 is a three-wire 5 kΩ rotary potentiometer. The available sensor documentation specifies:

- nominal resistance: 5 kΩ
- independent linearity: 0.1%
- effective electrical angle: 345° ± 2°
- continuous mechanical rotation

The Forest D1 schematic supplies the sensor from 3.3 V and routes its wiper to PA7 / ADC1_IN7.

The schematic does not by itself establish the assembled cable's wire order or the sign convention used by the control model. Before control integration, verify:

- which physical wire is the wiper
- ADC raw value at the upright position
- ADC change for positive and negative pendulum rotation
- usable raw range
- behavior near the sensor's electrical dead zone
- safe software travel limits

## Battery-voltage sense

The voltage-sense path is schematic-confirmed as PA6 / ADC1_IN6 with a 10 kΩ / 1 kΩ divider.

The ideal conversion is:

```text
battery_voltage = adc_voltage * 11
```

The actual scale and offset must be calibrated against a trusted meter before this value is used for protection or control decisions. Resistor tolerance, ADC reference accuracy, wiring drop, and board loading contribute error.

The maintenance firmware reports the nominal measurement as `vbus_mV` in
boot/status output and telemetry. It assumes VDDA = 3.300 V and supports a
maximum nominal input of 36.3 V. This is diagnostic telemetry, not yet an
undervoltage or overvoltage safety interlock.

## Encoder

The assembled six-pin motor/encoder connector labels the encoder phases A0 and
A1. The vendor's complete pendulum firmware and standalone encoder test both
initialize PA0/PA1 through TIM2 encoder mode. Physical motor testing separately
confirmed that the motor output itself uses the D2 H-bridge channel; the motor
and encoder therefore do not share the same timer/channel numbering.

Pending physical verification:

- phase A/B order
- positive arm-angle direction
- counts per mechanical revolution
- wrap handling
- whether input pull or filtering changes are required on the assembled board

The current firmware uses TIM2 encoder mode 3, counts both phases (quadrature
x4), and applies the vendor example's input-filter setting of 10. TIM4 is used
as the 1 MHz firmware timebase.

## Motor driver and safety implications

The diagnostic firmware can select either Forest D1 motor connector:

- D1: PB0 / TIM3_CH3 PWM with PB14/PB15 direction
- D2: PB1 / TIM3_CH4 PWM with PB13/PB12 direction
- STBY: tied to 5 V

Because STBY is not MCU-controlled, firmware cannot use it as an independent hardware shutdown. The motor interface must therefore be designed so that startup, reset, invalid state, and safety faults force:

- PWM duty to zero
- direction pins to a defined safe state
- no actuator command until initialization and safety checks complete

The firmware now initializes this interface in a safe stopped state and exposes
only a bounded maintenance test. It does not connect the balance controller to
the motor. Each test requires an explicit arm, is limited to 20% duty and 2 s,
then forces both PWM channels to zero, drives all four direction pins low, and
disarms. Selecting D1 or D2 also stops and disarms before changing the active
channel. The arm latch expires after 30 s if no test is started.

### Logic-level review point

The schematic shows the TB6612 logic supply at 5 V while STM32 GPIO outputs are 3.3 V. A 3.3 V high level is below a conservative `0.7 × VCC = 3.5 V` threshold calculation. The assembled product may operate, but this should be treated as an electrical margin concern and verified against the exact populated driver variant, its datasheet, temperature range, and measured input levels before relying on it for robust operation.

## Firmware mapping status

The firmware now matches the 2016 Forest D1 schematic mapping:

| File | Configured 2016 mapping |
|---|---|
| `platform/stm32f103/board/board_adc.c` | `ADC_CHANNEL7`, `GPIO7` |
| `app/main.c` | boot text reports `PA7 ADC1_IN7` |

This source-level correction does not complete physical validation. PA3 remains the Forest D1 M-button input, and the PA7 telemetry must not be treated as a calibrated pendulum-angle signal until its wiring, zero, range, direction, and electrical limits are measured on the assembled unit.

The 0.96-inch OLED module information supplied with this project identifies the 6-pin display as an SSD1306 128x64 serial module. The controller-board connector maps PB5/PB4/PB3/PA15 to clock/data/reset/D-C. Firmware disables the JTAG-DP function on PA15/PB3/PB4 while retaining SWD on PA13/PA14. Treat the exact populated OLED revision as pending physical confirmation until the display is exercised on the assembled unit.

V0.6 control-pipeline work must not be connected to motor output until the PA7 signal is physically verified.

## Physical validation checklist

Keep motor power disconnected for all steps below.

1. Confirm the actual controller and baseboard markings match the Forest S1 / Forest D1 2016 revision.
2. Confirm 8 MHz at the STM32 HSE circuit and distinguish it from the CH340G 12 MHz crystal.
3. Measure the sensor connector supply and ground before attaching the WDD35D4.
4. Confirm the firmware reports and samples PA7 / ADC1_IN7.
5. Record raw ADC values at upright, representative positive/negative angles, and near both electrical limits.
6. Establish the pendulum zero offset, sign, usable range, and fault thresholds.
7. Rotate the arm manually and record encoder count direction and counts per revolution.
8. Validate PA6 voltage-sense scaling against a trusted meter at more than one voltage.
9. With PWM forced to zero, verify PB1, PB12, and PB13 startup and reset levels.
10. Review the TB6612 3.3 V-to-5 V logic margin on the physical board.
11. Lift and secure the mechanism, apply a current-limited motor supply, and verify the bounded `motor test` command first at 5% for 100 ms.
12. Record positive/negative motor direction, startup/reset output levels, and automatic-stop behavior before connecting any closed-loop controller.

## Revision rule

Any future hardware document or firmware board definition must include an explicit board revision. At minimum, keep these families separate:

- Forest S1 + Forest D1 2016 Micro-USB / CH340G revision
- later Type-C / CH9102F revision
- future RP2350 / Raspberry Pi Pico 2 platform

Do not combine their pin maps into a single implicit default.
