# ⚡ STACKSWORTH Matrix  
**Open Source • Web Flashable • Bitcoin Metrics Display • Bitcoin at a Glance**

![STACKSWORTH Banner](https://github.com/BitcoinManor/STACKSWORTH_Matrix/raw/main/assets/stacksworth_banner.png)

Welcome to **STACKSWORTH**, the future of open-source Bitcoin displays.  

**This is Bitcoin’s Pulse, at a glance.**

Watch Bitcoin’s pulse live — price, block height, fees, sats per dollar, time, and more — all from a sleek plug-and-play LED matrix built for Bitcoiners.

---

## 🚀 Latest Firmware: v2.0.69

### 🔥 Major Improvements
- ✅ **Manual OTA Updates (Stable)**  
- 🔄 **Improved Stability & Error Handling**  
- 🧠 **Smart Cached Display System**  
- 🔌 **Safer Boot Sequence (Reduced Power Draw)**  
- 🌐 **Improved WiFi Reconnect Logic**  
- 🆔 **MAC-Based Device ID System**  

📦 [Download Firmware](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/releases)  
📓 [View Changelog](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/blob/main/CHANGELOG.md)

---

## ⚡ Quick Start

- 🔌 **Flash in Browser:**  
  https://bitcoinmanor.github.io/STACKSWORTH_WebFlasher  

- 🌐 **Connect to Device:**  
  Join `SW-MATRIX-XXXXXX`  

- ⚙️ **Configure:**  
  Enter WiFi, City, Timezone  

- 🚀 **Done:**  
  Device reboots and displays live Bitcoin data  

---

## 💡 What Is STACKSWORTH Matrix?

The **STACKSWORTH Matrix** is a self-contained Bitcoin display that delivers real-time data in a clean, always-on format.

Built for:
- desks  
- offices  
- retail environments  
- Bitcoin meetups  

No noise. Just signal.

---

## 📊 Metrics Displayed

Powered by our **SatoNak self-sovereign API** with fallback sources:

- 💰 BTC Price  
- 📦 Block Height  
- 🚦 Fee Rate (sat/vB)  
- 🏭 Mining Pool  
- 📈 24H Change  
- 🧮 Circulating Supply  
- ⚡ Hashrate  
- 🌤 Weather + Temperature  
- ⏰ Time / Date  

---

## 🌐 SatoNak API Endpoints

Self-hosted, sovereign data layer:

- Price → https://satonak.bitcoinmanor.com/api/price  
- Price (CAD) → https://satonak.bitcoinmanor.com/api/price?fiat=CAD  
- Price (EUR) → https://satonak.bitcoinmanor.com/api/price?fiat=EUR  
- Block Height → https://satonak.bitcoinmanor.com/api/height  
- Fee → https://satonak.bitcoinmanor.com/api/fee  
- Hashrate → https://satonak.bitcoinmanor.com/api/hashrate  
- Circulating Supply → https://satonak.bitcoinmanor.com/api/circsupply  
- Miner → https://satonak.bitcoinmanor.com/api/miner  
- 24H Change → https://satonak.bitcoinmanor.com/api/change24h  

---

## 🌐 Backup Data Sources

- CoinGecko  
- mempool.space  
- blockchain.info  
- blockstream.info  
- OpenWeatherMap  

---

## 🛠 Tech Stack

- ESP32 (NodeMCU or similar)
- 64×16 LED Matrix (MAX7219)
- Arduino / C++
- SPIFFS + AsyncWebServer
- ArduinoJson

---

## 🧰 Developer Setup

1. Clone the repo  
   git clone https://github.com/BitcoinManor/STACKSWORTH_Matrix.git  

2. Open `.ino` in Arduino IDE  

3. Install required libraries  

4. Upload firmware  

5. Upload `/data` folder (SPIFFS)  

6. Connect to device hotspot  

7. Configure WiFi and settings  

---

## 🔄 OTA Updates

Manual OTA is supported.

- Trigger update from web portal  
- Device downloads firmware  
- Safe reboot on success  

---

## ⚠️ Notes

- Designed for **24/7 operation**
- Handles API failures gracefully
- Uses cached data during outages
- Reset logging included for diagnostics

---

## 🙏 Inspiration

Inspired by the work of  
BenArc with his Hal Finney Bitcoin Ticker

Built on the shoulders of Bitcoin builders.

---

## 📜 License

MIT License — use freely, fork often, flash everything.

---

## ⚡ Final Word

This is not just another gadget.

This is **Bitcoin’s Pulse**, at a glance.

Built with ⚡ by Bitcoin Manor  
https://bitcoinmanor.com
