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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#include <tchar.h>
#include <windows.h>
#include <conio.h>
// access
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#include "J2534.h"
#include "librx8.h"
#include "util.h"
#include "progressbar.h"
#include "args.h"

static const char* TAG = "ECUDump";

static const long STATUS_OK            = 0;
static const long STATUS_FAIL_PASSTHRU = 1;
static const long STATUS_FAIL_VINCHECK = 2;
static const long STATUS_FAIL_CALID    = 3;
static const long STATUS_FAIL_DIAG     = 4;
static const long STATUS_FAIL_SEED     = 5;
static const long STATUS_FAIL_KEY      = 6;
static const long STATUS_FAIL_UNLOCK   = 7;
static const long STATUS_FAIL_DOWNLOAD = 8;
static const long STATUS_FAIL_BOOTLOADER = 9;
static const long STATUS_FAIL_UPLOAD     = 9;
static const long STATUS_FAIL_RESET      = 10;

static J2534 j2534;
static RX8* ecu;
static unsigned long devID, chanID;
static const unsigned int CAN_BAUD = 500000;

size_t j2534Initialize()
{
	if (!j2534.init()) {
		LOGE(TAG, "failed to connect to J2534 DLL.");
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruOpen(NULL, &devID)) {
		LOGE(TAG, "failed to PassThruOpen()");
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruConnect(devID, ISO15765, CAN_ID_BOTH, CAN_BAUD, &chanID)) {
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

	if (j2534.PassThruStartMsgFilter(chanID, PASS_FILTER, &maskMSG, &maskPattern, NULL, &filterID)) {
		LOGE(TAG, "Failed to set message filter");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruIoctl(chanID, CLEAR_TX_BUFFER, NULL, NULL)) {
		LOGE(TAG, "Failed to clear j2534 TX buffer");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}

	if (j2534.PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL)) {
		LOGE(TAG, "Failed to clear j2534 RX buffer");
		reportJ2534Error(j2534);
		return STATUS_FAIL_PASSTHRU;
	}
	return STATUS_OK;
}

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	unsigned long status = 0;

	// buffers for commands
	char *vin            = NULL, 
			 *calibrationID  = NULL, 
			 *transferBuffer = NULL;
	uint8_t *seed=NULL, *key=NULL;

	time_t commandStart, commandEnd;
	char transferFilename[255] = { 0 };
	FILE* transferFile = NULL;
	ecudump_args_t args = {0};

	if(getCommandArgs(argc, argv, &args)) {
		fprintf(stderr, "Arguments are invalid.\n");
		printUsage(argc, argv, &args);
		return 1;
	}

	// transfer params
	uint16_t chunkSize = args.params.transfer.chunkSize;
	uint32_t address = args.params.transfer.startAddress;
	uint32_t transferSize = args.params.transfer.transferSize;
	uint32_t bytesTransfered = 0;
	uint32_t endAddress = address+transferSize;
	uint16_t chunkRemainder = 0;
	
	float numChunks = (float)transferSize / (float)chunkSize;

	chunkSize ? chunkRemainder = transferSize%chunkSize : chunkRemainder = 0;

	// write params
	FILE* sblFile = NULL;
	size_t sblLength = 0;

	FILE* romFile = NULL;
	size_t rom_length = 0;

	unsigned char* writePayload = NULL;

	ecudump_cmd_t command = args.command;
	/*
	FILE* payload_file = fopen("sbl.bin", "wb");
	fwrite(bootloader_bin, 1, bootloader_bin_len, payload_file);
	fflush(payload_file);
	fclose(payload_file);
	return 1;
	*/

	/*
	size_t new_payload_len = bootloader_bin_len + rom_bin_len;
	assert(new_payload_len == out_bin_len);
	unsigned char* new_payload = (unsigned char*)malloc(new_payload_len);
	assert(new_payload != NULL);
	memcpy(new_payload, bootloader_bin, bootloader_bin_len);
	memcpy(new_payload + bootloader_bin_len, rom_bin, rom_bin_len);
	FILE* payload_file = fopen("payload.bin", "wb");
	fwrite(new_payload, 1, new_payload_len, payload_file);
	fflush(payload_file);
	fclose(payload_file);
	fprintf(stderr, "wrote %0x08x bytes to payload.bin\n", new_payload_len);
	return 0;
	*/
	/*
	assert(new_payload_len == out_bin_len);
	for (int i = 0; i < bootloader_bin_len; i++) {
		assert(new_payload[i] == bootloader_bin[i]);
		assert(new_payload[i] == out_bin[i]);
	}
	for (int i = 0; i < rom_bin_len; i++) {
		//if(payload[i] != rom_bin[i])
		assert(new_payload[bootloader_bin_len + i] == rom_bin[i]);
	}
	fprintf(stderr, "assertions passed?\n");
	//return 0;
	*/



	if(args.verbose) {
		decomposeArgs(&args);
		LOGI(TAG, "info=\n"
							"\t  start=0x%08x\n"
							"\t  end=0x%08x\n"
							"\t  chunksize=0x%08x\n"
							"\t  transfersize=0x%08x\n"
			                "\t  numchunks=%f\n"
							"\t  remainder=0x%08x\n",
							address,
							endAddress,
							chunkSize,
							transferSize,
						    numChunks,
							chunkRemainder
		);
	}
	if (command == 0)
		return 1;
	if (args.dryRun)
		return 1;

	if (j2534Initialize()) {
		LOGE(TAG, "j2534Initialize() failed");
		// no need to cleanup, just return here
		return -STATUS_FAIL_PASSTHRU;
	}
	LOGI(TAG, "j2534 connection initialized ok");

	ecu = new RX8(j2534, devID, chanID);

	if(_GET_VIN(command)) {
		if (ecu->getVIN(&vin)) {
			LOGE(TAG, "failed to get VIN");
			status = -STATUS_FAIL_VINCHECK;
			goto cleanup;
		}
		LOGI(TAG, "Got VIN = %s", vin);
	}

	if(_GET_CALID(command)) {
		if (ecu->getCalibrationID(&calibrationID)) {
			LOGE(TAG, "failed to get Calibration ID");
			status = -STATUS_FAIL_CALID;
			goto cleanup;
		}
		LOGI(TAG, "Got calibration ID = %s", calibrationID);
	}

	if (!ecu->initDiagSession(MAZDA_SBF_SESSION_81)) {
		LOGE(TAG, "failed to init diag session 81");
		status = -STATUS_FAIL_DIAG;
		goto cleanup;
	}

	if (!ecu->initDiagSession(MAZDA_SBF_SESSION_85)) {
		LOGE(TAG, "failed to init diag session 85");
		status = -STATUS_FAIL_DIAG;
		goto cleanup;
	}
	LOGI(TAG, "Diag sesion initialized ok");
	
	if(_GET_SEED(command)) {
		if (ecu->getSeed(&seed)) {
			LOGE(TAG, "failed to get seed");
			status = -STATUS_FAIL_SEED;
			goto cleanup;
		}
		LOGI(TAG, "Got seed = { %02X, %02X, %02X }", seed[0], seed[1], seed[2]);
	}
	if(_CALCULATE_KEY(command)) {
		if (ecu->calculateKey(seed, &key)) {
			LOGE(TAG, "failed to calculate key");
			status = -STATUS_FAIL_KEY;
			goto cleanup;
		}
		LOGI(TAG, "Got key  = { %02X, %02X, %02X }", key[0], key[1], key[2]);
	}

	if(_UNLOCK(command)) {
		if (!ecu->unlock(key)) {
			LOGE(TAG, "failed to unlock ECU using key");
			status = -STATUS_FAIL_UNLOCK;
			goto cleanup;
		}
		LOGI(TAG, "Unlocked ECU");
	}

	time(&commandStart);
	if(_READ_MEM(command)) {
		// sanity check assertions just in case of CAN errors
		assert(strlen(vin) > 0);
		assert(strlen(calibrationID) > 0);
		assert(strlen(vin) + 1 + strlen(calibrationID) + 3 < 255);

		if(args.fileName[0] == NULL) {
			// example: JM1FE173370212600-N3M5EF00013H6020.bin
			strcpy(transferFilename, vin);
			strcpy(transferFilename + strlen(vin), "-");
			strcpy(transferFilename + strlen(vin) + 1, calibrationID);
			strcpy(transferFilename + strlen(vin) + 1 + strlen(calibrationID), ".bin");
		} else {
			strcpy(transferFilename, args.fileName);
		}

		LOGI(TAG, "Using %s for transfer", transferFilename);

		if (args.overwrite) {
			if (access(transferFilename, F_OK) == 0) {
				LOGE(TAG, "Removing old file %s", transferFilename);
				if(remove(args.fileName) != 0) {
					LOGE(TAG, "Could not remove old file %s", strerror(errno));
					status = -errno;
					goto cleanup;
				}
			}
		}
		else {
			if (access(transferFilename, F_OK) == 0) {
				LOGE(TAG, "Not overwriting old file %s (use --overwrite if you want this)", transferFilename);
				status = -STATUS_FAIL_DOWNLOAD;
				goto cleanup;
			}
		}

		transferFile = fopen(transferFilename, "ab+");
		if(!transferFile) {
			LOGE(TAG, "Failed to open %s %s", transferFilename, strerror(errno));
			transferFile = NULL;
			status = -errno;
			goto cleanup;
		}
		transferBuffer = (char*)malloc(transferSize);
		if(!transferBuffer) {
			status=-ENOMEM;
			goto cleanup;
		}
		memset(transferBuffer, 0, transferSize);

		LOGI(TAG, "Starting memory read 0x%08X-0x%08X into %s", 
					address, 
					endAddress,
					transferFilename
		);
		for(bytesTransfered=0; address < endAddress; address+=chunkSize, bytesTransfered+=chunkSize, transferBuffer+=chunkSize) {
			assert(endAddress > address);
			if (ecu->readMem(address, chunkSize, &transferBuffer))
				break;
			//if(address > chunkSize) return -1;
			//fwrite(transferBuffer, chunkSize, 1, transferFile);
			printProgress(bytesTransfered, transferSize);
		}

		if(chunkRemainder > 0) {
			if (ecu->readMem(address, chunkRemainder, &transferBuffer)) {
				LOGE(TAG, "Failed to read remainder of memory %04X", address);
				status = -STATUS_FAIL_DOWNLOAD;
				goto cleanup;
			}
			bytesTransfered += chunkRemainder;
			printProgress(bytesTransfered, transferSize);
		}

		if (bytesTransfered != transferSize) {
			LOGE(TAG, "Only transfered %08X / %08X bytes", bytesTransfered, transferSize);
		}
		bytesTransfered = fwrite(transferBuffer-transferSize, transferSize, 1, transferFile);

// #if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
// 		// fwrite on windows returns the write count, not the number of bytes written.
// 		if(bytesTransfered > 0)
// 			bytesTransfered *= transferSize;
// #endif

// 		if (bytesTransfered != transferSize) {
// 			LOGE(TAG, "Only wrote %08X / %08X bytes", bytesTransfered, transferSize);
// 		}

		fflush(transferFile);
		time(&commandEnd);
		LOGI(TAG, "Successfully read memory to %s Took %.0lf seconds", args.fileName, difftime(commandEnd,commandStart));
	}

	if(_WRITE_MEM(command)) {
		LOGE(TAG, "ROM Upload is not finished yet. be very careful and be sure to have a backup!");

		LOGI(TAG, "Reading SBL %s", args.params.write.SBLfileName);

		// validate the SBL
		sblFile = fopen(args.params.write.SBLfileName, "rb");
		if (!sblFile) {
			LOGE(TAG, "Failed to open SBL for reading: %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}
		if (fseek(sblFile, 0, SEEK_END)) {
			LOGE(TAG, "Failed to get length of SBL: %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}
		sblLength = ftell(sblFile);
		rewind(sblFile);
		if (sblLength != MAZDA_SBL_LENGTH) {
			LOGE(TAG, "Incorrect SBL length 0x%08x. Expecting 0x%08x", sblLength, MAZDA_SBL_LENGTH);
			status = -STATUS_FAIL_BOOTLOADER;
			goto cleanup;
		}

		// validate the ROM
		strcpy(transferFilename, args.fileName);
		LOGI(TAG, "Reading ROM %s", transferFilename);

		transferFile = fopen(transferFilename, "rb");
		if (!transferFile) {
			LOGE(TAG, "Failed to open %s for reading: %s", transferFilename, strerror(errno));
			transferFile = NULL;
			status = -errno;
			goto cleanup;
		}
		if (fseek(transferFile, 0, SEEK_END)) {
			LOGE(TAG, "Failed to get length of ROM: %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}
		rom_length = ftell(transferFile);
		rewind(transferFile);
		if (rom_length != MAZDA_ROM_LENGTH) {
			LOGE(TAG, "Incorrect ROM length 0x%08x. Expecting 0x%08x", rom_length, MAZDA_ROM_LENGTH);
			status = -STATUS_FAIL_DOWNLOAD;
			goto cleanup;
		}

		writePayload = (unsigned char*)malloc((sblLength + rom_length) - MAZDA_ROM_START_OFFSET);
		if (!writePayload) {
			LOGE(TAG, "Failed to create payload buffer");
			status = -ENOMEM;
			goto cleanup;
		}

		if (fread(writePayload, 1, sblLength, sblFile) != sblLength) {
			LOGE(TAG, "Failed to read SBL File into payload buffer %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}
		if (fseek(transferFile, MAZDA_ROM_START_OFFSET, SEEK_CUR)) {
			LOGE(TAG, "Failed to read ROM File into payload buffer %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}

		if (fread(writePayload + sblLength, 1, rom_length - MAZDA_ROM_START_OFFSET, transferFile) != rom_length - MAZDA_ROM_START_OFFSET) {
			LOGE(TAG, "Failed to read ROM File into payload buffer %s", strerror(errno));
			status = -errno;
			goto cleanup;
		}

		LOGI(TAG, "Starting ROM upload. Do NOT exit or cut power to the ECU!!!!!");
		
		if(ecu->requestBootloaderMode()) {
			LOGE(TAG, "Could not enter bootloader(?) mode");
			status = -STATUS_FAIL_BOOTLOADER;
			goto cleanup;
		}
		
		if(!ecu->requestDownload(chunkSize, sblLength + rom_length - MAZDA_ROM_START_OFFSET)) {
		 	LOGE(TAG, "Could not enter request download mode chunksize=%08x payload=%08x", chunkSize, sblLength + rom_length - MAZDA_ROM_START_OFFSET);
		 	status = -STATUS_FAIL_DOWNLOAD;
		 	goto cleanup;
		}

		address = 0;
		transferSize = sblLength + rom_length - MAZDA_ROM_START_OFFSET;
		for (bytesTransfered = 0; address < transferSize; address += chunkSize, writePayload += chunkSize) {
			status = ecu->transferData(chunkSize, writePayload);
			bytesTransfered += chunkSize;
			if (bytesTransfered <= sblLength) {
				printProgress(bytesTransfered, sblLength);
				if (status == 0x78) {
					resetProgress();
					LOGI(TAG, "kernel transfered");
					continue;
				}
			} else {
				printProgress(bytesTransfered, transferSize);
				if (status == 0x78) continue;
			}

			if (status != 0) {
				LOGE(TAG, "payload failed to send at 0x%08x status=%d", address, status);
				status = -STATUS_FAIL_DOWNLOAD;
				goto cleanup;
			}
		}
		status = 0;

		time(&commandEnd);
		
		LOGI(TAG, "Successfully uploaded payload. Took %.0lf seconds", difftime(commandEnd,commandStart));

		if(!ecu->requestTransferExit()) {
			LOGE(TAG, "Could not complete upload");
			status = -STATUS_FAIL_DOWNLOAD;
			goto cleanup;
		}

		if(!ecu->reset()) {
			LOGE(TAG, "Could not reset ECU");
			status = -STATUS_FAIL_RESET;
			goto cleanup;
		}

		status = 0;
	}
cleanup:
	if(transferFile) {
		fflush(transferFile);
		fclose(transferFile);
		transferFile = NULL;
	}
	if (sblFile) {
		fclose(sblFile);
		sblFile = NULL;
	}
// TODO: it seems like calling PassThruDisconnect or PassThruClose causes
//       error inside the J2534 dll. For now, skip it
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
