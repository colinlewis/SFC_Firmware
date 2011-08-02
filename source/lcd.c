#include "sfc.h"
#include "field.h"
#include "lcd.h"
#include "mct485.h"
#include "mctbus.h"
#include "rtc.h"
#include "rtu.h"

#define LCD_FIFO_SIZE 128
ushort lcdFifo[LCD_FIFO_SIZE];
byte lcdFifoWr;
byte lcdFifoRd;
byte lcdDelay;

// Commands bytes used by the LCD
#define LCD_CMD_DISP_ON 0x0C
#define LCD_CMD_CLEAR   0x01
#define LCD_CMD_4LINE   0x09
#define LCD_CMD_FSET    0x3C
#define LCD_CMD_FCLR    0x38
#define LCD_CMD_WR_DATA 0x80



void LcdInit(void) {
  PORTSetPinsDigitalOut(IOPORT_E, PE_LCD_DATA);
  PORTSetPinsDigitalOut(IOPORT_G, PG_LCD_RS |
                                  PG_LCD_RW |
                                  PG_LCD_E);

  PORTClearBits(IOPORT_G, PG_LCD_E);
  // clear the FIFO
  lcdFifoWr = 0;
  lcdFifoRd = 0;
  lcdDelay = 0;
  // Initialize a 4 line LCD
  LcdFifoCmd(LCD_CMD_FSET);
  LcdFifoCmd(LCD_CMD_FSET);
  LcdFifoCmd(LCD_CMD_4LINE);
  LcdFifoCmd(LCD_CMD_FCLR);
  LcdFifoCmd(LCD_CMD_DISP_ON);
  LcdFifoCmd(LCD_CMD_CLEAR);
  LcdFifoCmd(LCD_CMD_WR_DATA);
  LcdNewState(LCD1_VERSION);

  LcdButtonWaitClear();  
  lcdDemoRate = 40;
  lcdDemoPause = 5000;
} // LcdInit()


void LcdUpdate(void) {
  if (lcdButtonDelay) {
    lcdButtonDelay--;
  }
  else {
    LcdButtonUpdate();
    lcdButtonDelay = 7;
  }
  
  // redraw when lcdRedraw is zero
  if (lcdRedraw) {
    if (lcdRedraw != 0xFFFF) {
      lcdRedraw--;
    }  
  }
  else {
    LcdDrawState();
  }
  
  // Some commands require a 2msec delay
  if (lcdDelay) {
    lcdDelay--;
    return;
  }
  if (lcdFifoRd != lcdFifoWr) {
    // 100nsec setup time before ECLK
    if (lcdFifo[lcdFifoRd] & 0x100) {
      // data byte
     PORTSetBits(IOPORT_G, PG_LCD_RS);
    }
    else {
      // Command byte
      PORTClearBits(IOPORT_G, PG_LCD_RS);
      lcdDelay = 1;
    }  
    PORTClearBits(IOPORT_G, PG_LCD_RW);
    
    PORTClearBits(IOPORT_E, PE_LCD_DATA);
    PORTSetBits(IOPORT_E, (byte)lcdFifo[lcdFifoRd]);
    // 300nsec high time for ECLK
    lcdFifoRd++;
    if (lcdFifoRd >= LCD_FIFO_SIZE) {
      lcdFifoRd = 0;
    }
    PORTSetBits(IOPORT_G, PG_LCD_E);
  }
} // LcdUpdate()


