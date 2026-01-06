
// 🚀 STACKSWORTH_MATRIX_MASTER USING OUR SATONAK API
// Built By BitcoinManor.com Stacksworth.com
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiManager.h> 
#include <HTTPClient.h>
#include <WiFiClient.h>
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
String savedCurrency;  // 🌍 New: User's preferred currency (USD, EUR, etc.)
String savedTheme;     // 🎨 New: User's preferred theme (scroll, fade)
int savedTimezone = -99;


// Fetch and Display Cycles
uint8_t fetchCycle = 0;   // 👈 for rotating which API we fetch
uint8_t displayCycle = 0; // 👈 for rotating which screen we show

// initializes the server so we can later attach our custom HTML page routes
AsyncWebServer server(80);

static WiFiClient httpClient;

// 🌍 API Endpoints
const char* BTC_API = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";;
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

// default fiat (can be "USD", "EUR", etc.) - now loaded from preferences
static String getCurrentFiatCode() {
  return savedCurrency.length() > 0 ? savedCurrency : "USD";
}

// Get currency symbol for display
static String getCurrencySymbol() {
  String fiat = getCurrentFiatCode();
  if (fiat == "USD") return "$";
  if (fiat == "CAD") return "$";       // Clean $ since top row shows "CAD PRICE"
  if (fiat == "EUR") return "";        // Clean number since top row shows "EUR PRICE"  
  if (fiat == "GBP") return "";        // Clean number since top row shows "GBP PRICE"
  if (fiat == "JPY") return "";        // Clean number since top row shows "JPY PRICE" (avoid encoding issues)
  if (fiat == "AUD") return "$";       // Clean $ since top row shows "AUD PRICE"
  if (fiat == "CHF") return "";        // Clean number since top row shows "CHF PRICE"
  if (fiat == "CNY") return "";        // Clean number since top row shows "CNY PRICE"
  if (fiat == "SEK") return "";        // Clean number since top row shows "SEK PRICE"
  if (fiat == "NOK") return "";        // Clean number since top row shows "NOK PRICE"
  return ""; // fallback to no symbol
}

static inline String satonakUrl(const char* path, const char* fiat = nullptr) {
  String u = String(SATONAK_BASE) + String(path);
  if (fiat && fiat[0] != '\0') {
    u += "?fiat="; u += fiat;
  }
  return u;
}

// 🎨 Get animation effects based on user's theme preference
static void getThemeEffects(textEffect_t &effectIn, textEffect_t &effectOut) {
  if (savedTheme == "fade") {
    Serial.println("🎨 Using WIPE theme (power-efficient alternative to fade)");
    effectIn = PA_WIPE_CURSOR;
    effectOut = PA_WIPE_CURSOR;
  } else {
    // Default to scroll theme (safe fallback)
    Serial.println("🎨 Using SCROLL theme");
    effectIn = PA_SCROLL_LEFT;
    effectOut = PA_SCROLL_LEFT;
  }
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
  { "LOW TIME",       "PREFERENCE" },  // fun variation
  { "INFINITE",        "GAME" },
  { "HARDER",          "MONEY" },
  { "BITCOIN",         "> FIAT" },
  { "LET'S",            "GO" }
};
#define NUM_PHRASES (sizeof(PHRASES) / sizeof(PHRASES[0]))

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
// 🌍 Timezone strings that auto-handle DST (no more manual updates needed!)
const char* TIMEZONE_STRINGS[] = {
  "UTC0",                                    // UTC +0
  "GMT0BST,M3.5.0/1,M10.5.0",              // London +0/+1
  "CET-1CEST,M3.5.0,M10.5.0/3",            // Paris/Berlin +1/+2
  "EET-2EEST,M3.5.0/3,M10.5.0/4",          // Helsinki +2/+3
  "MSK-3",                                  // Moscow +3 (no DST)
  "JST-9",                                  // Tokyo +9 (no DST)
  "AEST-10AEDT,M10.1.0,M4.1.0/3",          // Sydney +10/+11
  "NZST-12NZDT,M9.5.0,M4.1.0/3",           // Auckland +12/+13
  "HST10",                                  // Hawaii -10 (no DST)
  "AKST9AKDT,M3.2.0,M11.1.0",              // Alaska -9/-8
  "PST8PDT,M3.2.0,M11.1.0",                // Pacific -8/-7
  "MST7MDT,M3.2.0,M11.1.0",                // Mountain -7/-6
  "CST6CDT,M3.2.0,M11.1.0",                // Central -6/-5
  "EST5EDT,M3.2.0,M11.1.0"                 // Eastern -5/-4
};

