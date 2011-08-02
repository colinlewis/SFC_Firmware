#include <plib.h>


#define DEBUG //temp - turn on debugging/internal checking functions

#ifdef GLOBALS_HERE
#define EXTERN
#else
#define EXTERN extern
#endif

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef void (*fptr)(void);


// MCT 485 signals
#define PG_MCT_RX           0x0080
#define PG_MCT_TX           0x0100
#define PD_MCT_RX_ENN_A     0x0100
#define PD_MCT_TX_EN_A      0x0800
#define PC_MCT_RX_ENN_B     0x2000
#define PC_MCT_TX_EN_B      0x4000
#define PD_MCT_RX_ENN_C     0x0002
#define PD_MCT_TX_EN_C      0x0004
#define PD_MCT_RX_ENN_D     0x0008
#define PD_MCT_TX_EN_D      0x1000

#define PG_PS_EN_A          0x8000
#define PC_PS_EN_B          0x0002
#define PC_PS_EN_C          0x0004
#define PC_PS_EN_D          0x0008

#define PD_POWER_EN_A       0x0010
#define PD_POWER_EN_B       0x0020
#define PD_POWER_EN_C       0x0040
#define PD_POWER_EN_D       0x0080

#define PB_ISENSE_A         0x0004
#define PB_ISENSE_B         0x0008
#define PB_ISENSE_C         0x0010
#define PB_ISENSE_D         0x0020

#define PB_VSENSE_A         0x0100
#define PB_VSENSE_B         0x0200
#define PB_VSENSE_C         0x0400
#define PB_VSENSE_D         0x0800

#define PB_PS_SENSE_A       0x1000
#define PB_PS_SENSE_B       0x2000
#define PB_PS_SENSE_C       0x4000
#define PB_PS_SENSE_D       0x8000


// Pushbuttons
#define PA_PB_PREV          0x0001
#define PA_PB_DOWN          0x0002
#define PA_PB_NEXT          0x0010
#define PA_PB_UP            0x0020
#define PA_PB_ALL           0x0033


// Enclosure Signals
#define PB_THERMISTOR       0x0040
#define PB_HUMIDITY         0x0080
#define PG_DESSICANT_FAN_PWM 0x0001 // ON PortG
#define PG_UNUSED           0x0002  // On Port G


// LCD signals
#define PG_LCD_RS           0x2000
#define PG_LCD_RW           0x1000
#define PG_LCD_E            0x4000
#define PE_LCD_DATA         0x00FF

#define PD_LED_HEARTBEAT    0x2000
#define PA_SDA1             0x8000
#define PA_SCL1             0x4000


// FCE signals
#define PA_EXP_RD           0x0040
#define PA_EXP_WR           0x0080
#define PG_EXP_TX           0x0040
#define PG_EXP_RX           0x0200
#define PF_EXP_CAN_TX       0x0002
#define PF_EXP_CAN_RX       0x0001
#define PE_EXP_DATA         0x00FF

// FCE outputs using PORTD latch
#define PE_HC595_DIN        0x0001
#define PE_HC595_RCLK       0x0002
#define PE_HC595_SCLK       0x0004
#define PE_ADS1244_SCLK     0x0008
#define PE_EXP_RS485_EN     0x0010

// FCE inputs using PORTE latch
#define PE_FUSE_485         0x0001
#define PE_FUSE_INPUTs      0x0002
#define PE_FUSE_OUTPUTS     0x0004
#define PE_FUSE_PYR         0x0008
#define PE_EXP_RESETN       0x0010
#define PE_EXP_TYPE         0x0060
#define PE_ADS1244_DOUT     0x0080


// ADC10 channels
#define AN_ISENSE_A     2
#define AN_ISENSE_B     3
#define AN_ISENSE_C     4
#define AN_ISENSE_D     5
#define AN_THERMISTOR   6
#define AN_HUMIDITY     7
#define AN_VSENSE_A     8
#define AN_VSENSE_B     9
#define AN_VSENSE_C    10
#define AN_VSENSE_D    11
#define AN_PS_SENSE_A  12
#define AN_PS_SENSE_B  13
#define AN_PS_SENSE_C  14
#define AN_PS_SENSE_D  15


extern const char versString[21];
EXTERN byte sysTickEvent;
EXTERN ushort sysTicks;
EXTERN ulong sysSec;
EXTERN short sfcThermistor;
EXTERN ushort sfcHumidity;
EXTERN short sfcPcbTemp;


// main.c prototypes
int main(void);
void MainInit(void);
void MainDelay(ushort msec);
void UsbTimeoutUpdate();



// adc.c prototypes
void AdcInit(void);
void AdcUpdate(void);
ushort AdcConvHumidity(ushort adcHumidity);
ushort AdcConvThermistor(ushort adcThermistor);


// crc.c prototypes
ushort CrcCalc(byte len, byte *data);


// lcd.c prototypes
void LcdInit(void);
void LcdUpdate(void);
void LcdFifoCmd(ushort cmd);
void LcdFifoData(ushort data);
void LcdFifoString(byte row, byte col, char *s);


// tc74.c prototypes
void TC74Init(void);
void TC74Update(void);
void TC74ProcessResp(byte index);


// usb.c prototypes
void USBInit(void);
void USBUpdate(void);
void Usb485Resp(void);
EXTERN byte usb485RxLen;
EXTERN byte usb485RxBuffer[64];
EXTERN byte usb485TxString;
EXTERN byte usb485TxLen;
EXTERN byte usb485TxBuffer[64];
EXTERN ushort usbActivityTimeout; // if this counts down to zero then it has been too long since we have seen a USB message.
