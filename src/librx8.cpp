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

/*
 * TODO:
 * 		* handle uds errors in a common way
 *    * add common handler for each command
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <cstdlib>
#include <errno.h>

#include "J2534.h"

#include "librx8.h"
#include "UDS.h"
#include "OBD2.h"
#include "util.h"

static const char* TAG = "RX8";

/**
 * @brief Construct a new ecu object
 * 
 * @param j2534 - passthru interface
 * @param devID - passthru interface
 * @param chanID - passthru interface
 */
RX8::RX8(J2534* j2534, unsigned long devID, unsigned long chanID)
{
	if(uds_request_init(&request, j2534, devID, chanID)) {
		LOGE(TAG, "Failed to allocate UDS data");
		request = NULL;
	}
}

/**
 * @brief get the VIN from the ECU
 * 
 * @param vin     pointer to a buffer to store the vin
 * @return size_t 0 on success > 0 otherwise
 */
size_t RX8::getVIN(char** vin)
{
	if(!request) return 1;
	size_t  ret = 1; 
	uint8_t i   = 0;
	*vin = (char*)malloc(VIN_LENGTH);
	if (!(*vin)) return ENOMEM;
	memset(*vin, 0, VIN_LENGTH);

	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[getVIN] failed to prepare request %s", uds_request_error_string(request, ret));
		goto cleanup;
	}

	request->sid = OBD2_SID_REQUEST_VEHICLE_INFORMATION;
	request->payload[0] = OBD2_PID_REQUEST_VIN;
	request->length = 1;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[getVIN] request failed %s", uds_request_negative_response_error_string(request));
		goto cleanup;
	} else if(ret) {
		LOGE(TAG, "[getVIN] request failed %s", uds_request_error_string(request, ret));
		goto cleanup;
	} else if(request->sid == OBD2_SID_REQUEST_VEHICLE_INFORMATION_ACK) {
		if(request->payload[0] != OBD2_PID_REQUEST_VIN) {
			LOGE(TAG, "[getVIN] request failed");
			goto cleanup;
		}
		if(request->payload[1] != 0x01) {
			LOGE(TAG, "[getVIN] request failed");
			goto cleanup;
		}
		request->length-=2;
		request->payload+=2;
		assert(request->length == VIN_LENGTH-1);

		//hack: replace space characters with null. This is required for non USDM VINs.
		for(i=0; i < request->length; i++) {
			if(request->payload[i] == 0x20) {// ascii space character
				request->payload[i] = 0;
			}
		}
		request->length = i;
		memcpy(*vin, request->payload, request->length);
		return 0;
	} else {
		LOGE(TAG, "[getVIN] unknown error!");
		goto cleanup;
	}

cleanup:
	if((*vin) != NULL)
		free(*vin);
	*vin = NULL;
	return ret;
}

/**
 * @brief reads the calid from the ECU
 * 
 * @param calibrationID pointer to a buffer to store the id
 * @return size_t       0 if successful, >0 if not
 */
size_t RX8::getCalibrationID(char** calibrationID)
{
	if(!request) return 1;
	size_t ret = 0;
	*calibrationID = (char*)malloc(CALIBRATION_ID_LENGTH);
	if (!(*calibrationID)) return ENOMEM;
	memset(*calibrationID, 0, CALIBRATION_ID_LENGTH);

	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[getCalibrationID] failed to prepare request %s", uds_request_error_string(request, ret));
		goto cleanup;
	}

	request->sid = OBD2_SID_REQUEST_VEHICLE_INFORMATION;
	request->payload[0] = OBD2_PID_REQUEST_CALID;
	request->length = 1;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[getCalibrationID] request failed %s", uds_request_negative_response_error_string(request));
		goto cleanup;
	} else if(ret) {
		LOGE(TAG, "[getCalibrationID] request failed %s", uds_request_error_string(request, ret));
		goto cleanup;
	} else if(request->sid == OBD2_SID_REQUEST_VEHICLE_INFORMATION_ACK) {
		if(request->payload[0] != OBD2_PID_REQUEST_CALID) {
			LOGE(TAG, "[getCalibrationID] request failed");
			goto cleanup;
		}
		if(request->payload[1] != 0x01) {
			LOGE(TAG, "[getCalibrationID] request failed");
			goto cleanup;
		}
		request->length-=2;
		request->payload+=2;

		hexdump(request->payload+2, request->length-2);
		assert(request->length == CALIBRATION_ID_LENGTH-1);
		memcpy(*calibrationID, request->payload, request->length);
		return 0;
	} else {
		LOGE(TAG, "[getCalibrationID] unknown error!");
		goto cleanup;
	}

