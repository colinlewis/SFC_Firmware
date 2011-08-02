#include "sfc.h"
#include "datalog.h"
#include "field.h"
#include "mctbus.h"
#include "rtc.h"
#include "track.h"


// The datalog is stored in its own section, declared in the linker script
DATA_LOG_t dataLog[DATA_LOG_MAX_ENTRY] __attribute__((section(".datalog")));


void DataLogInit(void) {
  ushort pageID;
  ushort entry;

  mBMXSetFlashUserPartition(0x20000);
  DataLogClearData();  
  dataLogNextEmpty = 0xFFFF;
  entry = 0;
  // look for valid log pages
  while (entry < DATA_LOG_MAX_ENTRY) {
    if (DataLogPageValid(entry) == 1) {
      pageID = dataLog[entry].data[0];
      pageID <<= 8;
      pageID |= dataLog[entry].data[1];
      if (dataLogNextEmpty == 0xFFFF) {
        // this is the first valid page
        dataLogNextEmpty = entry;
        dataLogPageID = pageID;
      }
      else {
        if (pageID == dataLogPageID+1) {
          // this is the next page
          dataLogNextEmpty = entry;
          dataLogPageID = pageID;
        }
        else {
          // this page is not valid, stop
          entry = DATA_LOG_MAX_ENTRY;
        }
      }
    } // if (page valid)
    entry += (DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t));
  } // while ()

  if (dataLogNextEmpty != 0xFFFF) {
    // found valid pages
    entry = dataLogNextEmpty + (DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t));
    dataLogNextEmpty++;
    while (dataLogNextEmpty < entry) {
      if (dataLog[dataLogNextEmpty].type == DLOG_BLANK) {
        // found the next blank entry
        break;
      }
      dataLogNextEmpty++;
    }
    if (dataLogNextEmpty == entry) {
      // page is full, start a new one
      DataLogStartPage();
    }
  }
  else {
    // page is full, start a new one
    dataLogNextEmpty = 0;
    DataLogStartPage();
  }
} // DataLogInit()


byte DataLogPageValid(ushort entry) {
  DATA_LOG_t log;
  
  log.type = dataLog[entry].type;
  log.subtype = dataLog[entry].subtype;
  log.data[0] = dataLog[entry].data[0];
  log.data[1] = dataLog[entry].data[1];
  log.data[2] = dataLog[entry].data[2];
  log.data[3] = dataLog[entry].data[3];
  log.data[4] = dataLog[entry].data[4];
  log.data[5] = dataLog[entry].data[5];
  if (log.type != DLOG_PAGE_ID) {
    return 2;
  }
  if (log.subtype != 0xFE) {
    return 3;
  }
  if (log.data[0] != (byte)(~log.data[2])) {
    return 4;
  }
  if (log.data[1] != (byte)(~log.data[3])) {
    return 5;
  }
  if (log.data[4] != 0x55) {
    return 6;
  }
  if (log.data[5] != 0xAA) {
    return 7;
  }
//  if (log.type == DLOG_PAGE_ID &&
//      log.subtype == 0xFE &&
//      log.data[0] == ~log.data[2] &&
//      log.data[1] == ~log.data[3] &&
//      log.data[4] == 0x55 &&
//      log.data[5] == 0xAA) {
//    // page is valid
//    return 1;
//  }
  
  // page is not valid
  return 1;
} // DataLogPageValid()


void DataLogStartPage(void) {
  DATA_LOG_t logEntry;
  
  if (dataLogNextEmpty >= DATA_LOG_MAX_ENTRY) {
    // wrapped around
    dataLogNextEmpty = 0;
  }
  
  // erase the flash page
  NVMErasePage(&dataLog[dataLogNextEmpty]);
  
  // start the page with an ID entry
  dataLogPageID++;
  logEntry.type = DLOG_PAGE_ID;
  logEntry.subtype = 0xFE;
  logEntry.data[0] = (byte)(dataLogPageID >> 8);
  logEntry.data[1] = (byte)dataLogPageID;
  logEntry.data[2] = ~logEntry.data[0];
  logEntry.data[3] = ~logEntry.data[1];
  logEntry.data[4] = 0x55;
  logEntry.data[5] = 0xAA;
  DataLogAddEntry(&logEntry);
} // DataLogStartPage()


