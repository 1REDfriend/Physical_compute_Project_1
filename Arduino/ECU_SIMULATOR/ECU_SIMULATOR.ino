#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define btn 8
#define torque A0
#define l1 4

#define tx 1
#define rx 0

#define maxRpm 12000
#define minRpm 800

const float IAT_HEAT_RATE = 1.2f;
const float IAT_COOL_RATE = 0.3f;
const float ECT_HEAT_RATE = 0.6f;
const float ECT_COOL_RATE = 0.4f;

const int SPEED_UP_RATE = 3;
const int SPEED_DOWN_RATE = 7;

bool start = false;
int mode = 0;
int speed = 0;
int rpm = 0;
int tps = 0;
int mbar = 0;

float batt = 12; // volt

int rpmTemp = 0;
int speedRpmNow = 0;

float temp_iat = 30; // ท่อไอดี / MAF
float temp_ect = 30; // เสื้อน้ำ / ใกล้เทอร์โมสแตท

int ignitionTiming = 0;
int ignitionDeg = 0;

int lcdTime = 0;
int lcdRate = 100;

long timeSpeed = millis();
long timeNow = 0;
long ingnitionWait = 0;

// ------------------------- ECU Variable -------------------------
bool wakeup = false;
bool isInit = false;

int sleepTimeMs = 5000;
long ecuTimeoutTimeMs = millis();
int interByteMs = 5;
int interByteTimeoutMs = 50;
int interByteTimeMs = millis();
int toleranceMs = 5;

int LOW_TIME = 70;
int HIGH_TIME = 120;

int pattern_timeout_ms = 500;

enum PatternState {
  IDLE_WAIT_HIGH,
  GOT_FIRST_HIGH,
  GOT_LOW,
};

PatternState pstate = IDLE_WAIT_HIGH;
int lastLevel = -1;
unsigned long edgeMicros = 0;          // เวลา edge ล่าสุด (us)
unsigned long lastActivityMillis = 0;  // ใช้ไว้ reset ถ้านิ่งนาน
unsigned long accHighMs = 0;           // ระยะเวลาช่วง HIGH ที่เพิ่งจบ
unsigned long accLowMs  = 0;           // ระยะเวลาช่วง LOW ที่เพิ่งจบ
unsigned long lastLowMs  = 0;

static uint8_t rxBuf[128];
static size_t  rxLen = 0;
static unsigned long lastByteMs = 0;
static unsigned long ecuWakeStartMs = 0;

static inline float fmap(float x, float inMin, float inMax, float outMin, float outMax) {
  return (float)outMin + (float)(outMax - outMin) * (float)(x - inMin) / (float)(inMax - inMin);
}

void tempSimulator() {
   unsigned long now = millis();
  int tempOverhual[2] = {30, 120};
  float tempMap = fmap(rpm, minRpm, maxRpm, tempOverhual[0], tempOverhual[1]);

  if ( temp_iat < tempMap) {
    temp_iat = min(tempOverhual[1], temp_iat + IAT_HEAT_RATE);
    rpmTemp = rpm;
  }

  if (temp_iat > tempMap) {
    temp_iat = max(tempOverhual[0], temp_iat - IAT_COOL_RATE);
    rpmTemp = rpm;
  }

  if (temp_ect < tempMap) {
    temp_ect = min(tempOverhual[1], temp_ect + ECT_HEAT_RATE);
    rpmTemp = rpm;
  }

  if (temp_ect > tempMap) {
    temp_ect = max(tempOverhual[0], temp_ect - ECT_COOL_RATE);
    rpmTemp = rpm;
  }
}

void speedSimulator() {
  unsigned long tsnow = millis(); // time speed
  int speedOverhual[2] = {0, 150};
  float speedMap = fmap(rpm, minRpm, maxRpm, speedOverhual[0], speedOverhual[1]);
  float speedCal = (rpm - speedRpmNow)/1000.0f;

  if (speed < speedMap) {
    speed = min(speedOverhual[1], speed + (speedCal > SPEED_UP_RATE ? speedCal : SPEED_UP_RATE));
  }

  if (speed > speedMap) {
    speed = max(speedOverhual[0], speed - (speedCal > SPEED_DOWN_RATE ? speedCal : SPEED_DOWN_RATE));
  }

  if (tsnow > timeSpeed + 1000) {
    speedRpmNow = rpm;
    timeSpeed = tsnow;
  }
}

