// Includes
#include <LittleFS.h>
#include "FS.h"
#include "time.h"
#include <WiFi.h>
#include <string.h>
// ----------------------------------------------------------------------

/*
  TODO: 2. LittleFS: Just get it working great
  * Every Min:
      - getDateStr()
*/

#define FORMAT_LITTLEFS_IF_FAILED true


// Defines and Consts
const size_t accRunCriteria  = 11 * 11;
const size_t accPlayCriteria = 22 * 22;
const size_t gyrRunCriteria  = 109 * 109;
const size_t gyrPlayCriteria = 218 * 218;

const size_t timeToWakeUp_us   = 1000 * 1000 * 6; 
const uint32_t chanceOfMotion  = 50;
const uint32_t uploadFrequency = 5;                 // Once every 2 cycles

const char* SSID = "";  // @HardCode: Please insert your WiFi Credentials
const char* PASS = "";

const char* ntpServer = "pool.ntp.org";
const int32_t gmtOffsetSec = (5 * 60 * 60) + (30 * 60); // India: +5:30 GMT
const int32_t dayLightOffsetSec = 0;
// ----------------------------------------------------------------------

// Custom Types
struct IMUdata {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;
};

struct Date {
  unsigned int YYYY : 16;
  unsigned int MM : 8;
  unsigned int DD : 8;
};

struct Time {
  unsigned int hh : 8;
  unsigned int mm : 8;
  unsigned int ss : 8;
};

class TimeManager {
private:
  Date m_Date;
  Time m_Time;

public:
  void setupTime() {
    Serial.printf("Connecting to: %s\r\n", SSID);
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(".");
      delay(1500);
    }
    Serial.println("Connected Successfully");
    configTime(gmtOffsetSec, dayLightOffsetSec, ntpServer);
  };
  void updateDateTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to Obtain Time: Try Resetting");
      while (1) {
      };
    } else {
      m_Date.YYYY = 1900 + timeinfo.tm_year;
      m_Date.MM = timeinfo.tm_mon;
      m_Date.DD = timeinfo.tm_mday;
      m_Time.hh = timeinfo.tm_hour;
      m_Time.mm = timeinfo.tm_min;
      m_Time.ss = timeinfo.tm_sec;
      Serial.println(&timeinfo, "%A %B %d %Y %H:%M:%S");
    }
  }
  Time getTime() {
    return m_Time;
  }
  Date getDate() {
    return m_Date;
  }
  void getDateStr(char* data) {
    sprintf(data, "%d_%d_%d", m_Date.YYYY, m_Date.MM+1, m_Date.DD); // Apparently Months are 0 indexed
  }
};

class SimulatedQMI8658 {
private:
  bool motionDetected;
  void (*wakeupCallback)();

public:
  bool simulateMotion() {
    if (random(100) < chanceOfMotion && !motionDetected) { // NOTE: 5% chance of detecting Motion
      motionDetected = true;
      if (wakeupCallback) {
        wakeupCallback();
      }
    } else {
      motionDetected = false;
    }
    return motionDetected;
  }

  bool begin() { return true; }

  void setWakeUpMotionEventCallBack(void (*callback)()) {
    wakeupCallback = callback;
  }

  bool readFromFifo(IMUdata *acc, int accCount, IMUdata *gyr, int gyrCount) {
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
    /* TODO: Find a way to do this in Simulation instead of EXTI (i.e., using ULP) */
  }
};


// ----------------------------------------------------------------------

// Mutable Data
RTC_DATA_ATTR size_t bootCounter = 0;

RTC_DATA_ATTR size_t inactiveCounter   = 0;
RTC_DATA_ATTR size_t walkMotionCounter = 0;
RTC_DATA_ATTR size_t runMotionCounter  = 0;
RTC_DATA_ATTR size_t playMotionCounter = 0;

RTC_DATA_ATTR size_t lastLoggingTime_ms = 0;

IMUdata accData[5];
IMUdata gyrData[5];

SimulatedQMI8658 g_qmi8658;
TimeManager g_tm;

int findMaxMagIMU(IMUdata data) {
  int max = 0;
  if (data.x * data.x > max) {
    max = data.x * data.x;
  }
  if (data.y * data.y > max) {
    max = data.y * data.y;
  }
  if (data.z * data.z > max) {
    max = data.z * data.z;
  }
  return max;
}

