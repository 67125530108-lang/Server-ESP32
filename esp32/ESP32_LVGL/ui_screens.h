#pragma once
#include "config.h"
#include "ui_icons.h"
#include "wifi_manager.h"
#include "display_touch.h"

// ประกาศฟังก์ชันสร้างหน้าจอทั้งหมด
void build_main_screen();
void build_wifi_screen();
void build_menu_screen();
void build_weather_screen();
void build_settings_screen();
void build_cloud_hub_screen();

// -------------------------------------------------------------
// Callbacks การนำทางเปลี่ยนหน้าจอ
// -------------------------------------------------------------
inline void nav_to_wifi_cb(lv_event_t *e) {
  if (!wifi_scr) build_wifi_screen();
  lv_screen_load(wifi_scr);
  start_wifi_scan();
}

inline void nav_to_menu_cb(lv_event_t *e) {
  if (!menu_scr) build_menu_screen();
  lv_screen_load(menu_scr);
}

inline void nav_to_main_cb(lv_event_t *e) {
  if (pwd_modal) lv_obj_add_flag(pwd_modal, LV_OBJ_FLAG_HIDDEN);
  if (kb) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  if (!main_scr) build_main_screen();
  lv_screen_load(main_scr);
}

inline void nav_to_weather_cb(lv_event_t *e) {
  if (!weather_scr) build_weather_screen();
  lv_screen_load(weather_scr);
}

inline void nav_to_settings_cb(lv_event_t *e) {
  if (!settings_scr) build_settings_screen();
  lv_screen_load(settings_scr);
}

inline void nav_to_cloud_hub_cb(lv_event_t *e) {
  if (!cloud_hub_scr) build_cloud_hub_screen();
  lv_screen_load(cloud_hub_scr);
}

// -------------------------------------------------------------
// Callbacks หน้า Wi-Fi (Pop-up คีย์บอร์ดเฉพาะตอนเลือก SSID)
// -------------------------------------------------------------
inline void wifi_list_cb(lv_event_t *e) {
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  const char *txt = lv_list_get_button_text(wifi_list, btn);
  if (txt) {
    char clean_ssid[64];
    strncpy(clean_ssid, txt, sizeof(clean_ssid) - 1);
    clean_ssid[sizeof(clean_ssid) - 1] = '\0';
    char *paren = strchr(clean_ssid, '(');
    if (paren && paren > clean_ssid) {
      *(paren - 1) = '\0';
    }
    strncpy(selected_ssid, clean_ssid, sizeof(selected_ssid) - 1);
    selected_ssid[sizeof(selected_ssid) - 1] = '\0';

    if (pwd_title_lbl) lv_label_set_text_fmt(pwd_title_lbl, "Connect to: %s", selected_ssid);
    if (ta_pass) lv_textarea_set_text(ta_pass, "");

    // แสดง Pop-up กรอกรหัส และเปิดแป้นพิมพ์ขึ้นมาเฉพาะตอนแตะเลือก Wi-Fi
    if (pwd_modal) {
      lv_obj_remove_flag(pwd_modal, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(pwd_modal);
    }
    if (kb) {
      lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(kb);
    }
  }
}

inline void btn_pwd_connect_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED || code == LV_EVENT_READY) {
    if (strlen(selected_ssid) == 0) return;
    const char *pass = ta_pass ? lv_textarea_get_text(ta_pass) : "";
    if (wifi_status_label) lv_label_set_text_fmt(wifi_status_label, "Connecting to %s...", selected_ssid);
    save_wifi(selected_ssid, pass);
    WiFi.disconnect();
    WiFi.begin(selected_ssid, pass);
    is_connecting = true;
    connect_start_time = millis();

    if (pwd_modal) lv_obj_add_flag(pwd_modal, LV_OBJ_FLAG_HIDDEN);
    if (kb) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }
}

