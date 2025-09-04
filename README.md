# FTDI + SDL2 + Nuklear (C, SRP structure)

โครงโปรเจกต์ตัวอย่างสำหรับทำ GUI (SDL2 + Nuklear) ที่คุยกับ FTDI ได้ โดยแยกความรับผิดชอบชัดเจน (Single Responsibility Principle: SRP)

## โครงสร้าง
```
.
├── CMakeLists.txt
├── README.md
├── include
│   ├── common.h
│   ├── ftdi.h
│   ├── ringbuffer.h
│   ├── ui.h
│   └── worker.h
├── src
│   ├── ftdi_d2xx.c       (ไดรเวอร์ FTDI D2XX - ใช้เมื่อ -DUSE_D2XX และมี ftd2xx.h)
│   ├── ftdi_stub.c       (ไดรเวอร์จำลอง ใช้ทดสอบโดยไม่ต่ออุปกรณ์จริง)
│   ├── main.c            (entry point + SDL2/Nuklear init + main loop)
│   ├── ringbuffer.c
│   ├── ui.c
│   └── worker.c
└── third_party
    └── (ใส่ไฟล์ของ Nuklear ที่นี่)
```

> หมายเหตุ: โค้ดนี้ตั้งใจให้ *คอมไพล์ได้จริง* เมื่อคุณเตรียม dependency พร้อม โดยเฉพาะ **Nuklear** backend ของ SDL+OpenGL และ (ตัวเลือก) **FTDI D2XX SDK**

## Dependency
- **SDL2** development headers & libs
- **OpenGL** (สำหรับเรนเดอร์ด้วย `nuklear_sdl_gl3.h`)
- **Nuklear**: ดาวน์โหลดจาก repo (MIT) แล้ววางไว้ใน `third_party/`
  - `third_party/nuklear.h`
  - `third_party/nuklear_sdl_gl3.h` (ไฟล์ backend จากตัวอย่างของ Nuklear)
- **(ตัวเลือก) FTDI D2XX** (Windows เท่านั้น หากต้องการต่ออุปกรณ์จริงด้วย D2XX)
  - ต้องมี `ftd2xx.h`, `ftd2xx.lib`, และ `ftd2xx.dll`

> ถ้าคุณยังไม่มีอุปกรณ์จริง ให้เริ่มด้วย **ไดรเวอร์จำลอง (stub)** ก่อน (`ftdi_stub.c`) ซึ่งจะสร้างแพ็กเก็ตทดสอบให้ UI แสดงผลได้

## วิธีคอมไพล์ (CMake)
```bash
# Linux/macOS (ตัวอย่าง)
mkdir build && cd build
cmake -DUSE_STUB=ON -DUSE_D2XX=OFF ..
cmake --build .

# Windows (MSVC, ใช้ x64 Native Tools Command Prompt)
mkdir build && cd build
cmake -A x64 -DUSE_STUB=ON -DUSE_D2XX=OFF ..
cmake --build . --config Release
```

> ถ้าต้องการใช้ D2XX (Windows) ให้กำหนด `-DUSE_D2XX=ON` และชี้ตัวแปร `D2XX_INCLUDE_DIR`, `D2XX_LIBRARY` หาก CMake หาไม่เจอ

## รันโปรแกรม
- รัน executable ที่ได้ แล้วจะเห็นหน้าต่าง GUI
- ปุ่ม **Connect** จะเชื่อมกับไดรเวอร์ที่เลือก (stub/d2xx)
- ช่อง **Input** ให้พิมพ์ข้อความแล้วกด **Send** เพื่อส่งไปที่ driver
- หน้าต่าง **Console** แสดงไบต์ที่รับ (hex และ ascii) + สถิติจำนวน bytes RX/TX

## สถาปัตยกรรม (SRP)
- `ftdi.h` + `ftdi_*.c`: abstraction ของการคุยกับอุปกรณ์ (เปิด/ปิด/อ่าน/เขียน/ตั้งค่า)
- `worker.[ch]`: จัดการ thread I/O (ห้ามบล็อก UI)
- `ringbuffer.[ch]`: เก็บข้อมูลขาเข้าจาก worker อย่างปลอดภัย (mutex)
- `ui.[ch]`: จัดวาดหน้าจอ/อินพุต/ปุ่ม โดยดึง/ผลักข้อมูลผ่าน interface ที่กำหนด
- `main.c`: bootstrap SDL2 + Nuklear + loop

## หมายเหตุเรื่อง FTDI
- ถ้าคุณใช้ **FT232/FT2232 (USB↔UART)**: แนะนำใช้ **D2XX** สำหรับ Windows หรือ libftdi สำหรับ Linux/macOS (ไม่ได้ใส่ตัวอย่าง libftdi ในโปรเจกต์นี้เพื่อให้กระชับ)
- ถ้าคุณใช้ **FT600/FT601 (USB3 FIFO)**: ต้องใช้ **D3XX** หรือ libusb bulk (ไม่ได้รวมใน skeleton นี้ แต่แยกไฟล์/โมดูลคล้าย `ftdi_d2xx.c` ได้)

## งานต่อยอด
- เพิ่ม driver `ftdi_libusb.c` หรือ `ftdi_d3xx.c`
- เพิ่มกราฟสัญญาณ (ผ่าน SDL2 renderer หรือ OpenGL) เช่นแสดงค่าแบบสโคป
- บันทึก log เป็นไฟล์ `.bin` หรือ `.pcap`
