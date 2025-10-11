#include <Arduino.h>
#include <WiFi.h>
#include <stdarg.h>  // for va_list

// ===================================================================================
// Configuration
// ===================================================================================

// -------- Wi-Fi AP config --------
static const char *AP_SSID = "ESP32-OBD";
static const char *AP_PASS = "12345678";  // >= 8 chars
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const uint16_t TCP_PORT = 3333;

// -------- K-Line & Hardware Pins --------
static const int K_TX_PIN = 17;       // ESP32 TX -> Transceiver TXD
static const int K_RX_PIN = 16;       // ESP32 RX <- Transceiver RXD
static const int STATUS_LED_PIN = 2;  // Onboard LED for status indication
HardwareSerial KLine(1);              // Use UART1 on ESP32

// -------- KWP2000 Timing --------
static const uint32_t K_BAUD = 10400;          // ISO9141/14230 common rate
static const uint32_t REQ_GAP_MS = 55;         // P2: Min time between requests
static const uint32_t BYTE_TIMEOUT_MS = 200;   // P3: Timeout for single byte reception
static const uint32_t FRAME_TIMEOUT_MS = 400;  // Overall frame guard timeout
static const uint32_t IDLE_GAP_MS = 55;        // P1: Inter-byte timeout to detect end-of-frame

// ===================================================================================
// Multi-Core & Shared Data Management
// ===================================================================================
TaskHandle_t kline_task_handle;  // Task Handle for the K-Line Task
SemaphoreHandle_t log_mutex;     // Mutex to protect the shared log buffer
char shared_log_buffer[256];     // Shared buffer for messages between cores

// ===================================================================================
// Global State
// ===================================================================================

WiFiServer obdServer(TCP_PORT);
WiFiClient clients[4];  // up to 4 telnet/CLI readers

// ----- Keep-alive & re-init policy -----
static uint32_t last_keepalive_ms = 0;
static uint8_t no_reply_count = 0;

// ----- ECU Header Discovery -----
const uint8_t KWP_HEADERS[][3] = {
    // --- Honda Specific ---
    { 0xC1, 0x72, 0xF1 },  // Honda: Custom Diagnostics (PGM-FI)
    { 0xC1, 0x7E, 0xF1 },  // Honda: Custom Flash/Reprogram

    // --- Standard OBD-II / Generic ---
    { 0xC1, 0x33, 0xF1 },  // Standard: OBD-II Generic ECU (most common)
    { 0x81, 0x33, 0xF1 },  // Standard: OBD-II Generic ECU (with length byte)
    { 0xC1, 0x10, 0xF1 },  // Standard: Generic Engine ECU #1
    { 0xC1, 0x11, 0xF1 },  // Standard: Generic Engine ECU #2 (DA=0x11)
    { 0xC1, 0x18, 0xF1 },  // Standard: Generic Gearbox/Transmission ECU

    // --- Volkswagen Group (VW, Audi, Skoda, Seat) ---
    { 0x68, 0x6A, 0xF1 },  // VW ISO 9141-2: KW1281 Protocol (older models)
    { 0x80, 0x01, 0xF1 },  // VW KWP2000: Engine
    { 0x80, 0x02, 0xF1 },  // VW KWP2000: Auto Transmission
    { 0x80, 0x17, 0xF1 },  // VW KWP2000: Instrument Cluster
    { 0x80, 0x25, 0xF1 },  // VW KWP2000: Immobilizer
    { 0x80, 0x56, 0xF1 },  // VW KWP2000: Radio

    // --- Suzuki / Kawasaki (Motorcycles) ---
    { 0x80, 0x12, 0xF1 },  // Suzuki: Engine ECU (SDL Mode 1)
    { 0x81, 0x12, 0xF1 },  // Suzuki: Engine ECU (SDL Mode 2, with length)
    { 0x80, 0x11, 0xF1 },  // Kawasaki: Engine ECU

    // --- Fiat / Alfa Romeo ---
    { 0xC1, 0x11, 0xF1 },  // Fiat/Alfa: Engine Management (Marelli/Bosch)
    { 0xC1, 0x42, 0xF1 },  // Fiat/Alfa: ABS System

    // --- Other Common Headers ---
    { 0x68, 0x11, 0xF1 },  // ISO 9141-2: Common Engine ECU
    { 0xC2, 0x33, 0xF1 }   // Standard: Physical addressing with 2-byte header
};
const int NUM_HEADERS = sizeof(KWP_HEADERS) / sizeof(KWP_HEADERS[0]);
uint8_t active_header[3];
int current_header_index = 0;
bool ecu_has_responded = false;


