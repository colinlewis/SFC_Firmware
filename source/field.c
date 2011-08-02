#include "sfc.h"
#include "ads1115.h"
#include "ads1244.h"
#include "field.h"
#include "mct485.h"
#include "mctbus.h"
#include "rtu.h"
#include "track.h"


void FieldInit(void) {
  PORTClearBits(IOPORT_A, PA_EXP_WR);
  // default state for EXP_RD is high
  PORTSetBits(IOPORT_A, PA_EXP_RD);
  PORTSetPinsDigitalOut(IOPORT_A, PA_EXP_RD | PA_EXP_WR);
  
  // init globals
  exp485State = EXP485_IDLE;
  fieldInState = 0;
  FCE_outputs = 0x00;
  fceType = FCE_NONE;
  FieldNewState(FIELD_OFF);
  fieldTimeout = 0;
  fieldLoggingMode = 0;
  fieldAttenPercent = 0; // 0 - 100%
  fieldAttenNum = 0; // number of MCT's
  fieldAttenTimeout = 0; // milliseconds
} // FieldInit(void)


void FieldUpdate(void) {
  byte i, j;
  byte data[2];
  
  if (!(sysTicks & 0x3F)) {
    FieldUpdateOutputs();
  }

  // update ADS1115 every 8msec  
  if ((sysTicks & 0x7) == 1) {
    FieldUpdateI2C();
  }
  else {
    I2C2update();
  }

  // read the inputs every time through
  FieldReadInputs();
  // Ads1244 uses result from reading inputs
  Ads1244Update();
  
  FieldAttenUpdate();
  
  if (fieldTimeout) {
    fieldTimeout--;
    return;
  }
  // check for a change in state
  if (!(sysTicks & 0x7F)) {
    switch (fieldState) {
    case FIELD_LOGGING:
      if (!usbActivityTimeout) {
        fieldLoggingMode = 0;
        FieldNewState(FIELD_END_OF_DAY);
        break;
      }
    case FIELD_OFF:
      if (fieldInState & FIELD_STAT_UPS_SHUTDOWN) {
        // don't turn on field if running from UPS
      }
      if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        // don't turn on field if interlock is open
      }
      // look for HEAT_REQ and DNI
      else if ((fieldInState & FIELD_STAT_HEAT_REQ) &&
          ((fieldInState & FIELD_STAT_DNI_HIGH) ||
           (fieldInState & FIELD_STAT_PYR_BYPASS))) {
        // Request pump
        FieldNewState(FIELD_PUMP_ON);
      }
      break;

    case FIELD_PUMP_ON:
      if (fieldInState & FIELD_STAT_UPS_SHUTDOWN) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      else if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      else if ((fieldInState & FIELD_STAT_FLOW_SW) &&
           ((string[0].state == STRING_ACTIVE) || (string[0].state == STRING_POWER_OFF)) &&
           ((string[1].state == STRING_ACTIVE) || (string[1].state == STRING_POWER_OFF)) &&
           ((string[2].state == STRING_ACTIVE) || (string[2].state == STRING_POWER_OFF)) &&
           ((string[3].state == STRING_ACTIVE) || (string[3].state == STRING_POWER_OFF)) &&
           (string[0].state == STRING_ACTIVE || string[1].state == STRING_ACTIVE ||
            string[2].state == STRING_ACTIVE || string[3].state == STRING_ACTIVE)) {
        // try to turn on field
        FieldNewState(FIELD_GO_ON_SUN);
      }
      break;

    case FIELD_GO_ON_SUN:
      if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      if (!(fieldInState & FIELD_STAT_HEAT_REQ) ||
          !(fieldInState & FIELD_STAT_FLOW_SW) ||
          (!(fieldInState & FIELD_STAT_DNI_HIGH) &&
           !(fieldInState & FIELD_STAT_PYR_BYPASS)) ||
          (fieldInState & FIELD_STAT_UPS_SHUTDOWN)) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      // XYZ monitor overtemp
      FieldNewState(FIELD_OPERATE);
      break;

    case FIELD_OPERATE:
      DataLogHourly();
      if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      if (!(fieldInState & FIELD_STAT_HEAT_REQ) ||
          !(fieldInState & FIELD_STAT_FLOW_SW) ||
          (!(fieldInState & FIELD_STAT_DNI_HIGH) &&
           !(fieldInState & FIELD_STAT_PYR_BYPASS)) ||
          (fieldInState & FIELD_STAT_UPS_SHUTDOWN)) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      if (fieldOnSunRetryTicks) {
        fieldOnSunRetryTicks--;
      }
      else {
        fieldOnSunRetryTicks = 7031;
        for (i = 0; i < 4; i++) {
          for (j = 1; j <= string[i].maxAddr; j++) {
            if (string[i].mct[j-1].mirror[0].tracking == TRACK_OFF) {
              //if (string[i].maxAddr <= 5) {
                // 1-5 MCT's
              //  data[0] = ALTERNATE_OFF;
              //}
              //else {
                // 6-10 MCT's
                data[0] = ALTERNATE_ON;
              //}  
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_ALTERNATE,
                            data);
              data[0] = MIRROR1;
              data[1] = TRACK_HOME;
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_TRACK,
                            data);
            } // if TRACK_OFF
            if (string[i].mct[j-1].mirror[1].tracking == TRACK_OFF) {
              //if (string[i].maxAddr <= 5) {
                // 1-5 MCT's
              //  data[0] = ALTERNATE_OFF;
              //}
              //else {
                // 6-10 MCT's
                data[0] = ALTERNATE_ON;
              //}  
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_ALTERNATE,
                            data);
              data[0] = MIRROR2;
              data[1] = TRACK_HOME;
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_TRACK,
                            data);
            } // if TRACK_OFF
          } // for j
        } // for i
      } // else
      // XYZ monitor overtemp
      break;

    case FIELD_END_OF_DAY:
      // if field is requesting heat, turn it back on
      if (fieldInState & FIELD_STAT_UPS_SHUTDOWN) {
        // don't turn on field if running from UPS
      }
      else if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        // don't turn on field if interlock is open
      }
      // look for HEAT_REQ and DNI
      else if ((fieldInState & FIELD_STAT_HEAT_REQ) &&
          ((fieldInState & FIELD_STAT_DNI_HIGH) ||
           (fieldInState & FIELD_STAT_PYR_BYPASS))) {
        // Request pump
        FieldNewState(FIELD_PUMP_ON);
        break;
      }
      
      if (!fieldEndOfDayTicks) {
        // taking too long, just turn the field off
        FieldNewState(FIELD_TEST_OFF);
      }
      else {
        fieldEndOfDayTicks--;
      }
      
      for (i = 0; i < 4; i++) {
        for (j = 1; j <= string[i].maxAddr; j++) {
          if (string[i].mct[j-1].mirror[0].tracking != TRACK_OFF &&
            string[i].mct[j-1].mirror[0].tracking != TRACK_ERROR) {
            return;
          }
          if (string[i].mct[j-1].mirror[1].tracking != TRACK_OFF &&
            //string[i].mct[j-1].mirror[1].tracking == TRACK_END ||
            string[i].mct[j-1].mirror[1].tracking != TRACK_ERROR) {
            return;
          }
        }
      }
      FieldNewState(FIELD_OFF);
      break;

    case FIELD_TEST:
    case FIELD_TEST_UPDATE:
      if (!usbActivityTimeout) {
        //FieldNewState(FIELD_END_OF_DAY);
      }
      break;

    case FIELD_TEST_ON_SUN:
      // XYZ monitor overtemp
      FieldNewState(FIELD_TEST_OPERATE);
      break;

    case FIELD_TEST_OPERATE:
      // XYZ monitor overtemp
      if (!usbActivityTimeout) {
        FieldNewState(FIELD_END_OF_DAY);
      }
      if (fieldOnSunRetryTicks) {
        fieldOnSunRetryTicks--;
      }
      else {
        fieldOnSunRetryTicks = 7031;
        for (i = 0; i < 4; i++) {
          for (j = 1; j <= string[i].maxAddr; j++) {
            if (string[i].mct[j-1].mirror[0].tracking == TRACK_OFF) {
              //if (string[i].maxAddr <= 5) {
                // 1-5 MCT's
              //  data[0] = ALTERNATE_OFF;
              //}
              //else {
                // 6-10 MCT's
                data[0] = ALTERNATE_ON;
              //}  
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_ALTERNATE,
                            data);
              data[0] = MIRROR1;
              data[1] = TRACK_HOME;
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_TRACK,
                            data);
            } // if TRACK_OFF
            if (string[i].mct[j-1].mirror[1].tracking == TRACK_OFF) {
              //if (string[i].maxAddr <= 5) {
                // 1-5 MCT's
              //  data[0] = ALTERNATE_OFF;
              //}
              //else {
                // 6-10 MCT's
                data[0] = ALTERNATE_ON;
              //}  
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_ALTERNATE,
                            data);
              data[0] = MIRROR2;
              data[1] = TRACK_HOME;
              Mct485SendMsg(MSG_ORIGIN_NORMAL,
                            i,
                            j,
                            MCT_CMD_TRACK,
                            data);
            } // if TRACK_OFF
          } // for j
        } // for i
      } // else
      break;

    case FIELD_TEST_END_OF_DAY:
      // if field is requesting heat, turn it back on
      if (fieldInState & FIELD_STAT_UPS_SHUTDOWN) {
        // don't turn on field if running from UPS
      }
      else if (!(fieldInState & FIELD_STAT_INTERLOCK)) {
        // don't turn on field if interlock is open
      }
      // look for HEAT_REQ and DNI
      else if ((fieldInState & FIELD_STAT_HEAT_REQ) &&
          ((fieldInState & FIELD_STAT_DNI_HIGH) ||
           (fieldInState & FIELD_STAT_PYR_BYPASS))) {
        // Request pump
        FieldNewState(FIELD_PUMP_ON);
        break;
      }
      
      if (!fieldEndOfDayTicks) {
        // taking too long, just turn the field off
        FieldNewState(FIELD_TEST_OFF);
      }
      else {
        fieldEndOfDayTicks--;
      }
      
      // turn off the field when all MCT's are off
      for (i = 0; i < 4; i++) {
        for (j = 1; j <= string[i].maxAddr; j++) {
          if (string[i].mct[j-1].mirror[0].tracking != TRACK_OFF &&
            string[i].mct[j-1].mirror[0].tracking != TRACK_ERROR) {
            return;
          }
          if (string[i].mct[j-1].mirror[1].tracking != TRACK_OFF &&
            string[i].mct[j-1].mirror[1].tracking != TRACK_ERROR) {
            return;
          }
        }
      }
      FieldNewState(FIELD_TEST_OFF);
      break;
      
    case FIELD_TEST_OFF:
      if (!usbActivityTimeout) {
        FieldNewState(FIELD_OFF);
      }
      break;

    default:
      // shouldn't ever happen
      FieldInit();
      break;
    } // switch
  } // if ()
} // FieldUpdate()


