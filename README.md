# Rotary Inverted Pendulum

Re-engineered embedded firmware and control software for the existing **Forest D1 rotary inverted pendulum**, using its original **Forest S1 STM32 controller module**.

> **A known-working plant is not the same thing as a known control system.**

This project is not primarily about proving that an STM32 can balance an inverted pendulum. The historical Forest mechanism already demonstrated that the physical plant can work. The purpose of this repository is to rebuild that system into a **measurable, testable, safety-gated, and progressively commissionable embedded control platform**.

The current hardware baseline is the original **Forest D1 baseboard and mechanism with its Forest S1 STM32F103C8T6 controller module (2016 revision)**. Hardware facts, sensor calibration, plant behavior, control computation, and physical actuator authority are treated as separate engineering properties and are validated independently.

Platform-independent control modules are developed and tested on the host before they are connected to real motor output.

## Design principles

1. **Measurement before control**  
   Hardware mappings, polarity, calibration, timing, dead zones, and plant response are established independently before closed-loop control is allowed to depend on them.

2. **Control computation is not actuator authority**  
   A valid controller output does not automatically grant permission to drive the physical motor.

3. **Fail closed**  
   Missing, stale, invalid, unqualified, or faulted state must reduce authority rather than preserve the last actuator command.

4. **Progressive commissioning**  
   The new control stack proceeds from passive observation to bounded maintenance actuation, characterization, observe-only control, admission, authority, and only then physical closed-loop control.

5. **Control logic remains platform-independent where practical**  
   Estimation, safety, configuration, and controller logic are kept separate from STM32-specific I/O so deterministic behavior can be tested on the host.

6. **Unknown is an acceptable engineering state; assumption is not evidence**  
   Historical behavior, source-file existence, or a plausible inference is not promoted into a current validation claim without supporting evidence.

7. **Safety is part of the control architecture**  
   Admission, watchdog behavior, motor ownership, output qualification, and safe loss of authority are architectural concerns rather than afterthoughts around the controller.

## System architecture

The system separates three questions that are often conflated in small embedded-control projects:

```text
STATE ESTIMATION
"What is the plant doing?"
        |
        v
CONTROL REGIME / COMPUTATION
"What control strategy applies, and what command should it produce?"
        |
        v
AUTHORITY & SAFETY
"May that command physically reach the motor?"
```

### State-estimation plane

```text
Physical Sensors
    -> Signal Acquisition
    -> Calibration / Wrapping
    -> Filtering / Estimation
    -> State Validity / Safety
    -> validated plant state
```

The control state is based on pendulum and rotary-arm measurements. Kalman-estimator work exists in the architecture, while estimator source code, host validation, runtime validation, and physical commissioning remain distinct capability states.

### Control plane

The rotary inverted pendulum is treated as a **mode-dependent / hybrid control problem**, not as one controller expected to work over the entire state space.

```text
pendulum hanging / low-energy state
                |
                v
       ENERGY-BASED SWING-UP
        inject / remove energy
                |
                v
          capture region
                |
                v
             CAPTURE
       transition management
                |
                v
       upright neighborhood
          /      |      \
         v       v       v
       PD/PID    LQR     LQI
          \      |      /
                v
             BALANCE
```

Energy-based swing-up addresses the large-angle nonlinear problem of bringing the pendulum toward the energy required for the upright equilibrium. Capture logic manages the transition into the local stabilizing region. PD/PID, LQR, and integral-augmented LQI belong to the upright stabilization side of the architecture.

Controller availability and controller commissioning are intentionally separate concepts. The presence of LQI, Kalman-estimator, swing-up, or capture source files does **not** mean those paths are runtime-validated or physically commissioned.

Controllers operate behind a common state, safety, and actuator interface. A control mode can select a policy and compute an actuator request without gaining direct access to the motor.

### Authority and actuation plane

```text
Operator Intent
    -> Admission / Mode Management
    -> Continuous Run Permit
    -> Closed-loop Command Qualification
    -> Motor Authority Arbiter
    -> Watchdog / Safe-Shutdown Boundary
    -> board_motor
    -> rotary-arm motor
```

