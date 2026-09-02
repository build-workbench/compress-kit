# Tasks: add-lzss-algorithm

| ID | Task | Status |
|----|------|--------|
| CK-LZSS-001 | 实现 `algorithms/lzss/cpp/main.cpp`（编码器/解码器/CLI） | done |
| CK-LZSS-002 | 集成：constants/algorithms 声明、CMake target、metadata.py、test_lifecycle | done |
| CK-LZSS-003 | 生成 frozen fixtures（empty/byte42/ascii）+ manifest | done |
| CK-LZSS-004 | 文档：算法页 + 动手实验、架构表、README、对比页、基准刷新、CHANGELOG | done |

实现中发现并修复两个 bug：`dist=4096` 溢出 12 位字段（改为存 `dist-1`）；
编码器快速检查在匹配到达输入末尾时越界读（补充边界条件）。均由
ASan/lifecycle 与 CLI smoke 捕获。

验证入口：

```bash
make test        # lifecycle + CLI smoke（含 lzss round-trip）+ fixtures
make lint        # clang-format
make sanitize    # ASan/UBSan
make bench       # 刷新基准快照
cd docs && npm run build
```
