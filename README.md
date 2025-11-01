# OBD-II K-Line Reader / Simulator (ESP32)

เครื่องมืออ่าน/จำลอง OBD-II ผ่าน K-Line ด้วย ESP32: โฟกัสการสื่อสารแบบ ISO 9141-2 และ ISO 14230-4 (KWP2000) รองรับการดูข้อมูลสด บันทึกข้อมูล และเชื่อมต่อได้หลายวิธี

# Website

[GitHub Pages](./Web).

## Features

### Phase 1 – Core K-Line
- **K-Line Protocol I/O**: รองรับ ISO 9141-2 และ ISO 14230-4 (KWP2000) รวมถึงรูปแบบเฉพาะ Honda
- **Initialization Modes**: 5-baud init, Fast-init (KWP2000), และ Honda-style init
- **ESP32 UART Driver**: 10400 baud, inverted logic, จัดการ inter-byte timeout/byte spacing
- **Auto Checksum & Echo Handling**: คำนวณเช็กซัมอัตโนมัติ เคลียร์ echo หลังส่งเฟรม
- **Real-time Dashboard (พื้นฐาน)**: แสดงข้อมูลสด (ตัวอย่าง gauge/log)

> หมายเหตุ: ตัด ISO 15765-4 (CAN) ออก

> หมายเหตุ: ตัด การ fastInit ออก

### Phase 2 – Advanced
- **DTC Viewer**: อ่าน/แสดงรหัสปัญหา (Diagnostic Trouble Codes)
- **Data Logging**: บันทึก CSV/JSON/ไบนารี (พร้อม timestamp)
- **Connection Manager**: สลับ/จัดการแหล่งเชื่อมต่อ (ESP32 UART / Wi-Fi / FTDI)

### Phase 3 – Polish & Optimization
- **Export/Import**: ส่งออก CSV/JSON/HTML/PDF และนำเข้าไฟล์บันทึก
- **Settings**: ปรับตั้งค่าโปรโตคอล/timeout/baud 

## Prerequisites

### ฮาร์ดแวร์
- **ESP32** ที่มี UART ฮาร์ดแวร์ (เช่น GPIO16=RX, GPIO17=TX)
- **K-Line Transceiver** (เช่น L9637D / MC33660 / SI9243) สำหรับแปลงระดับ 3.3 V ↔ 12 V
- แหล่งจ่าย: 12 V → 5 V ด้วยเรกูเลเตอร์ที่เหมาะสม

![A1 Poster Image..](https://github.com/1REDfriend/Physical_compute_Project_1/blob/main/ECU%20HACKING.png)
