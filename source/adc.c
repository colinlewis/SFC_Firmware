#include "SFC.h"
#include <plib.h>


void AdcInit(void) {
  PORTClearBits(IOPORT_G, PG_DESSICANT_FAN_PWM | PG_UNUSED);
  PORTSetPinsDigitalOut(IOPORT_G,PG_DESSICANT_FAN_PWM | PG_UNUSED);
  
  CloseADC10();
  OpenADC10(ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON,
            ADC_VREF_EXT_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_14 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF,
            ADC_CONV_CLK_INTERNAL_RC | ADC_SAMPLE_TIME_15,
            ENABLE_AN2_ANA + ENABLE_AN3_ANA +
            ENABLE_AN4_ANA + ENABLE_AN5_ANA +
            ENABLE_AN6_ANA + ENABLE_AN7_ANA +
            ENABLE_AN8_ANA + ENABLE_AN9_ANA +
            ENABLE_AN10_ANA + ENABLE_AN11_ANA +
            ENABLE_AN12_ANA + ENABLE_AN13_ANA +
            ENABLE_AN14_ANA + ENABLE_AN15_ANA,
            SKIP_SCAN_AN0 + SKIP_SCAN_AN1);
  SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN6 |  ADC_CH0_NEG_SAMPLEB_NVREF | ADC_CH0_POS_SAMPLEB_AN7);
  EnableADC10();
} // AdcInit()


void AdcUpdate(void) {
  sfcHumidity = AdcConvHumidity(ReadADC10(AN_HUMIDITY-2));
  sfcThermistor = AdcConvThermistor(ReadADC10(AN_THERMISTOR-2));
} // AdcUpdate()


extern const ushort AdcHumidityTable[21];
ushort AdcConvHumidity(ushort adcHumidity) {
  byte i;
  ushort humidity;
  
  if (adcHumidity <= AdcHumidityTable[0]/2) {
    return 0;  // 0.0% - not connected
  }
  else if (adcHumidity <= AdcHumidityTable[0]) {
    return 1;  // 0.1%
  }
  else if (adcHumidity >= AdcHumidityTable[20]) {
    return 1001;  // 100.0%
  }
  
  i = 1;
  while (i < 20 && adcHumidity >= AdcHumidityTable[i]) {
    i++;
  }
  humidity = AdcHumidityTable[i];
  i--;
  humidity -= AdcHumidityTable[i];
  adcHumidity -= AdcHumidityTable[i];
  if (humidity > 0) {
    // table entries are every 5%, interpolate
    adcHumidity *= 50;
    adcHumidity /= humidity;
  }
  
  humidity = i;
  humidity *= 50;
  humidity += adcHumidity;
  
  return humidity;
} // AdcConvHumidity()


extern const short AdcThermistorTable[33];
ushort AdcConvThermistor(ushort adcThermistor) {
  byte i;
  short degC;
  
  if (adcThermistor <= 48) {
    return 32767;  // too low, may be shorted
  }
  else if (adcThermistor >= 980) {
    return -32768; // too high, probably open
  }

  i = (byte)(adcThermistor >> 5);
  i++;
  degC = AdcThermistorTable[i];
  i--;
  degC -= AdcThermistorTable[i];
  adcThermistor &= 31;
  // table entries are every 32 adc counts, interpolate
  degC *= adcThermistor;
  degC >>= 5;
  degC += AdcThermistorTable[i];
  
  return degC;
} // AdcConvThermistor()


const ushort AdcHumidityTable[21] = {
  236, //   0%
  267, //   5%
  299, //  10%
  330, //  15%
  360, //  20%
  391, //  25%
  419, //  30%
  448, //  35%
  475, //  40%
  503, //  45%
  532, //  50%
  559, //  55%
  587, //  60%
  615, //  65%
  644, //  70%
  673, //  75%
  704, //  80%
  735, //  85%
  767, //  90%
  801, //  95%
  835  // 100%
};


const short AdcThermistorTable[33] = {
  1660, //    0
  1400, //   32
  1146, //   64
   989, //   96
   846, //  128
   748, //  160
   687, //  192
   629, //  224
   574, //  256
   522, //  288
   474, //  320
   429, //  352
   388, //  384
   350, //  416
   318, //  448
   283, //  480
   250, //  512
   218, //  544
   186, //  576
   154, //  608
   123, //  640
    91, //  672
    59, //  704
    26, //  736
   -10, //  768
   -42, //  800
   -81, //  832
  -124, //  864
  -173, //  896
  -250, //  928
  -311, //  960
  -419, //  992
  -469  // 1024
};
