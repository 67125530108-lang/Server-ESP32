# ESP32 GitHub Cloud Hub 🌐⚡

ระบบตัวกลางเชื่อมต่อและควบคุมบอร์ด **ESP32** ผ่าน **GitHub REST API** และจัดเก็บสถานะบน Repository โดยไม่ต้องเสียค่าเช่า Cloud Server เพิ่มเติม สามารถทำงานร่วมกับ **GitHub Pages** เป็น Web Dashboard ได้ฟรี 100%

---

## 🌟 จุดเด่นของระบบ

- **Zero Cloud Cost**: ใช้ GitHub เป็นเสมือน Cloud Broker และ Database ไม่ต้องเช่า VPS หรือเปิดพอร์ตเราเตอร์
- **Full-Stack IoT Dashboard**: หน้าเว็บควบคุมแบบ Modern Dark Glassmorphism รองรับการใช้งานทั้งบนคอมพิวเตอร์และสมาร์ตโฟน
- **Two-Way Synchronization**:
  - **Commands (คำสั่ง)**: ควบคุม Relay 1, Relay 2, หลอดไฟ LED, สลับโหมด Auto/Manual, และปรับค่าอุณหภูมิเป้าหมาย
  - **Telemetry (สถานะ)**: รับค่าอุณหภูมิ, ความชื้น, ความแรงสัญญาณ WiFi (RSSI), Uptime, IP Address และสถานะออนไลน์/ออฟไลน์ของบอร์ด
- **Auto Conflict Resolution**: บริหารจัดการ Git Commit SHA ป้องกันปัญหาข้อมูลชนกัน (409 Conflict)
- **High Security**: รหัสผ่าน WiFi และ GitHub Token ถูกแยกไฟล์และตั้งค่าใน `.gitignore` เพื่อป้องกันการรั่วไหล

---

## 🏗️ แผนผังการทำงาน (Architecture)

```mermaid
flowchart LR
    subgraph UI ["🖥️ ฝั่งผู้ใช้งาน"]
        Dashboard["Web Dashboard\n(GitHub Pages / Local Browser)"]
    end

    subgraph GitHubCloud ["☁️ GitHub Cloud (ตัวกลาง & Broker)"]
        Repo["GitHub Repository\n(data/state.json)"]
        API["GitHub REST API\n(api.github.com)"]
    end

    subgraph IoT ["🤖 ฝั่งฮาร์ดแวร์"]
        ESP32["บอร์ด ESP32\n(WiFi + HTTPS TLS)"]
        Relay["โมดูลรีเลย์ / หลอดไฟ"]
        Sensor["เซนเซอร์วัดค่าต่างๆ"]
    end

    Dashboard -->|1. ส่งคำสั่ง (Commit PUT)| API
    API -->|บันทึกการเปลี่ยนแปลง| Repo
    ESP32 -->|2. ตรวจสอบคำสั่ง (GET Contents)| API
    ESP32 -->|3. สั่งเปิด-ปิดตามคำสั่ง| Relay
    Sensor -->|4. ส่งค่าสภาพแวดล้อม| ESP32
    ESP32 -->|5. ส่ง Telemetry & Heartbeat (PUT)| API
```

---

## 📁 โครงสร้างโปรเจกต์ (Project Structure)

```text
Server ESP32/
├── data/
│   └── state.json                 # ฐานข้อมูลกลางสำหรับจัดเก็บคำสั่งและสถานะเซนเซอร์
├── dashboard/
│   ├── index.html                 # หน้า Web Dashboard สำหรับมอนิเตอร์และสั่งงาน
│   ├── style.css                  # ชุดตกแต่งสไตล์ Dark Glassmorphism + Responsive
│   └── app.js                     # ตรรกะเชื่อมต่อ GitHub REST API และคำนวณสถานะ
├── esp32/
│   ├── esp32_github_client.ino    # โค้ดโปรแกรมหลักของบอร์ด ESP32 (Arduino C++)
│   ├── config.example.h           # เทมเพลตสำหรับตั้งค่า WiFi และ GitHub Token
│   └── README_ESP32.md            # คู่มือการติดตั้ง Library และแฟลชโค้ดลงบอร์ด
├── .gitignore                     # ไฟล์คัดกรองความปลอดภัย ป้องกัน Token หลุด
└── README.md                      # เอกสารคู่มือการใช้งานฉบับสมบูรณ์
```

---

## 🚀 ขั้นตอนการติดตั้งและเริ่มต้นใช้งาน

### ขั้นตอนที่ 1: เตรียม GitHub Repository และ Personal Access Token

1. นำไฟล์ทั้งหมดในโฟลเดอร์นี้ขึ้นไปยัง **GitHub Repository** ของคุณ (ตั้งชื่อ Repo เช่น `Server-ESP32`)
2. สร้าง **Personal Access Token (PAT)** บน GitHub:
   - ไปที่ **GitHub Profile > Settings > Developer Settings > Personal access tokens**
   - แนะนำเลือก **Fine-grained tokens** หรือ **Tokens (classic)**
   - หากเลือก **Tokens (classic)**: ติ๊กเลือกขอบเขต (Scope) `repo`
   - หากเลือก **Fine-grained tokens**: ในส่วน Repository access เลือก Repo ของคุณ และใน Permissions > Repository permissions ให้ตั้ง **Contents: Read and write**
   - คัดลอก Token (ขึ้นต้นด้วย `ghp_` หรือ `github_pat_`) เก็บไว้ในที่ปลอดภัย

