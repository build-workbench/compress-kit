#!/usr/bin/env python3
import os
from pathlib import Path
import random

# 统一的测试数据生成脚本
# 会在 tests/data/ 下生成多种分布的二进制测试文件：
# - random_1MiB.bin        随机数据（1 MiB）
# - random_10MiB.bin       随机数据（10 MiB）
# - repetitive_10MiB.bin   大量重复字节，适合 RLE 测试
# - textlike_10MiB.bin     类文本分布，偏向 ASCII 可见字符
# - fastq_10MiB.bin        FASTQ 风格测序数据（150bp reads、基因组碱基分布）
# - empty.bin              空文件
# - single_byte.bin        单字节边界样本
# - alternating.bin        交替字节模式
# - all_same_byte.bin      全相同字节样本（RLE/Huffman 边界情况）
# - small_dictionary_like.bin  小型重复词典风格样本

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "tests" / "data"


def ensure_dir():
    DATA_DIR.mkdir(parents=True, exist_ok=True)


def generate_random_file(path: Path, size_bytes: int):
    if path.exists() and path.stat().st_size == size_bytes:
        return
    print(f"[gen_testdata] generating random file: {path} ({size_bytes} bytes)")
    with path.open("wb") as f:
        remaining = size_bytes
        chunk_size = 1024 * 1024
        while remaining > 0:
            n = min(remaining, chunk_size)
            f.write(os.urandom(n))
            remaining -= n


def generate_repetitive_file(path: Path, size_bytes: int):
    if path.exists() and path.stat().st_size == size_bytes:
        return
    print(f"[gen_testdata] generating repetitive file: {path} ({size_bytes} bytes)")
    rng = random.Random(1)
    with path.open("wb") as f:
        written = 0
        while written < size_bytes:
            value = rng.randrange(0, 256)
            # 每个 run 的长度在 [4, 4096] 之间，保证有明显重复
            run_len = rng.randint(4, 4096)
            if written + run_len > size_bytes:
                run_len = size_bytes - written
            f.write(bytes([value]) * run_len)
            written += run_len


def generate_textlike_file(path: Path, size_bytes: int):
    if path.exists() and path.stat().st_size == size_bytes:
        return
    print(f"[gen_testdata] generating text-like file: {path} ({size_bytes} bytes)")
    rng = random.Random(2)
    alphabet = ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" "  \n.,;:!?-_")
    alphabet_bytes = alphabet.encode("ascii")
    table = bytes(alphabet_bytes[i % len(alphabet_bytes)] for i in range(256))
    with path.open("wb") as f:
        remaining = size_bytes
        chunk_size = 1024 * 1024
        while remaining > 0:
            n = min(remaining, chunk_size)
            f.write(rng.randbytes(n).translate(table))
            remaining -= n


def generate_literal_file(path: Path, data: bytes):
    if path.exists() and path.read_bytes() == data:
        return
    print(f"[gen_testdata] generating literal file: {path} ({len(data)} bytes)")
    path.write_bytes(data)


def generate_fastq_file(path: Path, size_bytes: int):
    """FASTQ-like corpus: 150 bp reads, genome-like base distribution,
    Phred+33 quality strings (mean ~Q35, like Illumina)."""
    if path.exists() and path.stat().st_size == size_bytes:
        return
    print(f"[gen_testdata] generating fastq-like file: {path} ({size_bytes} bytes)")
    rng = random.Random(3)
    bases = b"ACGTN"
    base_probs = (0.30, 0.20, 0.20, 0.29, 0.01)  # ~human genome, 1% N
    read_len = 150
    read_id = 0
    with path.open("wb") as f:
        written = 0
        while written < size_bytes:
            seq = bytes(rng.choices(bases, weights=base_probs, k=read_len))
            qual = bytes(
                33 + max(0, min(40, 35 + rng.randint(-6, 4))) for _ in range(read_len)
            )
            record = f"@read_{read_id}\n".encode() + seq + b"\n+\n" + qual + b"\n"
            if written + len(record) > size_bytes:
                record = record[: size_bytes - written]  # last record may truncate
            f.write(record)
            written += len(record)
            read_id += 1


def main():
    ensure_dir()
    generate_random_file(DATA_DIR / "random_1MiB.bin", 1 * 1024 * 1024)
    generate_random_file(DATA_DIR / "random_10MiB.bin", 10 * 1024 * 1024)
    generate_repetitive_file(DATA_DIR / "repetitive_10MiB.bin", 10 * 1024 * 1024)
    generate_textlike_file(DATA_DIR / "textlike_10MiB.bin", 10 * 1024 * 1024)
    generate_fastq_file(DATA_DIR / "fastq_10MiB.bin", 10 * 1024 * 1024)
    generate_literal_file(DATA_DIR / "empty.bin", b"")
    generate_literal_file(DATA_DIR / "single_byte.bin", b"\x00")
    generate_literal_file(DATA_DIR / "alternating.bin", (b"\xAA\x55" * 512))
    generate_literal_file(DATA_DIR / "all_same_byte.bin", (b"\x00" * 4096))
    generate_literal_file(
        DATA_DIR / "small_dictionary_like.bin",
        (b"compresskit-dict-alpha\n" * 128)
        + (b"compresskit-dict-beta\n" * 128)
        + (b"compresskit-dict-gamma\n" * 128),
    )
    print("[gen_testdata] done")


if __name__ == "__main__":
    main()
