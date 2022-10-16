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
RX8::RX8(J2534& j2534, unsigned long devID, unsigned long chanID)
{
	_j2534 = j2534;
	_devID = devID;
	_chanID = chanID;

	// initialize payload buffers to something noticable in memory
	// to make debugging easier
	memset(_tx_payload, 255, sizeof(PASSTHRU_MSG) * TX_BUFFER_LEN);
	memset(_rx_payload, 255, sizeof(PASSTHRU_MSG) * RX_BUFFER_LEN);
}

/**
 * @brief get the VIN from the ECU
 * 
 * @param vin     pointer to a buffer to store the vin
 * @return size_t 0 on success > 0 otherwise
 */
size_t RX8::getVIN(char** vin)
{
	*vin = (char*)malloc(VIN_LENGTH);
	if (!(*vin)) return -ENOMEM;
	memset(*vin, 0, VIN_LENGTH);

	unsigned long numTx=1, numRx=1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = OBD2_SID_REQUEST_VEHICLE_INFORMATION;
	_tx_payload[0].Data[5] = OBD2_PID_REQUEST_VIN;
	_tx_payload[0].DataSize = 6;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[getVIN] failed to write messages");
		reportJ2534Error(_j2534);
		goto cleanup;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[getVIN] failed to read messages");
			reportJ2534Error(_j2534);
			goto cleanup;
		}
		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == OBD2_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[getVIN] unknown error");
				dump_msg(&_rx_payload[0]);
				goto cleanup;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
				if (_rx_payload[0].Data[4] == OBD2_SID_REQUEST_VEHICLE_INFORMATION_ACK) {
					// replace the ' ' characters with null terminators. 
					if (_rx_payload[0].Data[17] == 0x20) {
						for (uint8_t i = 0, j = 17; i < VIN_LENGTH; i++, j++)
							_rx_payload[0].Data[j] = 0x0;
					}

					for (uint8_t i = 0, j = 7; j < _rx_payload[0].DataSize; i++, j++)
						(*vin)[i] = _rx_payload[0].Data[j];

					return 0;
				}
				LOGE(TAG, "Invalid response from ECU");
				dump_msg(&_rx_payload[0]);
				goto cleanup;
			}

			LOGE(TAG, "[getVIN] Unknown message");
			dump_msg(&_rx_payload[0]);
			continue;
			goto cleanup;
		}
	}

cleanup:
	if((*vin) != NULL)
		free(*vin);
	*vin = NULL;
	return 1;
}

/**
 * @brief reads the calid from the ECU
 * 
 * @param calibrationID pointer to a buffer to store the id
 * @return size_t       0 if successful, >0 if not
 */
size_t RX8::getCalibrationID(char** calibrationID)
{
	*calibrationID = (char*)malloc(CALIBRATION_ID_LENGTH);
	if (!(*calibrationID)) return -ENOMEM;
	memset(*calibrationID, 0, CALIBRATION_ID_LENGTH);

	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = OBD2_SID_REQUEST_VEHICLE_INFORMATION;
	_tx_payload[0].Data[5] = OBD2_PID_REQUEST_CALID;
	_tx_payload[0].DataSize = 6;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[getCalibrationID] failed to write messages");
		reportJ2534Error(_j2534);
		goto cleanup;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[getCalibrationID] failed to read messages");
			reportJ2534Error(_j2534);
			goto cleanup;
		}
		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == OBD2_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[getCalibrationID] unknown error");
				dump_msg(&_rx_payload[0]);
				goto cleanup;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
				if (_rx_payload[0].Data[4] == OBD2_SID_REQUEST_VEHICLE_INFORMATION_ACK) {
					for (size_t i = 0, j = 7; j < _rx_payload[0].DataSize; i++, j++)
						(*calibrationID)[i] = _rx_payload[0].Data[j];
					return 0;
				}
			}

			LOGE(TAG, "[getCalibrationID] Unknown message");
			dump_msg(&_rx_payload[0]);
			goto cleanup;
		}
	}

