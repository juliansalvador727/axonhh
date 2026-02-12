# Phase 2: Threshold Experiment Harness

`tools/analyze_phase2.py` automates repeated HH simulator runs across stimulus amplitudes to estimate spike threshold and summarize behavior.

Phase 2 focus:

- protocol-style amplitude scans
- spike/no-spike classification
- threshold estimation (sweep or binary search)
- summary plots and `summary.csv`

## Spike definition

A run is classified as a spike when:

- `max(V_mV) >= spike_vth_mV`

(`spike_vth_mV` defaults to `0` unless specified.)

## Basic usage

```bash
python tools/analyze_phase2.py \
  --exe .build/axonhh \
  --base_config configs/base.cfg \
  --out_dir runs/phase2 \
  --stim_type pulse \
  --t0_ms 5 --t1_ms 6 \
  --T_ms 60 --dt_ms 0.01 \
  --amin 0 --amax 20 --astep 0.5 \
  --threshold_mode binary_search \
  --threshold_tol 0.1 \
  --spike_vth_mV 0
```

## Sweep mode

Runs every amplitude in `[amin, amax]` with step `astep` and chooses the smallest spiking amplitude.

```bash
python tools/analyze_phase2.py \
  --exe ./axonhh \
  --base_config configs/base.toml \
  --out_dir runs/phase2_sweep \
  --stim_type step \
  --t0_ms 10 --t1_ms 40 \
  --amin 0 --amax 20 --astep 0.5 \
  --threshold_mode sweep
```

## Binary search mode

Uses spike/no-spike bracketing within `[amin, amax]` until interval width is `<= threshold_tol`.

- faster when spike behavior is monotonic with amplitude
- if monotonicity appears violated, script falls back to sweep and warns

```bash
python tools/analyze_phase2.py \
  --exe ./axonhh \
  --base_config configs/base.toml \
  --out_dir runs/phase2_binary \
  --stim_type pulse \
  --t0_ms 5 --t1_ms 6 \
  --amin 0 --amax 20 --astep 0.5 \
  --threshold_mode binary_search \
  --threshold_tol 0.1
```

## Output layout

All outputs go under `--out_dir`:

- `configs/` per-run generated config files
- `csv/` per-run simulator CSV outputs
- `summary.csv` run-level metrics table (amp, spiked, peak V, etc.)
- `threshold.txt` threshold estimate and mode used
- `plots/`:
  - `phase2_spike_vs_amplitude.png`
  - `phase2_peakV_vs_amplitude.png`
  - `phase2_gallery_voltage_traces.png`

## Notes on config handling

- The script never edits your base config in-place.
- It writes run-specific config files in key-value form expected by `axonhh`.
- `--base_config` can be key-value or TOML-like; known keys are normalized to simulator keys.

## Performance

- Use `--jobs N` (default `1`) for concurrent sweep runs.
- Binary search runs are mostly sequential by design.

## What to do next

After obtaining a baseline threshold, repeat Phase 2 with different integrators (for example Euler vs RK4) by changing integrator setting in the base config and comparing:

- threshold estimate
- spike/no-spike transition curve
- peak voltage and trace shape near threshold
