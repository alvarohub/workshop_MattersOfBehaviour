// M5StampS3: analog mic amplitude -> Dynamixel position.
// The motor carries the mic nearer to / farther from the sound source.
//
// Pin/bus conventions follow MisBKit:
//   MisBkit_firmware/include/Pins.h        (ARDUINO_M5Stack_StampS3 section)
//   MisBkit_firmware/src/motors/motorManager.h/.cpp
//
// Calibration UI: ESP32 runs as WiFi AP "ShyMic" (pw: shymic123).
// Connect, then open http://192.168.4.1
#include <M5Unified.h>
#include <Dynamixel2Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <VL53L1X.h>

// ---------- Interaction modes ----------
// SHY_MIC  : loud singing -> mic runs away (fast attack, slow release)
// ALERT_DOG: motor slowly sweeps "searching" and freezes when it hears sound
// MANUAL   : web slider directly sets the goal position
// EXPLORE  : torque off, position read back while you move the horn by hand
enum class Mode : uint8_t { SHY_MIC = 0, ALERT_DOG, MANUAL, EXPLORE };

struct ModeConfig {
  Mode     mode;
  float    minLevel;       // quiet-room level (0..1)     <- CALIBRATE per mode
  float    maxLevel;       // loud/close-source level     <- CALIBRATE per mode
  uint16_t moveSpeed;      // AX-12A moving speed (0..1023)
  float    release;        // per-window decay (attack is instant)
};

// >>> CHANGE MODE HERE (runtime switching comes later) <<<
//static ModeConfig mode = { Mode::SHY_MIC, 0.0f, 1.0f, 250, 0.92f };
static ModeConfig mode = { Mode::ALERT_DOG, 0.0f, 1.0f, 250, 0.92f };

// Preset for the "alert dog" mode — swap into `mode` above, or tweak:
//   Mode::ALERT_DOG, moveSpeed ~60-120, higher release for a linger-freeze

// ---------- WiFi AP + calibration web UI ----------
#define AP_SSID "ShyMic"
#define AP_PASS "shymic123"
WebServer server(80);

// ---------- Analog microphone (envelope follower) ----------
#define MIC_PIN          5      // pins::analogSensor1 (GPIO5). Alternatives: 7, 9, 11
#define SAMPLE_WINDOW_MS 30     // envelope window length
float quietPpMv = 80;           // peak-to-peak mV in a quiet room       <- CALIBRATE
float loudPpMv  = 1800;         // peak-to-peak mV at loud/close source  <- CALIBRATE

// ---------- Dynamixel bus (MisBKit motorManager conventions) ----------
#define DXL_SERIAL       Serial0  // USB CDC occupies `Serial`; motors use Serial0
#define DXL_RX_PIN       44       // RX1PIN
#define DXL_TX_PIN       43       // TX1PIN
#define DXL_DIR_PIN      41       // pins::motorsControl (stamps3A variant; use 1 on plain stamps3)
#define DXL_BAUD         1000000
#define DXL_PROTOCOL     1.0f     // AX-12A
#define DXL_ID           1        // <- set to your motor's ID

// ---------- Motion range (AX-12A joint units 0..1023) ----------
int posNear = 800;              // mic close to the source   <- CALIBRATE
int posFar  = 200;              // mic far from the source   <- CALIBRATE
#define POS_DEADBAND     3        // ignore goal changes smaller than this
#define LPF_TAU_MS       120      // goal-position low-pass time constant (0 = off)

// ---------- Mode 2 (ALERT_DOG) tuning ----------
int      dogStep     = 3;       // joint units per window while searching
float    dogThresh   = 0.45f;   // level that triggers the freeze (0..1)
uint32_t dogFreezeMs = 1500;    // how long it stays frozen after losing the sound

// ---------- Motor protection ----------
uint16_t torqueLimit = 300;     // AX-12A max torque (0..1023) - caps stall current/heat
#define TEMP_WARN_C   65        // cut torque above this temperature (deg C)
static bool heatCut = false;    // true while over-temperature protection is active

