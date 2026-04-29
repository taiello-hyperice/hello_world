# Pico W Temperature Monitor

A Raspberry Pi Pico W project that reads the onboard temperature sensor and serves the data over a lightweight HTTP server on your local network.

## Features

- Reads the Pico W's internal temperature sensor via ADC every ~1 second
- Hosts an HTTP server on port 80 that displays live temperature and free memory
- Auto-refreshes the web page every 2 seconds using JavaScript `fetch`
- Exposes a `/temp` endpoint returning raw CSV data (`temperature,free_memory`)
- Blinks the onboard LED at 2s during startup, solid on when connected and serving, fast 0.5s blink on error
- Tries two Wi-Fi networks in sequence (e.g. office + home) and falls back to the second if the first fails
- Prints temperature, even/odd status, free memory, and IP address over USB serial

## Wi-Fi Configuration

Wi-Fi credentials are kept out of version control. To configure:

1. Copy `wifi_config_example.h` to `wifi_config.h`:
   ```
   cp wifi_config_example.h wifi_config.h
   ```
2. Edit `wifi_config.h` and fill in your network details:
   ```c
   #define WIFI_SSID_1 "your_primary_network"
   #define WIFI_PASS_1 "your_primary_password"
   #define WIFI_SSID_2 "your_secondary_network"
   #define WIFI_PASS_2 "your_secondary_password"
   ```

`wifi_config.h` is listed in `.gitignore` and will never be committed.

## Building

Requires the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (v2.2.0) and the [VS Code Pico extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) or a manual CMake toolchain setup.

```bash
mkdir build && cd build
cmake ..
make
```

Flash the resulting `pico_w_temp_monitor.uf2` to your Pico W by holding BOOTSEL while connecting via USB, then drag-and-drop the file onto the mounted drive.

## Usage

1. Open a serial monitor (USB, 115200 baud) to see connection status and the assigned IP address.
2. Navigate to `http://<pico-ip-address>/` in a browser to see the live dashboard.
3. Query `http://<pico-ip-address>/temp` directly for raw data in the format `<temp_celsius>,<free_bytes>`.

## Hardware

- Raspberry Pi Pico W
- No external components required — uses the onboard temperature sensor and Wi-Fi
