#!/usr/bin/env python3
"""Phase-2 HH experiment harness: amplitude sweeps, spike detection, threshold estimation."""

from __future__ import annotations

import argparse
import concurrent.futures
import math
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np

REQUIRED_COLUMNS = [
    "t_ms",
    "V_mV",
    "m",
    "h",
    "n",
    "Iinj_uAcm2",
    "INa_uAcm2",
    "IK_uAcm2",
    "IL_uAcm2",
]

CONFIG_KEYS_IN_ORDER = [
    "dt_ms",
    "T_ms",
    "V0_mV",
    "integrator",
    "output.csv_path",
    "stimulus.kind",
    "stimulus.amp_uA_cm2",
    "stimulus.t0_ms",
    "stimulus.t1_ms",
    "stimulus.period_ms",
    "stimulus.duty",
    "params.C_m_uF_cm2",
    "params.gNa_bar_mS_cm2",
    "params.gK_bar_mS_cm2",
    "params.gL_bar_mS_cm2",
    "params.ENa_mV",
    "params.EK_mV",
    "params.EL_mV",
]

DEFAULT_CONFIG: dict[str, str] = {
    "dt_ms": "0.01",
    "T_ms": "60",
    "V0_mV": "-65",
    "integrator": "rk4",
    "output.csv_path": "out.csv",
    "stimulus.kind": "step",
    "stimulus.amp_uA_cm2": "10",
    "stimulus.t0_ms": "10",
    "stimulus.t1_ms": "40",
    "stimulus.period_ms": "0",
    "stimulus.duty": "0",
    "params.C_m_uF_cm2": "1",
    "params.gNa_bar_mS_cm2": "120",
    "params.gK_bar_mS_cm2": "36",
    "params.gL_bar_mS_cm2": "0.3",
    "params.ENa_mV": "50",
    "params.EK_mV": "-77",
    "params.EL_mV": "-54.387",
}

ALIASES = {
    "stimulus.amp_uAcm2": "stimulus.amp_uA_cm2",
    "stimulus.amp": "stimulus.amp_uA_cm2",
    "stimulus.t0": "stimulus.t0_ms",
    "stimulus.t1": "stimulus.t1_ms",
    "stimulus.period": "stimulus.period_ms",
    "output.csv": "output.csv_path",
    "output.path": "output.csv_path",
    "params.Cm": "params.C_m_uF_cm2",
}


@dataclass
class Phase2Args:
    exe: Path
    base_config: Path
    out_dir: Path
    stim_type: str
    t0_ms: float
    t1_ms: float
    T_ms: float | None
    dt_ms: float | None
    amin: float
    amax: float
    astep: float
    threshold_mode: str
    threshold_tol: float
    spike_vth_mV: float
    jobs: int
    max_binary_iters: int
    pulse_period_ms: float | None
    pulse_duty: float | None


@dataclass
class RunResult:
    amp_uAcm2: float
    spiked: int
    peak_V_mV: float
    t_peak_ms: float
    min_V_mV: float
    max_INa_uAcm2: float  # most negative INa (minimum value)
    max_INa_abs_uAcm2: float
    max_IK_uAcm2: float
    csv_path: Path
    config_path: Path


def parse_args() -> Phase2Args:
    parser = argparse.ArgumentParser(description="Phase-2 HH experiment harness")
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--base_config", type=Path, required=True)
    parser.add_argument("--out_dir", type=Path, required=True)

    parser.add_argument("--stim_type", choices=["step", "pulse"], required=True)
    parser.add_argument("--t0_ms", type=float, required=True)
    parser.add_argument("--t1_ms", type=float, required=True)
    parser.add_argument("--T_ms", type=float, default=None)
    parser.add_argument("--dt_ms", type=float, default=None)

    parser.add_argument("--amin", type=float, required=True)
    parser.add_argument("--amax", type=float, required=True)
    parser.add_argument("--astep", type=float, required=True)

    parser.add_argument("--threshold_mode", choices=["sweep", "binary_search"], default="binary_search")
    parser.add_argument("--threshold_tol", type=float, default=0.1)
    parser.add_argument("--spike_vth_mV", type=float, default=0.0)

    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--max_binary_iters", type=int, default=40)

    parser.add_argument(
        "--pulse_period_ms",
        type=float,
        default=None,
        help="Optional pulse period. If omitted and stim_type=pulse, uses (t1_ms - t0_ms).",
    )
    parser.add_argument(
        "--pulse_duty",
        type=float,
        default=None,
        help="Optional pulse duty [0,1]. If omitted and stim_type=pulse, uses 1.0.",
    )

    ns = parser.parse_args()

    args = Phase2Args(**vars(ns))
    validate_args(args)
    return args


