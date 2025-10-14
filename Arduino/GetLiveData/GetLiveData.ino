#include "OBD2_KLine.h"  // Include the library for OBD2 K-Line communication
//#include <AltSoftSerial.h>  // Optional alternative software serial (not used here)
//AltSoftSerial Alt_Serial;   // Create an alternative serial object (commented out)

#include "wifi_K.h"
#include "freertos/queue.h"

Wifi_K wifiManager;
QueueHandle_t log_queue = nullptr;
OBD2_KLine KLine(Serial1, 10400, 16, 17);

HondaLiveData myHondaData;

void logf(const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  Serial.print(buf); 
  if (log_queue) {
    xQueueSend(log_queue, buf, (TickType_t)0); 
  }
}

void setup() {
  Serial.begin(115200);
  wifiManager.begin();
  log_queue = wifiManager.getQueueHandle();
  logf("OBD2 K-Line Get Live Data Example");

  KLine.setDebug(Serial);          // Optional: outputs debug messages to the selected serial port
  KLine.setProtocol("ISO14230_Honda");  // Optional: communication protocol (default: Automatic; supported: ISO9141, ISO14230_Slow, ISO14230_Fast, Automatic)
  KLine.setByteWriteInterval(5);   // Optional: delay (ms) between bytes when writing
  KLine.setInterByteTimeout(60);   // Optional: sets the maximum inter-byte timeout (ms) while receiving data
  KLine.setReadTimeout(1000);      // Optional: maximum time (ms) to wait for a response after sending a request

  logf("OBD2 Starting.");
}

void loop() {
  if (KLine.initOBD2()) {
    if (KLine.getHondaLiveData(0x17, myHondaData)) {
      logf("RPM:%.0f, TPS:%.1f, ECT:%d, IAT:%d, VSS:%d, MAP:%d, BATT:%.2f\n",
           myHondaData.engineSpeed_rpm,
           myHondaData.tps_percent,
           myHondaData.ect_celsius,
           myHondaData.iat_celsius,
           myHondaData.vehicleSpeed_kmh,
           myHondaData.map_mbar,
           myHondaData.battery_volt
      );
    }
  }
  wifiManager.handle();
}
