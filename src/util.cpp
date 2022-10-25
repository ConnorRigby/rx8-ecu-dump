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

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

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

	printf("[%lu]", msg->Timestamp);
	for (unsigned int i = 0; i < msg->DataSize; i++)
		printf("%02X ", msg->Data[i]);
	printf("\n");
}

void hexdump_msg(PASSTHRU_MSG* msg) {
    hexdump(msg->Data, msg->DataSize);
}

// credit: https://stackoverflow.com/questions/29242/off-the-shelf-c-hex-dump-code
void hexdump(void *ptr, size_t buflen) {
    unsigned char* buf = (unsigned char*)ptr;
    size_t i, j;
    for (i = 0; i < buflen; i += 16) {
        printf("%06lx: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}

// credit: https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
void sleep_ms(int milliseconds) 
{ // cross-platform sleep function
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
        sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}