cleanup:
	if ((*calibrationID) != NULL)
		free(*calibrationID);
	*calibrationID = NULL;
	return 1;
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
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_SESSION;
	_tx_payload[0].Data[5] = session;
	_tx_payload[0].DataSize = 6;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[initDiagSession] failed to write messages");
		reportJ2534Error(_j2534);
		return 0;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[initDiagSession] failed to read messages");
			reportJ2534Error(_j2534);
			return 0;
		}
		
		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[initDiagSession] unknown error");
				dump_msg(&_rx_payload[0]);
				return 0;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB)
				return (_rx_payload[0].Data[4] == UDS_SID_SESSION_ACK) && (_rx_payload[0].Data[5] == session);

			LOGE(TAG, "[initDiagSession] Unknown message");
			dump_msg(&_rx_payload[0]);
			return 0;
		}
	}

	// unreachable
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
	*seed = (uint8_t*)malloc(SEED_LENGTH);
	if (!(*seed)) return -ENOMEM;
	memset(*seed, 0, SEED_LENGTH);

	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_SECURITY;
	_tx_payload[0].Data[5] = MAZDA_SBF_REQUEST_SEED;
	_tx_payload[0].DataSize = 6;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[getSeed] failed to write messages");
		reportJ2534Error(_j2534);
		goto cleanup;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[getSeed] failed to read messages");
			reportJ2534Error(_j2534);
			goto cleanup;
		}

		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[getSeed] unknown error");
				dump_msg(&_rx_payload[0]);
				goto cleanup;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
				if (_rx_payload[0].Data[4] == UDS_SID_SECURITY_ACK) {
					(*seed)[0] = (uint8_t)_rx_payload[0].Data[6];
					(*seed)[1] = (uint8_t)_rx_payload[0].Data[7];
					(*seed)[2] = (uint8_t)_rx_payload[0].Data[8];
					return 0;
				}
			}
			LOGE(TAG, "[getSeed] Unknown message");
			dump_msg(&_rx_payload[0]);
			goto cleanup;
		}
	}
cleanup:
	if ((*seed) != NULL)
		free(*seed);
	*seed = NULL;
	return 1;
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
	if (!(*keyOut)) return -ENOMEM;

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
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_SECURITY;
	_tx_payload[0].Data[5] = MAZDA_SBF_CHECK_KEY;
	_tx_payload[0].Data[6] = key[0];
	_tx_payload[0].Data[7] = key[1];
	_tx_payload[0].Data[8] = key[2];
	_tx_payload[0].DataSize = 9;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[unlock] failed to write messages");
		reportJ2534Error(_j2534);
		return 0;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[unlock] failed to read messages");
			reportJ2534Error(_j2534);
			return 0;
		}

		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[unlock] unknown error");
				dump_msg(&_rx_payload[0]);
				return 0;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) 
				return (_rx_payload[0].Data[4] == 0x67) && (_rx_payload[0].Data[5] == 0x02);

			LOGE(TAG, "[unlock] Unknown message");
			dump_msg(&_rx_payload[0]);
			return 0;
		}
	}

	// unreachable
	return 0;
}

/**
 * @brief Reads memory att address of size ChunkSize into data
 * 
 * if address is >= 0xffff6000, the read will be from RAM. 
 * reads between 0x80000 and 0xffff6000 are undefined, but will not error.
 * 
 * @param address - start address of the read
 * @param chunkSize - amount of data to read
 * @param data - pointer to a buffer. If this pointer is null, it will be malloc'd.
 * @return size_t 0 if successful, > 0 otherwise
 */
