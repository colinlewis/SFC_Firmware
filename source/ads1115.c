#include "sfc.h"
#include "I2CPrivate.h"
#include "ads1115.h"
#include "field.h"


void Ads1115Init(void) {
  // (72M / (2*100kHz)) - 2 = 358
  I2C2BRG = 358;
  
  i2c2QueueRd = 0;
  i2c2QueueWr = 0;
  i2c2Mode = I2C2_MODE_IDLE;
  I2CSetFrequency(I2C2, 72000000L, 90000L);
  I2CEnable(I2C2, TRUE);
  // enable interrupt
  IEC1 |= 0x2000;
  // priority1
  IPC8 |= 0x400;
} // Ads1115Init()


void I2C2update(void) {
  if (i2c2Mode == I2C2_MODE_IDLE) {
    if (i2c2QueueRd != i2c2QueueWr) {
      // show that I2C2 is busy
      i2c2Mode = I2C2_MODE_WRITE;
      // clear the write and read indices
      i2c2Queue[i2c2QueueRd].wi = 0;
      i2c2Queue[i2c2QueueRd].ri = 0;
      // start I2C2 transfer
      i2c2BusState = I2C_BUS_WR_DATA;
      I2C2CONSET = _I2CCON_SEN_MASK;
    }
  } // I2C2_IDLE
  else if (i2c2Mode == I2C2_MODE_WRITE_COMPLETE) {
    i2c2QueueRd++;
    if (i2c2QueueRd >= I2C2_QUEUE_SIZE) {
      i2c2QueueRd = 0;
    }
    i2c2Mode = I2C2_MODE_IDLE;
  } // I2C2_WRITE_COMPLETE
  else if (i2c2Mode == I2C2_MODE_READ_COMPLETE) {
    I2C2process();
    i2c2QueueRd++;
    if (i2c2QueueRd >= I2C2_QUEUE_SIZE) {
      i2c2QueueRd = 0;
    }
    i2c2Mode = I2C2_MODE_IDLE;
  } // I2C2_READ_COMPLETE
} // I2C2update()


void I2C2process(void) {
  byte chan;
  
  chan = i2c2Queue[i2c2QueueRd].chan;
  if (chan < ADS1115_MAX_CHAN) {
    ads1115[chan] = i2c2Queue[i2c2QueueRd].rd[0];
    ads1115[chan] *= 256;
    ads1115[chan] += i2c2Queue[i2c2QueueRd].rd[1];
    // trim off negative numbers
    if (ads1115[chan] & 0x8000) {
      ads1115[chan] = 0;
    }
  }
  switch (chan) {
  case ADS1115_IN1:
    // hardcoded for 5V operation
    if (fieldInState & 0x01) {
      // if drop below 1.5V, turn off
      if (ads1115[ADS1115_IN1] < 1324) {
        fieldInState &= ~0x01;
      }
    }
    else {
      // if rise above 3.5V, turn on
      if (ads1115[ADS1115_IN1] > 3089) {
        fieldInState |= 0x01;
      }
    }
    break;
  
  case ADS1115_IN2:
    // hardcoded for 5V operation
    if (fieldInState & 0x02) {
      // if drop below 1.5V, turn off
      if (ads1115[ADS1115_IN2] < 1324) {
        fieldInState &= ~0x02;
      }
    }
    else {
      // if rise above 3.5V, turn on
      if (ads1115[ADS1115_IN2] > 3089) {
        fieldInState |= 0x02;
      }
    }
    break;
  
  case ADS1115_IN3:
    // hardcoded for 5V operation
    if (fieldInState & 0x04) {
      // if drop below 1.5V, turn off
      if (ads1115[ADS1115_IN3] < 1324) {
        fieldInState &= ~0x04;
      }
    }
    else {
      // if rise above 3.5V, turn on
      if (ads1115[ADS1115_IN3] > 3089) {
        fieldInState |= 0x04;
      }
    }
    break;
  
  case ADS1115_IN4:
    // hardcoded for 5V operation
    if (fieldInState & 0x08) {
      // if drop below 1.5V, turn off
      if (ads1115[ADS1115_IN4] < 1324) {
        fieldInState &= ~0x08;
      }
    }
    else {
      // if rise above 3.5V, turn on
      if (ads1115[ADS1115_IN4] > 3089) {
        fieldInState |= 0x08;
      }
    }
    break;
  
  case ADS1115_IN5:
    // hardcoded for 5V operation
    if (fieldInState & 0x10) {
      // if drop below 1.5V, turn off
      if (ads1115[ADS1115_IN5] < 1324) {
        fieldInState &= ~FIELD_STAT_UPS_SHUTDOWN;
      }
    }
    else {
      // if rise above 3.5V, turn on
      if (ads1115[ADS1115_IN5] > 3089) {
        fieldInState |= FIELD_STAT_UPS_SHUTDOWN;
      }
    }
    break;
  
  case ADS1115_RTD:
    fceRTD = RTDconvert(ads1115[ADS1115_RTD]);
    break;
  
  case ADS1115_FLOW_SW:
    // hardcoded for 24V operation
    if (fieldInState & FIELD_STAT_FLOW_SW) {
      // if drop below 7V, turn off
      if (ads1115[ADS1115_FLOW_SW] < 7062) {
        fieldInState &= ~FIELD_STAT_FLOW_SW;
      }
    }
    else {
      // if rise above 14V, turn on
      if (ads1115[ADS1115_FLOW_SW] > 14124) {
        fieldInState |= FIELD_STAT_FLOW_SW;
      }
    }
    break;

  default:
    break;
  } // switch
} // I2C2process()


