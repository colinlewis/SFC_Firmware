#include "sfc.h"
#include "I2CPrivate.h"
#include "I2C1.h"


void I2C1init(void) {
  // (72M / (2*100kHz)) - 2 = 358
  I2C1BRG = 358;
  
  i2c1QueueRd = 0;
  i2c1QueueWr = 0;
  i2c1Mode = I2C1_MODE_IDLE;
  I2CSetFrequency(I2C1, 72000000L, 90000L);
  I2CEnable(I2C1, TRUE);
  // enable interrupt
  IEC0 |= 0x80000000;
  // priority1
  IPC6 |= 0x400;
} // I2C1init()


void I2C1update(void) {
  if (I2C1STAT & 0x0400) {
    // bus collision
    i2c1Mode = I2C1_MODE_ERROR;
    i2c1BusState = I2C1_BUS_ERROR;
    I2CEnable(I2C1, FALSE);
  }
  
  if (i2c1Delay) {
    i2c1Delay--;
    return;
  }
  
  if (i2c1Mode == I2C1_MODE_IDLE) {
    if (i2c1QueueRd != i2c1QueueWr) {
      // show that I2C1 is busy
      i2c1Mode = I2C1_MODE_WRITE;
      // clear the write and read indices
      i2c1Queue[i2c1QueueRd].wi = 0;
      i2c1Queue[i2c1QueueRd].ri = 0;
      // start I2C1 transfer
      i2c1BusState = I2C1_BUS_WR_DATA;
      I2C1CONSET = _I2CCON_SEN_MASK;
    }
  } // I2C1_IDLE
  else if (i2c1Mode == I2C1_MODE_WRITE_COMPLETE) {
    I2C1process();
    i2c1QueueRd++;
    if (i2c1QueueRd >= I2C1_QUEUE_SIZE) {
      i2c1QueueRd = 0;
    }
    i2c1Mode = I2C1_MODE_IDLE;
  } // I2C1_WRITE_COMPLETE
  else if (i2c1Mode == I2C1_MODE_READ_COMPLETE) {
    I2C1process();
    i2c1QueueRd++;
    if (i2c1QueueRd >= I2C1_QUEUE_SIZE) {
      i2c1QueueRd = 0;
    }
    i2c1Mode = I2C1_MODE_IDLE;
  } // I2C1_READ_COMPLETE
  else if (i2c1Mode == I2C1_MODE_ERROR) {
    // usually caused is SDA is low before start
    if (!(PORTA & PA_SCL1)) {
      // is clock is low, raise it
      PORTSetPinsDigitalOut(IOPORT_A, PA_SCL1);
      PORTSetBits(IOPORT_A, PA_SCL1);
    }
    else {
      if (!(PORTA & PA_SDA1)) {
        // if SDA is still low, try another clock pulse
        PORTSetPinsDigitalOut(IOPORT_A, PA_SCL1);
        PORTClearBits(IOPORT_A, PA_SCL1);
      }
      else {
        // SDA is now high, try clearing the error
        PORTSetPinsDigitalOut(IOPORT_A, PA_SDA1);
        PORTClearBits(IOPORT_A, PA_SDA1);
        i2c1Mode = I2C1_MODE_ERR_START;
      }
    }
  }
  else if (i2c1Mode == I2C1_MODE_ERR_START) {
    PORTSetBits(IOPORT_A, PA_SDA1);
    i2c1Mode = I2C1_MODE_ERR_STOP;
  }  
  else if (i2c1Mode == I2C1_MODE_ERR_STOP) {
    I2C1STAT &= ~0x400;
    IFS0 &= ~0x20000000;
    PORTSetPinsDigitalIn(IOPORT_A, PA_SCL1 | PA_SDA1);
    I2CEnable(I2C1, TRUE);
    i2c1Mode = I2C1_MODE_IDLE;
  }  
} // I2C1update()


void I2C1process(void) {
  i2c1Delay = i2c1Queue[i2c1QueueRd].postDelay;
  switch (i2c1Queue[i2c1QueueRd].src) {
  case I2C1_RTC_READ:
  case I2C1_RTC_WRITE:
    RtcProcessResp(i2c1QueueRd);
    break;

  case I2C1_EEPROM_READ:
    ParamProcessResp(i2c1QueueRd);
    break;

  case I2C1_TC74_READ:
  case I2C1_TC74_WRITE:
    TC74ProcessResp(i2c1QueueRd);
    break;
    
  default:
    break;
  } // switch (src)
} // I2C1process()


void I2C1write(void) {
  i2c1QueueWr++;
  if (i2c1QueueWr >= I2C1_QUEUE_SIZE) {
    i2c1QueueWr = 0;
  }
} // I2C1write()


void __ISR(_I2C_1_VECTOR, ipl2) IntI2C1AHandler(void) {
  byte ri, wi;
  
  INTClearFlag(INT_I2C1);
  switch (i2c1BusState) {
  case I2C1_BUS_WR_DATA:
    wi = i2c1Queue[i2c1QueueRd].wi;
    if (wi < i2c1Queue[i2c1QueueRd].wn) {
      // send next byte
      I2C1TRN = i2c1Queue[i2c1QueueRd].wd[wi++];
      i2c1Queue[i2c1QueueRd].wi = wi;
    }
    else if (i2c1Queue[i2c1QueueRd].rn) {
      // there is data to read
      i2c1BusState = I2C1_BUS_RD_ADDR;
      // issue REPEAT START
      I2C1CONSET = _I2CCON_RSEN_MASK;
    }
    else {
      // write only, transmit complete
      i2c1BusState = I2C1_BUS_WR_COMPLETE;
      // issue STOP
      I2C1CONSET = _I2CCON_PEN_MASK;
    }
    break;

  case I2C1_BUS_WR_COMPLETE:
    i2c1Mode = I2C1_MODE_WRITE_COMPLETE;
    break;

  case I2C1_BUS_RD_ADDR:
    I2C1TRN = i2c1Queue[i2c1QueueRd].ra;
    i2c1Queue[i2c1QueueRd].ri = 0;
    i2c1BusState = I2C1_BUS_RD_DATA;
    break;

  case I2C1_BUS_RD_DATA:
    I2C1CONSET = _I2CCON_RCEN_MASK;
    i2c1BusState = I2C1_BUS_RD_ACK;
    break;

  case I2C1_BUS_RD_ACK:
    ri = i2c1Queue[i2c1QueueRd].ri;
    // don't ack last byte
    if (ri+1 == i2c1Queue[i2c1QueueRd].rn) {
      I2C1CONSET = _I2CCON_ACKDT_MASK;
    }
    else {
      I2C1CONCLR = _I2CCON_ACKDT_MASK;
    }    
    I2C1CONSET = _I2CCON_ACKEN_MASK;
    i2c1Queue[i2c1QueueRd].rd[ri++] = I2C1RCV;
    i2c1Queue[i2c1QueueRd].ri = ri;
    if (ri < i2c1Queue[i2c1QueueRd].rn) {
      i2c1BusState = I2C1_BUS_RD_DATA;
    }
    else {
      i2c1BusState = I2C1_BUS_RD_STOP;
    }
    break;

  case I2C1_BUS_RD_STOP:
    // receive complete
    i2c1BusState = I2C1_BUS_RD_COMPLETE;
    // issue STOP
    I2C1CONSET = _I2CCON_PEN_MASK;
    break;
  
  case I2C1_BUS_RD_COMPLETE:
    i2c1Mode = I2C1_MODE_READ_COMPLETE;
    break;

  default:
    // should never happen
    break;
  } // switch
} // ISR()
