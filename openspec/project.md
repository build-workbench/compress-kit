# compress-kit - Project Context

## Identity

- **Canonical repository**: `build-workbench/compress-kit`
- **Version**: 2.0.0
- **Lifecycle**: educational/hobby; breaking changes allowed
- **Language**: C++17 (header-only algorithm library + CLI executables)

## Core contracts

| Capability | Path | Description |
|---|---|---|
| `binary-formats` | `openspec/specs/binary-formats/` | v2 binary format identities, CRC, size limits |

## External boundaries

- **`fq-compressor` (C++)** and **`fq-compressor-rust`**: use compress-kit's
  algorithms as referenced educational material. No runtime dependency.
- **Decision `CK-DEC-001`**: v2 uses `HFM2/AEN2/RCN2/RLE2`; legacy magic
  rejected; release `2.0.0`.

## Validation commands

```bash
make lint
make test
```

## Authority rules

- `algorithms/` source is the implementation source of truth.
- Models must not commit, push, create PRs, or publish without explicit
  authorization.
- High-risk changes (format, compatibility) use lightweight OpenSpec changes.