void __ISR(_I2C_2_VECTOR, ipl2) IntI2C2AHandler(void) {
  byte ri, wi;
  
  INTClearFlag(INT_I2C2);
  switch (i2c2BusState) {
  case I2C_BUS_WR_DATA:
    wi = i2c2Queue[i2c2QueueRd].wi;
    if (wi < i2c2Queue[i2c2QueueRd].wn) {
      // send next byte
      I2C2TRN = i2c2Queue[i2c2QueueRd].wd[wi++];
      i2c2Queue[i2c2QueueRd].wi = wi;
    }
    else if (i2c2Queue[i2c2QueueRd].rn) {
      // there is data to read
      i2c2BusState = I2C_BUS_RD_ADDR;
      // issue REPEAT START
      I2C2CONSET = _I2CCON_RSEN_MASK;
    }
    else {
      // write only, transmit complete
      i2c2BusState = I2C_BUS_WR_COMPLETE;
      // issue STOP
      I2C2CONSET = _I2CCON_PEN_MASK;
    }
    break;

  case I2C_BUS_WR_COMPLETE:
    i2c2Mode = I2C2_MODE_WRITE_COMPLETE;
    break;

  case I2C_BUS_RD_ADDR:
    I2C2TRN = i2c2Queue[i2c2QueueRd].ra;
    i2c2Queue[i2c2QueueRd].ri = 0;
    i2c2BusState = I2C_BUS_RD_DATA;
    break;

  case I2C_BUS_RD_DATA:
    I2C2CONSET = _I2CCON_RCEN_MASK;
    i2c2BusState = I2C_BUS_RD_ACK;
    break;

  case I2C_BUS_RD_ACK:
    // always ack
    I2C2CONCLR = _I2CCON_ACKDT_MASK;
    I2C2CONSET = _I2CCON_ACKEN_MASK;
    ri = i2c2Queue[i2c2QueueRd].ri;
    i2c2Queue[i2c2QueueRd].rd[ri++] = I2C2RCV;
    i2c2Queue[i2c2QueueRd].ri = ri;
    if (ri < i2c2Queue[i2c2QueueRd].rn) {
      i2c2BusState = I2C_BUS_RD_DATA;
    }
    else {
      i2c2BusState = I2C_BUS_RD_STOP;
    }
    break;

  case I2C_BUS_RD_STOP:
    // receive complete
    i2c2BusState = I2C_BUS_RD_COMPLETE;
    // issue STOP
    I2C2CONSET = _I2CCON_PEN_MASK;
    break;
  
  case I2C_BUS_RD_COMPLETE:
    i2c2Mode = I2C2_MODE_READ_COMPLETE;
    break;

  default:
    // should never happen
    break;
  } // switch
} // ISR()
