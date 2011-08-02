#include "sfc.h"
#include "mct485.h"
#include "mctbus.h"


void Mct485Init(void) {
  mct485TxQueueRd = 0;
  mct485TxQueueWr = 0;
  mct485RxIndex = 0;
  mct485TxIndex = 0;
  mct485TxLen = 0;
  mct485RxTimeout = 0;
  mct485TxRetry = 0;
  mct485State = MCT485_IDLE;
  // turn off UART to clear state
  UARTEnable(UART2A, 0);
  // pull-up for MCT_RX
  CNPUE = 0x200;
	// PBCLK = 72MHz
  UARTConfigure(UART2A, UART_ENABLE_PINS_TX_RX_ONLY);
  UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_RX_NOT_EMPTY);
  UARTSetLineControl(UART2A, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
  UARTSetDataRate(UART2A, 72000000L, 115200L);
  UARTEnable(UART2A, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

	// Configure UART2 RX Interrupt
	INTEnable(INT_SOURCE_UART_RX(UART2A), INT_ENABLED);
  INTSetVectorPriority(INT_VECTOR_UART(UART2A), INT_PRIORITY_LEVEL_2);
  INTSetVectorSubPriority(INT_VECTOR_UART(UART2A), INT_SUB_PRIORITY_LEVEL_0);
} // Mct485Init()


void Mct485Update(void) {
  if (mct485State == MCT485_IDLE) {
    if (usb485TxLen != 0) {
      // move this message to the front of the line
      Mct485PriorityMsg(MSG_ORIGIN_USB,
                        usb485TxString,
                        usb485TxLen,
                        usb485TxBuffer);
      usb485TxLen = 0;
    }
    if (mct485TxQueueRd != mct485TxQueueWr) {
//      while (UARTReceivedDataIsAvailable(UART2A)) {
      while (U2ASTA & 0x01) {
        mct485Flush = (byte)U2ARXREG;//UARTGetDataByte(UART2A);
      }
      // Tx queue is not empty, send next message
      Mct485SendNextMsg();
    }
  } // MCT485_IDLE
  else if (mct485State == MCT485_TX) {
    // wait for Tx to complete, changes to RX in ISR
  }
  else {
    mct485RxTimeout--;
    if (!mct485RxTimeout) {
      // turn off receiver
      PORTSetBits(string[mct485String[mct485TxQueueRd]].port_RxEnn,
                  string[mct485String[mct485TxQueueRd]].mask_RxEnn);
      if (Mct485ProcessResp()) {
        Mct485AdvRdQueue();
      }
      else {
        // XYZ add retries
        if (++mct485TxRetry <= MCT485_RETRY_LIMIT) {
          Mct485SendNextMsg();
        }
        else {
          // 3 failures in a row
          Mct485AdvRdQueue();
        }
      } // ProcessResp failed
    }
    else if (mct485RxIndex) {
      // started receiving
      if (UARTGetLineStatus(UART2A) & UART_RECEIVER_IDLE) {
        // line went idle, turn off receiver
        PORTSetBits(string[mct485String[mct485TxQueueRd]].port_RxEnn,
                    string[mct485String[mct485TxQueueRd]].mask_RxEnn);
        // flush receiver
        while (UARTReceivedDataIsAvailable(UART2A)) {
          if (mct485RxIndex < MCT485_MAX_INDEX) {
            mct485RxBuffer[mct485RxIndex++] = UARTGetDataByte(UART2A);
          }
          else {
            mct485RxBuffer[MCT485_MAX_INDEX-1] = UARTGetDataByte(UART2A);
          }
        }
        // process response
        if (Mct485ProcessResp()) {
          Mct485AdvRdQueue();
        }
        else {
          // XYZ add retries
          if (++mct485TxRetry <= 3) {
            Mct485SendNextMsg();
          }
          else {
            // 3 failures in a row
            Mct485AdvRdQueue();
          }
        } // ProcessResp failed
      } // IDLE
    } // receiving
  } // if (MCT485_RX)
} // Mct485Update()


void Mct485SendMsg(byte origin, byte stringNum, byte addr, byte cmd, byte *data) {
  ushort crc;
  byte len;
  byte i;
  
  switch (cmd) {
  case MCT_CMD_NULL:
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_JUMP_TO_APP:  // 0x02
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_ERASE_APP:    // 0x03
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_BLANK_CHECK:  // 0x04
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_PROG_APP:     // 0x05
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 9;
    // need to set addr, len, and data
    break;

  case MCT_CMD_READ_FLASH:   // 0x06
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 9;
    // need to set addr and len to read
    break;

  case MCT_CMD_FLASH_CKSUM:  // 0x07
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_JUMP_TO_BOOT: // 0x08
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_SET_ADDR:
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;
    
  case MCT_CMD_SET_DIR:
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;
    
  case MCT_CMD_SERNUM:      // 0x13
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_VERSION:     // 0x14
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_LOG:         // 0x15
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_TIME:        // 0x16
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_STRING:      // 0x17
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;

  case MCT_CMD_POSN:        // 0x20
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 11;
    for (i = 0; i < 5; i++) {
      mct485TxQueue[mct485TxQueueWr][MCT485_DATA+i] = data[i];
    }  
    break;

  case MCT_CMD_TARG:        // 0x21
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 11;
    for (i = 0; i < 5; i++) {
      mct485TxQueue[mct485TxQueueWr][MCT485_DATA+i] = data[i];
    }  
    break;

  case MCT_CMD_ADJ_TARG:    // 0x22
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 11;
    for (i = 0; i < 5; i++) {
      mct485TxQueue[mct485TxQueueWr][MCT485_DATA+i] = data[i];
    }  
    break;

  case MCT_CMD_RATE:        // 0x23
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_GET_SENSORS: // 0x30
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_GET_AD:      // 0x31
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_GET_RTD:     // 0x32
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_GET_CHAN:    // 0x33
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;
  
  case MCT_CMD_TRACK:       // 0x40
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 8;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+1] = data[1];
    break;

  case MCT_CMD_PARAM:       // 0x41
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 9;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+1] = data[1];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+2] = data[2];
    break;

  case MCT_CMD_GET_MIRRORS: // 0x42
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 6;
    break;

  case MCT_CMD_POWER:       // 0x43
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 9;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+1] = data[1];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+2] = data[2];
    break;

  case MCT_CMD_ALTERNATE:   // 0x44
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 8;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA+1] = data[1];
    break;

  case MCT_RESP_POSN:        // 0x20
    // strip off response bit
    cmd &= 0x7F;
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;

  case MCT_RESP_TARG:        // 0x21
    // strip off response bit
    cmd &= 0x7F;
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;

  case MCT_RESP_ADJ_TARG:    // 0x22
    // strip off response bit
    cmd &= 0x7F;
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;
  
  case MCT_RESP_TRACK:       // 0x40
    // strip off response bit
    cmd &= 0x7F;
    mct485TxQueue[mct485TxQueueWr][MCT485_LEN] = 7;
    mct485TxQueue[mct485TxQueueWr][MCT485_DATA] = data[0];
    break;

  default:
    return;
  } // switch(cmd)
  
  mct485TxQueue[mct485TxQueueWr][MCT485_ADDR] = addr;
  mct485TxQueue[mct485TxQueueWr][MCT485_CMD] = cmd;
  mct485TxQueue[mct485TxQueueWr][MCT485_MSG_ID] = mct485MsgID++;
  len = mct485TxQueue[mct485TxQueueWr][MCT485_LEN]-2;
  crc = CrcCalc(len, mct485TxQueue[mct485TxQueueWr]);
  mct485TxQueue[mct485TxQueueWr][len] = (byte)(crc >> 8);
  mct485TxQueue[mct485TxQueueWr][len+1] = (byte)crc;
  mct485String[mct485TxQueueWr] = stringNum;
  mct485Origin[mct485TxQueueWr] = origin;
  mct485TxQueueWr++;
  if (mct485TxQueueWr >= MCT485_QUEUE_SIZE) {
    mct485TxQueueWr = 0;
  }  
} // Mct485SendMsg()


