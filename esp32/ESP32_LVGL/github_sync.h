#pragma once
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <time.h>

// -------------------------------------------------------------
// ตัวแปรและสถานะสำหรับ GitHub Synchronization
// -------------------------------------------------------------
inline String lvglFileSha = "";
inline unsigned long lastCloudPollTime = 0;
inline unsigned long lastCloudHeartbeatTime = 0;

// ประกาศตัวแปร Global ที่แชร์กับ config.h
bool is_syncing_from_cloud = false;
float cloud_temp = 25.0;
float cloud_hum = 60.0;
inline String cloud_mode = "manual";
inline float cloud_target_temp = 25.0;

// ฟังก์ชันประกาศล่วงหน้าสำหรับอัปเดต UI ใน ui_screens.h
void update_relay_card_style(int ch);
void update_led_card_style();

// -------------------------------------------------------------
// Helper: URL Encode
// -------------------------------------------------------------
inline String urlEncode(const String& str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) code0 = c - 10 + 'A';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

// -------------------------------------------------------------
// Helper: Base64 Decode & Encode
// -------------------------------------------------------------
inline String lvglBase64Decode(const String& input) {
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
  }
  free(output);
  return result;
}

inline String lvglBase64Encode(const String& input) {
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

// -------------------------------------------------------------
// Helper: เวลามาตรฐาน ISO 8601
// -------------------------------------------------------------
inline String getLvglIsoTimestamp() {
  time_t now;
  time(&now);
  struct tm timeinfo;
  if (!gmtime_r(&now, &timeinfo) || now < 100000) {
    return "2026-09-22T00:00:00Z";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

// -------------------------------------------------------------
// 1. ดึงคำสั่งล่าสุดจาก GitHub (Fetch Remote State)
// -------------------------------------------------------------
inline bool fetchCloudCommands() {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(8);

  HTTPClient http;
  String cleanRepo = GITHUB_REPO;
  cleanRepo.trim();
  cleanRepo.replace(" ", "-");

  String encodedPath = urlEncode(GITHUB_FILE_PATH);
  String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + cleanRepo + 
               "/contents/" + encodedPath + "?ref=" + GITHUB_BRANCH;

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-LVGL-Display");
  http.addHeader("Accept", "application/vnd.github+json");
  if (strlen(GITHUB_TOKEN) > 0) {
    http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);
  }

  int httpCode = http.GET();
  bool success = false;

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    #if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument apiDoc;
    #else
      DynamicJsonDocument apiDoc(8192);
    #endif

    DeserializationError err = deserializeJson(apiDoc, payload);
    if (!err) {
      lvglFileSha = apiDoc["sha"].as<String>();
      String encodedContent = apiDoc["content"].as<String>();
      String decodedJson = lvglBase64Decode(encodedContent);

      #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument stateDoc;
      #else
        DynamicJsonDocument stateDoc(4096);
      #endif

      DeserializationError stateErr = deserializeJson(stateDoc, decodedJson);
      if (!stateErr) {
        success = true;

        // อ่านคำสั่งจากคลาวด์
        bool remoteRelays[4];
        remoteRelays[0] = stateDoc["commands"]["relay1"] | false;
        remoteRelays[1] = stateDoc["commands"]["relay2"] | false;
        remoteRelays[2] = stateDoc["commands"]["relay3"] | false;
        remoteRelays[3] = stateDoc["commands"]["relay4"] | false;
        bool remoteLed = stateDoc["commands"]["led"] | false;

        // บันทึก mode และ target_temp จากคลาวด์ไว้เพื่อไม่ให้ถูกเขียนทับ
        if (stateDoc["commands"].is<JsonObject>()) {
          cloud_mode = stateDoc["commands"]["mode"] | cloud_mode;
          cloud_target_temp = stateDoc["commands"]["target_temp"] | cloud_target_temp;
        }

        // อ่านค่าอุณหภูมิและความชื้นจากบอร์ดรีเลย์เพื่อไปแสดงที่ Weather
        if (stateDoc["telemetry"].is<JsonObject>()) {
          cloud_temp = stateDoc["telemetry"]["temperature"] | cloud_temp;
          cloud_hum = stateDoc["telemetry"]["humidity"] | cloud_hum;
        }

        // ตรวจสอบว่ามีการเปลี่ยนแปลงคำสั่งจากภายนอกหรือไม่ พร้อมระบบ Cooldown Hold Guard
        unsigned long now = millis();
        bool changed = false;
        for (int i = 0; i < 4; i++) {
          // หากสวิตช์แชนเนลนี้เพิ่งถูกแตะหน้าจอไม่เกิน 4 วินาที ให้ข้าม (ไม่ยอมให้ค่าเก่าจากคลาวด์มาทับ)
          if (now - lastTouchTime[i] >= MUTATION_HOLD_TIME_MS) {
            if (relay_states[i] != remoteRelays[i]) {
              relay_states[i] = remoteRelays[i];
              int pin = (i == 0) ? PIN_RELAY_1 : (i == 1) ? PIN_RELAY_2 : (i == 2) ? PIN_RELAY_3 : PIN_RELAY_4;
              digitalWrite(pin, (RELAY_ACTIVE_LOW ? !relay_states[i] : relay_states[i]));
              changed = true;
            }
          }
        }
        if (now - lastLedTouchTime >= MUTATION_HOLD_TIME_MS) {
          if (led_state != remoteLed) {
            led_state = remoteLed;
            digitalWrite(PIN_LED_BOARD, led_state ? HIGH : LOW);
            changed = true;
          }
        }

        if (changed) {
          Serial.println("[LVGL-SYNC] พบคำสั่งใหม่จาก Cloud อัปเดตสวิตช์บนหน้าจอ...");
          // ตั้งค่า Flag ป้องกันไม่ให้ Event Listener ของสวิตช์ส่งคำสั่งย้อนกลับไปคลาวด์ซ้ำ (Anti-Echo)
          is_syncing_from_cloud = true;

          // ป้องกันความปลอดภัยของหน่วยความจำ LVGL ด้วย Mutex
          if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < 4; i++) {
              update_relay_card_style(i);
            }
            update_led_card_style();
            xSemaphoreGive(lvgl_mutex);
          }

          is_syncing_from_cloud = false;
        }

        // อัปเดต Badge บนหน้าจอ Cloud Hub
        if (cloud_status_badge) {
          if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            lv_label_set_text(cloud_status_badge, LV_SYMBOL_WIFI " Synced");
            lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0x10B981), 0);
            xSemaphoreGive(lvgl_mutex);
          }
        }
      }
    }
  } else {
    Serial.printf("[LVGL-SYNC] ดึงคำสั่งไม่สำเร็จ HTTP Code: %d\n", httpCode);
    if (httpCode == 409) lvglFileSha = "";
  }

  http.end();
  return success;
}