const char* TIMEZONE_NAMES[] = {
  "UTC (+0)", "London (+0/+1)", "Paris (+1/+2)", "Helsinki (+2/+3)",
  "Moscow (+3)", "Tokyo (+9)", "Sydney (+10/+11)", "Auckland (+12/+13)",
  "Hawaii (-10)", "Alaska (-9/-8)", "Pacific (-8/-7)", "Mountain (-7/-6)",
  "Central (-6/-5)", "Eastern (-5/-4)"
};

#define NUM_TIMEZONES (sizeof(TIMEZONE_STRINGS) / sizeof(TIMEZONE_STRINGS[0]))

// Global Data Variables
int btcPrice = 0, blockHeight = 0, feeRate = 0, satsPerDollar = 0;
char btcText[16], blockText[16], feeText[16], satsText[16], satsText2[16];
char timeText[16], dateText[16], dayText[16];
char hashrateText[16];  // New global for hashrate display
char circSupplyText[16];  // New global for circulating supply top line  
char circPercentText[16]; // New global for circulating supply bottom line
float latitude = 0.0;
float longitude = 0.0;
String weatherCondition = "Unknown";
int temperature = 0;
float btcChange24h = 0.0;
char changeText[16];
String minerName = "Unknown";
String hashrate = "Unknown";  // New global for hashrate data
String circSupply = "Unknown"; // New global for circulating supply data
String athPrice = "Unknown";   // New global for ATH price data
String daysSinceAth = "Unknown"; // New global for days since ATH data
char athText[16];              // New global for ATH price display
char daysAthText[16];          // New global for days since ATH display

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
uint8_t BRIGHTNESS = 12;

// Function to adjust brightness
void setBrightness(uint8_t level) {
  if (level > 15) level = 15;  // Clamp to max
  BRIGHTNESS = level;
  P.setIntensity(BRIGHTNESS);
  Serial.printf("💡 Brightness set to: %d/15\n", BRIGHTNESS);
}

// Function to cycle brightness (for potential button control)
void cycleBrightness() {
  BRIGHTNESS = (BRIGHTNESS + 3) % 16;  // Step by 3 for noticeable changes
  if (BRIGHTNESS == 0) BRIGHTNESS = 3; // Don't go completely dark
  setBrightness(BRIGHTNESS);
}
unsigned long lastFetchTime = 0;
uint8_t cycle = 0;             // 🔥 Needed for animation control
unsigned long lastApiCall = 0; // 🔥 Needed for fetch timing
unsigned long lastMemoryCheck = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNTPUpdate = 0;

const unsigned long WEATHER_UPDATE_INTERVAL = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long NTP_UPDATE_INTERVAL = 10UL * 60UL * 1000UL;     // 10 minutes
const unsigned long MEMORY_CHECK_INTERVAL = 5UL * 60UL * 1000UL;    // 5 minutes

const uint32_t BTC_INTERVAL     = 300000;   // 5 min
const uint32_t FEE_INTERVAL     = 300000;   // 5 min
const uint32_t BLOCK_INTERVAL   = 300000;   // 5 min
const uint32_t WEATHER_INTERVAL = 1800000;  // 30 min

const uint32_t FEE_OFFSET     =  90000;   // +1.5 min after BTC
const uint32_t BLOCK_OFFSET   = 180000;   // +3   min after BTC
const uint32_t WEATHER_OFFSET =  60000;   // +1   min after BTC

