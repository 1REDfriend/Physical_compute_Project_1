#include <Arduino.h>
#include <WiFi.h>

static const char* AP_SSID = "ESP32-OBD";
static const char* AP_PASS = "12345678";     // >= 8 chars
static const IPAddress AP_IP(192,168,4,1);
static const IPAddress AP_GW(192,168,4,1);
static const IPAddress AP_MASK(255,255,255,0);
static const uint16_t TCP_PORT = 3333;

WiFiServer obdServer(TCP_PORT);
WiFiClient clients[4]; // up to 4 telnet/CLI readers

void net_begin_ap() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  // channel 6, not hidden, max 4 clients
  WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);
  obdServer.begin();
  obdServer.setNoDelay(true);
}

void net_poll_clients() {
  // accept new client if there is one
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
      if (!placed) c.stop(); // refuse if full
    }
  }
  // cleanup dead connections
  for (int i = 0; i < 4; ++i) {
    if (clients[i] && !clients[i].connected()) clients[i].stop();
  }
}

static void net_broadcast(const char* s) {
  for (int i = 0; i < 4; ++i)
    if (clients[i] && clients[i].connected()) clients[i].print(s);
}

static void net_broadcastf(const char* fmt, ...) {
  char buf[256];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  net_broadcast(buf);
}

// ---------- Wiring (change pins if needed) ----------
static const int K_TX_PIN = 17;   // ESP32 TX to K-Line transceiver TXD
static const int K_RX_PIN = 16;   // ESP32 RX from K-Line transceiver RXD

// UART1 on ESP32
HardwareSerial KLine(1);

// ---------- Timing ----------
static const uint32_t K_BAUD = 10400;   // ISO9141/14230 common rate
static const uint32_t REQ_GAP_MS = 55;  // gap between requests
static const uint32_t BYTE_TIMEOUT_MS = 40;
static const uint32_t FRAME_TIMEOUT_MS = 200;

// ---------- ISO 9141 3-byte header (common functional addressing) ----------
static const uint8_t HDR0 = 0x68; // Target (ECU)
static const uint8_t HDR1 = 0x6A; // Source (tester functional)
static const uint8_t HDR2 = 0xF1; // Tester address