void FieldNewState(FIELD_STATE_m newState) {
  byte i;
  byte data[2];

  if (fieldLoggingMode && newState == FIELD_OFF) {
    newState = FIELD_LOGGING;
  }
  if (!fieldLoggingMode && newState == FIELD_LOGGING) {
    fieldLoggingMode = 1;
    if (fieldState != FIELD_OFF &&
        fieldState != FIELD_TEST_OFF) {
      // don't interrupt operating states
      return;
    }
  }
  if (newState == FIELD_LOGGING_OFF) {
    fieldLoggingMode = 0;
    if (fieldState == FIELD_LOGGING) {
      // turn field off
      newState = FIELD_OFF;
    }
    else {
      // don't interfere with operation
      return;
    }
  }
  
  switch (newState) {
  case FIELD_OFF:
  case FIELD_TEST_OFF:
    // Field Test Off terminates logging mode
    fieldLoggingMode = 0;
    FCE_outputs &= ~FCE_OUT_PUMP;
    for (i = 0; i < 4; i++) {
      StringSetState(i, STRING_POWER_OFF);
    }  
    break;

  case FIELD_PUMP_ON:
    FCE_outputs |= FCE_OUT_PUMP;
    for (i = 0; i < 4; i++) {
      if (string[i].state == STRING_POWER_OFF) {
        StringSetState(i, STRING_POWER_UP);
      }
    }
    break;

  case FIELD_GO_ON_SUN:
    // send every MCT on sun
    fieldOnSunRetryTicks = 0;
    for (i = 0; i < 4; i++) {
      if (string[i].maxAddr) {
        //if (string[i].maxAddr <= 5) {
          // 1-5 MCT's
          data[0] = ALTERNATE_OFF;
        //}
        //else {
          // 6-10 MCT's
          data[0] = ALTERNATE_ON;
        //}  
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_ALTERNATE,
                      data);
        data[0] = MIRROR1;
        data[1] = TRACK_HOME;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
        data[0] = MIRROR2;
        data[1] = TRACK_HOME;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
      }
    } // for (i...)
    
    break;

  case FIELD_OPERATE:
    DataLogDateTime();
    DataLogStartofDay();
    // XYZ - hardcoded parameter
    fieldOnSunRetryTicks = 7031;
    break;

  case FIELD_END_OF_DAY:
  case FIELD_TEST_END_OF_DAY:
    // send every MCT home
    data[1] = TRACK_END;
    for (i = 0; i < 4; i++) {
      if (string[i].maxAddr) {
        data[0] = MIRROR1;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
        data[0] = MIRROR2;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
      }
    } // for (i...)
    fieldEndOfDayTicks = 7031;
    break;

  case FIELD_TEST:
  case FIELD_TEST_UPDATE:
    for (i = 0; i < 4; i++) {
      if (string[i].state == STRING_POWER_OFF) {
        StringSetState(i, STRING_POWER_UP);
      }
    }
    break;  

  case FIELD_TEST_ON_SUN:
    // send every MCT on sun
    data[1] = TRACK_HOME;
    for (i = 0; i < 4; i++) {
      if (string[i].maxAddr) {
        data[0] = MIRROR1;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
        data[0] = MIRROR2;
        Mct485SendMsg(MSG_ORIGIN_NORMAL,
                      i,
                      MCT485_ADDR_BCAST + string[i].maxAddr,
                      MCT_CMD_TRACK,
                      data);
      }
    } // for (i...)
    break;

  case FIELD_TEST_OPERATE:
    break;

  case FIELD_LOGGING:
    fieldLoggingMode = 1;
    FCE_outputs &= ~FCE_OUT_PUMP;
    for (i = 0; i < 4; i++) {
      if (string[i].state == STRING_POWER_OFF) {
        StringSetState(i, STRING_POWER_UP);
      }
    }
    break;

  default:
    // shouldn't ever happen
    return;
  } // switch
  
  fieldState = newState;
  // delay before updated
  fieldTimeout = 500;
} // FieldNewState()


