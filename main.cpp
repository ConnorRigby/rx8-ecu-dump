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

J2534 j2534;
unsigned long devID;
unsigned long chanID;
unsigned int baudrate = 500000;

void reportJ2534Error()
{
	char err[512];
	j2534.PassThruGetLastError(err);
	printf("J2534 error [%s].\n",err);
}

void dump_msg(PASSTHRU_MSG* msg)
{
	if (msg->RxStatus & START_OF_MESSAGE)
		return;
	printf("[%u] ", msg->Timestamp);
	for (unsigned int i = 0; i < msg->DataSize; i++)
		printf("%02X ", msg->Data[i]);
	printf("\n");
}

// 17 characters + a null terminator
#define VIN_LENGTH 18

/* Fills buffer `vin` with the vin. returns 0 on success */
size_t getVIN(char** vin)
{
	*vin = (char*)malloc(VIN_LENGTH);
	if (!(*vin))
		return -ENOMEM;

	// clear buffer
	memset(*vin, 0, VIN_LENGTH);

	PASSTHRU_MSG getVIN[5] = { 0 };
	for (int i = 0; i < 5; i++) {
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

	unsigned long NumMsgs = 2;
	if (j2534.PassThruWriteMsgs(chanID, &getVIN[1], &NumMsgs, 100)) {
		printf("failed to write messages\n");
		reportJ2534Error();
		return 1;
	}

	//JM1FE173370212600
	// 4a 4d 31 46 45 31 37 33 33 37 30 32 31 32 36 30 30

	unsigned long numRxMsg = 1;
	PASSTHRU_MSG rxmsg[1] = {0};
	rxmsg[0].ProtocolID = ISO15765;
	rxmsg[0].TxFlags = ISO15765_FRAME_PAD;

	for (;;) {
		if (j2534.PassThruReadMsgs(chanID, &rxmsg[0], &numRxMsg, 1000)) {
			printf("failed to read messages\n");
			reportJ2534Error();
			return 1;
		}
		if (numRxMsg) {
			if (rxmsg[0].Data[3] == 0xE0)
				continue;
			if (rxmsg[0].Data[3] == 0xE8) {
				if (rxmsg[0].Data[4] == 0x49) {
					for (size_t i = 0,j=7; j < rxmsg[0].DataSize; i++,j++) {
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

int _tmain(int argc, _TCHAR* argv[])
{
	if (!j2534.init())
	{
		printf("can't connect to J2534 DLL.\n");
		return 0;
	}

	if (j2534.PassThruOpen(NULL,&devID))
	{
		reportJ2534Error();
		return 0;
	}

	// use SNIFF_MODE to listen to packets without any acknowledgement or flow control
	// use CAN_ID_BOTH to pick up both 11 and 29 bit CAN messages
	if (j2534.PassThruConnect(devID, ISO15765,SNIFF_MODE|CAN_ID_BOTH,baudrate,&chanID))
	{
		reportJ2534Error();
		return 0;
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


		if (j2534.PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER,&maskMSG,&maskPattern,&flowControlMsg,&filterID))
		{
			reportJ2534Error();
			return 0;

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
		return 0;

	}
#endif
	// clear buffers
	j2534.PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL);

	char* vin;
	if (getVIN(&vin)) {
		printf("failed to get VIN\n");
		goto cleanup;
	}
	printf("vin=%s %d\n", vin, strlen(vin));

cleanup:
	// shut down the channel
	if (j2534.PassThruDisconnect(chanID))
	{
		reportJ2534Error();
		return 0;
	}

	// close the device

	if (j2534.PassThruClose(devID))
	{
		reportJ2534Error();
		return 0;
	}
	return 0;
}
