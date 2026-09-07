// AccelBot — M5StampS3 + 2x Dynamixel + MPU6886 accel module (I2C 0x68, PortA/Grove).
// Wearable "hat": head tilt (inclination vs gravity) morphs the shape.
//   tilt X (roll,  ear-to-shoulder)  -> motor 1
//   tilt Y (pitch, nod)              -> motor 2
// IMU init/register map follows MisBKit UnitMPU6886.h
#include <M5Unified.h>
#include <Dynamixel2Arduino.h>
#include <Wire.h>
#include <math.h>
#include "Pins.h"
#include "Led.h"

// ---------- Dynamixel bus ----------
#define DXL_SERIAL    Serial0
#define DXL_BAUD      1000000
#define DXL_PROTOCOL  1.0f     // AX-12A
#define DXL_ID_X      12       // motor driven by tilt X
#define DXL_ID_Y      19       // motor driven by tilt Y

// ---------- Tilt mapping ----------
#define TILT_MAX_DEG  45.0f    // tilt of +/- this angle spans the full motor range
#define POS_MIN       200      // AX-12A joint units (0..1023)  <- CALIBRATE per hat
#define POS_MAX       800
#define POS_CENTER    ((POS_MIN + POS_MAX) / 2)
#define TILT_LPF      0.15f    // smoothing factor per sample (smaller = dreamier)
#define MOVE_SPEED    150      // AX-12A moving speed (0..1023)
#define POS_DEADBAND  3        // ignore goal changes smaller than this

Dynamixel2Arduino dxl(DXL_SERIAL, pins::motorsControl);

static bool  readyX = false, readyY = false;
static float tiltXF = 0, tiltYF = 0;      // low-pass filtered tilt (deg)
static int   lastGoalX = -1, lastGoalY = -1;

// ---------- MPU6886 (external module, I2C 0x68) ----------
#define MPU_ADDR        0x68
#define MPU_REG_WHOAMI  0x75
#define MPU_REG_PWR1    0x6B
#define MPU_REG_ACCEL_CFG  0x1C
#define MPU_REG_ACCEL_XOUT 0x3B
static bool imuReady = false;

static void mpuWrite(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static bool mpuInit()
{
  // WHO_AM_I
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(MPU_REG_WHOAMI);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(MPU_ADDR, (uint8_t)1);
  if (!Wire.available()) return false;
  uint8_t id = Wire.read();
  Serial.printf("MPU6886 WHO_AM_I: 0x%02X\n", id);
  if (id != 0x19 && id != 0x68 && id != 0x71) return false;   // 6886/6050/9250

  // MisBKit init sequence: reset, wake, +/-8g
  mpuWrite(MPU_REG_PWR1, 0x00); delay(10);
  mpuWrite(MPU_REG_PWR1, 0x80); delay(10);   // reset
  mpuWrite(MPU_REG_PWR1, 0x01); delay(10);   // wake, auto clock
  mpuWrite(MPU_REG_ACCEL_CFG, 0x10);         // +/-8g -> 4096 LSB/g
  return true;
}

static bool mpuReadAccel(float& ax, float& ay, float& az)
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(MPU_REG_ACCEL_XOUT);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)6) != 6) return false;
  int16_t rx = (Wire.read() << 8) | Wire.read();
  int16_t ry = (Wire.read() << 8) | Wire.read();
  int16_t rz = (Wire.read() << 8) | Wire.read();
  ax = rx / 4096.0f; ay = ry / 4096.0f; az = rz / 4096.0f;   // g, at +/-8g scale
  return true;
}

// tiny non-blocking boot wiggle: +/- 40 units around center, ~1 s
static uint32_t bootStart = 0;
static bool     booting   = true;
#define BOOT_MS      1000
#define BOOT_SWING   40

static void setupMotor(uint8_t id, bool& ready)
{
  if (dxl.ping(id)) {
    dxl.torqueOff(id);
    dxl.setOperatingMode(id, OP_POSITION);
    dxl.writeControlTableItem(ControlTableItem::CW_ANGLE_LIMIT,  id, POS_MIN);
    dxl.writeControlTableItem(ControlTableItem::CCW_ANGLE_LIMIT, id, POS_MAX);
    dxl.writeControlTableItem(ControlTableItem::MOVING_SPEED,    id, MOVE_SPEED);
    dxl.torqueOn(id);
    ready = true;
    Serial.printf("Dynamixel ID %d found\n", id);
  } else {
    Serial.printf("Dynamixel ID %d NOT found\n", id);
  }
}