void FieldUpdateOutputs(void) {
  byte i;
  byte d;
  
  PORTClearBits(IOPORT_E, PE_HC595_RCLK
                        | PE_HC595_SCLK
                        | PE_ADS1244_SCLK
                        | PE_EXP_RS485_EN);
  if (rtu485State == RTU485_TX) {
    PORTSetBits(IOPORT_E, PE_EXP_RS485_EN);
  }
  if (ads1244sclks & 0x01) {
    PORTSetBits(IOPORT_E, PE_ADS1244_SCLK);
  }
  PORTSetBits(IOPORT_A, PA_EXP_WR);
  d = FCE_outputs;
  for (i = 0; i < 8; i++) {
    if (d & 0x80) {
      PORTSetBits(IOPORT_E, PE_HC595_DIN);
    }
    else {
      PORTClearBits(IOPORT_E, PE_HC595_DIN);
    }
    PORTSetBits(IOPORT_E, PE_HC595_SCLK);
    d <<= 1;
    PORTClearBits(IOPORT_E, PE_HC595_SCLK);
  }
  // latch the 595 outputs
  PORTSetBits(IOPORT_E, PE_HC595_RCLK);
  // turn off the 573 latch
  PORTClearBits(IOPORT_A, PA_EXP_WR);
} // FieldUpdateOutputs()