void LcdDrawState(void) {
  char line[21];
  MIRROR_t *pMirror;

  lcdRedraw  = 0;  
  switch (lcdState) {
    case LCD1_VERSION:
      lcdRedraw = 500;
      //LcdFifoString(1, 0, "MON dd, 20xx hr:mnam");
      LcdFifoString(1, 0, (char *)&lcdMonth[rtcMonth-1][0]);
      LcdDrawN(line, rtcDate, 2, 0);
      LcdFifoString(1, 4, line);
      line[0] = '0' + (rtcYear/10);
      line[1] = '0' + (rtcYear%10);
      line[2] = ' ';
      if (rtcHour == 0) {
        line[3] = '1';
        line[4] = '2';
      }
      else if (rtcHour <= 12) {
        LcdDrawN(line+3, rtcHour, 2, 0);
      }
      else {
        LcdDrawN(line+3, rtcHour-12, 2, 0);
      }
      line[5] = ':';
      line[6] = '0' + (rtcMin/10);
      line[7] = '0' + (rtcMin%10);
      if (rtcHour < 12) {
        line[8] = 'a';
      }
      else {
        line[8] = 'p';
      }  
      line[9] = '\0';
      LcdFifoString(1, 10, line);
      break;

    case LCD1_DEMO:
      lcdRedraw = 0xFFFF;
      break;

    case LCD2_DEMO_RATE:
      lcdRedraw = 65535;
      LcdDrawN(line, lcdDemoRate, 3, 0);
      LcdFifoString(2, 3, line);
      break;

    case LCD2_DEMO_PAUSE:
      lcdRedraw = 65535;
      LcdDrawN(line, lcdDemoPause, 6, 3);
      LcdFifoString(2, 3, line);
      break;

    case LCD2_DEMO_RUN:
      if (lcdDemoMode == LCD_DEMO_INIT) {
        if (string[0].state == STRING_ACTIVE) {
          if (string[0].maxAddr == 0) {
            LcdFifoString(2, 0, "Did not find any    ");
            LcdFifoString(3, 0, "MCT's on String A   ");
            lcdRedraw = 65535;
          }
          else {
            lcdRedraw = 0;
            line[0] = 0; // PARAM_STEP_RATE
            line[1] = (byte)(lcdDemoRate >> 8);
            line[2] = (byte)lcdDemoRate;
            Mct485SendMsg(MSG_ORIGIN_NORMAL,
                    0, // always string A
                    string[0].maxAddr + MCT485_ADDR_BCAST,
                    MCT_CMD_PARAM,
                    line);

            line[0] = 0x03;
            line[1] = 0xFF;
            line[2] = 0;
            line[3] = 0xFF;
            line[4] = 0;
            Mct485SendMsg(MSG_ORIGIN_NORMAL,
                    0, // always string A
                    string[0].maxAddr + MCT485_ADDR_BCAST,
                    MCT_CMD_TARG,
                    line);
            line[0] = 0x13;
            line[1] = 0xFF;
            line[2] = 0;
            line[3] = 0xFF;
            line[4] = 0;
            Mct485SendMsg(MSG_ORIGIN_NORMAL,
                    0, // always string A
                    string[0].maxAddr + MCT485_ADDR_BCAST,
                    MCT_CMD_TARG,
                    line);
            line[0] = 0x03;
            line[1] = (byte)(2495 >> 8);
            line[2] = (byte)2495;
            line[3] = (byte)(2495 >> 8);
            line[4] = (byte)2495;
            Mct485SendMsg(MSG_ORIGIN_NORMAL,
                    0, // always string A
                    string[0].maxAddr + MCT485_ADDR_BCAST,
                    MCT_CMD_POSN,
                    line);
            line[0] = 0x13;
            line[1] = (byte)(2495 >> 8);
            line[2] = (byte)2495;
            line[3] = (byte)(2495 >> 8);
            line[4] = (byte)2495;
            Mct485SendMsg(MSG_ORIGIN_NORMAL,
                    0, // always string A
                    string[0].maxAddr + MCT485_ADDR_BCAST,
                    MCT_CMD_POSN,
                    line);
            LcdFifoString(2, 0, "Moving to HOME      ");
            LcdFifoString(3, 0, "       degrees      ");
            lcdDemoMode = LCD_DEMO_HOME;
          }
        }
      }
      else if (lcdDemoMode == LCD_DEMO_HOME) {
        if ((string[0].mct[0].mirror[MIRROR1].sensors & SENSOR_HOME_A)
          && (string[0].mct[0].mirror[MIRROR2].sensors & SENSOR_HOME_A)) {
          lcdDemoMode = LCD_DEMO_HOME_PAUSE;
          lcdDemoCountdown = lcdDemoPause;
          LcdFifoString(2, 0, "At HOME, pausing    ");
          LcdFifoString(3, 0, "   seconds          ");
          LcdDrawN(line, lcdDemoCountdown/1000, 2, 0);
          LcdFifoString(3, 0, line);
        }
        else if ((string[0].mct[0].mirror[0].m[0].mdeg100 == 254)
          && (string[0].mct[0].mirror[0].m[0].mdeg100 == 254)) {
          lcdDemoMode = LCD_DEMO_HOME_PAUSE;
          lcdDemoCountdown = lcdDemoPause;
          LcdFifoString(2, 0, "At HOME, pausing    ");
          LcdFifoString(3, 0, "   seconds          ");
          LcdDrawN(line, lcdDemoCountdown/1000, 2, 0);
          LcdFifoString(3, 0, line);
        }
        else {
          LcdDrawN(line, string[0].mct[0].mirror[0].m[0].mdeg100, 6, 2);
          LcdFifoString(3, 0, line);
          lcdRedraw = 1000;
        }
      }
      else if (lcdDemoMode == LCD_DEMO_HOME_PAUSE) {
        lcdDemoCountdown--;
        if (!lcdDemoCountdown) {
          line[0] = 0x03;
          line[1] = (byte)(2495 >> 8);
          line[2] = (byte)2495;
          line[3] = (byte)(2495 >> 8);
          line[4] = (byte)2495;
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        0, // always string A
                        string[0].maxAddr + MCT485_ADDR_BCAST,
                        MCT_CMD_TARG,
                        line);
          line[0] = 0x13;
          line[1] = (byte)(2495 >> 8);
          line[2] = (byte)2495;
          line[3] = (byte)(2495 >> 8);
          line[4] = (byte)2495;
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        0, // always string A
                        string[0].maxAddr + MCT485_ADDR_BCAST,
                        MCT_CMD_TARG,
                        line);
          LcdFifoString(2, 0, "Moving to STOW      ");
          LcdFifoString(3, 0, "       degrees      ");
          LcdDrawN(line, string[0].mct[0].mirror[0].m[0].mdeg100, 6, 2);
          LcdFifoString(3, 0, line);
          lcdDemoMode = LCD_DEMO_STOW;
        }
        else if (!(lcdDemoCountdown % 1000)) {
          LcdDrawN(line, lcdDemoCountdown/1000, 2, 0);
          LcdFifoString(3, 0, line);
        }
      }  
      else if (lcdDemoMode == LCD_DEMO_STOW) {
        if (string[0].mct[0].mirror[MIRROR1].sensors & SENSOR_STOW_A) {
          lcdDemoMode = LCD_DEMO_STOW_PAUSE;
          lcdDemoCountdown = lcdDemoPause;
          LcdFifoString(2, 0, "At STOW, pausing    ");
          LcdFifoString(3, 0, "   seconds          ");
          LcdDrawN(line, lcdDemoCountdown/1000, 2, 0);
          LcdFifoString(3, 0, line);
        }
        else {
          LcdDrawN(line, string[0].mct[0].mirror[0].m[0].mdeg100, 6, 2);
          LcdFifoString(3, 0, line);
          lcdRedraw = 1000;
        }
      }
      else if (lcdDemoMode == LCD_DEMO_STOW_PAUSE) {
        lcdDemoCountdown--;
        if (!lcdDemoCountdown) {
          line[0] = 0x03;
          line[1] = 0xFF;
          line[2] = 0;
          line[3] = 0xFF;
          line[4] = 0;
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        0, // always string A
                        string[0].maxAddr + MCT485_ADDR_BCAST,
                        MCT_CMD_TARG,
                        line);
          line[0] = 0x13;
          line[1] = 0xFF;
          line[2] = 0;
          line[3] = 0xFF;
          line[4] = 0;
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        0, // always string A
                        string[0].maxAddr + MCT485_ADDR_BCAST,
                        MCT_CMD_TARG,
                        line);
          LcdFifoString(2, 0, "Moving to HOME      ");
          LcdFifoString(3, 0, "       degrees      ");
          LcdDrawN(line, string[0].mct[0].mirror[0].m[0].mdeg100, 6, 2);
          LcdFifoString(3, 0, line);
          lcdDemoMode = LCD_DEMO_HOME;
        }
        else if (!(lcdDemoCountdown % 1000)) {
          LcdDrawN(line, lcdDemoCountdown/1000, 2, 0);
          LcdFifoString(3, 0, line);
        }
        lcdRedraw = 0;
      }  
      break;

    case LCD1_STRINGS:
      lcdRedraw = 0xFFFF;
      break;

    case LCD1_MCT_STAT:
      lcdRedraw = 0xFFFF;
      break;

    case LCD2_MCT_STAT_MIR1:
      // "STR x MCT xx Mirror1"
      // "B xxx.xx  T xxx.xx  "
      // "xxx.xC xxx.xC xxx.xC"
      // "xxx.xC xxx.xC  xx.x%"
      pMirror = &string[lcdStringNum].mct[lcdMctNum-1].mirror[MIRROR1];
      LcdDrawN(line, pMirror->m[0].mdeg100, 6, 2);
      LcdFifoString(1, 2, line);
      LcdDrawN(line, pMirror->m[1].mdeg100, 6, 2);
      LcdFifoString(1, 12, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_1B], 5, 1);
      LcdFifoString(2, 0, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKRIGHT_1B], 5, 1);
      LcdFifoString(2, 7, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_MANIFOLD_1], 5, 1);
      LcdFifoString(2, 14, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_1A], 5, 1);
      LcdFifoString(3, 0, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_1A], 5, 1);
      LcdFifoString(3, 7, line);
      lcdRedraw = 1000;
      break;

    case LCD2_MCT_STAT_MIR2:
      // "STR x MCT xx Mirror1"
      // "B xxx.xx  T xxx.xx  "
      // "xxx.xC xxx.xC xxx.xC"
      // "xxx.xC xxx.xC  xx.x%"
      pMirror = &string[lcdStringNum].mct[lcdMctNum-1].mirror[MIRROR2];
      LcdDrawN(line, pMirror->m[0].mdeg100, 6, 2);
      LcdFifoString(1, 2, line);
      LcdDrawN(line, pMirror->m[1].mdeg100, 6, 2);
      LcdFifoString(1, 12, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_2B], 5, 1);
      LcdFifoString(2, 0, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKRIGHT_2B], 5, 1);
      LcdFifoString(2, 7, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_MANIFOLD_2], 5, 1);
      LcdFifoString(2, 14, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_2A], 5, 1);
      LcdFifoString(3, 0, line);
      LcdDrawRTD(line, string[lcdStringNum].mct[lcdMctNum-1].chan[MCT_RTD_TRACKLEFT_2A], 5, 1);
      LcdFifoString(3, 7, line);
      lcdRedraw = 1000;
      break;

    case LCD1_FCE:
      // DNI = x,xxx W/m2
      // IN REQ_HEAT
      // OUT PUMP_ON
      // IN FLOW_SW
      LcdDrawN(&line[6], fieldDNI, 4, 0);
      lcdRedraw = 1000;
      break;

    case LCD1_SYS30:
      // "Flow   = xxxxx l/hr"
      // "Power  = xxxxx W"
      // "INLET  = xxx.xC"
      // "OUTLET = xxx.xC"
      LcdDrawN(line, rtuData[1], 5, 0);
      LcdFifoString(0, 9, line);
      LcdDrawN(line, rtuData[0], 5, 0);
      LcdFifoString(1, 9, line);
      LcdDrawN(line, rtuData[2]/10, 4, 1);
      LcdFifoString(2, 9, line);
      LcdDrawN(line, rtuData[3]/10, 4, 1);
      LcdFifoString(3, 9, line);
      lcdRedraw = 1000;
      break;
  } // switch
} // LcdDrawState()