void Mct485CannedMsg(byte origin, byte stringNum, byte len, byte *data) {
  byte *queue;
  
  mct485String[mct485TxQueueWr] = stringNum;
  mct485Origin[mct485TxQueueWr] = origin;
  queue = mct485TxQueue[mct485TxQueueWr];
  while (len--) {
    *queue++ = *data++;
  }
  mct485TxQueueWr++;
  if (mct485TxQueueWr >= MCT485_QUEUE_SIZE) {
    mct485TxQueueWr = 0;
  }  
} // Mct485CannedMsg()


void Mct485PriorityMsg(byte origin, byte stringNum, byte len, byte *data) {
  byte *queue;
  
  if (mct485TxQueueRd == 0) {
    mct485TxQueueRd = MCT485_QUEUE_SIZE-1;
  }  
  else {
    mct485TxQueueRd--;
  }
  mct485String[mct485TxQueueRd] = stringNum;
  mct485Origin[mct485TxQueueRd] = origin;
  queue = mct485TxQueue[mct485TxQueueRd];
  while (len--) {
    *queue++ = *data++;
  }
} // Mct485CannedMsg()


void Mct485SendNextMsg(void) {
  mct485State = MCT485_TX;
  mct485TxLen = mct485TxQueue[mct485TxQueueRd][MCT485_LEN];
  mct485TxIndex = 0;
  PORTSetBits(string[mct485String[mct485TxQueueRd]].port_TxEn,
              string[mct485String[mct485TxQueueRd]].mask_TxEn);
  INTDisableInterrupts();
  UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
  while (mct485TxIndex < mct485TxLen && 
    UARTTransmitterIsReady(UART2A)) {
    UARTSendDataByte(UART2A, mct485TxQueue[mct485TxQueueRd][mct485TxIndex++]);
  }
  if (mct485TxIndex < mct485TxLen) {
    UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
  }
  INTClearFlag(INT_SOURCE_UART_TX(UART2A));
  INTEnable(INT_SOURCE_UART_TX(UART2A), INT_ENABLED);
  INTEnableInterrupts();
} // Mct485SendNextMsg()


