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

#include "..\common\J2534.h"

#include "librx8.h"
#include "util.h"

RX8::RX8(J2534& j2534, unsigned long devID, unsigned long chanID) {
	_j2534 = j2534;
	_devID = devID;
	_chanID = chanID;
}


/* Fills buffer `vin` with the vin. returns 0 on success */
size_t RX8::getVIN(char** vin)
{
	*vin = (char*)malloc(VIN_LENGTH);
	if (!(*vin))
		return -ENOMEM;

	// clear buffer
	memset(*vin, 0, VIN_LENGTH);

	unsigned long NumMsgs = 2;
	PASSTHRU_MSG getVIN[3] = { 0 };
	for (int i = 0; i < 3; i++) {
		getVIN[i].ProtocolID = ISO15765;
		getVIN[i].TxFlags = ISO15765_FRAME_PAD;
		// request 7e0
		getVIN[i].Data[0] = 0x0;
		getVIN[i].Data[1] = 0x0;
		getVIN[i].Data[2] = 0x07;
		getVIN[i].Data[3] = 0xE0;
	}

	getVIN[0].Data[4] = 0x01;
	getVIN[0].Data[5] = 0x0c;
	getVIN[0].DataSize = 6;

	getVIN[1].Data[4] = 0x09;
	getVIN[1].Data[5] = 0x02;
	getVIN[1].DataSize = 6;

	getVIN[2].ProtocolID = CAN;
	getVIN[2].TxFlags = ISO15765_ADDR_TYPE;
	getVIN[2].DataSize = 12;
	getVIN[2].Data[0] = 0x0;
	getVIN[2].Data[1] = 0x0;
	getVIN[2].Data[2] = 0x07;
	getVIN[2].Data[3] = 0xE0;
	getVIN[2].Data[4] = 0x30;
	getVIN[2].Data[5] = 0x00;
	getVIN[2].Data[6] = 0x00;
	getVIN[2].Data[7] = 0x00;

	if (_j2534.PassThruWriteMsgs(_chanID, &getVIN[1], &NumMsgs, 100)) {
		printf("failed to write messages\n");
		reportJ2534Error(_j2534);
		return 1;
	}

	unsigned long numRxMsg = 1;
	PASSTHRU_MSG rxmsg[1] = { 0 };
	rxmsg[0].ProtocolID = ISO15765;
	rxmsg[0].TxFlags = ISO15765_FRAME_PAD;

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &rxmsg[0], &numRxMsg, 1000)) {
			printf("failed to read messages\n");
			reportJ2534Error(_j2534);
			return 1;
		}
		if (numRxMsg) {
			if (rxmsg[0].Data[3] == 0xE0)
				continue;
			if (rxmsg[0].Data[3] == 0xE8) {
				if (rxmsg[0].Data[4] == 0x49) {
					for (size_t i = 0, j = 7; j < rxmsg[0].DataSize; i++, j++) {
						(*vin)[i] = rxmsg[0].Data[j];
					}
					return 0;
				}
			}
			else {
				printf("Unknown message\n");
				dump_msg(&rxmsg[0]);
				return 1;
			}
		}
	}

	// probably not possible?
	return 1;
}

size_t RX8::getCalibrationID(char** calibrationID)
{
	*calibrationID = (char*)malloc(CALIBRATION_ID_LENGTH);
	if (!(*calibrationID))
		return -ENOMEM;

	// clear buffer
	memset(*calibrationID, 0, CALIBRATION_ID_LENGTH);

	unsigned long NumMsgs = 2;
	PASSTHRU_MSG payload[2] = { 0 };
	for (int i = 0; i < 2; i++) {
		payload[i].ProtocolID = ISO15765;
		payload[i].TxFlags = ISO15765_FRAME_PAD;
		// request 7e0
		payload[i].Data[0] = 0x0;
		payload[i].Data[1] = 0x0;
		payload[i].Data[2] = 0x07;
		payload[i].Data[3] = 0xE0;
	}

	payload[0].Data[4] = 0x09;
	payload[0].Data[5] = 0x04;
	payload[0].DataSize = 6;

	payload[1].ProtocolID = CAN;
	payload[1].TxFlags = ISO15765_ADDR_TYPE;
	payload[1].DataSize = 12;
	payload[1].Data[0] = 0x0;
	payload[1].Data[1] = 0x0;
	payload[1].Data[2] = 0x07;
	payload[1].Data[3] = 0xE0;
	payload[1].Data[4] = 0x30;
	payload[1].Data[5] = 0x00;
	payload[1].Data[6] = 0x00;
	payload[1].Data[7] = 0x00;

	if (_j2534.PassThruWriteMsgs(_chanID, &payload[0], &NumMsgs, 100)) {
		printf("failed to write messages\n");
		reportJ2534Error(_j2534);
		return 1;
	}

	unsigned long numRxMsg = 1;
	PASSTHRU_MSG rxmsg[1] = { 0 };
	rxmsg[0].ProtocolID = ISO15765;
	rxmsg[0].TxFlags = ISO15765_FRAME_PAD;

	for (;;) {
		if (_j2534.PassThruReadMsgs(_chanID, &rxmsg[0], &numRxMsg, 1000)) {
			printf("failed to read messages\n");
			reportJ2534Error(_j2534);
			return 1;
		}
		if (numRxMsg) {
			if (rxmsg[0].Data[3] == 0xE0)
				continue;
			if (rxmsg[0].Data[3] == 0xE8) {
				if (rxmsg[0].Data[4] == 0x49) {
					for (size_t i = 0, j = 7; j < rxmsg[0].DataSize; i++, j++) {
						(*calibrationID)[i] = rxmsg[0].Data[j];
					}
					return 0;
				}
			}
			else {
				printf("Unknown message\n");
				dump_msg(&rxmsg[0]);
				return 1;
			}
		}
	}

	return 1;
}