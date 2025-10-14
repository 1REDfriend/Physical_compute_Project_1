#include <Arduino.h>
#include <WiFi.h>
#include <stdarg.h>  // for va_list
#include <esp_task_wdt.h>
#include "freertos/queue.h"

// ===================================================================================
// Configuration
// ===================================================================================

// -------- Wi-Fi AP config --------
static const char *AP_SSID = "ESP32-OBD";
static const char *AP_PASS = "12345678";
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const uint16_t TCP_PORT = 3333;

// -------- K-Line & Hardware Pins --------
// NOTE: Use a proper K-Line transceiver (e.g., L9637 / MC33290). Do NOT wire directly.
static const int K_TX_PIN = 17;
static const int K_RX_PIN = 16;
static const int STATUS_LED_PIN = 2;
HardwareSerial KLine(1);

// -------- KWP2000 Timing --------
static const uint32_t K_BAUD = 10400;
static const uint32_t BUS_IDLE_MS = 100;
static const uint32_t REQ_GAP_MS = 25;   // pacing between requests (derived from your logs)
static const uint32_t FRAME_TIMEOUT_MS = 50;
static const uint32_t IDLE_GAP_MS  = 25;
static const uint32_t FAST_INIT_MS  = 25;
static const uint32_t FAST_INIT_END_MS  = 0;

// Buffers / limits
static const size_t MAX_FRAME   = 64;
static const size_t LOGBUF_SIZE = 256;

// ===================================================================================
// Multi-Core & Shared Data Management
// ===================================================================================
TaskHandle_t      kline_task_handle = nullptr;
QueueHandle_t     log_queue = nullptr;
// SemaphoreHandle_t log_mutex = nullptr;
// char             shared_log_buffer[LOGBUF_SIZE];

// ===================================================================================
// Global State
// ===================================================================================
WiFiServer obdServer(TCP_PORT);
WiFiClient clients[4];

// ----- Honda Protocol Specifics -----
static const uint8_t HONDA_ECU_ADDR  = 0x72; // DA (request)
static const uint8_t HONDA_TOOL_ADDR = 0x71; // SA (request)
static const uint8_t HONDA_RESP_DA   = 0x02; // DA observed in ECU responses

bool is_comm_started = false;

// ----- Keep-alive & re-init policy -----
static uint8_t no_reply_count = 0;

// ===================================================================================
// Networking & Logging (Thread-Safe)
// ===================================================================================

static void net_broadcast(const char *s) {
  for (int i = 0; i < 4; ++i) {
    if (clients[i] && clients[i].connected())
      clients[i].print(s);
  }
}

static void logf(const char *fmt, ...) {
  char buf[LOGBUF_SIZE];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (Serial.availableForWrite() > strlen(buf)) Serial.print(buf);
  else Serial.print(".");

  // if (log_mutex && xSemaphoreTake(log_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
  //   strncpy(shared_log_buffer, buf, sizeof(shared_log_buffer) - 1);
  //   shared_log_buffer[sizeof(shared_log_buffer) - 1] = '\0';
  //   xSemaphoreGive(log_mutex);
  // }
  if (log_queue) {
    xQueueSend(log_queue, buf, pdMS_TO_TICKS(5));
  }
}

void net_begin_ap() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);
  obdServer.begin();
  obdServer.setNoDelay(true);
}

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
      if (!placed) c.stop();
    }
  }
  for (int i = 0; i < 4; ++i) {
    if (clients[i] && !clients[i].connected())
      clients[i].stop();
  }
}

// ===================================================================================
// K-Line Core Communication & Honda Protocol
// ===================================================================================

static uint8_t calculate_honda_checksum(const uint8_t *buf, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++) sum += buf[i];
  return (uint8_t)(0x100 - (sum & 0xFF));
}

