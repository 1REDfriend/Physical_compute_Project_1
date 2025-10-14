#include "wifi_K.h" // Include the header file to get the class blueprint

// --- Wi-Fi & TCP Configuration ---
static const char *AP_SSID = "ESP32-OBD";
static const char *AP_PASS = "12345678";
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const uint16_t TCP_PORT = 3333;

// --- Method Implementations ---

Wifi_K::Wifi_K() : server(TCP_PORT) {
  // Constructor is intentionally empty
}

void Wifi_K::begin() {
  _log_queue = xQueueCreate(LOG_QUEUE_LENGTH, LOG_BUFFER_SIZE);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  WiFi.softAP(AP_SSID, AP_PASS, 6, 0, MAX_WIFI_CLIENTS);
  server.begin();
  server.setNoDelay(true);
}

QueueHandle_t Wifi_K::getQueueHandle() {
  return _log_queue;
}

void Wifi_K::handle() {
  handleClients();
  broadcastFromQueue();
}

void Wifi_K::broadcastFromQueue() {
  if (_log_queue) {
    char local_buffer[LOG_BUFFER_SIZE];
    if (xQueueReceive(_log_queue, local_buffer, (TickType_t)0) == pdPASS) {
      broadcast(local_buffer);
    }
  }
}

void Wifi_K::handleClients() {
  if (server.hasClient()) {
    WiFiClient newClient = server.available();
    if (newClient) {
      bool placed = false;
      for (int i = 0; i < MAX_WIFI_CLIENTS; i++) {
        if (!clients[i] || !clients[i].connected()) {
          clients[i].stop();
          clients[i] = newClient;
          placed = true;
          break;
        }
      }
      if (!placed) {
        newClient.stop();
      }
    }
  }

  for (int i = 0; i < MAX_WIFI_CLIENTS; i++) {
    if (clients[i] && !clients[i].connected()) {
      clients[i].stop();
    }
  }
}

void Wifi_K::broadcast(const char *message) {
  for (int i = 0; i < MAX_WIFI_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      clients[i].print(message);
    }
  }
}

void Wifi_K::broadcast(const String &message) {
  for (int i = 0; i < MAX_WIFI_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      clients[i].print(message);
    }
  }
}