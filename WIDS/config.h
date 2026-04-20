#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// RAW WI-FI STRUCTURES
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration;
  uint8_t addr1[6];      
  uint8_t addr2[6];      
  uint8_t addr3[6];      
  uint16_t sequence_ctrl;
  uint8_t addr4[6];      
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0];    
} __attribute__((packed)) wifi_ieee80211_packet_t;

// HARDWARE PINS
#define LED_GREEN   2
#define LED_YELLOW  4
#define LED_RED     5
#define BUZZER      15

extern const char *ssid;
extern const char *password;

// SENSITIVITY ENGINE PARAMETERS
extern int sensitivity;         
extern int probeThreshold;       
extern int deauthThreshold;      
extern int beaconThreshold;      
extern int packetThreshold;      
extern float burstMultiplier;    
extern float stormMultiplier;    

// GLOBAL COUNTERS (volatile)
extern volatile uint32_t packetCount;
extern volatile uint32_t mgmtCount;
extern volatile uint32_t deauthCount;
extern volatile uint32_t probeCount;
extern volatile uint32_t beaconCount;
extern volatile uint32_t authCount;
extern volatile uint32_t deauthCount;
extern volatile uint32_t broadcastDeauthCount; 

// Snapshots of the previous analysis window (FIXED)
extern uint32_t lastPacketCount;
extern uint32_t lastMgmtCount;
extern uint32_t lastDeauthCount;
extern uint32_t lastProbeCount;
extern uint32_t lastBeaconCount;
extern uint32_t lastAuthCount;
extern uint32_t lastBroadcastDeauthCount;

// SYSTEM STATE & TIMERS
extern unsigned long lastAnalysisTime;   
extern unsigned long lastChannelScan;    
extern unsigned long alarmTimer;         

extern int threatScore;                  
extern int currentChannel;               
extern bool isCalibrating;
extern bool isScanning;               

// Buffer for JSON responses (FIXED SIZE)
extern char jsonBuffer[512];

// BASELINE HISTORY ARRAYS
extern float mgmtHistory[20];
extern int mgmtIdx;
extern int mgmtFilled;

extern float deauthHistory[20];
extern int deauthIdx;
extern int deauthFilled;

extern float probeHistory[20];
extern int probeIdx;
extern int probeFilled;

extern int deauthRateHistory[5];
extern int deauthRateIndex;

// ESP32 & LIBRARY INSTANCES
extern WebServer server;          
extern Preferences prefs;         
extern portMUX_TYPE countMux;     

// FUNCTION PROTOTYPES
void sniffer(void *buf, wifi_promiscuous_pkt_type_t type);
void analyzeTraffic();
void calibrateToTarget(int targetChannel);
void updateHardware();
String getStatus();
void loadThresholds();
void saveThresholds();
void applySensitivity();
void addToMgmtHistory(float val);
void getMgmtBaseline(float &avg, float &stdDev);
void addToDeauthHistory(float val);
void getDeauthBaseline(float &avg, float &stdDev);
void addToProbeHistory(float val);
void getProbeBaseline(float &avg, float &stdDev);
void startWebServer();
void updateJSON();
String getScanResults();   

#endif