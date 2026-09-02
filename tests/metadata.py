"""CompressKit test metadata — single source of truth for algorithms and corpus."""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path

METADATA_VERSION = "1.0"

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"
DATA_DIR = TESTS_DIR / "data"


@dataclass(frozen=True)
class AlgorithmEntry:
    name: str
    extension: str
    binary: Path


ALGORITHMS: tuple[AlgorithmEntry, ...] = (
    AlgorithmEntry("huffman", "huf", ROOT / "build/huffman_cpp"),
    AlgorithmEntry("arithmetic", "aenc", ROOT / "build/arithmetic_cpp"),
    AlgorithmEntry("range", "rcnc", ROOT / "build/rangecoder_cpp"),
    AlgorithmEntry("rle", "rle", ROOT / "build/rle_cpp"),
    AlgorithmEntry("lzss", "lzs", ROOT / "build/lzss_cpp"),
)

ALGORITHM_ORDER: tuple[str, ...] = tuple(a.name for a in ALGORITHMS)


@dataclass(frozen=True)
class CorpusEntry:
    name: str
    is_large: bool = False


CORPUS: tuple[CorpusEntry, ...] = (
    CorpusEntry("empty.bin"),
    CorpusEntry("single_byte.bin"),
    CorpusEntry("alternating.bin"),
    CorpusEntry("all_same_byte.bin"),
    CorpusEntry("small_dictionary_like.bin"),
    CorpusEntry("random_1MiB.bin", is_large=True),
    CorpusEntry("random_10MiB.bin", is_large=True),
    CorpusEntry("repetitive_10MiB.bin", is_large=True),
    CorpusEntry("textlike_10MiB.bin", is_large=True),
    CorpusEntry("fastq_10MiB.bin", is_large=True),
)

DEFAULT_CORPUS: tuple[str, ...] = tuple(c.name for c in CORPUS if not c.is_large)
LARGE_CORPUS: tuple[str, ...] = tuple(c.name for c in CORPUS if c.is_large)


def algorithm_by_name(name: str) -> AlgorithmEntry:
    for algo in ALGORITHMS:
        if algo.name == name:
            return algo
    raise KeyError(f"unknown algorithm: {name!r}")


def corpus_files(include_large: bool = False) -> tuple[str, ...]:
    if include_large:
        return tuple(c.name for c in CORPUS)
    return DEFAULT_CORPUS


def require_binaries() -> None:
    missing = [a.binary for a in ALGORITHMS if not a.binary.is_file()]
    if missing:
        raise SystemExit(
            "missing binaries; run `make build` first:\n" + "\n".join(str(p) for p in missing)
        )


def run_cli(command: list[str], *, timeout: float) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout,
        check=False,
    )


def run_cli_checked(command: list[str], *, timeout: float) -> None:
    proc = run_cli(command, timeout=timeout)
    if proc.returncode == 0:
        return
    raise RuntimeError(
        f"command failed (exit {proc.returncode}): {' '.join(command)}\n"
        f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}"
    )
