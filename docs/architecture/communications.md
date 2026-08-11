# Communication and Parameter Architecture

## Decision

The firmware uses two communication roles:

1. **Text maintenance mode** for bring-up, recovery, manual commands,
   low-rate telemetry, and runtime parameter changes.
2. **Micro XRCE-DDS operational mode** as a future feasibility milestone for
   structured high-rate telemetry and ROS 2 / DDS integration.

COBS is not implemented. XRCE-DDS serial transport already provides its own
framing, reliability mechanisms, and Agent integration. Maintaining a second
custom binary protocol would duplicate code, host tooling, and validation on
the resource-constrained STM32F103C8T6.

The control and sensor modules must remain independent of either transport.
They expose native application state and parameter data; a transport backend
adapts that data without introducing DDS types into the control core.

## Current maintenance mode

The current firmware always boots with:

- motor output uninitialized;
- text transport active;
- telemetry disabled;
- runtime parameters restored to compiled defaults.

Supported commands:

```text
help
status

telem status
telem on
telem off
telem rate <1..20>

param list
param get <name>
param set <name> <value>
param defaults

transport status
```

`param set` changes only the active RAM value. `param save` and `param load`
deliberately return an error until versioned, CRC-protected Flash storage is
implemented. Motor arm state and telemetry enable state must never be saved.

The PA3 M button and text commands update the same telemetry state. The last
valid operation wins. Text telemetry is capped at 20 Hz because the current
USART1 transmit path is blocking and the firmware is still in sensor-only
bring-up.

## Runtime parameters

The initial registry contains:

| Name | Default | Range | Meaning |
|---|---:|---:|---|
| `sensor.pendulum.upright_adc` | 2928 | 0..4095 | Provisional upright zero inferred from the measured natural-down point |
| `sensor.pendulum.direction` | 1 | -1 or 1 | `1` means increasing ADC is positive angle |
| `telem.rate_hz` | 10 | 1..20 | Text telemetry rate |

The upright default is provisional and must remain tunable until it is checked
against a mechanically referenced vertical-up position.

The pendulum conversion first wraps the ADC delta to `[-2048, 2047]` and only
then scales it to `[-pi, pi)`. Direct subtraction across `4095 <-> 0` is not
valid.

## XRCE-DDS feasibility gate

XRCE-DDS is a roadmap item, not part of the current firmware image. A future
isolated feasibility change must measure:

- Flash and static / peak RAM use;
- serial bandwidth at the intended publish rates;
- control-loop jitter at 100 Hz and 500 Hz publication;
- Agent disconnect and reconnect behavior;
- best-effort state stream and reliable command buffer costs;
- complete suppression of raw `printf` output while XRCE serial mode is active.

The first XRCE-DDS scope should be telemetry and low-risk, disarmed-only
parameter access. Remote motor arm is excluded from the first integration.

If the STM32F103 resource or timing budget is insufficient, text maintenance
mode remains available and XRCE-DDS moves to the planned RP2350 or a larger
STM32 target. COBS should only be reconsidered if there is a demonstrated need
for a lightweight Agent-free binary logger.
