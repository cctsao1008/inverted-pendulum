# Firmware Commissioning

This guide covers commissioning of the re-engineered firmware/control stack on the current known-working reference hardware baseline. It is not a generic product bring-up checklist; the purpose is to verify interfaces and control-relevant assumptions required by the new implementation.

## Initial sequence

1. Keep motor power disconnected for initial firmware and sensor checks.
2. Flash the STM32F103 firmware.
3. Confirm status LED and boot messages.
4. Confirm the pendulum input is reported on PA7 / ADC1_IN7.
5. Enable the required sensor observation through the current local-key / telemetry path.
6. Run `status`, `param list`, and `transport status` from the UART terminal.
7. Verify ADC range, encoder direction, zero/reference values, wrapped-angle behavior, and timing against the relevant validation record.
8. Mechanically secure the unit for bounded motor experiments.
9. With motor power still disconnected, verify `motor status`, `motor arm`, and a minimum bounded maintenance command return to stopped/disarmed behavior.
10. Connect motor power only for the specific approved commissioning procedure.
11. Increase duty or duration only within firmware limits and the experiment definition.

## Maintenance commands

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

`motor arm` authorizes only bounded maintenance operations and expires automatically. Completion and explicit stop paths return the motor interface to a safe, disarmed state.

## Motor channel selection

`motor channel d1|d2` selects the output used by the next maintenance test. Changing the channel stops both PWM outputs and disarms the maintenance path.

- D1: PB0 / TIM3_CH3 with PB14 / PB15.
- D2: PB1 / TIM3_CH4 with PB13 / PB12.
- D2 remains the current default.

## Voltage observation

`motor status`, telemetry, and commissioning reports expose nominal `vbus_mV`. The conversion assumes 3.300 V VDDA and the schematic 10 kOhm / 1 kOhm divider. Calibrate it against a trusted meter before using it for protection decisions.

## Ordinary bounded motor test

A normal `motor test` accepts signed duty from 1% to 20% and duration from 50 to 10000 ms. Completion and explicit stop force PWM to zero, direction pins low, and the interface back to disarmed state.

For plant-property measurement, use [Motor Commissioning and Characterization](motor-characterization.md).

## Closed-loop commissioning boundary

Automatic `CONTROL -> motor` binding remains separate from the maintenance path. Closed-loop commissioning requires admission, run-permit, output-limiting, authority, watchdog, and safe-shutdown contracts to be validated before physical actuator authority is enabled.