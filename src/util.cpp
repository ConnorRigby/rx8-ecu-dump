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

#include <stdio.h>
#include <ctype.h>

#include "J2534.h"
#include "util.h"

void reportJ2534Error(J2534 j2534)
{
	char err[512];
	j2534.PassThruGetLastError(err);
	printf("[J2534] %s\n", err);
}

void dump_msg(PASSTHRU_MSG* msg)
{
	if (msg->RxStatus & START_OF_MESSAGE)
		return;

	printf("[%lu] [%0b]", msg->Timestamp, msg->RxStatus);
	for (unsigned int i = 0; i < msg->DataSize; i++)
		printf("%02X ", msg->Data[i]);
	printf("\n");
}

// credit: https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
void hexdump_msg(PASSTHRU_MSG* msg) {
    unsigned char* buf = (unsigned char*)msg->Data;
    size_t i, j;
    for (i = 0; i < msg->DataSize; i += 16) {
        printf("%06lx: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < msg->DataSize)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < msg->DataSize)
                printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}
