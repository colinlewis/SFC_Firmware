
#define EEPROM_I2C_WR_ADDR  0xA0
#define EEPROM_I2C_RD_ADDR  0xA1

#define NUM_PARAMS 128

#define PARAM_DESICCANT_T1                  0 // time (hi byte = hour, low byte minute) in afternoon to begin drying
#define PARAM_DESICCANT_T2                  1 // time in morning to end drying
#define PARAM_DESICCANT_T3                  2 // time in early afternoon to close dessicant control
#define PARAM_DESICCANT_MODE                3 // override to Desiccant logic. Allows control to force a mode.
#define PARAM_DESICCANT_MAX_TEMP            4 // maximum temperature allowed (unit is .1 deg C; 1000 == 100 degrees C)
#define PARAM_DESICCANT_TEMP_HYST           5 // currently unused
#define PARAM_RTD_ZERO                      6 // calibration value (see RTDconvert)
#define PARAM_RTD_SPAN                      7 // calibration value (see RTD convert)
#define PARAM_DESICCANT_REGEN_HUMIDITY      8 // relative humidity threshold. If relative humidity is above this, regen is necessary. In .1 % units (10 = 1% relative humidity

// new names for old params. Same measurement, but instead of picking a duty cycle, we are picking a desired inlet temp.
//#define PARAM_DESICCANT_DUTY1_TEMP          9 // deprecated
//#define PARAM_DESICCANT_DUTY2_TEMP          10 // deprecated
//#define PARAM_DESICCANT_DUTY3_TEMP          11 // deprecated
#define PARAM_DESICCANT_REGEN_OUTLET_TEMP1  9 // desired outlet temperature marking end of phase 1 of the regen.
#define PARAM_DESICCANT_REGEN_OUTLET_TEMP2  10 // desired outlet temperature marking end of phase 2 of the regen.
#define PARAM_DESICCANT_REGEN_OUTLET_TEMP3  11 // desired outlet temperature marking end of the regeneration phase.

#define PARAM_DESICCANT_MIN_FAN_TEMP        12 // during drying, temperature must be below (?) this to turn on fan. 
#define PARAM_DESICCANT_REGEN_INLET_TEMP1   13 // desired inlet temperature for phase 1 of the regen. Controlled by duty cycle of heater.
#define PARAM_DESICCANT_REGEN_INLET_TEMP2   14 // desired inlet temperature for phase 2 of the regen. controlled by duty cycle of heater.
#define PARAM_DESICCANT_REGEN_INLET_TEMP3   15 // desired inlet temperature for phase 3 of the regen. controlled by duty cycle of heater.
#define PARAM_DESICCANT_REGEN_FAN_DUTYCYCLE     16 // 0 = off, 1 = 6000 rpm, 8 = 16000 rpm
#define PARAM_DESICCANT_DRYING_FAN_DUTYCYCLE    17 // 0- 8
#define PARAM_DESICCANT_COOLING_FAN_DUTYCYCLE   18 // 0- 8
#define PARAM_DESICCANT_COOLING_INLET_TEMP    19 // Cooling phase runs until inlet temp drops to this value AND ...
#define PARAM_DESICCANT_COOLING_OUTLET_TEMP   20 // ... outlet temp falls below this value. (unit is .1 deg C; 1000 == 100 degrees C)

#define PARAM_DESICCANT_HEATER_GAIN_PROPORTIONAL 21 // units TBD
#define PARAM_DESICCANT_HEATER_GAIN_INTEGRAL 22     // units TBD




EXTERN ulong param[NUM_PARAMS];
EXTERN byte paramInitComplete;

void ParamInit(void);
void ParamRead(byte num);
void ParamProcessResp(byte index);
void ParamWrite(byte paramNum, ulong newVal);