// -------------------------------------------------------------
// 2. คิวคำสั่งสัมผัสหน้าจอแบบ Asynchronous (Non-blocking GUI)
// -------------------------------------------------------------
inline volatile bool hasPendingCloudCommand = false;
inline volatile unsigned long pendingCommandRequestTime = 0;
inline String pendingActionName = "";

inline void queueCloudCommand(const String& actionName) {
  pendingActionName = actionName;
  hasPendingCloudCommand = true;
  pendingCommandRequestTime = millis();
}

// -------------------------------------------------------------
// ฟังก์ชันสำหรับส่งคำสั่งจากหน้าจอสัมผัส (รันไวใน 0.05ms ไม่บล็อกหน้าจอ)
// -------------------------------------------------------------
inline bool sendCloudCommand(int targetRelay, bool targetState) {
  if (is_syncing_from_cloud) return true;
  if (targetRelay >= 0 && targetRelay < 4) {
    lastTouchTime[targetRelay] = millis();
  }
  String action = String("Relay ") + (targetRelay + 1) + (targetState ? " ON" : " OFF");
  queueCloudCommand(action);
  return true;
}

inline bool sendAllRelaysCloudCommand(bool targetState) {
  if (is_syncing_from_cloud) return true;
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    lastTouchTime[i] = now;
  }
  String action = String(targetState ? "ALL ON" : "ALL OFF");
  queueCloudCommand(action);
  return true;
}

