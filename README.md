# CompressKit

<p align="center">
  <strong>使用 C++17 实现的经典无损压缩算法。</strong>
</p>

<p align="center">
  <a href="https://github.com/build-workbench/compress-kit/actions/workflows/ci.yml"><img src="https://github.com/build-workbench/compress-kit/actions/workflows/ci.yml/badge.svg" alt="CI Status"></a>
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/许可证-MIT-green.svg" alt="License"></a>
</p>

CompressKit 是一个面向学习与验证的压缩算法仓库：同一组经典算法用
C++17 实现，通过统一命令行契约和 round-trip 测试验证正确性。
它不是黑盒压缩库，而是可以阅读、运行、对比和验证的算法实验室。

📖 **在线文档**：<https://build-workbench.github.io/compress-kit/>

文档站包含算法详解、架构设计、API 参考与基准测试结果。如需本地预览：

```bash
cd docs && npm ci && npm run dev
```

## 包含内容

| 算法 | 适用场景 |
|------|----------|
| Huffman 编码 | 通用文本/数据，学习前缀码 |
| 算术编码 | 理解熵编码与压缩率对比 |
| 区间编码 | 对比算术编码风格实现 |
| RLE 行程编码 | 高重复数据与简单格式学习 |

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