void LcdNewState(LCD_STATES_m newState) {
  char line[21];
  byte i;
  
  lcdRedraw = 0;
  switch (newState) {
    case LCD1_VERSION:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "     Chromasun");
      LcdFifoString(1, 0, "MON dd, 20xx hr:mnam");
      LcdFifoString(2, 0, "Solar Field Control ");
      LcdFifoString(3, 0, (char *)versString);
      break;
    
    case LCD1_DEMO:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Press DOWN to enter");
      LcdFifoString(2, 0, "     Demo Mode");
      lcdRedraw = 0xFFFF;
      break;

    case LCD2_DEMO_RATE:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Demo Step Rate");
      LcdFifoString(2, 0, "   xxx msec/step");
      LcdFifoString(3, 0, "UP/DOWN to change");
      StringSetState(0, STRING_POWER_OFF);
      break;

    case LCD2_DEMO_PAUSE:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Demo Pause");
      LcdFifoString(2, 0, "   xx.000 sec");
      LcdFifoString(3, 0, "UP/DOWN to change");
      break;

    case LCD2_DEMO_RUN:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Demo Running");
      LcdFifoString(2, 0, "Initializing");
      LcdFifoString(3, 0, "String A");
      StringSetState(0, STRING_POWER_UP);
      lcdDemoMode = LCD_DEMO_INIT;
      break;

    case LCD1_STRINGS:
      LcdFifoCmd(LCD_CMD_CLEAR);
      for (i = 0; i < 4; i++) {
        LcdFifoString(i, 0, "String x: ");
        line[0] = 'A' + i;
        line[1] = '\0';
        LcdFifoString(i, 7, line);
        if (string[i].state == STRING_POWER_OFF) {
          LcdFifoString(i, 10, "OFF");
        }
        else if (string[i].state == STRING_ACTIVE) {
          LcdFifoString(i, 10, "xx MCT's");
          LcdDrawN(line, string[i].maxAddr, 2, 0);
          LcdFifoString(i, 10, line);
        }
        else {
          LcdFifoString(i, 10, "Init");
        }
      }
      break;
    
    case LCD1_MCT_STAT:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "String x (UP change)");
      if (lcdStringNum >= NUM_STRINGS) {
        lcdStringNum = 0;
      }
      line[0] = 'A' + lcdStringNum;
      line[1] = '\0';
      LcdFifoString(0, 7, line);
      if (string[lcdStringNum].state == STRING_POWER_OFF) {
        LcdFifoString(2, 10, "OFF");
      }
      else if (string[lcdStringNum].state == STRING_ACTIVE) {
        LcdFifoString(2, 10, "xx MCT's");
        LcdDrawN(line, string[lcdStringNum].maxAddr, 2, 0);
        LcdFifoString(2, 10, line);
        LcdFifoString(3, 0, "DOWN to view MCT's");
      }
      else {
        LcdFifoString(2, 10, "Init");
      }
      lcdRedraw = 0xFFFF;
      break;

    case LCD2_MCT_STAT_MIR1:
      //                  "01234567890123456789"
      LcdFifoString(0, 0, "STR x MCT xx Mirror1");
      LcdFifoString(1, 0, "B xxx.xx  T xxx.xx  ");
      LcdFifoString(2, 0, "xxx.xC xxx.xC xxx.xC");
      LcdFifoString(3, 0, "xxx.xC xxx.xC  xx.x%");
      line[0] = 'A' + lcdStringNum;
      line[1] = '\0';
      LcdFifoString(0, 4, line);
      if (lcdMctNum > string[lcdStringNum].maxAddr) {
        lcdMctNum = 1;
      }
      LcdDrawN(line, lcdMctNum, 2, 0);
      LcdFifoString(0, 10, line);
      break;

    case LCD2_MCT_STAT_MIR2:
      //                  "012345678901223456789"
      LcdFifoString(0, 0, "STR x MCT xx Mirror2");
      LcdFifoString(1, 0, "B xxx.xx  T xxx.xx  ");
      LcdFifoString(2, 0, "xxx.xC xxx.xC xxx.xC");
      LcdFifoString(3, 0, "xxx.xC xxx.xC  xx.x%");
      line[0] = 'A' + lcdStringNum;
      line[1] = '\0';
      LcdFifoString(0, 4, line);
      if (lcdMctNum > string[lcdStringNum].maxAddr) {
        lcdMctNum = 1;
      }
      LcdDrawN(line, lcdMctNum, 2, 0);
      LcdFifoString(0, 10, line);
      break;

    case LCD1_FCE:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "DNI = x,xxx W/m2");
      break;
    
    case LCD1_SYS30:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Flow   = xxxxx l/hr");
      LcdFifoString(1, 0, "Power  = xxxxx W");
      LcdFifoString(2, 0, "INLET  = xxx.xC");
      LcdFifoString(3, 0, "OUTLET = xxx.xC");
      break;
    
    case LCD2_SYS30:
      LcdFifoCmd(LCD_CMD_CLEAR);
      LcdFifoString(0, 0, "Sys30 message");
      LcdFifoString(1, 0, "Rx size = ");
      LcdDrawN(line, rtu485RxIndex, 2, 0);
      LcdFifoString(1, 10, line);
      if (rtu485State == RTU485_IDLE)
        LcdFifoString(1, 14, "IDLE");
      else
        LcdFifoString(1, 14, "BUSY");
      for (i = 0; i < 20; i++) {
        line[i] = ' ';
      }
      line[20] = '\0';
      for (i = 0; i < 6 && i < rtu485RxIndex; i++) {
        LcdDrawHex2(line+i*3, rtu485RxBuffer[i]);
      }
      LcdFifoString(2, 0, line);
      for (i = 0; i < 20; i++) {
        line[i] = ' ';
      }
      line[20] = '\0';
      for (i = 6; i < 12 && i < rtu485RxIndex; i++) {
        LcdDrawHex2(line+i*3-18, rtu485RxBuffer[i]);
      }
      LcdFifoString(3, 0, line);
      break;
      
    default:
      return;
  } // switch
  
  lcdState = newState;
} // LcdNewState()




