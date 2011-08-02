
// CMD's
#define MCT_CMD_NULL         0x00
#define MCT_CMD_RESEND       0x01
#define MCT_CMD_JUMP_TO_APP  0x02
#define MCT_CMD_ERASE_APP    0x03
#define MCT_CMD_BLANK_CHECK  0x04
#define MCT_CMD_PROG_APP     0x05
#define MCT_CMD_READ_FLASH   0x06
#define MCT_CMD_FLASH_CKSUM  0x07
#define MCT_CMD_JUMP_TO_BOOT 0x08
#define MCT_CMD_RESET        0x10
#define MCT_CMD_SET_ADDR     0x11
#define MCT_CMD_SET_DIR      0x12
#define MCT_CMD_SERNUM       0x13
#define MCT_CMD_VERSION      0x14
#define MCT_CMD_LOG          0x15
#define MCT_CMD_TIME         0x16
#define MCT_CMD_STRING       0x17
#define MCT_CMD_POSN         0x20
#define MCT_CMD_TARG         0x21
#define MCT_CMD_ADJ_TARG     0x22
#define MCT_CMD_RATE         0x23
#define MCT_CMD_GET_SENSORS  0x30
#define MCT_CMD_GET_AD       0x31
#define MCT_CMD_GET_RTD      0x32
#define MCT_CMD_GET_CHAN     0x33
#define MCT_CMD_TRACK        0x40
#define MCT_CMD_PARAM        0x41
#define MCT_CMD_GET_MIRRORS  0x42
#define MCT_CMD_POWER        0x43
#define MCT_CMD_ALTERNATE    0x44

// RESP's
#define MCT_RESP_NULL        0x80
#define MCT_RESP_BAD_MSG     0x81
#define MCT_RESP_JUMP_TO_APP 0x82
#define MCT_RESP_ERASE_APP   0x83
#define MCT_RESP_BLANK_CHECK 0x84
#define MCT_RESP_PROG_APP    0x85
#define MCT_RESP_READ_FLASH  0x86
#define MCT_RESP_FLASH_CKSUM 0x87
#define MCT_RESP_JUMP_TO_BOOT 0x88
#define MCT_RESP_RESET       0x90
#define MCT_RESP_SET_ADDR    0x91
#define MCT_RESP_SET_DIR     0x92
#define MCT_RESP_SERNUM      0x93
#define MCT_RESP_VERSION     0x94
#define MCT_RESP_LOG         0x95
#define MCT_RESP_TIME        0x96
#define MCT_RESP_STRING      0x97
#define MCT_RESP_POSN        0xA0
#define MCT_RESP_TARG        0xA1
#define MCT_RESP_ADJ_TARG    0xA2
#define MCT_RESP_RATE        0xA3
#define MCT_RESP_SETTING2    0xA8
#define MCT_RESP_GET_SENSORS 0xB0
#define MCT_RESP_GET_AD      0xB1
#define MCT_RESP_GET_RTD     0xB2
#define MCT_RESP_GET_CHAN    0xB3
#define MCT_RESP_TRACK       0xC0
#define MCT_RESP_PARAM       0xC1
#define MCT_RESP_GET_MIRRORS 0xC2
#define MCT_RESP_POWER       0xC3
#define MCT_RESP_ALTERNATE   0xC4


// ADDR field values
#define MCT485_ADDR_BLANK 0x00  // not initialized
#define MCT485_ADDR_GROUP 0x20  // group bit mask
#define MCT485_ADDR_BCAST 0x40  // broadcast bit mask, lower 6 bits define which MCT sends response
#define MCT485_ADDR_HOST  0x80  // 0x81-0xFD are messages to host
#define MCT485_MCT_MASTER 0xFE  // message within MCT

// Indices into 485 buffers
#define MCT485_ADDR     0
#define MCT485_MSG_ID   1
#define MCT485_LEN      2
#define MCT485_CMD      3
#define MCT485_RESP     3
#define MCT485_DATA     4
#define MCT485_MIN_LEN  6

#define MCT_ERR_BAD_LEN 0xFF
#define MCT_ERR_BAD_CMD 0xFE
#define MCT_ERR_BAD_PRM 0xFD

typedef enum {
  MSG_ORIGIN_NORMAL,
  MSG_ORIGIN_INIT,
  MSG_ORIGIN_USB
} MSG_ORIGIN_m;

typedef enum {
  MCT485_UNINIT,
  MCT485_FC_LEFT,
  MCT485_FC_RIGHT
} MCT485_MODES_m;

typedef enum {
  MCT485_IDLE,
  MCT485_TX,
  MCT485_RX
} MCT485_STATES_m;

EXTERN MCT485_STATES_m mct485State;

#define MCT485_MAX_INDEX  64
#define MCT485_QUEUE_SIZE 200
#define MCT485_RETRY_LIMIT 3
EXTERN byte mct485RxBuffer[MCT485_MAX_INDEX];
EXTERN byte mct485RxIndex;
EXTERN byte mct485RxTimeout;
EXTERN byte mct485TxQueue[MCT485_QUEUE_SIZE][MCT485_MAX_INDEX];
EXTERN byte mct485String[MCT485_QUEUE_SIZE];
EXTERN MSG_ORIGIN_m mct485Origin[MCT485_QUEUE_SIZE];
EXTERN byte mct485TxIndex;
EXTERN byte mct485TxLen;
EXTERN byte mct485TxQueueRd;
EXTERN byte mct485TxQueueWr;
EXTERN byte mct485TxRetry;
EXTERN byte mct485MsgID;
EXTERN byte mct485Flush;


// Mct485.c prototypes
void Mct485Init(void);
void Mct485Update(void);
void Mct485SendMsg(byte origin, byte stringNum, byte addr, byte cmd, byte *data);
void Mct485SendNextMsg(void);
void Mct485CannedMsg(byte origin, byte stringNum, byte len, byte *data);
void Mct485PriorityMsg(byte origin, byte stringNum, byte len, byte *data);
void Mct485AdvRdQueue(void);
byte Mct485RespValid(void);
byte Mct485ProcessResp(void);
ulong Mct485UnpackLong(byte *pBuffer);
ushort Mct485UnpackShort(byte *pBuffer);
