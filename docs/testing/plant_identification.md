# Plant Identification Plan

Controller quality depends on measured plant behavior. Identification should precede aggressive controller tuning.

## Initial measurements

- motor dead zone;
- command-to-arm-velocity gain;
- direction asymmetry;
- dominant motor/mechanical time constant;
- friction and stiction;
- encoder scale and sign;
- command-to-measurement delay;
- effective sample timing/jitter.

## Safe step-response sequence

Use mechanically restrained tests and bounded output levels. Candidate command points:

```text
+10%, +20%, +30%, -10%, -20%, -30%
```

Do not promote these exact values to hardware tests without first confirming that the configured limiter and physical fixture make them safe for the installed motor and supply.

## Output

Each identification experiment should produce a traceable dataset and a concise fitted/empirical model with stated uncertainty and applicable hardware conditions.