void LcdDrawRTD(char *s, long n, byte lj, byte dp) {
  if (n >= 10000) {
    *s++ = 'E';
    *s++ = 'R';
    *s++ = 'R';
    *s++ = '.';
    *s++ = '1';
    *s = '\0';
  }
  else {
    LcdDrawN(s, n, lj, dp);
  }
} // LcdDrawRTD()


void LcdDrawN(char *s, long n, byte lj, byte dp) {
  long whole;
  long fract;
  long tens;
  byte digits;
  byte neg;
  byte digit;
  byte i;
  
  if (dp >= lj) {
    return;
  }

  // adjust number of digits for neg numbers
  neg = 0;
  digits = 1;
  if (n < 0) {
    neg = 1;
    n *= -1;
    digits = 2;
  }
  if (dp) {
    // subtract off fraction and dp
    lj -= dp;
    lj--;
  }

  i = dp;
  tens = 1;
  while (i) {
    tens *= 10;
    i--;
  }
  fract = n % tens;
  whole = n / tens;
  
  tens = 1;
  while (tens*10 <= whole) {
    digits++;
    tens *= 10;
  }
  while (lj > digits) {
    *s++ = ' ';
    digits++;
  }
  if (neg) {
    *s++ = '-';
  }
  while (tens) {
    digit = whole / tens;
    *s++ = digit + '0';
    whole -= tens * digit;
    tens /= 10;
  }

  if (dp) {
    *s++ = '.';
    i = dp-1;
    tens = 1;
    while (i) {
      tens *= 10;
      i--;
    }
    while (tens) {
      digit = fract / tens;
      *s++ = digit + '0';
      fract -= tens * digit;
      tens /= 10;
    }
  }  

  *s = '\0';
} // LcdDrawN()


