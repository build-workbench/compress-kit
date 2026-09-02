---
title: 区间编码
description: 算术编码的整数字节级实现与文件格式
---

# 区间编码

区间编码（Range Coder）是算术编码的**整数、字节输出**变体。它维护半开区间
`[low, low+range)`，在 `range` 足够窄或顶字节已确定时输出一个字节并左移状态。
压缩率与算术编码接近，I/O 以字节为单位，吞吐更高。

## 工作原理

```cpp
uint32_t low = 0, range = UINT32_MAX, total = cumFreq.back();
for (uint8_t s : data) {
    uint32_t r = range / total;          // 重归一化保证 range >= 2^24 >= total
    low += r * cumFreq[s];
    range = r * (cumFreq[s + 1] - cumFreq[s]);
    while ((low ^ (low + range)) < (1u << 24) || range < (1u << 24)) {
        if ((low ^ (low + range)) >= (1u << 24))
            range = -low & ((1u << 24) - 1);   // 跨边界时先对齐端点
        output_byte(low >> 24);
        low <<= 8; range <<= 8;
    }
}
```

> 关键点：只在「顶字节相同」时移位是不充分的——当区间已经很窄且恰好跨越顶字节
> 边界时，不移位会使 `range` 跌破精度下限，导致后续符号子区间塌缩为 0，在接近
> 不可压缩的数据上静默丢数据。循环条件必须同时检查 `range` 是否仍低于下限。

## 本仓库实现

- 32 位状态，每次重归一化输出 1 字节。
- 与算术编码共享 `frequency_table`：静态模型、EOF、缩放到 `2^24`。
- 解码用二分查找定位符号（累积频率表长度 258）。
- v1 magic `RCNC` 被拒绝；v2 `RCN2` 修复了窄区间跨边界的精度塌缩问题，
  旧 `RCNC` payload 不能用当前解码器恢复。

## 文件格式

| 字段 | 大小 | 描述 |
|------|------|------|
| Magic | 4 字节 | `RCN2` (0x52 0x43 0x4E 0x32) |
| 频率表大小 | 4 字节 | 小端 uint32（始终 257） |
| 频率表 | 257 × 4 字节 | 小端 uint32 数组（符号 0-255 + EOF） |
| 编码数据 | 可变 | 字节流（重归一化区间输出） |
| CRC-32 | 4 字节 | 小端序，覆盖此前全部字节；解码前强制校验 |

## 命令行用法

```bash
./build/rangecoder_cpp encode input.bin output.rcnc
./build/rangecoder_cpp decode output.rcnc restored.bin
```

## 动手实验

以下命令默认已执行 `make build` 与 `make test-data`；数字为 2.0.0 实测值，量级不应变化。

### 实验 1：字节输出的熵代价

```bash
make stats
```

`range` 行的 `bits/byte` 比 `entropy` 列高约 0.38（textlike 语料），明显高于
`arithmetic` 行的 +0.001。同样的静态模型，区间编码每次重归一化**输出一个
整字节**而不是逐位输出，吞吐更高但压缩率略逊——这就是"速度与压缩率的
取舍"在真实数字上的样子。

### 实验 2：交替字节流上对比算术编码

```bash
python3 -c "open('/tmp/ab.bin','wb').write(b'AB' * 524288)"
./build/rangecoder_cpp encode /tmp/ab.bin /tmp/ab.rcn
./build/arithmetic_cpp encode /tmp/ab.bin /tmp/ab.aen
ls -l /tmp/ab.rcn /tmp/ab.aen
```

区间编码约 263 KB，算术编码约 132 KB（2 倍差距）。`ABAB...` 熵恰为 1 bit/byte
（1 MiB → 128 KiB），算术编码几乎贴着熵界，区间编码因字节粒度输出付出
约 1 bit/byte 的额外代价。

### 实验 3：CRC 完整性

```bash
cp /tmp/ab.rcn /tmp/ab-broken.rcn
printf '\xff' | dd of=/tmp/ab-broken.rcn bs=1 seek=10 conv=notrunc
./build/rangecoder_cpp decode /tmp/ab-broken.rcn /tmp/out.bin; echo "exit=$?"
```

解码应失败并输出 `checksum mismatch`。与算术编码一样，区间状态是上下文
相关的，损坏不会停留在单个符号——CRC 校验必须在任何解析之前完成。

## 复杂度

| 方面 | 复杂度 | 说明 |
|------|--------|------|
| 时间（编码/解码） | O(n) | 字节级 I/O 比位级更快 |
| 空间 | O(σ) | 累积频率表 |
| 精度 | 固定 | 32 位整数 |

## 注意事项

- 当前实现对 10 MiB 语料可在 CLI smoke 超时内 round-trip；基准请用 `make bench`
  在同一机器重跑，不要沿用旧的小样本快照。
- 与算术编码都不是自适应模型；若要 order-1 / PPM 需要新的格式世代。
- 详见 [基准测试](/benchmarks/results) 与 [架构概览](/architecture/)。
