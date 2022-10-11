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

static const uint8_t OBD2_ACK_OFFSET                          = 0x40;
static const uint8_t OBD2_NEGATIVE_RESPONSE                   = 0x7f;
static const uint8_t OBD2_SID_REQUEST_VEHICLE_INFORMATION     = 0x09;
static const uint8_t OBD2_SID_REQUEST_VEHICLE_INFORMATION_ACK = OBD2_SID_REQUEST_VEHICLE_INFORMATION + OBD2_ACK_OFFSET;
static const uint8_t OBD2_PID_REQUEST_VIN                     = 0x02;
static const uint8_t OBD2_PID_REQUEST_CALID                   = 0x04;