// ------------------------- ECU COMMUNICATION ------------------------- 

static inline uint8_t kwp_checksum(const uint8_t* d, size_t n) {
  uint16_t s = 0;
  for (size_t i = 0; i < n; ++i) s += d[i];
  return (uint8_t)(0x100 - (s & 0xFF));
}

static inline bool checksum_complete_frame(const uint8_t* d, size_t n) {
  if (n < 2) return false;               // น้อยไปยังตรวจไม่ได้
  uint8_t expect = kwp_checksum(d, n-1); // คำนวณจากทุกตัว "ยกเว้น" ตัวสุดท้าย
  return d[n-1] == expect;
}

static inline bool within(int val, int target, int tol) {
  return (val >= target - tol) && (val <= target + tol);
}

static bool send_response(const uint8_t* d, size_t cap) {
  if (!d || cap < 3) return false;                  // [addr,len,data...]

  uint8_t byteData = d[1];                          // จำนวนไบต์ใน DATA
  size_t n_wo_cs = (size_t)2 + (size_t)byteData;    // addr + len + data(L)
  if (n_wo_cs + 1 > cap) return false;              // ต้องมีพื้นที่พอสำหรับอ่านและต่อ checksum

  uint8_t cs = kwp_checksum(d, n_wo_cs);

  Serial.write(d, n_wo_cs);
  Serial.write(&cs, 1);
  Serial.flush();
  return true;
}

void ECU_COMM() {
  int level = digitalRead(rx);
  unsigned long ecuMicros = micros();
  unsigned long ecuMs = millis();

  if (!wakeup) {
    if (millis() - lastActivityMillis > pattern_timeout_ms) {
      pstate = IDLE_WAIT_HIGH;
      accHighMs = accLowMs = 0;
    }

    if (level != lastLevel) {
      unsigned long dur_us = ecuMicros - edgeMicros;
      unsigned long dur_ms = dur_us / 1000UL;
      edgeMicros = ecuMicros;
      lastActivityMillis = millis();

      if (lastLevel == HIGH) {
        accHighMs = dur_ms;

        if (pstate == IDLE_WAIT_HIGH) {
          if ((int)accHighMs >= 100) {
            pstate = GOT_FIRST_HIGH; 
          } else {
            pstate = IDLE_WAIT_HIGH;
          }
          
        } else if (pstate == GOT_LOW) {
          if (within((int)accHighMs, 120 , toleranceMs)) {
            wakeup = true;
            ecuTimeoutTimeMs = millis();
          }

          pstate = IDLE_WAIT_HIGH;
          accHighMs = accLowMs = 0;
        }

      } else {
        accLowMs = dur_ms;

        if (pstate == GOT_FIRST_HIGH) {
          if (within((int)accLowMs, 70, toleranceMs)) {
            pstate = GOT_LOW;
          } else {
            pstate = IDLE_WAIT_HIGH;
            accHighMs = accLowMs = 0;
          }
        }
      }

      lastLevel = level;
    }
  } else {
    if (ecuWakeStartMs == 0) ecuWakeStartMs = ecuMs;

    if (Serial.available() > 0) {
      int v = Serial.read();
      if (v > 0) {
        if (rxLen < sizeof(rxBuf)) {
          rxBuf[rxLen++] = (uint8_t)v;
          lastByteMs = ecuMs;
        } else {
          rxLen = 0;
        }
      }
    }

    if ((rxLen > 0 && (ecuMs - lastByteMs) >= interByteTimeoutMs) || checksum_complete_frame(rxBuf, rxLen)) {
      bool ok = checksum_complete_frame(rxBuf, rxLen);

      if (ok) {
        static const uint8_t FE_REQ[] = {0xFE, 0x04, 0x72, 0x8C};
        if (rxLen == sizeof(FE_REQ) && memcmp(rxBuf, FE_REQ, sizeof(FE_REQ)) == 0) {
          static const uint8_t RESP[] = {0x0E, 0x04, 0x72};
          send_response(RESP, sizeof(RESP));
          isInit = true;
          ecuWakeStartMs = ecuMs;
        }

        static const uint8_t LIVE_DATA_REQ[] = {0x72, 0x05, 0x71, 0x17, 0x01};
        if (isInit) {
          if (rxLen == sizeof(LIVE_DATA_REQ) && memcmp(rxBuf, LIVE_DATA_REQ, sizeof(LIVE_DATA_REQ)) == 0) {
            uint8_t RESP[] = {0x02, 0x18, 0x71, 0x17};
            uint8_t payload[16];

            auto u8 = [](int v) -> uint8_t { return (uint8_t)constrain(v, 0, 255); };
            auto u8f = [&](float v) -> uint8_t { return u8((int)lroundf(v)); };

            memset(payload, 0xFF, 16);

            uint16_t rpm16 = (uint16_t)constrain((int)rpm, 0, 65535);

            payload[0] = (rpm >> 8) & 0xFF;   // ไบต์สูง (MSB)
            payload[1] = rpm & 0xFF;
            payload[3] = u8(tps / 5.0f * 256);
            payload[4] = u8f((ignitionDeg * 2) + 64.0f);
            payload[5] = u8f(temp_iat + 40);
            payload[7] = u8f(temp_ect + 40);
            payload[9] = u8f(mbar / 10.0f);
            payload[10] = u8f(batt * 10.0f);
            payload[16] = u8(speed);

            for (int i = 0; i < 16; i ++) {
              RESP[i + 4] = payload[i];
            }
            
            send_response(RESP, sizeof(RESP));
            isInit = true;
            ecuWakeStartMs = ecuMs;
          }
        }
      }
      rxLen = 0;
      lastByteMs = 0;
    }

    if (ecuMs - ecuWakeStartMs >= sleepTimeMs) {
      wakeup = false;
      ecuWakeStartMs = 0;
      rxLen = 0;
      lastByteMs = 0;
    }
  }
}

