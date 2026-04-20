/**
 * WiFi Intrusion Detection System (IDS) for ESP32
 */

#include "config.h"
#include <esp_wifi.h>   // Required for channel switching


// ============================================================================
// SECTION 1: BASELINE HISTORY MANAGEMENT (Rolling Window of 20 Samples)
// ============================================================================
void addToMgmtHistory(float val) {
    mgmtHistory[mgmtIdx] = val;
    mgmtIdx = (mgmtIdx + 1) % 20;
    if (mgmtFilled < 20) mgmtFilled++;
}

void getMgmtBaseline(float &avg, float &stdDev) {
    if (mgmtFilled == 0) { avg = 0; stdDev = 0; return; }
    float sum = 0; for (int i = 0; i < mgmtFilled; i++) sum += mgmtHistory[i];
    avg = sum / mgmtFilled;
    float variance = 0; for (int i = 0; i < mgmtFilled; i++) variance += pow(mgmtHistory[i] - avg, 2);
    variance /= mgmtFilled; stdDev = sqrt(variance);
}

void addToDeauthHistory(float val) {
    deauthHistory[deauthIdx] = val;
    deauthIdx = (deauthIdx + 1) % 20;
    if (deauthFilled < 20) deauthFilled++;
}

void getDeauthBaseline(float &avg, float &stdDev) {
    if (deauthFilled == 0) { avg = 0; stdDev = 0; return; }
    float sum = 0; for (int i = 0; i < deauthFilled; i++) sum += deauthHistory[i];
    avg = sum / deauthFilled;
    float variance = 0; for (int i = 0; i < deauthFilled; i++) variance += pow(deauthHistory[i] - avg, 2);
    variance /= deauthFilled; stdDev = sqrt(variance);
}

void addToProbeHistory(float val) {
    probeHistory[probeIdx] = val;
    probeIdx = (probeIdx + 1) % 20;
    if (probeFilled < 20) probeFilled++;
}

void getProbeBaseline(float &avg, float &stdDev) {
    if (probeFilled == 0) { avg = 0; stdDev = 0; return; }
    float sum = 0; for (int i = 0; i < probeFilled; i++) sum += probeHistory[i];
    avg = sum / probeFilled;
    float variance = 0; for (int i = 0; i < probeFilled; i++) variance += pow(probeHistory[i] - avg, 2);
    variance /= probeFilled; stdDev = sqrt(variance);
}

// ============================================================================
// SECTION 2: HARDWARE INDICATORS (LEDs & Buzzer)
// ============================================================================
void updateHardware() {
    static unsigned long lastBuzzerToggle = 0;
    static bool buzzerState = false;
    unsigned long now = millis();

    if (isCalibrating) {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_YELLOW, HIGH);
        return;
    }

    if (threatScore > 70) {                     
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_YELLOW, LOW);
        
        // BUG FIX: Use Tone instead of digitalWrite to prevent ESP32 Pin Freezing
        if (!buzzerState && (now - lastBuzzerToggle >= 1000)) {
            tone(BUZZER, 2000); // 2kHz alarming pitch
            buzzerState = true;
            lastBuzzerToggle = now;
        } else if (buzzerState && (now - lastBuzzerToggle >= 200)) {
            noTone(BUZZER);
            buzzerState = false;
        }
    } else if (threatScore > 30) {              
        digitalWrite(LED_YELLOW, HIGH);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, LOW);
        noTone(BUZZER);
    } else {                                    
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_YELLOW, LOW);
        noTone(BUZZER);
    }
}

