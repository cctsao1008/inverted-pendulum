# Motor Commissioning and Characterization

This document contains the bounded motor/encoder experiments used to establish control-relevant plant properties for the re-engineered control stack.

The Forest S1 / D1 hardware is a known-working product baseline. These procedures are not basic hardware proof; they measure properties the new controller must know explicitly.

## Command summary

| Command | Purpose |
|---|---|
| `motor identify` | Establish motor / encoder polarity |
| `motor characterize` | Measure breakaway and minimum sustainable duty |
| `motor response` | Measure drive and coast-down response |
| `motor brake-response` | Measure bounded reverse-braking behavior |

## Motor / encoder polarity

```text
motor channel d2
motor arm
motor identify
```

`motor identify` applies a `+5%` pulse for 250 ms, stops for 250 ms, and retries once at `+8%` only when fewer than three encoder counts are observed. It then stops and disarms. The report includes encoder delta, inferred motor / encoder sign, and peak observed encoder velocity.

## Dead-zone characterization

```text
motor arm
motor characterize right
motor arm
motor characterize left
```

The sequence ramps from 5% to 30% in 2% steps, detects two consecutive 250 ms encoder-motion windows, confirms polarity, then ramps down in 1% steps. Each descending step lasts 1.5 seconds.

Reported values include `breakaway_pct`, `minimum_sustain_pct`, `dropout_pct`, encoder sign, and windowed peak velocity.

The firmware currently uses **1040 quadrature counts per output-shaft revolution** as the encoder reference. The rated-speed reference is 9516 counts/s, with 15000 counts/s used as the initial plausibility ceiling.

## Drive and coast-down response

```text
motor arm
motor response right 50 5000
motor arm
motor response left 50 5000
```

Accepted settings are 30% to 80% duty and 1000 to 10000 ms duration. After the drive interval, PWM is set to zero while the encoder is observed for up to 5 seconds. Three consecutive 100 ms windows with no more than one count per window declare the shaft stopped.

The result reports signed drive displacement, cutoff velocity, stopping time, signed coast displacement, and peak windowed velocity. Start with 50% / 5000 ms in both directions, then 70% / 5000 ms. Extend to 10000 ms only when the cutoff velocity shows the geared output was still accelerating.

## Reverse-braking response

```text
motor arm
motor brake-response right 50 5000 10
```

The sequence uses the same drive point followed by a 1 ms neutral guard, bounded opposite PWM, and a 300 ms output-off settling observation. Start at 10% reverse braking before considering 15% or 20%.

Encoder position is accumulated every 1 ms. A 10 ms sliding velocity estimate releases reverse PWM before estimated zero speed; a 40 ms estimate classifies the settling result. The initial release threshold is 600 counts/s and reverse PWM is limited to 300 ms.

Successful or reversal-classified runs emit:

```text
[MOTOR_CSV] brake_response,stop_reason,direction,drive_pct,drive_ms,brake_pct,drive_delta,cutoff_velocity_counts_s,neutral_delta,brake_entry_velocity_counts_s,brake_time_ms,braking_delta,release_velocity_counts_s,settling_delta,final_velocity_counts_s,peak_velocity_counts_s,vbus_mV
```

## Repeated brake-response scripts

```text
script load brake-sweep 50 5000
script list
motor arm
script run
```

`script load brake-sweep <drive_pct> <drive_ms> [brake_pct]` expands to three right / left `motor brake-response` pairs with 5 second waits. Accepted ranges remain 30% to 80% drive duty, 1000 to 10000 ms drive duration, and 10% to 20% brake duty.

Manual recording is also supported through `script begin`, whitelisted `motor brake-response ...` / `wait <ms>` lines, and `script end`. The script refuses `motor arm` internally, keeps PWM at zero during waits, aborts on motor failure or timeout, and clears on reset.

## Interpretation

These measurements are inputs to the new control architecture, not proof that the original Forest hardware was previously non-functional.

```text
known-working product hardware
        !=
known control-model parameters
```

The commissioning procedures replace undocumented assumptions with measured control facts.