// ---------- ToF4M distance sensor (VL53L1X, I2C 0x29 on Wire GPIO13/15) ----------
VL53L1X tof;
bool     tofReady      = false;
bool     presenceOn    = true;    // SHY_MIC: distance gates bow / freeze / escape
uint16_t presenceMm    = 800;     // person "close" below this distance
int      distMm        = -1;      // last valid reading (-1 = none)
float    bowPos        = 0.5f;    // bowing sweep progress (0..1, normalized)
#define  BOW_STEP      3          // joint units per window while bowing

Dynamixel2Arduino dxl(DXL_SERIAL, DXL_DIR_PIN);

static float smoothed   = 0.0f;
static int   lastGoal   = -1;
static bool  invert     = false;  // SHY_MIC: false: loud -> NEAR ; true: loud -> FAR
static bool  motorReady = false;
static int   manualPos  = 512;    // MANUAL mode goal (follows the horn in EXPLORE)
static int   presentPos = -1;     // last position read back from the motor
static int   presentTemp = -1;    // last temperature read back (deg C)
static float goalFilt   = -1;     // low-pass filtered goal (-1 = uninitialized)

// Apply the safe travel range to the motor's own angle limits + torque cap
static void applyMotorLimits()
{
  if (!motorReady) return;
  int lo = min(posFar, posNear), hi = max(posFar, posNear);
  dxl.writeControlTableItem(ControlTableItem::CW_ANGLE_LIMIT,  DXL_ID, lo);
  dxl.writeControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, DXL_ID, hi);
  dxl.writeControlTableItem(ControlTableItem::TORQUE_LIMIT,    DXL_ID, torqueLimit);
}

// ALERT_DOG state (sweep works in normalized progress 0..1, mapped to posFar..posNear)
static float    dogPos      = 0.5f;
static int      dogDir      = 1;
static uint32_t dogFrozenAt = 0;   // millis() when freeze started (0 = not frozen)
static float    lastPpMv    = 0.0f;

// Peak-to-peak amplitude (mV) over one sampling window
static float micEnvelopePPmV()
{
  uint32_t start = millis();
  uint32_t vmin = UINT32_MAX, vmax = 0;
  while (millis() - start < SAMPLE_WINDOW_MS) {
    uint32_t v = analogReadMilliVolts(MIC_PIN);
    if (v < vmin) vmin = v;
    if (v > vmax) vmax = v;
  }
  return (float)(vmax - vmin);
}

// ---------- Mode behaviors: return the desired goal position ----------

// Mode 1: the shy microphone — singer sings, mic backs away.
// With presence detection ON and nobody close: the arm slowly bows instead.
static int goalShyMic(float level)
{
  if (presenceOn && distMm >= 0 && distMm > presenceMm) {
    float range = (float)abs(posNear - posFar);
    if (range > 0) bowPos += dogDir * (BOW_STEP / range);
    if (bowPos >= 1.0f) { bowPos = 1.0f; dogDir = -1; }
    if (bowPos <= 0.0f) { bowPos = 0.0f; dogDir =  1; }
    return posFar + (int)(bowPos * (posNear - posFar));
  }
  float posF = invert ? (posNear - level * (posNear - posFar))
                      : (posFar  + level * (posNear - posFar));
  return constrain((int)posF, 0, 1023);
}

// Mode 2: the alert dog — slow sweep, freeze when it hears something
static int goalAlertDog(float level)
{
  int goal = posFar + (int)(dogPos * (posNear - posFar));   // normalized -> joint units
  if (level >= dogThresh) {
    dogFrozenAt = millis();              // heard something: stop and listen
    return goal;
  }
  if (dogFrozenAt != 0) {
    if (millis() - dogFrozenAt < dogFreezeMs) return goal;  // still frozen
    dogFrozenAt = 0;                     // lost it — resume searching
  }
  // slow sweep between the two ends (dogPos advances in fraction of the range)
  float range = (float)abs(posNear - posFar);
  if (range > 0) dogPos += dogDir * (dogStep / range);
  if (dogPos >= 1.0f) { dogPos = 1.0f; dogDir = -1; }
  if (dogPos <= 0.0f) { dogPos = 0.0f; dogDir =  1; }
  return posFar + (int)(dogPos * (posNear - posFar));
}