// ---------- Utils ----------
static uint8_t checksum8(const uint8_t *buf, size_t n) {
  uint16_t sum = 0;
  for (size_t i = 0; i < n; ++i) sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

// Read one byte with timeout
bool k_read_byte(uint8_t &b, uint32_t t_byte_ms = BYTE_TIMEOUT_MS) {
  uint32_t t0 = millis();
  while ((millis() - t0) < t_byte_ms) {
    if (KLine.available()) { b = (uint8_t)KLine.read(); return true; }
  }
  return false;
}

// Read a response frame with basic pattern: resp header + 0x40|svc + pid + data... + cs
// Store into out[], return length. Very simple framing: read until idle gap or max.
size_t k_read_frame(uint8_t *out, size_t max_len) {
  size_t n = 0;
  uint32_t t0 = millis();
  while ((millis() - t0) < FRAME_TIMEOUT_MS) {
    while (KLine.available()) {
      if (n < max_len) out[n++] = (uint8_t)KLine.read();
      t0 = millis(); // extend while we keep receiving
    }
  }
  return n;
}

// Send a simple 5-byte request [HDR0 HDR1 HDR2 SVC PID CS]
void k_send_req(uint8_t svc, uint8_t pid) {
  uint8_t f[6];
  f[0] = HDR0; f[1] = HDR1; f[2] = HDR2; f[3] = svc; f[4] = pid;
  f[5] = checksum8(f, 5);
  KLine.write(f, sizeof(f));
  KLine.flush();
}

// Very simple fast-init pulse (optional). Some ECUs need this before talking.
// NOTE: Depending on the transceiver, logic may be non-inverted or inverted.
// If it doesn't wake ECU, comment this out and use proper 5-baud init instead.
void kline_fast_init_pulse() {
  KLine.end();                  // Release UART so we can bit-bang TX pin
  pinMode(K_TX_PIN, OUTPUT);
  digitalWrite(K_TX_PIN, HIGH); // idle
  delay(100);
  // K-line fast init: 25ms LOW, 25ms HIGH, then wait ~200ms (per ISO 14230 "fast init")
  digitalWrite(K_TX_PIN, LOW);
  delay(25);
  digitalWrite(K_TX_PIN, HIGH);
  delay(25);
  delay(200);
  // Re-attach UART
  KLine.begin(K_BAUD, SERIAL_8N1, K_RX_PIN, K_TX_PIN);
}

// Parse/print common Mode 1 PIDs
void print_pid_decoded(uint8_t pid, const uint8_t *data, size_t n) {
  // data points to the bytes after [resp header, 0x40|svc, pid]
  // most Mode 01 replies are 1..4 bytes. Guard checks:
  auto u16 = [&](int i){ return (uint16_t)data[i] << 8 | data[i+1]; };

  switch (pid) {
    case 0x04: if (n>=1) Serial.printf("Load: %.1f %%\n", data[0]*100.0/255.0); break;
    case 0x05: if (n>=1) Serial.printf("Coolant Temp: %d C\n", (int)data[0]-40); break;
    case 0x0C: if (n>=2) Serial.printf("RPM: %.0f rpm\n", u16(0)/4.0); break;
    case 0x0D: if (n>=1) Serial.printf("Speed: %d km/h\n", data[0]); break;
    case 0x0F: if (n>=1) Serial.printf("Intake Air Temp: %d C\n", (int)data[0]-40); break;
    case 0x10: if (n>=2) Serial.printf("MAF: %.2f g/s\n", u16(0)/100.0); break;
    case 0x11: if (n>=1) Serial.printf("Throttle: %.1f %%\n", data[0]*100.0/255.0); break;
    case 0x2F: if (n>=1) Serial.printf("Fuel Level: %.1f %%\n", data[0]*100.0/255.0); break;
    case 0x33: if (n>=1) Serial.printf("Baro Pressure: %d kPa\n", data[0]); break;
    case 0x46: if (n>=1) Serial.printf("Ambient Temp: %d C\n", (int)data[0]-40); break;
    case 0x5E: if (n>=2) Serial.printf("Engine Fuel Rate: %.2f L/h\n", u16(0)/20.0); break;
    default: {
      Serial.printf("PID 0x%02X (raw):", pid);
      for (size_t i=0;i<n;i++) Serial.printf(" %02X", data[i]);
      Serial.println();
    } break;
  }
}

// Send Mode 01 + pid, read and print decoded value
bool request_mode01_and_print(uint8_t pid) {
  // drain any stale bytes
  while (KLine.available()) KLine.read();
  k_send_req(0x01, pid);
  delay(REQ_GAP_MS);

  uint8_t buf[64];
  size_t n = k_read_frame(buf, sizeof(buf));
  if (n < 6) {
    Serial.printf("PID 0x%02X: no/short reply (n=%u)\n", pid, (unsigned)n);
    return false;
  }

  // Find pattern: [xx xx xx] [0x41] [pid] [data...] [cs]
  // Some ECUs use 3-byte resp header; we’ll scan to find 0x41 and pid.
  int idx = -1;
  for (size_t i=0;i+2<n;i++) {
    if (buf[i] == 0x41 && buf[i+1] == pid) { idx = (int)i; break; }
  }
  if (idx < 0) {
    // Some use full header: [48 6B 11 41 pid ...]; try that
    for (size_t i=0;i+4<n;i++) {
      if (buf[i+3]==0x41 && buf[i+4]==pid) { idx = (int)i+3; break; }
    }
  }
  if (idx < 0) {
    Serial.printf("PID 0x%02X: unrecognized reply:", pid);
    for (size_t i=0;i<n;i++) Serial.printf(" %02X", buf[i]);
    Serial.println();
    return false;
  }

  // Data bytes are everything after [0x41, pid] up to last byte minus checksum if present.
  // We won't strictly verify checksum across all header variants here; we just trim last byte as cs if long enough.
  int data_start = idx + 2;
  int data_end   = (int)n - 1; // assume last is checksum
  int data_len   = max(0, data_end - data_start);
  print_pid_decoded(pid, &buf[data_start], data_len);
  return true;
}

// Parse bitfield from 01 00/20/40... and return a mask of supported PIDs
// returns the 4 bytes A..D where MSB of A = PID+1
bool get_supported_block(uint8_t base_pid, uint8_t out_bits[4]) {
  // Expect reply: 41 <base> A B C D
  while (KLine.available()) KLine.read();
  k_send_req(0x01, base_pid);
  delay(REQ_GAP_MS);

  uint8_t buf[64];
  size_t n = k_read_frame(buf, sizeof(buf));
  if (n < 7) return false;

  // Find [41 base]
  int idx = -1;
  for (size_t i=0;i+1<n;i++) {
    if (buf[i]==0x41 && buf[i+1]==base_pid) { idx = (int)i; break; }
  }
  if (idx < 0) {
    // try headered
    for (size_t i=0;i+4<n;i++) {
      if (buf[i+3]==0x41 && buf[i+4]==base_pid) { idx = (int)i+3; break; }
    }
  }
  if (idx < 0) return false;

  int s = idx + 2;
  if (s+3 >= (int)n) return false;
  for (int i=0;i<4;i++) out_bits[i] = buf[s+i];
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32 Network Config...\n ESP32-ODB\n PASSWORD : 12345678");
  net_begin_ap();
  delay(1000);
  Serial.println("\nESP32 OBD-II K-Line Mode 01 scanner (ISO 9141/14230 @ 10400 8N1)");

  KLine.begin(K_BAUD, SERIAL_8N1, K_RX_PIN, K_TX_PIN);
  delay(200);

  // OPTIONAL: try a fast-init pulse if your ECU needs waking.
  // If this makes comms worse, comment it out.
  kline_fast_init_pulse();

  Serial.println("Starting scan of supported PIDs...");
}

void loop() {
  // Discover supported PIDs across ranges 0x00, 0x20, 0x40, 0x60, 0x80
  const uint8_t bases[] = {0x00, 0x20, 0x40, 0x60, 0x80};
  for (uint8_t b : bases) {
    net_poll_clients();
    uint8_t bits[4] = {0,0,0,0};
    if (!get_supported_block(b, bits)) {
      Serial.printf("Base 0x%02X: no supported map (skip)\n", b);
      continue;
    }
    Serial.printf("Supported map for 0x%02X-0x%02X: %02X %02X %02X %02X\n",
                  (unsigned)b+1, (unsigned)b+0x20, bits[0], bits[1], bits[2], bits[3]);

    // For each bit set, query that PID
    for (int i=0;i<32;i++) {
      // MSB of A -> PID b+1, next bit -> b+2, ..., LSB of D -> b+32
      int byte_idx = i / 8;
      int bit_in_byte = 7 - (i % 8);
      if (bits[byte_idx] & (1 << bit_in_byte)) {
        uint8_t pid = b + 1 + i;
        // Skip the continuation PID (i=31) which signals next block availability
        if (pid == (uint8_t)(b + 0x20)) continue;
        request_mode01_and_print(pid);
        delay(REQ_GAP_MS);
      }
    }
  }

  Serial.println("Scan cycle complete. Sleeping 2s...\n");
}
