#pragma once
#include "config.h"

// ฟังก์ชันสร้างดวงไฟ LED ทรงกลมพร้อมเรืองแสง
inline lv_obj_t* create_glowing_led(lv_obj_t *parent, lv_color_t color, int size = 10) {
  lv_obj_t *led = lv_obj_create(parent);
  lv_obj_set_size(led, size, size);
  lv_obj_set_style_radius(led, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(led, color, 0);
  lv_obj_set_style_border_width(led, 0, 0);
  lv_obj_set_style_shadow_color(led, color, 0);
  lv_obj_set_style_shadow_width(led, 8, 0);
  lv_obj_set_style_shadow_spread(led, 2, 0);
  lv_obj_remove_flag(led, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(led, LV_OBJ_FLAG_CLICKABLE);
  return led;
}

// ไอคอนสภาพอากาศเวกเตอร์ 🌤️ (ดวงอาทิตย์ + ก้อนเมฆ)
inline lv_obj_t* draw_weather_app_icon(lv_obj_t *parent) {
  lv_obj_t *icon_cont = lv_obj_create(parent);
  lv_obj_set_size(icon_cont, 52, 42);
  lv_obj_set_style_bg_opa(icon_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(icon_cont, 0, 0);
  lv_obj_set_style_pad_all(icon_cont, 0, 0);
  lv_obj_remove_flag(icon_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(icon_cont, LV_OBJ_FLAG_CLICKABLE);

  // ดวงอาทิตย์สีเหลืองส้ม
  lv_obj_t *sun = lv_obj_create(icon_cont);
  lv_obj_set_size(sun, 24, 24);
  lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFB300), 0);
  lv_obj_set_style_border_width(sun, 0, 0);
  lv_obj_set_style_shadow_color(sun, lv_color_hex(0xFF9800), 0);
  lv_obj_set_style_shadow_width(sun, 12, 0);
  lv_obj_align(sun, LV_ALIGN_TOP_RIGHT, -4, 2);
  lv_obj_remove_flag(sun, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(sun, LV_OBJ_FLAG_CLICKABLE);

  // ก้อนเมฆหลัก
  lv_obj_t *cloud_base = lv_obj_create(icon_cont);
  lv_obj_set_size(cloud_base, 34, 16);
  lv_obj_set_style_radius(cloud_base, 8, 0);
  lv_obj_set_style_bg_color(cloud_base, lv_color_hex(0xE0F7FA), 0);
  lv_obj_set_style_border_width(cloud_base, 0, 0);
  lv_obj_align(cloud_base, LV_ALIGN_BOTTOM_LEFT, 2, -2);
  lv_obj_remove_flag(cloud_base, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(cloud_base, LV_OBJ_FLAG_CLICKABLE);

  // ลอนเมฆบน
  lv_obj_t *cloud_puff = lv_obj_create(icon_cont);
  lv_obj_set_size(cloud_puff, 18, 18);
  lv_obj_set_style_radius(cloud_puff, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(cloud_puff, lv_color_hex(0xE0F7FA), 0);
  lv_obj_set_style_border_width(cloud_puff, 0, 0);
  lv_obj_align_to(cloud_puff, cloud_base, LV_ALIGN_OUT_TOP_MID, -2, 7);
  lv_obj_remove_flag(cloud_puff, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(cloud_puff, LV_OBJ_FLAG_CLICKABLE);

  return icon_cont;
}

// ไอคอนเทคโนโลยี ESP32 Cloud Hub (High-Tech IoT Microchip Processor) 📟⚡
inline lv_obj_t* draw_cloud_hub_icon(lv_obj_t *parent) {
  lv_obj_t *icon_cont = lv_obj_create(parent);
  lv_obj_set_size(icon_cont, 56, 48);
  lv_obj_set_style_bg_opa(icon_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(icon_cont, 0, 0);
  lv_obj_set_style_pad_all(icon_cont, 0, 0);
  lv_obj_remove_flag(icon_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(icon_cont, LV_OBJ_FLAG_CLICKABLE);

  // 1. บอดี้ตัวชิปหลัก (Microchip Body)
  lv_obj_t *chip = lv_obj_create(icon_cont);
  lv_obj_set_size(chip, 36, 36);
  lv_obj_set_style_radius(chip, 6, 0);
  lv_obj_set_style_bg_color(chip, lv_color_hex(0x090D16), 0);
  lv_obj_set_style_border_color(chip, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(chip, 2, 0);
  lv_obj_set_style_shadow_color(chip, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_shadow_width(chip, 12, 0);
  lv_obj_set_style_shadow_spread(chip, 1, 0);
  lv_obj_center(chip);
  lv_obj_remove_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(chip, LV_OBJ_FLAG_CLICKABLE);

  // 2. ขาพินเชื่อมต่อรอบตัวชิป (Hardware Connection Pins)
  // ขาพินฝั่งซ้ายและขวา
  for (int p = 0; p < 3; p++) {
    int y_off = -9 + (p * 9);
    // พินซ้าย
    lv_obj_t *pin_l = lv_obj_create(icon_cont);
    lv_obj_set_size(pin_l, 6, 3);
    lv_obj_set_style_bg_color(pin_l, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(pin_l, 0, 0);
    lv_obj_set_style_radius(pin_l, 1, 0);
    lv_obj_align(pin_l, LV_ALIGN_CENTER, -20, y_off);
    lv_obj_remove_flag(pin_l, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(pin_l, LV_OBJ_FLAG_CLICKABLE);

    // พินขวา
    lv_obj_t *pin_r = lv_obj_create(icon_cont);
    lv_obj_set_size(pin_r, 6, 3);
    lv_obj_set_style_bg_color(pin_r, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(pin_r, 0, 0);
    lv_obj_set_style_radius(pin_r, 1, 0);
    lv_obj_align(pin_r, LV_ALIGN_CENTER, 20, y_off);
    lv_obj_remove_flag(pin_r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(pin_r, LV_OBJ_FLAG_CLICKABLE);
  }

  // ขาพินฝั่งบนและล่าง
  for (int p = 0; p < 3; p++) {
    int x_off = -9 + (p * 9);
    // พินบน
    lv_obj_t *pin_t = lv_obj_create(icon_cont);
    lv_obj_set_size(pin_t, 3, 6);
    lv_obj_set_style_bg_color(pin_t, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(pin_t, 0, 0);
    lv_obj_set_style_radius(pin_t, 1, 0);
    lv_obj_align(pin_t, LV_ALIGN_CENTER, x_off, -20);
    lv_obj_remove_flag(pin_t, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(pin_t, LV_OBJ_FLAG_CLICKABLE);

    // พินล่าง
    lv_obj_t *pin_b = lv_obj_create(icon_cont);
    lv_obj_set_size(pin_b, 3, 6);
    lv_obj_set_style_bg_color(pin_b, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(pin_b, 0, 0);
    lv_obj_set_style_radius(pin_b, 1, 0);
    lv_obj_align(pin_b, LV_ALIGN_CENTER, x_off, 20);
    lv_obj_remove_flag(pin_b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(pin_b, LV_OBJ_FLAG_CLICKABLE);
  }

  // 3. แกนประมวลผลเรืองแสงตรงกลาง (Inner Silicon Cyber Core)
  lv_obj_t *core_box = lv_obj_create(chip);
  lv_obj_set_size(core_box, 18, 18);
  lv_obj_set_style_radius(core_box, 4, 0);
  lv_obj_set_style_bg_color(core_box, lv_color_hex(0x03283B), 0);
  lv_obj_set_style_border_color(core_box, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(core_box, 1, 0);
  lv_obj_center(core_box);
  lv_obj_remove_flag(core_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(core_box, LV_OBJ_FLAG_CLICKABLE);

  // สัญลักษณ์ Charge นีออนตรงกลางแกนชิป
  lv_obj_t *core_icon = lv_label_create(core_box);
  lv_label_set_text(core_icon, LV_SYMBOL_CHARGE);
  lv_obj_set_style_text_color(core_icon, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_text_font(core_icon, FONT_SMALL, 0);
  lv_obj_center(core_icon);

  return icon_cont;
}

// มินิไอคอนแดด ☀️
inline lv_obj_t* draw_mini_sun(lv_obj_t *parent) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_set_size(c, 26, 26);
  lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(c, 0, 0);
  lv_obj_set_style_pad_all(c, 0, 0);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *sun = lv_obj_create(c);
  lv_obj_set_size(sun, 16, 16);
  lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFB300), 0);
  lv_obj_set_style_border_width(sun, 0, 0);
  lv_obj_set_style_shadow_color(sun, lv_color_hex(0xFF9800), 0);
  lv_obj_set_style_shadow_width(sun, 8, 0);
  lv_obj_center(sun);
  lv_obj_remove_flag(sun, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(sun, LV_OBJ_FLAG_CLICKABLE);
  return c;
}

// มินิไอคอนเมฆ ⛅
inline lv_obj_t* draw_mini_cloud(lv_obj_t *parent) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_set_size(c, 26, 26);
  lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(c, 0, 0);
  lv_obj_set_style_pad_all(c, 0, 0);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *cloud = lv_obj_create(c);
  lv_obj_set_size(cloud, 20, 10);
  lv_obj_set_style_radius(cloud, 5, 0);
  lv_obj_set_style_bg_color(cloud, lv_color_hex(0x90A4AE), 0);
  lv_obj_set_style_border_width(cloud, 0, 0);
  lv_obj_align(cloud, LV_ALIGN_CENTER, 0, 2);
  lv_obj_remove_flag(cloud, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(cloud, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *puff = lv_obj_create(c);
  lv_obj_set_size(puff, 12, 12);
  lv_obj_set_style_radius(puff, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(puff, lv_color_hex(0xB0BEC5), 0);
  lv_obj_set_style_border_width(puff, 0, 0);
  lv_obj_align(puff, LV_ALIGN_CENTER, -2, -2);
  lv_obj_remove_flag(puff, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(puff, LV_OBJ_FLAG_CLICKABLE);
  return c;
}

// มินิไอคอนฝน 🌦️
inline lv_obj_t* draw_mini_rain(lv_obj_t *parent) {
  lv_obj_t *c = draw_mini_cloud(parent);
  lv_obj_t *drop1 = lv_obj_create(c);
  lv_obj_set_size(drop1, 2, 5);
  lv_obj_set_style_bg_color(drop1, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(drop1, 0, 0);
  lv_obj_align(drop1, LV_ALIGN_BOTTOM_MID, -3, 0);
  lv_obj_remove_flag(drop1, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(drop1, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *drop2 = lv_obj_create(c);
  lv_obj_set_size(drop2, 2, 5);
  lv_obj_set_style_bg_color(drop2, lv_color_hex(0x00E5FF), 0);
  lv_obj_set_style_border_width(drop2, 0, 0);
  lv_obj_align(drop2, LV_ALIGN_BOTTOM_MID, 3, 0);
  lv_obj_remove_flag(drop2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(drop2, LV_OBJ_FLAG_CLICKABLE);
  return c;
}
