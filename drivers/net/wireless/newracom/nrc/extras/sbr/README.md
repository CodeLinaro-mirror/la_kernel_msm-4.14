# SBR (ESL) Test Package Guide

This directory contains a test package for SBR (Electronic Shelf Label) devices, providing the test application and firmware needed to perform AP integration testing with C5K devices.

## 1. File List

| File | Type | Description |
| :--- | :--- | :--- |
| `C5K_test_app.tar.gz` | **Test App** | Test application binary for C5K (Linux environment) |
| `sample_nrc5294r1.bin` | **Firmware** | Station (STA) firmware image |
| `example_config.json` | **Config** | Sample JSON configuration file for the test application |
| `UG-7394-001-SBR unified Test Guide (v1.0).pdf` | **Docs** | SBR unified test guide document |

## 2. Quick Start

### 1) Extract the test application
```bash
tar -xvzf C5K_test_app.tar.gz
```

### 2) Run the test application
Use the extracted `test` binary along with a configuration file (`config.json`):
```bash
./test -s2 -cconfig.json
```
> **Note:** The `-c` option specifies the path to the configuration file.

## 3. Notes
- **STA Firmware**: Flash `sample_nrc5294r1.bin` onto the target device by following the instructions in the test guide.
- **Configuration**: Copy `example_config.json` to `config.json` and adjust the settings to match your test environment before running the application.
- **Detailed Guide**: For full test scenarios and procedures, refer to `UG-7394-001-SBR unified Test Guide (v1.0).pdf`.
