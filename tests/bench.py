#!/usr/bin/env python3
"""Time each algorithm against the standard corpus and write a docs snapshot."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
import time
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metadata

TIMEOUT_SECONDS = 60.0

# Same-size comparisons first; repetitive is the RLE-meaningful extra set,
# fastq is the genome-flavored extra set.
DATASETS = (
    "textlike_10MiB.bin",
    "repetitive_10MiB.bin",
    "random_1MiB.bin",
    "fastq_10MiB.bin",
)


def _run(command: list[str]) -> None:
    metadata.run_cli_checked(command, timeout=TIMEOUT_SECONDS)


def throughput_label(mib_per_s: float) -> str:
    if mib_per_s >= 40:
        return "medium"
    if mib_per_s >= 10:
        return "low"
    return "very-low"


def time_command(command: list[str]) -> float:
    start = time.perf_counter()
    _run(command)
    return (time.perf_counter() - start) * 1000.0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=metadata.ROOT / "docs/.vitepress/data/benchmarks.json",
    )
    args = parser.parse_args(argv)

    metadata.require_binaries()

    results = []
    with tempfile.TemporaryDirectory(prefix=".bench-", dir=metadata.TESTS_DIR) as tmp:
        tmpdir = Path(tmp)
        for dataset in DATASETS:
            source = metadata.DATA_DIR / dataset
            if not source.is_file():
                raise SystemExit(f"missing corpus; run `make test-data` first: {source}")
            input_bytes = source.stat().st_size
            input_mib = input_bytes / (1024 * 1024)
            for algo in metadata.ALGORITHMS:
                encoded = tmpdir / f"{algo.name}-{dataset}.dat"
                decoded = tmpdir / f"{algo.name}-{dataset}.out"
                encode_ms = time_command(
                    [str(algo.binary), "encode", str(source), str(encoded)]
                )
                decode_ms = time_command(
                    [str(algo.binary), "decode", str(encoded), str(decoded)]
                )
                if decoded.read_bytes() != source.read_bytes():
                    raise SystemExit(f"round-trip mismatch: {algo.name} {dataset}")
                out_bytes = encoded.stat().st_size
                encode_speed = input_mib / (encode_ms / 1000.0)
                decode_speed = input_mib / (decode_ms / 1000.0)
                ratio = out_bytes / input_bytes
                row = {
                    "algorithm": algo.name,
                    "language": "cpp",
                    "dataset": Path(dataset).stem,
                    "encodeTime": round(encode_ms, 1),
                    "decodeTime": round(decode_ms, 1),
                    "encodeSpeed": round(encode_speed, 1),
                    "decodeSpeed": round(decode_speed, 1),
                    "compressionRatio": round(ratio, 3),
                    "throughput": throughput_label(min(encode_speed, decode_speed)),
                }
                results.append(row)
                print(
                    f"{algo.name:<12} {Path(dataset).stem:<20} "
                    f"enc={row['encodeSpeed']:>7.1f} MiB/s  "
                    f"dec={row['decodeSpeed']:>7.1f} MiB/s  "
                    f"ratio={row['compressionRatio']:.3f}"
                )

    snapshot = {
        "generated": date.today().isoformat(),
        "version": "2.0.0",
        "results": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.TimeoutExpired as exc:
        print(f"timed out: {' '.join(exc.cmd)}", file=sys.stderr)
        raise SystemExit(1)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