inline void btn_pwd_cancel_cb(lv_event_t *e) {
  if (pwd_modal) lv_obj_add_flag(pwd_modal, LV_OBJ_FLAG_HIDDEN);
  if (kb) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

inline void rescan_btn_cb(lv_event_t *e) {
  start_wifi_scan();
}

inline void disconnect_btn_cb(lv_event_t *e) {
  disconnect_wifi();
}

// -------------------------------------------------------------
// Callbacks หน้า Settings
// -------------------------------------------------------------
inline void brightness_slider_cb(lv_event_t *e) {
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  int32_t val = lv_slider_get_value(slider);
  set_screen_brightness((uint8_t)val, true);
  if (brightness_val_lbl) {
    lv_label_set_text_fmt(brightness_val_lbl, "%d%%", (int)val);
  }
}

inline void recal_btn_cb(lv_event_t *e) {
  run_touch_calibration_flow();
  lv_screen_load(main_scr);
}

inline void reboot_btn_cb(lv_event_t *e) {
  Serial.println("[SYSTEM] Reboot requested from Settings...");
  ESP.restart();
}

// -------------------------------------------------------------
// Helper อัปเดตสไตล์การ์ด Relay แบบไดนามิก (Glow & Neon Border)
// -------------------------------------------------------------
inline void update_relay_card_style(int ch) {
  if (ch < 0 || ch >= 4) return;
  bool is_on = relay_states[ch];
  if (card_relay[ch]) {
    if (is_on) {
      lv_obj_set_style_bg_color(card_relay[ch], lv_color_hex(0x0C243B), 0);
      lv_obj_set_style_border_color(card_relay[ch], lv_color_hex(0x00E5FF), 0);
      lv_obj_set_style_border_width(card_relay[ch], 2, 0);
      lv_obj_set_style_shadow_color(card_relay[ch], lv_color_hex(0x00E5FF), 0);
      lv_obj_set_style_shadow_width(card_relay[ch], 14, 0);
      lv_obj_set_style_shadow_spread(card_relay[ch], 1, 0);
    } else {
      lv_obj_set_style_bg_color(card_relay[ch], lv_color_hex(0x111726), 0);
      lv_obj_set_style_border_color(card_relay[ch], lv_color_hex(0x1E293B), 0);
      lv_obj_set_style_border_width(card_relay[ch], 1, 0);
      lv_obj_set_style_shadow_width(card_relay[ch], 0, 0);
    }
  }
  if (icon_relay_box[ch]) {
    lv_obj_set_style_bg_color(icon_relay_box[ch], is_on ? lv_color_hex(0x00E5FF) : lv_color_hex(0x1E293B), 0);
  }
  if (icon_relay[ch]) {
    lv_obj_set_style_text_color(icon_relay[ch], is_on ? lv_color_hex(0x000000) : lv_color_hex(0x64748B), 0);
  }
  if (badge_relay[ch]) {
    lv_label_set_text(badge_relay[ch], is_on ? LV_SYMBOL_OK " ACTIVE" : LV_SYMBOL_MINUS " STANDBY");
    lv_obj_set_style_text_color(badge_relay[ch], is_on ? lv_color_hex(0x00E5FF) : lv_color_hex(0x64748B), 0);
  }
  if (sw_relay[ch]) {
    if (is_on) lv_obj_add_state(sw_relay[ch], LV_STATE_CHECKED);
    else lv_obj_remove_state(sw_relay[ch], LV_STATE_CHECKED);
  }
}

// Helper อัปเดตสไตล์การ์ด Built-in LED (Amber Glow)
inline void update_led_card_style() {
  if (led_card) {
    if (led_state) {
      lv_obj_set_style_bg_color(led_card, lv_color_hex(0x281F08), 0);
      lv_obj_set_style_border_color(led_card, lv_color_hex(0xF59E0B), 0);
      lv_obj_set_style_border_width(led_card, 2, 0);
      lv_obj_set_style_shadow_color(led_card, lv_color_hex(0xF59E0B), 0);
      lv_obj_set_style_shadow_width(led_card, 10, 0);
    } else {
      lv_obj_set_style_bg_color(led_card, lv_color_hex(0x111726), 0);
      lv_obj_set_style_border_color(led_card, lv_color_hex(0x1E293B), 0);
      lv_obj_set_style_border_width(led_card, 1, 0);
      lv_obj_set_style_shadow_width(led_card, 0, 0);
    }
  }
  if (led_icon_box) {
    lv_obj_set_style_bg_color(led_icon_box, led_state ? lv_color_hex(0xF59E0B) : lv_color_hex(0x1E293B), 0);
  }
  if (led_icon) {
    lv_obj_set_style_text_color(led_icon, led_state ? lv_color_hex(0x000000) : lv_color_hex(0x64748B), 0);
  }
  if (led_badge) {
    lv_label_set_text(led_badge, led_state ? "ACTIVE" : "STANDBY");
    lv_obj_set_style_text_color(led_badge, led_state ? lv_color_hex(0xF59E0B) : lv_color_hex(0x64748B), 0);
  }
  if (sw_led) {
    if (led_state) lv_obj_add_state(sw_led, LV_STATE_CHECKED);
    else lv_obj_remove_state(sw_led, LV_STATE_CHECKED);
  }
}

#include "github_sync.h"

// -------------------------------------------------------------
// Callbacks หน้า ESP32 Cloud Hub (4-CH Relay & LED)
// -------------------------------------------------------------
inline void cloud_relay_toggle_cb(lv_event_t *e) {
  if (is_syncing_from_cloud) return;
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
  int ch = (int)(intptr_t)lv_event_get_user_data(e);
  bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
  if (ch >= 0 && ch < 4) {
    relay_states[ch] = is_on;
    int pin = (ch == 0) ? PIN_RELAY_1 : (ch == 1) ? PIN_RELAY_2 : (ch == 2) ? PIN_RELAY_3 : PIN_RELAY_4;
    digitalWrite(pin, (RELAY_ACTIVE_LOW ? !is_on : is_on));
    update_relay_card_style(ch);
    Serial.printf("[CLOUD HUB] Relay %d switch: %s (Pin %d)\n", ch + 1, is_on ? "ON" : "OFF", pin);
    sendCloudCommand(ch, is_on);
  }
}

inline void cloud_relay_card_cb(lv_event_t *e) {
  if (is_syncing_from_cloud) return;
  lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
  int ch = (int)(intptr_t)lv_event_get_user_data(e);
  if (ch >= 0 && ch < 4) {
    if (target == sw_relay[ch]) return; // ถ้าแตะที่ตัวสวิตช์โดยตรง ปล่อยให้ switch event ทำงาน
    bool new_state = !relay_states[ch];
    relay_states[ch] = new_state;
    int pin = (ch == 0) ? PIN_RELAY_1 : (ch == 1) ? PIN_RELAY_2 : (ch == 2) ? PIN_RELAY_3 : PIN_RELAY_4;
    digitalWrite(pin, (RELAY_ACTIVE_LOW ? !new_state : new_state));
    
    // ป้องกันไม่ให้การเปลี่ยนสถานะของสวิตช์ส่ง event ซ้ำซ้อน
    is_syncing_from_cloud = true;
    update_relay_card_style(ch);
    is_syncing_from_cloud = false;

    Serial.printf("[CLOUD HUB] Card %d tapped: %s (Pin %d)\n", ch + 1, new_state ? "ON" : "OFF", pin);
    sendCloudCommand(ch, new_state);
  }
}

inline void cloud_led_toggle_cb(lv_event_t *e) {
  if (is_syncing_from_cloud) return;
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
  led_state = lv_obj_has_state(sw, LV_STATE_CHECKED);
  digitalWrite(PIN_LED_BOARD, led_state ? HIGH : LOW);
  update_led_card_style();
  Serial.printf("[CLOUD HUB] Built-in LED switch: %s\n", led_state ? "ON" : "OFF");
  sendCloudLedCommand(led_state);
}

inline void cloud_led_card_cb(lv_event_t *e) {
  if (is_syncing_from_cloud) return;
  lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
  if (target == sw_led) return;
  led_state = !led_state;
  digitalWrite(PIN_LED_BOARD, led_state ? HIGH : LOW);
  
  is_syncing_from_cloud = true;
  update_led_card_style();
  is_syncing_from_cloud = false;

  Serial.printf("[CLOUD HUB] LED card tapped: %s\n", led_state ? "ON" : "OFF");
  sendCloudLedCommand(led_state);
}

inline void cloud_all_relays_cb(lv_event_t *e) {
  if (is_syncing_from_cloud) return;
  bool target_state = (bool)(intptr_t)lv_event_get_user_data(e);
  for (int i = 0; i < 4; i++) {
    relay_states[i] = target_state;
    int pin = (i == 0) ? PIN_RELAY_1 : (i == 1) ? PIN_RELAY_2 : (i == 2) ? PIN_RELAY_3 : PIN_RELAY_4;
    digitalWrite(pin, (RELAY_ACTIVE_LOW ? !target_state : target_state));
  }

  is_syncing_from_cloud = true;
  for (int i = 0; i < 4; i++) {
    update_relay_card_style(i);
  }
  is_syncing_from_cloud = false;

  Serial.printf("[CLOUD HUB] All relays set to: %s\n", target_state ? "ALL ON" : "ALL OFF");
  sendAllRelaysCloudCommand(target_state);
}

// -------------------------------------------------------------
// หน้าที่ 1: Main Screen (มีปุ่มรูปฟันเฟืองมุมบนขวา)
// -------------------------------------------------------------
inline void build_main_screen() {
  if (main_scr) lv_obj_del(main_scr);
  main_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(main_scr, lv_color_hex(0x0B0F19), 0);

  // การ์ดหลักตรงกลาง
  lv_obj_t *card = lv_obj_create(main_scr);
  lv_obj_set_size(card, 440, 280);
  lv_obj_center(card);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_shadow_color(card, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_shadow_width(card, 20, 0);
  lv_obj_set_style_shadow_spread(card, 2, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // สถานะ LED ดวงไฟเรืองแสง
  lv_obj_t *led = create_glowing_led(card, lv_color_hex(0x00E5FF), 10);
  lv_obj_align(led, LV_ALIGN_TOP_LEFT, 15, 15);

  lv_obj_t *title = lv_label_create(card);
  lv_label_set_text(title, "ESP32 SMART SYSTEM");
  lv_obj_set_style_text_font(title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), 0);
  lv_obj_align_to(title, led, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  // ปุ่มฟันเฟือง ⚙️ (Settings Button) ที่มุมบนขวา
  lv_obj_t *btn_gear = lv_button_create(card);
  lv_obj_set_size(btn_gear, 40, 36);
  lv_obj_align(btn_gear, LV_ALIGN_TOP_RIGHT, -10, 5);
  lv_obj_set_style_bg_color(btn_gear, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_gear, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_gear, 1, 0);
  lv_obj_set_style_radius(btn_gear, 8, 0);
  lv_obj_add_event_cb(btn_gear, nav_to_settings_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lbl_gear = lv_label_create(btn_gear);
  lv_label_set_text(lbl_gear, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_color(lbl_gear, lv_color_hex(0x00E5FF), 0);
  lv_obj_center(lbl_gear);

  // นาฬิกาดิจิทัลขนาดใหญ่
  clock_label = lv_label_create(card);
  lv_label_set_text(clock_label, "12:00:00");
  lv_obj_set_style_text_font(clock_label, FONT_CLOCK, 0);
  lv_obj_set_style_text_color(clock_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, -35);

  date_label = lv_label_create(card);
  lv_label_set_text(date_label, "Sun, 01 Jan 2026");
  lv_obj_set_style_text_font(date_label, FONT_NORMAL, 0);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0x8A99AD), 0);
  lv_obj_align_to(date_label, clock_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  status_label = lv_label_create(card);
  lv_label_set_text(status_label, is_connected ? "Wi-Fi: Connected" : "Wi-Fi: Disconnected");
  lv_obj_set_style_text_font(status_label, FONT_SMALL, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x00E5FF), 0);
  lv_obj_align_to(status_label, date_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // ปุ่มกดด้านล่าง 2 ปุ่ม จัดระยะสมดุลสวยงาม (ไม่มีปุ่ม Cal มาเกะกะ)
  lv_obj_t *btn_wifi = lv_button_create(card);
  lv_obj_set_size(btn_wifi, 170, 44);
  lv_obj_align(btn_wifi, LV_ALIGN_BOTTOM_LEFT, 25, -15);
  lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_wifi, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_wifi, 1, 0);
  lv_obj_set_style_radius(btn_wifi, 10, 0);
  lv_obj_add_event_cb(btn_wifi, nav_to_wifi_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_w = lv_label_create(btn_wifi);
  lv_label_set_text(lbl_w, LV_SYMBOL_WIFI " Wi-Fi Setup");
  lv_obj_center(lbl_w);

  lv_obj_t *btn_menu = lv_button_create(card);
  lv_obj_set_size(btn_menu, 170, 44);
  lv_obj_align(btn_menu, LV_ALIGN_BOTTOM_RIGHT, -25, -15);
  lv_obj_set_style_bg_color(btn_menu, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_radius(btn_menu, 10, 0);
  lv_obj_add_event_cb(btn_menu, nav_to_menu_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_m = lv_label_create(btn_menu);
  lv_label_set_text(lbl_m, "Apps Hub " LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(lbl_m, lv_color_hex(0x000000), 0);
  lv_obj_center(lbl_m);
}

// -------------------------------------------------------------
// หน้าที่ 2: Wi-Fi Setup Screen (แก้ปัญหาข้อความซ้อนทับกันเด็ดขาด)
// -------------------------------------------------------------
inline void build_wifi_screen() {
  if (wifi_scr) lv_obj_del(wifi_scr);
  wifi_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(wifi_scr, lv_color_hex(0x0B0F19), 0);

  // 1. Top Bar
  lv_obj_t *btn_back = lv_button_create(wifi_scr);
  lv_obj_set_size(btn_back, 80, 36);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 10);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_back, 1, 0);
  lv_obj_add_event_cb(btn_back, nav_to_main_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_b = lv_label_create(btn_back);
  lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " Back");
  lv_obj_center(lbl_b);

  lv_obj_t *title = lv_label_create(wifi_scr);
  lv_label_set_text(title, "Wi-Fi Configuration");
  lv_obj_set_style_text_font(title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  lv_obj_t *btn_refresh = lv_button_create(wifi_scr);
  lv_obj_set_size(btn_refresh, 85, 36);
  lv_obj_align(btn_refresh, LV_ALIGN_TOP_RIGHT, -15, 10);
  lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x00E5FF), 0);
  lv_obj_add_event_cb(btn_refresh, rescan_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_ref = lv_label_create(btn_refresh);
  lv_label_set_text(lbl_ref, LV_SYMBOL_REFRESH " Scan");
  lv_obj_set_style_text_color(lbl_ref, lv_color_hex(0x000000), 0);
  lv_obj_center(lbl_ref);

  // 2. แถบแสดงสถานะเชื่อมต่อปัจจุบัน (จัดตำแหน่งคงที่ y = 50 ไม่ซ้อน ไม่เลื่อน)
  current_conn_panel = lv_obj_create(wifi_scr);
  lv_obj_set_size(current_conn_panel, 450, 42);
  lv_obj_align(current_conn_panel, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_set_style_bg_color(current_conn_panel, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(current_conn_panel, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(current_conn_panel, 1, 0);
  lv_obj_set_style_radius(current_conn_panel, 8, 0);
  lv_obj_set_style_pad_all(current_conn_panel, 4, 0);
  lv_obj_remove_flag(current_conn_panel, LV_OBJ_FLAG_SCROLLABLE);

  // ป้องกันข้อความยาวเกินไปชนปุ่ม Disconnect ด้วยการตัดด้วยจุดไข่ปลา (...)
  current_conn_lbl = lv_label_create(current_conn_panel);
  lv_obj_set_width(current_conn_lbl, 290);
  lv_label_set_long_mode(current_conn_lbl, LV_LABEL_LONG_DOT);
  if (is_connected) {
    lv_label_set_text_fmt(current_conn_lbl, LV_SYMBOL_WIFI " Connected: %s", WiFi.SSID().c_str());
    lv_obj_set_style_text_color(current_conn_lbl, lv_color_hex(0x00E5FF), 0);
  } else {
    lv_label_set_text(current_conn_lbl, LV_SYMBOL_WARNING " Status: Disconnected");
    lv_obj_set_style_text_color(current_conn_lbl, lv_color_hex(0x94A3B8), 0);
  }
  lv_obj_set_style_text_font(current_conn_lbl, FONT_SMALL, 0);
  lv_obj_align(current_conn_lbl, LV_ALIGN_LEFT_MID, 8, 0);

  wifi_btn_disc = lv_button_create(current_conn_panel);
  lv_obj_set_size(wifi_btn_disc, 105, 30);
  lv_obj_align(wifi_btn_disc, LV_ALIGN_RIGHT_MID, -4, 0);
  lv_obj_set_style_bg_color(wifi_btn_disc, lv_color_hex(0xEF4444), 0);
  lv_obj_add_event_cb(wifi_btn_disc, disconnect_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_disc = lv_label_create(wifi_btn_disc);
  lv_label_set_text(lbl_disc, "Disconnect");
  lv_obj_center(lbl_disc);

  if (!is_connected) {
    lv_obj_add_flag(wifi_btn_disc, LV_OBJ_FLAG_HIDDEN);
  }

  // ข้อความสถานะด้านล่างกรอบเชื่อมต่อ (จัดตำแหน่งคงที่ y = 98 แยกขาดจากแถบด้านบน)
  wifi_status_label = lv_label_create(wifi_scr);
  lv_label_set_text(wifi_status_label, is_connected ? "Available Networks:" : "Select a Wi-Fi network to connect:");
  lv_obj_set_style_text_font(wifi_status_label, FONT_SMALL, 0);
  lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x8A99AD), 0);
  lv_obj_align(wifi_status_label, LV_ALIGN_TOP_LEFT, 15, 98);

  // 3. รายชื่อ Wi-Fi กว้างเต็มหน้าจอ (จัดตำแหน่งคงที่ y = 122 ความสูง 188)
  wifi_list = lv_list_create(wifi_scr);
  lv_obj_set_size(wifi_list, 450, 188);
  lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 122);
  lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(wifi_list, lv_color_hex(0x334155), 0);
  lv_obj_set_style_border_width(wifi_list, 1, 0);
  lv_obj_set_style_radius(wifi_list, 10, 0);

  // 4. Pop-up กรอกรหัสผ่าน (ซ่อนไว้ก่อน)
  pwd_modal = lv_obj_create(wifi_scr);
  lv_obj_set_size(pwd_modal, 440, 150);
  lv_obj_align(pwd_modal, LV_ALIGN_TOP_MID, 0, 15);
  lv_obj_set_style_bg_color(pwd_modal, lv_color_hex(0x0F172A), 0);
  lv_obj_set_style_border_color(pwd_modal, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(pwd_modal, 2, 0);
  lv_obj_set_style_radius(pwd_modal, 12, 0);
  lv_obj_set_style_shadow_color(pwd_modal, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_shadow_width(pwd_modal, 20, 0);
  lv_obj_remove_flag(pwd_modal, LV_OBJ_FLAG_SCROLLABLE);

  pwd_title_lbl = lv_label_create(pwd_modal);
  lv_label_set_text(pwd_title_lbl, "Connect to: Wi-Fi");
  lv_obj_set_style_text_font(pwd_title_lbl, FONT_TITLE, 0);
  lv_obj_set_style_text_color(pwd_title_lbl, lv_color_hex(0x00E5FF), 0);
  lv_obj_align(pwd_title_lbl, LV_ALIGN_TOP_MID, 0, 0);

  ta_pass = lv_textarea_create(pwd_modal);
  lv_obj_set_size(ta_pass, 380, 42);
  lv_obj_align(ta_pass, LV_ALIGN_CENTER, 0, -5);
  lv_textarea_set_placeholder_text(ta_pass, "Enter Password...");
  lv_textarea_set_password_mode(ta_pass, true);
  lv_textarea_set_one_line(ta_pass, true);
  lv_obj_set_style_bg_color(ta_pass, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_text_color(ta_pass, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_color(ta_pass, lv_color_hex(0x00E5FF), 0);

  btn_connect = lv_button_create(pwd_modal);
  lv_obj_set_size(btn_connect, 130, 36);
  lv_obj_align(btn_connect, LV_ALIGN_BOTTOM_RIGHT, -15, 0);
  lv_obj_set_style_bg_color(btn_connect, lv_color_hex(0x00E5FF), 0);
  lv_obj_add_event_cb(btn_connect, btn_pwd_connect_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_conn = lv_label_create(btn_connect);
  lv_label_set_text(lbl_conn, "Connect");
  lv_obj_set_style_text_color(lbl_conn, lv_color_hex(0x000000), 0);
  lv_obj_center(lbl_conn);

  lv_obj_t *btn_cancel = lv_button_create(pwd_modal);
  lv_obj_set_size(btn_cancel, 110, 36);
  lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 15, 0);
  lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x334155), 0);
  lv_obj_add_event_cb(btn_cancel, btn_pwd_cancel_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_can = lv_label_create(btn_cancel);
  lv_label_set_text(lbl_can, "Cancel");
  lv_obj_center(lbl_can);

  // 5. แป้นพิมพ์เสมือน Keyboard (ซ่อนไว้ก่อน)
  kb = lv_keyboard_create(wifi_scr);
  lv_obj_set_size(kb, 480, 140);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(kb, ta_pass);
  lv_obj_add_event_cb(kb, btn_pwd_connect_cb, LV_EVENT_READY, NULL);

  lv_obj_add_flag(pwd_modal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

// -------------------------------------------------------------
// หน้าที่ 3: Settings Screen (ปรับแสงสว่าง, ข้อมูลระบบ, Calibrate)
// -------------------------------------------------------------
inline void build_settings_screen() {
  if (settings_scr) lv_obj_del(settings_scr);
  settings_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(settings_scr, lv_color_hex(0x0B0F19), 0);

  // Top Bar
  lv_obj_t *btn_back = lv_button_create(settings_scr);
  lv_obj_set_size(btn_back, 80, 36);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 10);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_back, 1, 0);
  lv_obj_add_event_cb(btn_back, nav_to_main_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_b = lv_label_create(btn_back);
  lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " Home");
  lv_obj_center(lbl_b);

  lv_obj_t *title = lv_label_create(settings_scr);
  lv_label_set_text(title, "SYSTEM SETTINGS");
  lv_obj_set_style_text_font(title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  // กล่องที่ 1: แถบเลื่อนปรับแสงสว่างหน้าจอ (Brightness Slider)
  lv_obj_t *card_bright = lv_obj_create(settings_scr);
  lv_obj_set_size(card_bright, 450, 85);
  lv_obj_align(card_bright, LV_ALIGN_TOP_MID, 0, 55);
  lv_obj_set_style_bg_color(card_bright, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(card_bright, lv_color_hex(0x334155), 0);
  lv_obj_set_style_border_width(card_bright, 1, 0);
  lv_obj_set_style_radius(card_bright, 10, 0);
  lv_obj_remove_flag(card_bright, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_br_title = lv_label_create(card_bright);
  lv_label_set_text(lbl_br_title, LV_SYMBOL_EYE_OPEN " Screen Brightness");
  lv_obj_set_style_text_color(lbl_br_title, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_text_font(lbl_br_title, FONT_TITLE, 0);
  lv_obj_align(lbl_br_title, LV_ALIGN_TOP_LEFT, 5, 2);

  brightness_val_lbl = lv_label_create(card_bright);
  lv_label_set_text_fmt(brightness_val_lbl, "%d%%", screen_brightness);
  lv_obj_set_style_text_color(brightness_val_lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(brightness_val_lbl, FONT_TITLE, 0);
  lv_obj_align(brightness_val_lbl, LV_ALIGN_TOP_RIGHT, -5, 2);

  brightness_slider = lv_slider_create(card_bright);
  lv_slider_set_range(brightness_slider, 10, 100);
  lv_slider_set_value(brightness_slider, screen_brightness, LV_ANIM_OFF);
  lv_obj_set_size(brightness_slider, 410, 12);
  lv_obj_align(brightness_slider, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_add_event_cb(brightness_slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // กล่องที่ 2: ข้อมูลระบบ (System Info)
  lv_obj_t *card_info = lv_obj_create(settings_scr);
  lv_obj_set_size(card_info, 450, 95);
  lv_obj_align(card_info, LV_ALIGN_TOP_MID, 0, 150);
  lv_obj_set_style_bg_color(card_info, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(card_info, lv_color_hex(0x334155), 0);
  lv_obj_set_style_border_width(card_info, 1, 0);
  lv_obj_set_style_radius(card_info, 10, 0);
  lv_obj_remove_flag(card_info, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl_inf_title = lv_label_create(card_info);
  lv_label_set_text(lbl_inf_title, "DEVICE TELEMETRY");
  lv_obj_set_style_text_color(lbl_inf_title, lv_color_hex(0x8A99AD), 0);
  lv_obj_set_style_text_font(lbl_inf_title, FONT_SMALL, 0);
  lv_obj_align(lbl_inf_title, LV_ALIGN_TOP_LEFT, 5, 2);

  char ip_buf[48];
  snprintf(ip_buf, sizeof(ip_buf), "IP Address: %s", WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "Not Connected");
  lv_obj_t *lbl_ip = lv_label_create(card_info);
  lv_label_set_text(lbl_ip, ip_buf);
  lv_obj_set_style_text_color(lbl_ip, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl_ip, FONT_SMALL, 0);
  lv_obj_align(lbl_ip, LV_ALIGN_TOP_LEFT, 5, 25);

  char mac_buf[48];
  snprintf(mac_buf, sizeof(mac_buf), "MAC: %s | Free RAM: %d KB", WiFi.macAddress().c_str(), (int)(ESP.getFreeHeap() / 1024));
  lv_obj_t *lbl_mac = lv_label_create(card_info);
  lv_label_set_text(lbl_mac, mac_buf);
  lv_obj_set_style_text_color(lbl_mac, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_text_font(lbl_mac, FONT_SMALL, 0);
  lv_obj_align(lbl_mac, LV_ALIGN_TOP_LEFT, 5, 48);

  // แถวล่าง: ปุ่ม Calibrate หน้าจอ และปุ่ม Reboot
  lv_obj_t *btn_cal = lv_button_create(settings_scr);
  lv_obj_set_size(btn_cal, 215, 42);
  lv_obj_align(btn_cal, LV_ALIGN_BOTTOM_LEFT, 15, -12);
  lv_obj_set_style_bg_color(btn_cal, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_cal, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_cal, 1, 0);
  lv_obj_add_event_cb(btn_cal, recal_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_cal = lv_label_create(btn_cal);
  lv_label_set_text(lbl_cal, LV_SYMBOL_EDIT " Calibrate Touch");
  lv_obj_center(lbl_cal);

  lv_obj_t *btn_reboot = lv_button_create(settings_scr);
  lv_obj_set_size(btn_reboot, 215, 42);
  lv_obj_align(btn_reboot, LV_ALIGN_BOTTOM_RIGHT, -15, -12);
  lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0xEF4444), 0);
  lv_obj_add_event_cb(btn_reboot, reboot_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_reb = lv_label_create(btn_reboot);
  lv_label_set_text(lbl_reb, LV_SYMBOL_POWER " Restart ESP32");
  lv_obj_center(lbl_reb);
}

// -------------------------------------------------------------
// หน้าที่ 4: App Launcher (การ์ดขวาเปลี่ยนเป็น ESP32 Cloud Hub)
// -------------------------------------------------------------
inline void build_menu_screen() {
  if (menu_scr) lv_obj_del(menu_scr);
  menu_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(menu_scr, lv_color_hex(0x0B0F19), 0);

  // Header Bar
  lv_obj_t *btn_back = lv_button_create(menu_scr);
  lv_obj_set_size(btn_back, 75, 36);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 15);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_back, 1, 0);
  lv_obj_add_event_cb(btn_back, nav_to_main_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_b = lv_label_create(btn_back);
  lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " Home");
  lv_obj_center(lbl_b);

  lv_obj_t *title = lv_label_create(menu_scr);
  lv_label_set_text(title, "APPLICATIONS");
  lv_obj_set_style_text_font(title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // กล่องแอปที่ 1: Weather & Clock
  lv_obj_t *card_weather = lv_obj_create(menu_scr);
  lv_obj_set_size(card_weather, 205, 230);
  lv_obj_align(card_weather, LV_ALIGN_BOTTOM_LEFT, 25, -20);
  lv_obj_set_style_bg_color(card_weather, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(card_weather, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(card_weather, 1, 0);
  lv_obj_set_style_radius(card_weather, 12, 0);
  lv_obj_add_event_cb(card_weather, nav_to_weather_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *w_icon = draw_weather_app_icon(card_weather);
  lv_obj_align(w_icon, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *lbl_w_title = lv_label_create(card_weather);
  lv_label_set_text(lbl_w_title, "Weather & Clock");
  lv_obj_set_style_text_font(lbl_w_title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(lbl_w_title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl_w_title, LV_ALIGN_CENTER, 0, 5);

  lv_obj_t *lbl_w_sub = lv_label_create(card_weather);
  lv_label_set_text(lbl_w_sub, "Kanchanaburi Smart\nNTP Forecast");
  lv_obj_set_style_text_font(lbl_w_sub, FONT_SMALL, 0);
  lv_obj_set_style_text_color(lbl_w_sub, lv_color_hex(0x8A99AD), 0);
  lv_obj_set_style_text_align(lbl_w_sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lbl_w_sub, LV_ALIGN_BOTTOM_MID, 0, -15);

  // กล่องแอปที่ 2: ESP32 Cloud Hub (เชื่อมต่อกับ server-esp-32.vercel.app)
  lv_obj_t *card_cloud = lv_obj_create(menu_scr);
  lv_obj_set_size(card_cloud, 205, 230);
  lv_obj_align(card_cloud, LV_ALIGN_BOTTOM_RIGHT, -25, -20);
  lv_obj_set_style_bg_color(card_cloud, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(card_cloud, lv_color_hex(0x38BDF8), 0);
  lv_obj_set_style_border_width(card_cloud, 1, 0);
  lv_obj_set_style_radius(card_cloud, 12, 0);
  lv_obj_add_event_cb(card_cloud, nav_to_cloud_hub_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *c_icon = draw_cloud_hub_icon(card_cloud);
  lv_obj_align(c_icon, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *lbl_c_title = lv_label_create(card_cloud);
  lv_label_set_text(lbl_c_title, "ESP32 Cloud Hub");
  lv_obj_set_style_text_font(lbl_c_title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(lbl_c_title, lv_color_hex(0x38BDF8), 0);
  lv_obj_align(lbl_c_title, LV_ALIGN_CENTER, 0, 5);

  lv_obj_t *lbl_c_sub = lv_label_create(card_cloud);
  lv_label_set_text(lbl_c_sub, "4-CH Smart Relay\nsmart-iot-th.github.io");
  lv_obj_set_style_text_font(lbl_c_sub, FONT_SMALL, 0);
  lv_obj_set_style_text_color(lbl_c_sub, lv_color_hex(0x8A99AD), 0);
  lv_obj_set_style_text_align(lbl_c_sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lbl_c_sub, LV_ALIGN_BOTTOM_MID, 0, -15);
}

// -------------------------------------------------------------
// -------------------------------------------------------------
// หน้าที่ 5: ESP32 Cloud Hub Screen (แผงควบคุมรีเลย์ 4 ช่อง สไตล์ High-Tech Dashboard)
// -------------------------------------------------------------
inline void build_cloud_hub_screen() {
  if (cloud_hub_scr) lv_obj_del(cloud_hub_scr);
  cloud_hub_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(cloud_hub_scr, lv_color_hex(0x0B0F19), 0);

  // 1. Top Bar
  lv_obj_t *btn_back = lv_button_create(cloud_hub_scr);
  lv_obj_set_size(btn_back, 78, 36);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 12, 8);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_back, 1, 0);
  lv_obj_set_style_radius(btn_back, 8, 0);
  lv_obj_add_event_cb(btn_back, nav_to_menu_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_b = lv_label_create(btn_back);
  lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " Apps");
  lv_obj_center(lbl_b);

  lv_obj_t *title = lv_label_create(cloud_hub_scr);
  lv_label_set_text(title, "ESP32 CLOUD HUB");
  lv_obj_set_style_text_font(title, FONT_TITLE, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x38BDF8), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *sub_title = lv_label_create(cloud_hub_scr);
  lv_label_set_text(sub_title, "smart-iot-th.github.io/Server-ESP32");
  lv_obj_set_style_text_font(sub_title, FONT_SMALL, 0);
  lv_obj_set_style_text_color(sub_title, lv_color_hex(0x64748B), 0);
  lv_obj_align(sub_title, LV_ALIGN_TOP_MID, 0, 26);

  // Cloud Status Pill Badge (ไม่มีตัวอักษรสี่เหลี่ยมหลุดกรอบ)
  lv_obj_t *badge_box = lv_obj_create(cloud_hub_scr);
  lv_obj_set_size(badge_box, 105, 34);
  lv_obj_align(badge_box, LV_ALIGN_TOP_RIGHT, -12, 8);
  lv_obj_set_style_bg_color(badge_box, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(badge_box, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_width(badge_box, 1, 0);
  lv_obj_set_style_radius(badge_box, 17, 0);
  lv_obj_set_style_pad_all(badge_box, 0, 0);
  lv_obj_remove_flag(badge_box, LV_OBJ_FLAG_SCROLLABLE);

  cloud_status_badge = lv_label_create(badge_box);
  if (is_connected) {
    lv_label_set_text(cloud_status_badge, LV_SYMBOL_WIFI " Online");
    lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0x10B981), 0);
  } else {
    lv_label_set_text(cloud_status_badge, LV_SYMBOL_WARNING " Offline");
    lv_obj_set_style_text_color(cloud_status_badge, lv_color_hex(0xEF4444), 0);
  }
  lv_obj_set_style_text_font(cloud_status_badge, FONT_SMALL, 0);
  lv_obj_center(cloud_status_badge);

  // 2. 4-Channel Relay Grid (2 แถว x 2 คอลัมน์ ดีไซน์เรียบง่าย มินิมอล)
  const char* relay_names[4] = {"Relay 1", "Relay 2", "Relay 3", "Relay 4"};

  for (int i = 0; i < 4; i++) {
    int col = i % 2;
    int row = i / 2;
    int x = (col == 0) ? 12 : 246;
    int y = 48 + (row * 84);

    card_relay[i] = lv_obj_create(cloud_hub_scr);
    lv_obj_set_size(card_relay[i], 222, 78);
    lv_obj_align(card_relay[i], LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_set_style_radius(card_relay[i], 12, 0);
    lv_obj_set_style_pad_all(card_relay[i], 0, 0);
    lv_obj_remove_flag(card_relay[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(card_relay[i], cloud_relay_card_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

    // ไอคอน Power สวยงามเรียบง่าย ทางซ้าย
    icon_relay_box[i] = lv_obj_create(card_relay[i]);
    lv_obj_set_size(icon_relay_box[i], 42, 42);
    lv_obj_align(icon_relay_box[i], LV_ALIGN_LEFT_MID, 12, 0);
    lv_obj_set_style_radius(icon_relay_box[i], 10, 0);
    lv_obj_set_style_pad_all(icon_relay_box[i], 0, 0);
    lv_obj_remove_flag(icon_relay_box[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(icon_relay_box[i], LV_OBJ_FLAG_CLICKABLE);

    icon_relay[i] = lv_label_create(icon_relay_box[i]);
    lv_label_set_text(icon_relay[i], LV_SYMBOL_POWER);
    lv_obj_center(icon_relay[i]);

    // ชี่อ Relay ตรงกลาง (ขนาดชัดเจน กึ่งกลางพอดี)
    lv_obj_t *lbl_name = lv_label_create(card_relay[i]);
    lv_label_set_text(lbl_name, relay_names[i]);
    lv_obj_set_style_text_font(lbl_name, FONT_TITLE, 0);
    lv_obj_set_style_text_color(lbl_name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 64, 0);

    // สวิตช์เปิด-ปิด ด้านขวา
    sw_relay[i] = lv_switch_create(card_relay[i]);
    lv_obj_set_size(sw_relay[i], 46, 24);
    lv_obj_align(sw_relay[i], LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_bg_color(sw_relay[i], lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw_relay[i], lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw_relay[i], lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_add_event_cb(sw_relay[i], cloud_relay_toggle_cb, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);

    badge_relay[i] = nullptr;

    // กำหนดสีและเอฟเฟกต์เริ่มต้น
    update_relay_card_style(i);
  }

  // 3. แถบควบคุมด้านล่าง (Master Controls & Diagnostic LED)
  // ปุ่ม ALL ON
  lv_obj_t *btn_on = lv_button_create(cloud_hub_scr);
  lv_obj_set_size(btn_on, 136, 44);
  lv_obj_align(btn_on, LV_ALIGN_BOTTOM_LEFT, 12, -12);
  lv_obj_set_style_bg_color(btn_on, lv_color_hex(0x0284C7), 0);
  lv_obj_set_style_border_color(btn_on, lv_color_hex(0x38BDF8), 0);
  lv_obj_set_style_border_width(btn_on, 1, 0);
  lv_obj_set_style_radius(btn_on, 10, 0);
  lv_obj_add_event_cb(btn_on, cloud_all_relays_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);
  lv_obj_t *lbl_on = lv_label_create(btn_on);
  lv_label_set_text(lbl_on, LV_SYMBOL_CHARGE " ALL ON");
  lv_obj_set_style_text_font(lbl_on, FONT_TITLE, 0);
  lv_obj_center(lbl_on);

  // ปุ่ม ALL OFF
  lv_obj_t *btn_off = lv_button_create(cloud_hub_scr);
  lv_obj_set_size(btn_off, 136, 44);
  lv_obj_align(btn_off, LV_ALIGN_BOTTOM_LEFT, 156, -12);
  lv_obj_set_style_bg_color(btn_off, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_off, lv_color_hex(0x475569), 0);
  lv_obj_set_style_border_width(btn_off, 1, 0);
  lv_obj_set_style_radius(btn_off, 10, 0);
  lv_obj_add_event_cb(btn_off, cloud_all_relays_cb, LV_EVENT_CLICKED, (void*)(intptr_t)0);
  lv_obj_t *lbl_off = lv_label_create(btn_off);
  lv_label_set_text(lbl_off, LV_SYMBOL_STOP " ALL OFF");
  lv_obj_set_style_text_font(lbl_off, FONT_TITLE, 0);
  lv_obj_set_style_text_color(lbl_off, lv_color_hex(0xCBD5E1), 0);
  lv_obj_center(lbl_off);

  // การ์ด Built-in LED (GPIO 2)
  led_card = lv_obj_create(cloud_hub_scr);
  lv_obj_set_size(led_card, 164, 44);
  lv_obj_align(led_card, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_set_style_radius(led_card, 10, 0);
  lv_obj_set_style_pad_all(led_card, 0, 0);
  lv_obj_remove_flag(led_card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(led_card, cloud_led_card_cb, LV_EVENT_CLICKED, NULL);

  led_icon_box = lv_obj_create(led_card);
  lv_obj_set_size(led_icon_box, 30, 30);
  lv_obj_align(led_icon_box, LV_ALIGN_LEFT_MID, 6, 0);
  lv_obj_set_style_radius(led_icon_box, 6, 0);
  lv_obj_set_style_pad_all(led_icon_box, 0, 0);
  lv_obj_remove_flag(led_icon_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(led_icon_box, LV_OBJ_FLAG_CLICKABLE);

  led_icon = lv_label_create(led_icon_box);
  lv_label_set_text(led_icon, LV_SYMBOL_EYE_OPEN);
  lv_obj_center(led_icon);

  lv_obj_t *lbl_led_title = lv_label_create(led_card);
  lv_label_set_text(lbl_led_title, "LED (GPIO 2)");
  lv_obj_set_style_text_font(lbl_led_title, FONT_SMALL, 0);
  lv_obj_set_style_text_color(lbl_led_title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lbl_led_title, LV_ALIGN_TOP_LEFT, 42, 4);

  led_badge = lv_label_create(led_card);
  lv_obj_set_style_text_font(led_badge, FONT_SMALL, 0);
  lv_obj_align(led_badge, LV_ALIGN_TOP_LEFT, 42, 22);

  sw_led = lv_switch_create(led_card);
  lv_obj_set_size(sw_led, 38, 20);
  lv_obj_align(sw_led, LV_ALIGN_RIGHT_MID, -6, 0);
  lv_obj_set_style_bg_color(sw_led, lv_color_hex(0x1E293B), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sw_led, lv_color_hex(0xF59E0B), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sw_led, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_add_event_cb(sw_led, cloud_led_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);

  update_led_card_style();
}

// -------------------------------------------------------------
// หน้าที่ 6: Smart Weather & Clock (Kanchanaburi)
// -------------------------------------------------------------
inline void build_weather_screen() {
  if (weather_scr) lv_obj_del(weather_scr);
  weather_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(weather_scr, lv_color_hex(0x0B0F19), 0);

  // Top Bar
  lv_obj_t *btn_back = lv_button_create(weather_scr);
  lv_obj_set_size(btn_back, 75, 36);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 15, 12);
  lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_border_color(btn_back, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(btn_back, 1, 0);
  lv_obj_add_event_cb(btn_back, nav_to_menu_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_b = lv_label_create(btn_back);
  lv_label_set_text(lbl_b, LV_SYMBOL_LEFT " Apps");
  lv_obj_center(lbl_b);

  lv_obj_t *city_lbl = lv_label_create(weather_scr);
  lv_label_set_text(city_lbl, "Kanchanaburi, TH");
  lv_obj_set_style_text_font(city_lbl, FONT_TITLE, 0);
  lv_obj_set_style_text_color(city_lbl, lv_color_hex(0x00E5FF), 0);
  lv_obj_align(city_lbl, LV_ALIGN_TOP_MID, 0, 18);

  // กล่องแสดงผลหลัก (Main Weather Card)
  lv_obj_t *main_card = lv_obj_create(weather_scr);
  lv_obj_set_size(main_card, 440, 140);
  lv_obj_align(main_card, LV_ALIGN_TOP_MID, 0, 56);
  lv_obj_set_style_bg_color(main_card, lv_color_hex(0x131B2E), 0);
  lv_obj_set_style_border_color(main_card, lv_color_hex(0x1E293B), 0);
  lv_obj_set_style_radius(main_card, 12, 0);
  lv_obj_remove_flag(main_card, LV_OBJ_FLAG_SCROLLABLE);

  // นาฬิกาและวันที่
  weather_clock_label = lv_label_create(main_card);
  lv_label_set_text(weather_clock_label, "12:00:00");
  lv_obj_set_style_text_font(weather_clock_label, FONT_CLOCK, 0);
  lv_obj_set_style_text_color(weather_clock_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(weather_clock_label, LV_ALIGN_TOP_LEFT, 15, 15);

  weather_date_label = lv_label_create(main_card);
  lv_label_set_text(weather_date_label, "Sun, 01 Jan 2026");
  lv_obj_set_style_text_font(weather_date_label, FONT_NORMAL, 0);
  lv_obj_set_style_text_color(weather_date_label, lv_color_hex(0x8A99AD), 0);
  lv_obj_align_to(weather_date_label, weather_clock_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  // อุณหภูมิและไอคอนสภาพอากาศ
  weather_status_icon = draw_weather_app_icon(main_card);
  lv_obj_align(weather_status_icon, LV_ALIGN_TOP_RIGHT, -15, 10);

  weather_temp_label = lv_label_create(main_card);
  lv_label_set_text(weather_temp_label, "32 C");
  lv_obj_set_style_text_font(weather_temp_label, FONT_CLOCK, 0);
  lv_obj_set_style_text_color(weather_temp_label, lv_color_hex(0x00E5FF), 0);
  lv_obj_align(weather_temp_label, LV_ALIGN_RIGHT_MID, -15, 15);

  weather_desc_label = lv_label_create(main_card);
  lv_label_set_text(weather_desc_label, "Partly Cloudy");
  lv_obj_set_style_text_font(weather_desc_label, FONT_SMALL, 0);
  lv_obj_set_style_text_color(weather_desc_label, lv_color_hex(0x8A99AD), 0);
  lv_obj_align_to(weather_desc_label, weather_temp_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 2);

  // การ์ดพยากรณ์อากาศ 3 วันด้านล่าง
  const char* days[3] = {"Tomorrow", "Friday", "Saturday"};
  const char* temps[3] = {"33 C", "31 C", "29 C"};

  for (int i = 0; i < 3; i++) {
    lv_obj_t *f_card = lv_obj_create(weather_scr);
    lv_obj_set_size(f_card, 138, 100);
    lv_obj_align(f_card, LV_ALIGN_BOTTOM_LEFT, 16 + (i * 151), -14);
    lv_obj_set_style_bg_color(f_card, lv_color_hex(0x131B2E), 0);
    lv_obj_set_style_border_color(f_card, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_radius(f_card, 10, 0);
    lv_obj_remove_flag(f_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *d_lbl = lv_label_create(f_card);
    lv_label_set_text(d_lbl, days[i]);
    lv_obj_set_style_text_font(d_lbl, FONT_SMALL, 0);
    lv_obj_set_style_text_color(d_lbl, lv_color_hex(0x8A99AD), 0);
    lv_obj_align(d_lbl, LV_ALIGN_TOP_MID, 0, -2);

    if (i == 0) draw_mini_sun(f_card);
    else if (i == 1) draw_mini_cloud(f_card);
    else draw_mini_rain(f_card);

    lv_obj_t *t_lbl = lv_label_create(f_card);
    lv_label_set_text(t_lbl, temps[i]);
    lv_obj_set_style_text_font(t_lbl, FONT_NORMAL, 0);
    lv_obj_set_style_text_color(t_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(t_lbl, LV_ALIGN_BOTTOM_MID, 0, 2);
  }
}
