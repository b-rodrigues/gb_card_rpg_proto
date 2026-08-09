# Automated Tests

This directory contains test suites and validation scripts.

## Running Tests

Run:
```bash
make test
```

This target builds the ROM and verifies that:
1. Compilation succeeds without errors.
2. Linking completes successfully.
3. The output file `build/game.gb` exists and is non-empty.
4. ROM header checksums and headers pass `rgbfix -v` validation.
