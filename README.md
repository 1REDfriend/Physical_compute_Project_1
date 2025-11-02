<div align="center">

  ![C++](https://img.shields.io/badge/C++-00bbff?style=for-the-badge&labelColor=black&logo=c&logoColor=00bbff)
  ![Arduino](https://img.shields.io/badge/Arduiono-0051ff?style=for-the-badge&labelColor=black&logo=arduino&logoColor=0051ff)
  ![esp32](https://img.shields.io/badge/ESP32-ff9900?style=for-the-badge&labelColor=black&logo=x&logoColor=ff9900)
  
</div>

# OBD-II K-Line Reader / Simulator (ESP32)

เครื่องมืออ่าน/จำลอง OBD-II ผ่าน K-Line ด้วย ESP32: โฟกัสการสื่อสารแบบ ISO 9141-2 และ ISO 14230-4 (KWP2000) รองรับการดูข้อมูลสด บันทึกข้อมูล และเชื่อมต่อได้หลายวิธี

## Code Introduction

ตัวโค้ดจะมีอยู่ 2 ส่วนคือ 
1. GetLiveData คือโค้ดของ ESP32 ใช้จำลองการดึงค่าจาก ECU Honda
2. ECU_SIMULATOR คือโค้ดของ Arduino R4 จำลองการเป็น ECU Honda ESP32 จะต้องส่ง Request มาหาเพื่อรับข้อมูล

# Website

<a href="https://1redfriend.github.io/Physical_compute_Project_1/" target="blank">
  <img src="https://img.shields.io/badge/Website-4287f5?style=for-the-badge&logo=google&logoColor=white" alt="website" />
</a>

# Youtube

<a href="https://youtu.be/srPXrsJyhoA" target="blank">
  <img src="https://img.shields.io/badge/Youtube-DC143C?style=for-the-badge&logo=Youtube&logoColor=white" alt="youtube" />
</a>

## Features

### Phase 1 – Core K-Line
- **K-Line Protocol I/O**: รองรับ ISO 9141-2 และ ISO 14230-4 (KWP2000) รวมถึงรูปแบบเฉพาะ Honda
- **Initialization Modes**: 5-baud init, Fast-init (KWP2000), และ Honda-style init
- **ESP32 UART Driver**: 10400 baud, inverted logic, จัดการ inter-byte timeout/byte spacing
- **Auto Checksum & Echo Handling**: คำนวณเช็กซัมอัตโนมัติ เคลียร์ echo หลังส่งเฟรม
- **Real-time Dashboard (พื้นฐาน)**: แสดงข้อมูลสด (ตัวอย่าง gauge/log)

> หมายเหตุ: ตัด ISO 15765-4 (CAN) ออก

> หมายเหตุ: ตัด การ fastInit ออก

<img width="986" height="653" alt="image" src="https://github.com/user-attachments/assets/a034237d-ea39-46a7-8bc9-e904082dddef" />

### Phase 2 – Advanced
- **DTC Viewer**: อ่าน/แสดงรหัสปัญหา (Diagnostic Trouble Codes)
- **Data Logging**: บันทึก CSV/JSON/ไบนารี (พร้อม timestamp)
- **Connection Manager**: สลับ/จัดการแหล่งเชื่อมต่อ (ESP32 UART / Wi-Fi / FTDI)

<img  height="500" alt="image" src="https://github.com/user-attachments/assets/ea617b56-8ae9-40d1-b275-31035307d589" />

### Phase 3 – Polish & Optimization
- **Export/Import**: ส่งออก CSV/JSON/HTML/PDF และนำเข้าไฟล์บันทึก
- **Settings**: ปรับตั้งค่าโปรโตคอล/timeout/baud 

## Prerequisites

### ฮาร์ดแวร์
- **ESP32** ที่มี UART ฮาร์ดแวร์ (เช่น GPIO16=RX, GPIO17=TX)
- **K-Line Optocou0per** (เช่น 4n25 , 4n35) สำหรับแยกระดับ 5 V ↔ 12 V
- แหล่งจ่าย: 12 V → 5 V ด้วยเรกูเลเตอร์ที่เหมาะสม

<img width="1173" height="532" alt="image" src="https://github.com/user-attachments/assets/463bee1e-5bf3-4d15-ad9a-7aa2faa41db7" />

![A1 Poster Image..](https://github.com/1REDfriend/Physical_compute_Project_1/blob/main/ECU%20HACKING.png)
