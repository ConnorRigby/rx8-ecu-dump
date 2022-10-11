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

static const char* TAG = "ECUDump";

static const long STATUS_OK            = 0;
static const long STATUS_FAIL_PASSTHRU = 1;
static const long STATUS_FAIL_VINCHECK = 2;
static const long STATUS_FAIL_CALID    = 3;
static const long STATUS_FAIL_DIAG     = 4;
static const long STATUS_FAIL_SEED     = 5;
static const long STATUS_FAIL_KEY      = 6;
static const long STATUS_FAIL_UNLOCK   = 7;

static J2534 j2534;
static RX8* ecu;
static unsigned long devID, chanID;
static const unsigned int CAN_BAUD = 500000;

size_t j2534Initialize()
{
	if (!j2534.init())
	{
		LOGE(TAG, "failed to connect to J2534 DLL.");
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruOpen(NULL, &devID))
	{
		LOGE(TAG, "failed to PassThruOpen()");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}

	// use SNIFF_MODE to listen to packets without any acknowledgement or flow control
	// use CAN_ID_BOTH to pick up both 11 and 29 bit CAN messages
	if (j2534.PassThruConnect(devID, ISO15765, CAN_ID_BOTH, CAN_BAUD, &chanID))
	{
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}
	j2534.PassThruIoctl(chanID, CLEAR_MSG_FILTERS, NULL, NULL);

	unsigned long filterID = 0;

	PASSTHRU_MSG maskMSG = {0};
	PASSTHRU_MSG maskPattern = {0};
	PASSTHRU_MSG flowControlMsg = {0};
	for (uint8_t i = 0; i < 7; i++) {
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
			return STATUS_FAIL_PASSTHRU;

		}
	}
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
		LOGE(TAG, "Failed to set message filter");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;

	}

	if (j2534.PassThruIoctl(chanID, CLEAR_TX_BUFFER, NULL, NULL))
	{
		LOGE(TAG, "Failed to clear j2534 TX buffer");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL)) 
	{
		LOGE(TAG, "Failed to clear j2534 RX buffer");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}
	return STATUS_OK;
}

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned long status = 0;
	char* vin, * calibrationID, * dump;
	uint8_t* seed, * key;

	if (j2534Initialize()) {
		LOGE(TAG, "j2534Initialize() failed");
		// no need to cleanup, just return here
		return -STATUS_FAIL_PASSTHRU;
	}
	LOGI(TAG, "j2534 connection initialized ok");
	ecu = new RX8(j2534, devID, chanID);

#if 1
	if (ecu->getVIN(&vin)) {
		LOGE(TAG, "failed to get VIN");
		status = -STATUS_FAIL_VINCHECK;
		goto cleanup;
	}
	LOGI(TAG, "Got VIN = %s", vin);

	if (ecu->getCalibrationID(&calibrationID)) {
		LOGE(TAG, "failed to get Calibration ID");
		status = -STATUS_FAIL_CALID;
		goto cleanup;
	}
	LOGI(TAG, "Got calibration ID = %s", calibrationID);

	if (!ecu->initDiagSession()) {
		LOGE(TAG, "failed to init diag session");
		status = -STATUS_FAIL_DIAG;
		goto cleanup;
	}
	LOGI(TAG, "Diag sesion initialized ok");

	if (ecu->getSeed(&seed)) {
		LOGE(TAG, "failed to get seed");
		status = -STATUS_FAIL_SEED;
		goto cleanup;
	}
	LOGI(TAG, "Got seed = { %02X, %02X, %02X }", seed[0], seed[1], seed[2]);
	if (ecu->calculateKey(seed, &key)) {
		LOGE(TAG, "failed to calculate key");
		status = -STATUS_FAIL_KEY;
		goto cleanup;
	}
	LOGI(TAG, "Got key = { %02X, %02X, %02X }", key[0], key[1], key[2]);
	
	if (!ecu->unlock(key)) {
		LOGE(TAG, "failed to unlock ECU using key");
		status = -STATUS_FAIL_UNLOCK;
		goto cleanup;
	}
	LOGI(TAG, "Unlocked ECU");
#endif
	LOGI(TAG, "Starting Dump");
	
	if (ecu->readMem(0x0, 0x05, &dump)) {
		LOGE(TAG, "Failed to dump ROM");
		goto cleanup;
	}
	LOGI(TAG, "Dump complete");

// TODO: it seems like calling PassThruDisconnect or PassThruClose causes
//       error inside the J2534 dll. For now, skip it
cleanup:
	goto skip_cleanup;

	LOGI(TAG, "Start cleanup");

	// shut down the channel
	if (j2534.PassThruDisconnect(chanID)) {
		LOGE(TAG, "PassThruDisconnect failed");
		return -STATUS_FAIL_PASSTHRU;
	}
	LOGI(TAG, "PassThruDisconnect ok");

	// close the device
	if (j2534.PassThruClose(devID)) {
		LOGE(TAG, "PassThruClose failed");
		return -STATUS_FAIL_PASSTHRU;
	}
	LOGI(TAG, "PassThruClose ok");

skip_cleanup:
	return status;
}
