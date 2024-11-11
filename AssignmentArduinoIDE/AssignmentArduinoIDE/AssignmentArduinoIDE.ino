// #include <Arduino.h>
// #include <LittleFS.h>
// #include "FS.h"

/*
// TODO: 1. LittleFS: deleteFile, removeDir; listDir
TODO: 2. NTP: TimeFormat for Logging
TODO: 3. Simulation: Sensor Simulation
TODO: 4. Deep Sleep: (2 mins when IDLE)
*/

/*
#define FORMAT_LITTLEFS_IF_FAILED true

// Func Declarations
void listDir(fs::FS& fs, const char* dirName, uint8_t levels);
void createDir(fs::FS& fs, const char* path);
void readFile(fs::FS& fs, const char* path);
void writeFile(fs::FS& fs, const char* path, const char* msg);
void appendFile(fs::FS& fs, const char* path, const char* msg);
void deleteFile(fs::FS& fs, const char* path);

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("Failed to Mount LittleFS");
    while(1) {;;};
    return;
  }
  // Setup Code
}

void loop() {

}

void listDir(fs::FS& fs, const char* dirName, uint8_t levels) {
  Serial.printf("Listing the Directory: %s\r\n", dirName);
  File root = fs.open(dirName);
  if (!root) {
    Serial.printf("Failed to Open Dir: %s\r\n", dirName);
    return;
  }
  if (!root.isDirectory()) {
    Serial.printf("%s is not a Directory\r\n", dirName);
    return;
  }

  File innerUnit = root.openNextFile();
  if (innerUnit.isDirectory()) {
    Serial.printf("\tDirectory: %s; Size: %d\r\n", 
      innerUnit.name(), innerUnit.size());
  }
  while (innerUnit) {
    Serial.printf("\tFile: %s; Size: %d\r\n", innerUnit.name(), innerUnit.size());
    innerUnit = root.openNextFile();
  }

}

void createDir(fs::FS& fs, const char* path) {
  Serial.printf("Creating Dir: %s\r\n", path);
  if (!fs.mkdir(path)) {
    Serial.printf("Error while creating dir: %s\r\n", path);
    return;
  }
  Serial.printf("Successfully created Dir: %s\r\n", path);
}

void readFile(fs::FS& fs, const char* path) {
  Serial.printf("Reading File: %s\r\n", path);
  File f = fs.open(path, FILE_READ);
  if (!f.available() || f.isDirectory()) {
    Serial.printf("Error while Trying to Read File: %s\r\n", path);
    return;
  }
  Serial.printf("\r\n\r\nPrinting Contents of file(%s): \r\n", f.name());
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
}

void appendFile(fs::FS& fs, const char* path, const char* msg) {
  File f = fs.open(path, FILE_APPEND);
  if (!f) {
    Serial.println("Failed to Open File");
    return;
  }
  if (!f.print(msg)) {
    Serial.printf("Error Appending to File: %s\r\n", path);
    f.close();
    return;
  } 
  Serial.printf("Successfully Appended to File %s\r\n", f.name());
  f.close();
}

void deleteFile(fs::FS& fs, const char* path) {
  Serial.printf("Deleting the File: %s\r\n", path);
  if (!fs.remove(path)) {
    Serial.printf("Error while Deleting: %s\r\n", path);
    return;
  } 
  Serial.printf("Successfully Deleted File: %s\r\n", path);
}


// NTP
#include <WiFi.h>
#include "time.h"

const char* SSID = "TP-Link_E18C";
const char* PASS = "31502719";

const char* ntpServer           = "pool.ntp.org";
const int32_t gmtOffsetSec      = 5*60*60 + 30*60; // India: +5:30 GMT
const int32_t dayLightOffsetSec = 0;


void setup() {
  Serial.begin(115200);
  Serial.printf("Connecting to: %s\r\n", SSID);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    delay(500);
  }
  Serial.println("Connected Successfully");
  configTime(gmtOffsetSec, dayLightOffsetSec, ntpServer);

  // struct tm timeinfo;
  // if (!getLocalTime(&timeinfo)) {
  //   Serial.println("Failed To Obtain Time: Try Resetting");
  //   while (1) {;;};
  // }
  // Serial.println("%A %B %d %Y %H:%M:%S");

}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed To Obtain Time: Try Resetting");
    delay(500);
    // while (1) {;;};
  } else {
    Serial.println(&timeinfo, "%A %B %d %Y %H:%M:%S");
    delay(1000);
  }


}


RTC_DATA_ATTR size_t g_rtc_counter  = 0;
const size_t timeToWakeUp_us        = 2 * 1000 * 1000;

void setup() {
  ++g_rtc_counter;
  Serial.begin(115200);
  Serial.printf("\r\n\r\nBoot Count = %d\r\n", g_rtc_counter);
  esp_sleep_enable_timer_wakeup(timeToWakeUp_us);
  Serial.printf("Deep Sleep Interval: %d\r\n", timeToWakeUp_us);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This line Should never be Printed");
}

void loop() {

}

*/

typedef struct {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;
}IMUdata;

typedef struct {
  bool motionDetected;
  void (*wakeupCallback)();
}SimulatedQMI8658;
  


void simulatorWakeUpCallback() {
  Serial.printf("SimulatorWakeUpCallback: Say's Hello\r\n");
}

RTC_DATA_ATTR size_t readingCounter = 0;

IMUdata accData[5];
IMUdata gyrData[5];

// int32_t t = accData[2].x;


// g_qmi8658.setWakeUpMotionEventCallBack(&simulatorWakeUpCallback);


void setup() {
  Serial.begin(115200);
  SimulatedQMI8658 g_qmi8658;
  g_qmi8658.motionDetected = false;
  g_qmi8658.wakeupCallback = &simulatorWakeUpCallback;
}

void loop() {

}

/*
class SimulatedQMI8658 {
private:
  bool motionDetected;
  void (*wakeupCallback)();
public:
  SimulatedQMI8658(): motionDetected(false), wakeupCallback(nullptr) {}
  bool begin() {
    // Simulate sensor initialization
    return true;
  }
  bool readFromFifo(IMUdata* acc, int accCount, IMUdata* gyr, int gyrCount) {
    // Generate random accelerometer and gyroscope data
    for (int i = 0; i < accCount; i++) {
      acc[i].x = random(-32768, 32767) / 1000.0;
      acc[i].y = random(-32768, 32767) / 1000.0;
      acc[i].z = random(-32768, 32767) / 1000.0;
    }
    for (int i = 0; i < gyrCount; i++) {
      gyr[i].x = random(-32768, 32767) / 100.0;
      gyr[i].y = random(-32768, 32767) / 100.0;
      gyr[i].z = random(-32768, 32767) / 100.0;
    }
    return true;
  }
  void configWakeOnMotion() {
    // Simulate wake-on-motion configuration
    // TODO: Find a way to do this in Simulation instead of EXTI (i.e., using ULP)
    return;
  }
  void setWakeupMotionEventCallBack(void (*callback)()) {
    wakeupCallback = callback;
    return;
  }
  // Simulate motion detection (call this periodically in your main loop)
  void simulateMotion() {
    if (random(100) < 0 && !motionDetected) { 
      motionDetected = true;
    if (wakeupCallback) {
      wakeupCallback();
    }
    } else {
      motionDetected = false;
    }
    return;
  }
};
*/