void DataLogAddEntry(DATA_LOG_t *logEntry) {
  NVMWriteWord(&dataLog[dataLogNextEmpty], ((ulong *)logEntry)[0]);
  NVMWriteWord((byte *)&dataLog[dataLogNextEmpty]+4, ((ulong *)logEntry)[1]);
  
  // advance to the next location
  dataLogNextEmpty++;
  if (dataLogNextEmpty >= DATA_LOG_MAX_ENTRY) {
    // wrapped around
    dataLogNextEmpty = 0;
    DataLogStartPage();
  }
  else if (!(dataLogNextEmpty & ((DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t))-1))) {
    // advanced into the next page
    DataLogStartPage();
  }
} // DataLogAddEntry()


// find the index of the oldest page in the log
void DataLogFindFirstEntry(void) {
  ushort entry;
  ushort i;
  
  // find first valid page
  entry = dataLogNextEmpty & ((DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t))-1);
  for (i = 0; i < DATA_LOG_MAX_ENTRY/DATA_LOG_PAGE_SIZE; i++) {
    entry += (DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t));
    if (entry >= DATA_LOG_MAX_ENTRY) {
      entry = 0;
    }
    if (DataLogPageValid(entry)) {
      dataLogNextRead = entry;
      return;
    }
  }
  
  // did not find a valid page
  dataLogNextEmpty = 0;
  DataLogStartPage();
  dataLogNextRead = 0;
} // DataLogFindFirstEntry()


void DataLogErase(void) {
  ushort i;
  
  for (i = 0; i < DATA_LOG_MAX_ENTRY; i += DATA_LOG_PAGE_SIZE / sizeof(DATA_LOG_t)) {
    // erase the flash page
    NVMErasePage(&dataLog[i]);
  }
  
  // start over with an empty log
  dataLogNextEmpty = 0;
  DataLogStartPage();
  dataLogNextRead = 0;
} // DataLogErase()


byte DataLogReadNext(byte *buffer) {
  byte *entry;
  byte i;
  
  if (dataLogNextEmpty == dataLogNextRead) {
    // log has been read
    return 0;
  }
  
  entry = (byte *)&dataLog[dataLogNextRead];
  for (i = 0; i < sizeof(DATA_LOG_t); i++) {
    *buffer++ = *entry++;
  }
  
  dataLogNextRead++;
  if (dataLogNextRead >= DATA_LOG_MAX_ENTRY) {
    dataLogNextRead = 0;
  }
  
  return sizeof(DATA_LOG_t);
}


void DataLogDateTime(byte subtype) {
  DATA_LOG_t logEntry;
  
  logEntry.type = DLOG_DATE_TIME;
  logEntry.subtype = subtype;
  logEntry.data[0] = rtcYear;
  logEntry.data[1] = rtcMonth;
  logEntry.data[2] = rtcDate;
  logEntry.data[3] = rtcHour;
  logEntry.data[4] = rtcMin;
  logEntry.data[5] = rtcSec;
  DataLogAddEntry(&logEntry);
  
  dataLogLastHour = rtcHour;
} // DataLogDateTime()


void DataLogStartofDay(void) {
  DATA_LOG_t logEntry;
  
  logEntry.type = DLOG_START_OF_DAY;
  logEntry.subtype = 0;
  logEntry.data[0] = rtcMin;
  logEntry.data[1] = rtcSec;
  logEntry.data[2] = string[0].maxAddr;
  logEntry.data[3] = string[1].maxAddr;
  logEntry.data[4] = string[2].maxAddr;
  logEntry.data[5] = string[3].maxAddr;
  DataLogAddEntry(&logEntry);
} // DataLogStartofDay()


