# Design: LZSS binary format and implementation

## Stream layout

```
| Magic "LZS2" (4B) | symbol stream | CRC-32 (4B LE) |
```

CRC-32 覆盖其之前全部字节（与其余四算法同一契约，`verify_crc32` 复用）。

## Symbol stream

每 8 个符号一组，组首一个 flag 字节：

| flag bit (LSB first) | 符号编码 | 大小 |
|----------------------|---------|------|
| 1 | literal：1 字节原样 | 1 B |
| 0 | match：`u16 LE = (dist-1) \| ((len-3) << 12)` | 2 B |

- 最后一组不足 8 个符号时，flag 高位无对应数据，解码端按 `pos >= content`
  提前终止，不读取多余字节。
- match 的 `dist`（距离）1-based：`dist=1` 表示前一个字节，上限 4096（4 KiB
  窗口）。字段存 `dist-1`（0..4095），12 位恰好覆盖整个窗口——若直接存
  `dist`，`dist=4096` 会溢出 12 位被解码成 0。
- `len ∈ [3, 18]`（4 bit 存 `len-3`）。`len < 3` 不发 match：2 字节 match
  对 3 字节 literal + flag 均摊（约 3.375 B）才划算。

## Parameters

| 参数 | 值 | 理由 |
|------|----|------|
| `LZSS_MAGIC` | `"LZS2"` | v2 命名风格 |
| `WINDOW_SIZE` | 4096 | LZSS 论文经典值；12-bit dist |
| `MIN_MATCH` | 3 | 低于则不划算 |
| `MAX_MATCH` | 18 | 4-bit len 上限 |
| `HASH_BITS` | 16 | 3 字节前缀哈希表 65536 槽 |
| `MAX_CHAIN` | 32 | 每位置最多比较 32 个候选，防退化 |

## Encoder (memory-bounded)

输入可达接近 1 GiB，**禁止 O(n) 辅助数组**。使用固定内存环形结构：

- `head[65536]`：3 字节前缀哈希 → 最近位置；
- `prev[WINDOW]`：环形槽位（`pos % WINDOW`）的前驱位置。

每位置：哈希 3 字节前缀 → 沿链（最多 32 个候选）找最长匹配，候选距离
按链表顺序递增，一旦 `dist > WINDOW` 即终止（后续更旧）。匹配区间内的
每个位置也插入哈希表（否则后续匹配无法延伸）。输出按 8 符号一组打包。

最坏时间复杂度 O(n × 32 × 18)，对 10 MiB 语料秒级。

## Decoder

```
precheck_magic(input, LZSS_MAGIC, "LZSS", use_legacy_check=false)
content = verify_crc32(input, "LZSS")
verify_magic(...)
while pos < content:
    flag = data[pos++]
    for i in 0..7:
        if pos >= content: break
        if flag bit i set:  emit literal
        else:               emit match (2 B)
```

match 复制允许重叠（`dist < len` 时逐字节拷贝，如 `AAAA` 模式）。

## Rejection contract

| 条件 | 错误 |
|------|------|
| 非 `LZS2` 前缀 | `LZSS: bad magic` |
| 流短于 magic | `LZSS: input too short` |
| match 对截断 | `LZSS: truncated match pair` |
| `dist == 0` 或 `dist > out.size()` | `LZSS: match distance out of range` |
| 输出超过 1 GiB | `SizeLimitError`（`ERR_SIZE_LIMIT`） |
| 输入 ≥ 1 GiB（编码） | `LZSS: input too large` |

空输入合法：输出 `LZS2` + CRC（13 字节？4+4=8 字节，与 RLE 空流一致为
8 字节：magic 4 + CRC 4）。

## Expected corpus behavior

| 语料 | 预期 |
|------|------|
| textlike_10MiB | 弱匹配，ratio ≈ 0.8-0.9（flag 均摊 1/8 + 短匹配） |
| repetitive_10MiB | 极好：run 被 18 字节 match 吸收 |
| fastq_10MiB | 质量串/`@read_` 前缀可匹配，优于纯熵编码 |
| random_1MiB | ≈ 1.125（全 literal + flag），最坏膨胀 9/8 |