byte Mct485RespValid(void) {
  if (mct485RxIndex < MCT485_MIN_LEN) {
    // message is too short
    return 0;
  }
  else if (mct485RxIndex != mct485RxBuffer[MCT485_LEN]) {
    // message length doesn't match number of bytes received
    return 0;
  }
  else if (CrcCalc(mct485RxIndex, mct485RxBuffer)) {
    // non-zero CRC indicates failure
    return 0;
  }
  else if (!(mct485RxBuffer[MCT485_ADDR] & 0x80)) {
    // message is not addressed to the FC
    return 0;
  }
  else if (mct485RxBuffer[MCT485_MSG_ID] != mct485TxQueue[mct485TxQueueRd][MCT485_MSG_ID]) {
    // response MSG_ID does not match command MSG_ID
    return 0;
  }
  
  return 1;
} // Mct485RespValid()


void Mct485AdvRdQueue(void) {
  mct485TxQueueRd++;
  if (mct485TxQueueRd >= MCT485_QUEUE_SIZE) {
    mct485TxQueueRd = 0;
  }
  mct485TxRetry = 0;
  mct485State = MCT485_IDLE;
}

byte Mct485ProcessResp(void) {
  byte i;
  MCT_t *pMct;
  
  if (mct485Origin[mct485TxQueueRd] == MSG_ORIGIN_INIT) {
    // This response is part of a String Init sequence
    return StringInitResp();
  }
  else if (mct485Origin[mct485TxQueueRd] == MSG_ORIGIN_USB) {
    // Forward the response the to the USB response queue
  
    if (!Mct485RespValid()) {
      return 0;
    }
    Usb485Resp();
    return 1;
  }

  
  if (!Mct485RespValid()) {
    return 0;
  }
  
  if ((mct485RxBuffer[MCT485_ADDR] & 0x1F) >= 1 &&
      (mct485RxBuffer[MCT485_ADDR] & 0x1F) <= NUM_MCT) {
    pMct = &string[mct485String[mct485TxQueueRd]].mct[(mct485RxBuffer[MCT485_ADDR] & 0x1F)-1];
  }
  else {
    pMct = (MCT_t *)0;
  }

  switch (mct485RxBuffer[MCT485_CMD]) {
  case MCT_RESP_NULL:       // 0x80
    break;

  case MCT_RESP_BAD_MSG:    // 0x81
    break;

  case MCT_RESP_SET_ADDR:
    if (mct485RxBuffer[MCT485_LEN] == 7) {
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_SET_DIR:
    if (mct485RxBuffer[MCT485_LEN] == 8) {
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_JUMP_TO_APP: // 0x82
    break;

  case MCT_RESP_ERASE_APP:   // 0x83
    break;

  case MCT_RESP_BLANK_CHECK: // 0x84
    break;

  case MCT_RESP_PROG_APP:    // 0x85
    break;

  case MCT_RESP_READ_FLASH:  // 0x86
    break;

  case MCT_RESP_FLASH_CKSUM: // 0x87
    break;

  case MCT_RESP_SERNUM:     // 0x93
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 15) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      pMct->serNum = Mct485UnpackLong(&mct485RxBuffer[MCT485_DATA]+1);
      pMct->slaveSerNum = Mct485UnpackLong(&mct485RxBuffer[MCT485_DATA]+5);
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_VERSION:    // 0x94
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 13) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      pMct->fwType = mct485RxBuffer[MCT485_DATA+1];
      pMct->fwMajor = mct485RxBuffer[MCT485_DATA+2];
      pMct->fwMinor = mct485RxBuffer[MCT485_DATA+3];
      pMct->slaveFwType = mct485RxBuffer[MCT485_DATA+4];
      pMct->slaveFwMajor = mct485RxBuffer[MCT485_DATA+5];
      pMct->slaveFwMinor = mct485RxBuffer[MCT485_DATA+6];
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_LOG:        // 0x95
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 11) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      // log entry
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_TIME:       // 0x96
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 17) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      // not transmitting MSB for sysSec, zero out prev byte
      mct485RxBuffer[MCT485_DATA] = 0;
      pMct->sysSec = Mct485UnpackLong(&mct485RxBuffer[MCT485_DATA]);
      pMct->sysTicks = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
      // not transmitting MSB for sysSec, zero out prev byte
      mct485RxBuffer[MCT485_DATA+5] = 0;
      pMct->slaveSysSec = Mct485UnpackLong(&mct485RxBuffer[MCT485_DATA+5]);
      pMct->slaveSysTicks = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+9]);
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_STRING:     // 0x97
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 48) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      if (mct485RxBuffer[MCT485_DATA+1] == 0) {
        for (i = 0; i < 40; i++) {
          pMct->versionStr[i] = mct485RxBuffer[MCT485_DATA+2 + i];
        }
      }
      else
      {
        for (i = 0; i < 40; i++) {
          pMct->slaveVersionStr[i] = mct485RxBuffer[MCT485_DATA+2 + i];
        }
      }
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_POSN:       // 0xA0
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 12) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      if (mct485RxBuffer[MCT485_DATA+1] == 0x03) {
        // Mirror 1
        pMct->mirror[MIRROR1].m[MOTOR_A].posn = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+2]);
        pMct->mirror[MIRROR1].m[MOTOR_B].posn = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
      }
      else if (mct485RxBuffer[MCT485_DATA+1] == 0x13) {
        // Mirror 2
        pMct->mirror[MIRROR2].m[MOTOR_A].posn = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+2]);
        pMct->mirror[MIRROR2].m[MOTOR_B].posn = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
      }
      else {
        // Invalid motor ID
      }
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_TARG:       // 0xA1
  case MCT_RESP_ADJ_TARG:   // 0xA2
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 12) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      if (mct485RxBuffer[MCT485_DATA+1] == 0x03) {
        // Mirror 1
        pMct->mirror[MIRROR1].m[MOTOR_A].targ = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+2]);
        pMct->mirror[MIRROR1].m[MOTOR_B].targ = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
      }
      else if (mct485RxBuffer[MCT485_DATA+1] == 0x13) {
        // Mirror 2
        pMct->mirror[MIRROR2].m[MOTOR_A].targ = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+2]);
        pMct->mirror[MIRROR2].m[MOTOR_B].targ = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
      }
      else {
        // Invalid motor ID
      }
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_GET_SENSORS: // 0xB0
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 9) {
        pMct->stat = mct485RxBuffer[MCT485_DATA];
        // Mirror 1
        pMct->mirror[MIRROR1].sensors = mct485RxBuffer[MCT485_DATA+1];
        pMct->mirror[MIRROR2].sensors = mct485RxBuffer[MCT485_DATA+2];
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_GET_AD:   // 0xB1
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 12) {
      // not using raw A/D values
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_GET_RTD:   // 0xB2
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 12) {
      pMct->chan[mct485RxBuffer[MCT485_DATA+1] >> 4] = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+2]);
      pMct->chan[mct485RxBuffer[MCT485_DATA+1] & 0x0F] = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+4]);
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_GET_CHAN:  // 0xB3
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 39) {
      for (i = 0; i < 16; i++) {
        pMct->chan[i] = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+1+2*i]);
      }  
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_SETTING2:   // 0xA8
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 6) {
    }
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_TRACK:      // 0xC0
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 9) {
      pMct->stat = mct485RxBuffer[MCT485_DATA];
      if (mct485RxBuffer[MCT485_DATA+1] == MIRROR1) {
        pMct->mirror[MIRROR1].tracking = mct485RxBuffer[MCT485_DATA+2];
      } // if (MIRROR1)
      else if (mct485RxBuffer[MCT485_DATA+1] == MIRROR2) {
        pMct->mirror[MIRROR2].tracking = mct485RxBuffer[MCT485_DATA+2];
      } // else if (MIRROR2)
      else {
        // illegal value
      }
    } // if (mct485RxBuffer[MCT485_LEN] == 9)
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_PARAM:      // 0xC1
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 10) {
      if (mct485RxBuffer[MCT485_DATA+1] < 10) {
      }
    } // if (mct485RxBuffer[MCT485_LEN] == 10)
    else {
      // invalid response length
    }
    break;

  case MCT_RESP_GET_MIRRORS:
    if (!pMct) {
      // not an initialized MCT address
    }
    else if (mct485RxBuffer[MCT485_LEN] == 19) {
      pMct->mirror[MIRROR1].tracking = mct485RxBuffer[MCT485_DATA+1];
      pMct->mirror[MIRROR1].sensors = mct485RxBuffer[MCT485_DATA+2];
      pMct->mirror[MIRROR1].m[MOTOR_A].mdeg100 = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+3]);
      pMct->mirror[MIRROR1].m[MOTOR_B].mdeg100 = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+5]);
      pMct->mirror[MIRROR2].tracking = mct485RxBuffer[MCT485_DATA+7];
      pMct->mirror[MIRROR2].sensors = mct485RxBuffer[MCT485_DATA+8];
      pMct->mirror[MIRROR2].m[MOTOR_A].mdeg100 = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+9]);
      pMct->mirror[MIRROR2].m[MOTOR_B].mdeg100 = Mct485UnpackShort(&mct485RxBuffer[MCT485_DATA+11]);
    } // if (mct485RxBuffer[MCT485_LEN] == 19)
    else {
      // invalid response length
    }
    break;

  default:
    // unknown response type
    break;
  } // switch
  
  // successfully processed message
  return 1;
} // Mct485ProcessResp()