void DataLogHourly(void) {
  DATA_LOG_t logEntry;

  if (dataLogLastMin != rtcMin) {
    dataLogLastMin = rtcMin;
    DataLogUpdateData();
  }
  if (dataLogLastHour == rtcHour) {
    // not time for another log entry
    return;
  }
  
  dataLogLastHour = rtcHour;
  
  if (dataLogSumCount == 0) {
    DataLogClearData();
    return;
  }
  
  // time stamp the log
  DataLogDateTime(DLOG_HOURLY_STAMP);

  dataLogInsolSum /= dataLogSumCount;
  logEntry.type = DLOG_HOURLY_DATA;
  logEntry.subtype = DLOG_INSOLATION;
  logEntry.data[0] = (byte)(dataLogInsolSum >> 8);
  logEntry.data[1] = (byte)dataLogInsolSum;
  logEntry.data[2] = (byte)(dataLogInsolMin >> 8);
  logEntry.data[3] = (byte)dataLogInsolMin;
  logEntry.data[4] = (byte)(dataLogInsolMax >> 8);
  logEntry.data[5] = (byte)dataLogInsolMax;
  DataLogAddEntry(&logEntry);

  dataLogMirPosnSum /= dataLogSumCount;
  logEntry.subtype = DLOG_AVG_MIR_POSN;
  logEntry.data[0] = (byte)(dataLogMirPosnSum >> 8);
  logEntry.data[1] = (byte)dataLogMirPosnSum;
  logEntry.data[2] = (byte)(dataLogMirPosnMin >> 8);
  logEntry.data[3] = (byte)dataLogMirPosnMin;
  logEntry.data[4] = (byte)(dataLogMirPosnMax >> 8);
  logEntry.data[5] = (byte)dataLogMirPosnMax;
  DataLogAddEntry(&logEntry);

  dataLogPowerSum /= dataLogSumCount;
  logEntry.subtype = DLOG_POWER_OUTPUT;
  logEntry.data[0] = (byte)(dataLogPowerSum >> 8);
  logEntry.data[1] = (byte)dataLogPowerSum;
  logEntry.data[2] = (byte)(dataLogPowerMin >> 8);
  logEntry.data[3] = (byte)dataLogPowerMin;
  logEntry.data[4] = (byte)(dataLogPowerMax >> 8);
  logEntry.data[5] = (byte)dataLogPowerMax;
  DataLogAddEntry(&logEntry);

  dataLogInletSum /= dataLogSumCount;
  logEntry.subtype = DLOG_INLET_TEMP;
  logEntry.data[0] = (byte)(dataLogInletSum >> 8);
  logEntry.data[1] = (byte)dataLogInletSum;
  logEntry.data[2] = (byte)(dataLogInletMin >> 8);
  logEntry.data[3] = (byte)dataLogInletMin;
  logEntry.data[4] = (byte)(dataLogInletMax >> 8);
  logEntry.data[5] = (byte)dataLogInletMax;
  DataLogAddEntry(&logEntry);

  dataLogOutletSum /= dataLogSumCount;
  logEntry.subtype = DLOG_OUTLET_TEMP;
  logEntry.data[0] = (byte)(dataLogOutletSum >> 8);
  logEntry.data[1] = (byte)dataLogOutletSum;
  logEntry.data[2] = (byte)(dataLogOutletMin >> 8);
  logEntry.data[3] = (byte)dataLogOutletMin;
  logEntry.data[4] = (byte)(dataLogOutletMax >> 8);
  logEntry.data[5] = (byte)dataLogOutletMax;
  DataLogAddEntry(&logEntry);

  logEntry.subtype = DLOG_SFC_TEMP;
  logEntry.data[0] = (byte)(sfcThermistor >> 8);
  logEntry.data[1] = (byte)sfcThermistor;
  logEntry.data[2] = (byte)(sfcHumidity >> 8);
  logEntry.data[3] = (byte)sfcHumidity;
  logEntry.data[4] = (byte)(sfcPcbTemp >> 8);
  logEntry.data[5] = (byte)sfcPcbTemp;
  DataLogAddEntry(&logEntry);

  logEntry.subtype = DLOG_MCT_TEMP;
  logEntry.data[0] = dataLogMctTMinLoc;
  logEntry.data[1] = dataLogMctTMaxLoc;
  logEntry.data[2] = (byte)(dataLogMctTempMin >> 8);
  logEntry.data[3] = (byte)dataLogMctTempMin;
  logEntry.data[4] = (byte)(dataLogMctTempMax >> 8);
  logEntry.data[5] = (byte)dataLogMctTempMax;
  DataLogAddEntry(&logEntry);

  logEntry.subtype = DLOG_MCT_HUMIDITY;
  logEntry.data[0] = dataLogMctHMinLoc;
  logEntry.data[1] = dataLogMctHMaxLoc;
  logEntry.data[2] = (byte)(dataLogMctHumidMin >> 8);
  logEntry.data[3] = (byte)dataLogMctHumidMin;
  logEntry.data[4] = (byte)(dataLogMctHumidMax >> 8);
  logEntry.data[5] = (byte)dataLogMctHumidMax;
  DataLogAddEntry(&logEntry);
  
  DataLogClearData();
} // DataLogHourly()


