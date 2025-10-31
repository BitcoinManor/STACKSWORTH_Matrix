// 🚀 STACKSWORTH_MATRIX_MASTER: Dual_Row SCROLL_LEFT 20
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
#include <WiFi.h>
#include <esp_wifi.h>  // Needed for esp_read_mac
#include "esp_system.h"
#include <DNSServer.h>
DNSServer dnsServer;
#include <Preferences.h>
Preferences prefs;


// retrieve and store the MAC
String getShortMAC() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);  // Get STA interface MAC
  char shortID[7];
  sprintf(shortID, "%02X%02X%02X", mac[3], mac[4], mac[5]);  // last 3 bytes (6 hex chars)
  return String(shortID);
}

String macID;



bool wifiConnected = false;
bool buttonPressed = false;

String savedSSID;
String savedPassword;
String savedCity;
int savedTimezone = -99;


// Fetch and Display Cycles
uint8_t fetchCycle = 0;   // 👈 for rotating which API we fetch
uint8_t displayCycle = 0; // 👈 for rotating which screen we show

// initializes the server so we can later attach our custom HTML page routes
AsyncWebServer server(80);

// 🌍 API Endpoints
const char* BTC_API = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";
const char *BLOCK_API = "https://blockchain.info/q/getblockcount";
const char *FEES_API = "https://mempool.space/api/v1/fees/recommended";
const char *MEMPOOL_BLOCKS_API = "https://mempool.space/api/blocks";
const char *BLOCKSTREAM_TX_API_BASE = "https://blockstream.info/api/block/";

// ===== SatoNak API (authoritative) =====
#define USE_SATONAK_PRICE 1    // 1 = use SatoNak for price, 0 = keep old source

static const char* SATONAK_BASE   = "https://satonak.bitcoinmanor.com";
static const char* SATONAK_PRICE  = "/api/price";   // supports ?fiat=EUR etc.
static const char* SATONAK_HEIGHT = "/api/height";  // (for later)
static const char* SATONAK_MINER  = "/api/miner";   // (for later)

// default fiat (can be "USD", "EUR", etc.)
static const char* FIAT_CODE = "USD";

static inline String satonakUrl(const char* path, const char* fiat = nullptr) {
  String u = String(SATONAK_BASE) + String(path);
  if (fiat && fiat[0] != '\0') {
    u += "?fiat="; u += fiat;
  }
  return u;
}

// ---- Smash Buy phrases split into TOP / BOTTOM lines ----
const char* PHRASES[][2] = {
  { "SMASH",           "BUY!" },
  { "DON'T TRUST",     "VERIFY!" },
  { "STACK",           "SATS" },
  { "RUN A",           "NODE" },
  { "NOT YOUR",        "KEYS" },
  { "FIX THE",         "MONEY" },
  { "STAY",            "HUMBLE" },
  { "OPT",             "OUT" },
  { " LOW TIME",       "PREFERENCE" },  // fun variation
  { "INFINITE",        "GAME" },
  { "HARDER",          "MONEY" },
  { "BITCOIN",         "> FIAT" },
  { "LET'S",            "GO" }
};
#define NUM_PHRASES (sizeof(PHRASES) / sizeof(PHRASES[0]))

// ==== PACMAN MODE (using working original code) ====
enum UiMode { MODE_ROTATION, MODE_PACMAN };
static UiMode uiMode = MODE_ROTATION;

// Working PacMan animation variables (from original_bitcoin_pacman.ino)
const uint8_t pacman[4][18] =  // Bitcoin Symbol pursued by a pacman
{
  { 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0x7e, 0xff, 0xe7, 0xc3, 0x81, 0x00 },
  { 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xe7, 0xe7, 0x42, 0x00 },
  { 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xe7, 0x66, 0x24 },
  { 0xfe, 0x7b, 0xf3, 0x7f, 0xf3, 0x7b, 0xfe, 0x00, 0x00, 0x00, 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c },
};
const uint8_t DATA_WIDTH = (sizeof(pacman[0])/sizeof(pacman[0][0]));

uint32_t prevTimeAnim = 0;  // remember the millis() value in animations
int16_t pacIdx;             // display index (column) 
uint8_t pacFrame;           // current animation frame
uint8_t pacDeltaFrame;      // the animation frame offset for the next frame
bool pacmanInit = true;     // initialize the animation






String mapWeatherCode(int code)
{
  if (code == 0)
    return "Sunny";
  else if (code == 1)
    return "Mostly Sunny";
  else if (code == 2)
    return "Partly Cloudy";
  else if (code == 3)
    return "Cloudy";
  else if (code >= 45 && code <= 48)
    return "Foggy";
  else if (code >= 51 && code <= 57)
    return "Drizzle";
  else if (code >= 61 && code <= 67)
    return "Rain";
  else if (code >= 71 && code <= 77)
    return "Snowy";
  else if (code >= 80 && code <= 82)
    return "Showers";
  else if (code >= 85 && code <= 86)
    return "Snow Showers";
  else if (code >= 95 && code <= 99)
    return "Thunderstorm";
  else
    return "Unknown";
}

