#include <Arduino.h>
#include <WiFi.h>
#include <stdarg.h> // for va_list

// ===================================================================================
// Configuration
// ===================================================================================

// -------- Wi-Fi AP config --------
static const char *AP_SSID = "ESP32-OBD";
static const char *AP_PASS = "12345678"; // >= 8 chars
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const uint16_t TCP_PORT = 3333;

// -------- K-Line & Hardware Pins --------
static const int K_TX_PIN = 17;      // ESP32 TX -> Transceiver TXD
static const int K_RX_PIN = 16;      // ESP32 RX <- Transceiver RXD
static const int STATUS_LED_PIN = 2; // Onboard LED for status indication
HardwareSerial KLine(1);             // Use UART1 on ESP32

// -------- KWP2000 Timing --------
static const uint32_t K_BAUD = 10400;         // ISO9141/14230 common rate
static const uint32_t REQ_GAP_MS = 55;        // P2: Min time between requests
static const uint32_t BYTE_TIMEOUT_MS = 100;  // P3: Timeout for single byte reception
static const uint32_t FRAME_TIMEOUT_MS = 500; // Overall frame guard timeout
static const uint32_t IDLE_GAP_MS = 55;       // P1: Inter-byte timeout to detect end-of-frame

// ===================================================================================
// Global State
// ===================================================================================

WiFiServer obdServer(TCP_PORT);
WiFiClient clients[4]; // up to 4 telnet/CLI readers

// ----- Keep-alive & re-init policy -----
static uint32_t last_keepalive_ms = 0;
static uint8_t no_reply_count = 0;

// ----- ECU Header Discovery -----
const uint8_t KWP_HEADERS[][3] = {
    {0xC1, 0x72, 0xF1}, // KWP2000: Honda Custom Diagnostic DA=0x72
    {0xC1, 0x33, 0xF1}, // KWP2000: Standard OBD-II ECU DA=0x33
    {0xC1, 0x7E, 0xF1}, // KWP2000: Honda Custom Flash DA=0x7E
    {0x80, 0x11, 0xF1}, // KWP2000: Alternative ECU DA=0x11
    {0x68, 0x6A, 0xF1}  // ISO 9141-2: DA=0x6A
};
const int NUM_HEADERS = sizeof(KWP_HEADERS) / sizeof(KWP_HEADERS[0]);
uint8_t active_header[3];
int current_header_index = 0;
bool ecu_has_responded = false;

// ===================================================================================
// Networking & Logging
// ===================================================================================

static void net_broadcast(const char *s)
{
  for (int i = 0; i < 4; ++i)
  {
    if (clients[i] && clients[i].connected())
      clients[i].print(s);
  }
}

static void logf(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.print(buf);
  net_broadcast(buf);
}

void net_begin_ap()
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);
  obdServer.begin();
  obdServer.setNoDelay(true);
}