void FieldReadInputs(void) {
  PORTSetBits(IOPORT_E, PE_EXP_TYPE);
  PORTSetPinsDigitalIn(IOPORT_E, PE_EXP_DATA);
  PORTClearBits(IOPORT_A, PA_EXP_RD);
  FCE_inputs = PORTReadBits(IOPORT_E, PE_EXP_DATA);
  
  // restore I/O to default state
  PORTSetBits(IOPORT_A, PA_EXP_RD);
  PORTSetPinsDigitalOut(IOPORT_E, PE_EXP_DATA);
} // FieldReadInputs()


void FieldUpdateI2C(void) {
  byte rdAddr;
  byte wrAddr;
  byte wrChan;
  
  switch (ads1115_chan) {
  case ADS1115_IN1:
  case ADS1115_IN2:
  case ADS1115_IN3:
  case ADS1115_IN4:
    rdAddr = I2C2_ADDR1;
    break;

  case ADS1115_IN5:
  case ADS1115_RTD:
  case ADS1115_FLOW_SW:
    rdAddr = I2C2_ADDR2;
    break;

  default:
    ads1115_chan = ADS1115_IN1;
    return;
  } // switch

  i2c2Queue[i2c2QueueWr].wd[0] = rdAddr;
  // conversion register
  i2c2Queue[i2c2QueueWr].wd[1] = 0x00;
  i2c2Queue[i2c2QueueWr].wn = 2;
  i2c2Queue[i2c2QueueWr].ra = rdAddr+1;
  i2c2Queue[i2c2QueueWr].rn = 2;
  i2c2Queue[i2c2QueueWr].chan = ads1115_chan;
  if (++i2c2QueueWr >= I2C2_QUEUE_SIZE) {
    i2c2QueueWr = 0;
  }

  if (++ads1115_chan >= ADS1115_MAX_CHAN) {
    ads1115_chan = ADS1115_IN1;
  }
  
  switch (ads1115_chan) {
  case ADS1115_IN1:
    wrAddr = I2C2_ADDR1;
    // AIN0, PGA = 1, single shot
    wrChan = 0xC3;
    break;

  case ADS1115_IN2:
    wrAddr = I2C2_ADDR1;
    // AIN1, PGA = 1, single shot
    wrChan = 0xD3;
    break;

  case ADS1115_IN3:
    wrAddr = I2C2_ADDR1;
    // AIN2, PGA = 1, single shot
    wrChan = 0xE3;
    break;

  case ADS1115_IN4:
    wrAddr = I2C2_ADDR1;
    // AIN3, PGA = 1, single shot
    wrChan = 0xF3;
    break;

  case ADS1115_IN5:
    wrAddr = I2C2_ADDR2;
    // AIN0, PGA = 1, single shot
    wrChan = 0xC3;
    break;

  case ADS1115_RTD:
    wrAddr = I2C2_ADDR2;
    // AIN0, PGA = 1, single shot
    wrChan = 0xD3;
    break;

  case ADS1115_FLOW_SW:
    wrAddr = I2C2_ADDR2;
    // AIN0, PGA = 1, single shot
    wrChan = 0xE3;
    break;

  default:
    ads1115_chan = ADS1115_IN1;
    return;
  }

  i2c2Queue[i2c2QueueWr].wd[0] = wrAddr;
  // config register
  i2c2Queue[i2c2QueueWr].wd[1] = 0x01;
  i2c2Queue[i2c2QueueWr].wd[2] = wrChan;
  // 250SPS, disable comparator
  i2c2Queue[i2c2QueueWr].wd[3] = 0xA3;
  i2c2Queue[i2c2QueueWr].wn = 4;
  i2c2Queue[i2c2QueueWr].rn = 0;
  if (++i2c2QueueWr >= I2C2_QUEUE_SIZE) {
    i2c2QueueWr = 0;
  }
} // FieldUpdateI2C()


void FieldAttenUpdate(void) {
#if 0
  ushort adcValue;
  ushort percent;
  
  // Full scale is 37.128V = 32767
  
  adcValue = fieldAttenPercent * 
  adcValue += 411;
  percent = 0;
  while (adcValue)
  percent = 90;
  while (percent > fieldAttenPercent) {
    
  }
  if (fieldAttenTimeout) {
    fieldAttenTimeout--;
  }
  
#endif  
} // FieldAttenUpdate()
