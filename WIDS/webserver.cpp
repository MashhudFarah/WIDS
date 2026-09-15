#include "config.h"

// ================= WEB DASHBOARD HTML (CYBERPUNK ESP32) =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=yes">
<title>Neural WIDS | ESP32 Dashboard</title>
<style>
  :root {
    --bg-main: #07070a;
    --bg-card: rgba(15, 20, 25, 0.85);
    --primary: #00ffcc;
    --primary-dim: rgba(0, 255, 204, 0.2);
    --safe: #00ff00;
    --warning: #ffcc00;
    --danger: #ff0033;
    --calibrating: #00ccff;
    --text-main: #e0e0e0;
    --text-muted: #8899aa;
  }

  * { margin: 0; padding: 0; box-sizing: border-box; }
  
  body { 
    background: radial-gradient(circle at center, #12151f 0%, var(--bg-main) 100%); 
    color: var(--text-main); 
    font-family: system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif; 
    padding: 20px; 
    min-height: 100vh;
  }

  h1 { 
    text-align: center; 
    font-size: 28px; 
    letter-spacing: 4px; 
    margin-bottom: 30px; 
    color: var(--primary);
    text-shadow: 0 0 15px var(--primary-dim); 
    font-weight: 800;
  }

  .dashboard { 
    display: grid; 
    grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); 
    gap: 20px; 
    max-width: 1200px; 
    margin: 0 auto; 
  }

  .card { 
    background: var(--bg-card); 
    border: 1px solid rgba(0, 255, 204, 0.15); 
    border-radius: 12px; 
    padding: 20px; 
    box-shadow: 0 8px 32px rgba(0,0,0,0.5); 
    backdrop-filter: blur(4px);
    display: flex;
    flex-direction: column;
  }

  .card h2 { 
    font-size: 14px; 
    letter-spacing: 2px;
    margin-bottom: 15px; 
    color: var(--text-muted); 
    text-transform: uppercase;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .status-box { 
    text-align: center; 
    font-size: 36px; 
    font-weight: 900; 
    letter-spacing: 2px;
    padding: 20px; 
    border-radius: 8px; 
    margin-bottom: 20px; 
    transition: all 0.3s ease; 
    text-transform: uppercase;
  }
  
  .status-safe { background: rgba(0, 255, 0, 0.05); color: var(--safe); border: 1px solid var(--safe); box-shadow: inset 0 0 20px rgba(0, 255, 0, 0.1); }
  .status-warning { background: rgba(255, 204, 0, 0.05); color: var(--warning); border: 1px solid var(--warning); box-shadow: inset 0 0 20px rgba(255, 204, 0, 0.1); }
  .status-attack { background: rgba(255, 0, 51, 0.1); color: var(--danger); border: 1px solid var(--danger); box-shadow: 0 0 20px rgba(255, 0, 51, 0.4); animation: pulseAlert 1s infinite alternate; }
  .status-calibrating { background: rgba(0, 204, 255, 0.1); color: var(--calibrating); border: 1px solid var(--calibrating); box-shadow: inset 0 0 20px rgba(0, 204, 255, 0.2); animation: pulseCal 1.5s infinite; }

  @keyframes pulseAlert { 
    0% { box-shadow: 0 0 15px rgba(255, 0, 51, 0.3); } 
    100% { box-shadow: 0 0 35px rgba(255, 0, 51, 0.7); border-color: #ff4d4d; } 
  }
  @keyframes pulseCal { 
    0% { opacity: 1; } 
    50% { opacity: 0.6; } 
    100% { opacity: 1; } 
  }

  .info-row {
    display: flex;
    justify-content: space-between;
    padding: 8px 0;
    border-bottom: 1px solid rgba(255,255,255,0.05);
    font-family: 'Courier New', monospace;
    font-size: 14px;
  }

  .control-group { margin-bottom: 15px; }
  .control-group label { display: flex; justify-content: space-between; font-size: 12px; color: var(--text-muted); margin-bottom: 8px; text-transform: uppercase;}
  
  select {
    width: 100%;
    background: #1a1f2b;
    border: 1px solid var(--primary-dim);
    color: var(--primary);
    padding: 10px;
    border-radius: 5px;
    font-family: monospace;
    outline: none;
    margin-bottom: 15px;
  }

  button {
    width: 100%;
    background: var(--primary);
    color: #000;
    border: none;
    padding: 12px;
    border-radius: 5px;
    font-weight: bold;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 1px;
    transition: 0.3s;
    margin-bottom: 10px;
  }
  button:hover { background: #fff; box-shadow: 0 0 20px var(--primary); }
  button:active { transform: scale(0.98); }

  input[type=range] {
    -webkit-appearance: none;
    width: 100%;
    background: transparent;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    height: 16px;
    width: 16px;
    border-radius: 50%;
    background: var(--primary);
    cursor: pointer;
    margin-top: -6px;
    box-shadow: 0 0 10px var(--primary);
  }
  input[type=range]::-webkit-slider-runnable-track {
    width: 100%;
    height: 4px;
    cursor: pointer;
    background: rgba(255,255,255,0.1);
    border-radius: 2px;
  }

  .stats-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; }
  .stat-box { 
    background: rgba(0,0,0,0.4); 
    padding: 15px 10px; 
    border-radius: 8px; 
    text-align: center; 
    border: 1px solid rgba(255,255,255,0.05); 
    transition: 0.2s;
  }
  .stat-box:hover { border-color: var(--primary-dim); background: rgba(0,255,204,0.05); }
  .stat-val { font-family: 'Courier New', monospace; font-size: 24px; font-weight: bold; color: #fff; margin-bottom: 4px; }
  .stat-label { font-size: 10px; color: var(--text-muted); letter-spacing: 1px; }

  .chart-container { 
    width: 100%; 
    height: 220px; 
    margin-top: 10px; 
    background: rgba(0,0,0,0.2); 
    border-radius: 8px; 
    position: relative; 
    padding: 10px;
    border: 1px solid rgba(255,255,255,0.05);
  }
  canvas { width: 100%; height: 100%; display: block; }
  
  .col-span-2 { grid-column: span 2; }
  @media (max-width: 900px) { .col-span-2 { grid-column: span 1; } }

</style>
</head>
<body>

<h1>⚡ NEURAL WIDS DASHBOARD</h1>

<div class="dashboard">
  
  <div class="card">
    <h2>System Status <span id="uptime" style="color:var(--primary); font-family:monospace;">0s</span></h2>
    <div id="statusCard" class="status-box status-safe">SAFE</div>
    
    <div class="info-row"><span>Operating Channel:</span> <span id="channel" style="color:#fff;">--</span></div>
    <div class="info-row"><span>Threat Score:</span> <span id="threatScore" style="color:#fff;">0%</span></div>
  </div>

  <div class="card">
    <h2>Mission Control</h2>
    
    <div class="control-group">
      <label>Target Network (SSID / Channel)</label>
      <button id="btnScan" style="background: var(--warning); color: #000; margin-bottom: 10px;">📡 SCAN NEARBY NETWORKS</button>
      <select id="chanSelect">
        <option value="1">Channel 1 (Default)</option>
        <option value="6">Channel 6</option>
        <option value="11">Channel 11</option>
      </select>
    </div>
    
    <button id="btnLock">LOCK TARGET</button>

    <div class="control-group" style="margin-top: 20px;">
      <div style="display: flex; justify-content: space-between; margin-bottom: 10px; font-size: 12px;">
        <span style="color: var(--text-muted);">Low (Stable)</span>
        <span id="sensVal" style="color: var(--primary); font-weight: bold;">50%</span>
        <span style="color: var(--text-muted);">High (Paranoid)</span>
      </div>
      <input type="range" id="sensSlider" min="1" max="100" value="50" style="width: 100%;">
    </div>

  </div>

  <div class="card col-span-2">
    <h2>Traffic Analysis (Packets / Sec)</h2>
    <div class="chart-container">
      <canvas id="trafficGraph"></canvas>
    </div>
  </div>

  <div class="card col-span-2">
    <h2>Traffic Stats (Last Window)</h2>
    <div class="stats-grid">
      <div class="stat-box"><div class="stat-val" id="mgmt">0</div><div class="stat-label">MGMT FRAMES</div></div>
      <div class="stat-box"><div class="stat-val" id="deauth">0</div><div class="stat-label">DEAUTHS</div></div>
      <div class="stat-box"><div class="stat-val" id="probe">0</div><div class="stat-label">PROBES</div></div>
      <div class="stat-box"><div class="stat-val" id="beacon">0</div><div class="stat-label">BEACONS</div></div>
      <div class="stat-box"><div class="stat-val" id="auth">0</div><div class="stat-label">AUTHS</div></div>
    </div>
  </div>

</div>

<script>
let historyData = new Array(40).fill(0);
const canvas = document.getElementById('trafficGraph');
const ctx = canvas.getContext('2d');

let fetchInterval = setInterval(fetchData, 2500);
let isDraggingSlider = false;

const sensSlider = document.getElementById('sensSlider');
const sensVal = document.getElementById('sensVal');

sensSlider.addEventListener('mousedown', () => isDraggingSlider = true);
sensSlider.addEventListener('touchstart', () => isDraggingSlider = true, {passive: true});
sensSlider.addEventListener('mouseup', () => isDraggingSlider = false);
sensSlider.addEventListener('touchend', () => isDraggingSlider = false);

// ADD KEYBOARD SUPPORT
sensSlider.addEventListener('keydown', () => isDraggingSlider = true);
sensSlider.addEventListener('keyup', (e) => {
  isDraggingSlider = false;
  // Trigger the save event immediately after letting go of the arrow key
  fetch(`/control?sens=${e.target.value}`).catch(err => console.log('Control Error:', err));
});
const btnLock = document.getElementById('btnLock');
const btnScan = document.getElementById('btnScan');

// Scan network logic
btnScan.addEventListener('click', () => {
  btnScan.innerText = "⏳ SCANNING AIRWAVES...";

  clearInterval(fetchInterval);

  fetch('/scan')
    .then(res => res.json())
    .then(data => {
      const select = document.getElementById('chanSelect');
      select.innerHTML = ''; 
      
      if(data.networks.length === 0) {
          select.innerHTML = '<option value="1">No networks found (Default Ch 1)</option>';
      } else {
          data.networks.forEach(net => {
            const opt = document.createElement('option');
            opt.value = net.ch;
            opt.innerText = `${net.ssid} (CH: ${net.ch} | ${net.rssi}dBm)`;
            select.appendChild(opt);
          });
      }
      
      btnScan.innerText = "✅ SCAN COMPLETE";
      setTimeout(() => { btnScan.innerText = "📡 SCAN NEARBY NETWORKS"; }, 3000);
      fetchInterval = setInterval(fetchData, 1000);
    })
    .catch(e => {
      btnScan.innerText = "❌ SCAN FAILED";
      setTimeout(() => { btnScan.innerText = "📡 SCAN NEARBY NETWORKS"; }, 3000);
      fetchInterval = setInterval(fetchData, 1000);
      console.log('Scan Error:', e);
    });
});

// Send target lock request (MERGED WITH ALERT)
btnLock.addEventListener('click', () => {
  const ch = document.getElementById('chanSelect').value;
  
  // Warn the user about the hardware limitation
  alert("Switching channels! Your device will temporarily disconnect from the ESP32. Please wait 5-10 seconds for your Wi-Fi to reconnect, then the dashboard will resume.");
  
  // Send the correct calibration request
  fetch(`/calibrate?ch=${ch}`)
    .then(() => console.log('Calibrating to Channel ' + ch))
    .catch(e => console.log('Calibration Error:', e));
});

// Send request to ESP32 when dragging is finished
sensSlider.addEventListener('change', (e) => {
  fetch(`/control?sens=${e.target.value}`).catch(e => console.log('Control Error:', e));
});


// Send request to ESP32 when dragging is finished
sensSlider.addEventListener('change', (e) => {
  fetch(`/control?sens=${e.target.value}`).catch(e => console.log('Control Error:', e));
});

function resizeCanvas() {
  canvas.width = canvas.parentElement.clientWidth - 20;
  canvas.height = canvas.parentElement.clientHeight - 20;
}

function drawGraph() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  
  let trueMax = Math.max(...historyData);
  // Cap the graph ceiling at 300. Normalizes noise without squashing spikes!
  let maxVal = Math.max(50, Math.min(trueMax, 300)); 
  
  let stepX = canvas.width / (historyData.length - 1);
  
  let gradient = ctx.createLinearGradient(0, 0, 0, canvas.height);
  gradient.addColorStop(0, 'rgba(0, 255, 204, 0.4)');
  gradient.addColorStop(1, 'rgba(0, 255, 204, 0.0)');

  // Draw Fill
  ctx.beginPath();
  ctx.moveTo(0, canvas.height);
  for (let i = 0; i < historyData.length; i++) {
    let x = i * stepX;
    // Clamp the display value so it doesn't draw outside the canvas top
    let displayVal = Math.min(historyData[i], maxVal); 
    let y = canvas.height - ((displayVal / maxVal) * canvas.height * 0.85);
    ctx.lineTo(x, y);
  }
  ctx.lineTo(canvas.width, canvas.height);
  ctx.fillStyle = gradient;
  ctx.fill();

  // Draw Line
  ctx.beginPath();
  ctx.strokeStyle = '#00ffcc';
  ctx.lineWidth = 2;
  for (let i = 0; i < historyData.length; i++) {
    let x = i * stepX;
    let displayVal = Math.min(historyData[i], maxVal);
    let y = canvas.height - ((displayVal / maxVal) * canvas.height * 0.85);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function updateGraph(newVal) {
  historyData.push(newVal);
  historyData.shift();
  drawGraph();
}

function updateStatusUI(status, threat) {
  const card = document.getElementById('statusCard');
  
  card.innerText = status;
  card.className = 'status-box'; 
  
  if (status === 'SAFE') {
    card.classList.add('status-safe');
  } 
  else if (status === 'WARNING') {
    card.classList.add('status-warning');
  } 
  else if (status === 'CALIBRATING') {
    card.classList.add('status-calibrating');
  }
  else {
    card.classList.add('status-attack');
  }

  document.getElementById('threatScore').innerText = threat + '%';
}

function fetchData() {
  fetch('/stats')
    .then(res => res.json())
    .then(data => {
      document.getElementById('mgmt').innerText = data.mgmt;
      document.getElementById('deauth').innerText = data.deauth;
      document.getElementById('probe').innerText = data.probe;
      document.getElementById('beacon').innerText = data.beacon;
      document.getElementById('auth').innerText = data.auth;

      document.getElementById('channel').innerText = data.channel;
      document.getElementById('uptime').innerText = data.uptime + "s";

      if (!isDraggingSlider) {  // <-- Replaced document.activeElement check
  sensSlider.value = data.sensitivity;
  sensVal.innerText = data.sensitivity + '%';
}

      updateStatusUI(data.status, data.threat);
      updateGraph(data.mgmt);
    })
    .catch(err => {
      // 1. Show the user we lost connection
      updateStatusUI('DISCONNECTED', 0);
      
      // 2. Dash out the stats so they don't look like live numbers
      document.getElementById('mgmt').innerText = '-';
      document.getElementById('deauth').innerText = '-';
      document.getElementById('probe').innerText = '-';
      document.getElementById('beacon').innerText = '-';
      document.getElementById('auth').innerText = '-';
      document.getElementById('channel').innerText = '-';
      document.getElementById('uptime').innerText = '0s';
      
      // 3. Flatline the graph
      historyData.fill(0);
      drawGraph();
    });
}

window.addEventListener('resize', resizeCanvas);
resizeCanvas();
fetchData();
</script>
</body>
</html>
)rawliteral";

// ================= PREFERENCES (Storage) =================
void loadThresholds() {
  prefs.begin("wids", true);
  sensitivity = prefs.getInt("sens", 50); 
  prefs.end();
}

void saveThresholds() {
  prefs.begin("wids", false);
  prefs.putInt("sens", sensitivity);
  prefs.end();
}

// ================= JSON GENERATION =================
void updateJSON() {
  // Use a 512 byte document so it doesn't overflow!
  StaticJsonDocument<512> doc;

  // Use the snapshots generated by detection.cpp
  doc["mgmt"]    = lastMgmtCount;
  doc["deauth"]  = lastDeauthCount;
  doc["probe"]   = lastProbeCount;
  doc["beacon"]  = lastBeaconCount;
  doc["auth"]    = lastAuthCount;
  
  doc["status"]      = getStatus(); 
  doc["threat"]      = threatScore;
  doc["channel"]     = currentChannel;
  doc["uptime"]      = millis() / 1000;
  doc["sensitivity"] = sensitivity;

  serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
}

// ================= SERVER SETUP =================
void startWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });

  server.on("/stats", []() {
    server.send(200, "application/json", String(jsonBuffer));
  });

  server.on("/control", []() {
  if (server.hasArg("sens")) {
    int val = server.arg("sens").toInt();
    if (val >= 1 && val <= 100) {   
      sensitivity = val;
      applySensitivity();
      saveThresholds();
    }
  }
  server.send(200, "text/plain", "OK");
});

  server.on("/calibrate", []() {
  if (server.hasArg("ch")) {
    int targetChannel = server.arg("ch").toInt();
    if (targetChannel >= 1 && targetChannel <= 13) {  // ← ADD THIS GUARD
      calibrateToTarget(targetChannel);
    }
  }
  server.send(200, "text/plain", "CALIBRATING");
});

  // NEW SCAN ENDPOINT
  server.on("/scan", []() {
    server.send(200, "application/json", getScanResults());
  });

  server.begin();
}