static void hex_dump_line(const char *prefix, const uint8_t *data, size_t len) {
  char line[LOGBUF_SIZE];
  size_t pos = 0;
  pos += snprintf(line + pos, sizeof(line) - pos, "%s", prefix);
  for (size_t i = 0; i < len && (pos + 4) < sizeof(line); ++i)
    pos += snprintf(line + pos, sizeof(line) - pos, "%02X ", data[i]);
  if (pos + 2 < sizeof(line)) {
    line[pos++] = '\n';
    line[pos] = '\0';
  } else {
    line[sizeof(line) - 2] = '\n';
    line[sizeof(line) - 1] = '\0';
  }
  logf("%s", line);
}

static bool verify_honda_response(const uint8_t *resp, size_t n) {
  if (n < 4) return false;
  uint8_t len_byte = resp[1];
  if (len_byte != n) {
    logf("! RESP length mismatch: LEN=%u, actual=%u\n", (unsigned)len_byte, (unsigned)n);
    return false;
  }
  uint8_t cs = calculate_honda_checksum(resp, n - 1);
  if (cs != resp[n - 1]) {
    logf("! RESP bad checksum: calc=%02X, got=%02X\n", cs, resp[n - 1]);
    return false;
  }
  if (resp[0] != HONDA_RESP_DA) {
    logf("! RESP unexpected DA: %02X (expected %02X)\n", resp[0], HONDA_RESP_DA);
    return false;
  }
  if (resp[2] != HONDA_TOOL_ADDR) {
    logf("! RESP unexpected SA: %02X (expected %02X)\n", resp[2], HONDA_TOOL_ADDR);
    return false;
  }
  return true;
}

// ===================================================================================
// K-Line Fast Init
// ===================================================================================

static void kline_fast_init_pulse() {
  KLine.end();

  pinMode(K_TX_PIN, OUTPUT);
  vTaskDelay(pdMS_TO_TICKS(BUS_IDLE_MS));
  digitalWrite(K_TX_PIN, LOW);
  vTaskDelay(pdMS_TO_TICKS(FAST_INIT_MS));
  digitalWrite(K_TX_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(FAST_INIT_MS));

  KLine.begin(K_BAUD, SERIAL_8N1, K_RX_PIN, K_TX_PIN);

  while (KLine.available()) (void)KLine.read();                 // clear any noise
  KLine.flush();                                               // ensure TX idle
}

