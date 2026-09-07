/**
 * @file main.cpp
 * @authors Etienne Montenegro (MxLab - UQAM)
 *          Jerome Saint-Clair (Ensad)
 * @brief Main file of the MisBKit project
 *
 *
 */

#include <Arduino.h>
#include <ArduinoLog.h>
#ifdef WEB_SERIAL
#include <WebSerial.h>
#define LOG_DESTINATION &WebSerial
#else
#define LOG_DESTINATION &Serial
#endif
#include <ServoEasing.hpp>
#include "MisBKit.h"

MisBKit mbk{};

void setup()
{
  Serial.begin(115200);
  // while (!Serial)
  // {
  // }
  delay(1000);
  Log.begin(LOG_LEVEL_TRACE, LOG_DESTINATION);
  Log.warningln("MisBKit %d.%d.%d", V_MAJ, V_MIN, V_PATCH);
  mbk.initialize(false);
  Log.warningln("End on setup.");
}

void loop()
{
  mbk.update();
}