void DataLogClearData(void) {
  dataLogSumCount = 0;
  dataLogInsolSum = 0;
  dataLogInsolMin = 0xFFFF;
  dataLogInsolMax = 0;
  dataLogMirPosnSum = 0;
  dataLogMirPosnMin = 0xFFFF;
  dataLogMirPosnMax = 0;
  dataLogPowerSum = 0;
  dataLogPowerMin = 0xFFFF;
  dataLogPowerMax = 0;
  dataLogInletSum = 0;
  dataLogInletMin = 0x7FFF;
  dataLogInletMax = 0x8000;
  dataLogOutletSum = 0;
  dataLogOutletMin = 0x7FFF;
  dataLogOutletMax = 0x8000;
  dataLogMctTempMin = 0x7FFF;
  dataLogMctTMinLoc = 0;
  dataLogMctTempMax = 0x8000;
  dataLogMctTMaxLoc = 0;
  dataLogMctHumidMin = 0xFFFF;
  dataLogMctHMinLoc = 0;
  dataLogMctHumidMax = 0;
  dataLogMctHMaxLoc = 0;
} // dataLogMctTempMin()


void DataLogUpdateData(void) {
  byte i, j, n;
  byte tracking;
  ushort posn;
  ushort temp;
  unsigned long mirPosn;
  
  dataLogSumCount++;
  dataLogInsolSum += fieldDNI;
  if (fieldDNI < dataLogInsolMin) dataLogInsolMin = fieldDNI;
  if (fieldDNI > dataLogInsolMax) dataLogInsolMax = fieldDNI;
  
  n = 0;
  mirPosn = 0;
  for (i = 0; i < 4; i++) {
    for (j = 1; j <= string[i].maxAddr; j++) {
      tracking = string[i].mct[j].mirror[0].tracking;
      if (tracking != TRACK_OFF && tracking != TRACK_ERROR) {
        posn = string[i].mct[j].mirror[0].m[0].posn;
        mirPosn += posn;
        if (posn < dataLogMirPosnMin) dataLogMirPosnMin = posn;
        if (posn > dataLogMirPosnMax) dataLogMirPosnMax = posn;
        posn = string[i].mct[j].mirror[0].m[1].posn;
        mirPosn += posn;
        if (posn < dataLogMirPosnMin) dataLogMirPosnMin = posn;
        if (posn > dataLogMirPosnMax) dataLogMirPosnMax = posn;
        n += 2;
      }  
      tracking = string[i].mct[j].mirror[1].tracking;
      if (tracking != TRACK_OFF && tracking != TRACK_ERROR) {
        posn = string[i].mct[j].mirror[1].m[0].posn;
        mirPosn += posn;
        if (posn < dataLogMirPosnMin) dataLogMirPosnMin = posn;
        if (posn > dataLogMirPosnMax) dataLogMirPosnMax = posn;
        posn = string[i].mct[j].mirror[1].m[1].posn;
        mirPosn += posn;
        if (posn < dataLogMirPosnMin) dataLogMirPosnMin = posn;
        if (posn > dataLogMirPosnMax) dataLogMirPosnMax = posn;
        n += 2;
      }  
    }
  }
  if (n > 0) {
    mirPosn /= n;
  }
  dataLogMirPosnSum += (ushort)mirPosn;
  
  dataLogPowerSum += fieldPower;
  if (fieldPower < dataLogPowerMin) dataLogPowerMin = fieldPower;
  if (fieldPower > dataLogPowerMax) dataLogPowerMax = fieldPower;
  
  dataLogInletSum += fieldInlet;
  if (fieldInlet < dataLogInletMin) dataLogInletMin = fieldInlet;
  if (fieldInlet > dataLogInletMax) dataLogInletMax = fieldInlet;
  
  dataLogOutletSum += fieldOutlet;
  if (fieldOutlet < dataLogOutletMin) dataLogOutletMin = fieldOutlet;
  if (fieldOutlet > dataLogOutletMax) dataLogOutletMax = fieldOutlet;

  for (i = 0; i < 4; i++) {
    for (j = 1; j <= string[i].maxAddr; j++) {
      temp = string[i].mct[j].chan[MCT_LOCAL_TEMPA];
      if (temp < dataLogMctTempMin) {
        dataLogMctTempMin = temp;
        dataLogMctTMinLoc = i << 6;
        dataLogMctTMinLoc |= (j << 2);
      }
      if (temp > dataLogMctTempMax) {
        dataLogMctTempMax = temp;
        dataLogMctTMaxLoc = i << 6;
        dataLogMctTMaxLoc |= (j << 2);
      }
      temp = string[i].mct[j].chan[MCT_LOCAL_TEMPB];
      if (temp < dataLogMctTempMin) {
        dataLogMctTempMin = temp;
        dataLogMctTMinLoc = i << 6;
        dataLogMctTMinLoc |= (j << 2);
        dataLogMctTMinLoc |= 1;
      }
      if (temp > dataLogMctTempMax) {
        dataLogMctTempMax = temp;
        dataLogMctTMaxLoc = i << 6;
        dataLogMctTMaxLoc |= (j << 2);
        dataLogMctTMaxLoc |= 1;
      }
    }
  }    

  for (i = 0; i < 4; i++) {
    for (j = 1; j <= string[i].maxAddr; j++) {
      temp = string[i].mct[j].chan[MCT_HUMIDITY1];
      if (temp > 0) {
        if (temp < dataLogMctHumidMin) {
          dataLogMctHumidMin = temp;
          dataLogMctHMinLoc = i << 6;
          dataLogMctHMinLoc |= (j << 2);
        }
        if (temp > dataLogMctHumidMax) {
          dataLogMctHumidMax = temp;
          dataLogMctHMaxLoc = i << 6;
          dataLogMctHMaxLoc |= (j << 2);
        }  
      }
      temp = string[i].mct[j].chan[MCT_HUMIDITY2];
      if (temp > 0) {
        if (temp < dataLogMctHumidMin) {
          dataLogMctHumidMin = temp;
          dataLogMctHMinLoc = i << 6;
          dataLogMctHMinLoc |= (j << 2);
          dataLogMctHMinLoc |= 1;
        }
        if (temp > dataLogMctHumidMax) {
          dataLogMctHumidMax = temp;
          dataLogMctHMaxLoc = i << 6;
          dataLogMctHMaxLoc |= (j << 2);
          dataLogMctHMaxLoc |= 1;
        }  
      }
    }
  }    
} // DataLogUpdateData()