inline bool sendCloudLedCommand(bool targetState) {
  if (is_syncing_from_cloud) return true;
  lastLedTouchTime = millis();
  String action = String("LED ") + (targetState ? "ON" : "OFF");
  queueCloudCommand(action);
  return true;
}

// -------------------------------------------------------------
// 3. ฟังก์ชันส่งคำสั่งจริงขึ้น GitHub (รันบน Core 0 ใน Background)
// -------------------------------------------------------------
inline bool pushCurrentStateToCloud(const String& actionName) {
  if (WiFi.status() != WL_CONNECTED) return false;

  // หากยังไม่มี SHA ให้ดึงครั้งแรกก่อน
  if (lvglFileSha.length() == 0) {
    fetchCloudCommands();
  }

  String nowIso = getLvglIsoTimestamp();

  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument rootDoc;
  #else
    DynamicJsonDocument rootDoc(4096);
  #endif

  // บันทึกสถานะคำสั่งปัจจุบันทั้งหมด
  JsonObject commands = rootDoc["commands"].to<JsonObject>();
  commands["relay1"] = relay_states[0];
  commands["relay2"] = relay_states[1];
  commands["relay3"] = relay_states[2];
  commands["relay4"] = relay_states[3];
  commands["led"] = led_state;
  commands["test_relays"] = false;
  commands["mode"] = cloud_mode;
  commands["target_temp"] = cloud_target_temp;

  // คงสถานะเซนเซอร์ Telemetry ของบอร์ดรีเลย์ไว้
  JsonObject telemetry = rootDoc["telemetry"].to<JsonObject>();
  telemetry["temperature"] = cloud_temp;
  telemetry["humidity"] = cloud_hum;
  telemetry["rssi"] = WiFi.RSSI();
  telemetry["last_seen"] = nowIso;

  // รายงานสถานะจอภาพ
  JsonObject displayObj = rootDoc["display"].to<JsonObject>();
  displayObj["online"] = true;
  displayObj["last_seen"] = nowIso;
  displayObj["ip_address"] = WiFi.localIP().toString();
  displayObj["rssi"] = WiFi.RSSI();
  displayObj["uptime_sec"] = millis() / 1000;
  displayObj["last_action"] = actionName;

  // Meta ระบุว่าการเปลี่ยนแปลงนี้มาจากหน้าจอสัมผัส
  JsonObject meta = rootDoc["meta"].to<JsonObject>();
  meta["version"] = "1.1.0";
  meta["updated_by"] = "display_touch";
  meta["updated_at"] = nowIso;

  String jsonOutput;
  serializeJsonPretty(rootDoc, jsonOutput);
  String encodedOutput = lvglBase64Encode(jsonOutput);

  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument putDoc;
  #else
    DynamicJsonDocument putDoc(8192);
  #endif

  String commitMsg = String("LVGL Display: ") + actionName + " [skip ci] [silent] [no-notify]";
  putDoc["message"] = commitMsg;
  JsonObject committer = putDoc["committer"].to<JsonObject>();
  committer["name"] = "github-actions[bot]";
  committer["email"] = "41898282+github-actions[bot]@users.noreply.github.com";
  JsonObject author = putDoc["author"].to<JsonObject>();
  author["name"] = "github-actions[bot]";
  author["email"] = "41898282+github-actions[bot]@users.noreply.github.com";
  putDoc["content"] = encodedOutput;
  putDoc["sha"] = lvglFileSha;
  putDoc["branch"] = GITHUB_BRANCH;

  String putPayload;
  serializeJson(putDoc, putPayload);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(6);

  HTTPClient http;
  String cleanRepo = GITHUB_REPO;
  cleanRepo.trim();
  cleanRepo.replace(" ", "-");

  String encodedPath = urlEncode(GITHUB_FILE_PATH);
  String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + cleanRepo + 
               "/contents/" + encodedPath;

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-LVGL-Display");
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);

  int httpCode = http.PUT(putPayload);
  bool success = false;

  if (httpCode == HTTP_CODE_OK || httpCode == 201) {
    String resp = http.getString();
    #if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument resDoc;
    #else
      DynamicJsonDocument resDoc(4096);
    #endif
    deserializeJson(resDoc, resp);
    lvglFileSha = resDoc["content"]["sha"].as<String>();
    success = true;
    Serial.printf("[LVGL-TOUCH] ส่งคำสั่ง %s สำเร็จ! SHA: %s\n", actionName.c_str(), lvglFileSha.substring(0, 7).c_str());
  } else if (httpCode == 409) {
    Serial.println("[LVGL-TOUCH] 409 Conflict - กำลังดึง SHA ล่าสุดและส่งใหม่...");
    lvglFileSha = "";
    fetchCloudCommands();
    putDoc["sha"] = lvglFileSha;
    putPayload = "";
    serializeJson(putDoc, putPayload);
    httpCode = http.PUT(putPayload);
    if (httpCode == HTTP_CODE_OK || httpCode == 201) {
      String resp = http.getString();
      #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument resDoc;
      #else
        DynamicJsonDocument resDoc(4096);
      #endif
      deserializeJson(resDoc, resp);
      lvglFileSha = resDoc["content"]["sha"].as<String>();
      success = true;
      Serial.printf("[LVGL-TOUCH] ลองใหม่อีกครั้ง ส่งคำสั่ง %s สำเร็จ!\n", actionName.c_str());
    }
  } else {
    Serial.printf("[LVGL-TOUCH] ส่งคำสั่งไม่สำเร็จ HTTP Code: %d\n", httpCode);
  }

  http.end();
  return success;
}

