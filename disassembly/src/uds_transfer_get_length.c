#include <stdint.h>
#include <stdio.h>
typedef uint8_t byte;


uint8_t uds_request_download_get_address_offset(byte *payload)

{
  return ((int)(char)payload[1] & 0xffU) * 0x10000 + ((int)(char)payload[2] & 0xffU) * 0x100 +
         (char)*payload * 0x1000000 + ((int)(char)payload[3] & 0xffU);
}

int uds_transfer_get_length(byte *payload)

{
  return ((int)(char)payload[1] & 0xffU) * 0x100 + ((int)(char)*payload & 0xffU) * 0x10000 +
         ((int)(char)payload[2] & 0xffU);
}

int main(int argc, char const *argv[])
{
  // byte payload[3] = {0, 0x07, 0xf8};
  byte payload[] = {0x34, 00, 00, 0x40, 00, 00, 0x07, 0xF8, 00, 00, 00, 00, 00};
  int value = uds_transfer_get_length(payload+5);
  printf("value=0x%08X\n\n", value);
  printf("value-0x7e000=0x%08X\n\n\n", value-0x7e000);

  value = uds_request_download_get_address_offset(payload);
  printf("value=0x%08X\n\n", value);

  /* code */
  return 0;
}
