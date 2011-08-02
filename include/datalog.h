
#define DLOG_NOT_VALID      0x00
#define DLOG_PAGE_ID        0x01 // 0xFE, ID[2], invID[2], 0x55AA
#define DLOG_DATE_TIME      0x02 // subtype, YR, MO, DAY, HR, MIN, SEC
#define DLOG_START_OF_DAY   0x03 // subtype 0, MIN, SEC, MCT_A, MCT_B, MCT_C, MCT_D
#define DLOG_END_OF_DAY     0x04 // subtype 0, MIN, SEC, cause, 0, 0, 0
#define DLOG_HOURLY_DATA    0x05 // subtype
#define DLOG_BLANK          0xFF

// DLOG_DATA_TIME subtypes
#define DLOG_SFC_POWERUP      0x01
#define DLOG_HOURLY_STAMP     0x02

// DLOG_HOURLY_DATA subtypes
#define DLOG_INSOLATION       0x01 // avg, min, max
#define DLOG_AVG_MIR_POSN     0x02 // avg, min, max
#define DLOG_POWER_OUTPUT     0x03 // avg, min, max
#define DLOG_INLET_TEMP       0x04 // avg, min, max
#define DLOG_OUTLET_TEMP      0x05 // avg, min, max
#define DLOG_SFC_TEMP         0x06 // SFC therm, humidity, PCB temp
#define DLOG_MCT_TEMP         0x07 // min loc, max loc, min temp, max temp
#define DLOG_MCT_HUMIDITY     0x08 // min loc, max loc, min humid, max humid


typedef struct {
  byte type;
  byte subtype;
  byte data[6];
} DATA_LOG_t;

#define DATA_LOG_MAX_ENTRY 15360
#define DATA_LOG_PAGE_SIZE  4096

EXTERN ushort dataLogNextEmpty;
EXTERN ushort dataLogNextRead;
EXTERN ushort dataLogPageID;
EXTERN byte dataLogLastHour;
EXTERN byte dataLogLastMin;
EXTERN ushort dataLogSumCount;
EXTERN ulong dataLogInsolSum;
EXTERN ushort dataLogInsolMin;
EXTERN ushort dataLogInsolMax;
EXTERN ulong dataLogMirPosnSum;
EXTERN ushort dataLogMirPosnMin;
EXTERN ushort dataLogMirPosnMax;
EXTERN ulong dataLogPowerSum;
EXTERN ushort dataLogPowerMin;
EXTERN ushort dataLogPowerMax;
EXTERN long dataLogInletSum;
EXTERN short dataLogInletMin;
EXTERN short dataLogInletMax;
EXTERN long dataLogOutletSum;
EXTERN short dataLogOutletMin;
EXTERN short dataLogOutletMax;
EXTERN short dataLogMctTempMin;
EXTERN byte dataLogMctTMinLoc;
EXTERN short dataLogMctTempMax;
EXTERN byte dataLogMctTMaxLoc;
EXTERN ushort dataLogMctHumidMin;
EXTERN byte dataLogMctHMinLoc;
EXTERN ushort dataLogMctHumidMax;
EXTERN byte dataLogMctHMaxLoc;


// datalog.h prototypes
void DataLogInit(void);
byte DataLogPageValid(ushort page);
void DataLogStartPage(void);
void DataLogAddEntry(DATA_LOG_t *logEntry);
void DataLogFindFirstEntry(void);
void DataLogErase(void);
byte DataLogReadNext(byte *buffer);
void DataLogDateTime(byte subtype);
void DataLogStartofDay(void);
void DataLogHourly(void);
void DataLogClearData(void);
void DataLogUpdateData(void);
