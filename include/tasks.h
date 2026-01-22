#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include "scheduler.h"
#include "config.h"
#include "SoilSensor.h"

// Expose task functions
int Task_ToggleLED(int state);
int Task_SoilSensor(int state);
int Task_LcdUpdate(int state);

// Expose shared sensor data and status flag
extern SoilSensor::SensorData gSensorData;
extern bool gLastReadOk;

// Expose tasks array to scheduler
extern scheduler::Task tasks[scheduler::TOTAL_TASKS_NUM];

#endif // TASKS_H
