/*
 * Copyright (C) 2009-2012 Motorola, Inc.
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hardware/sensors.h>
#include <hardware/mot_sensorhub_msp430.h>
#include <float.h>

#include "nusensors.h"

/*****************************************************************************/

/*
 * The SENSORS Module
 */

static const struct sensor_t sSensorList[] = {
	{
		.name = "LIS3DH 3-axis Accelerometer",
		.vendor = "ST Micro",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_A,
		.type = SENSOR_TYPE_ACCELEROMETER,
		.maxRange = 16.0f*9.81f,
		.resolution = 9.81f/2048.0f,
		.power = 0.25f,
		.minDelay = 10000,
		.stringType = SENSOR_STRING_TYPE_ACCELEROMETER,
		.reserved = {}
	},
	{
		.name = "L3G4200G Gyroscope sensor",
		.vendor = "ST Micro",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_G,
		.type = SENSOR_TYPE_GYROSCOPE,
		.maxRange = 2000.0f,
		.resolution = 1.0f,
		.power = 6.1f,
		.minDelay = 20000,
		.stringType = SENSOR_STRING_TYPE_GYROSCOPE,
		.reserved = {}
	},
	{
		.name = "BMP180 Pressure sensor",
		.vendor = "Bosch",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_PR,
		.type = SENSOR_TYPE_PRESSURE,
		.maxRange = 125000.0f,
		.resolution = 1.0f,
		.minDelay = 20000,
		.stringType = SENSOR_STRING_TYPE_PRESSURE,
		.reserved = {}
	},
	{
		.name = "AK8975 3-axis Magnetic field sensor",
		.vendor = "Asahi Kasei",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_M,
		.type = SENSOR_TYPE_MAGNETIC_FIELD,
		.maxRange = 2000.0f,
		.resolution = 1.0f/10.0f,
		.power = 6.8f,
		.minDelay = 10000,
		.stringType = SENSOR_STRING_TYPE_MAGNETIC_FIELD,
		.reserved = {}
	},
	{
		.name = "AK8963 Orientation sensor",
		.vendor = "Asahi Kasei",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_O,
		.type = SENSOR_TYPE_ORIENTATION,
		.maxRange = 360.0f,
		.resolution = 1.0f/64.0f,
		.power = 7.05f,
		.minDelay = 10000,
		.stringType = SENSOR_STRING_TYPE_ORIENTATION,
		.reserved = {}
	},
	{
		.name = "MSP430 Temperature sensor",
		.vendor = "TI",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_T,
		.type = SENSOR_TYPE_TEMPERATURE,
		.maxRange = 115.0f,
		.resolution = 1.6f,
		.power = 3.0f,
		.stringType = SENSOR_STRING_TYPE_TEMPERATURE,
		.reserved = {}
	},
	{
		.name = "CT406 Light sensor",
		.vendor = "TAOS",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_L,
		.type = SENSOR_TYPE_LIGHT,
		.maxRange = 27000.0f,
		.resolution = 1.0f,
		.power = 0.175f,
		.stringType = SENSOR_STRING_TYPE_LIGHT,
		.reserved = {}
	},
	{
		.name = "Linear Acceleration",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_LA,
		.type = SENSOR_TYPE_LINEAR_ACCELERATION,
		.stringType = SENSOR_STRING_TYPE_LINEAR_ACCELERATION,
		.reserved = {}
	},
	{
		.name = "Gravity",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_GR,
		.type = SENSOR_TYPE_GRAVITY,
		.stringType = SENSOR_STRING_TYPE_GRAVITY,
		.reserved = {}
	},
	{
		.name = "Display Rotation sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_DR,
		.type = SENSOR_TYPE_DISPLAY_ROTATE,
		.maxRange = 4.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Display Brightness sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_DB,
		.type = SENSOR_TYPE_DISPLAY_BRIGHTNESS,
		.maxRange = 255.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Dock",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_D,
		.type = SENSOR_TYPE_DOCK,
		.maxRange = 3.0f,
		.resolution = 1.0f,
		.power = 0.01f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "CT406 Proximity sensor",
		.vendor = "TAOS",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_P,
		.type = SENSOR_TYPE_PROXIMITY,
		.maxRange = 100.0f,
		.resolution = 100.0f,
		.power = 3.0f,
		.minDelay = 0,
		.stringType = SENSOR_STRING_TYPE_PROXIMITY,
		.reserved = {}
	},
	{
		.name = "Flat Up",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_FU,
		.type = SENSOR_TYPE_FLAT_UP,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Flat Down",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_FD,
		.type = SENSOR_TYPE_FLAT_DOWN,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Stowed",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_S,
		.type = SENSOR_TYPE_STOWED,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Camera Activation sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_CA,
		.type = SENSOR_TYPE_CAMERA_ACTIVATE,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 20000,
		.reserved = {}
	},
	{
		.name = "NFC Detect sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_NFC,
		.type = SENSOR_TYPE_NFC_DETECT,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 0.0f,
		.minDelay = 0,
		.reserved = {}
	},
	{
		.name = "Significant Motion sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_SIM,
		.type = SENSOR_TYPE_SIGNIFICANT_MOTION,
		.maxRange = 1.0f,
		.resolution = 1.0f,
		.power = 3.0f,
		.minDelay = -1,
		.stringType = SENSOR_STRING_TYPE_SIGNIFICANT_MOTION,
		.reserved = {}
	},
	{
		.name = "Step Detector sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_STEP_DETECTOR,
		.type = SENSOR_TYPE_STEP_DETECTOR,
		.maxRange = 1.0f,
		.resolution = 0.0f,
		.power = 0.0f,
		.minDelay = 0,
		.stringType = SENSOR_STRING_TYPE_STEP_DETECTOR,
		.reserved = {}
	},
	{
		.name = "Step Counter sensor",
		.vendor = "Motorola",
		.version = 1,
		.handle = SENSORS_HANDLE_BASE+ID_STEP_COUNTER,
		.type = SENSOR_TYPE_STEP_COUNTER,
		.maxRange = FLT_MAX,
		.resolution = 0.0f,
		.power = 0.0f,
		.minDelay = 0,
		.stringType = SENSOR_STRING_TYPE_STEP_COUNTER,
		.reserved = {}
	},
};

/*    { "IR Gestures",
                "Motorola",
                1, SENSORS_HANDLE_BASE+ID_IR_GESTURE,
                SENSOR_TYPE_IR_GESTURE, 1.0f, 1.0f, 1.0f, 0, 8, 8, { 0 } },

    { "IR Raw Data",
                "Motorola",
                1, SENSORS_HANDLE_BASE+ID_IR_RAW,
                SENSOR_TYPE_IR_RAW, 4096.0f, 1.0f, 1.0f, 10000, 0, 0, { 0 } },

    { "Uncalibrated gyro sensor",
                "Motorola",
                1, SENSORS_HANDLE_BASE+ID_UNCALIB_GYRO,
                SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,2000.0f, 1.0f, 6.1f, 20000, 0, 0, { 0 } },

    { "AK8963 3-axis Uncalibrated Magnetic field sensor",
                "Asahi Kasei",
                1, SENSORS_HANDLE_BASE+ID_UNCALIB_MAG,
                SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 2000.0f, 1.0f/10.0f, 6.8f, 10000, 0, 0, { 0 } },
*/


static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 2,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Motorola Sensors Module",
        .author = "Motorola",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};

/*****************************************************************************/

static int open_sensors(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    return init_nusensors(module, device);
}
