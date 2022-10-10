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

#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <conio.h>
#include <time.h>

#include "..\common\J2534.h"

#include "librx8.h"
#include "util.h"

J2534 j2534;
unsigned long devID, chanID;
unsigned int baudrate = 500000;

size_t j2534Initialize()
{
	if (!j2534.init())
	{
		printf("can't connect to J2534 DLL.\n");
		return 1;
	}

	if (j2534.PassThruOpen(NULL, &devID))
	{
		reportJ2534Error(j2534);
		return 1;
	}

	// use SNIFF_MODE to listen to packets without any acknowledgement or flow control
	// use CAN_ID_BOTH to pick up both 11 and 29 bit CAN messages
	if (j2534.PassThruConnect(devID, ISO15765, SNIFF_MODE | CAN_ID_BOTH, baudrate, &chanID))
	{
		reportJ2534Error(j2534);
		return 1;
	}

	unsigned long filterID = 0;


	PASSTHRU_MSG maskMSG{};
	PASSTHRU_MSG maskPattern{};
	PASSTHRU_MSG flowControlMsg{};
	for (unsigned long i = 0; i < 7; i++) {
		maskMSG.ProtocolID = ISO15765;
		maskMSG.TxFlags = ISO15765_FRAME_PAD;
		maskMSG.Data[0] = 0x00;
		maskMSG.Data[1] = 0x00;
		maskMSG.Data[2] = 0x07;
		maskMSG.Data[3] = 0xff;
		maskMSG.DataSize = 4;

		maskPattern.ProtocolID = ISO15765;
		maskPattern.TxFlags = ISO15765_FRAME_PAD;
		maskPattern.Data[0] = 0x00;
		maskPattern.Data[1] = 0x00;
		maskPattern.Data[2] = 0x07;
		maskPattern.Data[3] = (0xE8 + i);
		maskPattern.DataSize = 4;

		flowControlMsg.ProtocolID = ISO15765;
		flowControlMsg.TxFlags = ISO15765_FRAME_PAD;
		flowControlMsg.Data[0] = 0x00;
		flowControlMsg.Data[1] = 0x00;
		flowControlMsg.Data[2] = 0x07;
		flowControlMsg.Data[3] = (0xE0 + i);
		flowControlMsg.DataSize = 4;


		if (j2534.PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &maskMSG, &maskPattern, &flowControlMsg, &filterID))
		{
			reportJ2534Error(j2534);
			return 1;

		}
	}
#if 0
	maskMSG.ProtocolID = ISO15765;
	maskMSG.TxFlags = ISO15765_FRAME_PAD;
	maskMSG.Data[0] = 0x00;
	maskMSG.Data[1] = 0x00;
	maskMSG.Data[2] = 0x07;
	maskMSG.Data[3] = 0xf8;
	maskMSG.DataSize = 4;

	maskPattern.ProtocolID = ISO15765;
	maskPattern.TxFlags = ISO15765_FRAME_PAD;
	maskPattern.Data[0] = 0x00;
	maskPattern.Data[1] = 0x00;
	maskPattern.Data[2] = 0x07;
	maskPattern.Data[3] = 0xE8;
	maskPattern.DataSize = 4;


	if (j2534.PassThruStartMsgFilter(chanID, PASS_FILTER, &maskMSG, &maskPattern, NULL, &filterID))
	{
		reportJ2534Error();
		return 1;

	}
#endif
	// clear buffers
	j2534.PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (j2534Initialize()) {
		printf("j2534Initialize() failed\n");
		return 1;
	}
	printf("j2534 connection initialized ok\n");

	RX8 ecu(j2534, devID, chanID);
	
	char *vin, *calibrationID;
	uint8_t *seed, *key;

	if (ecu.getVIN(&vin)) {
		printf("failed to get VIN\n");
		goto cleanup;
	}
	printf("Got VIN = %s\n", vin);

	j2534.PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL);

	if (ecu.getCalibrationID(&calibrationID)) {
		printf("failed to get Calibration ID\n");
		goto cleanup;
	}
	printf("Got calibration ID = %s\n", calibrationID);

	if (!ecu.initDiagSession()) {
		printf("failed to init diag session\n");
		goto cleanup;
	}
	printf("diag sesion initialized ok\n");

	if (ecu.getSeed(&seed)) {
		printf("failed to get seed\n");
		goto cleanup;
	}
	printf("Got seed = { %02X, %02X, %02X }\n", seed[0], seed[1], seed[2]);
	if (ecu.calculateKey(seed, &key)) {
		printf("failed to calculate key\n");
		goto cleanup;
	}
	printf("Got key = { %02X, %02X, %02X }\n", key[0], key[1], key[2]);
	
	if (!ecu.unlock(key)) {
		printf("failed to unlock ECU using key\n");
		goto cleanup;
	}
	printf("Unlocked ECU\n");

cleanup:
	// shut down the channel
	if (j2534.PassThruDisconnect(chanID))
	{
		reportJ2534Error(j2534);
		return 0;
	}

	// close the device

	if (j2534.PassThruClose(devID))
	{
		reportJ2534Error(j2534);
		return 0;
	}
	return 0;
}
