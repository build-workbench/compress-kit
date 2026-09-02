# 基准测试结果

交互式图表读取仓库内的 `docs/.vitepress/data/benchmarks.json`。数字以该快照为准；换机器后请 `make bench` 刷新。

## 交互式性能图表

<BenchmarkChart />

## 生成数据模型

| JSON 位置 | 字段 | 含义 |
|----------|------|------|
| 顶层 | `generated` | 快照日期 |
| 顶层 | `version` | 对应的项目版本 |
| `results[]` | `algorithm`、`dataset` | 图表坐标 |
| `results[]` | `encodeTime`、`decodeTime` | 挂钟时间，毫秒 |
| `results[]` | `encodeSpeed`、`decodeSpeed` | 吞吐量，MiB/s |
| `results[]` | `compressionRatio`、`throughput` | 输出/输入比值，以及粗粒度吞吐标签 |

## 当前数据集

| 数据集键 | 图表标签 |
|----------|----------|
| `textlike_10MiB` | 类文本 (10 MiB) |
| `repetitive_10MiB` | 重复数据 (10 MiB) |
| `random_1MiB` | 随机 (1 MiB) |
| `fastq_10MiB` | FASTQ 测序 (10 MiB) |

四个算法使用同一组文件。`fastq_10MiB` 是 150 bp reads、基因组碱基分布的
FASTQ 风格语料（A/C/G/T + Phred 质量字符串），熵编码器可压到约 0.45×
（DNA 序列本身只需 2 bits/碱基）；RLE 在随机数据上会膨胀（比值大于 1）；
这些都是格式契约，不是测量错误。

## 刷新

```bash
make bench
cd docs && npm run build
```

提交时带上生成的 JSON，不要手改本页里的数字。

## 另见

- [如何运行基准测试](/benchmarks/how-to-run)
- [算法说明](/algorithms/huffman) — 各算法原理与文件格式
- 熵对照：`make stats` 打印输入的 Shannon 熵与各算法实际比特/字节
