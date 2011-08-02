typedef enum {
  RTC_SEC_FRACT,
  RTC_SEC,      // BCD seconds, stop bit in 7
  RTC_MIN,      // BCD minutes
  RTC_HOUR,     // BCD hours, century bits in 6&7
  RTC_DAY,      // 1-7 day of week
  RTC_DATE,     // BCD 1-31
  RTC_MONTH,    // BCD 1-12
  RTC_YEAR,     // BCD 0-99
  RTC_CTRL,     // control register
  NUM_RTC
} mRTC;

#define RTC_I2C_WR_ADDR  0xD0
#define RTC_I2C_RD_ADDR  0xD1


EXTERN byte rtcYear;
EXTERN byte rtcMonth;
EXTERN byte rtcDate;
EXTERN byte rtcHour;
EXTERN byte rtcMin;
EXTERN byte rtcSec;


void RtcInit(void);
void RtcUpdate(void);
void RtcStartRead(void);
void RtcSetClock(byte *data);
void RtcWriteClock(void);
void RtcProcessResp(byte index);
void RtcClearHTFlag(void);
byte bcd2hex(byte bcd);
byte hex2bcd(byte hex);