// ------------------------- SETUP & LOOP ZONE ------------------------- 

void setup() {
  Serial.begin(10400); // ISO14230 Honda ECU COMUNICATION RATE

  pinMode(btn , INPUT_PULLUP);
  pinMode(l1 , OUTPUT);

  pinMode(tx, OUTPUT);
  pinMode(rx, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("PRESS B TO START");

}

void loop() {
  timeNow = millis();
  if (digitalRead(btn) == LOW && !start) {
    start = true;
    lcd.clear();

    batt -= 5;

    delay(300);

    ingnitionWait = timeNow;
    lcdTime = timeNow;
    rpmTemp = rpm;
    speedRpmNow = rpm;

    batt += 4.5;
  } else if (start && digitalRead(btn) == LOW) {
    mode = (mode + 1) % 2;
    delay(300);
  }

  if (start) {
    if (timeNow > lcdTime + lcdRate) {
      if (mode == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RPM : " + (String)rpm);
        lcd.setCursor(0, 1);
        lcd.print("SPEED : " + (String)speed + " KM/H");
      }else if (mode == 1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("temp IAT : " + (String)(int)temp_iat + " C");
        lcd.setCursor(0, 1);
        lcd.print("temp ECT : " + (String)(int)temp_ect + " C");
      }

      tempSimulator();
      speedSimulator();
      ECU_COMM();
      
      lcdTime = timeNow;
    }
    
    if (timeNow > ingnitionWait + ignitionTiming && digitalRead(l1) == LOW) {
      digitalWrite(l1 , HIGH);
      ingnitionWait = timeNow;
    }

    if (digitalRead(l1) == HIGH && timeNow > ingnitionWait + 30) {
      digitalWrite(l1 , LOW);
      ingnitionWait = timeNow;
    }
  }

  int torqueB = analogRead(torque);

  rpm = (int)map(torqueB ,0, 1023, minRpm, maxRpm);
  ignitionTiming = (int)map(torqueB ,0, 1023, 150, 0);
  tps = (int)map(torqueB ,0, 1023, 0, 100);
  ignitionDeg = (int)map(torqueB ,0, 1023, 17, 60);
  mbar = (int)map(torqueB ,0, 1023, 15, 30);
}
