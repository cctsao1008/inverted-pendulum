# Validation and Evidence Model

The repository separates evidence about the legacy reference product, the current implementation, and physical commissioning. Source-file existence or a plausible assumption is not treated as equivalent to measured behavior.

## Evidence vocabulary

- **LEGACY-VALIDATED** — behavior known to have worked on the legacy reference product or historical firmware; useful as reference, but not proof of the current implementation.
- **DOC** — supported by schematic, datasheet, vendor material, or another controlled document.
- **CODE** — implemented in the current repository.
- **MEASURED** — observed on the current physical specimen.
- **INFERRED** — engineering inference that still requires confirmation.
- **UNKNOWN** — not yet established.

These labels describe the provenance or confidence basis of a statement. They are not a linear maturity scale.

For example:

```text
schematic mapping
    != installed-specimen behavior
    != electrical calibration
    != coordinate convention
    != control-valid state
```

A pendulum input can therefore be `DOC + CODE` at the pin-mapping level while its zero reference, sign convention, wrap behavior, noise, and usable control limits remain separate measured or unknown properties.

## Capability maturity vocabulary

- **TARGET** — architectural capability intended by the design.
- **STUB** — interface/topology exists but behavior is intentionally incomplete or safe-zero/invalid.
- **IMPLEMENTED** — code exists in the current implementation.
- **HOST-VALIDATED** — deterministic host tests support the implementation.
- **RUNTIME-VALIDATED** — exercised on the embedded target with recorded evidence.
- **PHYSICALLY-COMMISSIONED** — validated with the real plant and actuator under the required commissioning procedure.

These labels describe implementation/commissioning maturity rather than evidence source.

## Why the distinction matters

The current reference hardware has a known-working commercial product history. That historical fact prevents the project from being misrepresented as unknown-hardware bring-up, but it does not make the re-engineered firmware or controller automatically correct.

Likewise, control-relevant plant properties are re-measured because the new architecture must not inherit undocumented assumptions about polarity, reference, dead zone, dynamics, saturation, or braking behavior.

```text
known-working product
        !=
validated new control implementation
```

The goal is traceable engineering claims: each important control assumption should carry a clear evidence basis and each capability should carry a clear commissioning state.