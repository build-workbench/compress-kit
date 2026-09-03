---
title: CompressKit
description: 使用现代 C++17 实现的经典无损压缩（熵编码）算法参考实现与评测基准
---

# CompressKit

使用现代 C++17 实现的经典无损压缩（熵编码）算法参考实现与评测基准。每个算法提供独立的命令行工具与 Buffer 接口，输出带魔数和 CRC-32 校验的确定性二进制流，支持严格的完整性验证与安全边界防护。

零第三方依赖，纯现代 C++ 标准库实现。

## 定位与适用场景

- **算法学习与参考实现**：结构清晰、遵循 RAII 的现代 C++ 熵编码范本，便于理解算法细节。
- **基准评测对照**：开箱即用的对比 Baseline，便于观察不同算法在不同数据分布下的压缩比与吞吐表现。
- **边界说明**：专注于经典算法的规范实现与对比验证，非工业级通用压缩器（生产环境如需极高吞吐与复合 LZ 压缩，请使用 Zstandard）。

| 算法 | 魔数 | 说明 |
|------|------|------|
| [Huffman 编码](/algorithms/huffman) | `HFM2` | 基于符号频率的最优前缀码 |
| [算术编码](/algorithms/arithmetic) | `AEN2` | 逼近香农熵极限的区间划分编码 |
| [区间编码](/algorithms/range) | `RCN2` | 算术编码的整数字节级实现 |
| [RLE 行程编码](/algorithms/rle) | `RLE2` | 对连续重复数据简单高效 |
| [LZSS 字典编码](/algorithms/lzss) | `LZS2` | 滑动窗口引用，gzip 系算法的 LZ 基础 |

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
