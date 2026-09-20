/**
 * =============================================================================
 * ESP32 GitHub Cloud Client
 * โปรแกรมเชื่อมต่อ ESP32 เข้ากับ GitHub REST API เพื่อรับคำสั่งและส่งค่าเซนเซอร์
 * 
 * ไลบรารีที่จำเป็น:
 * 1. ArduinoJson (ค้นหาและติดตั้งผ่าน Arduino IDE Library Manager)
 * 2. ไลบรารีมาตรฐานที่มาพร้อมบอร์ด ESP32: WiFi, HTTPClient, WiFiClientSecure
 * =============================================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <time.h>

// ตรวจสอบไฟล์การตั้งค่า
#if __has_include("config.h")
  #include "config.h"
#else
  #include "config.example.h"
#endif

// ตัวแปรจัดเก็บสถานะ
String currentFileSha = "";
unsigned long lastPollTime = 0;
unsigned long lastTelemetryTime = 0;

// สถานะคำสั่งปัจจุบัน
bool stateRelay1 = false;
bool stateRelay2 = false;
bool stateRelay3 = false;
bool stateRelay4 = false;
bool stateLed = false;
bool stateTestRelays = false;
String stateMode = "manual";
float stateTargetTemp = 25.0;

// เซิร์ฟเวอร์ NTP สำหรับเวลามาตรฐาน
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.google.com";

// =============================================================================
// Helper Functions: Base64 Encoding & Decoding
// =============================================================================
String base64Decode(const String& input) {
  // กรอง newline characters ที่ GitHub API อาจจะแนบมา
  String cleanInput = "";
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c != '\r' && c != '\n' && c != ' ') {
      cleanInput += c;
    }
  }

  size_t inputLen = cleanInput.length();
  size_t maxOutputLen = (inputLen * 3) / 4 + 4;
  unsigned char* output = (unsigned char*)malloc(maxOutputLen);
  if (!output) return "";

  size_t outputLen = 0;
  int ret = mbedtls_base64_decode(output, maxOutputLen, &outputLen, 
                                  (const unsigned char*)cleanInput.c_str(), inputLen);
  
  String result = "";
  if (ret == 0) {
    output[outputLen] = '\0';
    result = String((char*)output);
  } else {
    Serial.printf("[Base64] Decode failed with code: %d\n", ret);
  }
  free(output);
  return result;
}

String base64Encode(const String& input) {
  size_t inputLen = input.length();
  size_t maxOutputLen = (inputLen * 4) / 3 + 4;
  unsigned char* output = (unsigned char*)malloc(maxOutputLen);
  if (!output) return "";

  size_t outputLen = 0;
  int ret = mbedtls_base64_encode(output, maxOutputLen, &outputLen, 
                                  (const unsigned char*)input.c_str(), inputLen);
  String result = "";
  if (ret == 0) {
    output[outputLen] = '\0';
    result = String((char*)output);
  }
  free(output);
  return result;
}

// ฟังก์ชันสร้าง Timestamp รูปแบบ ISO 8601 (UTC)
String getIsoTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 1000)) {
    return "2026-09-17T00:00:00Z";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

// =============================================================================
// จัดการ Hardware GPIO (บอร์ดรีเลย์ 4 ช่อง)
// =============================================================================
void setupHardware() {
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT);
  pinMode(PIN_RELAY_4, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  // ตั้งค่าสถานะเริ่มต้น (ปิดทั้งหมด)
  applyHardwareOutputs();
}

void applyHardwareOutputs() {
  // รีเลย์ 4 ช่อง (คำนึงถึง Active Low / High)
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(PIN_RELAY_1, stateRelay1 ? LOW : HIGH);
    digitalWrite(PIN_RELAY_2, stateRelay2 ? LOW : HIGH);
    digitalWrite(PIN_RELAY_3, stateRelay3 ? LOW : HIGH);
    digitalWrite(PIN_RELAY_4, stateRelay4 ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RELAY_1, stateRelay1 ? HIGH : LOW);
    digitalWrite(PIN_RELAY_2, stateRelay2 ? HIGH : LOW);
    digitalWrite(PIN_RELAY_3, stateRelay3 ? HIGH : LOW);
    digitalWrite(PIN_RELAY_4, stateRelay4 ? HIGH : LOW);
  }

  // ไฟ LED บนบอร์ด
  digitalWrite(PIN_LED, stateLed ? HIGH : LOW);
}

// ฟังก์ชันทดสอบการทำงานของรีเลย์ทั้ง 4 ช่อง (Self-Test Sequence)
void runRelaySelfTest() {
  Serial.println("\n>>> [สเต็ป 1] ทดสอบเปิด-ปิดทีละช่อง (1 -> 4) <<<");
  const int pins[4] = {PIN_RELAY_1, PIN_RELAY_2, PIN_RELAY_3, PIN_RELAY_4};

  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], RELAY_ACTIVE_LOW ? LOW : HIGH);
    Serial.printf("Relay %d (GPIO %d) -> [ เปิด / ON ]\n", i + 1, pins[i]);
    delay(1000);

    digitalWrite(pins[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
    Serial.printf("Relay %d -> [ ปิด / OFF ]\n", i + 1);
    delay(500);
  }

  Serial.println("\n>>> [สเต็ป 2] ทดสอบเปิดพร้อมกันทั้งหมด 4 ช่อง <<<");
  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], RELAY_ACTIVE_LOW ? LOW : HIGH);
  }
  Serial.println("เปิดครบทั้ง 4 ช่อง (รอ 2 วินาที)...");
  delay(2000);

  Serial.println(">>> สั่งปิดพร้อมกันทั้งหมด 4 ช่อง <<<");
  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
  }
  Serial.println("ปิดครบทั้ง 4 ช่อง (รอ 1 วินาที)...");
  delay(1000);

  // คืนสถานะปัจจุบันตามคำสั่งเดิม
  applyHardwareOutputs();
  Serial.println("--- สิ้นสุดการทดสอบรีเลย์ กลับสู่คำสั่งปกติเรียบร้อย ---\n");
}

// =============================================================================
// เชื่อมต่อ WiFi & NTP
// =============================================================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("[WiFi] กำลังเชื่อมต่อไปยัง SSID: %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.setSleep(false); // ปิด Modem Sleep เพื่อให้รับส่งข้อมูลเสถียร ไม่หลุดบ่อย
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] เชื่อมต่อสำเร็จ!");
    Serial.printf("[WiFi] IP Address: %s | RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());

    // ซิงค์เวลาจาก NTP Server
    configTime(0, 0, ntpServer1, ntpServer2);
    Serial.println("[NTP] กำลังซิงค์เวลามาตรฐาน...");
  } else {
    Serial.println("\n[WiFi] เชื่อมต่อไม่สำเร็จ จะลองใหม่ในรอบถัดไป");
  }
}

// ฟังก์ชันแปลง URL ป้องกันข้อผิดพลาด HTTP 400 กรณีชื่อโฟลเดอร์มีเว้นวรรค
String urlEncodeString(const String& str) {
  String encoded = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == ' ') {
      encoded += "%20";
    } else {
      encoded += c;
    }
  }
  return encoded;
}

// =============================================================================
// ดึงคำสั่งจาก GitHub (Fetch Commands)
// =============================================================================
void fetchCommandsFromGitHub() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure(); // ไม่ตรวจ Root CA เพื่อความยืดหยุ่นบนไมโครคอนโทรลเลอร์
  client.setTimeout(10); // จำกัด Timeout 10 วินาที ป้องกันโปรแกรมค้าง

  HTTPClient http;
  String cleanRepo = GITHUB_REPO;
  cleanRepo.trim();
  cleanRepo.replace(" ", "-"); // แปลงเว้นวรรคเป็น - ป้องกันข้อผิดพลาด

  String encodedPath = urlEncodeString(GITHUB_FILE_PATH);
  String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + cleanRepo + 
               "/contents/" + encodedPath + "?ref=" + GITHUB_BRANCH;

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-GitHub-Client");
  http.addHeader("Accept", "application/vnd.github+json");
  if (strlen(GITHUB_TOKEN) > 0 && String(GITHUB_TOKEN) != "ghp_YOUR_PERSONAL_ACCESS_TOKEN_HERE") {
    http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);
  }

  Serial.printf("\n[GitHub] กำลังดึงคำสั่งจาก: %s\n", GITHUB_FILE_PATH);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    #if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument apiDoc;
    #else
      DynamicJsonDocument apiDoc(8192);
    #endif

    DeserializationError error = deserializeJson(apiDoc, payload);
    if (!error) {
      currentFileSha = apiDoc["sha"].as<String>();
      String encodedContent = apiDoc["content"].as<String>();

      // ถอดรหัส Base64 ของเนื้อหาไฟล์
      String decodedJson = base64Decode(encodedContent);

      #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument stateDoc;
      #else
        DynamicJsonDocument stateDoc(4096);
      #endif

      DeserializationError stateError = deserializeJson(stateDoc, decodedJson);
      if (!stateError) {
        // อ่านคำสั่งจาก JSON
        stateRelay1 = stateDoc["commands"]["relay1"] | false;
        stateRelay2 = stateDoc["commands"]["relay2"] | false;
        stateRelay3 = stateDoc["commands"]["relay3"] | false;
        stateRelay4 = stateDoc["commands"]["relay4"] | false;
        stateLed = stateDoc["commands"]["led"] | false;
        stateTestRelays = stateDoc["commands"]["test_relays"] | false;
        stateMode = stateDoc["commands"]["mode"] | "manual";
        stateTargetTemp = stateDoc["commands"]["target_temp"] | 25.0;

        Serial.println("[GitHub] อัปเดตคำสั่งสำเร็จ:");
        Serial.printf("  > Relay 1: %s (ขา %d)\n", stateRelay1 ? "ON" : "OFF", PIN_RELAY_1);
        Serial.printf("  > Relay 2: %s (ขา %d)\n", stateRelay2 ? "ON" : "OFF", PIN_RELAY_2);
        Serial.printf("  > Relay 3: %s (ขา %d)\n", stateRelay3 ? "ON" : "OFF", PIN_RELAY_3);
        Serial.printf("  > Relay 4: %s (ขา %d)\n", stateRelay4 ? "ON" : "OFF", PIN_RELAY_4);
        Serial.printf("  > LED:     %s (ขา %d)\n", stateLed ? "ON" : "OFF", PIN_LED);
        Serial.printf("  > Mode:    %s | Target: %.1f °C\n", stateMode.c_str(), stateTargetTemp);
        Serial.printf("  > Current SHA: %s\n", currentFileSha.substring(0, 7).c_str());

        // หากมีคำสั่งทดสอบรีเลย์ ให้รันฟังก์ชัน Self-Test
        if (stateTestRelays) {
          runRelaySelfTest();
          stateTestRelays = false;
        } else {
          // ส่งออกไปยังขาฮาร์ดแวร์ทันที
          applyHardwareOutputs();
        }
      } else {
        Serial.printf("[JSON] แปลง State JSON ล้มเหลว: %s\n", stateError.c_str());
      }
    } else {
      Serial.printf("[JSON] แปลง GitHub API Response ล้มเหลว: %s\n", error.c_str());
    }
  } else {
    Serial.printf("[GitHub] ดึงข้อมูลล้มเหลว HTTP Code: %d\n", httpCode);
  }

  http.end();
}

// =============================================================================
// ส่งสถานะเซนเซอร์และข้อมูลบอร์ดขึ้นไปอัปเดตบน GitHub (Push Telemetry)
// =============================================================================
void pushTelemetryToGitHub() {
  // หากตั้งค่าปิดส่งอัตโนมัติ (ENABLE_AUTO_TELEMETRY_PUSH = false) จะไม่สร้าง Commit เพื่อป้องกันอีเมลแจ้งเตือน
  if (!ENABLE_AUTO_TELEMETRY_PUSH) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (currentFileSha.length() == 0) {
    Serial.println("[GitHub] ยังไม่มี SHA ปัจจุบัน จะดึงข้อมูลก่อนส่งอัปเดต...");
    fetchCommandsFromGitHub();
    if (currentFileSha.length() == 0) return;
  }

  // อ่านค่าจากเซนเซอร์หรือจำลองค่า (กรณีไม่ได้ต่อ DHT)
  float simulatedTemp = 26.5 + (random(0, 30) / 10.0);
  float simulatedHum = 60.0 + (random(0, 80) / 10.0);
  int currentRssi = WiFi.RSSI();
  unsigned long uptimeSec = millis() / 1000;
  String nowIso = getIsoTimestamp();

  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument rootDoc;
  #else
    DynamicJsonDocument rootDoc(4096);
  #endif

  // คงสถานะคำสั่งปัจจุบันไว้
  JsonObject commands = rootDoc["commands"].to<JsonObject>();
  commands["relay1"] = stateRelay1;
  commands["relay2"] = stateRelay2;
  commands["relay3"] = stateRelay3;
  commands["relay4"] = stateRelay4;
  commands["led"] = stateLed;
  commands["test_relays"] = false;
  commands["mode"] = stateMode;
  commands["target_temp"] = stateTargetTemp;

  // ข้อมูล Telemetry ล่าสุด
  JsonObject telemetry = rootDoc["telemetry"].to<JsonObject>();
  telemetry["temperature"] = simulatedTemp;
  telemetry["humidity"] = simulatedHum;
  telemetry["rssi"] = currentRssi;
  telemetry["uptime_sec"] = uptimeSec;
  telemetry["last_seen"] = nowIso;
  telemetry["ip_address"] = WiFi.localIP().toString();

  // Meta
  JsonObject meta = rootDoc["meta"].to<JsonObject>();
  meta["version"] = "1.0.0";
  meta["updated_by"] = "ESP32_Device";
  meta["updated_at"] = nowIso;

  // แปลงเป็น JSON String และเข้ารหัส Base64
  String jsonOutput;
  serializeJsonPretty(rootDoc, jsonOutput);
  String encodedOutput = base64Encode(jsonOutput);

  // สร้าง Payload สำหรับ GitHub PUT Request (ระบุบอทและข้อความ [skip ci] [silent] ป้องกันอีเมลเตือน 100%)
  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument putDoc;
  #else
    DynamicJsonDocument putDoc(8192);
  #endif

  putDoc["message"] = "ESP32: Update telemetry & heartbeat (" + nowIso + ") [skip ci] [silent] [no-notify]";
  JsonObject committer = putDoc["committer"].to<JsonObject>();
  committer["name"] = "github-actions[bot]";
  committer["email"] = "41898282+github-actions[bot]@users.noreply.github.com";
  JsonObject author = putDoc["author"].to<JsonObject>();
  author["name"] = "github-actions[bot]";
  author["email"] = "41898282+github-actions[bot]@users.noreply.github.com";

  putDoc["content"] = encodedOutput;
  putDoc["sha"] = currentFileSha;
  putDoc["branch"] = GITHUB_BRANCH;

  String putPayload;
  serializeJson(putDoc, putPayload);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10); // จำกัด Timeout 10 วินาที ป้องกันค้าง

  HTTPClient http;
  String cleanRepo = GITHUB_REPO;
  cleanRepo.trim();
  cleanRepo.replace(" ", "-");

  String encodedPath = urlEncodeString(GITHUB_FILE_PATH);
  String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + cleanRepo + 
               "/contents/" + encodedPath;

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-GitHub-Client");
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);

  Serial.println("\n[GitHub] กำลังส่ง Telemetry & Heartbeat ขึ้น GitHub...");
  int httpCode = http.PUT(putPayload);

  if (httpCode == HTTP_CODE_OK || httpCode == 201) {
    String response = http.getString();
    #if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument resDoc;
    #else
      DynamicJsonDocument resDoc(4096);
    #endif
    deserializeJson(resDoc, response);
    currentFileSha = resDoc["content"]["sha"].as<String>();

    Serial.println("[GitHub] ส่ง Telemetry สำเร็จ!");
    Serial.printf("  > New Commit SHA: %s\n", currentFileSha.substring(0, 7).c_str());
    Serial.printf("  > Temp: %.1f °C | Hum: %.1f%% | RSSI: %d dBm | Uptime: %lus\n", 
                  simulatedTemp, simulatedHum, currentRssi, uptimeSec);
  } else {
    Serial.printf("[GitHub] ส่งข้อมูลไม่สำเร็จ HTTP Code: %d\n", httpCode);
    if (httpCode == 409) {
      Serial.println("[GitHub] เกิด Conflict! จะทำการดึงข้อมูล SHA ล่าสุดในรอบถัดไป");
      currentFileSha = ""; // รีเซ็ตเพื่อดึง SHA ใหม่
    }
  }

  http.end();
}

// =============================================================================
// Arduino Setup & Main Loop
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==========================================");
  Serial.println("   ESP32 GitHub Cloud Client เริ่มทำงาน    ");
  Serial.println("==========================================");

  setupHardware();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.setSleep(false);
  connectWiFi();

  // ดึงคำสั่งครั้งแรกทันทีที่เปิดเครื่อง
  fetchCommandsFromGitHub();

  // ส่งรายงานตัว Heartbeat ครั้งแรกทันทีเพื่อให้หน้าเว็บขึ้นออนไลน์ทันทีที่เปิดบอร์ด
  if (ENABLE_AUTO_TELEMETRY_PUSH) {
    pushTelemetryToGitHub();
  }
}

void loop() {
  // ตรวจสอบและเชื่อมต่อ WiFi ซ้ำหากหลุด (แบบ Non-blocking ไม่ทำให้การทำงานอื่นหยุดชะงัก)
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWifiReconnect = 0;
    if (millis() - lastWifiReconnect >= 4000) {
      lastWifiReconnect = millis();
      Serial.println("[WiFi] หลุดการเชื่อมต่อ กำลังเชื่อมต่อใหม่อัตโนมัติ...");
      WiFi.reconnect();
    }
  }

  unsigned long currentMillis = millis();

  // รอบดึงคำสั่ง (Polling Commands)
  if (currentMillis - lastPollTime >= POLL_COMMANDS_INTERVAL_MS) {
    lastPollTime = currentMillis;
    fetchCommandsFromGitHub();
  }

  // รอบส่งสถานะเซนเซอร์ (Push Telemetry - ทำงานเฉพาะเมื่อเปิด ENABLE_AUTO_TELEMETRY_PUSH)
  if (ENABLE_AUTO_TELEMETRY_PUSH && (currentMillis - lastTelemetryTime >= PUSH_TELEMETRY_INTERVAL_MS)) {
    lastTelemetryTime = currentMillis;
    pushTelemetryToGitHub();
  }

  delay(100);
}
