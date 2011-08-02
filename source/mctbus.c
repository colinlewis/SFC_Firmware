#include "sfc.h"
#include "field.h"
#include "mct485.h"
#include "mctbus.h"


void StringInit(void) {
  byte i;
  
  // turn off all power supplies
  PORTSetBits(IOPORT_G, PG_PS_EN_A);
  PORTSetBits(IOPORT_C, PC_PS_EN_B |
                        PC_PS_EN_C |
                        PC_PS_EN_D);
  // init 24V power pins
  PORTSetPinsDigitalOut(IOPORT_G, PG_PS_EN_A);
  PORTSetPinsDigitalOut(IOPORT_C, PC_PS_EN_B |
                                  PC_PS_EN_C |
                                  PC_PS_EN_D);
  
  // init MCT 24V pins
  PORTSetPinsDigitalOut(IOPORT_D, PD_POWER_EN_A |
                                  PD_POWER_EN_B |
                                  PD_POWER_EN_C |
                                  PD_POWER_EN_D);
  // All off for now
  PORTClearBits(IOPORT_D, PD_POWER_EN_A |
                          PD_POWER_EN_B |
                          PD_POWER_EN_C |
                          PD_POWER_EN_D);
  
  // init 485 transceiver I/O
  PORTSetBits(IOPORT_C, PC_MCT_RX_ENN_B);
  PORTClearBits(IOPORT_C, PC_MCT_TX_EN_B);
  PORTSetPinsDigitalOut(IOPORT_C, PC_MCT_RX_ENN_B |
                                  PC_MCT_TX_EN_B);
  PORTSetBits(IOPORT_D, PD_MCT_RX_ENN_A |
                        PD_MCT_RX_ENN_C |
                        PD_MCT_RX_ENN_D);
  PORTClearBits(IOPORT_D, PD_MCT_TX_EN_A |
                          PD_MCT_TX_EN_C |
                          PD_MCT_TX_EN_D);
  PORTSetPinsDigitalOut(IOPORT_D, PD_MCT_RX_ENN_A |
                                  PD_MCT_TX_EN_A |
                                  PD_MCT_RX_ENN_C |
                                  PD_MCT_TX_EN_C |
                                  PD_MCT_RX_ENN_D |
                                  PD_MCT_TX_EN_D);

  // Init the globals  
  for (i = 0; i < 4; i++) {
    string[i].state = STRING_POWER_OFF;
    string[i].maxAddr = 0;
  }
  string[0].port_PS_En = IOPORT_G;
  string[0].mask_PS_En = PG_PS_EN_A;
  string[1].port_PS_En = IOPORT_C;
  string[1].mask_PS_En = PC_PS_EN_B;
  string[2].port_PS_En = IOPORT_C;
  string[2].mask_PS_En = PC_PS_EN_C;
  string[3].port_PS_En = IOPORT_C;
  string[3].mask_PS_En = PC_PS_EN_D;
  string[0].port_PowerEn = IOPORT_D;
  string[0].mask_PowerEn = PD_POWER_EN_A;
  string[1].port_PowerEn = IOPORT_D;
  string[1].mask_PowerEn = PD_POWER_EN_B;
  string[2].port_PowerEn = IOPORT_D;
  string[2].mask_PowerEn = PD_POWER_EN_C;
  string[3].port_PowerEn = IOPORT_D;
  string[3].mask_PowerEn = PD_POWER_EN_D;
  string[0].port_RxEnn = IOPORT_D;
  string[0].mask_RxEnn = PD_MCT_RX_ENN_A;
  string[1].port_RxEnn = IOPORT_C;
  string[1].mask_RxEnn = PC_MCT_RX_ENN_B;
  string[2].port_RxEnn = IOPORT_D;
  string[2].mask_RxEnn = PD_MCT_RX_ENN_C;
  string[3].port_RxEnn = IOPORT_D;
  string[3].mask_RxEnn = PD_MCT_RX_ENN_D;
  string[0].port_TxEn = IOPORT_D;
  string[0].mask_TxEn = PD_MCT_TX_EN_A;
  string[1].port_TxEn = IOPORT_C;
  string[1].mask_TxEn = PC_MCT_TX_EN_B;
  string[2].port_TxEn = IOPORT_D;
  string[2].mask_TxEn = PD_MCT_TX_EN_C;
  string[3].port_TxEn = IOPORT_D;
  string[3].mask_TxEn = PD_MCT_TX_EN_D;
  
  stringActive = 0;
} // StringInit()