static uint32_t lastBTC = 0, lastFee = 0, lastBlock = 0, lastWeather = 0;
static uint32_t bootMs = 0;


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
    P.displayZoneText(ZONE_UPPER, "SW-MATRIX", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
    P.displayZoneText(ZONE_LOWER, "******", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
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
  savedCurrency = prefs.getString("currency", "USD");  // 🌍 Default to USD
  savedTheme = prefs.getString("theme", "scroll");     // 🎨 Default to scroll
  savedTimezone = prefs.getInt("timezone", -99);

  prefs.end();

  if (savedSSID != "" && savedPassword != "") {
    Serial.println("✅ Found Saved WiFi Credentials:");
    Serial.println("SSID: " + savedSSID);
    Serial.println("Password: " + savedPassword);
    Serial.println("City: " + savedCity);
    Serial.println("Currency: " + savedCurrency);        // 🌍 New
    Serial.println("Theme: " + savedTheme);              // 🎨 New
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
      
      // 🌍 Configure timezone using proper timezone strings (auto-handles DST!)
      if (savedTimezone != -99 && savedTimezone >= 0 && savedTimezone < NUM_TIMEZONES) {
        const char* tzString = TIMEZONE_STRINGS[savedTimezone];
        configTzTime(tzString, ntpServer);
        Serial.printf("🕒 Timezone configured: %s (%s)\n", TIMEZONE_NAMES[savedTimezone], tzString);
      } else {
        // Default to Mountain Time if no valid timezone saved
        configTzTime(TIMEZONE_STRINGS[11], ntpServer); // Mountain Time
        Serial.println("🕒 Using default Mountain Time timezone");
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

    String symbol = getCurrencySymbol();
    String currentFiat = getCurrentFiatCode();
    
    // Note: CoinGecko fallback only provides USD, so if user wants other currency,
    // they'll need to wait for SatoNak to come back online for FX conversion
    if (currentFiat == "USD") {
      sprintf(btcText, "$%s", formatWithCommas(btcPrice).c_str());
      sprintf(satsText, "$1=%d Sats", satsPerDollar);
    } else {
      sprintf(btcText, "$%s*", formatWithCommas(btcPrice).c_str()); // * indicates USD fallback
      sprintf(satsText, "$1=%d Sats*", satsPerDollar); // * shows it's USD fallback
    }
    sprintf(satsText2, "%d Sats", satsPerDollar);
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

  String currentFiat = getCurrentFiatCode();
  String full = satonakUrl(SATONAK_PRICE, currentFiat.c_str()); // e.g. /api/price?fiat=EUR
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak price GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Check if payload is plain text (just a number) vs JSON
  payload.trim();
  if (payload.length() > 0 && payload.length() < 16 && isdigit(payload.charAt(0))) {
    // Plain text response - just a price number like "103605.00"
    double px = payload.toFloat();
    if (px > 0) {
      btcPrice = (int)round(px);
      satsPerDollar = (int)(100000000.0 / px);
      
      String symbol = getCurrencySymbol();
      snprintf(btcText, sizeof(btcText), "%s%s", symbol.c_str(), formatWithCommas(btcPrice).c_str());
      
      // 🌍 Show sats per user's currency, not always USD!
      sprintf(satsText, "%s1=%d Sats", symbol.c_str(), satsPerDollar);
      sprintf(satsText2, "%d Sats", satsPerDollar);
      
      Serial.printf("✅ SatoNak Price (plain): %s | Sats/$: %d | Free heap: %d\n",
                    btcText, satsPerDollar, ESP.getFreeHeap());
      return true;
    }
  }

  // Try parsing as JSON
  DynamicJsonDocument doc(1536);
  DeserializationError e = deserializeJson(doc, payload);
  if (e) {
    Serial.printf("❌ SatoNak JSON parse error: %s\n", e.c_str());
    Serial.println("↪︎ Payload (trim): " + payload.substring(0, 220));
    return false;
  }

  // Respect your current fiat setting (e.g., "USD"/"EUR"/"CAD")
  String key = currentFiat; key.toLowerCase();

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

  String symbol = getCurrencySymbol();
  
  if (currentFiat == "USD") {
    snprintf(btcText, sizeof(btcText), "$%s", formatWithCommas(btcPrice).c_str());
  } else {
    snprintf(btcText, sizeof(btcText), "%s%s", symbol.c_str(), formatWithCommas(btcPrice).c_str());
  }
  
  // 🌍 Show sats per user's selected currency!
  sprintf(satsText,   "%s1=%d Sats", symbol.c_str(), satsPerDollar);
  sprintf(satsText2,  "%d Sats", satsPerDollar);
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
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak miner)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
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
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak height)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
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

// Fetch fee rate from SatoNak API
bool fetchFeeFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak fee fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak fee fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/fee";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak fee)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak fee GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Check if payload is plain text (just a number) vs JSON
  payload.trim();
  if (payload.length() > 0 && payload.length() < 8 && isdigit(payload.charAt(0))) {
    // Plain text response - just a fee number like "15"
    int newFee = payload.toInt();
    if (newFee > 0 && newFee < 1000) { // sanity check
      feeRate = newFee;
      snprintf(feeText, sizeof(feeText), "%d sat/vB", feeRate);
      Serial.printf("✅ SatoNak Fee (plain): %d sat/vB | Free heap: %d\n", feeRate, ESP.getFreeHeap());
      return true;
    }
  }

  // Try parsing as JSON
  DynamicJsonDocument doc(512);
  DeserializationError e = deserializeJson(doc, payload);
  if (e) {
    Serial.printf("❌ SatoNak fee JSON parse error: %s\n", e.c_str());
    Serial.println("↪︎ Payload: " + payload.substring(0, 100));
    return false;
  }

  // Parse JSON response
  int newFee = 0;
  if (doc.containsKey("value")) {
    newFee = doc["value"];
  }
  
  if (newFee > 0 && newFee < 1000) { // sanity check
    feeRate = newFee;
    snprintf(feeText, sizeof(feeText), "%d sat/vB", feeRate);
    Serial.printf("✅ SatoNak Fee: %d sat/vB | Free heap: %d\n", feeRate, ESP.getFreeHeap());
    return true;
  }

  Serial.println("❌ SatoNak fee: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
}

void fetchFeeRate() {
  // Try SatoNak first, then fallback to mempool.space
  if (fetchFeeFromSatoNak()) {
    Serial.println("✅ Fee rate fetched from SatoNak");
    return;
  }
  
  Serial.println("⚠️ SatoNak failed, trying mempool.space fallback");
  
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("🛑 Low heap before Fee fetch; skipping");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping Fee fetch");
    return;
  }

  Serial.println("🔄 Fetching Fee Rate from mempool.space…");
  HTTPClient http;
  // short, explicit timeouts so we never stall long enough to trip WDT
  http.setTimeout(2000);         // Reduced from 3000ms
  http.setConnectTimeout(1500);  // Reduced from 2000ms 
  http.useHTTP10(true);          // simpler, avoids chunking issues
  http.setReuse(false);          // no keep-alive reuse

  // FEES_API should be your existing endpoint string, unchanged
  if (!http.begin(httpClient, FEES_API)) {
    Serial.println("❌ http.begin failed; keeping last fee value");
    return;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    DeserializationError e = deserializeJson(doc, payload);
    if (e) {
      Serial.printf("❌ Fee JSON parse error: %s; keeping last value\n", e.c_str());
    } else {
      // keep previous value if field missing
      int newFee = doc["fastestFee"] | feeRate;
      feeRate = newFee;
      // feeText should be your existing global char buffer
      snprintf(feeText, sizeof(feeText), "%d sat/vB", feeRate);
      Serial.printf("✅ Updated Fee Rate: %d sat/vB\n", feeRate);
    }
  } else {
    Serial.printf("❌ Fee GET failed (%d); keeping last value\n", rc);
  }
  http.end();
}

// Fetch hashrate from SatoNak API
bool fetchHashrateFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak hashrate fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak hashrate fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/hashrate";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak hashrate)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak hashrate GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Parse and format the hashrate number
  payload.trim();
  if (payload.length() > 0 && payload.length() < 32 && payload != "na") {
    strncpy(hashrateText, payload.c_str(), sizeof(hashrateText));
    hashrateText[sizeof(hashrateText)-1] = '\0';
    hashrate = payload; // keep if you also want the raw string elsewhere
    Serial.printf("✅ SatoNak Hashrate -> Display: %s | Free heap: %d\n",
                  hashrateText, ESP.getFreeHeap());
    return true;
  }
  
  Serial.println("❌ SatoNak hashrate: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
}

// Fetch circulating supply from SatoNak API
bool fetchCircSupplyFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak circulating supply fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak circulating supply fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/circsupply";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak circulating supply)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak circulating supply GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // For simple text response, parse as number
  payload.trim();
  Serial.printf("🔍 Raw circulating supply payload: '%s'\n", payload.c_str());
  
  if (payload.length() > 0 && payload.length() < 16 && payload != "na") {
    // Remove commas temporarily for parsing
    String cleanPayload = payload;
    cleanPayload.replace(",", "");
    Serial.printf("🔍 After removing commas for parsing: '%s'\n", cleanPayload.c_str());
    
    long supply = cleanPayload.toInt();
    Serial.printf("🔍 Parsed supply as: %ld\n", supply);
    
    if (supply > 0 && supply <= 21000000) { // sanity check - supply should be reasonable
      circSupply = payload; // Store original with commas for display
      
      // Format for display: actual numbers with commas
      // Top: current supply with commas (e.g., "19,942,004")
      strncpy(circSupplyText, payload.c_str(), sizeof(circSupplyText));
      circSupplyText[sizeof(circSupplyText) - 1] = '\0';
      
      // Bottom: max supply (always "21,000,000")
      strncpy(circPercentText, "/21 Million", sizeof(circPercentText));
      circPercentText[sizeof(circPercentText) - 1] = '\0';
      
      Serial.printf("✅ SatoNak Circulating Supply: %s (%s, %s) | Free heap: %d\n", 
                    circSupply.c_str(), circSupplyText, circPercentText, ESP.getFreeHeap());
      return true;
    } else {
      Serial.printf("❌ SatoNak circulating supply: supply value %ld out of range (expected 0-21000000)\n", supply);
    }
  }
  
  Serial.println("❌ SatoNak circulating supply: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
}

// Fetch ATH price from SatoNak API
bool fetchAthFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak ATH fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak ATH fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/ath";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak ATH)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak ATH GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Parse ATH price - API returns plain text like "73750.07"
  payload.trim();
  if (payload.length() > 0 && payload.length() < 16 && payload != "na") {
    float athPriceNum = payload.toFloat();
    if (athPriceNum > 0) {
      athPrice = payload; // Store raw value
      
  // Format for display - add $ and format nicely with commas (e.g. $126,080)
  // Use existing helper to insert thousand separators
  snprintf(athText, sizeof(athText), "$%s", formatWithCommas((int)round(athPriceNum)).c_str());
      
      Serial.printf("✅ SatoNak ATH: %s -> Display: %s | Free heap: %d\n", 
                    athPrice.c_str(), athText, ESP.getFreeHeap());
      return true;
    }
  }
  
  Serial.println("❌ SatoNak ATH: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
}

// Fetch 24H change from SatoNak API
bool fetchChange24hFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak 24H change fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak 24H change fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/change24h";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak 24H change)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak 24H change GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Parse 24H change - API returns plain text like "+1.29%" or "-2.45%"
  payload.trim();
  if (payload.length() > 0 && payload.length() < 16 && payload != "na") {
    // Extract the numeric value for internal storage
    String numStr = payload;
    numStr.replace("+", "");
    numStr.replace("%", "");
    btcChange24h = numStr.toFloat();
    
    // Store the formatted display text
    strncpy(changeText, payload.c_str(), sizeof(changeText));
    changeText[sizeof(changeText) - 1] = '\0';
    
    Serial.printf("✅ SatoNak 24H Change: %s (%.2f%%) | Free heap: %d\n", 
                  changeText, btcChange24h, ESP.getFreeHeap());
    return true;
  }
  
  Serial.println("❌ SatoNak 24H change: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
}

// Fetch days since ATH from SatoNak API
bool fetchDaysSinceAthFromSatoNak() {
  if (ESP.getFreeHeap() < 160000) {
    Serial.println("❌ Low heap; skipping SatoNak days since ATH fetch");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 WiFi not connected; skipping SatoNak days since ATH fetch");
    return false;
  }

  String full = String(SATONAK_BASE) + "/api/days_since_ath";
  Serial.print("🌐 GET "); Serial.println(full);

  HTTPClient http;
  http.setTimeout(2000);      // Reduced from 4000ms to prevent WDT crashes
  http.setConnectTimeout(1500); // Reduced from 2500ms 
  http.useHTTP10(true);
  http.setReuse(false);

  if (!http.begin(full)) {
    Serial.println("❌ http.begin failed (SatoNak days since ATH)");
    return false;
  }

  esp_task_wdt_reset(); // Feed watchdog before long HTTP operation
  int rc = http.GET();
  esp_task_wdt_reset(); // Feed watchdog after HTTP operation
  if (rc != 200) {
    Serial.printf("❌ SatoNak days since ATH GET failed (%d)\n", rc);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  // Parse days since ATH - API returns plain text like "45"
  payload.trim();
  if (payload.length() > 0 && payload.length() < 8 && payload != "na") {
    int days = payload.toInt();
    if (days >= 0) {
      daysSinceAth = payload; // Store raw value
      
      // Format for display - "## Days" format for top row
      snprintf(daysAthText, sizeof(daysAthText), "%d Days", days);
      
      Serial.printf("✅ SatoNak Days Since ATH: %s -> Display: %s | Free heap: %d\n", 
                    daysSinceAth.c_str(), daysAthText, ESP.getFreeHeap());
      return true;
    }
  }
  
  Serial.println("❌ SatoNak days since ATH: invalid response");
  Serial.println("↪︎ Payload: " + payload.substring(0, 100));
  return false;
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
      P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
      P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
      P.setFont(nullptr);
      P.setIntensity(BRIGHTNESS);  // Set initial brightness

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

      // 🕒 Time Config - only set default if not already configured in loadSavedSettingsAndConnect()
      if (!wifiConnected) {
        Serial.println("🕒 Configuring default timezone (Mountain Time)...");
        configTzTime(TIMEZONE_STRINGS[11], ntpServer); // Default to Mountain Time
      }

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
    String currency = request->getParam("currency", true)->value();  // 🌍 New
    String theme = request->getParam("theme", true)->value();        // 🎨 New

    Serial.println("✅ Saving WiFi Settings:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("City: " + city);
    Serial.println("Timezone: " + timezone);
    Serial.println("Currency: " + currency);                        // 🌍 New
    Serial.println("Theme: " + theme);                              // 🎨 New

    prefs.begin("stacksworth", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("city", city);
    prefs.putString("currency", currency);                          // 🌍 Store currency
    prefs.putString("theme", theme);                                // 🎨 Store theme
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

      // Brightness control endpoint
      server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("level")) {
          String levelStr = request->getParam("level")->value();
          uint8_t newBrightness = levelStr.toInt();
          if (newBrightness >= 1 && newBrightness <= 15) {
            setBrightness(newBrightness);
            request->send(200, "text/plain", "Brightness set to " + String(BRIGHTNESS));
          } else {
            request->send(400, "text/plain", "Invalid brightness level. Use 1-15");
          }
        } else {
          request->send(200, "text/plain", "Current brightness: " + String(BRIGHTNESS) + "/15");
        }
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

      bootMs = millis();

      // Initial API Fetch
      Serial.println("🌍 Fetching initial data...");
      fetchBitcoinData();
      fetchBlockHeight();
      fetchMinerFromSatoNak();
      fetchHashrateFromSatoNak();
      fetchCircSupplyFromSatoNak();
      fetchAthFromSatoNak();
      fetchDaysSinceAthFromSatoNak();
      fetchChange24hFromSatoNak();
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

      // Initialize watchdog timer BEFORE any HTTP calls
      esp_task_wdt_config_t wdt_config = {
          .timeout_ms = 12000,                             // 12 seconds
          .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // All cores
          .trigger_panic = true                            // Reset if not fed in time
      };
      esp_task_wdt_init(&wdt_config);
      esp_task_wdt_add(NULL); // Add current task to WDT

      bootMs = millis();

      // Initial API Fetch
      Serial.println("🌍 Fetching initial data...");
    }

    

    
    void loop()
    {
      esp_task_wdt_reset();           // Reset watchdog
      dnsServer.processNextRequest(); // Handle captive portal DNS magic


// 🛠️ Smash Buy Button Polling (Debounced)
static bool lastButtonState = HIGH;
bool currentButtonState = digitalRead(BUTTON_PIN);

if (lastButtonState == HIGH && currentButtonState == LOW) {
  // Falling edge: button was released, now pressed
  Serial.println("🚨 SMASH BUY Button Pressed!");
  buttonPressed = true;
}

lastButtonState = currentButtonState;

// USED FOR RANDOM PHRASES
if (buttonPressed) {
  buttonPressed = false;  // consume the event so it fires once

int idx = random(NUM_PHRASES);
const char* topLine    = PHRASES[idx][0];
const char* bottomLine = PHRASES[idx][1];

// Optional: small press lockout to avoid double-fires on long press/bounce
static unsigned long pressLockUntil = 0;
if (millis() < pressLockUntil) return;
pressLockUntil = millis() + 600; // 0.6s cooldown

// Show the message (nice and clean fade). Use your zone IDs as you already do.
// If you prefer, you can show the same phrase on both lines for impact.
P.displayClear();
P.displayZoneText(1, topLine,    PA_CENTER, 0, 2500, PA_FADE, PA_FADE);
P.displayZoneText(0, bottomLine, PA_CENTER, 0, 2500, PA_FADE, PA_FADE);

// Let the animation finish while keeping WDT happy (ESP32)
while (!P.displayAnimate()) {
  esp_task_wdt_reset();
  delay(10);
}

P.displayClear();
P.synchZoneStart();
esp_task_wdt_reset(); // Feed watchdog after animation complete
Serial.print("🎯 Smash Buy: ");
Serial.print(topLine);
Serial.print(" / ");
Serial.println(bottomLine);

}




  unsigned long currentMillis = millis();
      

      // ✅ Monitor heap health every 60 seconds
      static unsigned long lastMemoryCheck = 0;
      static unsigned long lastHeapLog = 0;
      if (currentMillis - lastMemoryCheck >= 60000)
      {
        Serial.printf("🧠 Free heap: %d | Min ever: %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
        lastMemoryCheck = currentMillis;
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

      // ── HTTP scheduler: serialize network calls to avoid overlap
if (WiFi.status() == WL_CONNECTED) {
  uint32_t now = millis();

  // 1) BTC every BTC_INTERVAL
  if (now - lastBTC >= BTC_INTERVAL) {
    esp_task_wdt_reset(); // Feed watchdog before network operations
    fetchBitcoinData();
    lastBTC = now;
    esp_task_wdt_reset(); // Feed watchdog after network operations
  }
  // 2) Fee at +offset
  else if ((now - lastFee >= (FEE_INTERVAL + FEE_OFFSET)) && (now >= bootMs + FEE_OFFSET)) {
    esp_task_wdt_reset(); // Feed watchdog before network operations
    fetchFeeRate();
    lastFee = now;
    esp_task_wdt_reset(); // Feed watchdog after network operations
  }
  // 3) Block height at +offset
  else if ((now - lastBlock >= (BLOCK_INTERVAL + BLOCK_OFFSET)) && (now >= bootMs + BLOCK_OFFSET)) {
    esp_task_wdt_reset(); // Feed watchdog before network operations
    fetchBlockHeight();
    fetchMinerFromSatoNak();
    fetchHashrateFromSatoNak();
    fetchCircSupplyFromSatoNak();
    fetchAthFromSatoNak();
    fetchDaysSinceAthFromSatoNak();
    fetchChange24hFromSatoNak();
    lastBlock = now;
    esp_task_wdt_reset(); // Feed watchdog after network operations
  }
  // 4) Weather seldom, with a small offset
  else if ((now - lastWeather >= (WEATHER_INTERVAL + WEATHER_OFFSET)) && (now >= bootMs + WEATHER_OFFSET)) {
    esp_task_wdt_reset(); // Feed watchdog before network operations
    fetchWeather();
    lastWeather = now;
    esp_task_wdt_reset(); // Feed watchdog after network operations
  }
}



      // 🖥️ Rotate screens
  if (P.displayAnimate()) {
    Serial.print("🖥️ Displaying screen: ");
    Serial.println(displayCycle);

    switch (displayCycle) {
      case 0: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying BLOCK screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "BLOCK", blockText); 
        P.displayZoneText(ZONE_UPPER, "BLOCK",   PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, blockText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 1: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying MINER screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "MINER", minerName.c_str());
        P.displayZoneText(ZONE_UPPER, "MINED BY", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, minerName.c_str(), PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 2: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying CIRCULATING SUPPLY screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", circSupplyText, circPercentText);
        P.displayZoneText(ZONE_UPPER, circSupplyText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, circPercentText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 3: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        String currentFiat = getCurrentFiatCode();
        static char priceLabel[32];  // Static to persist after case ends
        snprintf(priceLabel, sizeof(priceLabel), "%s PRICE", currentFiat.c_str());
        Serial.println("🖥️ Displaying " + currentFiat + " PRICE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", priceLabel, btcText);
        Serial.printf("🔍 DEBUG: btcText='%s', length=%d\n", btcText, strlen(btcText));
        P.displayZoneText(ZONE_UPPER, priceLabel, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, btcText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear();
        P.synchZoneStart();
        break;
      } 

          
      case 4: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying 24H CHANGE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "24H CHANGE", changeText);
        P.displayZoneText(ZONE_UPPER, "24H CHANGE", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, changeText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear();
        P.synchZoneStart();
        break;
      }

      case 5: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying ATH PRICE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "ATH", athText);
        P.displayZoneText(ZONE_UPPER, "ATH", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, athText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 6: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying DAYS SINCE ATH screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", daysAthText, "Since ATH");
        P.displayZoneText(ZONE_UPPER, daysAthText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, "Since ATH", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 7: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        String currentFiat = getCurrentFiatCode();
        static char satsLabel[32];  // Static to persist after case ends
        snprintf(satsLabel, sizeof(satsLabel), "SATS/%s", currentFiat.c_str());
        Serial.println("🖥️ Displaying SATS per " + currentFiat + " screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", satsLabel, satsText);
        P.displayZoneText(ZONE_UPPER, satsLabel, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, satsText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization  
        break;
      }
        
      case 8: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying FEE RATE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "FEE RATE", feeText);
        P.displayZoneText(ZONE_UPPER, "FEE RATE", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, feeText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      case 9: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying HASHRATE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "HASHRATE", hashrateText);
        P.displayZoneText(ZONE_UPPER, "HASHRATE", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, hashrateText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

        
      case 10: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying TIME and City screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "TIME", timeText);
        P.displayZoneText(ZONE_UPPER, savedCity.c_str(), PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, timeText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

        
      case 11: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying DAY/DATE screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", dayText, dateText);
        P.displayZoneText(ZONE_UPPER, dayText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, dateText, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

        
      case 12: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
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
        P.displayZoneText(ZONE_UPPER, tempDisplay, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, condDisplay, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization
        break;
      }

      

      case 13: {
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        Serial.println("🖥️ Displaying MOSCOW TIME screen...");
        Serial.printf("🔤 Displaying text: %s (Top), %s (Bottom)\n", "MOSCOW TIME", satsText2);
        P.displayZoneText(ZONE_UPPER, "MOSCOW TIME", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayZoneText(ZONE_LOWER, satsText2, PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut);
        P.displayClear(); //  Force clear
        P.synchZoneStart(); // Force synchronization  
        break;
      }

      case 14: {// This is for the models we ship but can be changed for custom units
        textEffect_t effectIn, effectOut;
        getThemeEffects(effectIn, effectOut);
        P.displayZoneText(ZONE_UPPER, "Satoshi", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut); 
        P.displayZoneText(ZONE_LOWER, "Nakamoto", PA_CENTER, SCROLL_SPEED, 10000, effectIn, effectOut); 
        P.displayClear();
        P.synchZoneStart();
        break;
      }
    }

      Serial.println("✅ Screen update complete.");
      displayCycle = (displayCycle + 1) % 15;
      
    }
  }
