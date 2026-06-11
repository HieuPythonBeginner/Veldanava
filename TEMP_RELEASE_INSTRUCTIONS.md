# Veldanava (Temporary build for download)

This archive is intended as a **temporary** snapshot so others can download and run the current state.

## Contents
- `build/` (pre-built artifacts from this repo snapshot)
- `./build/veldanc` executable
- source + tests + docs (for reference)

## How to run
From the folder where you extracted the archive:

```bash
./build/veldanc tests/test_genesis_full.veldanava
```

Expected output:
- Lines printing: `0`, `1`, `2`
- Then: `[OK] Executed tests/test_genesis_full.veldanava`

## If the executable fails
Try rebuilding:

```bash
cmake -B build -S .
cmake --build build -j 4 --target veldanc
```

Then re-run the command above.