// ===================================================================================
// Networking & Logging (Thread-Safe)
// ===================================================================================

// This function is only called by the main loop (Core 0), so it's safe.
static void net_broadcast(const char *s) {
  for (int i = 0; i < 4; ++i) {
    if (clients[i] && clients[i].connected())
      clients[i].print(s);
  }
}

// This function is now thread-safe. It writes to Serial and the shared buffer.
static void logf(const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  // ตรวจสอบว่ามีที่ว่างใน Buffer พอที่จะเขียนหรือไม่
  if (Serial.availableForWrite() > strlen(buf)) {
    Serial.print(buf);
  } else {
    // ถ้า Buffer ใกล้เต็ม อาจจะแค่ print จุดเพื่อบอกว่ามี Log ที่ถูกข้ามไป
    Serial.print(".");
  }

  // Safely write to the shared buffer using the mutex
  if (log_mutex != NULL && xSemaphoreTake(log_mutex, portMAX_DELAY) == pdTRUE) {
    strncpy(shared_log_buffer, buf, sizeof(shared_log_buffer) - 1);
    xSemaphoreGive(log_mutex);
  }

  vTaskDelay(pdMS_TO_TICKS(1));
}

void net_begin_ap() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);
  obdServer.begin();
  obdServer.setNoDelay(true);
}

// This function is only called by the main loop (Core 0), so it's safe.
void net_poll_clients() {
  if (obdServer.hasClient()) {
    WiFiClient c = obdServer.available();
    if (c) {
      bool placed = false;
      for (int i = 0; i < 4; ++i) {
        if (!clients[i] || !clients[i].connected()) {
          clients[i].stop();
          clients[i] = c;
          placed = true;
          break;
        }
      }
      if (!placed)
        c.stop();
    }
  }
  for (int i = 0; i < 4; ++i) {
    if (clients[i] && !clients[i].connected())
      clients[i].stop();
  }
}


// ===================================================================================
// K-Line Core Communication & OBD-II (These functions don't change)
// ===================================================================================

void kline_fast_init_pulse() {
  KLine.end();
  logf("Fast init...\n");
  digitalWrite(K_TX_PIN, HIGH);
  delay(300);
  digitalWrite(K_TX_PIN, LOW);
  delay(70);
  digitalWrite(K_TX_PIN, HIGH);
  delay(120);
  KLine.begin(K_BAUD, SERIAL_8N1, K_RX_PIN, K_TX_PIN);
  clear_rx_buffer();
}

static uint8_t checksum8(const uint8_t *buf, size_t n_without_cs) {
  uint16_t sum = 0;
  for (size_t i = 0; i < n_without_cs; ++i)
    sum += buf[i];
  return (uint8_t)(0x100 - (sum & 0xFF));
}

static bool verify_checksum(const uint8_t *buf, size_t n_with_cs) {
  if (n_with_cs < 2) return false;
  uint8_t cs = buf[n_with_cs - 1];
  return checksum8(buf, n_with_cs - 1) == cs;
}

void clear_rx_buffer() {
  while (KLine.available()) { (void)KLine.read(); }
}

size_t k_read_frame(uint8_t *out, size_t max_len) {
  size_t n = 0;
  uint32_t t_start = millis();
  uint32_t last_rx = millis();
  bool got_any = false;

  while ((millis() - t_start) < FRAME_TIMEOUT_MS) {
    while (KLine.available()) {
      if (n < max_len) out[n++] = (uint8_t)KLine.read();
      last_rx = millis();
      got_any = true;
    }
    if (got_any && (millis() - last_rx) > IDLE_GAP_MS) break;
    vTaskDelay(pdMS_TO_TICKS(1));  // Small delay to prevent starving other tasks on the same core
  }
  return n;
}

