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

#include <stdio.h>
#include "J2534.h"

void reportJ2534Error(J2534 j2534);
void dump_msg(PASSTHRU_MSG* msg);
void hexdump_msg(PASSTHRU_MSG* msg);

#ifdef WIN32
// Windows console doesn't support colors
#define ANSI_COLOR_RED     ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_COLOR_RESET   ""
#else
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#endif

#define log_location stderr
#define LOGE(TAG, ...) do { fprintf(log_location, "[" ANSI_COLOR_RED); fprintf(log_location, TAG); fprintf(log_location, ANSI_COLOR_RESET "] "); fprintf(log_location, __VA_ARGS__); fprintf(log_location, "\r\n"); fflush(log_location); } while(0)
#define LOGI(TAG, ...) do { fprintf(log_location, "[" ANSI_COLOR_CYAN); fprintf(log_location, TAG); fprintf(log_location, ANSI_COLOR_RESET"] "); fprintf(log_location, __VA_ARGS__); fprintf(log_location, "\r\n"); fflush(log_location); } while(0)

#define DEBUG
#ifdef DEBUG
#define log_location stderr
#define debug(...) do { fprintf(log_location, __VA_ARGS__); fprintf(log_location, "\r\n"); fflush(log_location); } while(0)
#define error(...) do { debug(__VA_ARGS__); } while (0)
#else
#define debug(...)
#define error(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif
