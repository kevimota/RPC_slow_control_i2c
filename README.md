# RPC FEB Slow Control

ESP32-S2 based I2C slow control for RPC Front-End Boards (FEBs). Controls 4 FEBs, each with:

- 2× AD5316 quad DAC (10-bit, 4 ch)
- 2× AD7417 quad ADC + temperature sensor (10-bit, 4 ch)
- 1× PCF8574A 8-bit I/O expander (DAC chip-select + enable)

## Hardware

### I2C Bus

| Pin  | Function |
|------|----------|
| SDA  | GPIO 10  |
| SCL  | GPIO 11  |
| Freq | 100 kHz  |

### I2C Address Map

| Device         | Address(es)            | Notes                                      |
|----------------|------------------------|--------------------------------------------|
| PCF8574A       | `0x38` + 2×FEB         | FEB 0: `0x38`, FEB 1: `0x3A`, etc.        |
| AD7417 ADC 0   | `0x28` + 2×FEB + 0     | Chip 0 on each FEB                         |
| AD7417 ADC 1   | `0x28` + 2×FEB + 1     | Chip 1 on each FEB                         |
| AD5316 DAC     | **`0x0C` (shared)**    | All 8 DACs share address; selected via PCF |

### DAC Addressing

All 8 AD5316 chips share I2C address `0x0C`. The PCF8574A I/O expanders provide hardware chip-select:

- **Bits 0-1** (global, written to all 4 PCFs): Active-low chip-select. Only the target (FEB, chip) gets its bit cleared; all others are set high.
- **Bits 2-3** (per-FEB): Active-low power enable. `enDac()` clears, `disDac()` sets.

Write sequence: `selDac(feb, chip)` → `enDac(feb, chip)` → `writeRaw(channel, code)`.

### Channel Signal Names

| Chip 0 | Chip 1 | Signal  |
|--------|--------|---------|
| Ch 0   | Ch 0   | Vth1/Vth3 |
| Ch 1   | Ch 1   | Vth2/Vth4 |
| Ch 2   | Ch 2   | VMon1/VMon3 |
| Ch 3   | Ch 3   | VMon2/VMon4 |

## Firmware Architecture

```
src/main.cpp
include/
├── PCF8574A.h       I/O expander (read/write, DAC select/enable)
├── AD7417.h         ADC + temp sensor (readADC, readTemp)
├── AD5316.h         DAC protocol (voltageToCode, writeRaw)
├── FEBManager.h     Orchestrates one FEB (PCF + 2×ADC + DAC)
├── Config.h         LittleFS-backed WiFi credential storage
├── WiFiManager.h    AP "RPC-FEB-Control" + STA dual mode
└── WebServer.h      ESPAsyncWebServer + REST API
data/
├── index.html       Dashboard (all 4 FEBs)
├── feb.html         Per-FEB detail/control page
├── wifimanager.html WiFi credentials form
├── style.css        Dark theme
└── app.js           Live updates, DAC control, config import/export
```

### DAC Closed-Loop Calibration

`FEBManager::setDAC()` writes the DAC, reads the corresponding ADC channel, and iteratively adjusts the DAC code (up to 5 iterations) until the measured voltage is within 5 mV of the target. This compensates for DAC non-linearity and reference inaccuracies.

## Web UI

### REST API

| Route | Method | Params | Description |
|---|---|---|---|
| `/api/status` | GET | — | JSON with all FEBs: ADC, temp, PCF, DAC targets |
| `/api/dac` | POST | `feb`, `chip`, `channel`, `voltage` | Set single DAC channel (closed-loop) |
| `/api/dac/enable` | POST | `feb`, `chip` | Enable DAC chip |
| `/api/dac/disable` | POST | `feb`, `chip` | Disable DAC chip |
| `/api/dac/setall` | POST | `voltage`, `type` (threshold\|vmon) | Set Vth or VMon on all FEBs |
| `/api/dac/setfeb` | POST | `feb`, `voltage`, `type` (threshold\|vmon) | Set Vth or VMon on one FEB |
| `/api/config/export` | GET | — | Download `dac_config.json` |
| `/api/config/import` | POST | `data` (urlencoded JSON) | Upload and apply config file |
| `/wifi` | POST | `ssid`, `pass` | Save WiFi credentials & restart |

### API Response Format (`/api/status`)

```json
{
  "febs": [
    {
      "id": 0,
      "pcf": 3,
      "temp": [25.5, 25.3],
      "adc": [[1200,2400,3600,4800],[1250,2450,3650,4850]],
      "dac": {
        "target": [[2500,220,1800,0],[2500,220,0,0]],
        "enabled": [true, true]
      }
    }
  ],
  "wifi": {
    "ip": "192.168.4.1",
    "ssid": "RPC-FEB-Control (AP)",
    "connected": false
  }
}
```

## Build & Upload

```bash
# Build firmware
pio run

# Upload filesystem (HTML/CSS/JS/fonts)
pio run --target uploadfs

# Upload firmware
pio run --target upload

# Both
pio run --target uploadfs && pio run --target upload
```

### PlatformIO Environments

| Env | Board | I2C Pins |
|---|---|---|
| `esp32-s2-saola-1` | ESP32-S2 Saola-1 | SDA=10, SCL=11 |
| `esp32dev` | ESP32 Dev Module | SDA=21, SCL=22 |

## First Boot

1. Power the board — it creates an AP named `RPC-FEB-Control`
2. Connect to that WiFi network
3. Open `http://192.168.4.1` in a browser
4. Go to the **WiFi** page, enter your STA credentials, click Save
5. Board restarts and connects to your network (AP remains active)
6. Access via `http://rpc-feb-control.local` (mDNS) or the IP shown on the dashboard

## Config Export/Import

The **Export Config** button downloads a JSON file with all DAC voltages and enable states. **Import Config** loads a previously exported file. Both buttons are in the top nav bar.

Format:
```json
{"v":1,"f":[
  {"t":[[2500,220,1800,0],[2500,220,0,0]],"e":[1,1]},
  ...
]}
```
