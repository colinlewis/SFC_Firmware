
typedef enum {
  RTU485_IDLE,
  RTU485_TX,
  RTU485_RX
} RTU485_STATES_m;

EXTERN RTU485_STATES_m rtu485State;


#define RTU_MAX_INDEX 255
EXTERN byte rtu485RxBuffer[RTU_MAX_INDEX];
EXTERN byte rtu485RxIndex;
EXTERN byte rtu485RxTimeout;
EXTERN byte rtu485TxBuffer[RTU_MAX_INDEX];
EXTERN byte rtu485TxIndex;
EXTERN byte rtu485TxLen;
EXTERN byte rtu485TxTimeout;
EXTERN ushort rtuAddr[16];
EXTERN ushort rtuData[16];
EXTERN ushort rtuNextAddr;
EXTERN ushort rtuNumAddr;


// rtu.c prototypes
void RtuInit(void);
void RtuUpdate(void);
void RtuTxEn(byte en);
void RtuProcessMsg(void);
void RtuReadNextHolding(void);


// rtucrc.c prototypes
ushort RtuCrcCalc(byte *data, byte len);