void __ISR(_UART_2A_VECTOR, ipl2) IntUart2AHandler(void) {
  // Is this an RX interrupt?
  if (INTGetFlag(INT_SOURCE_UART_RX(UART2A))) {
    if (mct485RxIndex < MCT485_MAX_INDEX) {
      mct485RxBuffer[mct485RxIndex++] = (byte)U2ARXREG;;
    }
    else {
      mct485RxBuffer[MCT485_MAX_INDEX-1] = (byte)U2ARXREG;;
    }
    // Clear the RX interrupt Flag
    INTClearFlag(INT_SOURCE_UART_RX(UART2A));
  } // MCT485_RX

	// Is this an TX interrupt?
	if (mct485State == MCT485_TX) {
    if (INTGetFlag(INT_SOURCE_UART_TX(UART2A))) {
      if (mct485TxIndex < mct485TxLen) {
        UARTSendDataByte(UART2A, mct485TxQueue[mct485TxQueueRd][mct485TxIndex++]);
        if (mct485TxIndex >= mct485TxLen) {
          // enable TC interrupt
          UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
        }
      } // still sending data
      else {
        // transmission is complete, enable reception
        PORTClearBits(string[mct485String[mct485TxQueueRd]].port_TxEn,
                      string[mct485String[mct485TxQueueRd]].mask_TxEn);
        PORTClearBits(string[mct485String[mct485TxQueueRd]].port_RxEnn,
                      string[mct485String[mct485TxQueueRd]].mask_RxEnn);
        mct485RxIndex = 0;
        mct485State = MCT485_RX;
        mct485RxTimeout = 125;
        INTEnable(INT_SOURCE_UART_TX(UART2A), INT_DISABLED);
        // Clear the RX interrupt Flag
        INTClearFlag(INT_SOURCE_UART_RX(UART2A));
        INTEnable(INT_SOURCE_UART_RX(UART2A), INT_ENABLED);
      } // done sending data
      // Clear the TX interrupt Flag
      INTClearFlag(INT_SOURCE_UART_TX(UART2A));
    }
  } // MCT485_TX 
  else {
    INTEnable(INT_SOURCE_UART_TX(UART2A), INT_DISABLED);
  }
}


ulong Mct485UnpackLong(byte *pBuffer) {
  ulong l;
  
  l = *pBuffer++;
  l <<= 8;
  l |= *pBuffer++;
  l <<= 8;
  l |= *pBuffer++;
  l <<= 8;
  l |= *pBuffer++;
  
  return l;
} // Mct485UnpackLong()


ushort Mct485UnpackShort(byte *pBuffer) {
  ushort s;
  
  s = *pBuffer++;
  s <<= 8;
  s |= *pBuffer++;
  
  return s;
} // Mct485UnpackShort()
