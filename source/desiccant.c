#include "sfc.h"
#include "field.h"
#include "param.h"
#include "rtc.h"

//local helper functions
static void AtThisTimeGoto(ulong theTime, mDESICCANT_STATE theState);
static void DessicantRegenStateUpdate();
static void UpdateHeater(ulong desiredTemp);

//local variables
static byte heaterDutyCycle;
static byte heaterDutyCounter;
static byte fanDutyCycle; // 0- 8
static long cumulativeError;

void DesiccantInit(void) {
  if (param[PARAM_DESICCANT_MODE] == DESICCANT_OFF) {
    DesiccantNewState(DESICCANT_OFF, 0);
  }
  else {
    DesiccantNewState(DESICCANT_CLOSED, 0);
  } 
} // DesiccantInit()


// called once a second
void DesiccantUpdate(void) {
  switch (desiccantState) {
  case DESICCANT_OFF:
    // in this state we don't call DesiccantCheckTime, so we 
    // will not switch to another state unless forced.
    FCE_outputs &= ~FCE_OUT_VALVE;
    FCE_outputs &= ~FCE_OUT_HEAT;
    fanDutyCycle = 0;
    break;

  case DESICCANT_DRYING:
    FCE_outputs &= ~FCE_OUT_VALVE;
    FCE_outputs &= ~FCE_OUT_HEAT;
    DesiccantCheckTime();
    break;

  case DESICCANT_REGEN:
    DessicantRegenStateUpdate();
    break;

  case DESICCANT_CLOSED:
    FCE_outputs |=  FCE_OUT_VALVE;
    FCE_outputs &= ~FCE_OUT_HEAT;
    fanDutyCycle = 0;
    DesiccantCheckTime();
    break;

  case DESICCANT_MANUAL:
    //protect against over heating in manual mode  
    if (fceRTD > param[PARAM_DESICCANT_REGEN_OUTLET_TEMP3]) {
      // RTD > process end
      FCE_outputs &= ~FCE_OUT_HEAT;
    }

    if (!usbActivityTimeout) {
      DesiccantNewState(DESICCANT_DRYING, 0);
    }
    break;

  default:
    DesiccantNewState(DESICCANT_OFF, 0);
    break;
  }
} // DesiccantUpdate()


// Regular update while in the Regen state.
//TODO - need a substate where we are cooling before we exit regen.
void DessicantRegenStateUpdate()
{
  ulong outletTemp = fceRTD;
  if (outletTemp < param[PARAM_DESICCANT_REGEN_OUTLET_TEMP1])
    UpdateHeater(param[PARAM_DESICCANT_REGEN_INLET_TEMP1]);
       
  else if (outletTemp < param[PARAM_DESICCANT_REGEN_OUTLET_TEMP2])
    UpdateHeater(param[PARAM_DESICCANT_REGEN_INLET_TEMP2]);
    
  else if (outletTemp < param[PARAM_DESICCANT_REGEN_OUTLET_TEMP3])
    UpdateHeater(param[PARAM_DESICCANT_REGEN_INLET_TEMP3]);
    
  else //exceeded temp3, we are done
    DesiccantNewState(DESICCANT_CLOSED, 0);
 }

// set the HEAT output based on desired temperature (PD control)
//  desiredTemp in .1C units. (1000 == 100C)
// Called once a second
//TODO make sure we set cumulative error to 0, duty cycle to 50% when we start the heater code.

#define HEATER_PERIOD 30 // number of seconds in a single cycle of the heater control.
void UpdateHeater(ulong desiredTemp)
{
  long inletTemp = 0; //TODO read this value from an RTD.
  
  // error values are in .1 degree C units
  long error = inletTemp - (long) desiredTemp;  //make sure this is a signed operation.
  cumulativeError += error;
  
  // set the HEAT output based on the duty cycle
  heaterDutyCounter++;
  if (heaterDutyCounter >= HEATER_PERIOD) {
    // Every period, recalculate new duty cycle based on measurements over the last period
    
    //These params are fixed point signed numbers (8.8).
    long kP =(long) param[PARAM_DESICCANT_HEATER_GAIN_PROPORTIONAL]; 
    long kI =(long) param[PARAM_DESICCANT_HEATER_GAIN_INTEGRAL]; 
    long correction = (error * kP + cumulativeError * kI / HEATER_PERIOD )/ (1<<8); // divide to scale values to 8.8
    if (correction > heaterDutyCycle)
      heaterDutyCycle = 0;
    else if ((long) heaterDutyCycle - correction > HEATER_PERIOD)
      heaterDutyCycle = HEATER_PERIOD;
    else
      heaterDutyCycle = heaterDutyCycle - correction;
    cumulativeError = 0; // reset for period
    heaterDutyCounter = 0;
  }
  
  if (heaterDutyCounter < heaterDutyCycle) {
    FCE_outputs |= FCE_OUT_HEAT;
  }
  else {
    FCE_outputs &= ~FCE_OUT_HEAT;
  }
  
  DesiccantCheckTime();
}



