#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#include "../lib/getopt/getopt.h"
#else
#include <unistd.h>
#include <getopt.h>
#endif

#include "args.h"

void decomposeArgs(ecudump_args_t* args)
{
  assert(args != NULL);
  fprintf(stderr,
    "ARGS="
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    "\tcommand=%08x\n"
#else
    "\tcommand=%016b\n"
#endif
    "\tfileName=%s\n"
    "\tVIN  =%d\n"
    "\tCALID=%d\n"
    "\tSEED =%d\n"
    "\tKEY  =%d\n"
    "\tULOCK=%d\n"
    "\tDL   =%d\n"
    "\tUL   =%d\n"
    "\rPARAMS=\n"
    "\ttransfer.startAddress = 0x%08X\n"
    "\ttransfer.transferSize = 0x%08X\n"
    "\ttransfer.chunkSize    = 0x%04X\n"
    "\rWRITEMEM=\n"
    "\twritemem.SBLfileName = %s\n"
    ,
    args->command,
    args->fileName[0] ? args->fileName : "NULL",
    _GET_VIN(args->command),
    _GET_CALID(args->command),
    _GET_SEED(args->command),
    _CALCULATE_KEY(args->command),
    _UNLOCK(args->command),
    _READ_MEM(args->command),
    _WRITE_MEM(args->command),
    args->params.transfer.startAddress,
    args->params.transfer.transferSize,
    args->params.transfer.chunkSize,
    args->params.write.SBLfileName
  );
}

signed long long decodeHex(const char* param, size_t max)
{
  uint8_t base = 10;
  char* extra = NULL;
  long long out = 0;
  const char* substr = strstr(param, "0x");
  if(substr) {
    param = substr;
    base=16;
  }

  out = strtoll(param, &extra, base);

  if(strlen(extra) > 0) 
    return -(strlen(extra));

  if(out > max)
    return (out < 0) ? out : -out;

  return out;
}

void printUsage(int argc, char** argv, ecudump_args_t* args)
{
    assert(argv[0]);
    fprintf(stderr, "Usage: %s [-vcskhf] [-d [filename] | -u [filename]]\n", argv[0]);

}

