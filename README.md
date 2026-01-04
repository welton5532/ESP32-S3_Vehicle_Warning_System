
# Smart Vehicle Warning & Alcohol Detection System

This project leverages the powerful dual-core capabilities of the **ESP32-S3** to integrate a **64x64 HUB75 LED Matrix**, **MQ-3 Alcohol Sensor**, and **RemoteXY Bluetooth Control**. It creates an all-in-one smart vehicle safety device combining "Active Warning," "DUI Prevention," and "Remote Control."

The system is designed to solve the limitations of traditional warning triangles—such as **short visibility distance** due to passive reflection, the **high risk** of exiting the vehicle to place them, and **single-functionality**—effectively preventing secondary collisions.

---

## Repositories

The system was developed using a modular approach. Below are links to the individual modules and the final integrated version:

* **Final Integrated Version (Main Project)**
* **[ESP32-S3_Vehicle_Warning_System](https://www.google.com/search?q=https://github.com/welton5532/ESP32-S3_Vehicle_Warning_System)** - The complete system containing all features.


* **Sub-modules (Testing)**
* [ESP32-S3-MQ3-Alcohol-Sensor](https://www.google.com/search?q=https://github.com/welton5532/ESP32-S3-MQ3-Alcohol-Sensor) - Alcohol sensor ADC reading and calibration tests.
* [ESP32-S3-Chinese-Traditional-LED-Matrix](https://www.google.com/search?q=https://github.com/welton5532/ESP32-S3-Chinese-Traditional-LED-Matrix) - Traditional Chinese TTF font rendering and scrolling text tests.
* [ESP32-S3_RemoteXY_BLE_LED_Control](https://www.google.com/search?q=https://github.com/welton5532/ESP32-S3_RemoteXY_BLE_LED_Control) - Bluetooth interface control and menu logic tests.



---

## Project Structure

This project contains hardware schematics, software source code, and system resources. The directory structure is as follows:

```text
.
├── Circuit_and_PCB/
│   ├── ESP32-S3_Vehicle_Warning_schematic.kicad_sch  # KiCad Schematic Source File
│   └── ESP32-S3_Vehicle_Warning_schematic.png        # Schematic Preview Image
├── data/
│   └── font.ttf                                      # Pre-optimized Font File (Includes 4808 common Chinese chars)
├── src/
│   └── main.cpp                                      # Main Source Code
├── module_2d_image.excalidraw                        # System Connection Diagram (Excalidraw source)
├── partitions_custom.csv                             # Custom Partition Table (Allocates 5MB Flash for fonts)
├── platformio.ini                                    # PlatformIO Project Configuration File
└── README.md                                         # Project Documentation

```

---

## Table of Contents

1. [Motivation & Background](https://www.google.com/search?q=%23motivation--background)
2. [System Functionality](https://www.google.com/search?q=%23system-functionality)
3. [Hardware Architecture](https://www.google.com/search?q=%23hardware-architecture)
4. [Software Architecture](https://www.google.com/search?q=%23software-architecture)
5. [Installation Guide](https://www.google.com/search?q=%23installation-guide)
6. [Font Upload Guide](https://www.google.com/search?q=%23font-upload-guide) **(Crucial Step)**
7. [Operation Manual](https://www.google.com/search?q=%23operation-manual)
8. [Gallery & Demo](https://www.google.com/search?q=%23gallery--demo)

---

## Motivation & Background

1. **Solving "Secondary Collisions":** Traditional warning triangles rely on passive reflection, making them hard to see in rain, fog, or at night. Furthermore, they cannot convey specific information (e.g., Breakdown vs. Medical Emergency).
2. **Reducing Operational Risk:** Drivers risk their lives walking into traffic to place traditional triangles.
3. **The Solution:** A combination of Active LED Warning, Bluetooth Remote Control (stay inside the car), and Alcohol Detection.

---

## System Functionality

The system is controlled via the **RemoteXY** App over Bluetooth, featuring an intuitive menu-driven interface.

### App Interface Preview

*(Above: RemoteXY Bluetooth interface showing Alcohol readings and Warning controls)*

| Selector | Function | Details |
| --- | --- | --- |
| **1. Alcohol Tester** | **Alcohol Detector** | Supports **mg/L** and **PPM** units. When active, the Warning Light is forced OFF (Interlock). |
| **2. Triangle Light** | **Warning Triangle** | Controls the external LED strip (ON/OFF). |
| **3. Buzzer Alarm** | **Audio Alarm** | PWM linear volume control (0% ~ 100%). |
| **4. Matrix Brightness** | **Brightness** | Adjusts LED Matrix brightness (0% ~ 100%). |
| **5. Preset Messages** | **Preset Warnings** | Cycle through 9 modes: Accident, Breakdown, Temp Stop, Road Work, Fog Mode, SOS, etc. |
| **6. Custom Message** | **Custom Text** | Type any Traditional Chinese/English text to scroll instantly. |
| **7. Text Color** | **Text Color** | Switch between Rainbow, Red, Yellow, Green, Blue, White, etc. |
| **8. Text Speed** | **Scroll Speed** | Adjust scrolling speed (Level 1 ~ 10). |
| **9. Text Size** | **Text Size** | Dynamically adjust font size (8px ~ 60px). |

---

## Hardware Architecture

### Circuit Schematic (KiCad)
<img width="4200" height="2550" alt="image" src="https://github.com/user-attachments/assets/906c5ba1-b356-4466-bbe4-4c1d5c487d7d" />

*(Above: Complete circuit schematic including ESP32-S3, HUB75 interface)*

### Core Specifications

* **MCU**: Espressif **ESP32-S3-DevKitC-1U-N8R8**
* **Display**: 64x64 RGB HUB75 LED Matrix (P3)
* **Sensor**: MQ-3 Alcohol Gas Sensor
* **Power**: 5V 2A Power Bank + Independent Filtering Circuit

### Pin Mapping

| Module | Pin Name | ESP32-S3 GPIO | Note |
| --- | --- | --- | --- |
| **HUB75** | R1/G1/B1/R2/G2/B2 | 4/5/6/7/15/16 | Data Lines |
|  | A/B/C/D/E | 1/2/42/41/40 | Row Select |
|  | CLK/LAT/OE | 17/18/8 | Control |
| **Control** | **Shared Control** | **GPIO 2** | **Interlock Control** |
| **Sensor** | MQ-3 ADC | GPIO 1 | Analog Input |
| **Audio** | Buzzer | GPIO 42 | PWM Output |

---

## Software Architecture

* **Memory Optimization**: Uses `ps_calloc` to allocate graphics buffers in external PSRAM.
* **Smart Rendering**: Uses `OpenFontRender` to handle Traditional Chinese fonts.
* **Anti-Jamming**: Implements BLE Lazy Loading to prevent data congestion during sensor updates.
* **File System**: Uses LittleFS to store the font library.

---

## Installation Guide

1. Install **VS Code** and the **PlatformIO** extension.
2. `git clone` this repository.
3. Ensure drivers (CH343/CP210x) are installed on your computer.

---

## Font Upload Guide

**[IMPORTANT] This is the most critical step!**
The `data/` folder in this project comes with a pre-built **`font.ttf`**. This file has been optimized and includes:

* **Ministry of Education's 4808 Common Traditional Chinese Characters**
* **Common ASCII Characters**
* **Special Symbols (Degrees Celsius, Warning signs, Arrows, etc.)**

You **DO NOT** need to generate the font yourself. You simply need to upload it to the ESP32's Flash memory.

### Step 1: Upload via PlatformIO

1. Connect the ESP32-S3 to your computer.
2. Click the **PlatformIO Icon** (Alien head) in the VS Code sidebar.
3. In the **PROJECT TASKS** panel, expand:
* `esp32-s3-devkitc-1`
* `Platform`


4. Click **Upload Filesystem Image**.
5. PlatformIO will package the `data` folder and flash it to the board.

### Step 2: Verify

Once the terminal shows `SUCCESS`, restart the board. If the Serial Monitor shows `Font Loaded`, the system is ready.

---

## Operation Manual

1. **Startup**: Default mode is Alcohol Tester. Screen displays "Warming Up".
2. **Warning Mode**: Use the App to turn on the Triangle Light and select a Preset Message. The Alcohol Tester will power off automatically (Interlock).
3. **Customization**: You can change the text content, color, and size on the fly via the App.

---

## Gallery & Demo
![app_demo_ios](https://github.com/user-attachments/assets/f7ea3e33-943e-4b2c-b774-26eb039f4f1b)



### Live Demo Video

https://youtu.be/1AC4jYxGVUE?si=3yO8Ul78Wa-x0ozm

---

## Troubleshooting

* **Q: Screen is black/blank?** -> A: Please verify that you have performed the **Upload Filesystem Image** step to load the font.
* **Q: `Failed to mount LittleFS` error?** -> A: Check if `partitions_custom.csv` is correctly configured in `platformio.ini`.
* **Q: Bluetooth is laggy?** -> A: This is normal when the LED Matrix is refreshing at high speeds. The system prioritizes display stability.

---

*Created by Welton5532*
