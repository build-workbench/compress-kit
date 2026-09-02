---
title: 架构概览
description: 系统分层、二进制格式与安全边界
---

# 架构概览

CompressKit 在算法核心之上仅保留一层薄 buffer 层：

```text
CLI 层 (cli_launcher.hpp)
  -> Buffer 层 (encode_buffer / decode_buffer，接收 BufferTransform 函数指针)
  -> 算法核心 (Huffman / Arithmetic / Range / RLE)
  -> 共享工具 (result / buffer_api / bit_io / frequency_table / serialization / checksum)
```

- 文件到文件 CLI 与内存调用方共享相同的 buffer 层路径。
- 单一 `BufferTransform` 函数指针别名将所有算法统一到同一签名。
- 共享的体积上限检查与错误码保持接口精简。

## 二进制格式总览

通用结构：`| Magic (4B) | Header | Payload | CRC-32 (4B LE) |`。CRC-32 覆盖其
之前的全部字节，解码前强制校验。

| 算法 | Magic | Header | Payload |
|------|-------|--------|---------|
| Huffman | `HFM2` | FreqCount (4B LE) + 257×4B LE 频率表 | 位流 |
| Arithmetic | `AEN2` | 同上 | 位流 |
| Range Coder | `RCN2` | 同上 | 字节流 |
| RLE | `RLE2` | — | `(count:u32 LE, value:u8)*` |

频率表顺序：符号 0-255（字节值）+ 符号 256（EOF），小端序，总大小 1028 字节。
Legacy magic（`HFMN`/`AENC`/`RCNC`）与无 magic 的 v1 RLE 一律拒绝。

## 安全边界

| 限制 | 值 | 目的 |
|------|-----|------|
| 编码输入上限 | 严格小于 1 GiB | 频率计数不回绕 |
| 解码输出上限 | 不超过 1 GiB | 防解压炸弹 |
| 压缩输入上限（仅解码） | 严格小于 8 GiB | RLE 最坏膨胀约 5×，为 round-trip 留空间 |
| 频率表总和上限（解码校验） | ≤ 2²⁴ | 熵编码除法不变量；超限或无 EOF 一律拒绝 |

详见 [C++ API](/api/cpp) 与 [快速开始](/guide/getting-started)。
