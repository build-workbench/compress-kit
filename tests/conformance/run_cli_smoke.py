#!/usr/bin/env python3
"""CLI smoke checks for every shipped CompressKit binary."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import metadata

USAGE_FRAGMENT = "encode|decode input output"
TIMEOUT_SECONDS = 10.0


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return metadata.run_cli(command, timeout=TIMEOUT_SECONDS)


def run_checked(command: list[str]) -> None:
    metadata.run_cli_checked(command, timeout=TIMEOUT_SECONDS)


def assert_usage(binary: Path) -> None:
    proc = run([str(binary)])
    combined = proc.stdout + proc.stderr
    if proc.returncode == 0:
        raise RuntimeError(f"{binary} unexpectedly succeeded without args")
    if "Usage:" not in combined or USAGE_FRAGMENT not in combined:
        raise RuntimeError(f"{binary} did not print expected usage")


def assert_wrong_arity(binary: Path, args: list[str], scenario: str) -> None:
    proc = run([str(binary), *args])
    combined = proc.stdout + proc.stderr
    if proc.returncode == 0:
        raise RuntimeError(f"{binary} unexpectedly succeeded {scenario}")
    if "Usage:" not in combined:
        raise RuntimeError(f"{binary} did not print usage {scenario}")


def assert_invalid_mode(binary: Path, source: Path, output: Path) -> None:
    proc = run([str(binary), "invalid", str(source), str(output)])
    combined = (proc.stdout + proc.stderr).lower()
    if proc.returncode == 0:
        raise RuntimeError(f"{binary} accepted invalid mode")
    if "mode" not in combined or "encode" not in combined or "decode" not in combined:
        raise RuntimeError(f"{binary} did not explain invalid mode")


def assert_missing_input(binary: Path, output: Path) -> None:
    missing = output.with_name("does-not-exist.bin")
    proc = run([str(binary), "encode", str(missing), str(output)])
    if proc.returncode == 0:
        raise RuntimeError(f"{binary} succeeded with a missing input file")
    if not proc.stderr.strip():
        raise RuntimeError(f"{binary} failed silently on a missing input file")


def assert_help(binary: Path) -> None:
    for flag in ("--help", "-h"):
        proc = run([str(binary), flag])
        if proc.returncode != 0:
            raise RuntimeError(f"{binary} exited {proc.returncode} for {flag}")
        if "Usage:" not in proc.stdout + proc.stderr:
            raise RuntimeError(f"{binary} did not print usage for {flag}")


def assert_round_trip(binary: Path, source: Path, encoded: Path, decoded: Path) -> None:
    run_checked([str(binary), "encode", str(source), str(encoded)])
    run_checked([str(binary), "decode", str(encoded), str(decoded)])
    if source.read_bytes() != decoded.read_bytes():
        raise RuntimeError(f"{binary.name} round-trip mismatch for {source.name}")


def assert_detects_corruption(binary: Path, source: Path, encoded: Path, decoded: Path) -> None:
    run_checked([str(binary), "encode", str(source), str(encoded)])
    blob = bytearray(encoded.read_bytes())
    blob[len(blob) // 2] ^= 0x40
    encoded.write_bytes(bytes(blob))
    proc = run([str(binary), "decode", str(encoded), str(decoded)])
    if proc.returncode == 0:
        raise RuntimeError(f"{binary.name} decoded a corrupted stream for {source.name}")


def main() -> int:
    metadata.require_binaries()
    corpus = [metadata.DATA_DIR / name for name in metadata.DEFAULT_CORPUS]
    large_corpus = [metadata.DATA_DIR / name for name in metadata.LARGE_CORPUS]

    missing_corpus = [p for p in corpus + large_corpus if not p.is_file()]
    if missing_corpus:
        raise SystemExit(f"missing corpus; run `make test-data` first:\n"
                         + "\n".join(str(p) for p in missing_corpus))

    checks = 0
    with tempfile.TemporaryDirectory(prefix=".cli-smoke-", dir=metadata.TESTS_DIR) as tmp:
        tmpdir = Path(tmp)
        for algo in metadata.ALGORITHMS:
            binary = algo.binary
            assert_usage(binary)
            checks += 1
            print(f"PASS usage {algo.name}")

            assert_help(binary)
            checks += 2
            print(f"PASS help {algo.name}")

            assert_wrong_arity(binary, ["encode", str(corpus[0])], "too few args")
            checks += 1
            assert_wrong_arity(binary, ["encode", str(corpus[0]), "o", "extra"], "too many args")
            checks += 1
            assert_invalid_mode(binary, corpus[0], tmpdir / f"{algo.name}.invalid")
            checks += 1
            assert_missing_input(binary, tmpdir / f"{algo.name}.missing.out")
            checks += 1
            print(f"PASS arg-validation {algo.name}")

            assert_detects_corruption(
                binary, corpus[0],
                tmpdir / f"{algo.name}-corrupt.enc", tmpdir / f"{algo.name}-corrupt.dec")
            checks += 1
            print(f"PASS corruption-detection {algo.name}")

            for source in corpus:
                enc = tmpdir / f"{algo.name}-{source.name}.enc"
                dec = tmpdir / f"{algo.name}-{source.name}.dec"
                assert_round_trip(binary, source, enc, dec)
                checks += 1
                print(f"PASS round-trip {algo.name} {source.name}")

            # Large (multi-MiB) corpus: catches coder failures that only
            # appear on long or incompressible inputs.
            for source in large_corpus:
                enc = tmpdir / f"{algo.name}-{source.name}.enc"
                dec = tmpdir / f"{algo.name}-{source.name}.dec"
                assert_round_trip(binary, source, enc, dec)
                checks += 1
                print(f"PASS round-trip {algo.name} {source.name} (large)")

    print(f"cli smoke passed: {checks} check(s)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.TimeoutExpired as exc:
        print(f"timed out after {exc.timeout}s: {' '.join(exc.cmd)}", file=sys.stderr)
        raise SystemExit(1)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(1)