static size_t k_read_frame(uint8_t *out, size_t max_len, uint32_t timeout_ms) {
  size_t n = 0;
  const uint32_t t_start = millis();
  uint32_t last_rx = millis();
  bool got_any = false;
  int target_len = -1;

  while ((millis() - t_start) < timeout_ms) {
    while (KLine.available()) {
      if (n < max_len) out[n++] = (uint8_t)KLine.read();
      if (n == 2) target_len = out[1];
      last_rx = millis();
      got_any = true;
      if (target_len > 0 && (int)n >= target_len) return n;
    }
    if (got_any && (millis() - last_rx) > IDLE_GAP_MS) break;
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return n;
}

// ----------------- NEW: Initialize without SA (per HUD ECU Hacker) -----------------
static bool send_honda_init_no_sa() {
  // payload for Initialize (no SA): 00 F0
  // Frame: [DA=0x72][LEN][PAYLOAD...][CS]   (LEN = total bytes)
  const uint8_t payload[] = { 0x00, 0xF0 };
  const size_t  frame_len = 2 /*DA+LEN*/ + sizeof(payload) + 1 /*CS*/;

  if (frame_len > MAX_FRAME) {
    logf("! INIT frame too large\n");
    return false;
  }

  uint8_t frame[MAX_FRAME];
  frame[0] = HONDA_ECU_ADDR;             // 0x72
  frame[1] = (uint8_t)frame_len;         // total length
  memcpy(&frame[2], payload, sizeof(payload));
  frame[frame_len - 1] = calculate_honda_checksum(frame, frame_len - 1);

  while (KLine.available()) (void)KLine.read();
  size_t wrote = KLine.write(frame, frame_len);
  KLine.flush();
  hex_dump_line("> INIT: ", frame, frame_len);
  return (wrote == frame_len);
}

// --------------- Send request WITH SA=0x71 (your regular requests) -----------------
static bool send_honda_req_with_sa(const uint8_t *payload, size_t payload_len) {
  const size_t frame_len = 3 + payload_len + 1; // DA, LEN, SA + payload + CS
  if (frame_len > MAX_FRAME || payload_len > (MAX_FRAME - 4)) {
    logf("! send_honda_req_with_sa: payload too large (%u)\n", (unsigned)payload_len);
    return false;
  }

  uint8_t frame[MAX_FRAME];
  frame[0] = HONDA_ECU_ADDR;                 // 0x72
  frame[1] = (uint8_t)frame_len;             // total length
  frame[2] = HONDA_TOOL_ADDR;                // 0x71
  memcpy(&frame[3], payload, payload_len);
  frame[frame_len - 1] = calculate_honda_checksum(frame, frame_len - 1);

  while (KLine.available()) (void)KLine.read();
  size_t wrote = KLine.write(frame, frame_len);
  KLine.flush();
  hex_dump_line("> SENT: ", frame, frame_len);
  return (wrote == frame_len);
}

// ---------------------- Start Communication (2-step handshake) ---------------------
static bool start_communication_handshake() {
  logf("Handshake step 1: Initialize (no SA)\n");
  if (!send_honda_init_no_sa()) {
    logf("...INIT send failed.\n");
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(FRAME_TIMEOUT_MS));

  // Wait for INIT response
  uint8_t resp1[MAX_FRAME];
  size_t n1 = k_read_frame(resp1, sizeof(resp1), FRAME_TIMEOUT_MS);
  if (n1 == 0) {
    logf("...INIT failed. No response.\n");
    return false;
  }
  // For INIT we only verify checksum + basic shape (DA=0x02, LEN match).
  // Some ECUs may answer without SA in INIT phase; accept if checksum/len are OK.
  {
    // Minimal verify for INIT: length + checksum + DA=0x02
    uint8_t len_byte = resp1[1];
    uint8_t cs_calc  = calculate_honda_checksum(resp1, n1 - 1);
    if (len_byte != n1 || cs_calc != resp1[n1 - 1] || resp1[0] != HONDA_RESP_DA) {
      hex_dump_line("< BAD_INIT: ", resp1, n1);
      logf("...INIT failed. Invalid response.\n");
      return false;
    }
    hex_dump_line("< INIT_OK: ", resp1, n1);
  }

  // Step 2: Start Communication WITH SA=0x71 (your logged pattern: 00 18)
  logf("Handshake step 2: StartCommunication (with SA=0x71)\n");
  const uint8_t cmd_start[] = { 0x00 };
  if (!send_honda_req_with_sa(cmd_start, sizeof(cmd_start))) {
    logf("...StartCommunication send failed.\n");
    return false;
  }

  uint8_t resp2[MAX_FRAME];
  size_t n2 = k_read_frame(resp2, sizeof(resp2), FRAME_TIMEOUT_MS);
  if (n2 == 0) {
    logf("...StartCommunication failed. No response.\n");
    return false;
  }
  if (!verify_honda_response(resp2, n2)) {
    hex_dump_line("< BAD_SC: ", resp2, n2);
    logf("...StartCommunication failed. Invalid response.\n");
    return false;
  }
  hex_dump_line("< SC_OK: ", resp2, n2);

  logf("Handshake complete.\n");
  no_reply_count   = 0;
  is_comm_started  = true;
  return true;
}

// ===================================================================================
// K-Line Task (This runs on Core 1)
// ===================================================================================

static void kline_task_code(void *pvParameters) {
  logf("K-Line Task started on Core %d\n", xPortGetCoreID());

  // Commands (SID, PID) list
  const uint8_t commands[][2] = {
    {0x73, 0x01}, {0x73, 0x02}, {0x73, 0x03}, {0x73, 0x04}, {0x73, 0x05}, {0x73, 0x06},
    {0x73, 0x07}, {0x73, 0x08}, {0x73, 0x09}, {0x73, 0x0A}, {0x73, 0x0B},
    {0x74, 0x01}, {0x74, 0x02}, {0x74, 0x03}, {0x74, 0x04}, {0x74, 0x05}, {0x74, 0x06},
    {0x74, 0x07}, {0x74, 0x08}, {0x74, 0x09}, {0x74, 0x0A}, {0x74, 0x0B},
    {0x71, 0x17}, {0x71, 0x20}, {0x71, 0x21}, {0x71, 0x67}, {0x71, 0x70}, {0x71, 0xD0}, {0x71, 0xD1}
  };

  uint8_t response_buf[MAX_FRAME];

  // Do fast-init once before attempting handshake
  kline_fast_init_pulse();

  for (;;) {
    if (!is_comm_started) {
      if (!start_communication_handshake()) {
        logf("Handshake failed, re-initializing...\n");
        kline_fast_init_pulse();
        // no back-off as requested
      }
    } else {
      // Polling phase
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

      for (size_t i = 0; i < (sizeof(commands) / sizeof(commands[0])); ++i) {
        const uint8_t *cmd = commands[i];

        if (!send_honda_req_with_sa(cmd, 2)) {
          logf("Send failed for SID %02X, PID %02X\n", cmd[0], cmd[1]);
          vTaskDelay(pdMS_TO_TICKS(REQ_GAP_MS));
          continue;
        }

        size_t n = k_read_frame(response_buf, sizeof(response_buf), FRAME_TIMEOUT_MS);
        if (n == 0) {
          no_reply_count++;
          logf("No reply for SID %02X, PID %02X\n", cmd[0], cmd[1]);
          if (no_reply_count > 5) {
            logf("Too many failed responses, assuming connection lost.\n");
            is_comm_started = false;
            break;
          }
        } else {
          // For regular responses we verify strictly
          if (verify_honda_response(response_buf, n)) {
            no_reply_count = 0;
            hex_dump_line("< RECV: ", response_buf, n);
          } else {
            hex_dump_line("< BAD : ", response_buf, n);
            no_reply_count++;
            if (no_reply_count > 5) {
              logf("Too many invalid responses, assuming connection lost.\n");
              is_comm_started = false;
              break;
            }
          }
        }
        vTaskDelay(pdMS_TO_TICKS(REQ_GAP_MS));
      }

      if (is_comm_started) {
        logf("--- Polling Cycle Complete ---\n\n");
      }
    }
  }
}

// ===================================================================================
// Main Program Flow (setup runs on Core 1, loop runs on Core 0)
// ===================================================================================
void setup() {
  Serial.begin(115200);

  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(K_TX_PIN, OUTPUT);
  vTaskDelay(pdMS_TO_TICKS(200));

  // log_mutex = xSemaphoreCreateMutex();
  log_queue = xQueueCreate(10, LOGBUF_SIZE);

  logf("\nESP32 Honda K-Line Logger\n");
  logf("Main setup running on Core %d\n", xPortGetCoreID());

  net_begin_ap();
  logf("TCP server on port %d ready.\n", TCP_PORT);

  xTaskCreatePinnedToCore(
    kline_task_code,
    "K-Line Task",
    8192,
    NULL,
    1,
    &kline_task_handle,
    1);

  logf("K-Line task created and pinned to Core 1.\n");
  logf("Main setup complete. Handing over to Core 0 for networking.\n");
}

void loop() {
  net_poll_clients();

  char local_buffer[LOGBUF_SIZE];
  local_buffer[0] = '\0';

  if (log_queue && xQueueReceive(log_queue, &local_buffer, (TickType_t)0) == pdPASS) {
    net_broadcast(local_buffer);
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}
