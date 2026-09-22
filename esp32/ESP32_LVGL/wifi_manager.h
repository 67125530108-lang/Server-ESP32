#pragma once
#include "config.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

// บันทึก Wi-Fi ลง Flash
inline void save_wifi(const char* ssid, const char* pass) {
  Preferences prefs;
  prefs.begin("wifi_cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

// ล้างค่า Wi-Fi ที่บันทึกไว้
inline void clear_saved_wifi() {
  Preferences prefs;
  prefs.begin("wifi_cfg", false);
  prefs.clear();
  prefs.end();
}

// ซิงค์เวลาจาก NTP Server
inline void sync_ntp_time() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("[NTP] Syncing time with NTP...");
}

// เริ่มต้นสแกนหาเครือข่าย Wi-Fi
inline void start_wifi_scan() {
  if (wifi_list != nullptr) {
    lv_obj_clean(wifi_list);
    lv_obj_t *loading_btn = lv_list_add_button(wifi_list, LV_SYMBOL_REFRESH, "Scanning Wi-Fi networks...");
    lv_obj_remove_flag(loading_btn, LV_OBJ_FLAG_CLICKABLE);
  }
  WiFi.scanNetworks(true); // สแกนแบบ Asynchronous ไม่ทำให้จอค้าง
}

// ตัดการเชื่อมต่อ Wi-Fi
inline void disconnect_wifi() {
  WiFi.disconnect(true);
  is_connected = false;
  is_connecting = false;
  if (wifi_status_label) lv_label_set_text(wifi_status_label, "Select a Wi-Fi network to connect:");
  if (status_label) lv_label_set_text(status_label, "Wi-Fi: Disconnected");
  if (current_conn_lbl) {
    lv_label_set_text(current_conn_lbl, LV_SYMBOL_WARNING " Status: Disconnected");
    lv_obj_set_style_text_color(current_conn_lbl, lv_color_hex(0x94A3B8), 0);
  }
  if (wifi_btn_disc) lv_obj_add_flag(wifi_btn_disc, LV_OBJ_FLAG_HIDDEN);
  if (cloud_status_badge) {
    lv_label_set_text(cloud_status_badge, LV_SYMBOL_WARNING " Offline");
    lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0xEF4444), 0);
  }
}

// ดึงค่า Wi-Fi ที่เคยบันทึกไว้มาเชื่อมต่อ
inline void load_saved_wifi() {
  Preferences prefs;
  prefs.begin("wifi_cfg", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() == 0) {
    // หากยังไม่เคยตั้งค่าผ่านหน้าจอ ให้ใช้ค่าเริ่มต้นของระบบอัตโนมัติ
    ssid = "Romchale_2.4GHz";
    pass = "B5224938";
  }

  if (ssid.length() > 0) {
    strncpy(selected_ssid, ssid.c_str(), sizeof(selected_ssid));
    WiFi.begin(ssid.c_str(), pass.c_str());
    is_connecting = true;
    connect_start_time = millis();
    Serial.printf("[WIFI] Auto-connecting to SSID: %s\n", ssid.c_str());
  }
}

// Timer อัปเดตสถานะการเชื่อมต่อ Wi-Fi ทุก 500ms
inline void wifi_manager_timer_cb(lv_timer_t *timer) {
  if (is_connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      is_connecting = false;
      is_connected = true;
      if (wifi_status_label) lv_label_set_text(wifi_status_label, "Available Networks:");
      if (status_label) lv_label_set_text(status_label, "Wi-Fi: Connected");
      if (current_conn_lbl) {
        lv_label_set_text_fmt(current_conn_lbl, LV_SYMBOL_WIFI " Connected: %s", WiFi.SSID().c_str());
        lv_obj_set_style_text_color(current_conn_lbl, lv_color_hex(0x00E5FF), 0);
      }
      if (wifi_btn_disc) lv_obj_remove_flag(wifi_btn_disc, LV_OBJ_FLAG_HIDDEN);
      if (cloud_status_badge) {
        lv_label_set_text(cloud_status_badge, LV_SYMBOL_WIFI " Online");
        lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0x10B981), 0);
      }
      sync_ntp_time();
    } else if (millis() - connect_start_time > 15000) {
      is_connecting = false;
      is_connected = false;
      if (wifi_status_label) lv_label_set_text(wifi_status_label, "Connection Failed! Tap to retry:");
      if (status_label) lv_label_set_text(status_label, "Wi-Fi: Not Connected");
      if (current_conn_lbl) {
        lv_label_set_text(current_conn_lbl, LV_SYMBOL_WARNING " Connection Failed");
        lv_obj_set_style_text_color(current_conn_lbl, lv_color_hex(0xEF4444), 0);
      }
      if (wifi_btn_disc) lv_obj_add_flag(wifi_btn_disc, LV_OBJ_FLAG_HIDDEN);
      if (cloud_status_badge) {
        lv_label_set_text(cloud_status_badge, LV_SYMBOL_WARNING " Offline");
        lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0xEF4444), 0);
      }
    }
  }

  // แสดงผลลัพธ์การสแกน Wi-Fi
  int n = WiFi.scanComplete();
  if (n >= 0) {
    if (wifi_list != nullptr) {
      lv_obj_clean(wifi_list);
      if (n == 0) {
        lv_obj_t *btn = lv_list_add_button(wifi_list, NULL, "No networks found.");
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_CLICKABLE);
      } else {
        for (int i = 0; i < n; ++i) {
          String item_text = WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
          lv_obj_t *btn = lv_list_add_button(wifi_list, LV_SYMBOL_WIFI, item_text.c_str());
          extern void wifi_list_cb(lv_event_t *e);
          lv_obj_add_event_cb(btn, wifi_list_cb, LV_EVENT_CLICKED, NULL);
        }
      }
    }
    WiFi.scanDelete();
  }
}

// Timer อัปเดตเวลานาฬิกาดิจิทัลแบบ Live
inline void live_clock_timer_cb(lv_timer_t *timer) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year > (2020 - 1900)) {
    char time_str[16];
    char date_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y", &timeinfo);

    if (clock_label) lv_label_set_text(clock_label, time_str);
    if (date_label) lv_label_set_text(date_label, date_str);
    if (weather_clock_label) lv_label_set_text(weather_clock_label, time_str);
    if (weather_date_label) lv_label_set_text(weather_date_label, date_str);
  }
}
