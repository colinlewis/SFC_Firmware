#include "sfc.h"
#include "ads1244.h"
#include "field.h"
#include "rtu.h"

void Ads1244Init(void) {
  ads1244data = 0;
  ads1244sclks = 0;
} // Ads1244Init()


// conversion takes 66msec
// If called once/msec, this routine takes 50msec to shift out data
void Ads1244Update(void) {
  if (ads1244sclks < 50) {
    ads1244sclks++;
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
    PORTClearBits(IOPORT_A, PA_EXP_WR);
    if (ads1244sclks <= 48) {
      // data shifts out on first 24 clocks
      if (!(ads1244sclks & 0x01)) {
        ads1244data <<= 1;
        // data valid at falling edge
        if (FCE_inputs & PE_ADS1244_DOUT) {
          // DOUT high
          ads1244data++;
        }
      } // falling edge  
    } // ads1244sclks <= 48
    else if (ads1244sclks == 49) {
      // process result
      if (ads1244data & 0x00800000L) {
        // negative result
        fieldDNI = 0;
      }
      else {
        ads1244data *= 256;
        ads1244data += ADCpWpm2/2;
        ads1244data /= ADCpWpm2;
        if (ads1244data <= MAX_DNI) {
          fieldDNI = (ushort)ads1244data;
          // hardcoded for 5V operation
          if (fieldInState & FIELD_STAT_DNI_HIGH) {
            if (fieldDNI < DNI_STOP) {
              fieldInState &= ~FIELD_STAT_DNI_HIGH;
            }
          }
          else {
            if (fieldDNI > DNI_START) {
              fieldInState |= FIELD_STAT_DNI_HIGH;
            }
          }
        }
      }  
    } // ads1244sclks == 48  
  } // ads1244sclks < 50
  else {
    // make sure something gets clocked
    
    if (++ads1244timeout >= 100) {
      ads1244data = 0;
      ads1244timeout = 0;
    }
    // wait for DOUT to go low
    PORTSetPinsDigitalIn(IOPORT_E, PE_EXP_DATA);
    PORTClearBits(IOPORT_A, PA_EXP_RD);
    if (!(PORTReadBits(IOPORT_E, PE_EXP_DATA) & PE_ADS1244_DOUT)) {
      // DOUT high
      ads1244sclks = 0;
      ads1244data = 0;
    }
    // restore I/O to default state
    PORTSetBits(IOPORT_A, PA_EXP_RD);
    PORTSetPinsDigitalOut(IOPORT_E, PE_EXP_DATA);
  } // ads1244sclks == 50
} // Ads1244Update