void net_poll_clients()
{
  if (obdServer.hasClient())
  {
    WiFiClient c = obdServer.available();
    if (c)
    {
      bool placed = false;
      for (int i = 0; i < 4; ++i)
      {
        if (!clients[i] || !clients[i].connected())
        {
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
  for (int i = 0; i < 4; ++i)
  {
    if (clients[i] && !clients[i].connected())
      clients[i].stop();
  }
}

// ===================================================================================
// K-Line Core Communication
// ===================================================================================

static uint8_t checksum8(const uint8_t *buf, size_t n_without_cs)
{
  uint16_t sum = 0;
  for (size_t i = 0; i < n_without_cs; ++i)
    sum += buf[i];
  return (uint8_t)(sum & 0xFF);
}

static bool verify_checksum(const uint8_t *buf, size_t n_with_cs)
{
  if (n_with_cs < 2)
    return false;
  uint8_t cs = buf[n_with_cs - 1];
  return checksum8(buf, n_with_cs - 1) == cs;
}

void clear_rx_buffer()
{
  while (KLine.available())
  {
    (void)KLine.read();
  }
}

size_t k_read_frame(uint8_t *out, size_t max_len)
{
  size_t n = 0;
  uint32_t t_start = millis();
  uint32_t last_rx = millis();
  bool got_any = false;

  while ((millis() - t_start) < FRAME_TIMEOUT_MS)
  {
    while (KLine.available())
    {
      if (n < max_len)
        out[n++] = (uint8_t)KLine.read();
      last_rx = millis();
      got_any = true;
    }
    if (got_any && (millis() - last_rx) > IDLE_GAP_MS)
      break;
  }
  return n;
}

void k_send_req(uint8_t svc, uint8_t pid)
{
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

void kline_fast_init_pulse()
{
  KLine.end();                  // Release UART to bit-bang the TX pin
  digitalWrite(K_TX_PIN, HIGH); // Ensure bus is idle (High)
  delay(100);

  // KWP2000 Fast Init: 50ms LOW pulse to wake up the ECU
  digitalWrite(K_TX_PIN, LOW);
  delay(50);
  digitalWrite(K_TX_PIN, HIGH);

  // Re-attach UART immediately to send the first command
  KLine.begin(K_BAUD, SERIAL_8N1, K_RX_PIN, K_TX_PIN);
  clear_rx_buffer();
}

// ===================================================================================
// Session & Response Management
// ===================================================================================

inline void mark_rx_ok()
{
  no_reply_count = 0;
  if (!ecu_has_responded)
  {
    ecu_has_responded = true; // Lock the header on first success!
    logf("\n*** SUCCESS: ECU Responded with Header {0x%02X, 0x%02X, 0x%02X}. Locking this header. ***\n\n",
         active_header[0], active_header[1], active_header[2]);
  }
}

inline void mark_rx_fail()
{
  no_reply_count++;
}

bool send_tester_present()
{
  k_send_req(0x3E, 0x00); // Service 0x3E: Tester Present
  delay(REQ_GAP_MS);
  uint8_t buf[32];
  size_t n = k_read_frame(buf, sizeof(buf));

  if (n > 0)
  { // Any valid response is good enough
    mark_rx_ok();
    return true;
  }
  else
  {
    mark_rx_fail();
    return false;
  }
}

void session_maintain()
{
  uint32_t now = millis();
  if (now - last_keepalive_ms > 2000)
  {
    send_tester_present();
    last_keepalive_ms = now;
  }

  if (no_reply_count >= 3)
  {
    logf("!!! Connection lost. Re-initializing... !!!\n");
    ecu_has_responded = false; // Go back to discovery mode
    no_reply_count = 0;
    kline_fast_init_pulse();
  }
}

// ===================================================================================
// OBD-II PID Processing
// ===================================================================================

void print_pid_decoded(uint8_t pid, const uint8_t *data, size_t n)
{
  auto u16 = [&](int i)
  { return (uint16_t)data[i] << 8 | data[i + 1]; };
  switch (pid)
  {
  case 0x04:
    if (n >= 1)
      logf("Load: %.1f %%\n", data[0] * 100.0 / 255.0);
    break;
  case 0x05:
    if (n >= 1)
      logf("Coolant Temp: %d C\n", (int)data[0] - 40);
    break;
  case 0x0C:
    if (n >= 2)
      logf("RPM: %.0f rpm\n", u16(0) / 4.0);
    break;
  case 0x0D:
    if (n >= 1)
      logf("Speed: %d km/h\n", data[0]);
    break;
  case 0x0F:
    if (n >= 1)
      logf("Intake Air Temp: %d C\n", (int)data[0] - 40);
    break;
  case 0x11:
    if (n >= 1)
      logf("Throttle: %.1f %%\n", data[0] * 100.0 / 255.0);
    break;
  default:
    logf("PID 0x%02X (raw):", pid);
    for (size_t i = 0; i < n; i++)
      logf(" %02X", data[i]);
    logf("\n");
    break;
  }
}

bool get_supported_block(uint8_t base_pid, uint8_t out_bits[4])
{
  k_send_req(0x01, base_pid);
  delay(REQ_GAP_MS);
  uint8_t buf[64];
  size_t n = k_read_frame(buf, sizeof(buf));

  if (n < 7)
  {
    logf("PID 0x%02X: No/short reply (n=%u)\n", base_pid, (unsigned)n);
    mark_rx_fail();
    return false;
  }

  // Check for Negative Response (0x7F)
  if (buf[3] == 0x7F && buf[4] == 0x01)
  {
    logf("PID 0x%02X: Negative Response Code (NRC) 0x%02X\n", base_pid, buf[5]);
    mark_rx_fail();
    return false;
  }

  if (!verify_checksum(buf, n))
  {
    logf("PID 0x%02X: Checksum mismatch!\n", base_pid);
    // Not fatal, but worth noting
  }

  int idx = -1;
  for (size_t i = 0; i + 1 < n; i++)
  {
    if (buf[i] == 0x41 && buf[i + 1] == base_pid)
    {
      idx = (int)i;
      break;
    }
  }

  if (idx < 0)
  { // Fallback search if header format is different
    for (size_t i = 0; i + 4 < n; i++)
    {
      if (buf[i + 3] == 0x41 && buf[i + 4] == base_pid)
      {
        idx = (int)i + 3;
        break;
      }
    }
  }

  if (idx < 0 || (idx + 2 + 4 > n))
  {
    logf("PID 0x%02X: Unrecognized reply format.\n", base_pid);
    mark_rx_fail();
    return false;
  }

  mark_rx_ok();
  memcpy(out_bits, &buf[idx + 2], 4);
  return true;
}

// ===================================================================================
// Main Program Flow (setup & loop)
// ===================================================================================

void setup()
{
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(K_TX_PIN, OUTPUT);

  delay(200);
  logf("\nESP32 OBD-II K-Line Scanner\n");
  logf("Starting Wi-Fi AP: %s\n", AP_SSID);
  net_begin_ap();
  logf("TCP server on port %d ready.\n", TCP_PORT);

  // Initial K-Line setup
  memcpy(active_header, KWP_HEADERS[current_header_index], 3);
  kline_fast_init_pulse();
  logf("Starting ECU discovery...\n");
}

void loop()
{
  net_poll_clients();

  if (!ecu_has_responded)
  {
    // --- DISCOVERY PHASE ---
    digitalWrite(STATUS_LED_PIN, HIGH); // LED on during search
    logf("Searching... Trying Header #%d: {0x%02X, 0x%02X, 0x%02X}\n",
         current_header_index, active_header[0], active_header[1], active_header[2]);

    uint8_t dummy_bits[4];
    get_supported_block(0x00, dummy_bits); // Request PID 0x00 to check for a response

    if (!ecu_has_responded)
    {
      current_header_index = (current_header_index + 1) % NUM_HEADERS;
      memcpy(active_header, KWP_HEADERS[current_header_index], 3);
      logf("No response. Moving to next header.\n");
      delay(500); // Wait before trying the next header
    }
    digitalWrite(STATUS_LED_PIN, LOW); // LED off after attempt
  }
  else
  {
    // --- SCANNING PHASE ---
    session_maintain();

    const uint8_t bases[] = {0x00, 0x20, 0x40}; // Scan first 3 blocks
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN)); // Blinking LED when connected
    
    for (uint8_t b : bases)
    {
      uint8_t bits[4] = {0};
      if (!get_supported_block(b, bits))
      {
        logf("Base 0x%02X: No supported map (skip)\n", b);
        continue;
      }
      logf("Supported map for 0x%02X-0x%02X: %02X %02X %02X %02X\n",
           (unsigned)b + 1, (unsigned)b + 0x20, bits[0], bits[1], bits[2], bits[3]);

      for (int i = 0; i < 32; i++)
      {
        if ((bits[i / 8] >> (7 - (i % 8))) & 1)
        {
          uint8_t pid = b + 1 + i;
          if (pid == b || pid == (uint8_t)(b + 0x20))
            continue; // Don't request the map PID itself
          // For now, let's just log that we would request it
          logf("-> Found supported PID: 0x%02X\n", pid);
          // request_mode01_and_print(pid); // Uncomment to fetch and print data
          delay(REQ_GAP_MS);
          net_poll_clients();
        }
      }
    }
    logf("Scan cycle complete. \n\n");
    net_poll_clients();
  }
}