void LcdDrawHex2(char *s, byte n) {
  LcdDrawHex(s, n >> 4);
  s++;
  LcdDrawHex(s, n & 0x0F);
} // LcdDrawHex2()


void LcdDrawHex(char *s, byte n) {
  if (n >= 10)
    *s = n + 'A' - 10;
  else
    *s = n + '0';
} // LcdDrawHex()


void LcdFifoCmd(ushort cmd) {
  if ((lcdFifoRd - lcdFifoWr == 1) ||
      (lcdFifoWr == LCD_FIFO_SIZE-1 && !lcdFifoRd)) {
    // fifo is full
    return;
  }
  lcdFifo[lcdFifoWr] = cmd;
  lcdFifoWr++;
  if (lcdFifoWr >= LCD_FIFO_SIZE) {
    lcdFifoWr = 0;
  }
} // LcdFifoCmd()


void LcdFifoData(ushort data) {
  if ((lcdFifoRd - lcdFifoWr == 1) ||
      (lcdFifoWr == LCD_FIFO_SIZE-1 && !lcdFifoRd)) {
    // fifo is full
    return;
  }
  lcdFifo[lcdFifoWr] = 0x0100 | data;
  lcdFifoWr++;
  if (lcdFifoWr >= LCD_FIFO_SIZE) {
    lcdFifoWr = 0;
  }
} // LcdFifoData()


