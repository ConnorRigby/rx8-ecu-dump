#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "UDS.h"
#include "OBD2.h"

static const char* TAG = "UDS";

size_t uds_request_send(uds_request_t* request)
{
  size_t ret = UDS_ERROR_OK;

  request->txBuffer[0].DataSize = 
    request->length + // payload length
    1 +               // sid
    4;                // CANID
  request->txBuffer[0].Data[4] = request->sid;

  // nulify these fields
  request->payload = NULL;
  request->length = 0;
  request->sid = 0;

  ret = request->j2534->PassThruWriteMsgs(
    request->chanID, 
    &request->txBuffer[0], 
    &request->numTx, 
    TX_TIMEOUT
  );
  if (ret) return ret;

  for(;;) {
    ret = request->j2534->PassThruReadMsgs(
      request->chanID, 
      &request->rxBuffer[0], 
      &request->numRx, 
      RX_TIMEOUT
    );
    if (ret) break;

    if(request->numRx) {
      if (request->rxBuffer[0].RxStatus & START_OF_MESSAGE)      continue;
			if (request->rxBuffer[0].Data[3] == UDS_REQUEST_CANID_LSB) continue;
      if (request->rxBuffer[0].Data[3] == UDS_RESPONSE_CANID_LSB) {
        if(request->rxBuffer[0].Data[4] == UDS_NEGATIVE_RESPONSE) {
          if(request->rxBuffer[0].Data[6] == UDS_NEGATIVE_RESPONSE_REQUEST_RECEIVED_RESPONSE_PENDING) 
            continue;

          assert(request->rxBuffer[0].DataSize >= 5);
          request->sid     =  UDS_NEGATIVE_RESPONSE;
          request->payload = &request->rxBuffer[0].Data[5];
          request->length  =  request->rxBuffer[0].DataSize - 5;
          ret              = UDS_ERROR_NEGATIVE_RESPONSE;
          break;
        } else {
          assert(request->rxBuffer[0].DataSize >= 5);
          request->sid     =  request->rxBuffer[0].Data[4];
          request->payload = &request->rxBuffer[0].Data[5];
          request->length  =  request->rxBuffer[0].DataSize - 5;
          ret              = UDS_ERROR_OK;
          break;
        }
      }

      // fall thru, will turn into a RX Timeout next loop
      ret = UDS_ERROR_UNKNOWN;
    } 
  }

  return ret;
}


// helper function to clear the tx and rx buffers, and setup a 0x7E0 CANID ISO15765 frame
size_t uds_request_prepare(uds_request_t* request)
{
  request->sid = 0;
  request->payload = NULL;

	assert(request->numTx <= TX_BUFFER_LEN);
	assert(request->numRx <= RX_BUFFER_LEN);

	// zero out both buffers to prevent accidental tx/rx of irrelevant data
	memset(request->txBuffer, 0, TX_BUFFER_LEN);
	memset(request->rxBuffer, 0, RX_BUFFER_LEN);

	for (size_t i = 0; i < request->numTx; i++) {
		memset(request->txBuffer[i].Data, 0, PM_DATA_LEN);
		memset(request->rxBuffer[i].Data, 0, PM_DATA_LEN);

		request->txBuffer[i].ProtocolID = ISO15765;
		request->txBuffer[i].TxFlags = ISO15765_FRAME_PAD;
		
		request->txBuffer[i].Data[0] = 0x0;
		request->txBuffer[i].Data[1] = 0x0;
		request->txBuffer[i].Data[2] = UDS_REQUEST_CANID_MSB;
		request->txBuffer[i].Data[3] = UDS_REQUEST_CANID_LSB;
		request->rxBuffer[i].ProtocolID = ISO15765;
		request->rxBuffer[i].TxFlags = ISO15765_FRAME_PAD;
	}

  request->payload = &request->txBuffer[0].Data[5];
	return UDS_ERROR_OK;
}

size_t uds_request_init(uds_request_t** request_out, 
                        J2534* j2534, 
                        unsigned long devID, 
                        unsigned long chanID)
{
  uds_request_t* request = (uds_request_t*)malloc(sizeof(uds_request_t));
  if(!request) {
    request_out = NULL;
    return ENOMEM;
  }
  
  request->j2534 = j2534;
  request->devID = devID;
  request->chanID = chanID;
  request->numTx = 1;
  request->numRx = 1;
  request->sid = 0;
  request->payload = NULL;

  request->txBuffer = (PASSTHRU_MSG*)malloc(
    sizeof(PASSTHRU_MSG) * request->numTx
  );

  if(!request->txBuffer) {
    free(request);
    request_out = NULL;
    return ENOMEM;
  }

  request->rxBuffer = (PASSTHRU_MSG*)malloc(
    sizeof(PASSTHRU_MSG) * request->numRx
  );

  if(!request->rxBuffer) {
    free(request->txBuffer);
    request->txBuffer = NULL;
    free(request);
    request_out = NULL;
    return ENOMEM;
  }

  *request_out = request;
  return 0;
}

