#define MIRROR1 0
#define MIRROR2 1

#define MOTOR_A 0
#define MOTOR_B 1

#define SENSOR_HOME_A 0x01
#define SENSOR_STOW_A 0x02
#define SENSOR_HOME_B 0x10
#define SENSOR_STOW_B 0x20

// MCT Channel values used in SFC messages
#define MCT_RTD_TRACKLEFT_1A   0
#define MCT_RTD_TRACKRIGHT_1A  1
#define MCT_RTD_TRACKLEFT_1B   2
#define MCT_RTD_TRACKRIGHT_1B  3
#define MCT_RTD_MANIFOLD_1     4
#define MCT_HUMIDITY1          5
#define MCT_LOCAL_TEMPA        6
#define MCT_BUSVA              7
#define MCT_RTD_TRACKLEFT_2A   8
#define MCT_RTD_TRACKRIGHT_2A  9
#define MCT_RTD_TRACKLEFT_2B  10
#define MCT_RTD_TRACKRIGHT_2B 11
#define MCT_RTD_MANIFOLD_2    12
#define MCT_HUMIDITY2         13
#define MCT_LOCAL_TEMPB       14
#define MCT_BUSVB             15


typedef struct {
  short posn;  // motor position, in steps
  short targ;  // motor target, in steps
  short mdeg100; // motor position in 100ths of degree
} MOTOR_t;


typedef struct {
  MOTOR_t m[2];
  byte tracking; // tracking state
  byte sensors;  // home and stow
  short manDegC10;   // manifold temperature, degC * 10
} MIRROR_t;


// Definitions for both mirrors vs. 1 mirror at a time
typedef enum {
  ALTERNATE_OFF,
  ALTERNATE_ON
} mALTERNATE;


typedef struct {
  byte fwType;
  byte fwMajor;
  byte fwMinor;
  byte slaveFwType;
  byte slaveFwMajor;
  byte slaveFwMinor;
  char versionStr[40];
  char slaveVersionStr[40];
  ulong serNum;
  ulong slaveSerNum;
  ulong sysSec;
  ushort sysTicks;
  ulong slaveSysSec;
  ushort slaveSysTicks;
  MIRROR_t mirror[2];
  ushort chan[16];
  byte stat;
} MCT_t;

typedef enum {
  STRING_POWER_OFF,
  STRING_POWER_UP,
  STRING_INIT_PING0,
  STRING_INIT_SET_ADDR,
  STRING_INIT_SET_DIR,
  STRING_INIT_JUMP_TO_APP,
  STRING_INIT_GET_INFO,
  STRING_ACTIVE,
  STRING_REPING,
  NUM_STRING_STATES
} STRING_STATE_m;

#define NUM_STRINGS 4
#define NUM_MCT    10

typedef enum {
  MCT_POLL_OFF,
  MCT_POLL_CHAN,
  MCT_POLL_MIRRORS
} MCT_POLL_TYPE_m;

typedef struct {
  IoPortId port_PS_En;
  ushort mask_PS_En;
  IoPortId port_PowerEn;
  ushort mask_PowerEn;
  ushort powerupDelay;
  IoPortId port_RxEnn;
  ushort mask_RxEnn;
  IoPortId port_TxEn;
  ushort mask_TxEn;
  STRING_STATE_m state;
  ushort maxAddr;
  MCT_t mct[NUM_MCT];
  byte pollMct;
  byte pollDelay;
  MCT_POLL_TYPE_m pollType;
} STRING_t;


EXTERN STRING_t string[NUM_STRINGS];
EXTERN byte stringActive;
EXTERN byte mctInitDir;
EXTERN byte stringInitRetry;
EXTERN byte stringInitReset;

// string.c prototypes
void StringInit(void);
void StringUpdate(void);
void StringSetState(byte stringNum, STRING_STATE_m newState);
byte StringInitResp(void);
void StringKeepAliveBroadcast(void);
