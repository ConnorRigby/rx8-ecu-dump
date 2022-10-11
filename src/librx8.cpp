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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "..\common\J2534.h"

#include "librx8.h"
#include "UDS.h"
#include "OBD2.h"
#include "util.h"

static const char* TAG = "RX8";

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

/* Fills buffer `vin` with the vin. returns 0 on success */
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
					for (uint8_t i = 0, j = 7; j < _rx_payload[0].DataSize; i++, j++)
						(*vin)[i] = _rx_payload[0].Data[j];
					return 0;
				}
			}

			LOGE(TAG, "[getVIN] Unknown message");
			dump_msg(&_rx_payload[0]);
			goto cleanup;
		}
	}

cleanup:
	if((*vin) != NULL)
		free(*vin);
	*vin = NULL;
	return 1;
}

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

size_t RX8::initDiagSession()
{
	unsigned long numTx = 1, numRx = 1;
	prepareUDSRequest(_tx_payload, _rx_payload, numTx, numRx);
	_tx_payload[0].Data[4] = UDS_SID_SESSION;
	_tx_payload[0].Data[5] = MAZDA_SBF_SESSION_87;
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
				LOGE(TAG, "[getSeed] unknown error");
				dump_msg(&_rx_payload[0]);
				return 0;
			}

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB)
				return (_rx_payload[0].Data[4] == UDS_SID_SESSION_ACK) && (_rx_payload[0].Data[5] == MAZDA_SBF_SESSION_87);

			LOGE(TAG, "[initDiagSession] Unknown message");
			dump_msg(&_rx_payload[0]);
			return 0;
		}
	}

	// unreachable
	return 0;
}


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

			if (_rx_payload[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) return (_rx_payload[0].Data[4] == 0x67) && (_rx_payload[0].Data[5] == 0x02);

			LOGE(TAG, "[unlock] Unknown message");
			dump_msg(&_rx_payload[0]);
			return 0;
		}
	}

	// unreachable
	return 0;
}

size_t RX8::readMem(uint32_t start, uint16_t chunkSize, char** data)
{
	*data = (char*)malloc(chunkSize);
	if (!(*data)) return -ENOMEM;

	unsigned long NumMsgs = 1;
	PASSTHRU_MSG payload[2] = { 0 };
	payload[0].ProtocolID = ISO15765;
	payload[0].TxFlags = ISO15765_FRAME_PAD;

	payload[0].Data[0] = 0x0;
	payload[0].Data[1] = 0x0;
	payload[0].Data[2] = 0x07;
	payload[0].Data[3] = 0xE0;

	// ReadMemoryByAddress
	payload[0].Data[4] = UDS_SID_READ_MEMORY_BY_ADDRESS;
	payload[0].Data[5] = 0x00;
	payload[0].Data[6] = 0x00;
	payload[0].Data[7] = 0x00;
	payload[0].Data[8] = 0x20;
	payload[0].Data[9] = 0x00;
	payload[0].Data[10] = 0xff;
	// 7E0#07 23 00 00 20 00 00 40

#if 0
	// address
	payload[0].Data[6] = (start >> 16) & 0xff;
	payload[0].Data[7] = (start >> 8) & 0xff;
	payload[0].Data[8] = (start) & 0xff;

	// length
	payload[0].Data[9] = (chunkSize >> 8) & 0xff;
	payload[0].Data[10] = (chunkSize) & 0xff;
#endif
	payload[0].DataSize = 11;

	if (_j2534.PassThruWriteMsgs(_chanID, &payload[0], &NumMsgs, 100)) {
		LOGE(TAG, "[readMem] failed to write messages");
		reportJ2534Error(_j2534);
		goto cleanup;
	}

	// 0x7D000;

	unsigned long numRxMsg = 1;
	PASSTHRU_MSG rxmsg[1] = { 0 };
	rxmsg[0].ProtocolID = ISO15765;
	rxmsg[0].TxFlags = ISO15765_FRAME_PAD;

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &rxmsg[0], &numRxMsg, 1000)) {
			LOGE(TAG, "[readMem] failed to read messages");
			reportJ2534Error(_j2534);
			goto cleanup;
		}
		if (numRxMsg) {
			if (rxmsg[0].Data[3] == MAZDA_REQUEST_CANID_LSB)
				continue;
			
			if (rxmsg[0].Data[3] == MAZDA_RESPONSE_CANID_LSB) {
				
				if (rxmsg[0].Data[4] == UDS_NEGATIVE_RESPONSE) {
					LOGE(TAG, "[readMem] dump failed");
					dump_msg(&rxmsg[0]);
					goto cleanup;
				}
				
				else if (rxmsg[0].Data[4] == 0x63) {
					LOGI(TAG, "[readMem] got data");
					hexdump_msg(&rxmsg[0]);
					goto cleanup;
				}
			}
			else {
				LOGE(TAG, "[readMem] Unknown message");
				dump_msg(&rxmsg[0]);
				goto cleanup;
			}
		}
	}
cleanup:
	if ((*data) != NULL)
		free(*data);
	*data = NULL;
	return 1;
}

// helper function to clear the tx and rx buffers, and setup a 0x7E0 CANID ISO15765 frame
static size_t prepareUDSRequest(PASSTHRU_MSG* tx_buffer, PASSTHRU_MSG* rx_buffer, size_t numTx, size_t numRx)
{
	assert(numTx <= TX_BUFFER_LEN);
	assert(numRx <= RX_BUFFER_LEN);

	// zero out both buffers to prevent accidental tx/rx of irrelevant data
	memset(tx_buffer, 0, TX_BUFFER_LEN);
	memset(rx_buffer, 0, RX_BUFFER_LEN);

	for (size_t i = 0; i < numTx; i++) {
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