The **Control State Machine** owns controller-selection and mode-transition authority. The **Motor Authority Arbiter** owns physical actuator command authority. These responsibilities must not be conflated.

Motor authority is treated as an explicitly owned resource with `NONE / MAINTENANCE / CONTROL / FAULT` semantics. The current automatic path intentionally ends at an **unbound motor sink**; the bounded maintenance path is currently the only software path that can reach the physical motor.

### Safe-state contract

Loss of valid control authority must converge toward a defined motor-safe condition rather than preserving the previous actuator command.

Safe loss of authority may result from operator disable, invalid state, stale control output, runtime fault, authority conflict, watchdog expiration, or an independent emergency-stop path. Shutdown policy is enforced at the actuator-authority boundary so swing-up, capture, PID, LQR, and LQI share the same safety semantics.

The exact coast / brake / standby behavior belongs to the actuator and hardware contract rather than the controller itself.

See [Control Architecture](docs/architecture/control_architecture.md).

## Commissioning philosophy

The project intentionally avoids jumping directly from “the sensor moves” to “run the controller.”

```text
Observe
  -> Measure
  -> Characterize
  -> Identify
  -> Estimate
  -> Compute
  -> Admit
  -> Authorize
  -> Actuate
  -> Balance
```

At the plant/control boundary:

```text
sensor / actuator interface verification
        -> motor and encoder polarity
        -> dead-zone / response characterization
        -> plant identification
        -> state-estimation validation
        -> observe-only controller execution
        -> closed-loop admission proof
        -> bounded actuator authority
        -> restrained physical balance commissioning
```

**Balancing is a commissioning milestone, not the architecture.** A successful balance result is meaningful only when the assumptions, state validity, authority path, and safety behavior leading to it are traceable.

## Physical platform

Only hardware properties that directly shape the control architecture are summarized here:

- **Controller module:** Forest S1 with STM32F103C8T6 at 72 MHz, using bare-metal libopencm3.
- **Plant:** Forest D1 rotary inverted pendulum, 2016 hardware baseline.
- **Pendulum sensing:** analog angle sensor on **PA7 / ADC1_IN7**.
- **Rotary-arm sensing:** quadrature encoder; current firmware reference is **1040 counts per output-shaft revolution**, pending final specimen-level control validation.
- **Actuation:** TB6612FNG H-bridge driving the rotary-arm DC motor.
- **Motor / gearing:** nominal 12 V DC motor with 1:20 gearbox on the original Forest mechanism.
- **Control timing:** 1 kHz firmware timing baseline.

The Forest S1 controller module remains the immediate target because it plugs directly into the existing Forest D1 baseboard. Pin-level wiring, user-interface connections, OLED signals, maintenance UART details, electrical notes, and validation checklists are kept in [Forest D1 2016 hardware baseline](docs/hardware/forest-d1-2016-baseline.md).

A **Raspberry Pi Pico 2 / RP2350** port, including native USB HID and CDC, is a planned later platform and is not implemented on `main`.

## Current implementation state

The original **Forest D1 hardware with its Forest S1 controller module** is treated as a **legacy-validated, known-working physical baseline**. Current development therefore focuses on replacing and restructuring the firmware and control stack, not on re-proving the product hardware.

Control-relevant properties are still measured where the new architecture depends on them. Motor/encoder polarity, sensor reference and sign, dead zone, response dynamics, saturation, and braking behavior are control-model facts rather than basic hardware bring-up questions.

Current software foundations include:

- platform-independent state-estimation and control pipeline
- fail-closed state and actuator-authority architecture
- observe-only closed-loop runtime
- explicit closed-loop admission and Motor Authority Arbiter
- bounded maintenance and plant-characterization path
- host-side deterministic validation of control and safety contracts

Automatic `CONTROL -> motor` binding remains intentionally disabled while the remaining closed-loop boundaries are commissioned.

