/*
Copyright 2022 connorr@hey.com

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

#include <stdint.h>

// 17 characters + a null terminator
#define VIN_LENGTH 18

#define CALIBRATION_ID_LENGTH 19

class RX8
{
private:
	J2534 _j2534;
	unsigned long _devID;
	unsigned long _chanID;

public:
	// initializer
	RX8(J2534& j2534, unsigned long devID, unsigned long chanID);

	/** Get the VIN stored in the ECU*/
	size_t getVIN(char** vin);
	size_t getCalibrationID(char** calibrationID);
};