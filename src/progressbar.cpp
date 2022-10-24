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

/*
The Inspired by https://github.com/rpm-software-management/rpm/blob/a7c3886b356c2f3e3e640069623d44d62beabb33/lib/rpminstall.c#L30-L77
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

static size_t charsCurrent = 0;
static size_t charsTotal = 0;
static size_t progressCurrent = 0;
static size_t progressTotal = 0;

void resetProgress()
{
    charsCurrent = 0;
    charsTotal = 0;
    progressCurrent = 0;
    progressTotal = 0;
    fprintf(stdout, "\r");
}

void printProgress(const size_t amount, const size_t total)
{
  size_t charsNeeded;
  charsTotal = (isatty(fileno(stdout)) ? 34 : 40);

  if (charsCurrent != charsTotal) {
    float pct = (total ? (((float) amount) / total) : 1.0);
    charsNeeded = (charsTotal * pct) + 0.5;
    while (charsNeeded > charsCurrent) {
      if (isatty (fileno(stdout))) {
        size_t i;
        for (i = 0; i < charsCurrent; i++) 
          putchar ('#');
        for (; i < charsTotal; i++) 
          putchar (' ');

        fprintf(stdout, "(%3d%%)", (int)((100 * pct) + 0.5));

        for (i = 0; i < (charsTotal + 6); i++) 
          putchar ('\b');
      } else {
        fprintf(stdout, "#");
      }
      charsCurrent++;
    }
    fflush(stdout);

    if (charsCurrent == charsTotal) {
      size_t i;
      progressCurrent++;
      if (isatty(fileno(stdout))) {
        for (i = 1; i < charsCurrent; i++) putchar ('#');
        pct = (progressTotal ? (((float) progressCurrent) / progressTotal): 1);
        fprintf(stdout, " [%3d%%]", (int)((100 * pct) + 0.5));
      }
      fprintf(stdout, "\n");
    }
    fflush(stdout);
  }
}
