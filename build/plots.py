from __future__ import annotations

import argparse
import json
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import pandas as pd


@dataclass
class CaseFiles:
    name: str
    prefix: Path
    routes: Path
    config: Optional[Path]
    visibility: Optional[Path]
    summary: Optional[Path]
    queue_ascii: Optional[Path]
    queue_csv: Optional[Path]
    stdout: Optional[Path]
    stderr: Optional[Path]


GROUP_COLORS: Dict[str, str] = {
    "A sanity": "#8c8c8c",
    "B small scale": "#4C72B0",
    "C NY-Seattle": "#55A868",
    "D London-NY ping": "#C44E52",
    "E ISL sensitivity": "#8172B2",
    "Other": "#937860",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Starlink HTSim benchmark plots and raport.md from results/*.csv files."
    )
    parser.add_argument(
        "results_dir",
        nargs="?",
        default="results",
        help="Directory containing benchmark outputs, default: results",
    )
    parser.add_argument(
        "--out",
        default=None,
        help="Output directory for report and figures, default: <results_dir>/report",
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Optional path to starlink-htsim root. Used only for documenting repository structure.",
    )
    parser.add_argument(
        "--max-table-rows",
        type=int,
        default=80,
        help="Maximum rows rendered in the Markdown summary table, default: 80",
    )
    return parser.parse_args()


def ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def optional_file(path: Path) -> Optional[Path]:
    return path if path.exists() else None


def discover_cases(results_dir: Path) -> List[CaseFiles]:
    cases: List[CaseFiles] = []
    for routes in sorted(results_dir.rglob("*.routes.csv")):
        prefix = routes.with_suffix("")  # removes .csv -> *.routes
        if prefix.name.endswith(".routes"):
            prefix = prefix.with_name(prefix.name[: -len(".routes")])
        name = prefix.name
        cases.append(
            CaseFiles(
                name=name,
                prefix=prefix,
                routes=routes,
                config=optional_file(Path(str(prefix) + ".config.json")),
                visibility=optional_file(Path(str(prefix) + ".visibility.csv")),
                summary=optional_file(Path(str(prefix) + ".summary.csv")),
                queue_ascii=optional_file(Path(str(prefix) + ".queue_ascii.txt")),
                queue_csv=optional_file(Path(str(prefix) + ".queue.csv")),
                stdout=optional_file(Path(str(prefix) + ".stdout.txt")),
                stderr=optional_file(Path(str(prefix) + ".stderr.txt")),
            )
        )
    return cases


def read_json(path: Optional[Path]) -> Dict:
    if not path or not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def safe_read_csv(path: Path) -> pd.DataFrame:
    try:
        df = pd.read_csv(path)
    except pd.errors.EmptyDataError:
        return pd.DataFrame()
    except Exception as exc:
        print(f"Warning: failed to read {path}: {exc}", file=sys.stderr)
        return pd.DataFrame()
    return df


def to_numeric(df: pd.DataFrame, columns: Iterable[str]) -> pd.DataFrame:
    for col in columns:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df


def scenario_group(case_name: str) -> str:
    if case_name.startswith("A_"):
        return "A sanity"
    if case_name.startswith("B_"):
        return "B small scale"
    if case_name.startswith("C_"):
        return "C NY-Seattle"
    if case_name.startswith("D_"):
        return "D London-NY ping"
    if case_name.startswith("E_"):
        return "E ISL sensitivity"
    return "Other"


def route_segments(df: pd.DataFrame) -> Tuple[float, int]:
    """Return mean duration of found route segments and number of route-state changes."""
    if df.empty or "time_s" not in df.columns or "route_found" not in df.columns:
        return (math.nan, 0)

    d = df.sort_values("time_s").copy()
    found = d["route_found"].fillna(0).astype(int) == 1
    hashes = d.get("route_hash", pd.Series([""] * len(d), index=d.index)).fillna("")
    state = hashes.where(found, "NO_ROUTE")

    changed = state.ne(state.shift(1))
    change_indices = list(d.index[changed])
    if not change_indices:
        return (math.nan, 0)

    times = d["time_s"].to_numpy()
    states = state.to_numpy()
    found_arr = found.to_numpy()

    durations: List[float] = []
    state_changes = 0
    start_pos = 0
    for pos in range(1, len(d)):
        if states[pos] != states[start_pos]:
            state_changes += 1
            if found_arr[start_pos]:
                durations.append(max(0.0, float(times[pos - 1] - times[start_pos])))
            start_pos = pos
    if found_arr[start_pos]:
        durations.append(max(0.0, float(times[-1] - times[start_pos])))

    mean_duration = float(pd.Series(durations).mean()) if durations else math.nan
    return mean_duration, state_changes


def choose_direction(df: pd.DataFrame, preferred: str = "out") -> pd.DataFrame:
    if "direction" not in df.columns:
        return df
    if (df["direction"] == preferred).any():
        return df[df["direction"] == preferred].copy()
    return df.copy()


def summarize_routes(case: CaseFiles) -> Dict:
    cfg = read_json(case.config)
    df = safe_read_csv(case.routes)
    numeric_cols = [
        "time_s",
        "route_found",
        "rtt_ms",
        "one_way_latency_ms",
        "route_changed",
        "route_duration_s",
        "link_hops",
        "isl_hops",
        "ground_hops",
        "active_uplinks_src",
        "active_uplinks_dst",
        "inactive_sats_src",
        "inactive_sats_dst",
        "nearest_src_km",
        "nearest_dst_km",
        "dijkstra_cpu_ms",
        "num_planes",
        "sats_per_plane",
        "orbital_slots",
        "adjacent_sats",
        "num_sats",
    ]
    df = to_numeric(df, numeric_cols)
    d = choose_direction(df, "out")

    row: Dict[str, object] = {
        "case": case.name,
        "group": scenario_group(case.name),
        "routes_file": str(case.routes),
        "routing_only": cfg.get("routing_only", None),
        "num_planes": cfg.get("num_planes", first_valid(d, "num_planes")),
        "sats_per_plane": cfg.get("sats_per_plane", first_valid(d, "sats_per_plane")),
        "orbital_slots": cfg.get("orbital_slots", first_valid(d, "orbital_slots")),
        "num_sats": cfg.get("num_sats_actual", first_valid(d, "num_sats")),
        "src": coord_string(cfg, "src"),
        "dst": coord_string(cfg, "dst"),
    }

    if d.empty:
        row.update(empty_route_metrics())
        return row

    duration_s = float(d["time_s"].max() - d["time_s"].min()) if "time_s" in d else math.nan
    found = d["route_found"].fillna(0).astype(int) == 1 if "route_found" in d else pd.Series(False, index=d.index)
    valid = d[found].copy()

    mean_segment_duration_s, derived_state_changes = route_segments(d)
    route_changes_logged = int(d.get("route_changed", pd.Series(0, index=d.index)).fillna(0).sum())
    route_changes = max(route_changes_logged, derived_state_changes)
    changes_per_min = route_changes / (duration_s / 60.0) if duration_s and duration_s > 0 else math.nan

    row.update(
        {
            "samples": int(len(d)),
            "duration_s": duration_s,
            "availability_pct": float(found.mean() * 100.0) if len(found) else math.nan,
            "no_route_pct": float((~found).mean() * 100.0) if len(found) else math.nan,
            "mean_rtt_ms": col_mean(valid, "rtt_ms"),
            "median_rtt_ms": col_median(valid, "rtt_ms"),
            "p95_rtt_ms": col_quantile(valid, "rtt_ms", 0.95),
            "min_rtt_ms": col_min(valid, "rtt_ms"),
            "max_rtt_ms": col_max(valid, "rtt_ms"),
            "mean_one_way_ms": col_mean(valid, "one_way_latency_ms"),
            "mean_link_hops": col_mean(valid, "link_hops"),
            "mean_isl_hops": col_mean(valid, "isl_hops"),
            "mean_ground_hops": col_mean(valid, "ground_hops"),
            "mean_active_uplinks_src": col_mean(d, "active_uplinks_src"),
            "mean_active_uplinks_dst": col_mean(d, "active_uplinks_dst"),
            "mean_nearest_src_km": col_mean(d, "nearest_src_km"),
            "mean_nearest_dst_km": col_mean(d, "nearest_dst_km"),
            "mean_dijkstra_cpu_ms": col_mean(d, "dijkstra_cpu_ms"),
            "p95_dijkstra_cpu_ms": col_quantile(d, "dijkstra_cpu_ms", 0.95),
            "route_changes": int(route_changes),
            "route_changes_per_min": changes_per_min,
            "mean_route_segment_duration_s": mean_segment_duration_s,
        }
    )
    return row


def empty_route_metrics() -> Dict[str, object]:
    return {
        "samples": 0,
        "duration_s": math.nan,
        "availability_pct": math.nan,
        "no_route_pct": math.nan,
        "mean_rtt_ms": math.nan,
        "median_rtt_ms": math.nan,
        "p95_rtt_ms": math.nan,
        "min_rtt_ms": math.nan,
        "max_rtt_ms": math.nan,
        "mean_one_way_ms": math.nan,
        "mean_link_hops": math.nan,
        "mean_isl_hops": math.nan,
        "mean_ground_hops": math.nan,
        "mean_active_uplinks_src": math.nan,
        "mean_active_uplinks_dst": math.nan,
        "mean_nearest_src_km": math.nan,
        "mean_nearest_dst_km": math.nan,
        "mean_dijkstra_cpu_ms": math.nan,
        "p95_dijkstra_cpu_ms": math.nan,
        "route_changes": 0,
        "route_changes_per_min": math.nan,
        "mean_route_segment_duration_s": math.nan,
    }


def first_valid(df: pd.DataFrame, col: str):
    if col not in df.columns or df[col].dropna().empty:
        return None
    value = df[col].dropna().iloc[0]
    if isinstance(value, float) and value.is_integer():
        return int(value)
    return value


def coord_string(cfg: Dict, prefix: str) -> str:
    lat = cfg.get(f"{prefix}_lat")
    lon = cfg.get(f"{prefix}_lon")
    if lat is None or lon is None:
        return ""
    return f"{lat},{lon}"


def col_mean(df: pd.DataFrame, col: str) -> float:
    return float(df[col].mean()) if col in df.columns and not df[col].dropna().empty else math.nan


def col_median(df: pd.DataFrame, col: str) -> float:
    return float(df[col].median()) if col in df.columns and not df[col].dropna().empty else math.nan


def col_quantile(df: pd.DataFrame, col: str, q: float) -> float:
    return float(df[col].quantile(q)) if col in df.columns and not df[col].dropna().empty else math.nan


def col_min(df: pd.DataFrame, col: str) -> float:
    return float(df[col].min()) if col in df.columns and not df[col].dropna().empty else math.nan


def col_max(df: pd.DataFrame, col: str) -> float:
    return float(df[col].max()) if col in df.columns and not df[col].dropna().empty else math.nan


def parse_queue_ascii(path: Optional[Path]) -> pd.DataFrame:
    if not path or not path.exists():
        return pd.DataFrame()

    rows: List[Dict[str, object]] = []
    range_re = re.compile(
        r"^(?P<time>\S+)\s+Type\s+(?P<type>\S+)\s+ID\s+(?P<id>\d+)\s+Ev\s+RANGE\s+"
        r"LastQ\s+(?P<lastq>[-+eE0-9.]+)\s+MinQ\s+(?P<minq>[-+eE0-9.]+)\s+Max\s+(?P<maxq>[-+eE0-9.]+)"
    )
    over_re = re.compile(
        r"^(?P<time>\S+)\s+Type\s+(?P<type>\S+)\s+ID\s+(?P<id>\d+)\s+Ev\s+OVERLOW\s+"
        r"LastIdled\s+(?P<lastidled>[-+eE0-9.]+)\s+LastDropped\s+(?P<lastdropped>[-+eE0-9.]+)\s+QueueBuf\s+(?P<queuebuf>[-+eE0-9.]+)"
    )
    cum_re = re.compile(
        r"^(?P<time>\S+)\s+Type\s+(?P<type>\S+)\s+ID\s+(?P<id>\d+)\s+Ev\s+CUM_TRAFFIC\s+"
        r"CumArr\s+(?P<cumarr>[-+eE0-9.]+)\s+CumIdle\s+(?P<cumidle>[-+eE0-9.]+)\s+CumDrop\s+(?P<cumdrop>[-+eE0-9.]+)"
    )

    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = line.strip()
        if not line:
            continue
        for ev_name, regex in (("RANGE", range_re), ("OVERLOW", over_re), ("CUM_TRAFFIC", cum_re)):
            m = regex.match(line)
            if m:
                data: Dict[str, object] = {
                    "time_s": float(m.group("time")),
                    "queue_type": m.group("type"),
                    "id": int(m.group("id")),
                    "event": ev_name,
                }
                for key, value in m.groupdict().items():
                    if key in {"time", "type", "id"}:
                        continue
                    data[key] = float(value)
                rows.append(data)
                break
    return pd.DataFrame(rows)


def summarize_queue(case: CaseFiles) -> Dict[str, object]:
    df = parse_queue_ascii(case.queue_ascii)
    row: Dict[str, object] = {
        "case": case.name,
        "group": scenario_group(case.name),
        "queue_file": str(case.queue_ascii) if case.queue_ascii else "",
        "queue_samples": 0,
        "max_queue_bytes": math.nan,
        "mean_lastq_bytes": math.nan,
        "max_lastq_bytes": math.nan,
        "max_queue_buffer_bytes": math.nan,
        "max_last_dropped": math.nan,
        "max_cumulative_drop": math.nan,
        "max_cumulative_arrivals": math.nan,
    }
    if df.empty:
        return row

    row["queue_samples"] = int(len(df))
    range_df = df[df["event"] == "RANGE"]
    over_df = df[df["event"] == "OVERLOW"]
    cum_df = df[df["event"] == "CUM_TRAFFIC"]

    if not range_df.empty:
        row["max_queue_bytes"] = col_max(range_df, "maxq")
        row["mean_lastq_bytes"] = col_mean(range_df, "lastq")
        row["max_lastq_bytes"] = col_max(range_df, "lastq")
    if not over_df.empty:
        row["max_queue_buffer_bytes"] = col_max(over_df, "queuebuf")
        row["max_last_dropped"] = col_max(over_df, "lastdropped")
    if not cum_df.empty:
        row["max_cumulative_drop"] = col_max(cum_df, "cumdrop")
        row["max_cumulative_arrivals"] = col_max(cum_df, "cumarr")
    return row


def clean_label(case_name: str) -> str:
    label = case_name
    label = label.replace("A_sanity_", "A ")
    label = label.replace("B_", "B ")
    label = label.replace("C_", "C ")
    label = label.replace("D_", "D ")
    label = label.replace("E_", "E ")
    label = label.replace("_", " ")
    label = label.replace("planes", "planes")
    return label


def sort_summary(df: pd.DataFrame) -> pd.DataFrame:
    order = {"A sanity": 0, "B small scale": 1, "C NY-Seattle": 2, "D London-NY ping": 3, "E ISL sensitivity": 4, "Other": 5}
    d = df.copy()
    d["_group_order"] = d["group"].map(order).fillna(99)
    return d.sort_values(["_group_order", "case"]).drop(columns=["_group_order"])


def add_group_legend(ax, groups_present: Iterable[str]) -> None:
    handles = [Patch(color=GROUP_COLORS.get(g, "#4C72B0"), label=g) for g in dict.fromkeys(groups_present)]
    if handles:
        ax.legend(handles=handles, fontsize=8, loc="best")


def save_barh(
    df: pd.DataFrame,
    metric: str,
    title: str,
    xlabel: str,
    path: Path,
    *,
    dropna: bool = True,
    annotate: bool = True,
    color_by_group: bool = True,
) -> Optional[Path]:
    if df.empty or metric not in df.columns:
        return None
    d = sort_summary(df)
    if dropna:
        d = d.dropna(subset=[metric])
    if d.empty:
        return None

    height = max(4.0, 0.35 * len(d) + 1.5)
    fig, ax = plt.subplots(figsize=(10, height))
    labels = [clean_label(x) for x in d["case"].tolist()]
    values = d[metric].astype(float).tolist()
    y_pos = range(len(labels))

    colors = None
    if color_by_group and "group" in d.columns:
        colors = [GROUP_COLORS.get(g, "#4C72B0") for g in d["group"].tolist()]

    bars = ax.barh(list(y_pos), values, color=colors)
    ax.set_yticks(list(y_pos))
    ax.set_yticklabels(labels)
    ax.invert_yaxis()
    ax.set_title(title)
    ax.set_xlabel(xlabel)
    ax.grid(True, axis="x", alpha=0.3)

    if annotate and values:
        span = max(values) - min(min(values), 0.0)
        offset = span * 0.012 if span else 0.01
        for rect, value in zip(bars, values):
            ax.text(
                rect.get_width() + offset,
                rect.get_y() + rect.get_height() / 2,
                f"{value:.2f}",
                va="center",
                fontsize=8,
            )

    if colors is not None:
        add_group_legend(ax, d["group"].tolist())

    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)
    return path


def save_line_for_cases(
    cases: List[CaseFiles],
    matcher: re.Pattern,
    title: str,
    path: Path,
    y_col: str = "rtt_ms",
    y_label: str = "RTT [ms]",
    time_unit: str = "minutes",
) -> Optional[Path]:
    selected = [c for c in cases if matcher.search(c.name)]
    if not selected:
        return None

    fig, ax = plt.subplots(figsize=(11, 5.5))
    plotted = False
    for case in selected:
        df = safe_read_csv(case.routes)
        if df.empty:
            continue
        df = to_numeric(df, ["time_s", "route_found", y_col])
        d = choose_direction(df, "out").sort_values("time_s")
        if d.empty or y_col not in d.columns:
            continue
        d.loc[d["route_found"].fillna(0).astype(int) != 1, y_col] = math.nan
        x = d["time_s"] / 60.0 if time_unit == "minutes" else d["time_s"]
        ax.plot(x, d[y_col], label=clean_label(case.name), linewidth=1.5)
        plotted = True

    if not plotted:
        plt.close(fig)
        return None

    ax.set_title(title)
    ax.set_xlabel("Czas [min]" if time_unit == "minutes" else "Czas [s]")
    ax.set_ylabel(y_label)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)
    return path


def save_rtt_boxplot(cases: List[CaseFiles], matcher: re.Pattern, title: str, path: Path) -> Optional[Path]:
    labels: List[str] = []
    data: List[List[float]] = []
    for case in cases:
        if not matcher.search(case.name):
            continue
        df = safe_read_csv(case.routes)
        if df.empty:
            continue
        df = to_numeric(df, ["route_found", "rtt_ms"])
        d = choose_direction(df, "out")
        valid = d[d["route_found"].fillna(0).astype(int) == 1]
        values = valid["rtt_ms"].dropna().astype(float).tolist() if "rtt_ms" in valid.columns else []
        if values:
            labels.append(clean_label(case.name))
            data.append(values)
    if not data:
        return None

    fig, ax = plt.subplots(figsize=(10, 5.5))
    try:
        ax.boxplot(data, tick_labels=labels, showfliers=False, showmeans=True, meanline=True)
    except TypeError:
        ax.boxplot(data, labels=labels, showfliers=False, showmeans=True, meanline=True)
    ax.set_title(title)
    ax.set_ylabel("RTT [ms]")
    ax.grid(True, axis="y", alpha=0.3)
    plt.setp(ax.get_xticklabels(), rotation=20, ha="right")
    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)
    return path


def make_figures(cases: List[CaseFiles], summary: pd.DataFrame, out_dir: Path) -> List[Tuple[str, Path, str]]:
    figures_dir = ensure_dir(out_dir / "figures")
    figures: List[Tuple[str, Path, str]] = []

    specs = [
        (
            "Dostępność tras we wszystkich benchmarkach",
            save_barh(
                summary,
                "availability_pct",
                "Dostępność trasy według benchmarku",
                "Dostępność [% próbek z route_found=1]",
                figures_dir / "01_availability_by_case.png",
            ),
            "Odsetek próbek, w których Dijkstra znalazł trasę między endpointami. Kolor słupka koduje grupę scenariusza (A-E).",
        ),
        (
            "Średni RTT we wszystkich benchmarkach",
            save_barh(
                summary,
                "mean_rtt_ms",
                "Średni RTT dla znalezionych tras",
                "RTT [ms]",
                figures_dir / "02_mean_rtt_by_case.png",
            ),
            "Średni RTT liczony tylko dla próbek, w których trasa istniała. Wartości liczbowe podpisane bezpośrednio na słupkach.",
        ),
        (
            "Częstotliwość zmian tras",
            save_barh(
                summary,
                "route_changes_per_min",
                "Zmiany tras na minutę",
                "Zmiany tras / min",
                figures_dir / "03_route_changes_per_min.png",
            ),
            "Częstotliwość zmian identyfikatora trasy lub przejść do stanu NO_ROUTE.",
        ),
        (
            "Koszt obliczania tras",
            save_barh(
                summary,
                "mean_dijkstra_cpu_ms",
                "Średni czas obliczania trasy",
                "Czas CPU [ms]",
                figures_dir / "04_mean_dijkstra_cpu_ms.png",
            ),
            "Czas CPU mierzony przy wyszukiwaniu tras. Przy małych topologiach powinien być bardzo niski.",
        ),
        (
            "Średnia liczba hopów ISL",
            save_barh(
                summary,
                "mean_isl_hops",
                "Średnia liczba hopów ISL w trasie",
                "ISL hops",
                figures_dir / "05_mean_isl_hops.png",
            ),
            "Pokazuje, czy trasy korzystają głównie z jednego satelity, czy z sieci inter-satellite links.",
        ),
    ]
    figures.extend([(title, path, desc) for title, path, desc in specs if path is not None])

    path = save_line_for_cases(
        cases,
        re.compile(r"^C_NY_Seattle_.*planes"),
        "RTT w czasie dla benchmarku NY-Seattle",
        figures_dir / "06_C_NY_Seattle_rtt_timeseries.png",
        "rtt_ms",
        "RTT [ms]",
        "minutes",
    )
    if path:
        figures.append(("RTT w czasie: NY-Seattle", path, "Porównanie konfiguracji 6/12/24 płaszczyzn orbitalnych."))

    path = save_rtt_boxplot(
        cases,
        re.compile(r"^C_NY_Seattle_.*planes"),
        "Rozkład RTT dla NY-Seattle",
        figures_dir / "07_C_NY_Seattle_rtt_boxplot.png",
    )
    if path:
        figures.append(("Rozkład RTT: NY-Seattle", path, "Boxplot bez outlierów (z zaznaczoną średnią), liczony dla próbek z istniejącą trasą."))

    path = save_line_for_cases(
        cases,
        re.compile(r"^D_London_NY_.*planes_ping"),
        "RTT w czasie dla London-New York z ruchem ping",
        figures_dir / "08_D_London_NY_ping_rtt_timeseries.png",
        "rtt_ms",
        "RTT [ms]",
        "minutes",
    )
    if path:
        figures.append(("RTT w czasie: London-New York", path, "Benchmark z włączonym ruchem pakietowym i binlogiem kolejek."))

    return figures


def main() -> int:
    args = parse_args()
    results_dir = Path(args.results_dir).resolve()
    out_dir = Path(args.out).resolve() if args.out else (results_dir / "report").resolve()

    if not results_dir.exists():
        print(f"Results directory does not exist: {results_dir}", file=sys.stderr)
        return 2

    ensure_dir(out_dir)
    ensure_dir(out_dir / "figures")

    cases = discover_cases(results_dir)
    if not cases:
        print(f"No *.routes.csv files found in {results_dir}", file=sys.stderr)
        return 3

    summary_rows = [summarize_routes(case) for case in cases]
    summary = sort_summary(pd.DataFrame(summary_rows))
    summary_path = out_dir / "summary_metrics.csv"
    summary.to_csv(summary_path, index=False)

    queue_rows = [summarize_queue(case) for case in cases]
    queue_summary = sort_summary(pd.DataFrame(queue_rows)) if queue_rows else pd.DataFrame()
    queue_path = out_dir / "queue_metrics.csv"
    queue_summary.to_csv(queue_path, index=False)

    figures = make_figures(cases, summary, out_dir)

    print(f"Found cases: {len(cases)}")
    print(f"Summary: {summary_path}")
    print(f"Queue metrics: {queue_path}")
    print(f"Figures: {out_dir / 'figures'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())