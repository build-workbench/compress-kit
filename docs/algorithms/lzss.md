---
title: LZSS 字典编码
description: 滑动窗口 + 哈希链的字典压缩、格式与动手实验
---

# LZSS 字典编码

LZSS（Lempel-Ziv-Storer-Szymanski）是 LZ77 的教学化形态：把"刚才出现过的
片段"替换为**距离 + 长度**引用，而不是为每个符号建立概率模型。它和
Huffman/Arithmetic/Range 是完全不同的思路——**字典编码抓跨符号的结构，
熵编码抓符号的分布**。gzip/deflate 的 LZ 部分就是 LZ77 的后代。

## 工作原理

编码器维护一个 4 KiB 滑动窗口：处理到位置 `pos` 时，在窗口内寻找与
当前数据最长匹配的片段。匹配长度 ≥ 3 时发一个 match（2 字节：12 位距离 +
4 位长度），否则发 1 字节 literal。每 8 个符号用一个 flag 字节告诉解码器
每个符号是 literal 还是 match：

```
flag bit = 1 → literal（1 字节原样）
flag bit = 0 → match（2 字节：dist-1 (12 bit) | len-3 (4 bit)）
```

`AAAA...` 这样的数据：第一个 `A` 是 literal，之后每个 18 字节的块只需
一个 `(dist=1, len=18)` 引用——这就是 RLE 的"通用版"（不限连续，任意
距离的重复都能引用）。

## 本仓库实现

- 固定内存：16 位 3 字节前缀哈希表 + 窗口大小的前驱环，**辅助内存 O(1)**，
  与输入大小无关（输入可达接近 1 GiB）。
- 匹配搜索：哈希链最多比较 32 个候选；候选距离按时间从近到远，一旦超出
  窗口即终止。
- 匹配区间内的每个位置都插入哈希表，使后续匹配可以跨匹配延伸。
- 匹配长度上限 18（4 位字段），距离上限 4096（12 位字段存 `dist-1`，
  恰好覆盖整个窗口）。
- 重叠匹配（`dist < len`）用逐字节复制解码。
- 无 legacy 版本：非 `LZS2` 前缀一律报 bad magic。

## 文件格式

| 字段 | 大小 | 描述 |
|------|------|------|
| Magic | 4 字节 | `LZS2` |
| flag 字节 | 1 字节 | 8 个符号一组，bit i (LSB) = 1 表示 literal |
| literal | 1 字节 | 原样字节 |
| match | 2 字节 | 小端 `u16`：`dist-1` (12 bit) \| `len-3` (4 bit) |
| CRC-32 | 4 字节 | 小端序，覆盖此前全部字节；解码前强制校验 |

## 命令行用法

```bash
./build/lzss_cpp encode input.bin output.lzs
./build/lzss_cpp decode output.lzs restored.bin
```

## 动手实验

以下命令默认已执行 `make build` 与 `make test-data`；数字为 2.0.0 实测值，量级不应变化。

### 实验 1：字典编码看到熵编码看不到的结构

```bash
./build/lzss_cpp encode tests/data/repetitive_10MiB.bin /tmp/repetitive.lzs
./build/huffman_cpp encode tests/data/repetitive_10MiB.bin /tmp/repetitive.huf
ls -l /tmp/repetitive.lzs /tmp/repetitive.huf
```

LZSS 约 1.2 MB（0.119×），Huffman 约 10.5 MB（0.998×，几乎没压动）。
repetitive 语料由 4-4096 字节的 run 组成：Huffman 的逐字节频率模型看不见
"这段是那段的重复"，而 LZSS 的 `(dist, len)` 引用把它吸收掉了。这就是
LZ77 在 1977 年论文里回答的问题。

### 实验 2：无结构数据的 12.5% 膨胀

```bash
./build/lzss_cpp encode tests/data/random_1MiB.bin /tmp/random.lzs
ls -l /tmp/random.lzs
```

输出约 1.18 MB ≈ 1.125×。随机数据几乎没有 3 字节匹配，每个字节都以
literal 输出，外加每 8 个字节 1 个 flag 字节——`9/8 = 1.125` 是 LZSS
的最坏膨胀，远小于 RLE 的 5×，但仍需在文档里诚实标注。

### 实验 3：与熵编码互补（诚实对比）

```bash
./build/lzss_cpp encode tests/data/fastq_10MiB.bin /tmp/fastq.lzs
./build/arithmetic_cpp encode tests/data/fastq_10MiB.bin /tmp/fastq.aen
ls -l /tmp/fastq.lzs /tmp/fastq.aen
```

LZSS 约 5.8 MB（0.553×），算术编码约 4.8 MB（0.453×）——**这次熵编码
赢了**。FASTQ 语料的 read 之间没有真实重复（随机生成），质量字符串的
3 字节偶然匹配抵不过 flag 字节开销。LZSS 不是"更好的压缩器"，它是
**另一种工具**：真实测序数据有 read 间冗余时两者叠加（deflate 的 LZ +
Huffman）才接近现代压缩器。

### 实验 4：CRC 完整性

```bash
cp /tmp/repetitive.lzs /tmp/repetitive-broken.lzs
printf '\xff' | dd of=/tmp/repetitive-broken.lzs bs=1 seek=10 conv=notrunc
./build/lzss_cpp decode /tmp/repetitive-broken.lzs /tmp/out.bin; echo "exit=$?"
```

解码应失败并输出 `checksum mismatch`。注意 match 引用的是"已解码字节"，
篡改一个 flag 字节可能让整段输出错位——CRC 是解码前必须完成的第一道检查。

## 复杂度

| 方面 | 复杂度 | 说明 |
|------|--------|------|
| 时间（编码） | O(n·C) | C = 链长上限 32 × 匹配长度 18 |
| 时间（解码） | O(n) | 单次遍历，逐字节复制 |
| 空间 | O(1) | 哈希表 64 KiB + 窗口环 16 KiB，与输入无关 |

## 注意事项

- 窗口 4 KiB、匹配上限 18 字节是教学参数；现代 LZ 系（zstd/brotli）用
  MB 级窗口、更长匹配和熵后端，差距就在这些参数里。
- 对无结构数据膨胀 9/8；不要对已压缩或随机数据使用。
- 连续长重复数据上 RLE 更优（专用模型）；LZSS 的优势在**任意距离**的重复。
- 与熵编码是互补关系，不是替代关系——deflate = LZ77 + Huffman 才是
  gzip 的完整答案。
- 详见 [基准测试](/benchmarks/results) 与 [架构概览](/architecture/)。