// Copy *s to LCD buffer
void LcdFifoString(byte row, byte col, char *s) {
  byte i;

  if (row & 0x02) row += 0x12;
  if (row & 0x01) row += 0x3F;
  LcdFifoCmd(0x80 + row + col);
  i = 0;
  while (s[i] != '\0') {
    LcdFifoData((byte)s[i++]);
  }
} // LcdFifoString()


void LcdButtonEvent(void) {
  switch (lcdButton) {
  case LCD_BUTTON_PREV:
    LcdNewState(lcdStatePrev[lcdState]);
    LcdButtonWaitClear();
    break;
    
  case LCD_BUTTON_NEXT:
    LcdNewState(lcdStateNext[lcdState]);
    LcdButtonWaitClear();
    break;
    
  case LCD_BUTTON_UP:
    switch (lcdState) {
    case LCD2_DEMO_RATE:
      if (lcdDemoRate < 1000) {
        lcdDemoRate += 5;
        lcdRedraw = 0;
      }
      break;

    case LCD2_DEMO_PAUSE:
      if (lcdDemoPause < 30000) {
        lcdDemoPause += 100;
        lcdRedraw = 0;
      }
      break;

    case LCD1_MCT_STAT:
      lcdStringNum++;
      if (lcdStringNum >= NUM_STRINGS) {
        lcdStringNum = 0;
      }  
      LcdNewState(LCD1_MCT_STAT);
      LcdButtonWaitClear();
      break;

    case LCD2_MCT_STAT_MIR1:
    case LCD2_MCT_STAT_MIR2:
      lcdMctNum++;
      if (lcdMctNum > string[lcdStringNum].maxAddr) {
        lcdMctNum = 1;
      }  
      LcdNewState(lcdState);
      LcdButtonWaitClear();
      break;

    default:
      LcdButtonWaitClear();
      break;
    } // switch (lcdState)
    break;
    
  case LCD_BUTTON_DOWN:
    switch (lcdState) {
    case LCD1_DEMO:
      LcdNewState(LCD2_DEMO_RATE);
      LcdButtonWaitClear();
      break;

    case LCD2_DEMO_RATE:
      if (lcdDemoRate > 10) {
        lcdDemoRate -= 5;
        lcdRedraw = 0;
      }
      break;

    case LCD2_DEMO_PAUSE:
      if (lcdDemoPause > 100) {
        lcdDemoPause -= 100;
      }
      else {
        lcdDemoPause = 0;
      }
      lcdRedraw = 0;
      break;

    case LCD1_MCT_STAT:
      if (string[lcdStringNum].maxAddr) {
        lcdMctNum = 1;
        LcdNewState(LCD2_MCT_STAT_MIR1);
      }  
      LcdButtonWaitClear();
      break;

    case LCD2_MCT_STAT_MIR1:
    case LCD2_MCT_STAT_MIR2:
      lcdMctNum--;
      if (!lcdMctNum) {
        lcdMctNum = string[lcdStringNum].maxAddr;
      }  
      LcdNewState(lcdState);
      LcdButtonWaitClear();
      break;

    case LCD1_SYS30:
    case LCD2_SYS30:
      LcdNewState(LCD2_SYS30);
      LcdButtonWaitClear();
      break;

    default:
      LcdButtonWaitClear();
      break;
    } // switch (lcdState)
    break;
  
  default:
    LcdButtonWaitClear();
    break;
  } // switch (lcdButton)
} // LcdButtonEvent()