cleanup:
	if ((*calibrationID) != NULL)
		free(*calibrationID);
	*calibrationID = NULL;
	return ret;
}

/**
 * @brief enter diag mode
 * 
 * @param session one of:
 * 							MAZDA_SBF_SESSION_81
 *							MAZDA_SBF_SESSION_85
 *							MAZDA_SBF_SESSION_87
 * @return size_t 0 if successful > 0 if not
 */
size_t RX8::initDiagSession(uint8_t session)
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[initDiagSession] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid        = UDS_SID_SESSION;
	request->payload[0] = session;
	request->length = 1;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[initDiagSession] request failed %s", uds_request_negative_response_error_string(request));
		return 0;
	} else if(ret) {
		LOGE(TAG, "[initDiagSession] request failed %s", uds_request_error_string(request, ret));
		return 0;
	} else {
		assert(request->length == 1);
		return (request->sid == UDS_SID_SESSION_ACK) && (request->payload[0] == session);
	}

	LOGE(TAG, "[initDiagSession] unknown error!");
	return 0;
}

/**
 * @brief get the seed from the ECU to be used in the unlock procedure
 * 
 * @param seed    pointer to a buffer to store the seed
 * @return size_t 0 if successful, > 0 otherwise
 */
size_t RX8::getSeed(uint8_t** seed)
{
	if(!request) return 1;
	size_t ret;
	*seed = (uint8_t*)malloc(SEED_LENGTH);
	if (!(*seed)) return ENOMEM;
	memset(*seed, 0, SEED_LENGTH);

	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[getSeed] failed to prepare request %s", uds_request_error_string(request, ret));
		goto cleanup;
	}

	request->sid        = UDS_SID_SECURITY;
	request->payload[0] = MAZDA_SBF_REQUEST_SEED;
	request->length     = 1;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[getSeed] request failed %s", uds_request_negative_response_error_string(request));
		goto cleanup;
	} else if(ret) {
		LOGE(TAG, "[getSeed] request failed %s", uds_request_error_string(request, ret));
		goto cleanup;
	} else if(request->sid == UDS_SID_SECURITY_ACK) {
		assert(request->length == SEED_LENGTH+1);
		memcpy(*seed, request->payload+1, SEED_LENGTH);
		return 0;
	} else {
		LOGE(TAG, "[getSeed] unknown error!");
		goto cleanup;
	}

cleanup:
	if ((*seed) != NULL)
		free(*seed);
	*seed = NULL;
	return ret;
}
/**
 * @brief calculate the key with a given seed
 * 
 * @param seedInput three bytes as retrieved from the ECU
 * @param keyOut    pointer to a buffer to store the result
 * @return size_t   0 if successful, < 0 if not. (from malloc)
 */
