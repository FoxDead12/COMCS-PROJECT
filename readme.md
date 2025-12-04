# COMCS.Lda - Setup & Compilation

This document outlines the steps to configure the development environment, compile the code, and deploy the components for the COMCS.Lda monitoring solution.

## 1. Infrastructure Setup

### MQTT Broker (HiveMQ)

Ensure a HiveMQ broker is running and insert the server link in main.h and main.ino. Create the following users for authentication:

    - **UDP Server:** `alert-server` / `COMCSPassword1.`
    - **ESP32 Client:** `comcs2526cagg11` / `COMCSPassword1.`
    - **Pico Client:** `raspberry` / `COMCSPassword1.`

### Node-RED Command Centre

1.  **Install Node-RED:** Via virtual machine from the given template.
2.  **Install Dashboard:** Manage Palette -> Install `node-red-dashboard`.
3.  **Import Flows:** Import the provided `flows.json` file.
4.  **Configure MQTT:** Update the MQTT configuration nodes with your HiveMQ broker address and the credentials created above.

---

## 2. Arduino IDE Environment Setup (Boards & Libraries)

To compile the firmware for the ESP32 and Raspberry Pi Pico, you must configure the Arduino IDE as follows:

### Step 1: Configure Board Manager URLs

1.  Open Arduino IDE and go to **File > Preferences**.
2.  In the "Additional Boards Manager URLs" field, add the following link:
    ```text
    [https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json)
    ```
3.  Click **OK**.

### Step 2: Install Board Packages

1.  Go to **Tools > Board > Boards Manager...**
2.  Search for and install the following packages:
    - **ESP32:** `esp32` by Espressif Systems.
    - **Pico:** `Raspberry Pi Pico/RP2040` by Earle F. Philhower.

### Step 3: Install Required Libraries

Go to **Tools > Manage Libraries...** and install the exact versions of the following dependencies:

- **PubSubClient** by _Nick O'Leary_ (for MQTT communication).
- **DHT sensor library** by _Adafruit_ (requires **Adafruit Unified Sensor**).
- **ArduinoJson** by _Benoit Blanchon_ (for SmartData model serialization).
- **WiFi** (Built-in for ESP32).

---

## 3. UDP Alert Server (Linux/C)

### Dependencies

Install the required C libraries for JSON parsing and MQTT:

```bash
sudo apt-get update
sudo apt-get install build-essential libjson-c-dev libpaho-mqtt-dev
gcc main.c -o main -ljson-c -lpaho-mqtt3cs
./main
```
