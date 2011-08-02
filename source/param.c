#include "sfc.h"
#include "field.h"
#include "i2c1.h"
#include "param.h"


void ParamInit(void) {
  byte timeout;
  
  param[PARAM_DESICCANT_T1] = (5+12)<<8; // 5:00 pm - time to dry
  param[PARAM_DESICCANT_T2] = 0x0700; // 7:00am - time to regen or close
  param[PARAM_DESICCANT_T3] = (1+12)<<8; // 1:00 pm - time to close
  param[PARAM_DESICCANT_MODE] = DESICCANT_CLOSED;
  param[PARAM_DESICCANT_MAX_TEMP] = 1200; // 120.0C
  param[PARAM_DESICCANT_TEMP_HYST] = 200; // 20.0C
  param[PARAM_RTD_ZERO] = 0; // no adjustment
  param[PARAM_RTD_SPAN] = 0; // no adjustment
  param[PARAM_DESICCANT_REGEN_HUMIDITY] = 120; // 12.0%
//  param[PARAM_DESICCANT_DUTY1_TEMP] = 350; // 35.0 degrees C
//  param[PARAM_DESICCANT_DUTY2_TEMP] = 450; 
//  param[PARAM_DESICCANT_DUTY3_TEMP] = 550; 
//  param[PARAM_DESICCANT_MIN_FAN_TEMP] = 500; 
  param[PARAM_DESICCANT_REGEN_INLET_TEMP1] = 500;
  param[PARAM_DESICCANT_REGEN_INLET_TEMP2] =  800;
  param[PARAM_DESICCANT_REGEN_INLET_TEMP3] =  1000;
  param[PARAM_DESICCANT_REGEN_OUTLET_TEMP1] = 300;
  param[PARAM_DESICCANT_REGEN_OUTLET_TEMP2] = 450;
  param[PARAM_DESICCANT_REGEN_OUTLET_TEMP3 ] = 750;
  param[PARAM_DESICCANT_REGEN_FAN_DUTYCYCLE] =  4; // fan duty cycle range 0-8
  param[PARAM_DESICCANT_DRYING_FAN_DUTYCYCLE] = 4;   

  paramInitComplete = 0;
  ParamRead(0);

  // wait for response, then read next entry;
  timeout = 200;
  while (timeout) {
    if (sysTickEvent) {
      sysTickEvent = 0;
      timeout--;
    }
    I2C1update();
  }  
} // ParamInit()


void ParamRead(byte num) {
  ushort addr;
  I2C1_QUEUE_t *queue;

  // eeprom address to read  
  addr = num * 8;

  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_EEPROM_READ;
  queue->wn = 3;
  queue->wi = 0;
  queue->wd[0] = EEPROM_I2C_WR_ADDR;
  queue->wd[1] = (byte)(addr >> 8);
  queue->wd[2] = (byte)addr;
  queue->ra = EEPROM_I2C_RD_ADDR;
  queue->rn = 8;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
}


void ParamProcessResp(byte index) {
  I2C1_QUEUE_t *queue;
  ushort addr;
  byte paramNum;
  
  queue = &i2c1Queue[index];
  // retrieve the address
  addr = queue->wd[1];
  addr *= 256;
  addr += queue->wd[2];
  paramNum = addr >> 3;
  switch (queue->src) {
  case I2C1_EEPROM_READ:
    if ((addr & 0x07) != 0 || paramNum >= NUM_PARAMS) {
      // bad address
    }
    else if (queue->rd[0] == (byte)(~queue->rd[4]) &&
        queue->rd[1] == (byte)(~queue->rd[5]) &&
        queue->rd[2] == (byte)(~queue->rd[6]) &&
        queue->rd[3] == (byte)(~queue->rd[7])) {
      // inverse matches data, good entry
      param[paramNum] = queue->rd[0];
      param[paramNum] *= 256;
      param[paramNum] += queue->rd[1];
      param[paramNum] *= 256;
      param[paramNum] += queue->rd[2];
      param[paramNum] *= 256;
      param[paramNum] += queue->rd[3];
    }
    if (paramInitComplete == 0) {
      if (paramNum < NUM_PARAMS-1) {
          // read next param
          ParamRead(paramNum+1);
      }
      else {
        paramInitComplete = 1;
      }    
    }
    break;

  case I2C1_EEPROM_WRITE:
    // force a timestamp in the log
    break;
  
  default:
    break;
  }
} // ParamProcessResp()


void ParamWrite(byte paramNum, ulong newVal) {
  ushort addr;
  I2C1_QUEUE_t *queue;

  if (paramNum >= NUM_PARAMS) {
    return;
  }
  
  param[paramNum] = newVal;
  
  // eeprom address to read  
  addr = paramNum * 8;

  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_EEPROM_WRITE;
  queue->wn = 11;
  queue->wi = 0;
  queue->wd[0] = EEPROM_I2C_WR_ADDR;
  queue->wd[1] = (byte)(addr >> 8);
  queue->wd[2] = (byte)addr;
  queue->wd[3] = (byte)(newVal >> 24);
  queue->wd[4] = (byte)(newVal >> 16);
  queue->wd[5] = (byte)(newVal >>  8);
  queue->wd[6] = (byte)newVal;
  queue->wd[7] = ~queue->wd[3];
  queue->wd[8] = ~queue->wd[4];
  queue->wd[9] = ~queue->wd[5];
  queue->wd[10] = ~queue->wd[6];
  queue->ra = EEPROM_I2C_RD_ADDR;
  queue->rn = 0;
  queue->ri = 0;
  queue->postDelay = 10;
  I2C1write();
} // ParamWrite()