// tilt in degrees: 0 = that axis horizontal; gravity vector gives inclination
static int tiltToGoal(float tiltDeg)
{
  float t = constrain(tiltDeg / TILT_MAX_DEG, -1.0f, 1.0f);
  return POS_CENTER + (int)(t * (POS_MAX - POS_CENTER));
}

void setup()
{
  auto cfg = M5.config();
  cfg.internal_imu = false;   // MPU6886 is an external module on the I2C bus
  M5.begin(cfg);

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) delay(10);   // wait for USB CDC host

  Serial.println("\nAccelBot: head tilt -> 2x Dynamixel");
  Serial.printf("board detected by M5Unified: %d (LED on GPIO%d)\n",
                (int)M5.getBoard(), (int)pins::led);
  Serial.printf("mapping: +/-%.0f deg tilt -> %d..%d joint units\n",
                TILT_MAX_DEG, POS_MIN, POS_MAX);

  // external IMU on the M5 I2C bus (GPIO13 SDA / GPIO15 SCL)
  Wire.begin(13, 15);
  imuReady = mpuInit();
  Serial.printf("MPU6886 module: %s\n", imuReady ? "found" : "NOT found - check Grove wiring");

  // LED: MisBKit-style init (powers the LED gate on GPIO38, white flash)
  led::init();

  pinMode(pins::motorsControl, OUTPUT);
  DXL_SERIAL.begin(DXL_BAUD, SERIAL_8N1, pins::dxlRx, pins::dxlTx);
  dxl.begin(DXL_BAUD);
  dxl.setPortProtocolVersion(DXL_PROTOCOL);

  setupMotor(DXL_ID_X, readyX);
  setupMotor(DXL_ID_Y, readyY);

  // built-in RGB LED will glow green (led::update breathing) once running
  led::setColor(0, 255, 0, true);
  bootStart = millis();
}

void loop()
{
  M5.update();
  led::update();   // breathing brightness, MisBKit pattern

  // boot sequence: wiggle both motors so you can see they're alive
  if (booting) {
    uint32_t t = millis() - bootStart;
    if (t > BOOT_MS) {
      booting = false;
      lastGoalX = lastGoalY = -1;   // let the tilt control take over
    } else {
      int off = (int)(BOOT_SWING * sinf(t * 6.2832f / 300.0f));   // ~3 wiggles
      if (readyX) dxl.setGoalPosition(DXL_ID_X, POS_CENTER + off);
      if (readyY) dxl.setGoalPosition(DXL_ID_Y, POS_CENTER - off);
      return;
    }
  }

  // ~50 Hz control loop
  static uint32_t lastTick = 0;
  if (millis() - lastTick < 20) return;
  lastTick = millis();

  float ax = 0, ay = 0, az = 0;
  bool imuOk = imuReady && mpuReadAccel(ax, ay, az);

  // inclination of X and Y axes vs gravity, in degrees
  float tiltX = atan2f(ax, sqrtf(ay * ay + az * az)) * 57.2958f;
  float tiltY = atan2f(ay, sqrtf(ax * ax + az * az)) * 57.2958f;

  // smooth so the hat morphs gracefully
  tiltXF += (tiltX - tiltXF) * TILT_LPF;
  tiltYF += (tiltY - tiltYF) * TILT_LPF;

  if (readyX) {
    int g = tiltToGoal(tiltXF);
    if (abs(g - lastGoalX) > POS_DEADBAND) { dxl.setGoalPosition(DXL_ID_X, g); lastGoalX = g; }
  }
  if (readyY) {
    int g = tiltToGoal(tiltYF);
    if (abs(g - lastGoalY) > POS_DEADBAND) { dxl.setGoalPosition(DXL_ID_Y, g); lastGoalY = g; }
  }

  // telemetry (~5 Hz)
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.printf("accel x=%+.2f y=%+.2f z=%+.2f g  tilt x=%+6.1f y=%+6.1f deg  goal x=%4d y=%4d%s\n",
                  ax, ay, az, tiltXF, tiltYF, lastGoalX, lastGoalY,
                  imuOk ? "" : "  (IMU READ FAIL)");
  }
}
