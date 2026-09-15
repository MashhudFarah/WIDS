#include "config.h"

// ================= WIFI CREDENTIALS =================
const char *ssid = "ESP32-IDS";
const char *password = "12345678";

// ================= DEFINITIONS =================
int sensitivity = 50; 
int probeThreshold;
int deauthThreshold;
int beaconThreshold;
int packetThreshold;
float burstMultiplier;
float stormMultiplier;

portMUX_TYPE countMux = portMUX_INITIALIZER_UNLOCKED;


volatile uint32_t packetCount = 0;
volatile uint32_t mgmtCount = 0;
volatile uint32_t deauthCount = 0;
volatile uint32_t probeCount = 0;
volatile uint32_t beaconCount = 0;
volatile uint32_t authCount = 0;
volatile uint32_t broadcastDeauthCount = 0; 

uint32_t lastPacketCount = 0;
uint32_t lastMgmtCount = 0;
uint32_t lastDeauthCount = 0;
uint32_t lastProbeCount = 0;
uint32_t lastBeaconCount = 0;
uint32_t lastAuthCount = 0;
uint32_t lastBroadcastDeauthCount = 0;

unsigned long lastAnalysisTime = 0;
unsigned long lastChannelScan = 0;
unsigned long alarmTimer = 0;

int threatScore = 0;
int currentChannel = 1;

char jsonBuffer[512] = "{\"status\":\"INIT\"}";

int deauthRateHistory[5] = { 0 };
int deauthRateIndex = 0;

float mgmtHistory[20], deauthHistory[20], probeHistory[20];
int mgmtIdx = 0, mgmtFilled = 0, deauthIdx = 0, deauthFilled = 0, probeIdx = 0, probeFilled = 0;

bool isCalibrating = true; 
bool alarmState = false;
bool isScanning = false;

WebServer server(80);
Preferences prefs;

// ================= WIFI SCANNER =================
String getScanResults() {
  Serial.println("[SYS] Pausing sniffer for Wi-Fi Scan...");
  isScanning = true;
  esp_wifi_set_promiscuous(false); 
  
  int n = WiFi.scanNetworks(false, false, false,120); 
  
  // Increased to 4096 to handle dense urban airspaces safely
  DynamicJsonDocument doc(4096); 
  JsonArray networks = doc.createNestedArray("networks");
  
  // Cap the results to 25 to guarantee we never overflow the JSON buffer
  int maxNetworks = (n > 25) ? 25 : n; 
  for (int i = 0; i < maxNetworks; ++i) {
    JsonObject net = networks.createNestedObject();
    String ssidName = WiFi.SSID(i);
    net["ssid"] = ssidName.isEmpty() ? "<Hidden Network>" : ssidName;
    net["ch"] = WiFi.channel(i);
    net["rssi"] = WiFi.RSSI(i);
  }
  
  String output;
  serializeJson(doc, output);
  
  WiFi.scanDelete(); 
  esp_wifi_set_promiscuous(true); 
  isScanning = false;
  Serial.println("[SYS] Scan complete. Sniffer resumed.");
  
  return output;
}

// ================= MASTER SENSITIVITY ENGINE =================
void applySensitivity() {
  probeThreshold = map(sensitivity, 1, 100, 150, 50);
  deauthThreshold = map(sensitivity, 1, 100, 10, 1);
  beaconThreshold = map(sensitivity, 1, 100, 500, 200);
  packetThreshold = map(sensitivity, 1, 100, 1000, 200);
  burstMultiplier = map(sensitivity, 1, 100, 400, 150) / 100.0;
  stormMultiplier = map(sensitivity, 1, 100, 500, 200) / 100.0;
}

// ================= TARGETING & CALIBRATION =================
void calibrateToTarget(int targetChannel) {
  Serial.printf("[TARGET] Switching to Channel %d and starting calibration...\n", targetChannel);
  
  esp_wifi_set_promiscuous(false);
  currentChannel = targetChannel;
  
  WiFi.softAP(ssid, password, currentChannel);
  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  esp_wifi_set_promiscuous(true);
  
  tone(BUZZER, 3000, 500);
  delay(500);
  
  mgmtFilled = 0; deauthFilled = 0; probeFilled = 0;
  mgmtIdx = 0; deauthIdx = 0; probeIdx = 0;
  threatScore = 0;
  isCalibrating = true;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- BOOTING NEURAL WIDS ---");

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // BUG FIX 1: Removed duplicate prefs.begin()
  Serial.println("[SYS] Loading configurations...");
  loadThresholds(); 
  applySensitivity();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, currentChannel);
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  delay(1000);
  
  Serial.print("[WIFI] AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);

  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_promiscuous(true);
  
  updateJSON();
  startWebServer();
  Serial.println("[SYS] Web Server Started. System READY!");

  // Pin the web server task to Core 0
  xTaskCreatePinnedToCore(
    webServerTask,   /* Task function */
    "WebServer",     /* Name of task */
    4096,            /* Stack size (4KB is plenty for the webserver) */
    NULL,            /* Task input parameter */
    1,               /* Priority of the task */
    NULL,            /* Task handle */
    0                /* Pin task to Core 0 */
  );

}

// ================= LOOP =================
void loop() {
  // REMOVE OR COMMENT OUT THIS LINE:
  // server.handleClient(); 
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastAnalysisTime > 1000) {
    
    // Track calibration state before analysis
    bool wasCalibrating = isCalibrating;
    // Process math & safely snapshot the ISR variables
    analyzeTraffic(); 
    updateJSON();     

    // BUG FIX 2: Correctly checks if calibration just finished during analyzeTraffic()
    if (wasCalibrating) {
      if (isCalibrating) {
        tone(BUZZER, 1500, 50); 
      } else {
        tone(BUZZER, 3000, 400); // ← CHANGED to a single 400ms non-blocking tone
        Serial.println("[SYS] Calibration Complete. Baseline Established.");
      }
    }

    Serial.printf("[HEARTBEAT] Threat: %d%% | Packets: %d | Mgmt: %d | Deauth: %d | Probes: %d\n",
                  threatScore, lastPacketCount, lastMgmtCount, lastDeauthCount, lastProbeCount);
                  
    lastAnalysisTime = currentMillis;
  }

  updateHardware(); 
}

// ================= WEBSERVER TASK (CORE 0) =================
void webServerTask(void *pvParameters) {
  for (;;) {
    server.handleClient();
    
    // vTaskDelay is CRITICAL here. It yields the core to the RTOS watchdog 
    // and background Wi-Fi tasks so the ESP32 doesn't crash.
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}