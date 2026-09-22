#pragma once
#include "config.h"
#include <Preferences.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// -------------------------------------------------------------
// คอนฟิก LovyanGFX (ST7796 + XPT2046)
// -------------------------------------------------------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX() {
    // 1. SPI Bus จอภาพ
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host   = HSPI_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 27000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk   = 14;
      cfg.pin_mosi   = 13;
      cfg.pin_miso   = 12;
      cfg.pin_dc     = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // 2. จอ ST7796
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 320;
      cfg.panel_height     = 480;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }

    // 3. Backlight
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = 27;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    // 4. ทัชสกรีน XPT2046
    {
      auto cfg = _touch_instance.config();
      cfg.x_min           = 0;
      cfg.x_max           = 479;
      cfg.y_min           = 0;
      cfg.y_max           = 319;
      cfg.pin_int         = -1;
      cfg.bus_shared      = true;
      cfg.offset_rotation = 0;
      cfg.spi_host        = HSPI_HOST;
      cfg.freq            = 2000000;
      cfg.pin_sclk        = 14;
      cfg.pin_mosi        = 13;
      cfg.pin_miso        = 12;
      cfg.pin_cs          = 33;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

extern LGFX tft;

// ฟังก์ชันปรับความสว่างจอ LCD (10% - 100%) พร้อมบันทึกค่า
inline void set_screen_brightness(uint8_t percent, bool save = true) {
  if (percent < 10) percent = 10;
  if (percent > 100) percent = 100;
  screen_brightness = percent;

  uint8_t pwm = map(percent, 10, 100, 25, 255);
  tft.setBrightness(pwm);

  if (save) {
    Preferences prefs;
    prefs.begin("disp_cfg", false);
    prefs.putUChar("bright", percent);
    prefs.end();
  }
}

// โหลดค่าความสว่างจอที่บันทึกไว้
inline void load_screen_brightness() {
  Preferences prefs;
  prefs.begin("disp_cfg", true);
  uint8_t b = prefs.getUChar("bright", 85);
  prefs.end();
  set_screen_brightness(b, false);
}

// บันทึกและอ่านค่า Calibration ทัชสกรีน
inline void run_touch_calibration_flow() {
  Serial.println("[TOUCH] Running Screen Calibration...");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCenterString("TOUCH CALIBRATION", 240, 60);
  tft.setTextSize(1);
  tft.drawCenterString("Touch the arrows precisely with stylus", 240, 90);

  uint16_t calData[8];
  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 25);

  Preferences prefs;
  prefs.begin("touch_cfg", false);
  prefs.putBytes("calData", calData, sizeof(calData));
  prefs.putBool("calibrated", true);
  prefs.end();

  Serial.println("[TOUCH] Calibration saved to NVS!");
  tft.fillScreen(TFT_BLACK);
}

inline void load_touch_calibration(bool force_recal = false) {
  Preferences prefs;
  prefs.begin("touch_cfg", false);
  bool is_cal = prefs.getBool("calibrated", false);

  if (!is_cal || force_recal) {
    prefs.end();
    run_touch_calibration_flow();
  } else {
    uint16_t calData[8];
    prefs.getBytes("calData", calData, sizeof(calData));
    tft.setTouchCalibrate(calData);
    prefs.end();
    Serial.println("[TOUCH] Loaded calibration from NVS.");
  }
}

// Flush Buffer ไปยังหน้าจอ (LVGL v9 API)
inline void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// อ่านพิกัดทัชสกรีนส่งให้ LVGL (LVGL v9 API)
inline void my_touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);
  if (!touched) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touchX;
    data->point.y = touchY;
  }
}
