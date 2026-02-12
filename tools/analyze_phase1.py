#!/usr/bin/env python3
"""Phase-1 validation and visualization for axonhh CSV outputs.

This tool performs sanity checks and invariant-style diagnostics for HH simulation
runs, then saves quick-look plots for debugging.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Phase-1 validation + visualization for axonhh CSV runs.",
    )
    parser.add_argument("csv_path", type=Path, help="Path to simulation CSV")
    parser.add_argument(
        "--Cm",
        type=float,
        default=1.0,
        help="Membrane capacitance (uF/cm^2) for residual check; default: 1.0",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Display plots interactively in addition to saving PNG files",
    )
    return parser.parse_args()


def load_and_validate_csv(csv_path: Path):
    try:
        import pandas as pd
    except ImportError as exc:
        raise RuntimeError(
            "Missing dependency: pandas. Install with: pip install pandas matplotlib numpy"
        ) from exc

    if not csv_path.exists():
        raise FileNotFoundError(f"CSV file not found: {csv_path}")

    try:
        raw = pd.read_csv(csv_path)
    except pd.errors.EmptyDataError as exc:
        raise ValueError(f"CSV is empty: {csv_path}") from exc
    except pd.errors.ParserError as exc:
        raise ValueError(f"Malformed CSV: {exc}") from exc

    missing = [c for c in REQUIRED_COLUMNS if c not in raw.columns]
    if missing:
        raise ValueError(
            "CSV missing required columns: "
            + ", ".join(missing)
            + f"\nExpected header: {','.join(REQUIRED_COLUMNS)}"
        )

    data = raw.loc[:, REQUIRED_COLUMNS].copy()
    for col in REQUIRED_COLUMNS:
        data[col] = pd.to_numeric(data[col], errors="coerce")

    return data


def finite_min_max(x: np.ndarray) -> tuple[float, float]:
    finite = x[np.isfinite(x)]
    if finite.size == 0:
        return np.nan, np.nan
    return float(np.min(finite)), float(np.max(finite))


def compute_dvdt(t_ms: np.ndarray, v_mV: np.ndarray) -> np.ndarray:
    n = t_ms.size
    dvdt = np.full(n, np.nan, dtype=float)

    if n < 2:
        return dvdt

    dvdt[0] = (v_mV[1] - v_mV[0]) / (t_ms[1] - t_ms[0])
    dvdt[-1] = (v_mV[-1] - v_mV[-2]) / (t_ms[-1] - t_ms[-2])

    if n > 2:
        dvdt[1:-1] = (v_mV[2:] - v_mV[:-2]) / (t_ms[2:] - t_ms[:-2])

    return dvdt


def format_num(x: float) -> str:
    return "nan" if not np.isfinite(x) else f"{x:.6g}"


def print_report(data, cm: float) -> dict[str, np.ndarray]:
    arrays = {col: data[col].to_numpy(dtype=float) for col in REQUIRED_COLUMNS}

    t = arrays["t_ms"]
    v = arrays["V_mV"]
    m = arrays["m"]
    h = arrays["h"]
    n = arrays["n"]
    iinj = arrays["Iinj_uAcm2"]
    ina = arrays["INa_uAcm2"]
    ik = arrays["IK_uAcm2"]
    il = arrays["IL_uAcm2"]

    dt = np.diff(t)
    finite_dt = dt[np.isfinite(dt)]

    iion = ina + ik + il
    inet = iinj - iion
    dvdt = compute_dvdt(t, v)
    residual = inet - cm * dvdt

    print("Phase-1 Validation Report")
    print(f"rows: {len(data)}")

    if finite_dt.size > 0:
        print(
            "dt_ms stats: "
            f"min={format_num(float(np.min(finite_dt)))} "
            f"mean={format_num(float(np.mean(finite_dt)))} "
            f"max={format_num(float(np.max(finite_dt)))}"
        )
    else:
        print("dt_ms stats: min=nan mean=nan max=nan")

    v_min, v_max = finite_min_max(v)
    print(f"V_mV range: min={format_num(v_min)} max={format_num(v_max)}")

    for g in ("m", "h", "n"):
        g_min, g_max = finite_min_max(arrays[g])
        print(f"{g} range: min={format_num(g_min)} max={format_num(g_max)}")

    print("non-finite counts per column (NaN + Inf):")
    for col in REQUIRED_COLUMNS:
        col_data = arrays[col]
        nan_count = int(np.isnan(col_data).sum())
        inf_count = int(np.isinf(col_data).sum())
        total = nan_count + inf_count
        print(f"  {col}: total={total} (NaN={nan_count}, Inf={inf_count})")

    print("gating violations outside [0,1]:")
    for g in ("m", "h", "n"):
        x = arrays[g]
        finite = np.isfinite(x)
        outside_mask = finite & ((x < 0.0) | (x > 1.0))
        count = int(outside_mask.sum())

        if count == 0:
            print(f"  {g}: count=0, worst_value=n/a")
            continue

        outside_vals = x[outside_mask]
        deviations = np.maximum(0.0, np.maximum(-outside_vals, outside_vals - 1.0))
        idx = int(np.argmax(deviations))
        worst_value = float(outside_vals[idx])
        worst_deviation = float(deviations[idx])
        print(
            f"  {g}: count={count}, worst_value={format_num(worst_value)} "
            f"(deviation={format_num(worst_deviation)})"
        )

    finite_res = residual[np.isfinite(residual)]
    if finite_res.size > 0:
        res_min = float(np.min(finite_res))
        res_mean = float(np.mean(finite_res))
        res_max = float(np.max(finite_res))
        res_rms = float(np.sqrt(np.mean(finite_res**2)))
    else:
        res_min = np.nan
        res_mean = np.nan
        res_max = np.nan
        res_rms = np.nan

    print(f"Cm used: {format_num(cm)} uF/cm^2")
    print(
        "residual = Inet - Cm*dVdt stats (uA/cm^2): "
        f"min={format_num(res_min)} "
        f"mean={format_num(res_mean)} "
        f"max={format_num(res_max)} "
        f"rms={format_num(res_rms)}"
    )

    return {
        **arrays,
        "dt_ms": dt,
        "Iion_uAcm2": iion,
        "Inet_uAcm2": inet,
        "dVdt_mV_per_ms": dvdt,
        "residual_uAcm2": residual,
    }


def save_plots(
    csv_path: Path,
    arrays: dict[str, np.ndarray],
    show: bool,
) -> list[Path]:
    try:
        import matplotlib
    except ImportError as exc:
        raise RuntimeError(
            "Missing dependency: matplotlib. Install with: pip install matplotlib"
        ) from exc

    if not show:
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt

    t = arrays["t_ms"]
    v = arrays["V_mV"]
    m = arrays["m"]
    h = arrays["h"]
    n = arrays["n"]
    ina = arrays["INa_uAcm2"]
    ik = arrays["IK_uAcm2"]
    il = arrays["IL_uAcm2"]
    inet = arrays["Inet_uAcm2"]
    residual = arrays["residual_uAcm2"]

    plots_dir = csv_path.resolve().parent / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)

    saved: list[Path] = []

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, v, lw=1.5)
    ax.set_title("Membrane Voltage")
    ax.set_xlabel("t (ms)")
    ax.set_ylabel("V (mV)")
    ax.grid(True, alpha=0.3)
    p = plots_dir / "phase1_voltage_vs_time.png"
    fig.tight_layout()
    fig.savefig(p, dpi=160)
    saved.append(p)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, m, label="m", lw=1.2)
    ax.plot(t, h, label="h", lw=1.2)
    ax.plot(t, n, label="n", lw=1.2)
    ax.set_title("Gating Variables")
    ax.set_xlabel("t (ms)")
    ax.set_ylabel("value")
    ax.legend()
    ax.grid(True, alpha=0.3)
    p = plots_dir / "phase1_gating_vs_time.png"
    fig.tight_layout()
    fig.savefig(p, dpi=160)
    saved.append(p)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, ina, label="INa", lw=1.1)
    ax.plot(t, ik, label="IK", lw=1.1)
    ax.plot(t, il, label="IL", lw=1.1)
    ax.plot(t, inet, label="Inet = Iinj - (INa+IK+IL)", lw=1.4)
    ax.set_title("Currents")
    ax.set_xlabel("t (ms)")
    ax.set_ylabel("uA/cm^2")
    ax.legend()
    ax.grid(True, alpha=0.3)
    p = plots_dir / "phase1_currents_vs_time.png"
    fig.tight_layout()
    fig.savefig(p, dpi=160)
    saved.append(p)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, residual, lw=1.2)
    ax.set_title("Membrane Equation Residual")
    ax.set_xlabel("t (ms)")
    ax.set_ylabel("residual (uA/cm^2)")
    ax.grid(True, alpha=0.3)
    p = plots_dir / "phase1_residual_vs_time.png"
    fig.tight_layout()
    fig.savefig(p, dpi=160)
    saved.append(p)
    plt.close(fig)

    finite_res = residual[np.isfinite(residual)]
    if finite_res.size > 0:
        fig, ax = plt.subplots(figsize=(7, 4))
        ax.hist(finite_res, bins=60)
        ax.set_title("Residual Histogram")
        ax.set_xlabel("residual (uA/cm^2)")
        ax.set_ylabel("count")
        ax.grid(True, alpha=0.3)
        p = plots_dir / "phase1_residual_histogram.png"
        fig.tight_layout()
        fig.savefig(p, dpi=160)
        saved.append(p)
        plt.close(fig)

    if show:
        plt.show()

    return saved


def main() -> int:
    args = parse_args()

    if not np.isfinite(args.Cm):
        print("error: --Cm must be finite", file=sys.stderr)
        return 2

    try:
        data = load_and_validate_csv(args.csv_path)
        arrays = print_report(data, args.Cm)
        saved = save_plots(args.csv_path, arrays, args.show)
    except Exception as exc:  # explicit and user-facing CLI errors
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print("saved plots:")
    for p in saved:
        print(f"  {p}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