void LcdButtonUpdate(void) {
  byte buttons;
  
  buttons = PORTReadBits(IOPORT_A, PA_PB_ALL);
  buttons = (~buttons) & PA_PB_ALL;
  switch (lcdButton) {
  case LCD_BUTTON_NONE:
    if (!buttons) {
    }
    else if (buttons == PA_PB_PREV) {
      lcdButton = LCD_BUTTON_PREV;
      LcdButtonEvent();
    }
    else if (buttons == PA_PB_DOWN) {
      lcdButton = LCD_BUTTON_DOWN;
      lcdButtonCountdown = LCD_BUTTON_SLOW;
      lcdButtonRepeat = 0;
      LcdButtonEvent();
    }
    else if (buttons == PA_PB_UP) {
      lcdButton = LCD_BUTTON_UP;
      lcdButtonCountdown = LCD_BUTTON_SLOW;
      lcdButtonRepeat = 0;
      LcdButtonEvent();
    }
    else if (buttons == PA_PB_NEXT) {
      lcdButton = LCD_BUTTON_NEXT;
      LcdButtonEvent();
    }
    else {
      // not a valid combination
      LcdButtonWaitClear();
    }
    break;

  case LCD_BUTTON_PREV:
    if (buttons != PA_PB_PREV) {
      LcdButtonWaitClear();
    }
    break;

  case LCD_BUTTON_DOWN:
    if (buttons != PA_PB_DOWN) {
      LcdButtonWaitClear();
    }
    else if (lcdButtonCountdown) {
      lcdButtonCountdown--;
    }
    else {
      // button is still down, repeat
      if (lcdButtonRepeat < 3) {
        lcdButtonRepeat++;
        lcdButtonCountdown = LCD_BUTTON_SLOW;
        LcdButtonEvent();
      }
      else {
        lcdButtonCountdown = LCD_BUTTON_FAST;
        LcdButtonEvent();
      }
    }
    break;

  case LCD_BUTTON_UP:
    if (buttons != PA_PB_UP) {
      LcdButtonWaitClear();
    }
    else if (lcdButtonCountdown) {
      lcdButtonCountdown--;
    }
    else {
      // button is still down, repeat
      if (lcdButtonRepeat < 3) {
        lcdButtonRepeat++;
        lcdButtonCountdown = LCD_BUTTON_SLOW;
        LcdButtonEvent();
      }
      else {
        lcdButtonCountdown = LCD_BUTTON_FAST;
        LcdButtonEvent();
      }
    }
    break;

  case LCD_BUTTON_NEXT:
    if (buttons != PA_PB_NEXT) {
      LcdButtonWaitClear();
    }
    break;

  case LCD_BUTTON_WAIT_CLEAR:
    if (!buttons) {
      // no buttons pressed
      if (lcdButtonCountdown) {
        lcdButtonCountdown--;
      }
      else {
        lcdButton = LCD_BUTTON_NONE;
      }
    }
    else {
      // one or more buttons still pressed
      lcdButtonCountdown = LCD_BUTTON_DEBOUNCE;
    }
  }
} // LcdButtonUpdate()


