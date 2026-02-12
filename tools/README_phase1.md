# Phase-1 CSV Validation Tool

`tools/analyze_phase1.py` is a first-pass validation and visualization script for HH runs.

It is intended for debugging and invariants, not paper-level figure reproduction.

## Run

```bash
python tools/analyze_phase1.py path/to/run.csv
```

Optional flags:

- `--Cm 1.0` sets membrane capacitance for residual check (default `1.0` uF/cm^2).
- `--show` displays plots interactively in addition to saving PNG files.

Example:

```bash
python tools/analyze_phase1.py out.csv --Cm 1.0
```

## What it prints

- Row count
- `dt` statistics from `diff(t_ms)` (min/mean/max)
- Min/max of `V_mV`
- Min/max of `m`, `h`, `n`
- NaN/Inf counts per required column
- Count of gating values outside `[0,1]` and worst violation value
- Residual stats for membrane-equation consistency:
  - `Iion = INa + IK + IL`
  - `Inet = Iinj - Iion`
  - `dVdt` from finite differences (central interior, forward/backward ends)
  - `residual = Inet - Cm*dVdt`
  - prints residual min/mean/max/RMS

## Generated plots

Plots are saved to a `plots/` directory beside the CSV file:

- `phase1_voltage_vs_time.png`
- `phase1_gating_vs_time.png`
- `phase1_currents_vs_time.png`
- `phase1_residual_vs_time.png`
- `phase1_residual_histogram.png` (if residual has finite values)

## Plot meaning and "good" behavior

- Voltage vs time:
  - At rest (`Iinj=0`), `V_mV` should remain near a stable baseline (small drift/noise only).
- Gating (`m,h,n`) vs time:
  - Values should generally remain in `[0,1]`.
  - Large excursions outside `[0,1]` suggest integration instability or model bugs.
- Currents + `Inet`:
  - Current traces should be finite and smooth for a reasonable `dt`.
- Residual vs time:
  - Should stay near zero when dynamics and units are internally consistent.
  - Persistent large bias or spikes indicate possible equation/sign/unit/derivative issues.
- Residual histogram:
  - Should be centered near zero with narrow spread in stable runs.

## Notes

- This is Phase 1 sanity checking only.
- Use this before trying to match canonical HH spike plots.
