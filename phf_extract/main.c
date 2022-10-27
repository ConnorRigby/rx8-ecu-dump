#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HeaderEntry {
  unsigned char* key;
  unsigned char* value;
} header_entry_t;

typedef struct Header {
  header_entry_t* entries;
  int numEntries;
} header_t;

#define KV_MAX_LEN 100

int main(int argc, char const *argv[])
{
  FILE* phfFile = NULL;
  phfFile = fopen("/home/connor/workspace/connorrigby/rx8-ecu-dump/lib/SW-N3Z2EU000.PHF", "rb");
  fseek(phfFile, 0, SEEK_END);
  int bufferSize = ftell(phfFile);
  assert(bufferSize);
  rewind(phfFile);
  assert(phfFile);
  unsigned char* buffer = (unsigned char*)malloc(bufferSize);
  assert(buffer);

  memset(buffer, 0, bufferSize);

  int bytesRead = 0;
  do {
    fread(buffer+bytesRead, 1, 1, phfFile);
    bytesRead+=1;
  } while ((buffer[bytesRead-1] != '$') && bytesRead < bufferSize);
  assert(bytesRead < bufferSize);

  int index = 0;
  int numEntries = 0;
  do {
    if(buffer[index] == '>')
      numEntries += 1;
    index++;
  } while (index <= bytesRead);

  index = 0;
  int entriesProcessed = 0;
  int kvState = 0;
  int kvIndex = 0;
  int tmpBufferIndex = 0;
  header_t header;
  memset(&header, 0, sizeof(header_t));
  header.numEntries = numEntries;
  header.entries = malloc(sizeof(header_entry_t) * numEntries);
  
  unsigned char* tmpBuffer = (unsigned char*) malloc(sizeof(unsigned char) * KV_MAX_LEN);
  memset(tmpBuffer, 0, KV_MAX_LEN);
  fprintf(stderr, "processing header\n");
  int strStart = 0;
  do {
    assert(tmpBufferIndex < KV_MAX_LEN);
    if(buffer[index] == '>') {
        header.entries[kvIndex].key = malloc(sizeof(unsigned char) * KV_MAX_LEN);
        fprintf(stderr, "processing key %d '%s'\n", tmpBufferIndex, tmpBuffer);
        strStart = 0;
        for(int i = 0; i < KV_MAX_LEN; i++) {
          if(i == tmpBufferIndex)
            break;
          if(tmpBuffer[i] == '\0' || tmpBuffer[i] == ' ')
            strStart +=1;
          else break;
        }
        memset(header.entries[kvIndex].key, 0, KV_MAX_LEN);
        memcpy(header.entries[kvIndex].key, tmpBuffer+strStart, KV_MAX_LEN-strStart);
        for(int i = 0; i < KV_MAX_LEN; i++) {
          if(header.entries[kvIndex].key[i] == ' ' && header.entries[kvIndex].key[i+1] == ' ') {
            header.entries[kvIndex].key[i] = '\0';
            break;
          }
        }
        fprintf(stderr, "key=%s %d\r\n", header.entries[kvIndex].key, strStart);
        kvState = 1;
        tmpBufferIndex=0;
        memset(tmpBuffer, 0, KV_MAX_LEN);
    }
    if(buffer[index] == '\0') {
      fprintf(stderr, "processing value \r\n");
      header.entries[kvIndex].value = malloc(sizeof(unsigned char) * KV_MAX_LEN);
        strStart = 0;
        for(int i = 0; i < KV_MAX_LEN; i++) {
          if(i == tmpBufferIndex)
            break;
          if(tmpBuffer[i] == '\0' || tmpBuffer[i] == ' ')
            strStart +=1;
          else break;
        }
      memset(header.entries[kvIndex].value, 0, KV_MAX_LEN);
      memcpy(header.entries[kvIndex].value, tmpBuffer+1, KV_MAX_LEN-1);
        for(int i = 0; i < KV_MAX_LEN; i++) {
          if(header.entries[kvIndex].key[i] == ' ' && header.entries[kvIndex].key[i+1] == ' ') {
            header.entries[kvIndex].key[i] = '\0';
            break;
          }
        }
      fprintf(stderr, "value=%s \r\n", header.entries[kvIndex].value);
      kvState = 0;
      kvIndex += 1;
      tmpBufferIndex=0;
      memset(tmpBuffer, 0, KV_MAX_LEN);
      entriesProcessed+=1;
    }

    tmpBuffer[tmpBufferIndex] = buffer[index];
    fprintf(stderr, "processing byte %d '%c' '%c'\r\n", tmpBufferIndex, buffer[index], tmpBuffer[tmpBufferIndex]);
    tmpBufferIndex += 1;
    index += 1;
  } while (entriesProcessed < numEntries);

  fprintf(stderr, "read header buffer size=%d number of entries = %d processedEntries = %d\r\n", bytesRead, numEntries, entriesProcessed);
  for(int i = 0; i < numEntries; i++) {
    fprintf(stderr, "key=%s, value=%s\n", header.entries[i].key, header.entries[i].value);
  }
  
  return 0;
}