size_t RX8::readMem(uint32_t address, uint16_t chunkSize, char** data)
{
	if(*data == NULL) {
		*data = (char*)malloc(chunkSize);
		if (!(*data)) return -ENOMEM;
		memset((*data), 0, chunkSize);
	}

	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_READ_MEMORY_BY_ADDRESS;

	if(address >= 0xffff6000) {
		_tx_payload[0].Data[5] = 0xff;
		_tx_payload[0].Data[6] = 0xff;
	} else {
		_tx_payload[0].Data[5] = 0;
		_tx_payload[0].Data[6] = 0;
	}
	_tx_payload[0].Data[7] = address >> 8;
	_tx_payload[0].Data[8] = address;
	_tx_payload[0].Data[9] = chunkSize >> 8;
	_tx_payload[0].Data[10] = chunkSize;
	_tx_payload[0].DataSize = 11;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[readMem] failed to write messages");
		reportJ2534Error(_j2534);
		goto cleanup;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, 1000)) {
			LOGE(TAG, "[readMem] failed to read messages");
			reportJ2534Error(_j2534);
			goto cleanup;
		}
		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
				switch(_rx_payload[0].Data[6]) {
					case 0x31: {
						LOGE(TAG, "[readMem] request out of range");
						goto cleanup;
					}
					case 0x78: continue;
					default: {
						LOGE(TAG, "[readMem] unknown error");
						dump_msg(&_rx_payload[0]);
						goto cleanup;
					}
				}
			}
				
			if (_rx_payload[0].Data[4] == 0x63) {
				memcpy((*data), &_rx_payload[0].Data[5], chunkSize);
				return 0;
			}

			LOGE(TAG, "[readMem] Unknown message");
			dump_msg(&_rx_payload[0]);
			goto cleanup;
		}
	}
cleanup:
	if ((*data) != NULL)
		free(*data);
	*data = NULL;
	return 1;
}

/**
 * 7E0#04 B1 00 B2 00 00 00 00 ????
 * 7E8#03 F1 00 B2 00 00 00 00 ????
 */
size_t RX8::requestBootloaderMode()
{
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = 0xB1;
	_tx_payload[0].Data[5] = 0x0;
	_tx_payload[0].Data[6] = 0xB2;
	_tx_payload[0].Data[7] = 0x0;
	_tx_payload[0].DataSize = 8;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[requestBootloaderMode] failed to write messages");
		reportJ2534Error(_j2534);
		return 1;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[requestBootloaderMode] failed to read messages");
			reportJ2534Error(_j2534);
			return 1;
		}
		if (numRx) {
			if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
			if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
			if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
				LOGE(TAG, "[requestBootloaderMode] unknown error");
				dump_msg(&_rx_payload[0]);
				return 1;
			}

			return _rx_payload[0].Data[3] == 0xB1;
		}
	}

	// unreachable
	return 1;
}

/*
7E0#10 09 34 00 00 40 00 00 
7E0#21 07 F8 00 00 00 00 00
*/
size_t RX8::requestDownload(uint32_t size)
{
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_REQUEST_DOWNLOAD;
	_tx_payload[0].Data[5] = 0x00;
	_tx_payload[0].Data[6] = 0x00;
	_tx_payload[0].Data[7] = 0x40;
	_tx_payload[0].Data[8] = 0x00;

	_tx_payload[0].Data[9] = size >> 24;
	_tx_payload[0].Data[10] = size >> 16;
	_tx_payload[0].Data[11] = size >> 8;
	_tx_payload[0].Data[12] = size;

	_tx_payload[0].DataSize = 13;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[requestDownload] failed to write messages");
		reportJ2534Error(_j2534);
		return 0;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[requestDownload] failed to read messages");
			reportJ2534Error(_j2534);
			return 0;
		}
		if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
		if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
		if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
			LOGE(TAG, "[requestDownload] unknown error");
			dump_msg(&_rx_payload[0]);
			return 0;
		}

		// TODO: check (_rx_payload[0].Data[4] == 0x74) && (_rx_payload[0].Data[5] == 0x04)
		if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) 
			return 1;
	}
	
	// unreachable
	return 0;
}

