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
#define GITHUB_REPO     "Server-ESP32"            // ชื่อ Repository (ใช้ขีดกลาง -)
#define GITHUB_BRANCH   "main"                    // Branch ที่ต้องการเชื่อมต่อ
#define GITHUB_FILE_PATH "data/state.json"  // พาธไฟล์ state บน repo

// GitHub Personal Access Token (PAT) ที่มีสิทธิ์ repo หรือ Contents: Read and write
// แนะนำ: ใช้ Fine-grained Token เพื่อความปลอดภัยสูงสุด
#define GITHUB_TOKEN    "ghp_YOUR_PERSONAL_ACCESS_TOKEN_HERE"

// =============================================================================
// การกำหนดขาต่อฮาร์ดแวร์ (GPIO Pins)
// =============================================================================
#define PIN_RELAY_1     4     // ขาควบคุม Relay 1
#define PIN_RELAY_2     5     // ขาควบคุม Relay 2
#define PIN_LED         2     // ขาไฟ LED บนบอร์ด ESP32 (Built-in LED ส่วนใหญ่คือ GPIO 2)
#define PIN_DHT_SENSOR  15    // ขาต่อเซนเซอร์อุณหภูมิและความชื้น (ถ้ามี)

// รีเลย์ทำงานแบบ Active LOW หรือ Active HIGH (บอร์ดรีเลย์ส่วนใหญ่เป็น Active LOW = ส่ง LOW คือเปิด)
#define RELAY_ACTIVE_LOW true

// =============================================================================
// รอบเวลาการทำงาน (มิลลิวินาที)
// =============================================================================
// ดึงคำสั่งใหม่จาก GitHub ทุกๆ 10 วินาที
#define POLL_COMMANDS_INTERVAL_MS   10000 

// ส่งข้อมูลเซนเซอร์และสถานะ (Telemetry) ขึ้นไปอัปเดตบน GitHub ทุกๆ 30 วินาที
// (แนะนำ 30-60 วินาที เพื่อประหยัด GitHub API Rate Limit และไม่สร้าง commit ถี่เกินไป)
#define PUSH_TELEMETRY_INTERVAL_MS  30000 

#endif // CONFIG_H
