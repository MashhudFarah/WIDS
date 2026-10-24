# WIDS-ESP32

A standalone WiFi intrusion detection system on a single ESP32. It listens to
802.11 management frames in promiscuous mode, learns what normal traffic looks
like on your channel, and flags deauth floods, beacon spam and probe storms in
real time. No cloud, no companion app, no internet.

## What it does

It captures every management frame in range — probe, beacon, auth, deauth,
disassoc — not just traffic addressed to it. On boot it spends about 20 seconds
calibrating: watching the channel and building a rolling baseline (average and
standard deviation) per frame type. After that it scores how far current traffic
has drifted from that baseline using a z-score, rather than flat thresholds.

Every hit adds weighted points to one threat score from 0 to 100, which decays
15 points a second — so it reports current conditions instead of staying stuck
on "attack". Green, yellow and red LEDs plus a buzzer give the status without a
screen. A sensitivity slider remaps every threshold at once and survives a
reboot. The web server runs in its own FreeRTOS task on Core 0, so serving the
dashboard never blocks packet capture or trips the watchdog.

## How it works

```text
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

## What it detects

| Attack | How it's recognised |
|---|---|
| Deauth / disassoc flood | Burst z-score above baseline, plus a 5-second storm counter weighted by `stormMultiplier` |
| Broadcast deauth | Any deauth addressed to `FF:FF:FF:FF:FF:FF` is scored on its own — one is a strong signal |
| Beacon flood / fake AP spam | Beacon count exceeds the sensitivity-scaled `beaconThreshold` |
| Probe request storm | Probe volume spikes 4x above its learned baseline |
| Auth flood | Raw auth frame count exceeds a fixed ceiling |
| General anomaly | Overall management frame volume drifts past `burstMultiplier` standard deviations |

| Threat score | Status | LED |
|---|---|---|
| 0–30 | `SAFE` | Green |
| 31–70 | `WARNING` | Yellow |
| 71–100 | `ATTACK` | Red + buzzer |

## Hardware

| Component | ESP32 pin | Purpose |
|---|---|---|
| Green LED | `GPIO 2` | Safe |
| Yellow LED | `GPIO 4` | Calibrating / warning |
| Red LED | `GPIO 5` | Attack |
| Buzzer | `GPIO 15` | Audible alarm |

Any standard ESP32 dev board works. It uses the onboard radio, so no external
WiFi hardware is needed.

## Getting started

You need an ESP32 dev board, the Arduino IDE with the ESP32 board package
installed, and the `ArduinoJson` library.

```bash
git clone https://github.com/MashhudFarah/WIDS-ESP32.git
```

1. Open `WIDS.ino` in the Arduino IDE
2. Select your board and port under **Tools**
3. Upload
4. Open the Serial Monitor at `115200` to watch boot logs and heartbeats

Then join the `ESP32-IDS` access point and open `http://192.168.4.1`. The
default password is in `WIDS.ino` — change it before you use this anywhere real.

## API

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Dashboard HTML |
| `/stats` | GET | Live JSON: `mgmt`, `deauth`, `probe`, `beacon`, `auth`, `status`, `threat`, `channel`, `uptime`, `sensitivity` |
| `/control?sens=<1-100>` | GET | Set sensitivity, persisted to flash |
| `/calibrate?ch=<1-13>` | GET | Switch channel and restart calibration |
| `/scan` | GET | Pause sniffing, scan nearby networks, return SSID/channel/RSSI |

## Project structure

```text
WIDS/
├── WIDS.ino        # Setup, main loop, sensitivity engine, core pinning
├── config.h        # Pins, shared globals, prototypes
├── sniffer.cpp     # Promiscuous frame capture and classification
├── detection.cpp   # Baseline maths, z-score scoring, threat score
└── webserver.cpp   # Dashboard, JSON API, REST endpoints
```

## Tested on

- ESP32 DevKit v1 (30-pin), Arduino core 2.x
- Deauth floods and beacon spam, launched from a Kali Linux machine and a
  second ESP32 against a test access point we controlled
- Channels 1, 6 and 11
- Our own network only

## Limits

- It only reads management frames. It cannot see anything inside encrypted
  data traffic.
- The baseline is per-channel and per-environment. Move it somewhere new and it
  needs to recalibrate.
- It cannot tell a legitimate deauth (a router kicking a client) from a
  malicious one. It reports volume, not intent.
- No logging or history — the dashboard shows current state only.

## Disclaimer

Built for educational and defensive research only. It passively monitors
traffic and does not transmit attacks. Using it on networks you do not own or
have permission to test is prohibited and may be illegal.

## Credits

Built with [Shafayet22](https://github.com/Shafayet22).

I wrote the firmware — the promiscuous-mode sniffer, the detection and scoring
logic in `detection.cpp`, the web server, and the LED and buzzer wiring.

Shafayet22 tested it end to end: a Kali Linux machine and a second ESP32 firing
deauth floods and beacon spam at a test access point, then confirming the
dashboard and LEDs responded. That testing is how the detection thresholds got
set.

His repository: https://github.com/Shafayet22/WIDS-ESP32

## AI

I used AI tools to help write and debug the firmware. I ran everything myself
and fixed what broke.

## License

MIT — see [LICENSE](LICENSE).
