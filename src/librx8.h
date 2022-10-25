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

#include "J2534.h"
#include "UDS.h"

// 17 characters + a null terminator
static const uint8_t VIN_LENGTH = 18;

// unsure if this value will ever change..
static const uint8_t CALIBRATION_ID_LENGTH = 17;

// 3 uint8s
static const uint8_t SEED_LENGTH = 3;
 
#define MAZDA_KEY_SECRET {0x4d, 0x61, 0x7a, 0x64, 0x41} // M a z d A
//#define MAZDA_KEY_SECRET {'M', 'a', 'z', 'd', 'A'}

// extensions of UDS, not offical spec, so namespaced as such

static const uint8_t MAZDA_SBF_REQUEST_SEED = 0x01;
static const uint8_t MAZDA_SBF_CHECK_KEY    = 0x02;
static const uint8_t MAZDA_SBF_SESSION_81   = 0x81;
static const uint8_t MAZDA_SBF_SESSION_85   = 0x85;
static const uint8_t MAZDA_SBF_SESSION_87   = 0x87;

static const uint16_t MAZDA_SBL_LENGTH      = 6144;
static const uint16_t MAZDA_ROM_START_OFFSET = 0x2000;
static const uint32_t MAZDA_ROM_LENGTH      = 516096 + MAZDA_ROM_START_OFFSET;

class RX8
{
private:
	uds_request_t* request;

public:
	RX8(J2534* j2534, unsigned long devID, unsigned long chanID);

	/** Get the VIN stored in the ECU*/
	size_t getVIN(char** vin);

	/** Get the Calibration ID */
	size_t getCalibrationID(char** calibrationID);
	
	/** Initialize a diag session with the ECU */
	size_t initDiagSession(uint8_t session);

	/** Request a seed to be used in the key handshake */
	size_t getSeed(uint8_t** seed);

	/** Calculate the key using the seed from the `getSeed()` function */
	static size_t calculateKey(uint8_t* seedInput, uint8_t** keyOut);

	/** Unlock the ECU using the key calculated with `calculateKey()` */
	size_t unlock(uint8_t* key);

	/** Read memory starting at `start` and of size `chunkSize` into `data`. ECU must be `unlock()`ed for this to work. */
	size_t readMem(uint32_t start, uint16_t chunkSize, char* data);

	/** Puts the ECU into bootloader mode. this allows requestDownload to work */
	size_t requestBootloaderMode();

	size_t requestDownload(uint32_t address, uint32_t size);

	size_t transferData(uint32_t chunkSize, unsigned char* data);
	
	size_t requestTransferExit();

	size_t reset();
};