void simulatorWakeUpCallback() {
  bool success = g_qmi8658.readFromFifo(accData, 5, gyrData, 5);
  if (success) {
    Serial.printf("Recieved Motion: Reading from FIFO\r\n\r\n");
    for (int i = 0; i < 5; i++) {
      Serial.printf("accx[%d]: %d | ", i, accData[i].x);
      Serial.printf("gyrx[%d]: %d\r\n", i, gyrData[i].x);

      Serial.printf("accy[%d]: %d | ", i, accData[i].y);
      Serial.printf("gyry[%d]: %d\r\n", i, gyrData[i].y);

      Serial.printf("accz[%d]: %d | ", i, accData[i].z);
      Serial.printf("gyrz[%d]: %d\r\n", i, gyrData[i].z);

      uint32_t accMxMag = findMaxMagIMU(accData[i]);
      uint32_t gyrMxMag = findMaxMagIMU(gyrData[i]);
      Serial.printf("accMxMag: %d\r\n", accMxMag);
      Serial.printf("gyrMaxMg: %d\r\n", gyrMxMag);

      // Print the Types of Motions
      if (accMxMag > accPlayCriteria || gyrMxMag > gyrPlayCriteria) {
        Serial.printf("[MOTION]: PLAYING\r\n");
        playMotionCounter++;
      } else if (accMxMag > accRunCriteria || gyrMxMag > gyrRunCriteria) {
        Serial.printf("[MOTION]: RUNNING\r\n");
        runMotionCounter++;
      } else {
        Serial.printf("[MOTION]: WALKING\r\n");
        walkMotionCounter++;
      }

      Serial.printf("\r\n");
      delay(500);
    }
  } else {
    Serial.printf("Error: Couldn't Read from FIFO\r\n");
  }
  return;
}

// Func Declarations for LittleFS
void listDir(fs::FS& fs, const char* dirName, uint8_t levels);
void createDir(fs::FS& fs, const char* path);
void readFile(fs::FS& fs, const char* path);
void writeFile(fs::FS& fs, const char* path, const char* msg);
void appendFile(fs::FS& fs, const char* path, const char* msg);
void deleteFile(fs::FS& fs, const char* path);

char lineBuffer[80] = {0x00};

void uploadData() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("Failed to Mount LittleFS");
    while (1) {;;};
  }
  Serial.printf("\r\n\r\n");
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Logging Started");
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  
  listDir(LittleFS, "/", 1);
  
  sprintf(lineBuffer, "%d %d %d %d %d\r\n", g_tm.getTime(), inactiveCounter,
          walkMotionCounter, runMotionCounter, playMotionCounter);
  char path[80] = "";
  char date[40] = "";
  g_tm.getDateStr(date);
  sprintf(path, "/data/%s.DAT", date);
  Serial.printf("lineBuffer: %s\r\n",lineBuffer);
  Serial.printf("path: %s\r\n", path);

  appendFile(LittleFS, path, lineBuffer);
  readFile(LittleFS, path);
  
  inactiveCounter = 0;
  walkMotionCounter = 0;
  runMotionCounter = 0;
  playMotionCounter = 0;
  
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println("All Counters Cleared");
  Serial.println("Logging Completed");
  Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.printf("\r\n\r\n");
}

// ----------------------------------------------------------------------

// Main Function
void setup() {
  Serial.begin(115200);
  ++bootCounter;
  Serial.printf("\r\nBoot Counter: %d\r\n", bootCounter);
  if (g_qmi8658.begin()) {
    Serial.printf("Sensor Simulation Started Successfully\r\n");
    Serial.printf("Percentage Chance of Detecting Motion: %d \r\n",
                  chanceOfMotion);
    Serial.printf("Uploads are Performed every %d cycles\r\n", uploadFrequency);
  }
  esp_sleep_enable_timer_wakeup(timeToWakeUp_us);
  Serial.printf("Deep Sleep Interval: %d us (%f)min\r\n", timeToWakeUp_us,
                timeToWakeUp_us / 1000000 / 60.0);

  g_qmi8658.setWakeUpMotionEventCallBack(&simulatorWakeUpCallback);

  g_tm.setupTime();
  g_tm.updateDateTime();
}

void loop() {
  if (bootCounter % uploadFrequency == 0) {
    Serial.printf("\r\n\r\nCalling UploadData\r\n\r\n");
    uploadData();
  }
  bool motionDetected = g_qmi8658.simulateMotion();
  if (!motionDetected) {
    inactiveCounter++;
    Serial.printf("[MOTION]: INACTIVE\r\n");
    Serial.println("Going to DeepSleep\r\n");
    Serial.printf("INACTIVE_COUNTER: %d\r\n", inactiveCounter);
    Serial.printf("WALKING_COUNTER:  %d\r\n", walkMotionCounter);
    Serial.printf("RUNNING_COUNTER:  %d\r\n", runMotionCounter);
    Serial.printf("PLAYING_COUNTER:  %d\r\n", playMotionCounter);
    Serial.println("-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-");
    esp_deep_sleep_start();
  } else {
    Serial.printf("Going to DeepSleep\r\n");
    Serial.printf("INACTIVE_COUNTER: %d\r\n", inactiveCounter);
    Serial.printf("WALKING_COUNTER:  %d\r\n", walkMotionCounter);
    Serial.printf("RUNNING_COUNTER:  %d\r\n", runMotionCounter);
    Serial.printf("PLAYING_COUNTER:  %d\r\n", playMotionCounter);
    Serial.println("-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-x-");
  }
  
  delay(1000);
}

// Func Defs LittleFS
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
  Serial.printf("\r\n\r\n");
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