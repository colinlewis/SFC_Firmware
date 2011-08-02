#include "sfc.h"
#include "ads1244.h"
#include "field.h"
#include "rtu.h"

void RtuInit(void) {
  rtu485State = RTU485_IDLE;
  rtu485TxTimeout = 0;
  
  // turn off UART to clear state
  UARTEnable(UART2B, 0);
  // pull-up for MCT_RX
  CNPUE = 0x200;
	// PBCLK = 72MHz
  UARTConfigure(UART2B, UART_ENABLE_PINS_TX_RX_ONLY);
  UARTSetFifoMode(UART2B, UART_INTERRUPT_ON_RX_NOT_EMPTY);
  UARTSetLineControl(UART2B, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
  UARTSetDataRate(UART2B, 72000000L, 9600L);
  UARTEnable(UART2B, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

	// Configure UART2 RX Interrupt
	INTEnable(INT_SOURCE_UART_RX(UART2B), INT_ENABLED);
  INTSetVectorPriority(INT_VECTOR_UART(UART2B), INT_PRIORITY_LEVEL_2);
  INTSetVectorSubPriority(INT_VECTOR_UART(UART2B), INT_SUB_PRIORITY_LEVEL_0);
  rtuAddr[0] = 40003; // 40003 = Btu/Hr
  rtuAddr[1] = 40015; // 40015 = L/Hr
  rtuAddr[2] = 40024; // 40024 = Supply degC * 100
  rtuAddr[3] = 40025; // 40025 = Return degC * 100
  rtuNumAddr = 4;
} // RtuInit()


void RtuUpdate(void) {
  if (rtu485State == RTU485_IDLE) {
    if (rtu485TxTimeout) {
      rtu485TxTimeout--;
    }
    else {
      RtuReadNextHolding();
    }
  } // RTU485_IDLE
  else if (rtu485State == RTU485_RX) {
    // switch to receive mode
    rtu485RxTimeout--;
    if (!rtu485RxTimeout) {
      rtu485TxTimeout = 8;
      rtu485State = RTU485_IDLE;
    }  
    else if (rtu485RxIndex) {
      // started receiving
      if (UARTGetLineStatus(UART2B) & UART_RECEIVER_IDLE) {
        RtuProcessMsg();
        rtu485TxTimeout = 208;
        rtu485State = RTU485_IDLE;
      }
    }    
  }
  else {
    // wait for Tx to complete, changes to RX in ISR
  }
} // RtuUpdate()


void RtuTxEn(byte en) {
  PORTClearBits(IOPORT_E, PE_HC595_RCLK
                        | PE_HC595_SCLK
                        | PE_ADS1244_SCLK
                        | PE_EXP_RS485_EN);
  if (en) {
    PORTSetBits(IOPORT_E, PE_EXP_RS485_EN);
  }
  if (ads1244sclks & 0x01) {
    PORTSetBits(IOPORT_E, PE_ADS1244_SCLK);
  }
  PORTSetBits(IOPORT_A, PA_EXP_WR);
  PORTClearBits(IOPORT_A, PA_EXP_WR);
}


void RtuReadNextHolding(void) {
  ushort crc;
  
  if (++rtuNextAddr >= rtuNumAddr) {
    rtuNextAddr = 0;
  }
  rtu485TxBuffer[0] = 0x11;    // default
  rtu485TxBuffer[1] = 0x03;  // read holding
  //rtu485TxBuffer[1] = 17;  // read holding
  rtu485TxBuffer[2] = (byte)((rtuAddr[rtuNextAddr]-40001) >> 8);
  rtu485TxBuffer[3] = (byte)(rtuAddr[rtuNextAddr]-40001);
  rtu485TxBuffer[4] = 0;
  rtu485TxBuffer[5] = 1;
  crc = RtuCrcCalc(rtu485TxBuffer, 6);
  rtu485TxBuffer[7] = (byte)(crc >> 8);
  rtu485TxBuffer[6] = (byte)crc;
  rtu485TxLen = 8;
  //crc = RtuCrcCalc(rtu485TxBuffer, 2);
  //rtu485TxBuffer[3] = (byte)(crc >> 8);
  //rtu485TxBuffer[2] = (byte)crc;
  //rtu485TxLen = 4;
  rtu485State = RTU485_TX;
  rtu485TxIndex = 0;
  RtuTxEn(1);
  INTDisableInterrupts();
  UARTSetFifoMode(UART2B, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
  while (rtu485TxIndex < rtu485TxLen && 
    UARTTransmitterIsReady(UART2B)) {
    UARTSendDataByte(UART2B, rtu485TxBuffer[rtu485TxIndex++]);
  }
  if (rtu485TxIndex < rtu485TxLen) {
    UARTSetFifoMode(UART2B, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
  }
  INTClearFlag(INT_SOURCE_UART_TX(UART2B));
  INTEnable(INT_SOURCE_UART_TX(UART2B), INT_ENABLED);
  INTEnableInterrupts();
} // RtuReadNextHolding()


void RtuProcessMsg(void) {
  unsigned long val;
  
  if (rtu485RxIndex < 4) {
    rtu485TxLen = 1;
  }
  else if (RtuCrcCalc(rtu485RxBuffer, rtu485RxIndex)) {
    // crc failed
    rtu485TxLen = 2;
  }
  else {
    if (rtu485RxBuffer[1] == 3) {
      // response to read holding
      if (rtu485RxIndex == 7 && rtu485RxBuffer[2] == 2) {
        rtuData[rtuNextAddr] = rtu485RxBuffer[3];
        rtuData[rtuNextAddr] <<= 8;
        rtuData[rtuNextAddr] += rtu485RxBuffer[4];
        if (rtuAddr[rtuNextAddr] == 40003) {
          // convert BTU/Hr to W
          val = rtuData[rtuNextAddr];
          val *= 19207;
          val /= 65536;
          rtuData[rtuNextAddr] = (ushort)val;
          fieldPower = val;
        }
        else if (rtuAddr[rtuNextAddr] == 40015) {
          fieldFlow = rtuData[rtuNextAddr];
        }  
        else if (rtuAddr[rtuNextAddr] == 40024) {
          fieldInlet = rtuData[rtuNextAddr];
        }  
        else if (rtuAddr[rtuNextAddr] == 40025) {
          fieldOutlet = rtuData[rtuNextAddr];
        }  
      }
    }
  }
  rtu485RxIndex = 0;
} // RtuProcessMsg()


void __ISR(_UART_2B_VECTOR, ipl2) IntUart2BHandler(void) {
  // Is this an RX interrupt?
  if (INTGetFlag(INT_SOURCE_UART_RX(UART2B))) {
    if (rtu485RxIndex < RTU_MAX_INDEX) {
      rtu485RxBuffer[rtu485RxIndex++] = (byte)U2BRXREG;;
    }
    else {
      rtu485RxBuffer[RTU_MAX_INDEX-1] = (byte)U2BRXREG;;
    }
    // Clear the RX interrupt Flag
    INTClearFlag(INT_SOURCE_UART_RX(UART2B));
  } // RTU_RX

	// Is this an TX interrupt?
	if (rtu485State == RTU485_TX) {
    if (INTGetFlag(INT_SOURCE_UART_TX(UART2B))) {
      if (rtu485TxIndex < rtu485TxLen) {
        UARTSendDataByte(UART2B, rtu485TxBuffer[rtu485TxIndex++]);
        if (rtu485TxIndex >= rtu485TxLen) {
          // enable TC interrupt
          UARTSetFifoMode(UART2B, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
        }
      } // still sending data
      else {
        // transmission is complete, enable reception
        if (!(PORTA & PA_EXP_WR)) {
          // if WR is low, then not interrupting another write
          RtuTxEn(0);
        }
        else {
          // just clear the bit
          PORTClearBits(IOPORT_E, PE_EXP_RS485_EN);
        }
        rtu485RxIndex = 0;
        rtu485State = RTU485_RX;
        rtu485RxTimeout = 20;
        INTEnable(INT_SOURCE_UART_TX(UART2B), INT_DISABLED);
        // Clear the RX interrupt Flag
        INTClearFlag(INT_SOURCE_UART_RX(UART2B));
        INTEnable(INT_SOURCE_UART_RX(UART2B), INT_ENABLED);
      } // done sending data
      // Clear the TX interrupt Flag
      INTClearFlag(INT_SOURCE_UART_TX(UART2B));
    }
  } // RTU_TX 
  else {
    INTEnable(INT_SOURCE_UART_TX(UART2B), INT_DISABLED);
  }
}