// Time Config
const char *ntpServer = "pool.ntp.org";
long gmtOffset_sec = -7 * 3600;
int daylightOffset_sec = 3600;

// Global Data Variables
int btcPrice = 0, blockHeight = 0, feeRate = 0, satsPerDollar = 0;
char btcText[16], blockText[16], feeText[16], satsText[16];
char timeText[16], dateText[16], dayText[16];
float latitude = 0.0;
float longitude = 0.0;
String weatherCondition = "Unknown";
int temperature = 0;
float btcChange24h = 0.0;
char changeText[16];
String minerName = "Unknown";

String formatWithCommas(int number)
{
  String numStr = String(number);
  String result = "";
  int len = numStr.length();
  for (int i = 0; i < len; i++)
  {
    if (i > 0 && (len - i) % 3 == 0)
      result += ",";
    result += numStr[i];
  }
  return result;
}

// LED Matrix Config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_ZONES 2
#define ZONE_SIZE 8
#define MAX_DEVICES (MAX_ZONES * ZONE_SIZE)
#define SCROLL_SPEED 20
#define FETCH_INTERVAL 120000

#define ZONE_LOWER 0
#define ZONE_UPPER 1

#define CLK_PIN 18
#define DATA_PIN 23
#define CS_PIN 5
#define BUTTON_PIN 25   //Pin for Smash Buy Button

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Brightness: 0 = dimmest, 15 = brightest
uint8_t BRIGHTNESS = 1;
unsigned long lastFetchTime = 0;
uint8_t cycle = 0;             // 🔥 Needed for animation control
unsigned long lastApiCall = 0; // 🔥 Needed for fetch timing
unsigned long lastMemoryCheck = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNTPUpdate = 0;

const unsigned long WEATHER_UPDATE_INTERVAL = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long NTP_UPDATE_INTERVAL = 10UL * 60UL * 1000UL;     // 10 minutes
const unsigned long MEMORY_CHECK_INTERVAL = 5UL * 60UL * 1000UL;    // 5 minutes