size_t getCommandArgs(int argc, char** argv, ecudump_args_t* args)
{
  assert(args != NULL);
  memset(args, 0, sizeof(args));
  ecudump_cmd_t command = 0;
  int c;
  for(;;) {
    int option_index = 0;
    static struct option long_options[] = 
    {
      {"download", optional_argument, NULL,  'd'},
      {"upload",   required_argument, NULL,  'u'},
      {"vin",      no_argument,       NULL,  0  },
      {"calid",    no_argument,       NULL,  'c'},
      {"seed",     no_argument,       NULL,  's'},
      {"unlock",   no_argument,       NULL,  'n'},
      {"key",      no_argument,       NULL,  'k'},

      // transfer options
      {"start-address", required_argument, NULL, 0},
      {"transfer-size", required_argument, NULL, 0},
      {"chunk-size",    required_argument, NULL, 0},
      {"overwrite",     no_argument,       NULL, 'f'},

      // write mem options
      {"sbl", required_argument, NULL, 0},

      // meta
      {"help",     no_argument,       NULL,  'h'},
      {"verbose",  no_argument,       NULL,  'v'},
      {"version",  no_argument,       NULL,   0 },
      {"dry-run",  no_argument,       NULL,   0 },
      {NULL,       0,                 NULL,   0 }
    };

    c = getopt_long(argc, argv, "-:d::u::vcskhf", long_options, &option_index);
    if (c == -1) break;
    switch(c) {
      case 0: {
        if (strcmp(long_options[option_index].name, "version") == 0) {
            fprintf(stderr, "%s Version 0.9.0\n", argv[0] ? argv[0] : "ecudump");
            return 0;
        }
        
        if(strcmp(long_options[option_index].name, "start-address") == 0) {
          signed long long ret = decodeHex(optarg, 0xffffffff);
          if(ret < 0) {
            fprintf(stderr, "could not decode %s=%s (%lld)\n", long_options[option_index].name, optarg, ret);
            return ret;
          }
          args->params.transfer.startAddress = ret;
          break;
        }

        if(strcmp(long_options[option_index].name, "chunk-size") == 0) {
          signed long long ret = decodeHex(optarg, 0xffff);
          if(ret < 0) {
            fprintf(stderr, "could not decode %s=%s (%lld)\n", long_options[option_index].name, optarg, ret);
            return ret;
          }
          args->params.transfer.chunkSize = ret;
          break;
        }

        if(strcmp(long_options[option_index].name, "transfer-size") == 0) {
          signed long long ret = decodeHex(optarg, 0xffffffff);
          if(ret < 0) {
            fprintf(stderr, "could not decode %s=%s (%lld)\n", long_options[option_index].name, optarg, ret);
            return ret;
          }
          args->params.transfer.transferSize = ret;
          break;
        }

        if (strcmp(long_options[option_index].name, "sbl") == 0) {
            if (optarg)
                strcpy(args->params.write.SBLfileName, optarg);
            break;
        }

        if (strcmp(long_options[option_index].name, "overwrite") == 0) {
          args->overwrite = true;
          break;
        }

        if (strcmp(long_options[option_index].name, "dry-run") == 0) {
            args->dryRun = true;
            break;
        }

        if(command) {fprintf(stderr, "only one command may be provided\n"); command=0; break;}
        if(strcmp(long_options[option_index].name, "vin") == 0) {
          command = ECUDUMP_GET_VIN;
          break;
        }
        if(strcmp(long_options[option_index].name, "calid") == 0) {
          command = ECUDUMP_GET_CALID;
          break;
        }
        if(strcmp(long_options[option_index].name, "seed") == 0) {
          command = ECUDUMP_SEED;
          break;
        }
        if(strcmp(long_options[option_index].name, "key") == 0) {
          command = ECUDUMP_CALCULATE_KEY;
          break;
        }
        if(strcmp(long_options[option_index].name, "unlock") == 0) {
          command = ECUDUMP_UNLOCK;
          break;
        }
        if(strcmp(long_options[option_index].name, "download") == 0) {
          command = ECUDUMP_READ_MEM;
          if (optarg)
            strcpy(args->fileName, optarg);
          break;
        }
        if(strcmp(long_options[option_index].name, "upload") == 0) {
          command = ECUDUMP_WRITE_MEM;
          if (optarg)
            strcpy(args->fileName, optarg);
          break;
        }
        if(strcmp(long_options[option_index].name, "verbose") == 0) {
          args->verbose = true;
          break; 
        }

        if(strcmp(long_options[option_index].name, "version") == 0) {
          command = 0;
          break;
        }
      }
      case 'd': {
        if(command == 0) {
          command = ECUDUMP_READ_MEM;
          if (optarg)
              strcpy(args->fileName, optarg);
          break;
        }
        command = 0;
        break;
      }
      case 'u': {
        if(command == 0) {
          command = ECUDUMP_WRITE_MEM;
          if (optarg)
              strcpy(args->fileName, optarg);
          break;
        }
        fprintf(stderr, "command must be [u]pload OR [d]ownload\n");
        command = 0;
        break;
      }
      case 'c':
        if(command) {fprintf(stderr, "CALID command is exclusive\n"); command = 0; break;}
        command = ECUDUMP_GET_CALID;
        break;
      case 's':
        if(command) {fprintf(stderr, "SEED command is exclusive\n"); command = 0; break;}
        command = ECUDUMP_SEED;
        break;
      case 'k':
        if(command) {fprintf(stderr, "KEY command is exclusive\n"); command = 0; break;}
        command = ECUDUMP_CALCULATE_KEY;
        break;
      case 'n':
        if(command) {fprintf(stderr, "UNLOCK command is exclusive\n"); command = 0; break;}
        command = ECUDUMP_UNLOCK;
        break;
      case 'f':
        args->overwrite = true;
        break;
      case 'v':
        args->verbose = true;
        break;
      case 'h': 
        command = 0;
        break;
      case '?':
      case 1:
      default: 
        command = 0;
        break;
    }
	}
  args->command = command;
  if (_READ_MEM(command)) {
      if (args->params.transfer.startAddress == 0) {
          if (args->verbose) fprintf(stderr, "[readmem] using default start address 0x%08x\n", 0x0);
          args->params.transfer.startAddress = 0;
      }
      if (args->params.transfer.chunkSize == 0) {
          if (args->verbose) fprintf(stderr, "[readmem] using default chunk size 0x%08x\n", 0x100);
          args->params.transfer.chunkSize = 0x100;
      }
      if (args->params.transfer.transferSize == 0) {
          if (args->verbose) fprintf(stderr, "[readmem] using default transfer size 0x%08x\n", 0x80000);
          args->params.transfer.transferSize = 0x80000;
      }
  }
  else if (_WRITE_MEM(command)) {
      if (args->params.transfer.startAddress == 0) {
          if (args->verbose) fprintf(stderr, "[writemem] using default start address 0x%08x\n", 0x400000);
          args->params.transfer.startAddress = 0x400000;
      }
      if (args->params.transfer.chunkSize == 0) {
          if (args->verbose) fprintf(stderr, "[writemem] using default chunk size 0x%08x\n", 0x400);
          args->params.transfer.chunkSize = 0x400;
      }
      if (args->params.transfer.transferSize == 0) {
          if (args->verbose) fprintf(stderr, "[writemem] using default transfer size 0x%08x\n", 0x80000);
          args->params.transfer.transferSize = 0x80000;
      }
      if (strlen(args->params.write.SBLfileName) == 0) {
          fprintf(stderr, "[writemem] SBL file is required to upload rom\n");
          return 1;
      }
  }

  if (args->params.transfer.chunkSize > args->params.transfer.transferSize) {
      fprintf(stderr, "[transfer] Chunk size cannot be larger than transfer size\n");
      return 1;
  }

  return command == 0;
}