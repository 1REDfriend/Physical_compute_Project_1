#ifndef WIFI_K_H
#define WIFI_K_H

#include <WiFi.h>
#include "freertos/queue.h"

// Define constants here so they are available to any file that includes this header
#define MAX_WIFI_CLIENTS 4
#define LOG_QUEUE_LENGTH 10
#define LOG_BUFFER_SIZE  256

class Wifi_K {
public:
  // Constructor
  Wifi_K();

  // Public methods
  void begin();
  void handle();
  QueueHandle_t getQueueHandle();

private:
  // Private helper methods
  void handleClients();
  void broadcastFromQueue();
  void broadcast(const char *message);
  void broadcast(const String &message);

  // Member variables
  WiFiServer server;
  WiFiClient clients[MAX_WIFI_CLIENTS];
  QueueHandle_t _log_queue = nullptr;
};

#endif // WIFI_K_H