// -------------------------------------------------------------
// 4. ส่ง Heartbeat รายงานสถานะออนไลน์ของจอภาพขึ้น GitHub
// -------------------------------------------------------------
inline bool sendDisplayHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return false;

  // หากเพิ่งมีการสัมผัสหรือมีคำสั่งรอส่งอยู่ ให้ข้ามการส่ง Heartbeat ไปก่อนเพื่อเปิดทางด่วนให้คำสั่ง
  if (hasPendingCloudCommand) return false;
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    if (now - lastTouchTime[i] < MUTATION_HOLD_TIME_MS) return false;
  }
  if (now - lastLedTouchTime < MUTATION_HOLD_TIME_MS) return false;

  fetchCloudCommands();

  String nowIso = getLvglIsoTimestamp();

  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument rootDoc;
  #else
    DynamicJsonDocument rootDoc(4096);
  #endif

  JsonObject commands = rootDoc["commands"].to<JsonObject>();
  for (int i = 0; i < 4; i++) {
    commands[String("relay") + (i + 1)] = relay_states[i];
  }
  commands["led"] = led_state;
  commands["test_relays"] = false;
  commands["mode"] = cloud_mode;
  commands["target_temp"] = cloud_target_temp;

  JsonObject telemetry = rootDoc["telemetry"].to<JsonObject>();
  telemetry["temperature"] = cloud_temp;
  telemetry["humidity"] = cloud_hum;
  telemetry["rssi"] = WiFi.RSSI();
  telemetry["last_seen"] = nowIso;

  JsonObject displayObj = rootDoc["display"].to<JsonObject>();
  displayObj["online"] = true;
  displayObj["last_seen"] = nowIso;
  displayObj["ip_address"] = WiFi.localIP().toString();
  displayObj["rssi"] = WiFi.RSSI();
  displayObj["uptime_sec"] = millis() / 1000;
  displayObj["device_name"] = "ESP32_ST7796_Touchscreen";

  JsonObject meta = rootDoc["meta"].to<JsonObject>();
  meta["version"] = "1.1.0";
  meta["updated_by"] = "display_heartbeat";
  meta["updated_at"] = nowIso;

  String jsonOutput;
  serializeJsonPretty(rootDoc, jsonOutput);
  String encodedOutput = lvglBase64Encode(jsonOutput);

  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument putDoc;
  #else
    DynamicJsonDocument putDoc(8192);
  #endif

  putDoc["message"] = "LVGL Display: Heartbeat & Status (" + nowIso + ") [skip ci] [silent] [no-notify]";
  JsonObject committer = putDoc["committer"].to<JsonObject>();
  committer["name"] = "github-actions[bot]";
  committer["email"] = "41898282+github-actions[bot]@users.noreply.github.com";
  JsonObject author = putDoc["author"].to<JsonObject>();
  author["name"] = "github-actions[bot]";
  author["email"] = "41898282+github-actions[bot]@users.noreply.github.com";
  putDoc["content"] = encodedOutput;
  putDoc["sha"] = lvglFileSha;
  putDoc["branch"] = GITHUB_BRANCH;

  String putPayload;
  serializeJson(putDoc, putPayload);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(6);

  HTTPClient http;
  String cleanRepo = GITHUB_REPO;
  cleanRepo.trim();
  cleanRepo.replace(" ", "-");

  String encodedPath = urlEncode(GITHUB_FILE_PATH);
  String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + cleanRepo + 
               "/contents/" + encodedPath;

  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-LVGL-Display");
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + GITHUB_TOKEN);

  int httpCode = http.PUT(putPayload);
  bool success = false;

  if (httpCode == HTTP_CODE_OK || httpCode == 201) {
    String resp = http.getString();
    #if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument resDoc;
    #else
      DynamicJsonDocument resDoc(4096);
    #endif
    deserializeJson(resDoc, resp);
    lvglFileSha = resDoc["content"]["sha"].as<String>();
    success = true;
    Serial.println("[LVGL-HEARTBEAT] ส่งรายงานสถานะจอภาพขึ้น GitHub สำเร็จ (หน้าเว็บขึ้นออนไลน์พร้อมใช้งาน)");
  } else {
    Serial.printf("[LVGL-HEARTBEAT] ส่งรายงานไม่สำเร็จ HTTP Code: %d\n", httpCode);
    if (httpCode == 409) lvglFileSha = "";
  }

  http.end();
  return success;
}