void StringUpdate(void) {
  byte data[2];
  STRING_t *pString;
  
  pString = &string[stringActive];
  switch (pString->state) {
  case STRING_POWER_OFF:
    // nothing to do
    break;

  case STRING_POWER_UP:
    if (pString->powerupDelay == 100) {
      PORTSetBits(pString->port_PowerEn,
                  pString->mask_PowerEn);
      pString->powerupDelay--;
    }
    else if (!pString->powerupDelay) {
      StringSetState(stringActive, STRING_INIT_PING0);
      // exit to keep this string active
      return;
    }
    else {
      pString->powerupDelay--;
    }  
    break;

  case STRING_INIT_PING0:
    // wait for RS485 response to kick to next state
    StringKeepAliveBroadcast();
    return;

  case STRING_INIT_SET_ADDR:
    // wait for RS485 response to kick to next state
    StringKeepAliveBroadcast();
    return;

  case STRING_INIT_SET_DIR:
    // wait for RS485 response to kick to next state
    StringKeepAliveBroadcast();
    return;

  case STRING_INIT_JUMP_TO_APP:
    // wait for RS485 response to kick to next state
    StringKeepAliveBroadcast();
    return;

  case STRING_INIT_GET_INFO:
    if (pString->powerupDelay) {
      pString->powerupDelay--;
      return;
    }
    // Success, get info then try next node
    Mct485SendMsg(MSG_ORIGIN_NORMAL,
                  stringActive,
                  pString->maxAddr,
                  MCT_CMD_SERNUM,
                  0);
    Mct485SendMsg(MSG_ORIGIN_NORMAL,
                  stringActive,
                  pString->maxAddr,
                  MCT_CMD_VERSION,
                  0);
    data[0] = 0;
    Mct485SendMsg(MSG_ORIGIN_NORMAL,
                  stringActive,
                  pString->maxAddr,
                  MCT_CMD_STRING,
                  data);
    stringInitRetry = 0;
    data[0] = 1;
    Mct485SendMsg(MSG_ORIGIN_NORMAL,
                  stringActive,
                  pString->maxAddr,
                  MCT_CMD_STRING,
                  data);
    stringInitRetry = 0;
    StringSetState(stringActive, STRING_ACTIVE);
    return;

  case STRING_ACTIVE:
    if (pString->maxAddr) {
      // periodically poll
      if (fieldState == FIELD_TEST_UPDATE) {
        // don't poll
      }
      else if (pString->pollDelay && mct485TxQueueRd != mct485TxQueueWr) {
        // if the queue is empty, poll immediately, else wait
        pString->pollDelay--;
      }
      else {
        pString->pollDelay = 50;
        if (pString->pollMct > pString->maxAddr) {
          pString->pollMct = 1;
        }
        if (pString->pollType == MCT_POLL_OFF) {
        }
        else if (pString->pollType == MCT_POLL_CHAN) {
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        stringActive,
                        pString->pollMct,
                        MCT_CMD_GET_CHAN,
                        0);
          pString->pollType = MCT_POLL_MIRRORS;
        }
        else {
          Mct485SendMsg(MSG_ORIGIN_NORMAL,
                        stringActive,
                        pString->pollMct,
                        MCT_CMD_GET_MIRRORS,
                        0);
          pString->pollType = MCT_POLL_CHAN;
          pString->pollMct++;
        }
      }
    }
    else {
      StringSetState(stringActive, STRING_POWER_OFF);
    }
    break;

  case STRING_REPING:
    // wait for RS485 response to kick to next state
    StringKeepAliveBroadcast();
    return;

  default:
    return;
  } // switch

  stringActive++;
  if (stringActive >= NUM_STRINGS) {
    stringActive = 0;
  }
} // StringUpdate()


void StringSetState(byte stringNum, STRING_STATE_m newState) {
  byte data[2];
  STRING_t *pString;
  
  pString = &string[stringNum];
  switch (newState) {
  case STRING_POWER_OFF:
    PORTSetBits(pString->port_PS_En,
                pString->mask_PS_En);
    PORTClearBits(pString->port_PowerEn,
                  pString->mask_PowerEn);
    string[stringNum].maxAddr = 0;
    stringInitReset = 0;
    break;

  case STRING_POWER_UP:
    // turn on power supply
    PORTClearBits(pString->port_PS_En,
                  pString->mask_PS_En);
    // initially clear bits to force reset
    PORTClearBits(pString->port_PowerEn,
                  pString->mask_PowerEn);
    string[stringNum].maxAddr = 0;
    pString->powerupDelay = 500;
    break;

  case STRING_INIT_PING0:
    Mct485SendMsg(MSG_ORIGIN_INIT, stringNum, 0, MCT_CMD_NULL, data);
    break;

  case STRING_INIT_SET_ADDR:
    string[stringNum].maxAddr++;
    data[0] = string[stringNum].maxAddr;
    Mct485SendMsg(MSG_ORIGIN_INIT, stringNum, 0, MCT_CMD_SET_ADDR, data);
    break;

  case STRING_INIT_SET_DIR:
    data[0] = mctInitDir;
    Mct485SendMsg(MSG_ORIGIN_INIT, stringNum, pString->maxAddr, MCT_CMD_SET_DIR, data);
    break;

  case STRING_INIT_JUMP_TO_APP:
    Mct485SendMsg(MSG_ORIGIN_INIT, stringNum, pString->maxAddr, MCT_CMD_JUMP_TO_APP, data);
    break;

  case STRING_INIT_GET_INFO:
    pString->powerupDelay = 100;
    break;

  case STRING_ACTIVE:
    pString->pollType = MCT_POLL_CHAN;
    break;

  case STRING_REPING:
    Mct485SendMsg(MSG_ORIGIN_INIT, stringNum, pString->maxAddr, MCT_CMD_NULL, data);
    break;

  default:
    return;
  } // switch
  
  pString->state = newState;
} // StringSetState()