def validate_args(args: Phase2Args) -> None:
    if args.t1_ms < args.t0_ms:
        raise ValueError("t1_ms must be >= t0_ms")
    if args.astep <= 0:
        raise ValueError("astep must be > 0")
    if args.amax < args.amin:
        raise ValueError("amax must be >= amin")
    if args.threshold_tol <= 0:
        raise ValueError("threshold_tol must be > 0")
    if args.jobs < 1:
        raise ValueError("jobs must be >= 1")
    if args.max_binary_iters < 1:
        raise ValueError("max_binary_iters must be >= 1")
    if args.dt_ms is not None and args.dt_ms <= 0:
        raise ValueError("dt_ms must be > 0")
    if args.T_ms is not None and args.T_ms <= 0:
        raise ValueError("T_ms must be > 0")
    if args.stim_type == "pulse":
        if args.pulse_period_ms is not None and args.pulse_period_ms <= 0:
            raise ValueError("pulse_period_ms must be > 0")
        if args.pulse_duty is not None and not (0.0 <= args.pulse_duty <= 1.0):
            raise ValueError("pulse_duty must be in [0,1]")


def normalize_key(key: str) -> str:
    k = key.strip()
    if k in ALIASES:
        return ALIASES[k]
    return k


def load_base_config(path: Path) -> dict[str, str]:
    if not path.exists():
        raise FileNotFoundError(f"base_config does not exist: {path}")

    ext = path.suffix.lower()
    if ext == ".toml":
        cfg = parse_toml_config(path)
    elif ext in {".ini", ".cfg"}:
        cfg = parse_ini_config(path)
    else:
        cfg = parse_key_value_config(path)

    known: dict[str, str] = {}
    unknown: list[str] = []
    for k, v in cfg.items():
        nk = normalize_key(k)
        if nk in CONFIG_KEYS_IN_ORDER:
            known[nk] = str(v)
        else:
            unknown.append(k)

    if unknown:
        print(
            "warning: ignoring unknown keys from base config: " + ", ".join(sorted(set(unknown))),
            file=sys.stderr,
        )

    return known