void k_send_req(uint8_t svc, uint8_t pid) {
  clear_rx_buffer();
  uint8_t f[6];
  f[0] = active_header[0];
  f[1] = active_header[1];
  f[2] = active_header[2];
  f[3] = svc;
  f[4] = pid;
  f[5] = checksum8(f, 5);
  KLine.write(f, sizeof(f));
  KLine.flush();
}

inline void mark_rx_ok() {
  no_reply_count = 0;
  if (!ecu_has_responded) {
    ecu_has_responded = true;
    logf("\n*** SUCCESS: ECU Responded with Header {0x%02X, 0x%02X, 0x%02X}. Locking this header. ***\n\n",
         active_header[0], active_header[1], active_header[2]);
  }
}

inline void mark_rx_fail() {
  no_reply_count++;
}

bool send_tester_present() {
  k_send_req(0x3E, 0x00);
  vTaskDelay(pdMS_TO_TICKS(REQ_GAP_MS));
  uint8_t buf[32];
  size_t n = k_read_frame(buf, sizeof(buf));
  if (n > 1) {
    mark_rx_ok();
    logf("(n=%zu)\n\n", n);
    return true;
  } else {
    mark_rx_fail();
    return false;
  }
}

void session_maintain() {
  uint32_t now = millis();
  if (now - last_keepalive_ms > 2000) {
    send_tester_present();
    last_keepalive_ms = now;
  }

  if (no_reply_count >= (NUM_HEADERS * 2)) {
    logf("!!! Connection lost. Re-initializing... !!!\n");
    ecu_has_responded = false;
    no_reply_count = 0;
    kline_fast_init_pulse();
  }
}

bool get_supported_block(uint8_t base_pid, uint8_t out_bits[4]) {
  k_send_req(0x01, base_pid);
  vTaskDelay(pdMS_TO_TICKS(REQ_GAP_MS));

  if (ESP.getFreeHeap() < sizeof(uint8_t) + sizeof(size_t)) {
    logf("Not enough heap memory to proceed!");
    return false;
  }

  UBaseType_t klineStackRemaining = uxTaskGetStackHighWaterMark(kline_task_handle);
  if (klineStackRemaining < 300) {
    logf("Not enough stack memory to proceed!");
    return false;
  }

  uint8_t buf[64];
  size_t n = k_read_frame(buf, sizeof(buf));

  if (n < 7) {
    logf("PID 0x%02X: No/short reply (n=%u)\n", base_pid, (unsigned)n);
    mark_rx_fail();
    return false;
  }
  if (buf[3] == 0x7F && buf[4] == 0x01) {
    logf("PID 0x%02X: NRC 0x%02X\n", base_pid, buf[5]);
    mark_rx_fail();
    return false;
  }
  if (!verify_checksum(buf, n)) { logf("PID 0x%02X: Checksum mismatch!\n", base_pid); }

  int idx = -1;
  for (size_t i = 0; i + 1 < n; i++) {
    if (buf[i] == 0x41 && buf[i + 1] == base_pid) {
      idx = (int)i;
      break;
    }
  }
  if (idx < 0) {
    for (size_t i = 0; i + 4 < n; i++) {
      if (buf[i + 3] == 0x41 && buf[i + 4] == base_pid) {
        idx = (int)i + 3;
        break;
      }
    }
  }
  if (idx < 0 || (idx + 2 + 4 > n)) {
    logf("PID 0x%02X: Unrecognized reply format.\n", base_pid);
    mark_rx_fail();
    return false;
  }

  mark_rx_ok();
  memcpy(out_bits, &buf[idx + 2], 4);
  return true;
}


