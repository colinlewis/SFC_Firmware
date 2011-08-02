

typedef enum {
  EXP485_IDLE,
  EXP485_TX,
  EXP485_RX
} EXP485_STATES_m;


typedef enum {
  FIELD_OFF,
  FIELD_PUMP_ON,
  FIELD_GO_ON_SUN,
  FIELD_OPERATE,
  FIELD_END_OF_DAY,
  FIELD_TEST,
  FIELD_TEST_ON_SUN,
  FIELD_TEST_OPERATE,
  FIELD_TEST_UPDATE,
  FIELD_TEST_END_OF_DAY,
  FIELD_TEST_OFF,
  FIELD_LOGGING,
  FIELD_LOGGING_OFF
} FIELD_STATE_m;


// fieldStatus bits
#define FIELD_STAT_HEAT_REQ     0x01
#define FIELD_STAT_PYR_BYPASS   0x02
#define FIELD_STAT_INTERLOCK    0x08
#define FIELD_STAT_UPS_SHUTDOWN 0x10
#define FIELD_STAT_FLOW_SW      0x40
#define FIELD_STAT_DNI_HIGH     0x80


typedef enum {
  FCE_TYPE1,  // D5:6 = 00
  FCE_TYPE2,  // D5:6 = 01
  FCE_TYPE3,  // D5:6 = 10
  FCE_NONE    // D5:6 = 11
} FCE_TYPE_m;


// FCE_outputs
#define FCE_OUT_PUMP    0x01
#define FCE_OUT_FAN     0x02
#define FCE_OUT_VALVE   0x04
#define FCE_OUT_HEAT    0x08
#define FCE_OUT_4_20mA1 0x10
#define FCE_OUT_4_20mA2 0x20
#define FCE_OUT_4_20mA3 0x40
#define FCE_OUT_4_20mA4 0x80


typedef enum {
  DESICCANT_OFF,
  DESICCANT_DRYING,
  DESICCANT_REGEN,
  DESICCANT_CLOSED,
  DESICCANT_MANUAL,
} mDESICCANT_STATE;

EXTERN mDESICCANT_STATE desiccantState;



EXTERN EXP485_STATES_m exp485State;
EXTERN FIELD_STATE_m fieldState;
EXTERN ushort fieldTimeout;
EXTERN byte fieldLoggingMode;
EXTERN FCE_TYPE_m fceType;
EXTERN byte fieldInState;
EXTERN byte FCE_inputs;
EXTERN byte FCE_outputs;
EXTERN short fceRTD;
EXTERN byte ads1244sclks;
EXTERN long ads1244data;
EXTERN byte ads1244timeout;
EXTERN ushort fieldDNI;   // W/m2
EXTERN ushort fieldPower; // BTU/Hr
EXTERN ushort fieldInlet; // degC * 10
EXTERN ushort fieldOutlet; // degC * 10
EXTERN ushort fieldFlow;  // L/Hr
EXTERN ushort fieldOnSunRetryTicks;
EXTERN ushort fieldEndOfDayTicks;
EXTERN ushort fieldAttenPercent; // 0 - 100%
EXTERN ushort fieldAttenOldPercent;
EXTERN ushort fieldAttenNum; // number of MCT's
EXTERN ushort fieldAttenTimeout; // milliseconds
// 10.35uV/W/m2 * 2^23 / 2*2.5Vref * 256
#define ADCpWpm2  4445
#define MAX_DNI 2000
#define DNI_START 300
#define DNI_STOP  220


void FieldInit(void);
void FieldUpdate(void);
void FieldNewState(FIELD_STATE_m newState);
void FieldUpdateOutputs(void);
void FieldReadInputs(void);
void FieldUpdateI2C(void);
void FieldAttenUpdate(void);


// desiccant.c prototypes
void DesiccantInit(void);
void DesiccantUpdate(void);
void DesiccantNewState(mDESICCANT_STATE newState, byte outputs);
void DesiccantCheckTime(void);
void DessicantFanPWMupdate(void);
