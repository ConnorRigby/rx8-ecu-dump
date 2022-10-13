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

static const uint16_t ECUDUMP_GET_VIN       = 0b100000000;
static const uint16_t ECUDUMP_GET_CALID     = 0b010000000;
static const uint16_t ECUDUMP_SEED          = 0b101000000;
static const uint16_t ECUDUMP_CALCULATE_KEY = 0b101100000;
static const uint16_t ECUDUMP_UNLOCK        = 0b101110000;
static const uint16_t ECUDUMP_READ_MEMORY   = 0b101111000;
static const uint16_t ECUDUMP_WRITE_MEMORY  = 0b101110100;
static const uint16_t ECUDUMP_DOWNLOAD_ROM  = 0b111110010;
static const uint16_t ECUDUMP_UPLOAD_ROM    = 0b111110001;

#define _GET_VIN(COMMAND)       ((COMMAND >> 8) & 1)
#define _GET_CALID(COMMAND)     ((COMMAND >> 7) & 1)
#define _GET_SEED(COMMAND)      ((COMMAND >> 6) & 1)
#define _CALCULATE_KEY(COMMAND) ((COMMAND >> 5) & 1)
#define _UNLOCK(COMMAND)        ((COMMAND >> 4) & 1)
#define _READMEM(COMMAND)       ((COMMAND >> 3) & 1)
#define _WRITEMEM(COMMAND)      ((COMMAND >> 2) & 1)
#define _DOWNLOAD_ROM(COMMAND)  ((COMMAND >> 1) & 1)
#define _UPLOAD_ROM(COMMAND)    ((COMMAND >> 0) & 1)

typedef uint16_t ecudump_cmd_t;