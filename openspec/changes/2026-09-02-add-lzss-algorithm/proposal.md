# Change Proposal: add-lzss-algorithm

## Metadata

- Status: `Draft`
- Repository: `build-workbench/compress-kit`
- Capability: `lzss`（新增）；继承 `binary-formats` 契约
- Task IDs: `CK-LZSS-001`, `CK-LZSS-002`, `CK-LZSS-003`, `CK-LZSS-004`

## Why

教学拼图缺一块：四个算法全是**熵编码**（Huffman / Arithmetic / Range /
RLE 的行程模型也属于"符号统计"思路），没有任何**字典编码**。"经典 vs 现代"
对比页的实测已经证明：repetitive 语料上 zstd/gzip（LZ77 系）与 ck-rle 同量级，
但 ck-huffman/arithmetic 完全失效（ratio ≈ 1.0）——逐字节频率模型看不到
跨符号结构。LZSS（Lempel-Ziv-Storer-Szymanski）是 LZ77 的教学化形态：
滑动窗口 + 哈希链 + flag 位打包，代码量小、可读性强，补上"字典编码"这
一块后，仓库才覆盖完整的压缩思想谱系（字典 / 熵 / 行程）。

## Changes

**Add LZSS as the fifth algorithm**

- From: 无字典编码算法。
- To: `lzss_cpp` CLI + `lzss_encode_buffer` / `lzss_decode_buffer`，遵循 v2
  格式契约：4 字节 magic `LZS2` + 编码流 + 小端 CRC-32 trailer。
- Reason: 教学谱系完整性；FASTQ 语料（fq-compressor 语境）上可直观演示
  字典编码对结构数据的优势。
- Impact: 纯 additive。既有四算法的 magic/格式/文件不变；LZSS 无 legacy
  版本，解码器对任何非 `LZS2` 前缀报 bad magic（与 RLE 相同策略）。

## Format design summary

```
| Magic "LZS2" (4B) | 符号流 | CRC-32 (4B LE) |
```

符号流：每 8 个符号一个 flag 字节（LSB 优先，1=literal，0=match）；
literal 1 字节原样；match 2 字节（dist 12-bit LE + (len-3) 4-bit），
dist ∈ [1, 4095]（4 KiB 窗口），len ∈ [3, 18]。空输入只写 magic + CRC。

## Scope

- LZSS 编码器（哈希链匹配，O(1) 辅助内存）、解码器、CLI；
- 生命周期测试 + CLI smoke + frozen fixtures（empty / byte42 / ascii）；
- 文档：算法页（含动手实验）、架构表、README、对比页、基准、CHANGELOG；
- `lzss` capability spec + `binary-formats` 主规格的 magic 表更新。

## Out of scope

- 不改变现有四算法的任何格式或行为；
- 不做 streaming、自适应模型、更优匹配（btree/后缀数组）；
- 不新增 v1 legacy 识别（LZSS 是全新格式代）。

## Compatibility and rollback

纯 additive：v2 既有流不受影响；`LZS2` 不与任何已知 magic 冲突
（classify_magic 对未知 magic 返回 Unknown → bad magic）。若需回滚，
删除 LZSS 相关文件与文档条目即可，无格式牵连。

## Approval

- 待维护者评审后批准。
