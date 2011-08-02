#include "SFC.h"
#include "I2C1.h"


#define TC74_I2C_WR_ADDR   0x90
#define TC74_I2C_RD_ADDR   0x91
#define TC74_RTR           0
#define TC74_RWCR          1


void TC74Init(void) {
  I2C1_QUEUE_t *queue;
  
  sfcPcbTemp = 0;
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_TC74_WRITE;
  queue->wn = 3;
  queue->wi = 0;
  queue->wd[0] = TC74_I2C_WR_ADDR;
  queue->wd[1] = TC74_RWCR;
  queue->wd[1] = 0;
  queue->ra = 0;
  queue->rn = 0;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
}


void TC74Update(void) {
  I2C1_QUEUE_t *queue;
  
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_TC74_READ;
  queue->wn = 2;
  queue->wi = 0;
  queue->wd[0] = TC74_I2C_WR_ADDR;
  queue->wd[1] = TC74_RTR;
  queue->ra = TC74_I2C_RD_ADDR;
  queue->rn = 1;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
} // TC74Update()


void TC74ProcessResp(byte index) {
  I2C1_QUEUE_t *queue;
  
  queue = &i2c1Queue[index];
  switch (queue->src) {
  case I2C1_TC74_READ:
    if (queue->rn == 1 && queue->rd[0] != 0xFF) {
      sfcPcbTemp = queue->rd[0];
      sfcPcbTemp *= 10;
    }
    break;

  default:
    break;
  }
} // void TC74ProcessResp()
