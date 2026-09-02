---
title: CompressKit
description: 使用 C++17 实现的四种经典无损压缩算法
---

# CompressKit

使用 C++17 实现的四种经典无损压缩算法。每个算法提供独立的命令行工具，
输出带 CRC-32 校验的二进制流，可完整 round-trip。

| 算法 | 魔数 | 说明 |
|------|------|------|
| [Huffman 编码](/algorithms/huffman) | `HFM2` | 基于频率的最优前缀码 |
| [算术编码](/algorithms/arithmetic) | `AEN2` | 逼近熵极限的区间编码 |
| [区间编码](/algorithms/range) | `RCN2` | 算术编码的整数、字节级实现 |
| [RLE 行程编码](/algorithms/rle) | `RLE2` | 对连续重复数据简单高效 |

## 快速开始

```bash
git clone https://github.com/build-workbench/compress-kit.git
cd compress-kit
make build
make test
```

每个算法的命令行接口一致：

```bash
./build/<binary> <encode|decode> <input> <output>
```

## 导航

- [快速开始指南](/guide/getting-started) — 环境配置、构建、测试
- [算法说明](/algorithms/huffman) — 各算法原理、用法与文件格式
- [架构概览](/architecture/) — 系统分层与二进制格式
- [C++ API](/api/cpp) — 共享头文件与 Buffer 门面
- [基准测试](/benchmarks/results) — 性能数据与对比
- [更新日志](https://github.com/build-workbench/compress-kit/blob/main/CHANGELOG.md)
