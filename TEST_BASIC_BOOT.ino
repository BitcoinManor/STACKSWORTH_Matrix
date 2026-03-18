// 🧪 MINIMAL ESP32 BOOT TEST
// Tests ONLY core ESP32 functionality - no libraries, no SPI, no matrix
// If this fails, the ESP32 itself is damaged

void setup() {
  Serial.begin(115200);
  delay(500);  // Give serial time to stabilize
  
  Serial.println("========================================");
  Serial.println("✅ ESP32 BOOT SUCCESS!");
  Serial.println("✅ Serial communication working");
  Serial.println("✅ Code execution reached setup()");
  Serial.println("========================================");
  
  // Test basic GPIO (LED on pin 2)
  pinMode(2, OUTPUT);
  Serial.println("✅ GPIO initialized (pin 2)");
  
  Serial.println("");
  Serial.println("🔍 DIAGNOSTIC INFO:");
  Serial.print("   Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("   Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("   Flash Size: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.println("");
  Serial.println("✨ If you can see this, ESP32 core is OK!");
  Serial.println("   Problem is likely in libraries or SPI hardware.");
}

void loop() {
  // Blink LED on pin 2
  digitalWrite(2, HIGH);
  Serial.println("💡 LED ON  - Loop running...");
  delay(500);
  
  digitalWrite(2, LOW);
  Serial.println("💡 LED OFF - Loop running...");
  delay(500);
}
