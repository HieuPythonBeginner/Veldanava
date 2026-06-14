# Veldanava Temporary Build Notes

This archive is a temporary snapshot of the current experimental Veldanava compiler state.

## Contents

- `build/` pre-built artifacts from this repo snapshot
- `./build/veldanc` executable
- Source, tests, and docs for reference

## Current status

Veldanava can build and run small programs, but it is not a complete compiler yet.

Known limitations:

- `sanction` enforces arithmetic operators and non-built-in calls; real backend policy can still be expanded.
- `Incorporate` has minimal module tracking; known math functions require `genesis Incorporate "math";`.
- `ownership::init()` is currently a no-op.
- Class/struct support is incomplete.
- Legacy tests have been migrated to current syntax.

## Build

```bash
cmake -B build -S .
cmake --build build -j 4 --target veldanc
```

## Run smoke test

From the extracted folder:

```bash
./build/veldanc tests/test_genesis_full.veldanava
```

Expected output:

```text
0
1
2
[OK] Executed tests/test_genesis_full.veldanava
```

## Current sanctioned syntax

```veldanava
Primordial_Regalia;

sanction:
    plus;
    myfunc();
    myclass();
;
```

Duplicate names are rejected within the same shape. `plus;` and `plus();` are allowed because they are different shapes.

Sectioned sanction syntax is intentionally rejected:

```veldanava
sanction:
    operators:
        plus;
    funcs:
        myfunc();
    oop:
        myclass();
;
```
