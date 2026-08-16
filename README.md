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

## Process documentation

- `docs/process/ai_assisted_embedded_restart_postmortem.md`
- `docs/process/ai_assisted_embedded_project_playbook.md`
- `docs/templates/project_rebaseline_checklist.md`
- `docs/templates/experiment_validation_record_template.md`

These documents define the evidence-driven rebaseline workflow, GitHub/Google Drive source-of-truth split, and reusable project/validation templates.
