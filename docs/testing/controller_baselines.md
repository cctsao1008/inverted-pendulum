# Controller Baselines

Maintain at least two understandable control baselines before adding advanced estimation or nonlinear techniques.

## PID baseline

Purpose:

- simple commissioning reference;
- intuitive debugging;
- independent comparison against state-space control.

## LQR baseline

Purpose:

- state-space baseline appropriate to the rotary inverted pendulum;
- explicit tradeoff between state error and control effort;
- foundation for later estimator integration.

## Comparison metrics

- capture region;
- settling time;
- overshoot;
- RMS angle error;
- control effort;
- actuator saturation time;
- robustness to initial condition and disturbance;
- fault/safety interaction.

## Estimator order

Begin with the simplest estimator that meets measured requirements. Add Kalman filtering only after sensor noise, bias, derivative quality, sample timing, and baseline controller limitations are measured.