size_t RX8::calculateKey(uint8_t* seedInput, uint8_t** keyOut)
{
	*keyOut = (uint8_t*)malloc(3);
	if (!(*keyOut)) return ENOMEM;

	uint8_t secret[5] = MAZDA_KEY_SECRET;
	uint32_t seed = (seedInput[0] << 16) + (seedInput[1] << 8) + seedInput[2];
	uint32_t or_ed_seed = ((seed & 0xFF0000) >> 16) | (seed & 0xFF00) | (secret[0] << 24) | (seed & 0xff) << 16;
	uint32_t mucked_value = 0xc541a9;
	for (size_t i = 0; i < 32; i++) {
		uint32_t a_bit = ((or_ed_seed >> i) & 1 ^ mucked_value & 1) << 23;
		uint32_t v9, v10, v8;
		v9 = v10 = v8 = a_bit | (mucked_value >> 1);
		mucked_value = v10 & 0xEF6FD7 | ((((v9 & 0x100000) >> 20) ^ ((v8 & 0x800000) >> 23)) << 20) | (((((mucked_value >> 1) & 0x8000) >> 15) ^ ((v8 & 0x800000) >> 23)) << 15) | (((((mucked_value >> 1) & 0x1000) >> 12) ^ ((v8 & 0x800000) >> 23)) << 12) | 32 * ((((mucked_value >> 1) & 0x20) >> 5) ^ ((v8 & 0x800000) >> 23)) | 8 * ((((mucked_value >> 1) & 8) >> 3) ^ ((v8 & 0x800000) >> 23));
	}

	for (size_t j = 0; j < 32; j++) {
		uint32_t a_bit = ((((secret[4] << 24) | (secret[3] << 16) | secret[1] | (secret[2] << 8)) >> j) & 1 ^ mucked_value & 1) << 23;
		uint32_t v14, v13, v12;
		v14 = v13 = v12 = a_bit | (mucked_value >> 1);
		mucked_value = v14 & 0xEF6FD7 | ((((v13 & 0x100000) >> 20) ^ ((v12 & 0x800000) >> 23)) << 20) | (((((mucked_value >> 1) & 0x8000) >> 15) ^ ((v12 & 0x800000) >> 23)) << 15) | (((((mucked_value >> 1) & 0x1000) >> 12) ^ ((v12 & 0x800000) >> 23)) << 12) | 32 * ((((mucked_value >> 1) & 0x20) >> 5) ^ ((v12 & 0x800000) >> 23)) | 8 * ((((mucked_value >> 1) & 8) >> 3) ^ ((v12 & 0x800000) >> 23));
	}
	uint32_t key = ((mucked_value & 0xF0000) >> 16) | 16 * (mucked_value & 0xF) | ((((mucked_value & 0xF00000) >> 20) | ((mucked_value & 0xF000) >> 8)) << 8) | ((mucked_value & 0xFF0) >> 4 << 16);
	(*keyOut)[0] = (key & 0xff0000) >> 16;
	(*keyOut)[1] = (key & 0xff00) >> 8;
	(*keyOut)[2] = key & 0xff;
	return 0;
}

/**
 * @brief unlock the ECU to allow read/writes
 * 
 * @param key the key calculated with the seed
 * @return size_t 1 if successful, 0 if not.
 */
size_t RX8::unlock(uint8_t* key) 
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[unlock] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid        = UDS_SID_SECURITY;
	request->payload[0] = MAZDA_SBF_CHECK_KEY;
	request->payload[1] = key[0];
	request->payload[2] = key[1];
	request->payload[3] = key[2];
	request->length     = 4;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[unlock] request failed %s", uds_request_negative_response_error_string(request));
		return 0;
	} else if(ret) {
		LOGE(TAG, "[unlock] request failed %s", uds_request_error_string(request, ret));
		return 0;
	} else {
		assert(request->length == 1);
		return (request->sid == UDS_SID_SECURITY_ACK) && (request->payload[0] == 0x02);
	}

	LOGE(TAG, "[unlock] unknown error!");
	return 0;
}

/**
 * @brief Reads memory at address of size ChunkSize into data
 * 
 * if address is >= 0xffff6000, the read will be from RAM. 
 * reads between 0x80000 and 0xffff6000 are undefined, but will not error.
 * 
 * @param address - start address of the read
 * @param chunkSize - amount of data to read
 * @param data - pointer to a buffer. If this pointer is null, it will be malloc'd.
 * @return size_t 0 if successful, > 0 otherwise
 */
size_t RX8::readMem(uint32_t address, uint16_t chunkSize, char* data)
{
	if(!request) return 1;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[unlock] failed to prepare request %s", uds_request_error_string(request, ret));
		return ret;
	}

	request->sid        = UDS_SID_READ_MEMORY_BY_ADDRESS;
	request->payload[0] = 0;
	request->payload[1] = address >> 16;
	request->payload[2] = address >> 8;
	request->payload[3] = address;
	request->payload[4] = chunkSize >> 8;
	request->payload[5] = chunkSize;
	request->length     = 6;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[readMem] request failed %s", uds_request_negative_response_error_string(request));
		return 1;
	} else if(ret) {
		LOGE(TAG, "[readMem] request failed %s", uds_request_error_string(request, ret));
		return 1;
	} else if(request->sid == UDS_SID_READ_MEMORY_BY_ADDRESS_ACK) {
		debug("l=%08x c=%08x t=%08x", request->length, chunkSize, request->rxBuffer[0].DataSize);
		
		assert(request->length == chunkSize + 1);
		memcpy(data, request->payload+1, chunkSize);
		return 0;
	}

	LOGE(TAG, "[readMem] unknown error!");
	return 1;
}

