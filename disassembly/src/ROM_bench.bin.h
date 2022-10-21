typedef unsigned char   undefined;

typedef unsigned char    bool;
typedef unsigned char    byte;
typedef unsigned int    uint;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned short    ushort;
typedef enum uds_sid {
    UDS_SID_DIAGNOSTIC_SESSION_CONTROL=16,
    UDS_SID_ECU_RESET=17,
    UDS_SID_READ_DATA_BY_ID=34,
    UDS_SID_SECURITY_ACCESS_CONTROL=39,
    UDS_SID_COMMUNICATION_CONTROL=40,
    UDS_SID_REQUEST_DOWNLOAD=52,
    UDS_SID_TRANSFER_DATA=54,
    UDS_SID_TRANSFER_EXIT=55,
    UDS_SID_TESTER_PRESENT=62,
    UDS_SID_ECU_RESET_ACK=81,
    UDS_SID_SECURITY_ACCESS_CONTROL_ACK=103,
    UDS_SID_REQUEST_DOWNLOAD_ACK=116,
    UDS_SID_TRANSFER_DATA_ACK=118,
    UDS_SID_TRANSFER_EXIT_ACK=119,
    UDS_SID_NEGATIVE_RESPONSE=127,
    UDS_SID_DTC_CONTROL=133
} uds_sid;