def parse_key_value_config(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        if "=" not in line:
            raise ValueError(f"base_config parse error on line {line_no}: missing '='")
        k, v = line.split("=", 1)
        key = k.strip()
        value = v.strip().strip('"').strip("'")
        if not key:
            raise ValueError(f"base_config parse error on line {line_no}: empty key")
        if value == "":
            raise ValueError(f"base_config parse error on line {line_no}: empty value")
        out[key] = value
    return out


def parse_toml_config(path: Path) -> dict[str, str]:
    try:
        import tomllib
    except ImportError as exc:
        raise RuntimeError("Python 3.11+ is required for TOML parsing (tomllib)") from exc

    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except tomllib.TOMLDecodeError as exc:
        raise ValueError(f"Invalid TOML in base_config: {exc}") from exc

    flat: dict[str, str] = {}

    def _flatten(prefix: str, obj: Any) -> None:
        if isinstance(obj, dict):
            for key, value in obj.items():
                next_prefix = f"{prefix}.{key}" if prefix else key
                _flatten(next_prefix, value)
            return

        if isinstance(obj, bool):
            flat[prefix] = "1" if obj else "0"
        else:
            flat[prefix] = str(obj)

    _flatten("", data)
    return flat


def parse_ini_config(path: Path) -> dict[str, str]:
    import configparser

    text = path.read_text(encoding="utf-8")
    cp = configparser.ConfigParser(interpolation=None)
    cp.optionxform = str
    try:
        cp.read_string(text)
    except configparser.MissingSectionHeaderError:
        # Accept legacy plain key=value config under .ini/.cfg names.
        return parse_key_value_config(path)

    out: dict[str, str] = {}
    for k, v in cp.defaults().items():
        out[k] = v
    for section in cp.sections():
        for k, v in cp.items(section):
            out[f"{section}.{k}"] = v
    return out


def amp_to_key(amp: float) -> float:
    return round(float(amp), 12)


def amp_to_tag(amp: float) -> str:
    key = amp_to_key(amp)
    s = f"{key:.12f}".rstrip("0").rstrip(".")
    s = s.replace("-", "m").replace(".", "p")
    if not s:
        s = "0"
    return s


def make_amplitude_grid(amin: float, amax: float, astep: float) -> list[float]:
    span = max(0.0, amax - amin)
    n = int(math.floor(span / astep + 1e-12)) + 1
    amps = [amin + i * astep for i in range(n)]
    if amps[-1] < amax - 1e-12:
        amps.append(amax)
    return [amp_to_key(a) for a in amps]


def write_config_for_amp(
    base_cfg: dict[str, str],
    amp_uAcm2: float,
    args: Phase2Args,
    config_path: Path,
    csv_path: Path,
) -> None:
    cfg = dict(DEFAULT_CONFIG)
    cfg.update(base_cfg)

    cfg["output.csv_path"] = str(csv_path)
    cfg["stimulus.kind"] = args.stim_type
    cfg["stimulus.amp_uA_cm2"] = str(amp_uAcm2)
    cfg["stimulus.t0_ms"] = str(args.t0_ms)
    cfg["stimulus.t1_ms"] = str(args.t1_ms)

    if args.dt_ms is not None:
        cfg["dt_ms"] = str(args.dt_ms)
    if args.T_ms is not None:
        cfg["T_ms"] = str(args.T_ms)

    if args.stim_type == "pulse":
        period = args.pulse_period_ms if args.pulse_period_ms is not None else max(1e-9, args.t1_ms - args.t0_ms)
        duty = args.pulse_duty if args.pulse_duty is not None else 1.0
        cfg["stimulus.period_ms"] = str(period)
        cfg["stimulus.duty"] = str(duty)

    lines = [f"{k}={cfg[k]}" for k in CONFIG_KEYS_IN_ORDER]
    config_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def run_sim(exe: Path, config_path: Path, workdir: Path) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(
        [str(exe), str(config_path)],
        cwd=str(workdir),
        text=True,
        capture_output=True,
    )
    if proc.returncode != 0:
        msg = (
            f"Simulator failed for config: {config_path}\n"
            f"exit_code={proc.returncode}\n"
            f"stdout:\n{proc.stdout}\n"
            f"stderr:\n{proc.stderr}"
        )
        raise RuntimeError(msg)
    return proc


def load_run_csv(csv_path: Path):
    try:
        import pandas as pd
    except ImportError as exc:
        raise RuntimeError("Missing dependency: pandas. Install with: pip install pandas matplotlib numpy") from exc

    if not csv_path.exists():
        raise FileNotFoundError(f"run CSV not found: {csv_path}")

    try:
        df = pd.read_csv(csv_path)
    except pd.errors.EmptyDataError as exc:
        raise ValueError(f"run CSV is empty: {csv_path}") from exc
    except pd.errors.ParserError as exc:
        raise ValueError(f"malformed run CSV ({csv_path}): {exc}") from exc

    missing = [c for c in REQUIRED_COLUMNS if c not in df.columns]
    if missing:
        raise ValueError(f"run CSV missing required columns ({csv_path}): {', '.join(missing)}")

    out = df.loc[:, REQUIRED_COLUMNS].copy()
    for col in REQUIRED_COLUMNS:
        out[col] = pd.to_numeric(out[col], errors="coerce")

    return out


def classify_spike(df, spike_vth_mV: float) -> dict[str, float | int]:
    v = df["V_mV"].to_numpy(dtype=float)
    t = df["t_ms"].to_numpy(dtype=float)
    ina = df["INa_uAcm2"].to_numpy(dtype=float)
    ik = df["IK_uAcm2"].to_numpy(dtype=float)

    finite_v = np.isfinite(v)
    if not np.any(finite_v):
        raise ValueError("V_mV contains no finite values")

    peak_idx = int(np.nanargmax(v))
    peak_v = float(v[peak_idx])
    t_peak = float(t[peak_idx]) if np.isfinite(t[peak_idx]) else float("nan")

    min_v = float(np.nanmin(v))
    min_ina = float(np.nanmin(ina))
    max_ina_abs = float(np.nanmax(np.abs(ina)))
    max_ik = float(np.nanmax(ik))

    spiked = int(peak_v >= spike_vth_mV)

    return {
        "spiked": spiked,
        "peak_V_mV": peak_v,
        "t_peak_ms": t_peak,
        "min_V_mV": min_v,
        "max_INa_uAcm2": min_ina,
        "max_INa_abs_uAcm2": max_ina_abs,
        "max_IK_uAcm2": max_ik,
    }


def run_single_amp(
    amp: float,
    args: Phase2Args,
    base_cfg: dict[str, str],
    run_root: Path,
    workdir: Path,
) -> RunResult:
    tag = amp_to_tag(amp)
    config_path = run_root / "configs" / f"run_amp_{tag}.cfg"
    csv_path = run_root / "csv" / f"run_amp_{tag}.csv"

    write_config_for_amp(base_cfg, amp, args, config_path=config_path, csv_path=csv_path)
    run_sim(args.exe, config_path=config_path, workdir=workdir)
    df = load_run_csv(csv_path)
    info = classify_spike(df, args.spike_vth_mV)

    return RunResult(
        amp_uAcm2=amp,
        spiked=int(info["spiked"]),
        peak_V_mV=float(info["peak_V_mV"]),
        t_peak_ms=float(info["t_peak_ms"]),
        min_V_mV=float(info["min_V_mV"]),
        max_INa_uAcm2=float(info["max_INa_uAcm2"]),
        max_INa_abs_uAcm2=float(info["max_INa_abs_uAcm2"]),
        max_IK_uAcm2=float(info["max_IK_uAcm2"]),
        csv_path=csv_path,
        config_path=config_path,
    )


def monotonic_spike_pattern(results: dict[float, RunResult]) -> bool:
    ordered = sorted(results.items(), key=lambda kv: kv[0])
    saw_spike = False
    for _, res in ordered:
        if res.spiked:
            saw_spike = True
        elif saw_spike and not res.spiked:
            return False
    return True


def run_many_amps(
    amps: list[float],
    args: Phase2Args,
    base_cfg: dict[str, str],
    run_root: Path,
    workdir: Path,
    cache: dict[float, RunResult],
) -> None:
    need = [amp_to_key(a) for a in amps if amp_to_key(a) not in cache]
    if not need:
        return

    print(f"running {len(need)} amplitudes (jobs={args.jobs})")

    if args.jobs == 1 or len(need) == 1:
        for amp in need:
            print(f"  amp={amp:.6g}")
            cache[amp] = run_single_amp(amp, args, base_cfg, run_root, workdir)
        return

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        futures = {
            ex.submit(run_single_amp, amp, args, base_cfg, run_root, workdir): amp
            for amp in need
        }
        for fut in concurrent.futures.as_completed(futures):
            amp = futures[fut]
            try:
                cache[amp] = fut.result()
                print(f"  amp={amp:.6g} done")
            except Exception as exc:
                for f in futures:
                    f.cancel()
                raise RuntimeError(f"failed at amp={amp:.6g}: {exc}") from exc


def sweep_threshold_from_cache(amps: list[float], cache: dict[float, RunResult]) -> float | None:
    for amp in sorted(amps):
        if cache[amp_to_key(amp)].spiked:
            return amp_to_key(amp)
    return None


def estimate_threshold(
    args: Phase2Args,
    sweep_amps: list[float],
    base_cfg: dict[str, str],
    run_root: Path,
    workdir: Path,
    cache: dict[float, RunResult],
) -> tuple[float | None, str]:
    if args.threshold_mode == "sweep":
        run_many_amps(sweep_amps, args, base_cfg, run_root, workdir, cache)
        return sweep_threshold_from_cache(sweep_amps, cache), "sweep"

    # binary_search
    lo = amp_to_key(args.amin)
    hi = amp_to_key(args.amax)

    run_many_amps([lo, hi], args, base_cfg, run_root, workdir, cache)

    lo_spike = cache[lo].spiked
    hi_spike = cache[hi].spiked

    if lo_spike and hi_spike:
        print("warning: both amin and amax spike; threshold is at or below amin", file=sys.stderr)
        return lo, "binary_search"

    if not hi_spike:
        print(
            "warning: no spike at amax, binary search not bracketed; falling back to sweep",
            file=sys.stderr,
        )
        run_many_amps(sweep_amps, args, base_cfg, run_root, workdir, cache)
        return sweep_threshold_from_cache(sweep_amps, cache), "sweep_fallback"

    if lo_spike and not hi_spike:
        print(
            "warning: non-monotonic endpoints detected; falling back to sweep",
            file=sys.stderr,
        )
        run_many_amps(sweep_amps, args, base_cfg, run_root, workdir, cache)
        return sweep_threshold_from_cache(sweep_amps, cache), "sweep_fallback"

    # Bracket is [lo (no spike), hi (spike)]
    for _ in range(args.max_binary_iters):
        if hi - lo <= args.threshold_tol:
            break

        mid = amp_to_key(0.5 * (lo + hi))
        run_many_amps([mid], args, base_cfg, run_root, workdir, cache)

        if not monotonic_spike_pattern(cache):
            print(
                "warning: non-monotonic spike pattern observed during binary search; "
                "falling back to sweep",
                file=sys.stderr,
            )
            run_many_amps(sweep_amps, args, base_cfg, run_root, workdir, cache)
            return sweep_threshold_from_cache(sweep_amps, cache), "sweep_fallback"

        if cache[mid].spiked:
            hi = mid
        else:
            lo = mid

    return hi, "binary_search"


def results_to_dataframe(results: dict[float, RunResult]):
    try:
        import pandas as pd
    except ImportError as exc:
        raise RuntimeError("Missing dependency: pandas. Install with: pip install pandas matplotlib numpy") from exc

    rows = []
    for amp in sorted(results):
        r = results[amp]
        rows.append(
            {
                "amp_uAcm2": r.amp_uAcm2,
                "spiked": r.spiked,
                "peak_V_mV": r.peak_V_mV,
                "t_peak_ms": r.t_peak_ms,
                "min_V_mV": r.min_V_mV,
                "max_INa_uAcm2": r.max_INa_uAcm2,
                "max_INa_abs_uAcm2": r.max_INa_abs_uAcm2,
                "max_IK_uAcm2": r.max_IK_uAcm2,
                "csv_path": str(r.csv_path),
                "config_path": str(r.config_path),
            }
        )
    return pd.DataFrame(rows)


def representative_amps(
    amin: float,
    amax: float,
    threshold: float | None,
    delta: float,
) -> list[float]:
    vals = [amin, amax]
    if threshold is not None:
        vals.extend([threshold - delta, threshold, threshold + delta])

    out: list[float] = []
    seen: set[float] = set()
    for a in vals:
        a_key = amp_to_key(a)
        if a_key < amp_to_key(amin) or a_key > amp_to_key(amax):
            continue
        if a_key in seen:
            continue
        seen.add(a_key)
        out.append(a_key)
    return out


def make_plots(
    out_dir: Path,
    summary_df,
    threshold: float | None,
    run_results: dict[float, RunResult],
    args: Phase2Args,
    base_cfg: dict[str, str],
    workdir: Path,
) -> None:
    try:
        import matplotlib
    except ImportError as exc:
        raise RuntimeError("Missing dependency: matplotlib. Install with: pip install matplotlib") from exc

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plots_dir = out_dir / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)

    df = summary_df.sort_values("amp_uAcm2").reset_index(drop=True)
    amp = df["amp_uAcm2"].to_numpy(dtype=float)
    spiked = df["spiked"].to_numpy(dtype=float)
    peak_v = df["peak_V_mV"].to_numpy(dtype=float)

    fig, ax = plt.subplots(figsize=(9, 4))
    ax.step(amp, spiked, where="post", label="spiked")
    ax.set_ylim(-0.1, 1.1)
    ax.set_xlabel("Amplitude (uA/cm^2)")
    ax.set_ylabel("Spike (0/1)")
    ax.set_title("Spike Detection vs Amplitude")
    ax.grid(True, alpha=0.3)
    if threshold is not None:
        ax.axvline(threshold, color="red", linestyle="--", label=f"threshold={threshold:.4g}")
    ax.legend()
    fig.tight_layout()
    fig.savefig(plots_dir / "phase2_spike_vs_amplitude.png", dpi=160)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(9, 4))
    ax.plot(amp, peak_v, marker="o", lw=1.2)
    ax.set_xlabel("Amplitude (uA/cm^2)")
    ax.set_ylabel("Peak V (mV)")
    ax.set_title("Peak Voltage vs Amplitude")
    ax.grid(True, alpha=0.3)
    if threshold is not None:
        ax.axvline(threshold, color="red", linestyle="--", label=f"threshold={threshold:.4g}")
        ax.legend()
    fig.tight_layout()
    fig.savefig(plots_dir / "phase2_peakV_vs_amplitude.png", dpi=160)
    plt.close(fig)

    delta = max(args.threshold_tol, args.astep)
    reps = representative_amps(args.amin, args.amax, threshold, delta)

    fig, ax = plt.subplots(figsize=(10, 5))
    lines_plotted = 0
    for rep in reps:
        key = amp_to_key(rep)
        if key not in run_results:
            run_results[key] = run_single_amp(key, args, base_cfg, out_dir, workdir)

        df_run = load_run_csv(run_results[key].csv_path)
        t = df_run["t_ms"].to_numpy(dtype=float)
        v = df_run["V_mV"].to_numpy(dtype=float)
        ax.plot(t, v, label=f"amp={key:.4g}", lw=1.1)
        lines_plotted += 1

    ax.set_xlabel("t (ms)")
    ax.set_ylabel("V (mV)")
    ax.set_title("Representative V(t) Traces")
    ax.grid(True, alpha=0.3)
    if lines_plotted > 0:
        ax.legend()
    fig.tight_layout()
    fig.savefig(plots_dir / "phase2_gallery_voltage_traces.png", dpi=160)
    plt.close(fig)


