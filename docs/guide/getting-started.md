# 快速开始

本指南将帮助您配置开发环境、构建实现并运行测试。

## 前置要求

### 必需工具

| 工具 | 最低版本 | 用途 |
|------|----------|------|
| g++ 或 clang++ | 9+ / 10+ | C++17 编译 |
| CMake | 3.16+ | 构建系统 |
| Python | 3.8+ | 测试编排与基准测试脚本 |
| Make | 任意版本 | 构建自动化 |

### 可选工具

| 工具 | 用途 |
|------|------|
| Node.js 20.19+ | 文档站点 |
| clang-format | C++ 代码格式化 |

### 安装方法

::: code-group

```bash [Ubuntu/Debian]
sudo apt update
sudo apt install g++ cmake python3 make
```

```bash [macOS (Homebrew)]
brew install gcc cmake python3 make
```

```bash [Windows (Chocolatey)]
choco install mingw cmake python3 make
```

:::

## 克隆与构建

### 克隆仓库

```bash
git clone https://github.com/build-workbench/compress-kit.git
cd compress-kit
```

### 构建所有实现

```bash
make build
```

`make build` 封装了 CMake 调用（`cmake -S . -B build && cmake --build build`），将所有算法实现编译到 `build/` 目录。

### 手动编译

如果您更喜欢直接调用 CMake：

```bash
cmake -S . -B build && cmake --build build
./build/huffman_cpp encode input.bin output.huf
./build/huffman_cpp decode output.huf restored.bin
```

## 运行测试

### 运行所有测试

```bash
make test
```

这将运行共享生命周期测试、CLI smoke、冻结格式 fixture 以及熵诊断单测。

## Makefile 命令参考

| 命令 | 描述 |
|------|------|
| `make build` | 构建所有算法实现（封装 CMake） |
| `make test` | 运行单元测试、CLI smoke、冻结 fixture 与熵诊断单测 |
| `make test-cli-smoke` | 运行 CLI smoke 一致性测试 |
| `make sanitize` | ASan/UBSan 构建并运行生命周期测试 |
| `make bench` | 刷新文档站基准快照 |
| `make stats` | 打印类文本语料的熵与各算法压缩比特/字节 |
| `make test-data` | 生成测试语料到 `tests/data/` |
| `make format` | 用 clang-format 格式化所有 C++ 代码 |
| `make lint` | clang-format dry-run 检查 |
| `make clean` | 删除 `build/`、`build-san/` 与 `tests/data/` |
| `make help` | 列出所有可用目标 |

## 故障排除

### C++ 编译错误

```bash
# 检查编译器版本
g++ --version  # 应为 9+

# 如果 g++ 失败，尝试 clang
clang++ -std=c++17 -O2 main.cpp -o huffman_cpp
```

### Range Coder 很大的文件

当前实现对 10 MiB 语料可以 round-trip。若要看吞吐，用 `make bench`，不要再截成 100 KiB 代替 Range 的正式结果。

## 下一步

- 阅读 [算法说明](/algorithms/huffman) 了解各算法原理与差异
- 查看 [架构概览](/architecture/) 了解系统分层与二进制格式
- 查看 [CHANGELOG](https://github.com/build-workbench/compress-kit/blob/main/CHANGELOG.md) 了解最新更新