// ToF4M readout (~10 Hz when presence detection is on)
static void updateTof()
{
  if (!presenceOn || !tofReady) return;
  static uint32_t lastRead = 0;
  if (millis() - lastRead < 100) return;
  lastRead = millis();
  tof.read();
  if (tof.ranging_data.range_status == VL53L1X::RangeValid)
    distMm = tof.ranging_data.range_mm;
  else
    distMm = -1;   // invalid/out of range -> treat as "nobody there"
}

static void updateMotor(float level)
{
  if (!motorReady || mode.mode == Mode::EXPLORE || heatCut) return;  // read-only / overheat
  int goal;
  switch (mode.mode) {
    case Mode::SHY_MIC:   goal = goalShyMic(level);   break;
    case Mode::ALERT_DOG: goal = goalAlertDog(level); break;
    default:              goal = manualPos;           break;  // MANUAL
  }
  // low-pass the goal: removes wiggle, adds a graceful glide to every motion
#if LPF_TAU_MS > 0
  if (goalFilt < 0) goalFilt = goal;
  goalFilt += (goal - goalFilt) * ((float)SAMPLE_WINDOW_MS / LPF_TAU_MS);
  goal = (int)(goalFilt + 0.5f);
#endif
  if (abs(goal - lastGoal) > POS_DEADBAND) {
    dxl.setGoalPosition(DXL_ID, goal);   // protocol 1.0: units of 0.29 deg
    lastGoal = goal;
  }
}

