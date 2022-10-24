#include <stdint.h>
#include <stdio.h>
typedef uint8_t byte;


int uds_request_download_get_chunk_size(byte *payload)

{
  return ((int)(char)payload[1] & 0xffU) * 0x10000 + ((int)(char)payload[2] & 0xffU) * 0x100 +
         (char)*payload * 0x1000000 + ((int)(char)payload[3] & 0xffU);
}

int uds_transfer_get_length(byte *payload)

{
  return ((int)(char)payload[1] & 0xffU) * 0x100 + ((int)(char)*payload & 0xffU) * 0x10000 +
         ((int)(char)payload[2] & 0xffU);
}

char* RAM_START;

int main(int argc, char const *argv[])
{
  int chunk_size, length;
  char payload[] = { 0x00, 0x00, 0x40, 0x00, 0x00,  0x07, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00};
  chunk_size = uds_request_download_get_chunk_size(payload);
  printf("chunksize=%08x\n\n", chunk_size);
              /* firmware gets coppied here */
  RAM_START = 0xffff6000;
  int uds_transfer_start_ptr = RAM_START + chunk_size;
  printf("uds_transfer_start_ptr=%08x\n\n", uds_transfer_start_ptr);

  chunk_size = uds_transfer_get_length(payload + 5);
  printf("transfer length=%08x\n\n", chunk_size);

  int uds_transfer_bytes_remaining = chunk_size - 0x7e000;
  int uds_transfer_address = uds_transfer_start_ptr;
  if ((0 < uds_transfer_bytes_remaining) /* &&
      ((uds_frame *)(uds_transfer_start_ptr + chunk_size + -0x7e001) < frame)*/) {
    printf("0 < uds_transfer_bytes_remaining %08x\n", uds_transfer_bytes_remaining);
    if (uds_transfer_bytes_remaining < 0) {
      length = ~(~uds_transfer_bytes_remaining + 1U & 0x3ff) + 1;
      printf("uds_transfer_bytes_remaining < 0 %08x \n", length);
    }
    else {
      length = uds_transfer_bytes_remaining & 0x3ff;
      printf("uds_transfer_bytes_remaining & 0x3ff %08x \n", length);
    }
    if (length == 0) {
      printf("length = 0\n");
      // uds_flash_unlocked = false;
      // uds_download_in_progress = true;
      // UDS_REPLY.sid = UDS_SID_REQUEST_DOWNLOAD_ACK;
      // UDS_REPLY.sbf = 4;
      // UDS_REPLY.data[0] = 1;
              /* 7E8#03 74 04 01 00 00 00 00 */
      // UDS_REPLY.length = 2;
      // uds_download_kernel_complete = false;
      // uds_transfer_counter = 1;
      // goto uds_handle_request_download_send_frame;
    }
  }


  /* code */
  return 0;
}
