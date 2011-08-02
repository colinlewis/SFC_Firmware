#define GLOBALS_HERE
#include "sfc.h"
#include "ads1115.h"
#include "ads1244.h"
#include "datalog.h"
#include "field.h"
#include "i2c1.h"
#include "lcd.h"
#include "mct485.h"
#include "mctbus.h"
#include "param.h"
#include "rtc.h"
#include "rtu.h"


// Run SYSCLK at 72MHz, PBCLK at 72MHz
#pragma config FPLLMUL = MUL_18, FPLLIDIV = DIV_2
#pragma config FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = XT, FNOSC = PRIPLL, FPBDIV = DIV_1
#pragma config FSOSCEN = OFF
#pragma config ICESEL = ICS_PGx1
#pragma config UPLLEN   = ON   	// USB PLL Enabled
#pragma config UPLLIDIV = DIV_2 // USB PLL Input Divider
#pragma config CP = OFF, PWP = OFF


// Let compile time pre-processor calculate the PR1 (period)
#define SYS_FREQ 			(72000000L)
#define PB_DIV         		1
#define PRESCALE       		64
#define TOGGLES_PER_SEC		1000
#define T1_TICK       		(SYS_FREQ/PB_DIV/PRESCALE/TOGGLES_PER_SEC)


const char versString[21] = "FW V0.3HACK 8/ 1/2011";
byte test;
byte t1;

#ifdef DEBUG
unsigned int missedMilliseconds=0;
#endif

int main(void) {
  MainInit();
  I2C1init();
  RtcInit();
  LcdInit();
  DataLogInit();
  StringInit();
  Mct485Init();
  FieldInit();
  Ads1115Init();
  Ads1244Init();
  USBInit();
  RtuInit();
  AdcInit();
  TC74Init();

  // enable multi-vector interrupts
  INTEnableSystemMultiVectoredInt();

  MainDelay(50);
  DataLogDateTime(DLOG_SFC_POWERUP);

  // init param after interrupts are enabled 
  ParamInit();
  // wait to init desiccant until after ParamInit
  DesiccantInit();

  string[0].mct[0].chan[4] = 0x7FFF;



	// 
	// Begin Main Loop
	//
  for (;;) {
    if (test == 1) {
      test = 0;
      FieldNewState((FIELD_STATE_m)t1);
    }
    
    USBUpdate(); // called as often as possible
    
    //sysTickEvent every ms
    if (sysTickEvent) {
      sysTickEvent = 0;
      sysTicks++;

      UsbTimeoutUpdate();

      LcdUpdate();
      
      // fill time before dropping LCD_E
      if (sysTicks >= 1000) {
        sysSec++;
        sysTicks = 0;
        
        mPORTDToggleBits(PD_LED_HEARTBEAT);
        
        // These Updates are 
        // called once a second
        //TODO if any of these are long, we could split them on separate milliseconds.
        DesiccantUpdate();
        AdcUpdate();
        TC74Update();
        
      }// end 1 Hz
      
      else if (sysTicks == 250) {
        mPORTDToggleBits(PD_LED_HEARTBEAT);
      }
      else if (sysTicks == 500) {
        mPORTDToggleBits(PD_LED_HEARTBEAT);
      }
      else if (sysTicks == 750) {
        mPORTDToggleBits(PD_LED_HEARTBEAT);
      }
      // Complete LcdUpdate() by dropping LCD_E)
      PORTClearBits(IOPORT_G, PG_LCD_E);

      // These Updates called once each millisecond
      RtcUpdate();
      I2C1update();
      StringUpdate();
      Mct485Update();
      FieldUpdate();
      RtuUpdate();
      DessicantFanPWMupdate();
      
    } // if (sysTickEvent)
  } // for (;;)
} // main()


void MainInit(void) {
  PORTSetPinsDigitalOut(IOPORT_D, PD_LED_HEARTBEAT);
  sysSec = 0;
  sysTicks = 0;
  sysTickEvent = 0;

	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

  OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_64, T1_TICK);
  
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
} // MainInit()


void MainDelay(ushort msec) {
  while (msec) {
    if (sysTickEvent) {
      sysTickEvent = 0;
      sysTicks++;

      if (sysTicks >= 1000) {
        sysSec++;
        sysTicks = 0;
      }

      I2C1update();

      msec--;
    } // if (sysTickEvent)
  } // while (msec)
} // MainDelay()

// Count the milliseconds since we have received a message via USB.
// If we reach zero, several test operations (logging, test mode, update mode and dessicant drying) will be shut down.
void UsbTimeoutUpdate(void)
{
  if (usbActivityTimeout > 0) {         // this will count down to zero and stick
    if (usbActivityTimeout != 0xFFFF) { // value of 0xFFFF means we will never time out.
      usbActivityTimeout--;
    }  
  }
}

void __ISR(_TIMER_1_VECTOR, ipl2) Timer1Handler(void) {
  // clear the interrupt flag
  mT1ClearIntFlag();
  
  #ifdef DEBUG
//Detect overruns
 if (sysTickEvent) // main loop did not notice the last sysTickEvent
   missedMilliseconds++;
//or,
// sysTickEvent++; 
// in main loop, if sysTickEvent >1 then we missed milliseconds
//or,
// at end of main loop, assert sysTickEvent == 0
#endif
  sysTickEvent = 1;
}

