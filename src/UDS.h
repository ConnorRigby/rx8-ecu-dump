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
#include "OBD2.h"

static const uint8_t UDS_SID_SESSION                  = 0x10;
static const uint8_t UDS_SID_SESSION_ACK              = UDS_SID_SESSION + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_RESET                    = 0x11;
static const uint8_t UDS_SID_READ_MEMORY_BY_ADDRESS   = 0x23;
static const uint8_t UDS_SID_SECURITY                 = 0x27;
static const uint8_t UDS_SID_SECURITY_ACK             = UDS_SID_SECURITY + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_COMMUNICATION_CONTROL    = 0x28;
static const uint8_t UDS_SID_REQUEST_DOWNLOAD         = 0x34;
static const uint8_t UDS_SID_REQUEST_DOWNLOAD_ACK     = UDS_SID_SECURITY + OBD2_ACK_OFFSET;;
static const uint8_t UDS_SID_TRANSFER_DATA            = 0x36;
static const uint8_t UDS_SID_TRANSFER_DATA_ACK        = UDS_SID_TRANSFER_DATA + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_TESTER_PRESENT           = 0x3E;
static const uint8_t UDS_SID_ACCESS_TIMING_PARAMETER  = 0x83;
static const uint8_t UDS_SID_SECURE_DATA_TRANSMISSION = 0x84;
static const uint8_t UDS_SID_CONTROL_DTC_SETTING      = 0x85;
static const uint8_t UDS_SID_RESPONSE_TO_EVENT        = 0x86;
static const uint8_t UDS_SID_LINK_CONTROL             = 0x87;

static const uint8_t UDS_NEGATIVE_RESPONSE            = OBD2_NEGATIVE_RESPONSE;