// Pre Connection Message for home users
void showPreConnectionMessage()
{
  static uint8_t step = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate < 2500)
    return; // Wait for 2.5 seconds between steps
  lastUpdate = millis();

  switch (step)
  {
  case 0:
    P.displayZoneText(ZONE_UPPER, "ENTER THE", PA_CENTER, 0, 2500, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "MATRIX", PA_CENTER, 0, 2500, PA_FADE, PA_FADE);
    break;
  case 1:
    P.displayZoneText(ZONE_UPPER, "Connect Your", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "Device Inside", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    break;
  case 2:
    P.displayZoneText(ZONE_UPPER, "WiFi Settings", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "Labelled", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    break;
  case 3:
    P.displayZoneText(ZONE_UPPER, "Stacksworth", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "MATRIX", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    break;
  case 4:
    P.displayZoneText(ZONE_UPPER, "OR TYPE", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "192.168.4.1", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    break;
  case 5:
    P.displayZoneText(ZONE_UPPER, "SETUP WiFi", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "and hit SAVE", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    break;
  default:
    step = 0; // Reset the sequence
    return;
  }

  step++;
}

 //Load Saved WiFi + City + Timezone on Boot
void loadSavedSettingsAndConnect() {
  prefs.begin("stacksworth", true);  

  savedSSID = prefs.getString("ssid", "");
  savedPassword = prefs.getString("password", "");
  savedCity = prefs.getString("city", "");
  savedTimezone = prefs.getInt("timezone", -99);

  prefs.end();

  if (savedSSID != "" && savedPassword != "") {
    Serial.println("✅ Found Saved WiFi Credentials:");
    Serial.println("SSID: " + savedSSID);
    Serial.println("Password: " + savedPassword);
    Serial.println("City: " + savedCity);
    Serial.print("Timezone offset (hours): ");
    Serial.println(savedTimezone);

    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    Serial.print("🔌 Connecting to WiFi...");
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Connected to WiFi successfully!");
      Serial.print("🌍 IP Address: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true; // 👉 set this!!
      
      if (savedTimezone != -99) {
        gmtOffset_sec = savedTimezone * 3600;
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Serial.println("🕒 Timezone configured");
      }
    } else {
      Serial.println("\n❌ Failed to connect to WiFi, falling back to Access Point...");
      startAccessPoint();
    }
  } else {
    Serial.println("⚠️ No saved WiFi credentials found, starting Access Point...");
    startAccessPoint();
  }
}

  
    // Access Point Code
    void startAccessPoint()
    {
      Serial.println("🚀 Starting Access Point...");
      WiFi.mode(WIFI_AP);
      macID = getShortMAC();  // Store globally
      String ssid = "SW-MATRIX-" + getShortMAC();
      WiFi.softAP(ssid.c_str());


      IPAddress myIP = WiFi.softAPIP();
      Serial.print("🌍 AP IP address: ");
      Serial.println(myIP);
      Serial.print("📶 AP SSID: ");
      Serial.println(ssid); // Helpful for debug

      // DNS Captive portal
      dnsServer.start(53, "*", myIP);
      Serial.println("🚀 DNS Server started for captive portal.");
    }

    // FETCH FUNCTIONS
    void fetchBitcoinData() {
  // Try SatoNak first, then fallback to CoinGecko
  if (fetchPriceFromSatoNak()) {
    Serial.println("✅ Bitcoin price fetched from SatoNak");
    return;
  }
  
  Serial.println("⚠️ SatoNak failed, trying CoinGecko fallback");
  
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Not enough heap to safely fetch. Skipping BTC fetch.");
    return;
  }
  Serial.println("🔄 Fetching BTC Price from CoinGecko...");
  HTTPClient http;
  http.begin(BTC_API);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());
    btcPrice = doc["bitcoin"]["usd"];
    btcChange24h = doc["bitcoin"]["usd_24h_change"];
    satsPerDollar = 100000000 / btcPrice;

    sprintf(btcText, "$%s", formatWithCommas(btcPrice).c_str());
    sprintf(satsText, "%d sats", satsPerDollar);
    snprintf(changeText, sizeof(changeText), "%+.2f%%", btcChange24h);

    Serial.printf("✅ Updated BTC Price: $%d | Sats per $: %d\n", btcPrice, satsPerDollar);
    Serial.printf("✅ BTC Price: %s (%s)\n", btcText, satsText);
  } else {
    Serial.println("❌ Failed to fetch BTC Price");
  }
  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}

// Returns true on success, false on any failure (so callers can fallback)
bool fetchPriceFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak price fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak price fetch");
    return false;
  }

  String full = satonakUrl(SATONAK_PRICE, FIAT_CODE); // e.g. /api/price?fiat=USD
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(4000);
  http.setConnectTimeout(2500);
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak)");
    return false;
  }

  int rc = http.GET();
  if (rc != 200) {
    Serial.printf("❌ SatoNak price GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(1536);
  DeserializationError e = deserializeJson(doc, payload);
  if (e) {
    Serial.printf("❌ SatoNak JSON parse error: %s\n", e.c_str());
    Serial.println("↪︎ Payload (trim): " + payload.substring(0, 220));
    return false;
  }

  // Respect your FIAT_CODE (e.g., "USD"/"EUR")
  String key = String(FIAT_CODE); key.toLowerCase();

  double px = 0.0;
  if (doc.containsKey("price") && doc["price"].is<JsonObject>()) {
    if (doc["price"][key].is<double>()) px = (double)doc["price"][key];
    else if (doc["price"][key].is<long>()) px = (double)((long)doc["price"][key]);
  }
  if (px <= 0.0) {
    if (doc[key].is<double>()) px = (double)doc[key];
    else if (doc[key].is<long>()) px = (double)((long)doc[key]);
  }
  if (px <= 0.0) {
    Serial.println("❌ SatoNak: no valid price in payload");
    Serial.println("↪︎ Payload (trim): " + payload.substring(0, 220));
    return false;
  }

  double change = 0.0;
  if (doc["change_24h"].is<double>()) change = (double)doc["change_24h"];

  long sps = 0;
  if (doc["sats_per_usd"].is<long>()) sps = (long)doc["sats_per_usd"];
  if (sps == 0 && key == "usd") sps = (long)(100000000.0 / px);

  // Update your existing globals/buffers (exact names as in your sketch)
  btcPrice      = (int)round(px);
  btcChange24h  = (float)change;
  satsPerDollar = (int)sps;

  if (key == "usd") {
    snprintf(btcText, sizeof(btcText), "$%s", formatWithCommas(btcPrice).c_str());
  } else {
    snprintf(btcText, sizeof(btcText), "%s", formatWithCommas(btcPrice).c_str());
  }
  snprintf(satsText,   sizeof(satsText),  "%d sats", satsPerDollar);
  snprintf(changeText, sizeof(changeText), "%+.2f%%", btcChange24h);

  Serial.printf("✅ SatoNak Price: %s | 24h: %+.2f%% | Sats/$: %d | Free heap: %d\n",
                btcText, btcChange24h, satsPerDollar, ESP.getFreeHeap());
  return true;
}

// Fetch miner info from SatoNak API
bool fetchMinerFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak miner fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak miner fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + String(SATONAK_MINER);
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(4000);
  http.setConnectTimeout(2500);
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak miner)");
    return false;
  }

  int rc = http.GET();
  if (rc != 200) {
    Serial.printf("❌ SatoNak miner GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // For simple text response, just use the payload directly
  payload.trim();
  if (payload.length() > 0 && payload.length() < 32) {
    minerName = payload;
    Serial.printf("✅ SatoNak Miner: %s | Free heap: %d\n", minerName.c_str(), ESP.getFreeHeap());
    return true;
  } else {
    Serial.println("❌ SatoNak miner: invalid response");
    Serial.println("↪︎ Payload: " + payload.substring(0, 100));
    return false;
  }
}

// Fetch block height from SatoNak API
bool fetchHeightFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak height fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak height fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + String(SATONAK_HEIGHT);
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(4000);
  http.setConnectTimeout(2500);
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak height)");
    return false;
  }

  int rc = http.GET();
  if (rc != 200) {
    Serial.printf("❌ SatoNak height GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // For simple text response, parse as integer
  payload.trim();
  int newHeight = payload.toInt();
  if (newHeight > 0 && newHeight > blockHeight - 100) { // sanity check
    blockHeight = newHeight;
    sprintf(blockText, "%d", blockHeight);
    Serial.printf("✅ SatoNak Height: %d | Free heap: %d\n", blockHeight, ESP.getFreeHeap());
    return true;
  } else {
    Serial.println("❌ SatoNak height: invalid response");
    Serial.println("↪︎ Payload: " + payload.substring(0, 50));
    return false;
  }
}


    void fetchBlockHeight()
    {
      // Try SatoNak first, then fallback to blockchain.info
      if (fetchHeightFromSatoNak()) {
        Serial.println("✅ Block height fetched from SatoNak");
        return;
      }
      
      Serial.println("⚠️ SatoNak failed, trying blockchain.info fallback");
      
      if (ESP.getFreeHeap() < 160000)
      {
        Serial.println("❌ Not enough heap to safely fetch. Skipping block height fetch.");
        return;
      }
      Serial.println("🔄 Fetching Block Height from blockchain.info...");
      HTTPClient http;
      http.begin(BLOCK_API);
      if (http.GET() == 200)
      {
        blockHeight = http.getString().toInt();
        sprintf(blockText, "%d", blockHeight);
        Serial.printf("✅ Updated Block Height: %d\n", blockHeight);
        Serial.printf("✅ Block Height: %s\n", blockText);
      }
      else
      {
        Serial.println("❌ Failed to fetch Block Height");
      }
      http.end();
      Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
    }

/*
     void fetchMinerName() {
    if (ESP.getFreeHeap() < 160000) {
      Serial.println("❌ Not enough heap to safely fetch. Skipping miner fetch.");
      return;
    }

    Serial.println("🔄 Fetching Miner Name...");

    static String lastValidMiner = "Unknown";  // Cache last known valid miner

    HTTPClient http;
    http.begin(MEMPOOL_BLOCKS_API);
    if (http.GET() == 200) {
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error) {
        Serial.println("❌ Failed to parse mempool blocks JSON.");
        minerName = lastValidMiner != "Unknown" ? lastValidMiner : "Checking...";
        http.end();
        return;
      }

      String blockHash = doc[0]["id"];
      Serial.printf("🧱 Latest Block Hash: %s\n", blockHash.c_str());
     http.end();

      http.begin(String(BLOCKSTREAM_TX_API_BASE) + blockHash + "/txs");
      if (http.GET() == 200) {
        String payload = http.getString();
        DynamicJsonDocument txDoc(8192);
        DeserializationError txError = deserializeJson(txDoc, payload);

        if (!txError && txDoc.size() > 0) {
          String rawScriptSig = txDoc[0]["vin"][0]["scriptsig"].as<String>();
          Serial.printf("📜 Raw ScriptSig (Hex): %s\n", rawScriptSig.c_str());

          String decoded = hexToAscii(rawScriptSig);
          Serial.printf("🔍 Decoded ScriptSig (ASCII): %s\n", decoded.c_str());

          String identified = identifyMiner(decoded);
          if (identified != "Unknown") {
            minerName = identified;
            lastValidMiner = identified;
          } else {
            Serial.println("⚠️ Miner tag not recognized, using last known valid.");
            minerName = lastValidMiner != "Unknown" ? lastValidMiner : "Checking...";
          }
        } else {
          Serial.println("❌ Failed to parse TX JSON.");
          minerName = lastValidMiner != "Unknown" ? lastValidMiner : "Checking...";
          }
        } else {
          Serial.println("❌ Failed to fetch TXs from Blockstream.");
          minerName = lastValidMiner != "Unknown" ? lastValidMiner : "Checking...";
      }
    } else {
      Serial.println("❌ Failed to fetch blocks from mempool.");
      minerName = lastValidMiner != "Unknown" ? lastValidMiner : "Checking...";
    }

    http.end();
    Serial.printf("✅ Mined By: %s\n", minerName.c_str());
    Serial.printf("📈 Free heap after miner fetch: %d bytes\n", ESP.getFreeHeap());
  }

*/

    void fetchFeeRate()
    {
      if (ESP.getFreeHeap() < 160000)
      {
        Serial.println("❌ Not enough heap to safely fetch. Skipping BTC fetch.");
        return;
      }
      Serial.println("🔄 Fetching Fee Rate...");
      HTTPClient http;
      http.begin(FEES_API);
      if (http.GET() == 200)
      {
        DynamicJsonDocument doc(512);
        deserializeJson(doc, http.getString());
        feeRate = doc["fastestFee"];
        sprintf(feeText, "%d sat/vB", feeRate);
        Serial.printf("✅ Updated Fee Rate: %d sat/vB\n", feeRate);
        Serial.printf("✅ Fee Rate: %s\n", feeText);
      }
      else
      {
        Serial.println("❌ Failed to fetch Fee Rate");
      }
      http.end();
      Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
    }

    void fetchTime()
    {
      if (ESP.getFreeHeap() < 160000)
      {
        Serial.println("❌ Not enough heap to safely fetch. Skipping BTC fetch.");
        return;
      }
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo))
      {
        Serial.println("❌ Failed to fetch local time! Keeping previous timeText...");
        return; // Don't overwrite global time values if fetch fails
      }

      Serial.println("⏰ Local time fetched successfully!");

      // Format to HH:MMam/pm, then strip leading zero
      char buf[16];
      strftime(buf, sizeof(buf), "%I:%M%p", &timeinfo);
      if (buf[0] == '0')
        memmove(buf, buf + 1, strlen(buf + 1) + 1); // Strip leading 0

      // ✅ Update globals only if time fetch succeeded
      strncpy(timeText, buf, sizeof(timeText));
      timeText[sizeof(timeText) - 1] = '\0';

      strftime(dateText, sizeof(dateText), "%b %d", &timeinfo);
      strftime(dayText, sizeof(dayText), "%A", &timeinfo);

      Serial.printf("✅ Updated Time: %s | Date: %s | Day: %s\n", timeText, dateText, dayText);
      Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
    }

    void fetchLatLonFromCity()
    {
      if (ESP.getFreeHeap() < 160000)
      {
        Serial.println("❌ Not enough heap to safely fetch. Skipping BTC fetch.");
        return;
      }
      if (savedCity == "")
      {
        Serial.println("⚠️ No saved city found, skipping geolocation fetch.");
        return;
      }

      HTTPClient http;
      String url = "https://nominatim.openstreetmap.org/search?city=" + savedCity + "&format=json";
      http.begin(url);
      int httpResponseCode = http.GET();

      if (httpResponseCode == 200)
      {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getString());

        if (!doc.isNull() && doc.size() > 0)
        {
          String latStr = doc[0]["lat"];
          String lonStr = doc[0]["lon"];

          Serial.println("🌎 Found City Location:");
          Serial.println("Latitude: " + latStr);
          Serial.println("Longitude: " + lonStr);

          latitude = latStr.toFloat();
          longitude = lonStr.toFloat();
        }
        else
        {
          Serial.println("❌ No matching city found!");
        }
      }
      else
      {
        Serial.print("❌ HTTP Request failed, code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
      Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
    }

    void fetchWeather()
    {
      if (ESP.getFreeHeap() < 160000)
      {
        Serial.println("❌ Not enough heap to safely fetch. Skipping BTC fetch.");
        return;
      }
      if (savedCity == "")
      {
        Serial.println("❌ City not set, skipping weather fetch.");
        return;
      }

      String weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 6) +
                          "&longitude=" + String(longitude, 6) +
                          "&current=temperature_2m,weather_code&timezone=auto";

      HTTPClient http;
      http.begin(weatherURL);
      int httpCode = http.GET();

      if (httpCode == 200)
      {
        String payload = http.getString();
        if (payload.length() == 0)
        {
          Serial.println("❌ Empty weather payload received!");
          http.end();
          return;
        }

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error)
        {
          float temp = doc["current"]["temperature_2m"];
          int weatherCode = doc["current"]["weather_code"];
          String condition = mapWeatherCode(weatherCode);

          temperature = (int)temp;
          weatherCondition = condition;
          Serial.printf("✅ Updated Weather: %d°C | Condition: %s\n", temperature, weatherCondition.c_str());
          Serial.print("🌡️ Temperature: ");
          Serial.println(temperature);
          Serial.println("🌦️ Condition: " + weatherCondition);
        }
        else
        {
          Serial.println("❌ Failed to parse weather JSON");
        }
      }
      else
      {
        Serial.println("❌ Weather fetch failed, HTTP code: " + String(httpCode));
      }

      http.end(); // ✅ Always clean up!
      Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
    }

    // Setup of device

    void setup()
    {
      Serial.begin(115200);
      Serial.println("🚀 Starting STACKSWORTH Matrix Setup...");


      //Adding MAC Address to ID
      macID = getShortMAC();
      Serial.println("🆔 MAC Fragment: " + macID);

      prefs.begin("device", false);
      prefs.putString("shortMAC", macID);
      prefs.end();

    

      // Monitor available heap memory
      Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
      Serial.printf("Minimum free heap: %d bytes\n", ESP.getMinFreeHeap());
      Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

      // 🗂️ Mount SPIFFS
      Serial.println("🗂️ Mounting SPIFFS...");
      if (!SPIFFS.begin(true))
      {
        Serial.println("❌ Failed to mount SPIFFS");
        return;
      }
      Serial.println("✅ SPIFFS mounted successfully!");

      if (!SPIFFS.exists("/STACKS_Wifi_Portal.html.gz"))
      {
        Serial.println("❌ HTML file NOT found");
      }
      else
      {
        Serial.println("✅ Custom HTML file found");
      }

      // Try WiFi first, fallback if needed
      Serial.println("📡 Loading saved WiFi and settings...");
      loadSavedSettingsAndConnect();

      // LED Matrix Startup
      Serial.println("💡 Initializing LED Matrix...");
      P.begin(MAX_ZONES);
      P.setIntensity(BRIGHTNESS);
      P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
      P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
      P.setFont(nullptr);


      randomSeed(esp_random());

      // Show Welcome Loop
      if (!wifiConnected)
      {
        // Show Welcome Loop only if WiFi NOT connected
        unsigned long startTime = millis();
        while (millis() - startTime < 21000)
        {
          showPreConnectionMessage();
          P.displayAnimate();
        }
      }

      // 🕒 Time Config
      Serial.println("🕒 Configuring time...");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

      // Serve Custom HTML File
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                {
  if (SPIFFS.exists("/STACKS_Wifi_Portal.html.gz")) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/STACKS_Wifi_Portal.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip"); // Inform the browser that the file is GZIP-compressed
    request->send(response);
  } else {
    request->send(404, "text/plain", "Custom HTML file not found");
  } });

      // 📝 Handle Save Form Submission
      server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
                {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    String city = request->getParam("city", true)->value();
    String timezone = request->getParam("timezone", true)->value();

    Serial.println("✅ Saving WiFi Settings:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("City: " + city);
    Serial.println("Timezone: " + timezone);

    prefs.begin("stacksworth", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("city", city);
    prefs.putInt("timezone", timezone.toInt());
    prefs.end();
    Serial.println("✅ Settings saved to NVS!");


    // ✅ SEND HTTP 200 RESPONSE FIRST
    request->send(200, "text/plain", "Settings saved! Rebooting...");

    delay(2000); // small delay to let browser receive the message
        // Matrix Feedback
    P.displayZoneText(ZONE_UPPER, "SETTINGS", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "SAVED", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    delay(2500);

    P.displayZoneText(ZONE_UPPER, "REBOOTING", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "...", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    delay(2000);

    ESP.restart();
  } else {
    Serial.println("❌ Missing parameters in form submission!");
    request->send(400, "text/plain", "Missing parameters");
  } });


      // Serve MAC fragment to the portal
      server.on("/macid", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", getShortMAC());
    });


      // Captive Portal Redirect
      server.onNotFound([](AsyncWebServerRequest *request)
                        { request->redirect("/");
                        });

      // Start Web Server
      Serial.println("🌐 Starting Async Web Server...");
      delay(2000); // 🕒 Let WiFi fully stabilize first
      server.begin();
      Serial.println("🌍 Async Web server started");
      delay(2000); // 🕒 Let server stabilize after starting

      // Initial API Fetch
      Serial.println("🌍 Fetching initial data...");
      fetchBitcoinData();
      fetchBlockHeight();
      fetchMinerFromSatoNak();
      fetchFeeRate();
      fetchTime();
      fetchLatLonFromCity();
      fetchWeather();
      lastFetchTime = millis();
      Serial.println("✅ Initial data fetch complete!");

      lastWeatherUpdate = millis() - WEATHER_UPDATE_INTERVAL; // ⬅️ force weather update ready immediately

      // Show Connection Success Message
      Serial.println("📢 Displaying WiFi connected message on Matrix...");
      P.displayZoneText(ZONE_UPPER, "WIFI", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "CONNECTED", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      delay(2000);
      

      // 👇  Manually trigger first animation cycle!
      cycle = 0;                                              // Start at first data set
      lastApiCall = millis() - FETCH_INTERVAL;                // Force immediate fetch
      lastWeatherUpdate = millis() - WEATHER_UPDATE_INTERVAL; // Force weather update soon
      lastNTPUpdate = millis() - NTP_UPDATE_INTERVAL;         // Force NTP update soon

     pinMode(BUTTON_PIN, INPUT_PULLUP);  //added this for the Smash Buy Button!!!

      esp_task_wdt_config_t wdt_config = {
          .timeout_ms = 12000,                             // 12 seconds
          .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // All cores
          .trigger_panic = true                            // Reset if not fed in time
      };
      esp_task_wdt_init(&wdt_config);

      esp_task_wdt_add(NULL); // Add current task to WDT
    }

// Working PacMan animation functions (from original_bitcoin_pacman.ino)
static bool runPacmanAnimation() {
  // Is it time to animate?
  if (millis() - prevTimeAnim < 75)  // 75ms delay like original
    return true;  // Still animating
  prevTimeAnim = millis();

  MD_MAX72XX* mx = P.getGraphicObject();
  if (!mx) return false;

  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  // Initialize
  if (pacmanInit) {
    mx->clear();
    pacIdx = -DATA_WIDTH;
    pacFrame = 0;
    pacDeltaFrame = 1;
    pacmanInit = false;

    // Show "SMASH BUY!" on top zone
    P.displayZoneText(ZONE_UPPER, "SMASH BUY!", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    
    // Lay out the Bitcoin symbols (just like original)
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
      mx->setPoint(0, (i * 8) + 4, true);
      mx->setPoint(0, (i * 8) + 2, true);
      mx->setPoint(1, (i * 8) + 5, true);
      mx->setPoint(1, (i * 8) + 4, true);
      mx->setPoint(1, (i * 8) + 3, true);
      mx->setPoint(1, (i * 8) + 2, true);
      mx->setPoint(2, (i * 8) + 5, true);
      mx->setPoint(2, (i * 8) + 1, true);
      mx->setPoint(3, (i * 8) + 5, true);
      mx->setPoint(3, (i * 8) + 4, true);
      mx->setPoint(3, (i * 8) + 3, true);
      mx->setPoint(3, (i * 8) + 2, true);
      mx->setPoint(4, (i * 8) + 5, true);
      mx->setPoint(4, (i * 8) + 1, true);
      mx->setPoint(5, (i * 8) + 5, true);
      mx->setPoint(5, (i * 8) + 4, true);
      mx->setPoint(5, (i * 8) + 3, true);
      mx->setPoint(5, (i * 8) + 2, true);
      mx->setPoint(6, (i * 8) + 4, true);
      mx->setPoint(6, (i * 8) + 2, true);
    }
  }

  // Clear old graphic
  for (uint8_t i = 0; i < DATA_WIDTH; i++) {
    int16_t col = pacIdx - DATA_WIDTH + i;
    if (col >= 0 && col < mx->getColumnCount())
      mx->setColumn(col, 0);
  }

  // Move reference column and draw new graphic
  pacIdx++;
  for (uint8_t i = 0; i < DATA_WIDTH; i++) {
    int16_t col = pacIdx - DATA_WIDTH + i;
    if (col >= 0 && col < mx->getColumnCount())
      mx->setColumn(col, pacman[pacFrame][i]);
  }

  // Advance the animation frame
  pacFrame += pacDeltaFrame;
  if (pacFrame == 0 || pacFrame == 3)
    pacDeltaFrame = -pacDeltaFrame;

  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  // Check if animation is complete
  if (pacIdx >= mx->getColumnCount() + DATA_WIDTH) {
    pacmanInit = true;  // Reset for next time
    return false;       // Animation finished
  }

  return true;  // Still animating
}





/*
  // ----- Phase 1: top band (rowBase=8), LEFT -> RIGHT -----
  {
    int rowBase   = 8;                 // top band
    int stepInRow = pacmanStep;        // 0..ROW_COLS-1
    int col0      = stepInRow;         // L->R

    pacmanDrawFrame(rowBase, col0);

    pacFrame += pacDir;
    if (pacFrame == 0 || pacFrame == 3) pacDir = -pacDir;

    pacmanStep++;
    if (pacmanStep >= ROW_COLS) {
      // all done — clean down and hand back to rotation
      if (prevCol0 > -1000) {
        drawSpriteBits(rowBase, prevCol0, PACMAN_FRAMES[pacFrame], PAC_W, false);
      }
      prevCol0 = -1000;
      P.displayClear();
      P.displaySuspend(false);
      P.synchZoneStart();
      return false;         // finished animation
    }
    return true;            // still animating
  }
}


*/


//LOOP
    void loop()
    {
      esp_task_wdt_reset();           // Reset watchdog
      dnsServer.processNextRequest(); // Handle captive portal DNS magic

      // If Pac-Man mode is active, run the working animation
if (uiMode == MODE_PACMAN) {
  // Keep text animations running (for "SMASH BUY!" text)
  P.displayAnimate();
  
  // Run the working PacMan animation
  if (!runPacmanAnimation()) {
    uiMode = MODE_ROTATION;     // Finished: return to normal rotation
    P.displayClear();           // Clean up
    P.synchZoneStart();         // Reset zones
  }
  return;                       // Skip the rest of loop this tick
}




// 🛠️ Smash Buy Button Polling (Debounced)
static bool lastButtonState = HIGH;
bool currentButtonState = digitalRead(BUTTON_PIN);

if (lastButtonState == HIGH && currentButtonState == LOW) {
  // Falling edge: button was released, now pressed
  Serial.println("🚨 SMASH BUY Button Pressed!");
  buttonPressed = true;
}

lastButtonState = currentButtonState;


// ── Smash-Buy: trigger working Pac-Man animation
if (buttonPressed) {
  buttonPressed = false;                 // consume the event

  // (optional) cooldown you already use elsewhere:
  static unsigned long pressLockUntil = 0;
  if (millis() < pressLockUntil) {
    // ignore long-press repeats but keep loop running
  } else {
    pressLockUntil = millis() + 600;     // 0.6s lockout

    // Start working PacMan animation
    uiMode = MODE_PACMAN;
    pacmanInit = true;  // Reset animation
    prevTimeAnim = 0;   // Reset timer
    Serial.println("🎮 Working Pac-Man animation starting…");
  }
}





  unsigned long currentMillis = millis();
      

      // ✅ Monitor heap health every 60 seconds
      static unsigned long lastMemoryCheck = 0;
      static unsigned long lastHeapLog = 0;
      if (currentMillis - lastHeapLog >= 60000) {
        Serial.printf("🧠 Free heap: %d | Min ever: %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
        lastHeapLog = currentMillis;
      }

      // 🚨 Auto-reboot if heap drops too low
      if (ESP.getFreeHeap() < 140000)
      {
        Serial.println("🚨 CRITICAL: Free heap dangerously low. Rebooting to recover...");
        delay(1000); // Give time for message to print
        ESP.restart();
      }

      // ⏰ Fetch Time every 1 minute
      static unsigned long lastTimeFetch = 0;
      if (currentMillis - lastTimeFetch >= 60000)
      {
        fetchTime();
        lastTimeFetch = currentMillis;
      }

      // 🌦️ Fetch Weather every 30 minutes
      static unsigned long lastWeatherFetch = 0;
      if (currentMillis - lastWeatherFetch >= 1800000)
      {
        fetchWeather();
        lastWeatherFetch = currentMillis;
      }

      // 🔄 Fetch BTC Price and Fee Rate every 5 minutes
      static unsigned long lastBTCFeeFetch = 0;
      if (currentMillis - lastBTCFeeFetch >= 300000)
      {
        fetchBitcoinData();
        fetchFeeRate();
        lastBTCFeeFetch = currentMillis;
      }

      // 🔄 Fetch Block Height every 5 minutes (offset by 2.5 minutes)
      static unsigned long lastBlockHeightFetch = 0;
      if (currentMillis - lastBlockHeightFetch >= 300000)
      {
        fetchBlockHeight();
        fetchMinerFromSatoNak();
        lastBlockHeightFetch = currentMillis;
      }


      // 🖥️ Rotate screens
  if (P.displayAnimate()) {
    Serial.print("🖥️ Displaying screen: ");
    Serial.println(displayCycle);

    switch (displayCycle) {
      case 0:
        Serial.println("🖥️ Displaying BLOCK screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "BLOCK", blockText); 
        P.displayZoneText(ZONE_UPPER, "BLOCK",   PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, blockText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;


      case 1:
        Serial.println("🖥️ Displaying MINER screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "MINER", minerName.c_str());
        P.displayZoneText(ZONE_UPPER, "MINED BY", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, minerName.c_str(), PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;

      case 2:
        Serial.println("🖥️ Displaying USD PRICE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "USD PRICE", btcText);
        P.displayZoneText(ZONE_UPPER, "USD PRICE", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, btcText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break; 

          
      case 3:
        Serial.println("🖥️ Displaying 24H CHANGE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "24H CHANGE", changeText);
        P.displayZoneText(ZONE_UPPER, "24H CHANGE", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, changeText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear();
        P.synchZoneStart();
        break;

      case 4:
        Serial.println("🖥️ Displaying SATS/$ screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "MOSCOW TIME", satsText);
        P.displayZoneText(ZONE_UPPER, "SATS/USD", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, satsText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization  
        break;
        
      case 5:
        Serial.println("🖥️ Displaying FEE RATE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "FEE RATE", feeText);
        P.displayZoneText(ZONE_UPPER, "FEE RATE", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, feeText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;

        
      case 6:
        Serial.println("🖥️ Displaying TIME and City screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "TIME", timeText);
        P.displayZoneText(ZONE_UPPER, savedCity.c_str(), PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, timeText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;

        
      case 7:
        Serial.println("🖥️ Displaying DAY/DATE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", dayText, dateText);
        P.displayZoneText(ZONE_UPPER, dayText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, dateText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;

        
      case 8: {
        Serial.println("🖥️ Displaying WEATHER screen...");
        static char tempDisplay[16];
        snprintf(tempDisplay, sizeof(tempDisplay), (temperature >= 0) ? "+%dC" : "%dC", temperature);
        String cond = weatherCondition;
        cond.replace("_", " ");
        cond.toLowerCase();
        cond[0] = toupper(cond[0]);
        static char condDisplay[32];
        strncpy(condDisplay, cond.c_str(), sizeof(condDisplay));
        condDisplay[sizeof(condDisplay) - 1] = '\0';

        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", tempDisplay, condDisplay);
        P.displayZoneText(ZONE_UPPER, tempDisplay, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, condDisplay, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      

      case 9:
        Serial.println("🖥️ Displaying MOSCOW TIME screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "MOSCOW TIME", satsText);
        P.displayZoneText(ZONE_UPPER, "MOSCOW TIME", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(ZONE_LOWER, satsText, PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization  
        break;


      case 10:// This is for the models we ship but can be changed for custom units
        P.displayZoneText(ZONE_UPPER, "Stacksworth", PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT); 
        P.displayZoneText(ZONE_LOWER, "MATRIX",      PA_CENTER, SCROLL_SPEED, 10000, PA_SCROLL_LEFT, PA_SCROLL_LEFT); 
        P.displayClear();
        P.synchZoneStart();
        break;
    }

      Serial.println("✅ Screen update complete.");
      displayCycle = (displayCycle + 1) % 11;
      
    }
  }
