# CompressKit

<p align="center">
  <strong>使用现代 C++17 实现的经典无损压缩（熵编码）算法参考实现与评测基准。</strong>
</p>

<p align="center">
  <a href="https://github.com/build-workbench/compress-kit/actions/workflows/ci.yml"><img src="https://github.com/build-workbench/compress-kit/actions/workflows/ci.yml/badge.svg" alt="CI Status"></a>
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/许可证-MIT-green.svg" alt="License"></a>
</p>

CompressKit 涵盖 **Huffman 编码、算术编码 (Arithmetic Coding)、区间编码 (Range Coding) 与 RLE**。提供统一的二进制流规范（魔数 + 频率表 + CRC-32 校验）、严格的安全边界检查与可复现的评测基准。

零外部依赖，纯现代 C++ 标准库实现。

📖 **在线文档**：<https://build-workbench.github.io/compress-kit/>

文档站包含算法详解、二进制格式规范、API 参考与交互式基准测试图表。如需本地预览：

```bash
cd docs && npm ci && npm run dev
```

## 目标与定位

- **算法学习与实现参考**：网上现存的算术/区间编码实现多为早年遗留代码（缺乏现代规范与抽象）。CompressKit 提供代码结构规范、注释清晰、遵循 RAII 的现代 C++ 实现，适合作为学习资料或无依赖的代码参考。
- **评测对照与基准（Baseline）**：提供统一的 CLI 契约与测试语料，便于在自研或对比新编码策略时，直观对照不同经典算法在各类数据分布下的压缩率与吞吐量。
- **明确非目标（Non-goals）**：本项目专注于经典算法的规范实现与对比验证，非工业级通用压缩器。如需生产环境极致吞吐与 LZ 字典压缩，请使用 Zstandard 或 libdeflate。

## 包含内容

| 算法 | 魔数 | 特点与适用场景 |
|------|------|----------------|
| Huffman 编码 | `HFM2` | 基于符号频率的最优前缀码，适合通用文本与前缀码原理验证 |
| 算术编码 | `AEN2` | 逼近香农熵极限的区间划分位流编码，适合理解高压缩率熵编码原理 |
| 区间编码 | `RCN2` | 算术编码的整数字节级变体，适合对比吞吐量与工程实现差异 |
| RLE 行程编码 | `RLE2` | 针对连续重复数据的极简编码，格式直观、开销低 |
| LZSS 字典编码 | `LZS2` | 基于滑动窗口的 LZ 字典编码，gzip 系算法的 LZ 基础，适合重复片段数据 |

所有命令行工具都遵循：

```bash
<binary> <encode|decode> <input> <output>
```

## 快速开始

```bash
git clone https://github.com/build-workbench/compress-kit.git
cd compress-kit

make build
make test
```

快速 round-trip 验证：

```bash
printf "Hello CompressKit\n" > input.txt
./build/huffman_cpp encode input.txt output.huf
./build/huffman_cpp decode output.huf restored.txt
diff input.txt restored.txt
```

## 仓库结构

```text
algorithms/
  huffman/cpp/      # Huffman 编码 CLI
  arithmetic/cpp/   # 算术编码 CLI
  range/cpp/        # 区间编码 CLI
  rle/cpp/          # RLE 行程编码 CLI
  lzss/cpp/         # LZSS 字典编码 CLI
  shared/cpp/       # 公共库（序列化、位读写、频率表、CLI 框架）
docs/               # VitePress 中文文档站
tests/              # 测试语料生成与 CLI smoke 测试
```

## 工程基线

| 命令 | 用途 |
|------|------|
| `make build` | 构建全部 C++ CLI 工具（CMake） |
| `make test` | 运行单元测试与 CLI smoke 测试 |
| `make lint` | clang-format dry-run |

## 许可证

[MIT 许可证](LICENSE) · 版权所有 © 2024-2026 encoding contributors