// Switch to a new state.
// parameter 'outputs' is only used for Manual State;
// specifies a new value for FCE_outputs.
void DesiccantNewState(mDESICCANT_STATE newState, byte outputs) {
  switch (newState) {
  case DESICCANT_OFF:
    FCE_outputs &= ~FCE_OUT_VALVE;
    FCE_outputs &= ~FCE_OUT_HEAT;
    if (param[PARAM_DESICCANT_MODE] != DESICCANT_OFF) {
      // change param so default state is OFF
      ParamWrite(PARAM_DESICCANT_MODE, DESICCANT_OFF);
    }
    break;

  case DESICCANT_DRYING:
    heaterDutyCycle = 0;

    if (fceRTD < param[PARAM_DESICCANT_MIN_FAN_TEMP]) {
      fanDutyCycle = param[PARAM_DESICCANT_DRYING_FAN_DUTYCYCLE];
    }
    else {
      fanDutyCycle = 0;
    }
    
    FCE_outputs &= ~FCE_OUT_VALVE;
    FCE_outputs &= ~FCE_OUT_HEAT;
    if (param[PARAM_DESICCANT_MODE] == DESICCANT_OFF) {
      // change param so default state is ON
      ParamWrite(PARAM_DESICCANT_MODE, DESICCANT_DRYING);
    }
    break;

  case DESICCANT_REGEN:
    heaterDutyCounter = 0;
    heaterDutyCycle = 10;
    cumulativeError = 0;
    fanDutyCycle = param[PARAM_DESICCANT_REGEN_FAN_DUTYCYCLE];
    FCE_outputs |=  FCE_OUT_VALVE;
    FCE_outputs |=  FCE_OUT_HEAT;
    
    if (param[PARAM_DESICCANT_MODE] == DESICCANT_OFF) {
      // change param so default state is ON
      ParamWrite(PARAM_DESICCANT_MODE, DESICCANT_DRYING);
    }
    break;

  case DESICCANT_CLOSED:
    FCE_outputs |=  FCE_OUT_VALVE;
    if (param[PARAM_DESICCANT_MODE] == DESICCANT_OFF) {
      // change param so default state is ON
      ParamWrite(PARAM_DESICCANT_MODE, DESICCANT_DRYING);
    }
    break;

  case DESICCANT_MANUAL:
    outputs &= (FCE_OUT_FAN + FCE_OUT_VALVE + FCE_OUT_HEAT); // only allow 1's in relevant bits
    FCE_outputs &= ~(FCE_OUT_FAN + FCE_OUT_VALVE + FCE_OUT_HEAT); // turn off by default
    FCE_outputs |= outputs; // turn on specified bits.
    if (outputs & FCE_OUT_FAN)
      fanDutyCycle = param[PARAM_DESICCANT_DRYING_FAN_DUTYCYCLE];
    break;

  default:
    return;
  }
  
  desiccantState = newState;
} // DesiccantNewState()


void DesiccantCheckTime(void) {
  
  int t2state = DESICCANT_CLOSED; // low humidity
  if (sfcHumidity > param[PARAM_DESICCANT_REGEN_HUMIDITY])
    t2state = DESICCANT_REGEN;
    
	AtThisTimeGoto (param[PARAM_DESICCANT_T1], DESICCANT_DRYING);
	AtThisTimeGoto (param[PARAM_DESICCANT_T2], t2state); // closed or regen.
	AtThisTimeGoto (param[PARAM_DESICCANT_T3], DESICCANT_CLOSED);
}


// Helper for DessicantCheckTime
void AtThisTimeGoto(ulong theTime, mDESICCANT_STATE theState) {
  if (rtcHour == (theTime >> 8) &&
    rtcMin == (theTime & 0xFF) && 
    desiccantState != theState) {

      DesiccantNewState(theState, 0);
  } 
}

// called once a millisecond to regulate fan speed
// we have 8 steps of PWM (8ms => 125 Hz). We also have "off", so 9 possible settings (0-8)
// Note that fan does not go completely off unless you turn off power.
void DessicantFanPWMupdate(void) {

  if (fanDutyCycle ==0) {
    FCE_outputs  &= ~FCE_OUT_FAN; // power off
  }  
  else {
    FCE_outputs |= FCE_OUT_FAN; // power fan on 
    
    // each 4 ticks (4ms) emit a pulse  
    if ((sysTicks & 0x0007) < fanDutyCycle)
     PORTSetBits(IOPORT_G,PG_DESSICANT_FAN_PWM);
    else
     PORTClearBits(IOPORT_G,PG_DESSICANT_FAN_PWM);
  }
}
