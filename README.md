# AmcacheParser

Cross-platform implementation of Amcache.hve parser.

This project is a fully compatible, cross-platform alternative to the original AmcacheParser, designed to parse Windows Amcache.hve registry hive files and extract forensic artifacts on any operating system.

## Overview

AmcacheParser reads Windows Amcache.hve files — registry hives that store application installation metadata, file execution evidence, and device/driver information. This implementation supports both the **Windows 7/8 (old)** and **Windows 10+ (new)** Amcache formats, producing the same CSV output structure as the reference implementation.

## Key Features

- **Cross-platform** — works on Windows, macOS, and Linux
- **Format compatible** — identical CSV column ordering and CLI behavior
- **No external dependencies** — all third-party libraries (CLI11, fmt, spdlog) are vendored in `third_party/`
- **Stand-alone** — single binary with no runtime installation requirements
- **CMake-based build** — simple `mkdir build && cmake .. && make` workflow

## Building

```bash
make build
```

The binary will be available at `build/AmcacheParser`.

## Usage

```bash
./AmcacheParser -f /path/to/Amcache.hve --csv /output/directory
```

All standard options are supported: whitelist/blacklist filtering, precision timestamps, transaction log handling, and more.
