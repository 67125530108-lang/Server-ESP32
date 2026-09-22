#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "config.h"
#include "display_touch.h"
#include "ui_icons.h"
#include "wifi_manager.h"
#include "ui_screens.h"

// -------------------------------------------------------------
// สร้าง Instance ของฮาร์ดแวร์และตัวแปร Global
// -------------------------------------------------------------
LGFX tft;
uint8_t *draw_buf = nullptr;
lv_display_t *disp = nullptr;
lv_indev_t *indev = nullptr;
SemaphoreHandle_t lvgl_mutex = NULL;

lv_obj_t *main_scr = nullptr;
lv_obj_t *wifi_scr = nullptr;
lv_obj_t *menu_scr = nullptr;
lv_obj_t *weather_scr = nullptr;
lv_obj_t *settings_scr = nullptr;
lv_obj_t *cloud_hub_scr = nullptr;

lv_obj_t *clock_label = nullptr;
lv_obj_t *date_label = nullptr;
lv_obj_t *weather_clock_label = nullptr;
lv_obj_t *weather_date_label = nullptr;
lv_obj_t *status_label = nullptr;

lv_obj_t *wifi_list = nullptr;
lv_obj_t *wifi_status_label = nullptr;
lv_obj_t *current_conn_panel = nullptr;
lv_obj_t *current_conn_lbl = nullptr;
lv_obj_t *pwd_modal = nullptr;
lv_obj_t *pwd_title_lbl = nullptr;
lv_obj_t *ta_pass = nullptr;
lv_obj_t *kb = nullptr;
lv_obj_t *btn_connect = nullptr;
lv_obj_t *wifi_btn_disc = nullptr;

lv_obj_t *brightness_slider = nullptr;
lv_obj_t *brightness_val_lbl = nullptr;
uint8_t screen_brightness = 85;

lv_obj_t *weather_status_icon = nullptr;
lv_obj_t *weather_temp_label = nullptr;
lv_obj_t *weather_desc_label = nullptr;

lv_obj_t *card_relay[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *icon_relay_box[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *icon_relay[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *badge_relay[4] = {nullptr, nullptr, nullptr, nullptr};
lv_obj_t *sw_relay[4] = {nullptr, nullptr, nullptr, nullptr};

lv_obj_t *led_card = nullptr;
lv_obj_t *led_icon_box = nullptr;
lv_obj_t *led_icon = nullptr;
lv_obj_t *led_badge = nullptr;
lv_obj_t *sw_led = nullptr;

lv_obj_t *cloud_status_badge = nullptr;
bool relay_states[4] = {false, false, false, false};
bool led_state = false;
unsigned long lastTouchTime[4] = {0, 0, 0, 0};
unsigned long lastLedTouchTime = 0;

char selected_ssid[64] = "";
bool is_connected = false;
bool is_connecting = false;
uint32_t connect_start_time = 0;

static uint32_t my_tick_fn() {
  return millis();
}

// -------------------------------------------------------------
// เริ่มต้น UI และสร้าง Timers
// -------------------------------------------------------------
void ui_init() {
  load_saved_wifi();
  build_main_screen();
  lv_screen_load(main_scr);

  // Timer จัดการ Wi-Fi และนาฬิกา
  lv_timer_create(wifi_manager_timer_cb, 500, NULL);
  lv_timer_create(live_clock_timer_cb, 1000, NULL);
}

// FreeRTOS Task รัน UI บน Core 1 แยกอิสระ (Thread-safe ด้วย Mutex)
void guiTask(void *pvParameters) {
  while (1) {
    if (lvgl_mutex && xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(25)) == pdTRUE) {
      lv_timer_handler();
      xSemaphoreGive(lvgl_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// -------------------------------------------------------------
// Arduino Setup
// -------------------------------------------------------------
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // ปิด Brownout ป้องกันจอรีเซ็ตช่วงดึงกระแสไฟ
  delay(100);
  Serial.begin(115200);
  Serial.println("\n[SYSTEM] ESP32 ST7796 LVGL v9 Modular System Starting...");

  // สร้าง Mutex ควบคุมสิทธิ์การเข้าถึง LVGL ระหว่าง Core 0 และ Core 1
  lvgl_mutex = xSemaphoreCreateMutex();

  // ตั้งค่าขาฮาร์ดแวร์ Relay & LED
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT);
  pinMode(PIN_RELAY_4, OUTPUT);
  pinMode(PIN_LED_BOARD, OUTPUT);
  digitalWrite(PIN_RELAY_1, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_2, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_3, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_4, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_LED_BOARD, LOW);

  // ตั้งค่าหน้าจอและทัชสกรีน
  tft.init();
  tft.setRotation(1); // แนวนอน 480x320
  load_screen_brightness();
  load_touch_calibration();

  // ตั้งค่า LVGL v9
  lv_init();
  lv_tick_set_cb(my_tick_fn);

  draw_buf = (uint8_t *)malloc(DRAW_BUF_SIZE);
  if (!draw_buf) {
    Serial.println("[ERROR] Failed to allocate LVGL draw buffer!");
    while (1) delay(1000);
  }

  disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, disp);

  // สร้างหน้าจอ UI
  ui_init();

  // สั่งรัน guiTask บน Core 1
  xTaskCreatePinnedToCore(guiTask, "guiTask", 8192, NULL, 1, NULL, 1);
  Serial.println("[SYSTEM] Setup completed successfully!");
}

// -------------------------------------------------------------
// Arduino Loop (รันงาน Network & GitHub Sync บน Core 0 แยกอิสระ)
// -------------------------------------------------------------
void loop() {
  // จัดการซิงค์คำสั่งและส่ง Heartbeat ของจอภาพขึ้น GitHub Cloud Hub
  handleGitHubSync();
  vTaskDelay(pdMS_TO_TICKS(10));
}