// ===================================================================================
// K-Line Task (This runs on Core 1)
// ===================================================================================
void kline_task_code(void *pvParameters) {
  logf("K-Line Task started on Core %d\n", xPortGetCoreID());

  // Initial K-Line setup
  memcpy(active_header, KWP_HEADERS[current_header_index], 3);
  kline_fast_init_pulse();
  logf("Starting ECU discovery...\n");

  // This is the infinite loop for the K-Line task
  for (;;) {
  
    session_maintain();

    if (!ecu_has_responded) {
      // --- DISCOVERY PHASE ---
      digitalWrite(STATUS_LED_PIN, HIGH);
      logf("Searching... Trying Header #%d: {0x%02X, 0x%02X, 0x%02X}\n",
           current_header_index, active_header[0], active_header[1], active_header[2]);

      uint8_t dummy_bits[4];
      get_supported_block(0x00, dummy_bits);

      if (!ecu_has_responded) {
        current_header_index = (current_header_index + 1) % NUM_HEADERS;
        memcpy(active_header, KWP_HEADERS[current_header_index], 3);
        logf("No response. Moving to next header.\n");
      }
      digitalWrite(STATUS_LED_PIN, LOW);
      // vTaskDelay(pdMS_TO_TICKS(400));
    } else {
      // --- SCANNING PHASE ---
      const uint8_t bases[] = { 0x00, 0x20, 0x40 };
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

      for (uint8_t b : bases) {
        uint8_t bits[4] = { 0 };
        if (!get_supported_block(b, bits)) {
          logf("Base 0x%02X: No supported map (skip)\n", b);
          continue;
        }
        logf("Supported map for 0x%02X-0x%02X: %02X %02X %02X %02X\n",
             (unsigned)b + 1, (unsigned)b + 0x20, bits[0], bits[1], bits[2], bits[3]);

        for (int i = 0; i < 32; i++) {
          if ((bits[i / 8] >> (7 - (i % 8))) & 1) {
            uint8_t pid = b + 1 + i;
            if (pid == b || pid == (uint8_t)(b + 0x20)) continue;
            logf("-> Found supported PID: 0x%02X\n", pid);
            vTaskDelay(pdMS_TO_TICKS(REQ_GAP_MS));
          }
        }
      }
      // logf("Scan cycle complete. Sleeping...\n\n");
      // vTaskDelay(pdMS_TO_TICKS(400));
    }
  }
}

// ===================================================================================
// Main Program Flow (setup runs on Core 1, loop runs on Core 0)
// ===================================================================================
void setup() {
  // This part runs on Core 1 by default
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(K_TX_PIN, OUTPUT);
  digitalWrite(K_TX_PIN, HIGH);
  delay(200);

  // Create the mutex before starting any tasks that might use it
  log_mutex = xSemaphoreCreateMutex();

  logf("\nESP32 OBD-II K-Line Scanner - Multi-Core\n");
  logf("Main setup running on Core %d\n", xPortGetCoreID());

  // Start Wi-Fi
  net_begin_ap();
  logf("TCP server on port %d ready.\n", TCP_PORT);

  // Create and pin the K-Line task to Core 1
  xTaskCreatePinnedToCore(
    kline_task_code,     // Task function
    "K-Line Task",       // Name for debugging
    8192,                // Stack size
    NULL,                // Parameters
    1,                   // Priority
    &kline_task_handle,  // Task handle
    1);                  // Pin to Core 1

  logf("K-Line task created and pinned to Core 1.\n");
  logf("Main setup complete. Handing over to Core 0 for networking.\n");

  // After setup(), the Arduino framework will run loop() on the other core (Core 0)
}

void loop() {
  // This loop now runs on Core 0 and acts as our dedicated Network Task.

  // 1. Handle incoming/disconnected clients
  net_poll_clients();

  // FIXED: Implement deadlock prevention logic
  // ------------------------------------------------------------------------
  char local_buffer[256];  // สร้าง buffer ท้องถิ่น
  local_buffer[0] = '\0';  // เคลียร์ buffer ท้องถิ่น

  // 2. Safely check the shared buffer and broadcast any new messages
  if (log_mutex != NULL && xSemaphoreTake(log_mutex, (TickType_t)20) == pdTRUE) {
    if (shared_log_buffer[0] != '\0') {
      strncpy(local_buffer, shared_log_buffer, sizeof(local_buffer) - 1);
      shared_log_buffer[0] = '\0';
    }
    xSemaphoreGive(log_mutex);
  }

  if (local_buffer[0] != '\0') {
    net_broadcast(local_buffer);
  }
  // ------------------------------------------------------------------------

  // 3. Small delay to prevent this loop from running at 100% CPU
  vTaskDelay(pdMS_TO_TICKS(10));
}