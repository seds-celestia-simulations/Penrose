"""Schema-aware CSV loading for benchmark outputs."""

from __future__ import annotations

import re
from pathlib import Path

import pandas as pd

from .config import benchmark_data_dir
from .schemas import (
    FREEFALL_SCHEMA,
    NULL_GEODESIC_SCHEMA,
    ORBITAL_SCHEMA,
    BenchmarkSchema,
    validate_columns,
)


def _load_csv(path: Path, schema: BenchmarkSchema) -> pd.DataFrame:
    if not path.is_file():
        raise FileNotFoundError(
            f"Benchmark CSV not found: {path}\n"
            f"Run ./build/benchmark_test from the repository root first."
        )
    if path.stat().st_size == 0:
        raise ValueError(f"Benchmark CSV is empty (still being written?): {path}")
    df = pd.read_csv(path)
    if df.empty:
        raise ValueError(f"Benchmark CSV has no data rows: {path}")
    validate_columns(df.columns, schema)
    return df


def load_freefall(data_dir: Path | None = None) -> pd.DataFrame:
    data_dir = data_dir or benchmark_data_dir()
    return _load_csv(data_dir / FREEFALL_SCHEMA.filename, FREEFALL_SCHEMA)


def load_orbital(data_dir: Path | None = None) -> pd.DataFrame:
    data_dir = data_dir or benchmark_data_dir()
    return _load_csv(data_dir / ORBITAL_SCHEMA.filename, ORBITAL_SCHEMA)


def impact_parameter_from_filename(path: Path) -> float:
    match = re.fullmatch(r"null_b_(.+)\.csv", path.name)
    if not match:
        raise ValueError(f"Cannot parse impact parameter from filename: {path.name}")
    return float(match.group(1))


def list_null_geodesic_csvs(data_dir: Path | None = None) -> list[Path]:
    data_dir = data_dir or benchmark_data_dir()
    paths = sorted(data_dir.glob(NULL_GEODESIC_SCHEMA.filename_glob))
    if not paths:
        raise FileNotFoundError(
            f"No null geodesic CSVs matching '{NULL_GEODESIC_SCHEMA.filename_glob}' "
            f"in {data_dir}. Run benchmark_test first."
        )
    return paths


def load_null_geodesic(path: Path) -> pd.DataFrame:
    df = _load_csv(path, NULL_GEODESIC_SCHEMA)
    df.attrs["impact_parameter"] = impact_parameter_from_filename(path)
    df.attrs["source_file"] = path.name
    return df


def load_all_null_geodesics(data_dir: Path | None = None) -> dict[float, pd.DataFrame]:
    data_dir = data_dir or benchmark_data_dir()
    out: dict[float, pd.DataFrame] = {}
    skipped: list[str] = []
    for path in list_null_geodesic_csvs(data_dir):
        try:
            df = load_null_geodesic(path)
        except (ValueError, pd.errors.EmptyDataError) as exc:
            skipped.append(f"{path.name} ({exc})")
            continue
        out[df.attrs["impact_parameter"]] = df
    if not out:
        raise FileNotFoundError(
            f"No readable null geodesic CSVs in {data_dir}."
            + (f" Skipped: {skipped}" if skipped else "")
        )
    return out
