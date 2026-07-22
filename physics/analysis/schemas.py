"""CSV column schemas for the three benchmark drivers."""

from dataclasses import dataclass
from typing import Sequence


@dataclass(frozen=True)
class BenchmarkSchema:
    name: str
    required_columns: tuple[str, ...]
    filename: str | None = None
    filename_glob: str | None = None


FREEFALL_SCHEMA = BenchmarkSchema(
    name="freefall",
    required_columns=("tau", "r", "vt", "vr"),
    filename="freefall.csv",
)

ORBITAL_SCHEMA = BenchmarkSchema(
    name="orbital",
    required_columns=("tau", "r", "phi", "vt", "vr", "vph", "norm"),
    filename="orbital.csv",
)

NULL_GEODESIC_SCHEMA = BenchmarkSchema(
    name="null_geodesic",
    required_columns=(
        "lambda",
        "r",
        "phi",
        "vt",
        "vr",
        "vph",
        "H",
        "E",
        "L",
        "dE",
        "dL",
        "dvt",
        "dvph",
        "phi_total",
    ),
    filename_glob="null_b_*.csv",
)


ALL_SCHEMAS: dict[str, BenchmarkSchema] = {
    FREEFALL_SCHEMA.name: FREEFALL_SCHEMA,
    ORBITAL_SCHEMA.name: ORBITAL_SCHEMA,
    NULL_GEODESIC_SCHEMA.name: NULL_GEODESIC_SCHEMA,
}


def validate_columns(columns: Sequence[str], schema: BenchmarkSchema) -> None:
    missing = [c for c in schema.required_columns if c not in columns]
    if missing:
        raise ValueError(
            f"CSV schema mismatch for '{schema.name}': missing columns {missing}. "
            f"Expected {list(schema.required_columns)}, got {list(columns)}."
        )
