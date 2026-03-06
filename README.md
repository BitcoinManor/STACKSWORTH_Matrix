# ⚡ STACKSWORTH Matrix  
**Open Source • Web Flashable • Bitcoin Metrics Display • Bitcoin at a Glance**

![STACKSWORTH Banner](https://github.com/BitcoinManor/STACKSWORTH_Matrix/raw/main/assets/stacksworth_banner.png)

Welcome to **STACKSWORTH**, the future of open-source Bitcoin displays.  
Watch Bitcoin's Pulse at a Glance in real time  — price, block height, mempool fees, sats/dollar and weather — all from a sleek, plug-and-play LED matrix 
that’s built for Bitcoiners, by Bitcoiners.
****WORKING TO PROVIDE OTA ABILITIES TO MAKE THINGS EASIER FOR THE USER, Coming MARCH 2026---

## 🚀 Latest Firmware: v1.0.2

🆕 MAC-based AP ID (e.g. `SW-MATRIX-E4E204`)  
🌐 Matching ID in hotspot, HTML portal, and network list  
🛠️ Improved UX for multi-device setups and flashing

📦 [Download v1.0.2 ZIP](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/releases/download/v1.0.2/STACKSWORTH_MATRIX_v1.0.2.zip)  
📓 [See Changelog](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/blob/main/CHANGELOG.md)

---

## 🚀 Quick Start

- **🔌 Flash Instantly:**  
  [StacksWorth Web Flasher →](https://bitcoinmanor.github.io/STACKSWORTH_WebFlasher) *(custom URL coming soon)*

- **📦 Order Units or Kits:**  
  [stacksworth.com](https://stacksworth.com)

- **🛠 Full Source Code + Firmware:**  
  [STACKSWORTH GitHub](https://github.com/BitcoinManor/STACKSWORTH_Matrix)

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
ALL METRICS DISPLAYED USING OUR SATONAK SELF SOVEREIGN SERVER

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
- We use our own SatoNak Server and thank the others below that we use for our backups
- SATONAK API'S
  - - - [`Satonak Price`](https://www.satonak.bitcoinmanor.com/api/price)
  - - - [`Satonak Block Height`](https://www.satonak.bitcoinmanor.com/api/height)
  - - - [`Satonak CAD Price`](https://satonak.bitcoinmanor.com/api/price?fiat=CAD)
  - - - [`Satonak Fee`](https://satonak.bitcoinmanor.com/api/fee)
  - - - [`Satonak Hashrate`](https://satonak.bitcoinmanor.com/api/hashrate)
  - - - [`Satonak Circulating Supply`](https://satonak.bitcoinmanor.com/api/circsupply)
  - - - [`Satonak Miner`](https://satonak.bitcoinmanor.com/api/miner)
  - - - [`Satonak 24hr Price Chamge`](https://satonak.bitcoinmanor.com/api/change24h)
  - - - [`Satonak EUR Price`](https://satonak.bitcoinmanor.com/api/price?fiat=EUR)
        OTHER BACKUP API's  
- - [`CoinGecko`](https://www.coingecko.com/en/api)
  - [`Blockchain.info`](https://blockchain.info)
  - [`Mempool.space`](https://mempool.space)
  - [`Blockstream.info`](https://blockstream.info)
  - [`OpenWeatherMap`](https://openweathermap.org)

---

## 🧰 Dev Setup

```bash
# 1. Clone the repo
git clone https://github.com/BitcoinManor/STACKSWORTH_Matrix.git

# 2. Open the .ino in Arduino IDE

# 3. Install these libraries:
MD_Parola.h  
MD_MAX72xx.h  
SPI.h  
WiFiManager.h  
HTTPClient.h  
ArduinoJson.h  
esp_task_wdt.h  
Font_Data.h  
time.h  
FS.h  
SPIFFS.h  
ESPAsyncWebServer.h  
AsyncTCP.h  
DNSServer.h  
Preferences.h

# 4. Upload sketch to your ESP32, then make certain the html.gz file is inside the data folder
# 5. Use "ESP32 Sketch Data Upload" under Tools to flash /data folder to SPIFFS
# 6. Enter the SW-MATRIX network and add your SSID, Password, City, TimeZone and hit the Save button.
#    It will reboot and connect to your WiFi
```

---
## 📜 License

MIT License — use freely, fork often, flash everything.
⚖️ 


---

## 🙏 Data Sources & Attribution
STACKSWORTH proudly leverages the power of open APIs to deliver real-time Bitcoin data:

💱 CoinGecko – for reliable Bitcoin price data

🧱 blockchain.info – for current block height

🚦 mempool.space – for accurate fee estimates

Huge thanks to these projects for their dedication to open data and transparency 
in the Bitcoin ecosystem.
We encourage supporting them by contributing, using their services respectfully, 
or self-hosting when possible.

Built by [Bitcoin Manor](https://bitcoinmanor.com) for the sovereign individual.

- **Inspired by Hal Finney Ticker by Ben (arcbtc)**
- **Bitcoin data powered by CoinGecko & Mempool.space & blockchain.info**.
- LED Matrix Libraries: MajicDesigns
- Thank you to the Bitcoin community

---

**Powered by Bitcoin.** Not just another gadget. This is **signal**.

Built with ⚡ by [Bitcoin Manor](https://bitcoinmanor.com)
Open Source | Web Flashable | Self-Sovereign | Made for Bitcoiners
https://github.com/BitcoinManor/STACKSWORTH_Matrix
[github.com/BitcoinManor/STACKSWORTH_MATRIX](https://github.com/BitcoinManor/STACKSWORTH_MATRIX)
