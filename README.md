# 🌤️ IoT Smart Weather & Environmental Clock

> A standalone, real-time IoT smart clock powered by an ESP32. This device integrates a hardware GPS module for dynamic location tracking, fetches live weather telemetry via the OpenWeatherMap API, and monitors local ambient conditions using high-precision I2C sensors.

## 🚀 Features

*   **Global Time Synchronization:** Accurate timekeeping achieved by configuring Network Time Protocol (NTP) over Wi-Fi.
*   **Dynamic Weather Fetching:** Parses real-time outdoor weather data (HTTP payload) from the OpenWeatherMap API via `ArduinoJson` based on live GPS coordinates.
*   **Local Environmental Monitoring:** Interfaces with an SHT31 sensor via I2C to provide highly accurate indoor temperature and humidity readings.
*   **Non-Blocking UI Architecture:** Implements a robust `millis()` state machine, allowing smooth capacitive touch UI page transitions and display updates without disrupting background sensor polling or data fetching.
*   **Clear Visual Display:** Renders multi-page data elegantly on an SSD1306 OLED display.

## 🛠️ Hardware Components

*   **Microcontroller:** ESP32 Development Board
*   **Display:** SSD1306 OLED Display (I2C)
*   **Environment Sensor:** SHT31 Temperature & Humidity Sensor (I2C)
*   **Location Tracking:** Hardware GPS Module (e.g., NEO-6M / ATGM336H)
*   **User Input:** Capacitive Touch Sensor

## 🔌 Hardware Wiring (Pinout Mapping)

| Component | ESP32 Pin | Interface |
| :--- | :--- | :--- |
| **SHT31 / OLED** | SDA (e.g., GPIO 8) | I2C |
| **SHT31 / OLED** | SCL (e.g., GPIO 9) | I2C |
| **GPS Module** | RX -> TX (e.g., GPIO 20) | UART |
| **GPS Module** | TX -> RX (e.g., GPIO 21) | UART |
| **Touch Sensor** | Digital Input Pin | GPIO |

*(Note: Pin definitions can be adjusted in the source code according to the specific ESP32 board variant.)*

## 💻 Software & Libraries

This project is written in **C/C++** and utilizes the following key libraries:
*   `WiFi.h` - Network connectivity
*   `NTPClient` - Time synchronization
*   `ArduinoJson` - API HTTP payload parsing
*   `Adafruit_SHT31` - Environmental sensor control
*   `Adafruit_SSD1306` & `Adafruit_GFX` - Display rendering
*   `TinyGPS++` - GPS NMEA sentence parsing

## 📸 Demo & Gallery

<img width="2448" height="3264" alt="IMG20260319172443" src="https://github.com/user-attachments/assets/8e509a23-c1e1-49c5-84ca-eb30b863ad57" />
<img width="2448" height="3264" alt="IMG_20260623_221506" src="https://github.com/user-attachments/assets/c405b563-dac6-4692-a68f-8fe5f1acb5c9" />


## ⚙️ Getting Started

### Prerequisites
1. Clone this repository to your local machine.
2. Open the project in the Arduino IDE or PlatformIO.
3. Install the required libraries via the Library Manager.

### Configuration (Crucial Step)
To run this project, you need to input your own Wi-Fi credentials and API key. 
Locate the configuration section in the source code and update the following variables:

```cpp
const char* ssid     = "YourWiFi";
const char* password = "YourPassword";
String openWeatherMapApiKey = "YourAPIKey";