```text
control architecture             IMPLEMENTED
observe-only runtime             IMPLEMENTED
authority / admission model      IMPLEMENTED
safe-shutdown boundary           IN PROGRESS
plant identification             PLANNED
LQR / LQI commissioning          PLANNED
energy swing-up / capture        PLANNED
physical closed-loop balance     BLOCKED
```

The hardware is legacy-validated as a functioning product; the new control implementation is commissioned independently so undocumented assumptions are not inherited as control facts.

## Validation model

README uses two compact vocabularies:

```text
Evidence:
LEGACY-VALIDATED / DOC / CODE / MEASURED / INFERRED / UNKNOWN

Capability maturity:
TARGET / STUB / IMPLEMENTED / HOST-VALIDATED /
RUNTIME-VALIDATED / PHYSICALLY-COMMISSIONED
```

Evidence describes **what supports a claim**. Capability maturity describes **how far an implementation has been validated or commissioned**.

See [Validation and Evidence Model](docs/validation/evidence-model.md).

## Communication and ROS 2 integration

Communication remains outside the control core.

```text
                 Control / Estimation Core
                          |
                 native application state
                          |
              +-----------+-----------+
              |                       |
              v                       v
      Text maintenance         Micro XRCE-DDS
       bring-up / debug        ROS 2 / DDS path
```

The current target uses a text UART maintenance interface for commissioning, bounded motor commands, low-rate telemetry, and RAM-only runtime parameter changes. **Micro XRCE-DDS** is reserved as a measured feasibility milestone for structured higher-rate telemetry and ROS 2 / DDS integration; it is not part of the current firmware image.

The first XRCE-DDS scope is intended to remain low risk: telemetry and disarmed-only parameter access. Remote motor arm is explicitly excluded from the initial integration. **ROS 2 connectivity must not implicitly confer actuator authority.**

See [Communication and Parameter Architecture](docs/architecture/communications.md).

## Commissioning tools

The firmware provides bounded tools for establishing the plant properties required by the new control stack:

| Tool | Purpose |
|---|---|
| `motor identify` | motor / encoder polarity |
| `motor characterize` | breakaway and sustainable duty |
| `motor response` | drive and coast-down dynamics |
| `motor brake-response` | reverse-braking dynamics |

Detailed thresholds, timing, CSV output, and scripting are kept in [Motor Commissioning and Characterization](docs/commissioning/motor-characterization.md).

The initial firmware/control commissioning sequence is documented in [Firmware Commissioning](docs/commissioning/firmware-commissioning.md).

## Repository layout

```text
app/                    STM32 firmware application and system-level adapters
cmake/                  Cross-compilation toolchain
control/                Platform-independent estimator, safety, and control logic
drivers/ssd1315/        Active SSD1315-class display driver for the Forest target
drivers/ssd1306/        Legacy/reference SSD1306 implementation
docs/architecture/      Source-coupled architecture and contracts
docs/commissioning/     Firmware and plant commissioning procedures
docs/hardware/          Schematic-derived hardware baselines and validation notes
docs/process/           Engineering process records
docs/validation/        Evidence and capability-validation model
docs/templates/         Reusable engineering templates
platform/stm32f103/     STM32F103 board support and linker configuration
tests/                  Host-side unit tests
third_party/libopencm3/ Git submodule
tools/                  Reproducible validation and runtime tooling
```

## Build and test

Clone with the libopencm3 submodule:

```bash
git clone --recursive https://github.com/cctsao1008/inverted-pendulum.git
cd inverted-pendulum
```

For an existing clone:

```bash
git submodule update --init --recursive
```

### Host tests

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

### STM32F103 firmware

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

## Documentation

- [Control Architecture](docs/architecture/control_architecture.md)
- [Communication and Parameter Architecture](docs/architecture/communications.md)
- [Forest D1 2016 Hardware Baseline](docs/hardware/forest-d1-2016-baseline.md)
- [Firmware Commissioning](docs/commissioning/firmware-commissioning.md)
- [Motor Commissioning and Characterization](docs/commissioning/motor-characterization.md)
- [Validation and Evidence Model](docs/validation/evidence-model.md)
