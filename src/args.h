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
#include <stdbool.h>

/* Below commands are encoded as a bitfield. This allows some commands to
 * easily rely on each other. For example, to unlock the ECU, one needs
 * to read the seed and calculate the key first.
 * And to read or write to memory, one must unlock the ECU first.
 * See the below macros for more details.
 */

static const uint16_t ECUDUMP_GET_VIN       = 0b1000000000000000;
static const uint16_t ECUDUMP_GET_CALID     = 0b0100000000000000;
static const uint16_t ECUDUMP_SEED          = 0b0010000000000000;
static const uint16_t ECUDUMP_CALCULATE_KEY = 0b0011000000000000;
static const uint16_t ECUDUMP_UNLOCK        = 0b0011100000000000;
static const uint16_t ECUDUMP_READ_MEM      = 0b1111110000000000;
static const uint16_t ECUDUMP_WRITE_MEM     = 0b1111101000000000;

#define _GET_VIN(COMMAND)       ((COMMAND >> 15) & 1)
#define _GET_CALID(COMMAND)     ((COMMAND >> 14) & 1)
#define _GET_SEED(COMMAND)      ((COMMAND >> 13) & 1)
#define _CALCULATE_KEY(COMMAND) ((COMMAND >> 12) & 1)
#define _UNLOCK(COMMAND)        ((COMMAND >> 11) & 1)
#define _READ_MEM(COMMAND)    ((COMMAND >> 10) & 1)
#define _WRITE_MEM(COMMAND)  ((COMMAND >>  9) & 1)

typedef uint16_t ecudump_cmd_t;

/**
 * @brief params for a transfer. 
 * Optional, but helpful for tuning
 * read/write speeds or if reading
 * from ram is desired
 * 
 */
typedef struct transfer_params {
	uint32_t startAddress;
	uint16_t chunkSize;
	uint32_t transferSize;
} transfer_params_t;

typedef union ecudump_params {
	transfer_params_t transfer;
} ecudump_params_t; 

typedef struct ecudump_args {
	ecudump_cmd_t command;
	char fileName[255];
	bool verbose;
	bool overwrite;
	ecudump_params_t params;
} ecudump_args_t;

void printUsage(int argc, char** argv, ecudump_args_t* args);
void decomposeArgs(ecudump_args_t* args);
size_t getCommandArgs(int argc, char** argv, ecudump_args_t* args);