// -------------------------------------------------------------
// 5. ลูปหลักสำหรับเรียกใช้ใน loop() (Core 0 Background Worker)
// -------------------------------------------------------------
inline void handleGitHubSync() {
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();

  // 1. ตรวจสอบว่ามีคำสั่งจากการสัมผัสหน้าจอรอส่งหรือไม่ (Highest Priority Preemption)
  if (hasPendingCloudCommand) {
    // Debounce สั้นๆ 40ms เพื่อรวบคำสั่งหากสัมผัสปุ่มติดๆ กัน
    if (now - pendingCommandRequestTime >= 40) {
      hasPendingCloudCommand = false;
      String action = pendingActionName;
      pushCurrentStateToCloud(action);
      lastCloudPollTime = now;       // เลื่อนเวลา Poll ป้องกันการชนกัน
      lastCloudHeartbeatTime = now;  // เลื่อนเวลา Heartbeat ป้องกันการชนกัน
      return;
    }
    return; // ระงับ Poll/Heartbeat ชั่วคราวระหว่างรอส่งคำสั่งสัมผัส
  }

  // 2. ดึงคำสั่งจากคลาวด์ตามรอบเวลา
  if (now - lastCloudPollTime >= CLOUD_POLL_INTERVAL_MS) {
    lastCloudPollTime = now;
    fetchCloudCommands();
  }

  // 3. ส่ง Heartbeat จอภาพทุก 30 วินาที
  if (now - lastCloudHeartbeatTime >= CLOUD_HEARTBEAT_INTERVAL_MS) {
    lastCloudHeartbeatTime = now;
    sendDisplayHeartbeat();
  }
}