/**
 * 7E0#04 B1 00 B2 00 00 00 00 ????
 * 7E8#03 F1 00 B2 00 00 00 00 ????
 */
size_t RX8::requestBootloaderMode()
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[requestBootloaderMode] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid        = 0xB1;
	request->payload[0] = 0x0;
	request->payload[1] = 0xB2;
	request->payload[2] = 0x0;
	request->length     = 3;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[requestBootloaderMode] request failed %s", uds_request_negative_response_error_string(request));
	} else if(ret) {
		LOGE(TAG, "[requestBootloaderMode] request failed %s", uds_request_error_string(request, ret));
	} else {
		return (request->sid == 0xB1);
	}

	LOGE(TAG, "[requestBootloaderMode] unknown error!");
	return 0;
}

/*
7E0#10 09 34 00 00 40 00 00 
7E0#21 07 F8 00 00 00 00 00
*/
size_t RX8::requestDownload(uint16_t chunk_size, uint32_t size)
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[requestDownload] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid        = UDS_SID_REQUEST_DOWNLOAD;
	request->payload[0] = 0x0;
	request->payload[1] = chunk_size >> 24;
	request->payload[2] = chunk_size >> 16;
	request->payload[3] = chunk_size;
	request->payload[4] = size >> 24;
	request->payload[5] = size >> 16;
	request->payload[6] = size >> 8;
	request->payload[7] = size >> 0;
	request->length     = 8;

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[requestDownload] request failed %s", uds_request_negative_response_error_string(request));
	} else if(ret) {
		LOGE(TAG, "[requestDownload] request failed %s", uds_request_error_string(request, ret));
	} else {
		assert(request->length == 0x4);
		return (request->sid == UDS_SID_REQUEST_DOWNLOAD_ACK);
	}

	LOGE(TAG, "[requestDownload] unknown error!");
	return 0;
}

/*
7E0#14 01 36 9D 6F 4D 0B 00 - 36 - transfer data
*/
size_t RX8::transferData(uint32_t chunkSize, unsigned char* data)
{
	if(!request) return 1;
	size_t ret = 1;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[transferData] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid    = UDS_SID_TRANSFER_DATA;
	request->length = chunkSize + 1; 
	memcpy(request->payload, data, chunkSize + 1);

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[transferData] request failed %s", uds_request_negative_response_error_string(request));
	} else if(ret) {
		LOGE(TAG, "[transferData] request failed %s", uds_request_error_string(request, ret));
	} else {
		return !(request->sid == UDS_SID_TRANSFER_DATA_ACK);
	}

	LOGE(TAG, "[transferData] unknown error!");
	return 1;
}

size_t RX8::requestTransferExit()
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[requestTransferExit] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid    = UDS_SID_REQUEST_TRANSFER_EXIT;
	request->length = 0; 

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[requestTransferExit] request failed %s", uds_request_negative_response_error_string(request));
	} else if(ret) {
		LOGE(TAG, "[requestTransferExit] request failed %s", uds_request_error_string(request, ret));
	} else {
		return (request->sid == UDS_SID_REQUEST_TRANSFER_EXIT_ACK);
	}

	LOGE(TAG, "[requestTransferExit] unknown error!");
	return 0;
}

size_t RX8::reset()
{
	if(!request) return 0;
	size_t ret = 0;
	ret = uds_request_prepare(request);
	if(ret) {
		LOGE(TAG, "[requestTransferExit] failed to prepare request %s", uds_request_error_string(request, ret));
		return 0;
	}

	request->sid        = UDS_SID_RESET;
	request->payload[0] = 0x01;
	request->length     = 1; 

	ret = uds_request_send(request);
	if(ret == UDS_ERROR_NEGATIVE_RESPONSE) {
		LOGE(TAG, "[requestTransferExit] request failed %s", uds_request_negative_response_error_string(request));
		return 0;
	} else if(ret) {
		LOGE(TAG, "[requestTransferExit] request failed %s", uds_request_error_string(request, ret));
		return 0;
	} else {
		return (request->sid == UDS_SID_RESET_ACK);
	}

	LOGE(TAG, "[requestTransferExit] unknown error!");
	return 0;
}