// ================= Calibration web UI =================
static const char PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ShyMic calib</title>
<style>
body{font-family:sans-serif;max-width:460px;margin:auto;padding:12px;background:#111;color:#eee}
body>*{width:80%}   /* right 20% stays touch-free for scrolling */
h1{font-size:1.2rem}label{display:block;margin-top:10px;font-size:.85rem}
input[type=range]{width:100%}select,button{width:100%;padding:8px;margin-top:8px;font-size:1rem;box-sizing:border-box}
.val{float:right;color:#7fd4ff}
.grp{border:1px solid #444;border-radius:12px;padding:8px 10px 12px;margin-top:16px}
.grp legend{font-size:.8rem;color:#7fd4ff;padding:0 6px}
.grp .hint{font-size:.72rem;color:#888;margin:2px 0 6px}
.st{font-size:.8rem;color:#999;margin-top:8px}
.btnrow{display:flex;gap:8px;width:80%}
.btnrow button{flex:1}
</style></head><body>
<h1>ShyMic</h1>
<select id="mode"><option value="0">SHY_MIC - runs away from sound</option><option value="1">ALERT_DOG - sweeps, freezes on sound</option><option value="2">MANUAL - slider drives motor</option><option value="3">EXPLORE - torque off, move by hand</option></select>

<fieldset class="grp"><legend>LIVE</legend>
<label>Mic level - the red dashed line is the ALERT_DOG sound trigger (drag it)<span class="val" id="lvv">-</span></label>
<canvas id="scope" style="width:100%;height:130px;background:#000;border-radius:4px;touch-action:none"></canvas>
<div class="st" id="st">-</div>
<div class="grp-ex">
<label>Motor position (your hand moves it)<span class="val" id="mv"></span></label>
<input type="range" id="m" min="0" max="1023" step="5">
<div class="btnrow">
<button id="bf" onclick="setEnd('far')">FAR</button>
<button id="bn" onclick="setEnd('near')">NEAR</button>
</div>
<div class="hint">Capture buttons store the motor's current position as the FAR / NEAR endpoint.</div>
</div>
</fieldset>

<fieldset class="grp"><legend>SAFE TRAVEL RANGE (all modes)</legend>
<div class="hint">Hard limits written to the motor itself - nothing can move outside these.</div>
<label>POS far<span class="val" id="fv"></span></label>
<input type="range" id="f" min="0" max="1023" step="5">
<label>POS near<span class="val" id="nv"></span></label>
<input type="range" id="n" min="0" max="1023" step="5">
<label>Torque limit (lower = cooler, weaker)<span class="val" id="tv2"></span></label>
<input type="range" id="tq" min="50" max="1023" step="10">
</fieldset>

<fieldset class="grp"><legend>SOUND MAPPING (all modes)</legend>
<div class="hint">Raw mic amplitude (mV peak-to-peak) mapped to 0..1 level. Watch the live mV readout: set Quiet floor just above resting room noise, Loud ceiling at your loudest sound.</div>
<label>Quiet floor (mV pp)<span class="val" id="qv"></span></label>
<input type="range" id="q" min="0" max="1000" step="10">
<label>Loud ceiling (mV pp)<span class="val" id="lv2"></span></label>
<input type="range" id="l" min="100" max="3300" step="50">
</fieldset>

<fieldset class="grp"><legend id="mt">MODE SETTINGS</legend>
<div class="grp-s">
<div class="hint">Presence detection (ToF sensor): person FAR = arm slowly bows. Person CLOSE = stands still, and speaks = mic escapes (shy behavior).</div>
<label style="font-size:1rem"><input type="checkbox" id="pon" style="width:auto;transform:scale(1.4);margin-right:8px">Presence detection</label>
<label>Presence threshold - "close" below this (mm)<span class="val" id="pdv"></span></label>
<input type="range" id="pd" min="100" max="3000" step="50">
<label>Release - how fast the mic returns after a peak (higher = slower, dreamier)<span class="val" id="rv"></span></label>
<input type="range" id="r" min="80" max="99" step="1">
<button onclick="inv()">Toggle direction (loud = NEAR / FAR)</button>
</div>
<div class="grp-d">
<label>Dog threshold - sound level that freezes the search (the red line)<span class="val" id="tv"></span></label>
<input type="range" id="t" min="5" max="100" step="1">
<label>Dog step - sweep speed while searching (small = slow creep)<span class="val" id="dv"></span></label>
<input type="range" id="d" min="1" max="30" step="1">
<label>Dog freeze (ms) - stay frozen this long after the sound stops<span class="val" id="cv"></span></label>
<input type="range" id="c" min="200" max="10000" step="100">
</div>
<div class="grp-m">
<label>Manual position<span class="val" id="mv2"></span></label>
<input type="range" id="m2" min="0" max="1023" step="5">
</div>
<div class="grp-e">
<div class="hint">Torque is off. Move the horn by hand and watch the position in LIVE, then use the capture buttons there.</div>
</div>
</fieldset>

<fieldset class="grp"><legend>MOTOR FEEL (all modes)</legend>
<label>Motor speed<span class="val" id="sv"></span></label>
<input type="range" id="s" min="10" max="1023" step="5">
</fieldset>

<div class="btnrow">
<button onclick="loadD()">LOAD DEFAULTS</button>
<button onclick="save()" style="background:#2ecc71;color:#111;font-weight:bold">SAVE DEFAULTS to flash</button>
</div>
<div class="st">Saved values are restored automatically at boot.</div>
<script>
const ids={mode:'mode',m:'manual',m2:'manual',q:'quiet',l:'loud',s:'speed',r:'release',f:'far',n:'near',t:'thresh',d:'step',c:'freeze',tq:'torque',pd:'presmm'};
const out={m:'mv',m2:'mv2',q:'qv',l:'lv2',s:'sv',r:'rv',f:'fv',n:'nv',t:'tv',d:'dv',c:'cv',tq:'tv2',pd:'pdv'};
function fmt(k,v){if(k=='r')return(v/100).toFixed(2);if(k=='t')return(v/100).toFixed(2);return v;}
let active=null;   // slider being dragged (poll must not fight it)
let lo=0,hi=1023;  // safe travel range - position sliders clamp to this
function showGrp(sel,on){document.querySelectorAll(sel).forEach(e=>e.style.display=on?'':'none');}
function applyMode(m){
 showGrp('.grp-s',m==0);showGrp('.grp-d',m==1);showGrp('.grp-m',m==2);showGrp('.grp-e',m==3);
 document.getElementById('mt').textContent=['MODE: SHY_MIC','MODE: ALERT_DOG','MODE: MANUAL','MODE: EXPLORE'][m];
 document.querySelector('#scope').style.opacity=(m==2||m==3)?0.4:1;
}
function setLimits(){
 document.getElementById('m').min=lo;document.getElementById('m').max=hi;
 document.getElementById('m2').min=lo;document.getElementById('m2').max=hi;}

// --- scope canvas: level history + draggable threshold line ---
const cv=document.getElementById('scope'),ctx=cv.getContext('2d');
const MAXH=120;let hist=[];let thr=45;
function fit(){cv.width=cv.clientWidth;cv.height=cv.clientHeight;}
window.addEventListener('resize',fit);
function draw(){
 const w=cv.width,h=cv.height;
 ctx.fillStyle='#000';ctx.fillRect(0,0,w,h);
 ctx.strokeStyle='#2ecc71';ctx.lineWidth=1.5;ctx.beginPath();
 hist.forEach((v,i)=>{const px=i/(MAXH-1)*w,py=h-v*h;i?ctx.lineTo(px,py):ctx.moveTo(px,py);});
 ctx.stroke();
 const ty=h-(thr/100)*h;
 ctx.strokeStyle='#e74c3c';ctx.lineWidth=2;ctx.setLineDash([5,4]);
 ctx.beginPath();ctx.moveTo(0,ty);ctx.lineTo(w,ty);ctx.stroke();ctx.setLineDash([]);
 ctx.fillStyle='#e74c3c';ctx.font='11px sans-serif';ctx.fillText('dog thr '+(thr/100).toFixed(2),4,ty-4);
}
let dragging=false;
function setThr(e){
 const r=cv.getBoundingClientRect();
 const y=(e.touches?e.touches[0].clientY:e.clientY)-r.top;
 thr=Math.max(0,Math.min(100,Math.round((1-y/r.height)*100)));
 document.getElementById('tv').textContent=(thr/100).toFixed(2);
 document.getElementById('t').value=thr;
 fetch('/set?thresh='+thr);
 draw();
}
cv.addEventListener('pointerdown',e=>{dragging=true;setThr(e);});
cv.addEventListener('pointermove',e=>{if(dragging)setThr(e);});
window.addEventListener('pointerup',()=>{dragging=false;active=null;});

async function refresh(){let j=await(await fetch('/get')).json();
 for(let k in ids){if(k=='mode'){document.getElementById('mode').value=j.mode;continue;}
  let e=document.getElementById(k);e.value=j[ids[k]];document.getElementById(out[k]).textContent=fmt(k,j[ids[k]]);}
 document.getElementById('pon').checked=j.presen;
 thr=j.thresh;lo=Math.min(j.far,j.near);hi=Math.max(j.far,j.near);setLimits();applyMode(j.mode);fit();draw();}
async function poll(){let j=await(await fetch('/get')).json();
 hist.push(j.level);if(hist.length>MAXH)hist.shift();
 // sliders follow the device (e.g. EXPLORE position readback) unless you are dragging one
 for(let k in out){if(k===active||(dragging&&k==='t'))continue;let e=document.getElementById(k);
  e.value=j[ids[k]];document.getElementById(out[k]).textContent=fmt(k,j[ids[k]]);}
 if(!dragging)thr=j.thresh;
 let nlo=Math.min(j.far,j.near),nhi=Math.max(j.far,j.near);
 if(nlo!=lo||nhi!=hi){lo=nlo;hi=nhi;setLimits();}
 draw();
 document.getElementById('lvv').textContent=j.level.toFixed(2)+' ('+j.pp.toFixed(0)+' mV)';
 document.getElementById('st').textContent='goal='+j.goal+' pos='+j.pos+' '+j.temp+'C'+(j.presen?' dist='+(j.dist<0?'--':j.dist)+'mm':'')+(j.frozen?' FROZEN':'')+(j.heat?' !!HOT - TORQUE CUT':'')+(j.motor?'':' NO MOTOR');
 document.getElementById('bf').textContent='FAR: '+j.far+'  <- set '+j.pos;
 document.getElementById('bn').textContent='NEAR: '+j.near+'  <- set '+j.pos;
 setTimeout(poll,300);}
for(let k in ids){if(k=='mode')continue;
 let e=document.getElementById(k);
 e.oninput=()=>{document.getElementById(out[k]).textContent=fmt(k,e.value);
  fetch('/set?'+ids[k]+'='+e.value);};
 e.addEventListener('pointerdown',()=>active=k);}
document.getElementById('mode').onchange=e=>{applyMode(+e.target.value);fetch('/set?mode='+e.target.value);};
function inv(){fetch('/set?invert=1');}
function setEnd(w){fetch('/set?set'+w+'=1');}
function save(){fetch('/save').then(()=>alert('Defaults saved to flash'));}
function loadD(){fetch('/load').then(()=>refresh());}
document.getElementById('pon').onchange=e=>fetch('/set?preson='+(e.target.checked?1:0));
refresh();poll();
</script></body></html>)rawliteral";

static void handleRoot() { server.send_P(200, "text/html", PAGE); }

static void handleGet()
{
  char buf[420];
  snprintf(buf, sizeof(buf),
    "{\"mode\":%d,\"manual\":%d,\"quiet\":%.0f,\"loud\":%.0f,\"speed\":%u,\"release\":%.0f,"
    "\"far\":%d,\"near\":%d,\"thresh\":%.0f,\"step\":%d,\"freeze\":%u,\"torque\":%u,"
    "\"presen\":%d,\"presmm\":%u,\"dist\":%d,"
    "\"level\":%.3f,\"pp\":%.1f,\"goal\":%d,\"pos\":%d,\"temp\":%d,\"frozen\":%d,\"heat\":%d,\"motor\":%d}",
    (int)mode.mode, manualPos, quietPpMv, loudPpMv, mode.moveSpeed, mode.release * 100,
    posFar, posNear, dogThresh * 100, dogStep, (unsigned)dogFreezeMs, torqueLimit,
    presenceOn ? 1 : 0, presenceMm, distMm,
    smoothed, lastPpMv, lastGoal, presentPos, presentTemp, dogFrozenAt ? 1 : 0,
    heatCut ? 1 : 0, motorReady ? 1 : 0);
  server.send(200, "application/json", buf);
}

static void handleSet()
{
  if (server.hasArg("mode")) {
    Mode m = (Mode)server.arg("mode").toInt();
    if (m != mode.mode) {
      if (motorReady) {
        if (m == Mode::EXPLORE) {
          dxl.torqueOff(DXL_ID);                       // free the horn
        } else if (mode.mode == Mode::EXPLORE) {
          dxl.torqueOn(DXL_ID);                        // re-engage
          dxl.writeControlTableItem(ControlTableItem::MOVING_SPEED, DXL_ID, mode.moveSpeed);
        }
      }
      mode.mode = m;
      lastGoal  = -1;                                  // force a goal refresh
      goalFilt  = -1;                                  // restart the low-pass
    }
  }
  if (server.hasArg("manual"))  manualPos      = constrain(server.arg("manual").toInt(), 0, 1023);
  if (server.hasArg("setfar") && presentPos >= 0)  { posFar  = presentPos; applyMotorLimits(); }
  if (server.hasArg("setnear") && presentPos >= 0) { posNear = presentPos; applyMotorLimits(); }
  if (server.hasArg("torque")) {
    torqueLimit = constrain(server.arg("torque").toInt(), 0, 1023);
    if (motorReady) dxl.writeControlTableItem(ControlTableItem::TORQUE_LIMIT, DXL_ID, torqueLimit);
  }
  if (server.hasArg("preson")) presenceOn = server.arg("preson").toInt() != 0;
  if (server.hasArg("presmm")) presenceMm = constrain(server.arg("presmm").toInt(), 50, 4000);
  if (server.hasArg("quiet"))   quietPpMv      = server.arg("quiet").toFloat();
  if (server.hasArg("loud"))    loudPpMv       = server.arg("loud").toFloat();
  if (server.hasArg("far"))    { posFar  = constrain(server.arg("far").toInt(), 0, 1023);  applyMotorLimits(); }
  if (server.hasArg("near"))   { posNear = constrain(server.arg("near").toInt(), 0, 1023); applyMotorLimits(); }
  if (server.hasArg("thresh"))  dogThresh      = server.arg("thresh").toFloat() / 100.0f;
  if (server.hasArg("step"))    dogStep        = server.arg("step").toInt();
  if (server.hasArg("freeze"))  dogFreezeMs    = server.arg("freeze").toInt();
  if (server.hasArg("invert"))  invert         = !invert;
  if (server.hasArg("release")) mode.release   = server.arg("release").toFloat() / 100.0f;
  if (server.hasArg("speed")) {
    mode.moveSpeed = server.arg("speed").toInt();
    if (motorReady)
      dxl.writeControlTableItem(ControlTableItem::MOVING_SPEED, DXL_ID, mode.moveSpeed);
  }
  server.send(200, "text/plain", "ok");
}

// ================= NVS: persistent defaults =================
static void loadPrefs()
{
  Preferences p;
  p.begin("shymic", true);   // read-only
  quietPpMv      = p.getFloat("quiet",   quietPpMv);
  loudPpMv       = p.getFloat("loud",    loudPpMv);
  posFar         = p.getInt("far",       posFar);
  posNear        = p.getInt("near",      posNear);
  mode.moveSpeed = p.getUShort("speed",  mode.moveSpeed);
  mode.release   = p.getFloat("release", mode.release);
  dogThresh      = p.getFloat("thresh",  dogThresh);
  dogStep        = p.getInt("step",      dogStep);
  dogFreezeMs    = p.getUInt("freeze",   dogFreezeMs);
  torqueLimit    = p.getUShort("torque", torqueLimit);
  presenceOn     = p.getBool("preson",   presenceOn);
  presenceMm     = p.getUShort("presmm", presenceMm);
  invert         = p.getBool("invert",   invert);
  p.end();
}

static void handleLoad()
{
  loadPrefs();
  if (motorReady) {
    applyMotorLimits();
    dxl.writeControlTableItem(ControlTableItem::MOVING_SPEED, DXL_ID, mode.moveSpeed);
  }
  Serial.println("defaults reloaded from flash");
  server.send(200, "text/plain", "loaded");
}

static void handleSave()
{
  Preferences p;
  p.begin("shymic", false);  // read-write
  p.putFloat("quiet",   quietPpMv);
  p.putFloat("loud",    loudPpMv);
  p.putInt("far",       posFar);
  p.putInt("near",      posNear);
  p.putUShort("speed",  mode.moveSpeed);
  p.putFloat("release", mode.release);
  p.putFloat("thresh",  dogThresh);
  p.putInt("step",      dogStep);
  p.putUInt("freeze",   dogFreezeMs);
  p.putUShort("torque", torqueLimit);
  p.putBool("preson",   presenceOn);
  p.putUShort("presmm", presenceMm);
  p.putBool("invert",   invert);
  p.end();
  Serial.printf("defaults saved: far=%d near=%d speed=%u torque=%u quiet=%.0f loud=%.0f thr=%.2f\n",
                posFar, posNear, mode.moveSpeed, torqueLimit, quietPpMv, loudPpMv, dogThresh);
  server.send(200, "text/plain", "saved");
}

void setup()
{
  auto cfg = M5.config();
  cfg.internal_mic = false;   // external analog mic on MIC_PIN
  cfg.internal_spk = false;
  M5.begin(cfg);

  loadPrefs();   // restore saved defaults from flash (if any)

  Serial.println("\nM5StampS3 mic -> Dynamixel");
  const char* modeNames[] = {"SHY_MIC", "ALERT_DOG", "MANUAL", "EXPLORE"};
  Serial.printf("mode=%s\n", modeNames[(int)mode.mode]);
  Serial.println("BtnA (GPIO0) toggles invert mode (SHY_MIC only)");

  // --- analog mic ---
  analogReadResolution(12);
  analogSetPinAttenuation(MIC_PIN, ADC_11db);   // full ~3.3 V range

  // --- Dynamixel bus ---
  pinMode(DXL_DIR_PIN, OUTPUT);
  DXL_SERIAL.begin(DXL_BAUD, SERIAL_8N1, DXL_RX_PIN, DXL_TX_PIN);
  dxl.begin(DXL_BAUD);
  dxl.setPortProtocolVersion(DXL_PROTOCOL);

  if (dxl.ping(DXL_ID)) {
    dxl.torqueOff(DXL_ID);
    dxl.setOperatingMode(DXL_ID, OP_POSITION);
    applyMotorLimits();            // safe range + torque cap before torque goes on
    dxl.writeControlTableItem(ControlTableItem::MOVING_SPEED, DXL_ID, mode.moveSpeed);
    dxl.torqueOn(DXL_ID);
    motorReady = true;
    Serial.printf("Dynamixel ID %d found\n", DXL_ID);
  } else {
    Serial.printf("Dynamixel ID %d NOT found - check wiring/baud/ID\n", DXL_ID);
  }

  // --- WiFi AP + calibration server ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", handleRoot);
  server.on("/get", handleGet);
  server.on("/set", handleSet);
  server.on("/save", handleSave);
  server.on("/load", handleLoad);
  server.begin();
  Serial.printf("AP '%s' up, UI at http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  // --- ToF4M distance sensor (shares M5Unified's Wire bus, GPIO13/15) ---
  tof.setBus(&Wire);
  tof.setTimeout(50);
  if (tof.init()) {
    tof.setDistanceMode(VL53L1X::Short);      // better in ambient light
    tof.setMeasurementTimingBudget(20000);    // 20 ms
    tof.startContinuous(20);
    tofReady = true;
    Serial.println("ToF4M found");
  } else {
    Serial.println("ToF4M NOT found - presence detection unavailable");
  }
}

void loop()
{
  M5.update();
  if (M5.BtnA.wasPressed() && mode.mode == Mode::SHY_MIC) {
    invert = !invert;
    Serial.printf("invert=%d\n", invert);
  }

  float pp    = micEnvelopePPmV();
  lastPpMv    = pp;
  float level = constrain((pp - quietPpMv) / (loudPpMv - quietPpMv), 0.0f, 1.0f);
  smoothed = (level > smoothed) ? level : smoothed * mode.release;

  // EXPLORE mode: torque is off - read where the horn actually is
  if (mode.mode == Mode::EXPLORE && motorReady) {
    static uint32_t lastRead = 0;
    if (millis() - lastRead > 100) {
      lastRead = millis();
      float rp = dxl.getPresentPosition(DXL_ID, UNIT_RAW);
      if (rp >= 0 && rp <= 1023) { presentPos = (int)rp; manualPos = presentPos; }
    }
  }

  // motor health: position + temperature readback (~2 Hz), overheat cut
  if (motorReady) {
    static uint32_t lastHealth = 0;
    if (millis() - lastHealth > 500) {
      lastHealth = millis();
      int t = dxl.readControlTableItem(ControlTableItem::PRESENT_TEMPERATURE, DXL_ID);
      if (t > 0 && t < 150) presentTemp = t;   // protocol 1.0: 1 byte, deg C
      float rp = dxl.getPresentPosition(DXL_ID, UNIT_RAW);
      if (rp >= 0 && rp <= 1023) presentPos = (int)rp;
      if (!heatCut && presentTemp >= TEMP_WARN_C) {
        heatCut = true;
        dxl.torqueOff(DXL_ID);                        // let it cool down
        Serial.printf("!! motor HOT (%d C) - torque cut\n", presentTemp);
      } else if (heatCut && presentTemp <= TEMP_WARN_C - 10) {
        heatCut = false;
        dxl.torqueOn(DXL_ID);                         // cooled enough, resume
        lastGoal = -1;
        Serial.printf("motor cooled (%d C) - torque back on\n", presentTemp);
      }
    }
  }

  updateTof();
  updateMotor(smoothed);
  server.handleClient();

  // Calibration telemetry (~5 Hz): watch pp while testing quiet/loud
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.printf("pp=%5.0f mV  level=%.2f  goal=%d%s%s\n",
                  pp, smoothed, lastGoal,
                  dogFrozenAt ? "  FROZEN" : "",
                  motorReady ? "" : "  (no motor)");
  }
}
