# 🛡️ WIDS-ESP32

**A standalone WiFi Intrusion Detection System that fits in your pocket.**

Built on a single ESP32, WIDS-ESP32 sniffs 802.11 management traffic in promiscuous mode, scores it against a self-learned statistical baseline, and flags deauth floods, evil-twin-style probe storms, and beacon spam in real time — no cloud, no companion app, no internet connection required.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-active-brightgreen)

---

## ✨ Features

- 📡 **Promiscuous-mode sniffing** — captures every 802.11 management frame in range (probe, beacon, auth, deauth, disassoc), not just traffic addressed to it
- 📊 **Self-calibrating baseline** — learns normal traffic for your environment over a 20-sample rolling window before it starts detecting, instead of relying on hardcoded "normal" values
- 🧠 **Statistical anomaly scoring** — flags bursts using z-score deviation from the learned baseline (mean + standard deviation) rather than flat thresholds alone
- 🚨 **Weighted threat score (0–100)** — combines burst detection, deauth storm tracking, and broadcast-deauth heuristics into a single live score with automatic decay
- 💡 **Physical alerting** — RGB LED status (green / yellow / red) plus an active buzzer alarm on high-threat states, no screen required
- 🌐 **Live web dashboard** — self-hosted at `192.168.4.1`, no internet needed, auto-refreshing charts and stats
- 🎚️ **Adjustable sensitivity** — one slider remaps every detection threshold at once, persisted to flash across reboots
- 📶 **Channel scanner & calibration** — rescan and re-lock onto a specific WiFi channel on demand, pausing detection safely while it does
- ⚙️ **Dual-core stable** — the web server runs in its own FreeRTOS task pinned to Core 0, so serving dashboard requests never blocks packet capture or trips the watchdog

---

## 🧩 How It Works

```
 802.11 frames (air)
        │
        ▼
┌───────────────────┐      ISR context, Core 1
│   sniffer.cpp      │  →  filters management frames, classifies subtype,
│  (promiscuous cb)  │     increments atomic counters
└───────────────────┘
        │  counters snapshotted every 1s
        ▼
┌───────────────────┐
│  detection.cpp     │  →  rolling baseline (avg + stdDev) per frame type
│ (analyzeTraffic)   │     z-score burst detection + storm/heuristic rules
└───────────────────┘     →  threatScore (0–100)
        │
        ├──────────────► RGB LED + buzzer (updateHardware)
        │
        ▼
┌───────────────────┐      Core 0, own FreeRTOS task
│  webserver.cpp     │  →  serves dashboard + JSON API over the
│                     │     ESP32-IDS access point
└───────────────────┘
```

The ESP32 boots as its own access point (`ESP32-IDS`) and spends the first ~20 seconds **calibrating** — quietly learning what normal probe/beacon/deauth volume looks like on the current channel — before it starts flagging anomalies. This is what keeps the false-positive rate low in busy environments instead of just screaming at every beacon frame that passes by.

---

## 🎯 What It Detects

| Attack | How it's recognized |
|---|---|
| **Deauth / disassoc flood** | Burst z-score above baseline, plus a rolling 5-second storm counter weighted by `stormMultiplier` |
| **Broadcast deauth** | Any deauth frame addressed to `FF:FF:FF:FF:FF:FF` is scored independently — a single one is a strong signal of a mass-deauth attack |
| **Beacon flood / fake AP spam** | Beacon count exceeds the sensitivity-scaled `beaconThreshold` |
| **Probe request storm** | Probe volume spikes 4x+ above its learned baseline |
| **Auth flood** | Raw authentication frame count exceeds a fixed ceiling |
| **General traffic anomaly** | Overall management frame volume deviates from its baseline by more than `burstMultiplier` standard deviations |

Each hit adds weighted points to a single **threat score**, which decays by 15 points every second so the system reports on *current* conditions rather than getting stuck at "attack" forever.

| Threat Score | Status | LED |
|---|---|---|
| 0–30 | `SAFE` | 🟢 Green |
| 31–70 | `WARNING` | 🟡 Yellow |
| 71–100 | `ATTACK` | 🔴 Red + buzzer |

---

## 🔧 Hardware

| Component | ESP32 Pin | Purpose |
|---|---|---|
| Green LED | `GPIO 2` | Safe status |
| Yellow LED | `GPIO 4` | Calibrating / warning |
| Red LED | `GPIO 5` | Attack detected |
| Buzzer | `GPIO 15` | Audible alarm |

Any standard ESP32 dev board works — no external WiFi hardware needed, since it uses the onboard radio in promiscuous mode.

---

## 🚀 Getting Started

### Requirements
- ESP32 dev board
- [Arduino IDE](https://www.arduino.cc/en/software) with the [ESP32 board package](https://github.com/espressif/arduino-esp32) installed
- Libraries (installable via Library Manager): `ArduinoJson`, `Preferences` (bundled with ESP32 core)

### Flash it

```bash
git clone https://github.com/MashhudFarah/WIDS.git
```

1. Open `WIDS.ino` in the Arduino IDE
2. Select your ESP32 board and port under **Tools**
3. Hit **Upload**
4. Open the Serial Monitor at `115200` baud to watch boot logs and live heartbeat stats

### Connect to the dashboard

1. On your phone or laptop, scan for WiFi networks
2. Connect to **`ESP32-IDS`** (password: `12345678`)
3. Open `http://192.168.4.1` in a browser
4. Watch the threat score, live packet counts, and charts update in real time

No internet connection is required — the ESP32 serves everything locally.

---

## 📡 API Reference

The dashboard talks to the ESP32 over a small local JSON API — you can hit these directly too:

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Serves the dashboard HTML |
| `/stats` | GET | Returns live JSON: `mgmt`, `deauth`, `probe`, `beacon`, `auth`, `status`, `threat`, `channel`, `uptime`, `sensitivity` |
| `/control?sens=<1-100>` | GET | Updates detection sensitivity and persists it to flash |
| `/calibrate?ch=<1-13>` | GET | Switches WiFi channel and restarts the calibration baseline |
| `/scan` | GET | Pauses sniffing, scans nearby networks, returns SSID/channel/RSSI as JSON |

Example `/stats` response:
```json
{
  "mgmt": 42,
  "deauth": 0,
  "probe": 8,
  "beacon": 15,
  "auth": 1,
  "status": "SAFE",
  "threat": 12,
  "channel": 6,
  "uptime": 184,
  "sensitivity": 50
}
```

---

## 📂 Project Structure

```
WIDS/
├── WIDS.ino          # Setup, main loop, sensitivity engine, core task pinning
├── config.h           # Pin defs, shared globals, function prototypes
├── sniffer.cpp        # ISR-level promiscuous frame capture & classification
├── detection.cpp       # Baseline math, z-score anomaly detection, threat scoring
└── webserver.cpp       # Dashboard HTML/JS, JSON API, REST endpoints
```

---

## ⚠️ Disclaimer

This project is built for **educational and defensive research purposes only**. It passively monitors traffic in your own environment — it does not transmit attacks. Any misuse to monitor networks you don't own or have permission to test is strictly prohibited and may be illegal in your jurisdiction.

---

## 👥 Credits

Built by [MashhudFarah](https://github.com/MashhudFarah) and [Shafayet22](https://github.com/Shafayet22).

## 📄 License

Released under the [MIT License](LICENSE).