/*
7E0#14 01 36 9D 6F 4D 0B 00 - 36 - transfer data
*/
size_t RX8::sendPayload(unsigned char* payload, uint32_t size)
{
	uint32_t frameIndex = 0;
	uint32_t chunkSize = 0x401;
	unsigned long numTx = 1, numRx = 1;

	while(frameIndex < size) {
		prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);

		_tx_payload[0].Data[4] = UDS_SID_TRANSFER_DATA;
		memcpy(&_tx_payload[0].Data[5], payload+frameIndex, chunkSize);
		_tx_payload[0].DataSize = chunkSize + 4;
		
		if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, 100)) {
			LOGE(TAG, "[sendPayload] failed to write frame %d", frameIndex);
			return 1;
		}

		for(;;) {
			if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, 1000)) {
				LOGE(TAG, "[sendPayload] failed to read reply frame %d", frameIndex);
				return 1;
			}
			if(numRx) {
				if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
				if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
				if ((_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) && (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE)) {
					if(_rx_payload[0].Data[6] == 0x78) continue; 
						LOGE(TAG, "[sendPayload] unknown error sending frame %d", frameIndex);
						dump_msg(&_rx_payload[0]);
						return 1;
					}

					if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
						if(_rx_payload[0].Data[4] != 0x76) {
							LOGE(TAG, "[sendPayload] error sending frame");
							return 1;
						}
						break;
					}

					LOGE(TAG, "[sendPayload] unexpected reply");
					dump_msg(&_rx_payload[0]);
			}
		}
		frameIndex+=0x400;
		//frameIndex+=0xff;
	}
	// unreachable
	return 0;
}

size_t RX8::requestTransferExit()
{
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = 0x37;
	_tx_payload[0].DataSize = 5;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[requestTransferExit] failed to write messages");
		reportJ2534Error(_j2534);
		return 0;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[requestTransferExit] failed to read messages");
			reportJ2534Error(_j2534);
			return 0;
		}
		if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
		if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
		if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
			if (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE) {
				LOGE(TAG, "[requestTransferExit] unknown error");
				dump_msg(&_rx_payload[0]);
				return 0;
			}

			return 1;
		}
	}
	
	// unreachable
	return 0;
}

size_t RX8::reset()
{
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = 0x11;
	_tx_payload[0].Data[5] = 0x01; // 0x02;// 0x00; // 0x01
	_tx_payload[0].DataSize = 6;

	if (_j2534.PassThruWriteMsgs(_chanID, &_tx_payload[0], &numTx, TX_TIMEOUT)) {
		LOGE(TAG, "[reset] failed to write messages");
		reportJ2534Error(_j2534);
		return 0;
	}

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &_rx_payload[0], &numRx, RX_TIMEOUT)) {
			LOGE(TAG, "[reset] failed to read messages");
			reportJ2534Error(_j2534);
			return 0;
		}
		if (_rx_payload[0].RxStatus & START_OF_MESSAGE) continue;
		if (_rx_payload[0].Data[3] == MAZDA_REQUEST_CANID_LSB) continue;
		if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
			if (_rx_payload[0].Data[4] == UDS_NEGATIVE_RESPONSE) {
				LOGE(TAG, "[reset] unknown error");
				dump_msg(&_rx_payload[0]);
				return 0;
			}

			return 1;
		}
	}
	
	// unreachable
	return 0;
}

// helper function to clear the tx and rx buffers, and setup a 0x7E0 CANID ISO15765 frame
size_t prepareUDSRequest(PASSTHRU_MSG* tx_buffer, PASSTHRU_MSG* rx_buffer, size_t numTx, size_t numRx)
{
	assert(numTx <= TX_BUFFER_LEN);
	assert(numRx <= RX_BUFFER_LEN);

	// zero out both buffers to prevent accidental tx/rx of irrelevant data
	memset(tx_buffer, 0, TX_BUFFER_LEN);
	memset(rx_buffer, 0, RX_BUFFER_LEN);

	for (size_t i = 0; i < numTx; i++) {
		memset(tx_buffer[i].Data, 0, PM_DATA_LEN);
		memset(rx_buffer[i].Data, 0, PM_DATA_LEN);

		tx_buffer[i].ProtocolID = ISO15765;
		tx_buffer[i].TxFlags = ISO15765_FRAME_PAD;

		
		tx_buffer[i].Data[0] = 0x0;
		tx_buffer[i].Data[1] = 0x0;
		tx_buffer[i].Data[2] = MAZDA_REQUEST_CANID_MSB;
		tx_buffer[i].Data[3] = MAZDA_REQUEST_CANID_LSB;
		rx_buffer[i].ProtocolID = ISO15765;
		rx_buffer[i].TxFlags = ISO15765_FRAME_PAD;
	}

	return 0;
}
