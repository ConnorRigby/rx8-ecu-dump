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

#include "payload.h"

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

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	ecudump_cmd_t command = 0;
	unsigned long status = 0;
	int opt;

	// user input for optargs
	char *inputAddress     = NULL,
			 *inputAddressSize = NULL,
			 *inputValue       = NULL,
			 *inputROMDownload = NULL,
			 *inputROMUpload   = NULL;

	// buffers for commands
	char *vin           = NULL, 
			 *calibrationID = NULL, 
			 *dump          = NULL;
	uint8_t *seed=NULL, *key=NULL;

	// state for command progress
	time_t commandStart, commandEnd;
	uint32_t address = 0;
	//uint16_t chunkSize = 0x0280;
	//uint32_t dumpSize = 0x80000;
	uint16_t chunkSize = 0x00ff;
	uint32_t dumpSize = 0xfffe;
	uint16_t remainder = 128;

	FILE* romFile = NULL;
	char fileName[255] = {0};
	char* buffer = NULL;
	size_t shouldReset = false;

	while ((opt = getopt(argc, argv, "rc:a:s:v:i:o::")) != -1) {
		switch (opt) {
			case 'r': {
				shouldReset = true;
				break;
			}
			case 'c': {
				assert(optarg != NULL);
				if(strcmp(optarg, "vin") == 0) {
					command = ECUDUMP_GET_VIN;
					break;
				}
				if(strcmp(optarg, "calid") == 0) {
					command = ECUDUMP_GET_CALID;
					break;
				}
				if(strcmp(optarg, "seed") == 0) {
					command = ECUDUMP_SEED;
					break;
				}
				if(strcmp(optarg, "key") == 0) {
					command = ECUDUMP_CALCULATE_KEY;
					break;
				}
				if(strcmp(optarg, "unlock") == 0) {
					command = ECUDUMP_UNLOCK;
					break;
				}
				if(strcmp(optarg, "read") == 0) {
					command = ECUDUMP_READ_MEMORY;
					break;
				}
				if(strcmp(optarg, "write") == 0) {
					command = ECUDUMP_WRITE_MEMORY;
					break;
				}
				if(strcmp(optarg, "download") == 0) {
					command = ECUDUMP_DOWNLOAD_ROM;
					break;
				}
				if(strcmp(optarg, "upload") == 0) {
					command = ECUDUMP_UPLOAD_ROM;
					break;
				}

				if(command == 0)
					fprintf(stderr, "Unknown command %s\n", optarg);

				break;
			}
			case 'a': {
				assert(optarg != NULL);
				inputAddress = optarg;
				break;
			}
			case 'v': {
				assert(optarg != NULL);
				inputValue = optarg;
				break;
			}
			case 'i': {
				assert(optarg != NULL);
				inputROMUpload = optarg;
				break;
			}
			case 'o': {
				assert(optarg != NULL);
				inputROMDownload = optarg;
				break;
			}
			case 's': {
				assert(optarg != NULL);
				inputAddressSize = optarg;
				break;
			}
		default:
				fprintf(stderr, "Usage: %s [-c] command\n", argv[0]);
				return 1;
		}
	}

	if(command == 0) {
		fprintf(stderr, "Usage: %s [-c] command\n", argv[0]);
		return 1;
	}

	if(command == ECUDUMP_READ_MEMORY) {
		if(inputAddress == NULL) {
			fprintf(stderr, "read requires -a [address]\n");
			return 1;
		}
		if(inputAddressSize == NULL) {
			fprintf(stderr, "read requires -s [size]\n");
			return 1;
		}

		LOGE(TAG, "read not implemented");
		return 1;
	}

	if(command == ECUDUMP_WRITE_MEMORY) {
		if((inputAddress == NULL) || (inputValue == NULL)) {
			fprintf(stderr, "write requires -a [address]\n");
			return 1;
		}

		if((inputValue == NULL) || (inputValue == NULL)) {
			fprintf(stderr, "write requires -v [value]\n");
			return 1;
		}

		LOGE(TAG, "write not implemented");
		return 1;
	}

	if (j2534Initialize()) {
		LOGE(TAG, "j2534Initialize() failed");
		// no need to cleanup, just return here
		return -STATUS_FAIL_PASSTHRU;
	}
	LOGI(TAG, "j2534 connection initialized ok");
	ecu = new RX8(j2534, devID, chanID);
	if(shouldReset) {
		LOGI(TAG, "resetting ECU");
		if(!ecu->reset()) {
		   LOGI(TAG, "ECU reset failed");
		}
	}
	// if(ecu->sendPayload(out_bin, out_bin_len)) {
	// 	LOGE(TAG, "payload failed to send");
	// 	goto cleanup;
	// }