size_t uds_request_deinit(uds_request_t* request)
{
  if(request->txBuffer)
    free(request->txBuffer);

  request->txBuffer = NULL;

  if(request->rxBuffer)
    free(request->rxBuffer);

  request->rxBuffer = NULL;
  
  return 0;
}

static const char* uds_error_string[] = {
  "not an error",
  "unknown",
  "negative response (0x7f)",
  NULL
};
char UDSlastErrorString[255] = {0};

const char* uds_request_error_string(uds_request_t* request, size_t err)
{
  if(err > UDS_ERROR_START) {
    uds_error_string[err - UDS_ERROR_START];
  } else if(err > 0) {
    request->j2534->PassThruGetLastError(UDSlastErrorString);
    return UDSlastErrorString;
  }
  return uds_error_string[0];
}

static const char* uds_negative_response_error_string[] = {
  "GENERAL_REJECT",
  "SERVICE_NOT_SUPPORTED",
  "SUBFUNCTION_NOT_SUPPORTED",
  "INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT",
  "RESPONSE_TOO_LONG",
  "BUSY_REPEAT_REQUES",
  "CONDITIONS_NOT_CORRECT",
  "REQUEST_SEQUENCE_ERROR",
  "REQUEST_OUT_OF_RANGE",
  "INVALID_KEY",
  "EXCEDED_NUMBER_OF_ATTEMPTS",
  "REQUIRED_TIME_DELAY_NOT_EXPIRED",
  "UPLOAD_NOT_ACCEPTED",
  "TRANSFER_DATA_SUSPENDED",
  "GENERAL_PROGRAMMING_FAILURE",
  "WRONG_BLOCK_SEQUENCE_COUNTER",
  "REQUEST_RECEIVED_RESPONSE_PENDING",
  "SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION",
  "SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION",
  "Unknown or reserved",
  NULL
};

const char* uds_request_negative_response_error_string(uds_request_t* request) 
{
  // sorry
  switch(request->rxBuffer[0].Data[6]){
  case UDS_NEGATIVE_RESPONSE_GENERAL_REJECT:                              return uds_negative_response_error_string[0];
  case UDS_NEGATIVE_RESPONSE_SERVICE_NOT_SUPPORTED:                       return uds_negative_response_error_string[1];
  case UDS_NEGATIVE_RESPONSE_SUBFUNCTION_NOT_SUPPORTED:                   return uds_negative_response_error_string[2];
  case UDS_NEGATIVE_RESPONSE_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT:  return uds_negative_response_error_string[3];
  case UDS_NEGATIVE_RESPONSE_RESPONSE_TOO_LONG:                           return uds_negative_response_error_string[4];
  case UDS_NEGATIVE_RESPONSE_BUSY_REPEAT_REQUES:                          return uds_negative_response_error_string[5];
  case UDS_NEGATIVE_RESPONSE_CONDITIONS_NOT_CORRECT:                      return uds_negative_response_error_string[6];
  case UDS_NEGATIVE_RESPONSE_REQUEST_SEQUENCE_ERROR:                      return uds_negative_response_error_string[7];
  case UDS_NEGATIVE_RESPONSE_REQUEST_OUT_OF_RANGE:                        return uds_negative_response_error_string[8];
  case UDS_NEGATIVE_RESPONSE_INVALID_KEY:                                 return uds_negative_response_error_string[9];
  case UDS_NEGATIVE_RESPONSE_EXCEDED_NUMBER_OF_ATTEMPTS:                  return uds_negative_response_error_string[10];
  case UDS_NEGATIVE_RESPONSE_REQUIRED_TIME_DELAY_NOT_EXPIRED:             return uds_negative_response_error_string[11];
  case UDS_NEGATIVE_RESPONSE_UPLOAD_NOT_ACCEPTED:                         return uds_negative_response_error_string[12];
  case UDS_NEGATIVE_RESPONSE_TRANSFER_DATA_SUSPENDED:                     return uds_negative_response_error_string[13];
  case UDS_NEGATIVE_RESPONSE_GENERAL_PROGRAMMING_FAILURE:                 return uds_negative_response_error_string[14];
  case UDS_NEGATIVE_RESPONSE_WRONG_BLOCK_SEQUENCE_COUNTER:                return uds_negative_response_error_string[15];
  case UDS_NEGATIVE_RESPONSE_REQUEST_RECEIVED_RESPONSE_PENDING:           return uds_negative_response_error_string[16];
  case UDS_NEGATIVE_RESPONSE_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION: return uds_negative_response_error_string[17];
  case UDS_NEGATIVE_RESPONSE_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION:     return uds_negative_response_error_string[18];
  default:                                                                return uds_negative_response_error_string[19];
  }
}