// ============================================================================
// SECTION 3: TRAFFIC ANALYSIS & THREAT DETECTION
// ============================================================================
void analyzeTraffic() {
    threatScore = max(0, threatScore - 15);
    
    // ---- 0. ATOMIC SNAPSHOT (Fixes dropped packets & zeroed dashboard) ----
    portENTER_CRITICAL(&countMux);
    lastPacketCount = packetCount; packetCount = 0;
    lastMgmtCount   = mgmtCount;   mgmtCount   = 0;
    lastDeauthCount = deauthCount; deauthCount = 0;
    
    // -> ADDED: Safely snapshot and reset the separated broadcast counter
    lastBroadcastDeauthCount = broadcastDeauthCount; broadcastDeauthCount = 0; 
    
    lastProbeCount  = probeCount;  probeCount  = 0;
    lastBeaconCount = beaconCount; beaconCount = 0;
    lastAuthCount   = authCount;   authCount   = 0;
    portEXIT_CRITICAL(&countMux);

    // If we are currently scanning, throw away this empty snapshot 
    // and skip the baseline math so we don't record fake "0 packet" drops.
    if (isScanning) {
        return; 
    }
    // <-------------------->

    // ---- 1. CALIBRATION MODE ----
    if (isCalibrating) {
        addToMgmtHistory(lastMgmtCount);
        // lastDeauthCount is now pure frame volume, perfect for the baseline
        addToDeauthHistory(lastDeauthCount); 
        addToProbeHistory(lastProbeCount);
        
        if (mgmtFilled >= 20 && deauthFilled >= 20 && probeFilled >= 20) {
            isCalibrating = false;
        }
        return;
    }

    // ---- 2. DETECTION MODE ----
    if (lastPacketCount > packetThreshold) {
        threatScore += 20;
    }

    float mgmtAvg, mgmtStdDev, deauthAvg, deauthStdDev, probeAvg, probeStdDev;
    getMgmtBaseline(mgmtAvg, mgmtStdDev);
    getDeauthBaseline(deauthAvg, deauthStdDev);
    getProbeBaseline(probeAvg, probeStdDev);

    if (mgmtStdDev < 2)   mgmtStdDev = 2;
    if (probeStdDev < 5)  probeStdDev = 5;
    if (deauthStdDev < 1) deauthStdDev = 1;

    bool mgmtAnomaly   = (mgmtFilled >= 5)   && (lastMgmtCount   > mgmtAvg   + burstMultiplier * mgmtStdDev);
    bool deauthAnomaly = (deauthFilled >= 5) && (lastDeauthCount > deauthAvg + burstMultiplier * deauthStdDev);
    bool probeAnomaly  = (probeFilled >= 5)  && (lastProbeCount  > probeAvg  + burstMultiplier * probeStdDev);

    if (!mgmtAnomaly)   addToMgmtHistory(lastMgmtCount);
    if (!deauthAnomaly) addToDeauthHistory(lastDeauthCount);
    if (!probeAnomaly)  addToProbeHistory(lastProbeCount);

    float mgmtZ = 0, probeZ = 0, deauthZ = 0;
    if (mgmtStdDev > 0)   mgmtZ   = (lastMgmtCount   - mgmtAvg)   / mgmtStdDev;
    if (probeStdDev > 0)  probeZ  = (lastProbeCount  - probeAvg)  / probeStdDev;
    if (deauthStdDev > 0) deauthZ = (lastDeauthCount - deauthAvg) / deauthStdDev;

    mgmtZ   = max(0.0f, mgmtZ);
    probeZ  = max(0.0f, probeZ);
    deauthZ = max(0.0f, deauthZ);

    if (mgmtZ > burstMultiplier) {
        int score = min(40, (int)((mgmtZ - burstMultiplier) * 12));
        threatScore += score;
    }

    if (probeZ > burstMultiplier && lastProbeCount > 30) {
        int score = min(40, (int)((probeZ - burstMultiplier) * 10));
        threatScore += score;
    }

    if (deauthZ > burstMultiplier) {
        int score = min(45, (int)((deauthZ - burstMultiplier) * 35));
        threatScore += score;
    }

    // -> ADDED: Independent threat scoring for Broadcast Deauths
    // A single broadcast deauth is a very strong indicator of a widespread attack.
    if (lastBroadcastDeauthCount > 0) {
        // Adds 20 points per broadcast frame seen, capped at 50 to prevent overflow
        int score = min(50, (int)(lastBroadcastDeauthCount * 20)); 
        threatScore += score;
    }

    // --- 2.5 Heuristic fixed-pattern detectors ---
    if (lastMgmtCount > mgmtAvg * 4 && lastMgmtCount > 120 && lastProbeCount < 20) {
        threatScore += 25;
    }

    if (lastProbeCount > probeAvg * 4 && lastProbeCount > probeThreshold) {
        threatScore += 30;
    }

    if (lastMgmtCount > mgmtAvg * 5 && lastMgmtCount > 150) {
        threatScore += 35;
    }

    if (deauthFilled < 5 && lastDeauthCount > deauthThreshold) {
        threatScore += 40;
    }

    deauthRateHistory[deauthRateIndex] = lastDeauthCount;
    deauthRateIndex = (deauthRateIndex + 1) % 5;
    int totalDeauths = 0;
    for (int i = 0; i < 5; i++) totalDeauths += deauthRateHistory[i];
    if (totalDeauths > (deauthThreshold * stormMultiplier)) {
        int excess = totalDeauths - (deauthThreshold * stormMultiplier);
        int add = 50 + min(30, excess * 2);
        threatScore += add;
    }

    if (lastBeaconCount > beaconThreshold) {
        threatScore += 30;
    }

    if (lastAuthCount > 50) {
        threatScore += 50;
    }

    if (threatScore > 100) threatScore = 100;
    
    updateHardware();
}

String getStatus() {
    if (isCalibrating)    return "CALIBRATING";
    if (threatScore > 70) return "ATTACK";
    if (threatScore > 30) return "WARNING";
    return "SAFE";
}