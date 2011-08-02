

typedef enum {
  LCD1_VERSION,
  LCD1_DEMO,
  LCD2_DEMO_RATE,
  LCD2_DEMO_PAUSE,
  LCD2_DEMO_RUN,
  LCD1_STRINGS,
  LCD1_MCT_STAT,
  LCD2_MCT_STAT_MIR1,
  LCD2_MCT_STAT_MIR2,
  LCD1_FCE,
  LCD1_SYS30,
  LCD2_SYS30,
  LCD_NUM_STATES
} LCD_STATES_m;


typedef enum {
  LCD_BUTTON_NONE,
  LCD_BUTTON_PREV,
  LCD_BUTTON_DOWN,
  LCD_BUTTON_UP,
  LCD_BUTTON_NEXT,
  LCD_BUTTON_WAIT_CLEAR
} LCD_BUTTON_m;


#define LCD_BUTTON_DEBOUNCE 5   //  40 msec
#define LCD_BUTTON_SLOW     60  // 480 msec
#define LCD_BUTTON_FAST     8   //  64 msec
#define LCD_BUTTON_NUM_SLOW 3   // first 3 count slow

typedef enum {
  LCD_DEMO_INIT,
  LCD_DEMO_HOME,
  LCD_DEMO_HOME_PAUSE,
  LCD_DEMO_STOW,
  LCD_DEMO_STOW_PAUSE
} LCD_DEMO_MODE_m;

EXTERN LCD_STATES_m lcdState;
EXTERN LCD_BUTTON_m lcdButton;
EXTERN LCD_BUTTON_m lcdButtonFilter;
EXTERN byte lcdButtonDelay;
EXTERN byte lcdButtonRepeat;
EXTERN byte lcdButtonCountdown;
EXTERN byte lcdStringNum;
EXTERN byte lcdMctNum;
EXTERN ushort lcdRedraw;
EXTERN LCD_DEMO_MODE_m lcdDemoMode;
EXTERN ushort lcdDemoRate;
EXTERN ushort lcdDemoPause;
EXTERN ushort lcdDemoCountdown;
extern const LCD_STATES_m lcdStatePrev[LCD_NUM_STATES];
extern const LCD_STATES_m lcdStateNext[LCD_NUM_STATES];
extern const char lcdMonth[12][4];

void LcdDrawState(void);
void LcdNewState(LCD_STATES_m newState);
void LcdDrawRTD(char *s, long n, byte lj, byte dp);
void LcdDrawN(char *s, long n, byte lj, byte dp);
void LcdDrawHex2(char *s, byte n);
void LcdDrawHex(char *s, byte n);
void LcdFifoString(byte row, byte col, char *s);
void LcdButtonEvent(void);
void LcdButtonUpdate(void);
void LcdButtonWaitClear(void);