byte StringInitResp(void) {
  byte mctRespValid;
  byte stringNum;
  STRING_t *pString;
  byte data[2];
  
  mctRespValid = Mct485RespValid();
  stringNum = mct485String[mct485TxQueueRd];
  pString = &string[stringNum];
  
  if (pString->state == STRING_INIT_PING0) {
    if (mctRespValid) {
      stringInitRetry = 0;
      StringSetState(stringNum, STRING_INIT_SET_ADDR);
    }
    else if (mct485TxRetry < MCT485_RETRY_LIMIT) {
      // resend msg
      return 0;
    }
    else {  
      // failed 3 times, quit
      StringSetState(stringNum, STRING_INIT_GET_INFO);
      return 0;
    }
  } // PING0  
  else if (pString->state == STRING_INIT_SET_ADDR) {
    if (mctRespValid) {
      // MCT responded to set addr, find direction
      mctInitDir = MCT485_FC_LEFT;
      stringInitRetry = 0;
      StringSetState(stringNum, STRING_INIT_SET_DIR);
    }
    else {
      // retry setting address
      if (mct485TxRetry < MCT485_RETRY_LIMIT) {
        // resend msg
        return 0;
      }
      else {
        // XYZ, initial response was probably garbled, try to continue
        mctInitDir = MCT485_FC_LEFT;
        stringInitRetry = 0;
        StringSetState(stringNum, STRING_INIT_SET_DIR);
      }  
    }
  }   // SET_ADDR
  else if (pString->state == STRING_INIT_SET_DIR) {
    if (mctRespValid) {
      // set dir was successful, verify communication at new address
      stringInitRetry = 0;
      StringSetState(stringNum, STRING_INIT_JUMP_TO_APP);
    }  
    else if (++stringInitRetry < 6) {
      // flip direction
      if (mctInitDir == MCT485_FC_LEFT) {
        mctInitDir = MCT485_FC_RIGHT;
      }
      else {
        mctInitDir = MCT485_FC_LEFT;
      }
      // try again
      StringSetState(stringNum, STRING_INIT_SET_DIR);
      // pull last msg from queue
      return 1;
    }
    else {
      // reset string and try again?
      string[stringNum].maxAddr--;
      stringInitReset++;
      if (stringInitReset < 3) {
        stringInitRetry = 0;
        StringSetState(stringNum, STRING_INIT_PING0);
      }
      else {
        // Post Error???
        StringSetState(stringNum, STRING_INIT_GET_INFO);
      }  
      return 0;
    }
  } // SET_DIR 
  else if (pString->state == STRING_INIT_JUMP_TO_APP) {
    if (mctRespValid) {
      StringSetState(stringNum, STRING_INIT_PING0);
    }  
    else if (mct485TxRetry < MCT485_RETRY_LIMIT) {
      // resend msg
      return 0;
    }
    else {
      // MCT's in boot mode don't always respond quickly
      StringSetState(stringNum, STRING_REPING);
    }
  }  
  else if (pString->state == STRING_REPING) {
    if (mctRespValid) {
      StringSetState(stringNum, STRING_INIT_PING0);
    }  
    else if (mct485TxRetry < MCT485_RETRY_LIMIT) {
      // resend msg
      return 0;
    }
    else {
      string[stringNum].maxAddr--;
      // Post Error???
      StringSetState(stringNum, STRING_INIT_GET_INFO);
      // remove msg from queue
      return 1;
    }  
  } // JUMP_TO_APP
  
  return 1;
} // StringInitResp()


void StringKeepAliveBroadcast(void) {
  byte i;
  STRING_t *pString;

return;  
  for (i = 0; i < 4; i++) {
    pString = &string[i];
    if (pString->state == STRING_ACTIVE) {
      if (pString->pollDelay) {
        pString->pollDelay--;
      }
      else {
        pString->pollDelay = 50;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      pString->maxAddr | MCT485_ADDR_BCAST,
                      MCT_CMD_NULL,
                      0);
      }                
    } // STRING_ACTIVE
  } // for ()
} // StringKeepAliveBroadcast()
