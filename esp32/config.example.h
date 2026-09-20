#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// การตั้งค่าเครือข่าย WiFi
// =============================================================================
#define WIFI_SSID       "YOUR_WIFI_SSID"          // ชื่อ WiFi ของคุณ
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"      // รหัสผ่าน WiFi

// =============================================================================
// การตั้งค่า GitHub Repository & API
// =============================================================================
#define GITHUB_OWNER    "YOUR_GITHUB_USERNAME"    // ชื่อผู้ใช้ GitHub เจ้าของ Repo
#define GITHUB_REPO     "Server-ESP32"            // ชื่อ Repository
#define GITHUB_BRANCH   "main"                    // Branch ที่ต้องการเชื่อมต่อ
#define GITHUB_FILE_PATH "data/state.json"        // พาธไฟล์ state บน repo

// GitHub Personal Access Token (PAT) ที่มีสิทธิ์ repo หรือ Contents: Read and write
// แนะนำ: ใช้ Fine-grained Token เพื่อความปลอดภัยสูงสุด
#define GITHUB_TOKEN    "ghp_YOUR_PERSONAL_ACCESS_TOKEN_HERE"

// =============================================================================
// การกำหนดขาต่อฮาร์ดแวร์ (GPIO Pins) - บอร์ด ESP32-Relay-X4 (4 ช่อง)
// =============================================================================
#define PIN_RELAY_1     32    // ขาควบคุม Relay 1 (K1)
#define PIN_RELAY_2     33    // ขาควบคุม Relay 2 (K2)
#define PIN_RELAY_3     25    // ขาควบคุม Relay 3 (K3)
#define PIN_RELAY_4     26    // ขาควบคุม Relay 4 (K4)
#define PIN_LED         2     // ขาไฟ LED บนบอร์ด ESP32 (Built-in LED)
#define PIN_DHT_SENSOR  15    // ขาต่อเซนเซอร์อุณหภูมิและความชื้น (เตรียมไว้ในอนาคต)

// ระดับสัญญาณควบคุมรีเลย์ (บอร์ดนี้เป็นแบบ Active HIGH: ส่ง HIGH คือเปิด, LOW คือปิด)
#define RELAY_ACTIVE_LOW false

// =============================================================================
// รอบเวลาการทำงาน & การส่งข้อมูล (มิลลิวินาที)
// =============================================================================
// ดึงคำสั่งใหม่จาก GitHub ทุกๆ 2 วินาที
#define POLL_COMMANDS_INTERVAL_MS   2000 

// เปิดการส่ง Heartbeat เพื่อให้หน้าเว็บ Dashboard ทราบสถานะออนไลน์ของ ESP32
#define ENABLE_AUTO_TELEMETRY_PUSH  true

// ส่งข้อมูลรายงานตัว Heartbeat & Telemetry ทุกๆ 45 วินาที
#define PUSH_TELEMETRY_INTERVAL_MS  45000 

#endif // CONFIG_H