---

### ขั้นตอนที่ 2: เปิดใช้งาน Web Dashboard

คุณสามารถใช้งาน Web Dashboard ได้ 2 วิธี:

#### วิธีที่ 2.1: เปิดผ่านเบราว์เซอร์ในเครื่องโดยตรง (ไม่ต้องติดตั้งอะไรเพิ่ม)
- ดับเบิลคลิกเปิดไฟล์ `dashboard/index.html` บนบราวเซอร์ เช่น Google Chrome, Edge หรือ Safari

#### วิธีที่ 2.2: โฮสต์ฟรีผ่าน GitHub Pages
1. ในหน้า Repository บน GitHub ไปที่ **Settings > Pages**
2. ภายใต้หัวข้อ **Build and deployment > Branch**:
   - เลือก Branch: `main`
   - เลือก Folder: `/dashboard` (หรือ root ถ้าคุณย้ายไฟล์หน้าเว็บไว้ด้านนอก)
3. กด **Save** รอประมาณ 1-2 นาที คุณจะได้ URL ประจำตัว เช่น `https://your-username.github.io/Server-ESP32/dashboard/`

#### การตั้งค่าใน Dashboard:
1. กดปุ่ม **"ตั้งค่า GitHub"** มุมบนขวา
2. กรอก **GitHub Username**, **Repository Name**, และ **Token** ที่สร้างไว้
3. กด **"ทดสอบการเชื่อมต่อ"** จากนั้นกด **"บันทึกการตั้งค่า"**

---

### ขั้นตอนที่ 3: แฟลชโค้ดลงบอร์ด ESP32

1. เข้าไปที่โฟลเดอร์ `esp32/esp32_github_client/`
2. มีไฟล์ `config.h` อยู่ในโฟลเดอร์เดียวกับ `esp32_github_client.ino` (หรือคัดลอกมาจาก `config.example.h`)
3. แก้ไขข้อมูล WiFi SSID, Password และ GitHub Token ของคุณใน `config.h`
4. เปิดไฟล์ `esp32_github_client.ino` ด้วย **Arduino IDE** (จะเห็นแท็บ `config.h` อยู่คู่กัน)
5. ติดตั้งไลบรารี `ArduinoJson` ผ่าน Library Manager
6. กดปุ่ม **Upload** ลงบอร์ด ESP32
7. เปิด Serial Monitor ที่ Baud rate `115200` เพื่อดูการเชื่อมต่อ

*(ดูรายละเอียดเพิ่มเติมได้ที่ [esp32/README_ESP32.md](esp32/README_ESP32.md))*

---

## 📊 โครงสร้างข้อมูลกลาง (`data/state.json`)

```json
{
  "commands": {
    "relay1": false,       // คำสั่ง Relay 1 (true = เปิด, false = ปิด)
    "relay2": false,       // คำสั่ง Relay 2
    "led": true,           // หลอดไฟ LED บนบอร์ด
    "mode": "auto",        // โหมดการทำงาน (auto / manual)
    "target_temp": 25.0    // อุณหภูมิเป้าหมาย
  },
  "telemetry": {
    "temperature": 27.8,   // ค่าอุณหภูมิปัจจุบัน (°C)
    "humidity": 62.5,      // ค่าความชื้นสัมพัทธ์ (%)
    "rssi": -65,           // ความแรงสัญญาณ WiFi (dBm)
    "uptime_sec": 360,     // เวลาทำงานของบอร์ด (วินาที)
    "last_seen": "2026-09-17T23:00:00Z", // เวลาที่บอร์ดติดต่อล่าสุด
    "ip_address": "192.168.1.150"
  },
  "meta": {
    "version": "1.0.0",
    "updated_by": "web_dashboard",
    "updated_at": "2026-09-17T23:00:00Z"
  }
}
```

---

## ⏱️ อัตราการใช้งาน GitHub API (Rate Limits)

- เมื่อใช้ **Personal Access Token**: GitHub อนุญาตให้ส่งคำขอ API ได้สูงสุด **5,000 requests/ชั่วโมง**
- การตั้งค่าความถี่ที่แนะนำ:
  - **ESP32 Polling คำสั่ง**: ทุก 10 วินาที (~360 requests/ชั่วโมง)
  - **ESP32 Push Telemetry**: ทุก 30 วินาที (~120 requests/ชั่วโมง)
  - **Dashboard Polling**: ทุก 10 วินาที (~360 requests/ชั่วโมง)
- ยอดรวมทั้งหมดประมาณ **~840 requests/ชั่วโมง** ซึ่งอยู่ต่ำกว่าขีดจำกัด 5,000 ครั้งอย่างปลอดภัย ใช้งานได้ต่อเนื่องตลอด 24 ชั่วโมง
