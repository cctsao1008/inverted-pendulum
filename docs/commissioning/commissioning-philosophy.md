# Commissioning Philosophy

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
