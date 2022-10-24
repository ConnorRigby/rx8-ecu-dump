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

#include "J2534.h"
#define PM_DATA_LEN	4128

static const uint8_t UDS_REQUEST_CANID_MSB  = 0x07;
static const uint8_t UDS_REQUEST_CANID_LSB  = 0xE0;
static const uint8_t UDS_RESPONSE_CANID_MSB = UDS_REQUEST_CANID_MSB;
static const uint8_t UDS_RESPONSE_CANID_LSB = 0xE8;

// number of tx PASSTHRU_MSG structs to keep around
static const uint8_t TX_BUFFER_LEN = 5;
static const uint8_t TX_TIMEOUT = 100;

// number of rx PASSTHRU_MSG structs to keep around
static const size_t RX_BUFFER_LEN = 5;
static const size_t RX_TIMEOUT = 200;

static const uint8_t UDS_SID_SESSION                  = 0x10;
static const uint8_t UDS_SID_SESSION_ACK              = UDS_SID_SESSION + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_RESET                    = 0x11;
static const uint8_t UDS_SID_RESET_ACK                = UDS_SID_RESET + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_READ_MEMORY_BY_ADDRESS   = 0x23;
static const uint8_t UDS_SID_READ_MEMORY_BY_ADDRESS_ACK = UDS_SID_READ_MEMORY_BY_ADDRESS + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_SECURITY                 = 0x27;
static const uint8_t UDS_SID_SECURITY_ACK             = UDS_SID_SECURITY + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_COMMUNICATION_CONTROL    = 0x28;
static const uint8_t UDS_SID_REQUEST_DOWNLOAD         = 0x34;
static const uint8_t UDS_SID_REQUEST_DOWNLOAD_ACK     = UDS_SID_SECURITY + OBD2_ACK_OFFSET;;
static const uint8_t UDS_SID_TRANSFER_DATA            = 0x36;
static const uint8_t UDS_SID_TRANSFER_DATA_ACK        = UDS_SID_TRANSFER_DATA + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_REQUEST_TRANSFER_EXIT    = 0x37;
static const uint8_t UDS_SID_REQUEST_TRANSFER_EXIT_ACK = UDS_SID_REQUEST_TRANSFER_EXIT + OBD2_ACK_OFFSET;
static const uint8_t UDS_SID_TESTER_PRESENT           = 0x3E;
static const uint8_t UDS_SID_ACCESS_TIMING_PARAMETER  = 0x83;
static const uint8_t UDS_SID_SECURE_DATA_TRANSMISSION = 0x84;
static const uint8_t UDS_SID_CONTROL_DTC_SETTING      = 0x85;
static const uint8_t UDS_SID_RESPONSE_TO_EVENT        = 0x86;
static const uint8_t UDS_SID_LINK_CONTROL             = 0x87;

static const uint8_t UDS_NEGATIVE_RESPONSE            = OBD2_NEGATIVE_RESPONSE;

static const uint8_t UDS_NEGATIVE_RESPONSE_GENERAL_REJECT                              = 0x10;
static const uint8_t UDS_NEGATIVE_RESPONSE_SERVICE_NOT_SUPPORTED                       = 0x11;
static const uint8_t UDS_NEGATIVE_RESPONSE_SUBFUNCTION_NOT_SUPPORTED                   = 0x12;
static const uint8_t UDS_NEGATIVE_RESPONSE_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT  = 0x13;
static const uint8_t UDS_NEGATIVE_RESPONSE_RESPONSE_TOO_LONG                           = 0x15;
static const uint8_t UDS_NEGATIVE_RESPONSE_BUSY_REPEAT_REQUES                          = 0x21;
static const uint8_t UDS_NEGATIVE_RESPONSE_CONDITIONS_NOT_CORRECT                      = 0x22;
static const uint8_t UDS_NEGATIVE_RESPONSE_REQUEST_SEQUENCE_ERROR                      = 0x24;
static const uint8_t UDS_NEGATIVE_RESPONSE_REQUEST_OUT_OF_RANGE                        = 0x31;
static const uint8_t UDS_NEGATIVE_RESPONSE_INVALID_KEY                                 = 0x35;
static const uint8_t UDS_NEGATIVE_RESPONSE_EXCEDED_NUMBER_OF_ATTEMPTS                  = 0x36;
static const uint8_t UDS_NEGATIVE_RESPONSE_REQUIRED_TIME_DELAY_NOT_EXPIRED             = 0x37;
static const uint8_t UDS_NEGATIVE_RESPONSE_UPLOAD_NOT_ACCEPTED                         = 0x70;
static const uint8_t UDS_NEGATIVE_RESPONSE_TRANSFER_DATA_SUSPENDED                     = 0x71;
static const uint8_t UDS_NEGATIVE_RESPONSE_GENERAL_PROGRAMMING_FAILURE                 = 0x72;
static const uint8_t UDS_NEGATIVE_RESPONSE_WRONG_BLOCK_SEQUENCE_COUNTER                = 0x73;
static const uint8_t UDS_NEGATIVE_RESPONSE_REQUEST_RECEIVED_RESPONSE_PENDING           = 0x78;
static const uint8_t UDS_NEGATIVE_RESPONSE_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7E;
static const uint8_t UDS_NEGATIVE_RESPONSE_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION     = 0x7F;

static const size_t UDS_ERROR_START = 0x100;
enum uds_error {
	UDS_ERROR_OK                = 0,
	UDS_ERROR_UNKNOWN           = UDS_ERROR_START+1,
	UDS_ERROR_NEGATIVE_RESPONSE = UDS_ERROR_START+2,
};

typedef struct UDS_Request {
	J2534* j2534;
	unsigned long devID;
	unsigned long chanID;
	unsigned long numTx;
	unsigned long numRx;
	PASSTHRU_MSG* txBuffer;
	PASSTHRU_MSG* rxBuffer;

	/* Service ID */
	uint16_t      sid;

	/* Payload Length */
	uint32_t      length;
	uint8_t*      payload;
} uds_request_t;

size_t uds_request_init(uds_request_t** request_out, 
                        J2534* j2534, 
                        unsigned long devID, 
                        unsigned long chanID);
size_t uds_request_prepare(uds_request_t* request);
size_t uds_request_send(uds_request_t* request);

const char* uds_request_error_string(uds_request_t* request, size_t ret);
const char* uds_request_negative_response_error_string(uds_request_t* request);