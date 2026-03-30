# ⚡ STACKSWORTH Matrix  
**Open Source • Web Flashable • Bitcoin Metrics Display • Bitcoin at a Glance**

![STACKSWORTH Banner](https://github.com/BitcoinManor/STACKSWORTH_Matrix/raw/main/assets/stacksworth_banner.png)

Welcome to **STACKSWORTH**, the future of open-source Bitcoin displays.  
Watch Bitcoin’s pulse at a glance — live price, block height, fees, sats per dollar, time, and more — all from a sleek plug-and-play LED matrix built for Bitcoiners.

---

## 🚀 Latest Firmware: v2.0.69

### 🔥 Major Improvements
- ✅ **Manual OTA Updates (Stable)**  
  Trigger updates directly from the web portal  
- 🔄 **Improved Stability & Error Handling**  
  Graceful fallback during API failures (no crashes, no freezing)  
- 🧠 **Smart Cached Display System**  
  Continues running even during network outages  
- 🔌 **Safer Boot Sequence**  
  Reduced LED power draw during startup  
- 🌐 **Improved WiFi Reconnect Logic**  
  Background reconnect without interrupting display  
- 🆔 **MAC-Based Device ID**  
  Unique ID across hotspot, portal, and UI  

📦 [Download Firmware](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/releases)  
📓 [View Changelog](https://github.com/BitcoinManor/STACKSWORTH_MATRIX/blob/main/CHANGELOG.md)

---

## ⚡ Quick Start

- 🔌 **Flash in Browser:**  
  https://bitcoinmanor.github.io/STACKSWORTH_WebFlasher  

- 🌐 **Connect to Device:**  
  Join `SW-MATRIX-XXXXXX`  

- ⚙️ **Configure:**  
  Enter WiFi, City, Timezone via portal  

- 🚀 **Done:**  
  Device reboots and begins displaying Bitcoin data  

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

Powered primarily by the **SatoNak self-sovereign API**, with fallback support:

- 💰 BTC Price (multi-currency)
- 📦 Block Height  
- 🚦 Fee Rate (sat/vB)  
- 🏭 Mining Pool  
- 📈 24H Change  
- 🧮 Circulating Supply  
- ⚡ Hashrate  
- 🌤 Weather + Temperature  
- ⏰ Time / Date  

---

## 🌐 Data Infrastructure

### Primary (Self-Sovereign)
- SatoNak API  
  https://satonak.bitcoinmanor.com

### Fallback Sources
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

```bash
git clone https://github.com/BitcoinManor/STACKSWORTH_Matrix.git