// if(ecu->sendThatWeirdPayload()) {
// 			LOGE(TAG, "Could not enter bootloader mode");

// 			goto cleanup;

// }
// 		if(ecu->requestDownload(out_bin_len)) {
// 			LOGE(TAG, "Could not enter request download mode");
// 			goto cleanup;
// 		}

		// if(ecu->requestDownload(out_bin_len)) {
		// 	LOGE(TAG, "Could not enter request download mode");
		// 	goto cleanup;
		// }

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

	// if(ecu->sendThatWeirdPayload()) {
	// 	LOGE(TAG, "Could not enter bootloader(?) mode");
	// 	status = -69;
	// 	goto cleanup;
	// }

	// if(!ecu->requestDownload(out_bin_len)) {
	// 	LOGE(TAG, "Could not enter request download mode");
	// 	status = -69;
	// 	goto cleanup;
	// }
		// if(!ecu->requestUpload(out_bin_len)) {
		// 	LOGE(TAG, "Could not enter request download mode");
		// 	status = -69;
		// 	goto cleanup;
		// }
/*
	address = 0x6000;
	LOGE(TAG, "ugh %032X", address);
	for(address = 0x6000; address < 0xffff; address += 0xff) {

		if (ecu->readMem(address,0xff, &buffer)) {
		LOGE(TAG, "Failed to read memory 0x%016X", address);
		// goto cleanup;
		}
	}
	return 1;
*/
	time(&commandStart);
	if(_DOWNLOAD_ROM(command)) {
		LOGI(TAG, "Starting ROM dump, this will take a moment..");
		// sanity check assertions just in case of CAN errors
		assert(strlen(vin) > 0);
		assert(strlen(calibrationID) > 0);
		assert(strlen(vin) + 1 + strlen(calibrationID) + 3 < 255);

		// default rom name
		if(inputROMDownload == NULL) {
			// example: JM1FE173370212600-N3M5EF00013H6020.bin
			strcpy(fileName, vin);
			strcpy(fileName + strlen(vin), "-");
			strcpy(fileName + strlen(vin) + 1, calibrationID);
			strcpy(fileName + strlen(vin) + 1 + strlen(calibrationID), ".bin");
		} else {
			strcpy(fileName, inputROMDownload);
		}

		if (access(fileName, F_OK) == 0) {
			LOGE(TAG, "Removing old ROM %s", fileName);
			if(remove(fileName) != 0) {
				LOGE(TAG, "Could not delete old file %s", strerror(errno));
				status = -errno;
				goto cleanup;
			}
		}

		romFile = fopen(fileName, "ab+");
		if(!romFile) {
			LOGE(TAG, "Failed to open dump file %s", strerror(errno));
			romFile = NULL;
			status = -errno;
			goto cleanup;
		}

		for(address = 0x6000; address < dumpSize; address+=chunkSize) {
			if (ecu->readMem(address, chunkSize, &dump))
				break;
			fwrite(dump, chunkSize, 1, romFile);
			printProgress(address, dumpSize);
		}

		if (ecu->readMem(address, remainder, &dump)) {
			LOGE(TAG, "Failed to dump remainder of ROM %2X", address);
			goto cleanup;
		}
		fwrite(dump, remainder, 1, romFile);
		fflush(romFile);
		printProgress(dumpSize, dumpSize);

		time(&commandEnd);
		LOGI(TAG, "Successfully dumped ROM to %s Took %.0lf seconds", fileName, difftime(commandEnd,commandStart));
	}

	if(_UPLOAD_ROM(command)) {
		LOGE(TAG, "ROM Upload not implemented yet");

		if(ecu->sendThatWeirdPayload()) {
			LOGE(TAG, "Could not enter bootloader(?) mode");
			status = -69;
			goto cleanup;
		}

		if(!ecu->requestDownload(out_bin_len)) {
		 	LOGE(TAG, "Could not enter request download mode");
		 	status = -69;
		 	goto cleanup;
		 }

		if(ecu->sendPayload(out_bin, out_bin_len)) {
			LOGE(TAG, "payload failed to send");
			status = -69;
//			goto cleanup;
		}

		time(&commandEnd);
		LOGI(TAG, "Successfully uploaded payload. Took %.0lf seconds", difftime(commandEnd,commandStart));

		if(!ecu->requestTransferExit()) {
			LOGE(TAG, "Could not complete upload");
			status = -69;
//			goto cleanup;
		}

		if(!ecu->reset()) {
			LOGE(TAG, "Could not reset ECU");
			status = -69;
			goto cleanup;
		}

		status = 0;
	}
cleanup:
	if(romFile) {
		fflush(romFile);
		fclose(romFile);
		romFile = NULL;
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
