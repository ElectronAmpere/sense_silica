#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include "config.h"
#include "SoilSensor.h"
#include "lcd.h"
#include "ModbusMaster.h"
#include <SoftwareSerial.h>

// Hardware instances (defined in setup.cpp)
extern SoftwareSerial mySerial;
extern ModbusMaster node;
extern SoilSensor gSensor;
extern LCD gLcd;

// Setup APIs
void setupHardware();
void setupScheduler();

#endif // SETUP_H
