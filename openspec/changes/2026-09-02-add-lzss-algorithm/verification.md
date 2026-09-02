# Verification: add-lzss-algorithm

## Commands

```bash
make test        # 生命周期 round-trip + corrupt-input、CLI smoke（含 fastq 大语料）、fixtures
make lint        # clang-format dry-run
make sanitize    # ASan/UBSan lifecycle
make bench       # 基准快照含 lzss × 4 语料
cd docs && npm run build
```

## Manual checks

```bash
./build/lzss_cpp encode tests/data/repetitive_10MiB.bin /tmp/r.lzs
ls -l /tmp/r.lzs                       # 预期极小（run 被 18B match 吸收）
./build/lzss_cpp decode /tmp/r.lzs /tmp/r.out && cmp tests/data/repetitive_10MiB.bin /tmp/r.out

./build/lzss_cpp encode tests/data/random_1MiB.bin /tmp/x.lzs
ls -l /tmp/x.lzs                       # 预期 ≈ 1.125×（literal + flag 开销）

# 破坏：翻转一字节后解码必须失败
python3 -c "
d = bytearray(open('/tmp/r.lzs','rb').read()); d[10] ^= 0xFF
open('/tmp/broken.lzs','wb').write(bytes(d))"
./build/lzss_cpp decode /tmp/broken.lzs /tmp/o.bin; echo $?   # 非零 + checksum mismatch
```

## Expected results

- 全部语料 round-trip 一致（empty / byte42 / ascii / 256 全表 / sequential / lcg / 10MiB 语料集）；
- 随机输入输出 ≈ 1.125×（最坏膨胀 9/8，写入文档）；
- 快照 `docs/.vitepress/data/benchmarks.json` 含 `lzss` 四语料行。
