#include "sfc.h"
#include "i2c1.h"
#include "rtc.h"


byte rtcTest;
void RtcInit(void) {
  rtcYear = 10;   // 2010
  rtcMonth = 11;  // November
  rtcDate = 8;    // 8th
  rtcHour = 1;
  rtcMin = 2;
  rtcSec = 3;
  rtcTest = 0;
  RtcClearHTFlag();
  RtcStartRead();
} // RtcInit()


void RtcUpdate(void) {
  if (rtcTest != 0) {
    rtcTest = 0;
    RtcWriteClock();
  }  
  if (sysTicks == 256) {
    RtcStartRead();
  }
} // RtcUpdate()


void RtcStartRead(void) {
  I2C1_QUEUE_t *queue;
  
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_RTC_READ;
  queue->wn = 2;
  queue->wi = 0;
  queue->wd[0] = RTC_I2C_WR_ADDR;
  queue->wd[1] = RTC_SEC;
  queue->ra = RTC_I2C_RD_ADDR;
  queue->rn = RTC_YEAR - RTC_SEC + 1;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
} // RtcStartRead()


void RtcProcessResp(byte index) {
  I2C1_QUEUE_t *queue;
  
  queue = &i2c1Queue[index];
  switch (queue->src) {
  case I2C1_RTC_READ:
    rtcSec = bcd2hex(queue->rd[RTC_SEC-RTC_SEC] & 0x7F);
    rtcMin = bcd2hex(queue->rd[RTC_MIN-RTC_SEC]);
    rtcHour = bcd2hex(queue->rd[RTC_HOUR-RTC_SEC] & 0x3F);
    rtcDate = bcd2hex(queue->rd[RTC_DATE-RTC_SEC]);
    rtcMonth = bcd2hex(queue->rd[RTC_MONTH-RTC_SEC]);
    rtcYear = bcd2hex(queue->rd[RTC_YEAR-RTC_SEC]);
    break;

  case I2C1_RTC_WRITE:
    // force a timestamp in the log
    break;
  
  default:
    break;
  }
} // RtcProcessResp()


void RtcSetClock(byte *data) {
  I2C1_QUEUE_t *queue;
  
  if (data[0] > 59) {
    return;
  }
  if (data[1] > 59) {
    return;
  }
  if (data[2] > 23) {
    return;
  }
  if (data[3] > 31) {
    return;
  }
  if (data[4] > 12) {
    return;
  }

  RtcClearHTFlag();
  
  // create a message to update the clock  
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_RTC_WRITE;
  queue->wn = 9;
  queue->wi = 0;
  queue->wd[0] = RTC_I2C_WR_ADDR;
  queue->wd[1] = RTC_SEC;
  queue->wd[2] = hex2bcd(data[0]);
  queue->wd[3] = hex2bcd(data[1]);
  queue->wd[4] = hex2bcd(data[2]);
  queue->wd[6] = hex2bcd(data[3]);
  queue->wd[7] = hex2bcd(data[4]);
  queue->wd[8] = hex2bcd(data[5]);
  queue->ra = RTC_I2C_RD_ADDR;
  queue->rn = 0;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
} // RtcSetClock()


void RtcWriteClock(void) {
  I2C1_QUEUE_t *queue;
  
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_RTC_WRITE;
  queue->wn = 9;
  queue->wi = 0;
  queue->wd[0] = RTC_I2C_WR_ADDR;
  queue->wd[1] = RTC_SEC;
  queue->wd[2] = hex2bcd(rtcSec);
  queue->wd[3] = hex2bcd(rtcMin);
  queue->wd[4] = hex2bcd(rtcHour);
  queue->wd[6] = hex2bcd(rtcDate);
  queue->wd[7] = hex2bcd(rtcMonth);
  queue->wd[8] = hex2bcd(rtcYear);
  queue->ra = RTC_I2C_RD_ADDR;
  queue->rn = 0;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
}


void RtcClearHTFlag(void) {
  I2C1_QUEUE_t *queue;
  
  // clear the HT bit and the flag register
  queue = &i2c1Queue[i2c1QueueWr];
  queue->src = I2C1_RTC_WRITE;
  queue->wn = 6;
  queue->wi = 0;
  queue->wd[0] = RTC_I2C_WR_ADDR;
  queue->wd[1] = 0x0C;
  queue->wd[2] = 0; // D6 is HT bit
  queue->wd[3] = 0;
  queue->wd[4] = 0;
  queue->wd[5] = 0; // flag register
  queue->ra = RTC_I2C_RD_ADDR;
  queue->rn = 0;
  queue->ri = 0;
  queue->postDelay = 0;
  I2C1write();
} // RtcClearHTFlag()


byte bcd2hex(byte bcd) {
  byte hex;
  
  hex = (bcd >> 4) * 10;
  hex += (bcd & 0x0F);
  
  return hex;
} // bcd2hex()


byte hex2bcd(byte hex) {
  byte bcd;
  
  bcd = (hex / 10) * 16;
  bcd |= (hex % 10);
  
  return bcd;
} // bcd2hex()
