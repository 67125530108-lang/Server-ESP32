# คู่มือการติดตั้งและใช้งานเฟิร์มแวร์ ESP32 กับ GitHub

คู่มือนี้จะอธิบายขั้นตอนการนำโค้ดในโฟลเดอร์นี้ไปคอมไพล์และแฟลชลงบนบอร์ด **ESP32** เพื่อเชื่อมต่อกับ GitHub Repository

---

## 1. เตรียมโปรแกรมและไลบรารี

### 1.1 ติดตั้ง Arduino IDE
- ดาวน์โหลดและติดตั้ง [Arduino IDE](https://www.arduino.cc/en/software) (เวอร์ชัน 2.x แนะนำ)
- ไปที่ **File > Preferences** ในช่อง *Additional boards manager URLs* ให้เพิ่ม:
  ```text
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- ไปที่ **Tools > Board > Boards Manager...** ค้นหา `esp32` โดย **Espressif Systems** แล้วกด **Install**

### 1.2 ติดตั้งไลบรารี ArduinoJson
1. ใน Arduino IDE ไปที่เมนู **Tools > Manage Libraries...** (หรือกด `Ctrl + Shift + I`)
2. ค้นหาคำว่า `ArduinoJson` โดย **Benoit Blanchon**
3. เลือกเวอร์ชันล่าสุด (v6 หรือ v7 รองรับทั้งสองเวอร์ชัน) แล้วกด **Install**

*(หมายเหตุ: ไลบรารี WiFi, HTTPClient, WiFiClientSecure และ mbedtls เป็นไลบรารีมาตรฐานที่ติดตั้งมาพร้อมกับ Core ESP32 อยู่แล้ว ไม่ต้องติดตั้งเพิ่ม)*

---

## 2. ตั้งค่าไฟล์ `config.h`

1. ทำการคัดลอกไฟล์ `config.example.h` แล้วเปลี่ยนชื่อเป็น `config.h`:
   ```bash
   cp config.example.h config.h
   ```
2. เปิดไฟล์ `config.h` และแก้ไขข้อมูลให้เป็นของคุณ:
   ```cpp
   #define WIFI_SSID       "ชื่อWiFiของคุณ"
   #define WIFI_PASSWORD   "รหัสผ่านWiFiของคุณ"

   #define GITHUB_OWNER    "ชื่อผู้ใช้GitHub"      // เช่น somchai-iot
   #define GITHUB_REPO     "Server-ESP32"         // ชื่อ Repository ที่เก็บ state.json
   #define GITHUB_BRANCH   "main"
   #define GITHUB_FILE_PATH "data/state.json"

   // ใส่ Personal Access Token ที่สร้างจาก GitHub
   #define GITHUB_TOKEN    "ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
   ```

---

## 3. การต่อสายฮาร์ดแวร์ (Pinout Diagram)

ค่าเริ่มต้นตามที่กำหนดในโค้ด:

| ขา ESP32 | ต่อเข้ากับอุปกรณ์ | คำอธิบาย |
|:---:|:---:|:---|
| **GPIO 2** | Built-in LED บนบอร์ด | ไฟแสดงสถานะบนตัวบอร์ด |
| **GPIO 4** | Relay Module ช่องที่ 1 | สวิตช์ควบคุมไฟ/เครื่องใช้ไฟฟ้า 1 |
| **GPIO 5** | Relay Module ช่องที่ 2 | สวิตช์ควบคุมไฟ/เครื่องใช้ไฟฟ้า 2 |
| **GPIO 15** | Data Pin เซนเซอร์ (DHT22 / DHT11) | ขาอ่านข้อมูลอุณหภูมิและความชื้น |
| **GND** | GND ของทุกโมดูล | กราวด์ร่วม |
| **VIN / 5V** | VCC ของโมดูลรีเลย์ | ไฟเลี้ยงรีเลย์ 5V |
| **3.3V** | VCC ของเซนเซอร์ | ไฟเลี้ยงเซนเซอร์ 3.3V |

> [!TIP]
> หากยังไม่ได้ต่อเซนเซอร์จริง บอร์ด ESP32 จะทำการจำลองค่าอุณหภูมิและความชื้นพร้อมส่ง RSSI และ Uptime จริงขึ้นไปยัง GitHub ให้โดยอัตโนมัติ เพื่อให้ทดสอบระบบได้ทันที

---

## 4. แฟลชโค้ดลงบอร์ด ESP32

1. เสียบสาย Micro USB หรือ Type-C เข้ากับคอมพิวเตอร์
2. ใน Arduino IDE:
   - ไปที่ **Tools > Board** เลือกบอร์ดของคุณ (เช่น `DOIT ESP32 DEVKIT V1` หรือ `ESP32 Dev Module`)
   - ไปที่ **Tools > Port** เลือกพอร์ต COM ที่เชื่อมต่อกับบอร์ด
   - ไปที่ **Tools > Upload Speed** เลือก `921600` หรือ `115200`
3. เปิดไฟล์ `esp32_github_client.ino`
4. กดปุ่ม **Upload** (ลูกศรชี้ขวา)
5. เปิด **Serial Monitor** (ตั้งค่า Baud Rate ที่ `115200`) เพื่อดูผลการทำงาน
