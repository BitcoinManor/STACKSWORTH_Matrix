# ⚡ STACKSWORTH Matrix v1.01  
**Open Source • Web Flashable • Bitcoin at a Glance**

![STACKSWORTH Banner](https://github.com/YourUser/STACKSWORTH_Matrix_v1.01/raw/main/assets/stacksworth_banner.png)

Welcome to **STACKSWORTH**, the future of open-source Bitcoin displays.  
Track real-time price, block height, mempool fees, and weather — all from a sleek, plug-and-play LED matrix that’s built for Bitcoiners, by Bitcoiners.

---

## 🚀 Quick Start

- **🔌 Flash Instantly:**  
  [StacksWorth Web Flasher →](https://bitcoinmanor.github.io/BLOKDBIT_WebFlasher/) *(custom URL coming soon)*

- **📦 Order Units or Kits:**  
  [stacksworth.com](https://stacksworth.com)

- **🛠 Full Source Code + Firmware:**  
  [STACKSWORTH GitHub](https://github.com/BitcoinManor/STACKSWORTH_Matrix_v1.01)

- **🎉 Follow Us:**  
  [Twitter/X](https://x.com/BitcoinManor) | [Instagram](https://www.instagram.com/bitcoinmanor/)

---

## 💡 What Is STACKSWORTH Matrix?

The **STACKSWORTH Matrix** is a self-contained, open-source Bitcoin display system featuring:

- 🧠 Real-time Bitcoin metrics
- 🔥 Dual-row LED scrolling text
- 🌐 Easy AP-mode configuration (WiFi, city, timezone)
- 💻 Full Arduino-based firmware for developers
- 🟩 Web flashable from any modern browser

Built for signal, not noise.

---

## 🧱 Metrics Displayed

- **💰 BTC Price (USD)** — via CoinGecko  
- **📦 Block Height** — via Blockchain.info  
- **🚦 Fee Rate (sat/vB)** — via Mempool.space  
- **🌤 Local Weather + Temp** — via OpenWeatherMap  
- **⏰ Time** — via NTP

Future versions will include:
- 🏷️ Miner Tag Detection  
- 📶 MAC ID–based SSID Broadcasting  
- 🎲 Weighted Animation Mode (Smash Buy Button)

---

## 🛠 Tech Stack

- **Hardware:** ESP32 (NodeMCU or similar), 64x16 LED matrix
- **Firmware:** Arduino/C++  
- **Web Config:** SPIFFS + AsyncWebServer  
- **APIs:**
  - [`CoinGecko`](https://www.coingecko.com/en/api)
  - [`Blockchain.info`](https://blockchain.info)
  - [`Mempool.space`](https://mempool.space)
  - [`Blockstream.info`](https://blockstream.info)
  - [`OpenWeatherMap`](https://openweathermap.org)

---

## 🧰 Dev Setup

```bash
# 1. Clone the repo
git clone https://github.com/BitcoinManor/STACKSWORTH_Matrix_v1.01.git

# 2. Open the .ino in Arduino IDE

# 3. Install these libraries:
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"
#include "Font_Data.h" // Optional, if using custom fonts
#include "time.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
DNSServer dnsServer;
#include <Preferences.h>
Preferences prefs;

# 4. Upload sketch to your ESP32,then make certain the html.gz file is inside the data folder
# 5. Use "ESP32 Sketch Data Upload" under Tools to flash /data folder to SPIFFS
# 6. Enter the SW-MATRIX network and add your SSID, Password, City, TimeZone and hit the Save button.
 It will reboot and connect to your WiFi



## 🙏 Data Sources & Attribution
STACKSWORTH proudly leverages the power of open APIs to deliver real-time Bitcoin data:

💱 CoinGecko – for reliable Bitcoin price data

🧱 blockchain.info – for current block height

🚦 mempool.space – for accurate fee estimates

Huge thanks to these projects for their dedication to open data and transparency in the Bitcoin ecosystem.
We encourage supporting them by contributing, using their services respectfully, or self-hosting when possible.

Built by [Bitcoin Manor](https://bitcoinmanor.com) for the sovereign individual.

- **Inspired by Hal Finney Ticker by Ben (arcbtc)**
- **Bitcoin data powered by CoinGecko & Mempool.space & blockchain.info**.
- LED Matrix Libraries: MajicDesigns
- Thank you to the Bitcoin community

---

## 📜 License

MIT License — use freely, fork often, flash everything.
⚖️ 

**Powered by Bitcoin.** Not just another gadget. This is **signal**.

STACKSWORTH_Matrix_v1.01 | Bitcoin Manor
Open Source | Web Flashable | Self-Sovereign
https://github.com/BitcoinManor/STACKSWORTH_Matrix_v1.01

