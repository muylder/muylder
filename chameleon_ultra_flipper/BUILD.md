# 🔨 Build Instructions

## Prerequisites

### Install uFBT

```bash
pip install ufbt
```

### Update SDK

```bash
ufbt update
```

## Building

### Quick Build

```bash
cd chameleon_ultra_flipper
ufbt
```

This will:
1. Compile all C files
2. Link libraries
3. Create `chameleon_ultra.fap` in `dist/` folder

### Clean Build

```bash
ufbt clean
ufbt
```

### Build and Install

```bash
ufbt launch
```

This will build and copy to connected Flipper via USB.

## Troubleshooting

### Error: "furi.h not found"

Make sure you've run:
```bash
ufbt update
```

### Error: "Unknown type FuriHalBtSerial"

Your SDK may be outdated. Momentum firmware is recommended:
```bash
ufbt update --channel dev
```

### Error: Icon not found

Create a placeholder icon:
```bash
cd assets/icons
# Create a simple 10x10 icon or use the placeholder
```

### Build fails with linker errors

Check that all function declarations match implementations:
```bash
grep -r "function_name" --include="*.h" --include="*.c"
```

## Advanced Build Options

### Build for specific target

```bash
ufbt COMPACT=1 DEBUG=0
```

### Generate compile_commands.json

```bash
ufbt compile_commands
```

This helps with IDE autocomplete.

### Build with verbose output

```bash
ufbt VERBOSE=1
```

## Size Optimization

Current binary size: ~30-40KB (estimated)

To reduce size:
1. Remove unused features
2. Use `-Os` optimization
3. Strip debug symbols
4. Reduce string constants

```bash
ufbt COMPACT=1
```

## Testing

### Lint

```bash
ufbt lint
```

### Format

```bash
ufbt format
```

### Static Analysis

```bash
ufbt lint_py
```

## Deployment

### Copy to SD Card

After building:
```bash
cp dist/chameleon_ultra.fap /path/to/sd/apps/RFID/
```

### Install via qFlipper

1. Build: `ufbt`
2. Open qFlipper
3. Navigate to SD Card > apps > RFID
4. Drag and drop `chameleon_ultra.fap`

## Build for Release

```bash
# Clean build
ufbt clean

# Build with optimizations
ufbt COMPACT=1 DEBUG=0

# Verify size
ls -lh dist/chameleon_ultra.fap

# Create release package
tar -czf chameleon_ultra_v0.1.0.tar.gz dist/chameleon_ultra.fap README.md LICENSE
```

## Continuous Integration

Example GitHub Actions workflow:

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install uFBT
        run: pip install ufbt
      - name: Update SDK
        run: ufbt update
      - name: Build
        run: ufbt
      - name: Upload artifact
        uses: actions/upload-artifact@v3
        with:
          name: chameleon_ultra.fap
          path: dist/chameleon_ultra.fap
```
