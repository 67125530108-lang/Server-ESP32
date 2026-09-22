#pragma once
#include <Arduino.h>
#include <lvgl.h>

// -------------------------------------------------------------
// ขนาดหน้าจอ LCD และ Buffer สำหรับ LVGL v9
// -------------------------------------------------------------
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320
#define BUFFER_LINES  20
#define DRAW_BUF_SIZE (SCREEN_WIDTH * BUFFER_LINES * sizeof(lv_color16_t))

// -------------------------------------------------------------
// ขาควบคุมฮาร์ดแวร์
// -------------------------------------------------------------
#define PIN_RELAY_1     4       // ขา Relay 1 (K1)
#define PIN_RELAY_2     5       // ขา Relay 2 (K2)
#define PIN_RELAY_3     18      // ขา Relay 3 (K3)
#define PIN_RELAY_4     19      // ขา Relay 4 (K4)
#define PIN_LED_BOARD   2       // ไฟสถานะบนบอร์ด ESP32
#define RELAY_ACTIVE_LOW true   // รีเลย์ทำงานแบบ Active LOW

// -------------------------------------------------------------
// การตั้งค่า GitHub Cloud Hub & API
// -------------------------------------------------------------
#define GITHUB_OWNER        "smart-iot-Th"
#define GITHUB_REPO         "Server-ESP32"
#define GITHUB_BRANCH       "main"
#define GITHUB_FILE_PATH    "data/state.json"
#define GITHUB_TOKEN        "YOUR_GITHUB_PERSONAL_ACCESS_TOKEN" // ใส่ Token ของท่านที่นี่
#define CLOUD_POLL_INTERVAL_MS      3000  // ดึงคำสั่งทุก 3.0 วินาที
#define CLOUD_HEARTBEAT_INTERVAL_MS 30000 // ส่ง Heartbeat จอภาพทุก 30 วินาที
#define MUTATION_HOLD_TIME_MS       4000  // Cooldown Protection Guard: ล็อกสถานะ 4 วินาที ป้องกันข้อมูลเก่าจาก Cloud มาเขียนทับ

// FreeRTOS Mutex สำหรับจัดการความปลอดภัยเธรดระหว่าง GUI (Core 1) และ Network (Core 0)
extern SemaphoreHandle_t lvgl_mutex;

// ตัวแปรบันทึกเวลาสัมผัสล่าสุด ป้องกันการดีดกลับ (State Snap-Back Guard)
extern unsigned long lastTouchTime[4];
extern unsigned long lastLedTouchTime;

// -------------------------------------------------------------
// ฟอนต์มาตรฐาน
// -------------------------------------------------------------
#define FONT_CLOCK  &lv_font_montserrat_24
#define FONT_TITLE  &lv_font_montserrat_14
#define FONT_NORMAL &lv_font_montserrat_14
#define FONT_SMALL  &lv_font_montserrat_14

// -------------------------------------------------------------
// ตัวแปรอ้างอิงหน้าจอทั้งหมด (Screens)
// -------------------------------------------------------------
extern lv_obj_t *main_scr;
extern lv_obj_t *wifi_scr;
extern lv_obj_t *menu_scr;
extern lv_obj_t *weather_scr;
extern lv_obj_t *settings_scr;
extern lv_obj_t *cloud_hub_scr;

// วิดเจ็ตหน้าหลัก
extern lv_obj_t *clock_label;
extern lv_obj_t *date_label;
extern lv_obj_t *status_label;

// วิดเจ็ตหน้า Wi-Fi
extern lv_obj_t *wifi_list;
extern lv_obj_t *wifi_status_label;
extern lv_obj_t *current_conn_panel;
extern lv_obj_t *current_conn_lbl;
extern lv_obj_t *pwd_modal;
extern lv_obj_t *pwd_title_lbl;
extern lv_obj_t *ta_pass;
extern lv_obj_t *kb;
extern lv_obj_t *btn_connect;
extern lv_obj_t *wifi_btn_disc;

// วิดเจ็ตหน้า Settings
extern lv_obj_t *brightness_slider;
extern lv_obj_t *brightness_val_lbl;
extern uint8_t screen_brightness;

// วิดเจ็ตหน้า Weather
extern lv_obj_t *weather_clock_label;
extern lv_obj_t *weather_date_label;
extern lv_obj_t *weather_status_icon;
extern lv_obj_t *weather_temp_label;
extern lv_obj_t *weather_desc_label;

// วิดเจ็ตหน้า Cloud Hub
extern lv_obj_t *card_relay[4];
extern lv_obj_t *icon_relay_box[4];
extern lv_obj_t *icon_relay[4];
extern lv_obj_t *badge_relay[4];
extern lv_obj_t *sw_relay[4];

extern lv_obj_t *led_card;
extern lv_obj_t *led_icon_box;
extern lv_obj_t *led_icon;
extern lv_obj_t *led_badge;
extern lv_obj_t *sw_led;

extern lv_obj_t *cloud_status_badge;
extern bool relay_states[4];
extern bool led_state;

// ตัวแปรการซิงค์ Cloud & ป้องกัน Echo Loopback
extern bool is_syncing_from_cloud;
extern float cloud_temp;
extern float cloud_hum;

// ตัวแปรสถานะทั่วไป
extern char selected_ssid[64];
extern bool is_connected;
extern bool is_connecting;
extern uint32_t connect_start_time;

// LVGL v9 Display & Indev pointers
extern uint8_t *draw_buf;
extern lv_display_t *disp;
extern lv_indev_t *indev;