def write_threshold_file(path: Path, threshold: float | None, mode_used: str) -> None:
    if threshold is None:
        text = f"threshold_uAcm2=nan\nmode={mode_used}\n"
    else:
        text = f"threshold_uAcm2={threshold:.12g}\nmode={mode_used}\n"
    path.write_text(text, encoding="utf-8")


def main() -> int:
    try:
        args = parse_args()

        if not args.exe.exists():
            raise FileNotFoundError(f"executable not found: {args.exe}")

        out_dir = args.out_dir
        out_dir.mkdir(parents=True, exist_ok=True)
        (out_dir / "configs").mkdir(exist_ok=True)
        (out_dir / "csv").mkdir(exist_ok=True)
        (out_dir / "plots").mkdir(exist_ok=True)

        base_cfg = load_base_config(args.base_config)
        sweep_amps = make_amplitude_grid(args.amin, args.amax, args.astep)

        print(f"loaded base config: {args.base_config}")
        print(f"amplitudes in sweep grid: {len(sweep_amps)}")

        run_results: dict[float, RunResult] = {}

        threshold, mode_used = estimate_threshold(
            args=args,
            sweep_amps=sweep_amps,
            base_cfg=base_cfg,
            run_root=out_dir,
            workdir=Path.cwd(),
            cache=run_results,
        )

        # Ensure summary contains the full sweep grid (useful for protocol plots).
        run_many_amps(sweep_amps, args, base_cfg, out_dir, Path.cwd(), run_results)

        summary_df = results_to_dataframe(run_results)
        summary_csv = out_dir / "summary.csv"
        summary_df.sort_values("amp_uAcm2").to_csv(summary_csv, index=False)

        threshold_file = out_dir / "threshold.txt"
        write_threshold_file(threshold_file, threshold, mode_used)

        make_plots(out_dir, summary_df, threshold, run_results, args, base_cfg, Path.cwd())

        if threshold is None:
            print("threshold: not found in scanned range")
        else:
            print(f"threshold_uAcm2: {threshold:.12g}")

        print(f"summary saved: {summary_csv}")
        print(f"threshold saved: {threshold_file}")
        print(f"plots dir: {out_dir / 'plots'}")
        return 0

    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