void LcdButtonWaitClear(void) {
  lcdButtonRepeat = 0;
  lcdButtonCountdown = LCD_BUTTON_DEBOUNCE;
  lcdButton = LCD_BUTTON_WAIT_CLEAR;
} // LcdButtonWaitClear()


const LCD_STATES_m lcdStatePrev[LCD_NUM_STATES] = {
  LCD1_SYS30,    // LCD1_VERSION,
  LCD1_VERSION,  // LCD1_DEMO,
  LCD1_DEMO,     // LCD2_DEMO_RATE,
  LCD1_DEMO,     // LCD2_DEMO_PAUSE,
  LCD2_DEMO_RATE,// LCD2_DEMO_RUN,
  LCD1_DEMO,     // LCD1_STRINGS,
  LCD1_STRINGS,  // LCD2_MCT_STAT,
  LCD1_MCT_STAT, // LCD2_MCT_STAT_MIR1,
  LCD1_MCT_STAT, // LCD2_MCT_STAT_MIR2,
  LCD1_MCT_STAT, // LCD1_FCE,
  LCD1_FCE,      // LCD1_SYS30,
  LCD1_SYS30     // LCD2_SYS30
};

const LCD_STATES_m lcdStateNext[LCD_NUM_STATES] = {
  LCD1_DEMO,       // LCD1_VERSION,
  LCD1_STRINGS,    // LCD1_DEMO,
  LCD2_DEMO_PAUSE, // LCD2_DEMO_RATE,
  LCD2_DEMO_RUN,   // LCD2_DEMO_PAUSE,
  LCD2_DEMO_RATE,  // LCD2_DEMO_RUN,
  LCD1_MCT_STAT,   // LCD1_STRINGS,
  LCD1_FCE,        // LCD2_MCT_STAT,
  LCD2_MCT_STAT_MIR2, // LCD2_MCT_STAT1,
  LCD2_MCT_STAT_MIR1,  // LCD2_MCT_STAT2,
  LCD1_SYS30,      // LCD1_FCE,
  LCD1_VERSION,    // LCD1_SYS30,
  LCD2_SYS30       // LCD2_SYS30
};


const char lcdMonth[12][4] = {
  "JAN",
  "FEB",
  "MAR",
  "APR",
  "MAY",
  "JUN",
  "JUL",
  "AUG",
  "SEP",
  "OCT",
  "NOV",
  "DEC"
};
