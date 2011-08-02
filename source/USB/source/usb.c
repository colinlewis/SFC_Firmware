/********************************************************************
 FileName:		main.c
 Dependencies:	See INCLUDES section
 Processor:		PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
 				boards supported by the MCHPFSUSB stack.  See release notes for
 				support matrix.  This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release

  2.1   Updated for simplicity and to use common
        coding style

  2.6   Added support for PIC32MX795F512L & PIC24FJ256DA210

  2.6a  Added support for PIC24FJ256GB210

  2.7   No change

  8/16/2010 Modified for SFC V1.0
********************************************************************/

/** INCLUDES *******************************************************/
#include "usb.h"
#include "usb_function_generic.h"
#include "sfc.h"
#include "ads1115.h"
#include "datalog.h"
#include "field.h"
#include "mct485.h"
#include "mctbus.h"
#include "param.h"
#include "rtc.h"
#include "rtu.h"
#include "usbMsg.h"

//#include "../include/HardwareProfile.h"


unsigned char OUTPacket[64];	//User application buffer for receiving and holding OUT packets sent from the host
unsigned char INPacket[64];		//User application buffer for sending IN packets to the host
unsigned char usb485String;
USB_HANDLE USBGenericOutHandle;
USB_HANDLE USBGenericInHandle;

/** PRIVATE PROTOTYPES *********************************************/
void InitializeSystem(void);
void USBDeviceTasks(void);
void YourHighPriorityISRCode(void);
void YourLowPriorityISRCode(void);
void UserInit(void);
void ProcessIO(void);
void BlinkUSBStatus(void);

/******************************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
#if 0
#if defined(__18CXX)
void main(void)
#else
int main(void)
#endif
{
    InitializeSystem();

    #if defined(USB_INTERRUPT)
        USBDeviceAttach();
    #endif

    while(1)
    {
        #if defined(USB_POLLING)
		// Check bus status and service USB interrupts.
        USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
        				  // this function periodically.  This function will take care
        				  // of processing and responding to SETUP transactions
        				  // (such as during the enumeration process when you first
        				  // plug in).  USB hosts require that USB devices should accept
        				  // and process SETUP packets in a timely fashion.  Therefore,
        				  // when using polling, this function should be called
        				  // frequently (such as once about every 100 microseconds) at any
        				  // time that a SETUP packet might reasonably be expected to
        				  // be sent by the host to your device.  In most cases, the
        				  // USBDeviceTasks() function does not take very long to
        				  // execute (~50 instruction cycles) before it returns.
        #endif


		// Application-specific tasks.
		// Application related code may be added here, or in the ProcessIO() function.
        ProcessIO();
    }//end while
}//end main
#endif

/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.
 *
 * Note:            None
 *******************************************************************/
void USBInit(void) {
	USBGenericOutHandle = 0;
	USBGenericInHandle = 0;

    USBDeviceInit();	//usb_device.c.  Initializes USB module SFRs and firmware
    					//variables to known states.
}//end InitializeSystem



/******************************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user routines.
 *                  It is a mixture of both USB and non-USB tasks.
 *
 * Note:            None
 *****************************************************************************/
void USBUpdate(void) {
  byte data[2];
  byte i;
  byte paramNum;
  ulong tempLong;
  
  USBDeviceTasks();
  
  if ((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl==1)) {
    return;
  }

  //As the device completes the enumeration process, the USBCBInitEP() function will
  //get called.  In this function, we initialize the user application endpoints (in this
  //example code, the user application makes use of endpoint 1 IN and endpoint 1 OUT).
  //The USBGenRead() function call in the USBCBInitEP() function initializes endpoint 1 OUT
  //and "arms" it so that it can receive a packet of data from the host.  Once the endpoint
  //has been armed, the host can then send data to it (assuming some kind of application software
  //is running on the host, and the application software tries to send data to the USB device).

  //If the host sends a packet of data to the endpoint 1 OUT buffer, the hardware of the SIE will
  //automatically receive it and store the data at the memory location pointed to when we called
  //USBGenRead().  Additionally, the endpoint handle (in this case USBGenericOutHandle) will indicate
  //that the endpoint is no longer busy.  At this point, it is safe for this firmware to begin reading
  //from the endpoint buffer, and processing the data.  In this example, we have implemented a few very
  //simple commands.  For example, if the host sends a packet of data to the endpoint 1 OUT buffer, with the
  //first byte = 0x80, this is being used as a command to indicate that the firmware should "Toggle LED(s)".
  if(!USBHandleBusy(USBGenericOutHandle) && //Check if the endpoint has received any data from the host.
	  //Now check to make sure no previous attempts to send data to the host are still pending.  If any attemps are still
    //pending, we do not want to write to the endpoint 1 IN buffer again, until the previous transaction is complete.
    //Otherwise the unsent data waiting in the buffer will get overwritten and will result in unexpected behavior.
     (!USBGenericInHandle || !USBHandleBusy(USBGenericInHandle))) {
    if (OUTPacket[USB_PACKET_LEN] < 7) {
      // message too short
    }
    else {
      usbActivityTimeout = 3000; // reset timeout (in ms) 3 seconds
      
      switch(OUTPacket[USB_PACKET_CMD]) {				//Data arrived, check what kind of command might be in the packet of data.
      case USB_CMD_NULL:        // 0x00
        if (OUTPacket[USB_PACKET_LEN] != 7) {
          data[0] = USB_ERR_BAD_LEN;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else {
          UsbSendResp(USB_RESP_NULL, data);
        }  
        break;

      case USB_CMD_GET_VSTRING:
        if (OUTPacket[USB_PACKET_LEN] != 7) {
          data[0] = USB_ERR_BAD_LEN;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else {
          UsbSendResp(USB_RESP_GET_VSTRING, data);
        }  
        break;
        
      case USB_CMD_GET_CHAN:
        if (OUTPacket[USB_PACKET_LEN] != 7) {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else if (OUTPacket[USB_PACKET_STR] >= NUM_STRINGS) {
          data[0] = USB_ERR_BAD_PRM;
          data[1] = 0;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else if (!OUTPacket[USB_PACKET_MCT] || OUTPacket[USB_PACKET_MCT] > NUM_MCT) {
          data[0] = USB_ERR_BAD_PRM;
          data[1] = 1;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else {
          UsbSendResp(USB_RESP_GET_CHAN, data);
        }
        break;

      case USB_CMD_GET_MIRRORS:
        if (OUTPacket[USB_PACKET_LEN] != 7) {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else if (OUTPacket[USB_PACKET_STR] >= NUM_STRINGS) {
          data[0] = USB_ERR_BAD_PRM;
          data[1] = 0;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else if (!OUTPacket[USB_PACKET_MCT] || OUTPacket[USB_PACKET_MCT] > NUM_MCT) {
          data[0] = USB_ERR_BAD_PRM;
          data[1] = 1;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else {
          UsbSendResp(USB_RESP_GET_MIRRORS, data);
        }
        break;

      case USB_CMD_GET_STRING:
        if (OUTPacket[USB_PACKET_LEN] != 7) {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else if (OUTPacket[USB_PACKET_STR] >= NUM_STRINGS) {
          data[0] = USB_ERR_BAD_PRM;
          data[1] = 0;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        else {
          UsbSendResp(USB_RESP_GET_STRING, data);
        }
        break;

      case USB_CMD_FIELD_STATE:
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_FIELD_STATE, data);
        }  
        else if (OUTPacket[USB_PACKET_LEN] == 8) {
          FieldNewState(OUTPacket[USB_PACKET_DATA]);
          UsbSendResp(USB_RESP_FIELD_STATE, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;

      case USB_CMD_GET_FCE:
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_GET_FCE, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_GET_RTU:
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_GET_RTU, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;

      case USB_CMD_SEND_MCT485:  // 0x64
        if (OUTPacket[USB_PACKET_LEN] >= 11
            && OUTPacket[USB_PACKET_LEN] <  MCT485_MAX_INDEX + 7) {
          //Mct485CannedMsg(MSG_ORIGIN_USB,
          //               OUTPacket[USB_PACKET_STR],
          //               OUTPacket[USB_PACKET_LEN]-7,
          //               &OUTPacket[USB_PACKET_DATA]);
          usb485TxString = OUTPacket[USB_PACKET_STR];
          usb485TxLen = OUTPacket[USB_PACKET_LEN]-7;
          for (i = 0; i < usb485TxLen; i++) {
            usb485TxBuffer[i] = OUTPacket[USB_PACKET_DATA+i];
          }
          // clear 485 response buffer
          usb485RxLen = 0;
          UsbSendResp(USB_RESP_SEND_MCT485, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;

      case USB_CMD_GET_MCT485:   // 0x65
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_GET_MCT485, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_RTC:         // 0x66
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_RTC, data);
        }
        else if (OUTPacket[USB_PACKET_LEN] == 13) {
          RtcSetClock(&OUTPacket[USB_PACKET_DATA]);
          UsbSendResp(USB_RESP_RTC, data);
        }
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_LOG:        // 0x67
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_LOG, data);
        }
        else if (OUTPacket[USB_PACKET_LEN] == 8) {
          if (OUTPacket[USB_PACKET_DATA] == 0) {
            DataLogFindFirstEntry();
          }
          else if (OUTPacket[USB_PACKET_DATA] == 0xFF) {  
            DataLogErase();
          }
          UsbSendResp(USB_RESP_LOG, data);
        }
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_DESICCANT:   // 0x68
        if (OUTPacket[USB_PACKET_LEN] == 7) {
          UsbSendResp(USB_RESP_DESICCANT, data);
        }
        else if (OUTPacket[USB_PACKET_LEN] == 9) {
          // manual force dessicant to set of outputs or state
          DesiccantNewState(OUTPacket[USB_PACKET_DATA], OUTPacket[USB_PACKET_DATA+1]);
          UsbSendResp(USB_RESP_DESICCANT, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_SFC_PARAM:  // 0x69
        if (OUTPacket[USB_PACKET_LEN] == 8) {
          UsbSendResp(USB_RESP_SFC_PARAM, data);
        }
        else if (OUTPacket[USB_PACKET_LEN] > 8 && !(OUTPacket[USB_PACKET_LEN] & 0x03)) {
          // process one param at a time, prevents I2C queue entry overflow and avoids
          // I2C page boundary problems
          paramNum = OUTPacket[USB_PACKET_DATA];
          i = USB_PACKET_DATA+1;
          while (i < OUTPacket[USB_PACKET_LEN]-2) {
            tempLong = OUTPacket[i++];
            tempLong *= 256;
            tempLong += OUTPacket[i++];
            tempLong *= 256;
            tempLong += OUTPacket[i++];
            tempLong *= 256;
            tempLong += OUTPacket[i++];
            ParamWrite(paramNum, tempLong);
            paramNum++;
          }
          UsbSendResp(USB_RESP_SFC_PARAM, data);
        }  
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_MEMORY:     // 0x6A
        if (OUTPacket[USB_PACKET_LEN] == 11) {
          data[4] = 16;
          UsbSendResp(USB_RESP_MEMORY, data);
        }
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      
      case USB_CMD_TEST:      // 0x6B
        if (OUTPacket[USB_PACKET_LEN] == 8) {
          UsbSendResp(USB_RESP_TEST, data);
          for (tempLong = 0; tempLong < 1000000; tempLong++) {
          }
          if (OUTPacket[USB_PACKET_DATA] == 0x01) {
            Mct485Init();
          }
          else if (OUTPacket[USB_PACKET_DATA] == 0x02) {
            StringInit();
          }
          else if (OUTPacket[USB_PACKET_DATA] == 0x03) {
            FieldInit();
          }
          else if (OUTPacket[USB_PACKET_DATA] == 0x04) {
            SoftReset();
          }
        }
        else {
          data[0] = USB_ERR_BAD_LEN;
          data[1] = 7;
          UsbSendResp(USB_RESP_BAD_MSG, data);
        }
        break;
      } // switch (cmd)
    } // else process messages  

    //Re-arm the OUT endpoint for the next packet:
	  //The USBGenRead() function call "arms" the endpoint (and makes it "busy").  If the endpoint is armed, the SIE will
	  //automatically accept data from the host, if the host tries to send a packet of data to the endpoint.  Once a data
	  //packet addressed to this endpoint is received from the host, the endpoint will no longer be busy, and the application
	  //can read the data which will be sitting in the buffer.
    USBGenericOutHandle = USBGenRead(USBGEN_EP_NUM,(BYTE*)&OUTPacket,USBGEN_EP_SIZE);
  } // if
}// UsbUpdate()


void UsbSendResp(byte resp, byte *data) {
  MCT_t *pMct;
  byte i;
  byte *pData;
  byte paramNum;
  ushort temp;
  ulong tempL;
  
  pMct = &string[OUTPacket[USB_PACKET_STR]].mct[OUTPacket[USB_PACKET_MCT]-1];
  INPacket[USB_PACKET_STR] = OUTPacket[USB_PACKET_STR];
  INPacket[USB_PACKET_MCT] = OUTPacket[USB_PACKET_MCT];
  INPacket[USB_PACKET_PID] = OUTPacket[USB_PACKET_PID];
  INPacket[USB_PACKET_CMD] = resp;
  switch (resp) {
  case USB_RESP_NULL:        // 0x80
    INPacket[USB_PACKET_LEN] = 7;
    break;

  case USB_RESP_BAD_MSG:     // 0x81
    INPacket[USB_PACKET_LEN] = 9;
    INPacket[USB_PACKET_DATA + 0] = data[0];
    INPacket[USB_PACKET_DATA + 1] = data[1];
    break;

  case USB_RESP_GET_VSTRING:
    INPacket[USB_PACKET_LEN] = 27;
    for (i = 0; i < 20; i++) {
      INPacket[USB_PACKET_DATA + i] = versString[i];
    }
    break;

  case USB_RESP_GET_CHAN:    // 0xB3
    INPacket[USB_PACKET_LEN] = 39;
    for (i = 0; i < 16; i++) {
      temp = pMct->chan[i];
      INPacket[5 + 2*i] = (byte)(temp >> 8);
      INPacket[6 + 2*i] = (byte)temp;
    }
    break;

  case USB_RESP_GET_MIRRORS: // 0xC2
    INPacket[USB_PACKET_LEN] = 19;
    INPacket[5] = pMct->mirror[MIRROR1].tracking;
    INPacket[6] = pMct->mirror[MIRROR1].sensors;
    temp = pMct->mirror[MIRROR1].m[MOTOR_A].mdeg100;
    INPacket[7] = (byte)(temp >> 8);
    INPacket[8] = (byte)temp;
    temp = pMct->mirror[MIRROR1].m[MOTOR_B].mdeg100;
    INPacket[9] = (byte)(temp >> 8);
    INPacket[10] = (byte)temp;
    INPacket[11] = pMct->mirror[MIRROR2].tracking;
    INPacket[12] = pMct->mirror[MIRROR2].sensors;
    temp = pMct->mirror[MIRROR2].m[MOTOR_A].mdeg100;
    INPacket[13] = (byte)(temp >> 8);
    INPacket[14] = (byte)temp;
    temp = pMct->mirror[MIRROR2].m[MOTOR_B].mdeg100;
    INPacket[15] = (byte)(temp >> 8);
    INPacket[16] = (byte)temp;
    break;

  case USB_RESP_GET_STRING:
    INPacket[USB_PACKET_LEN] = 9;
    INPacket[5] = string[OUTPacket[USB_PACKET_STR]].maxAddr;
    INPacket[6] = string[OUTPacket[USB_PACKET_STR]].state;
    break;

  case USB_RESP_FIELD_STATE:
    INPacket[USB_PACKET_LEN] = 9;
    INPacket[USB_PACKET_DATA] = fieldState;
    break;

  case USB_RESP_GET_FCE:
    INPacket[USB_PACKET_LEN] = 28;
    INPacket[USB_PACKET_DATA] = fieldInState;
    INPacket[USB_PACKET_DATA+1] = FCE_inputs;
    INPacket[USB_PACKET_DATA+2] = FCE_outputs;
    for (i = 0; i < 7; i++) {
      INPacket[USB_PACKET_DATA+3+2*i] = (byte)(ads1115[i] >> 8);
      INPacket[USB_PACKET_DATA+4+2*i] = (byte)ads1115[i];
    }
    INPacket[USB_PACKET_DATA+17] = (byte)(fieldDNI >> 8);
    INPacket[USB_PACKET_DATA+18] = (byte)fieldDNI;
    INPacket[USB_PACKET_DATA+19] = (byte)(fceRTD >> 8);
    INPacket[USB_PACKET_DATA+20] = (byte)fceRTD;
    break;

  case USB_RESP_GET_RTU:
    INPacket[USB_PACKET_LEN] = 44;
    for (i = 0; i < 10; i++) {
      INPacket[USB_PACKET_DATA+0+4*i] = (byte)(rtuAddr[i] >> 8);
      INPacket[USB_PACKET_DATA+1+4*i] = (byte)rtuAddr[i];
      INPacket[USB_PACKET_DATA+2+4*i] = (byte)(rtuData[i] >> 8);
      INPacket[USB_PACKET_DATA+3+4*i] = (byte)rtuData[i];
    }
    break;

  case USB_RESP_SEND_MCT485:
    INPacket[USB_PACKET_LEN] = 7;
    break;

  case USB_RESP_GET_MCT485:
    INPacket[USB_PACKET_STR] = usb485String;
    INPacket[USB_PACKET_MCT] = usb485RxBuffer[MCT485_ADDR];
    INPacket[USB_PACKET_LEN] = 7 + usb485RxLen;
    for (i = 0; i < usb485RxLen; i++) {
      INPacket[USB_PACKET_DATA + i] = usb485RxBuffer[i];
    }
    usb485RxLen = 0;
    break;

  case USB_RESP_RTC:    // 0xE6        
    INPacket[USB_PACKET_LEN] = 19;
    INPacket[USB_PACKET_DATA+0] = rtcSec;
    INPacket[USB_PACKET_DATA+1] = rtcMin;
    INPacket[USB_PACKET_DATA+2] = rtcHour;
    INPacket[USB_PACKET_DATA+3] = rtcDate;
    INPacket[USB_PACKET_DATA+4] = rtcMonth;
    INPacket[USB_PACKET_DATA+5] = rtcYear;
    INPacket[USB_PACKET_DATA+6] = (byte)(sfcPcbTemp >> 8);
    INPacket[USB_PACKET_DATA+7] = (byte)sfcPcbTemp;
    INPacket[USB_PACKET_DATA+8] = (byte)(sfcThermistor >> 8);
    INPacket[USB_PACKET_DATA+9] = (byte)sfcThermistor;
    INPacket[USB_PACKET_DATA+10] = (byte)(sfcHumidity >> 8);
    INPacket[USB_PACKET_DATA+11] = (byte)sfcHumidity;
    break;

  case USB_RESP_LOG:    // 0xE7
    INPacket[USB_PACKET_LEN] = 7;
    do {
      i = DataLogReadNext(&INPacket[INPacket[USB_PACKET_LEN]-2]);
      INPacket[USB_PACKET_LEN] += i;
    } while (INPacket[USB_PACKET_LEN] <= 55);
    break;
    
  case USB_RESP_DESICCANT:   // 0xE8
    INPacket[USB_PACKET_LEN] = 9;
    INPacket[USB_PACKET_DATA+0] = desiccantState;
    INPacket[USB_PACKET_DATA+1] = FCE_outputs &
                               (FCE_OUT_FAN + FCE_OUT_VALVE + FCE_OUT_HEAT);
    break;

  case USB_RESP_SFC_PARAM:  // 0xE9
    paramNum = OUTPacket[USB_PACKET_DATA];
    INPacket[USB_PACKET_DATA] = paramNum;
    INPacket[USB_PACKET_LEN] = 47;
    i = 0;
    while (i < 40) {
      INPacket[USB_PACKET_DATA+i+1] = (byte)(param[paramNum] >> 24);
      INPacket[USB_PACKET_DATA+i+2] = (byte)(param[paramNum] >> 16);
      INPacket[USB_PACKET_DATA+i+3] = (byte)(param[paramNum] >> 8);
      INPacket[USB_PACKET_DATA+i+4] = (byte)(param[paramNum]);
      i += 4;
      paramNum++;
    }
    break;

  case USB_RESP_MEMORY:    // 0xEA
    INPacket[USB_PACKET_DATA+0] = OUTPacket[USB_PACKET_DATA+0];
    INPacket[USB_PACKET_DATA+1] = OUTPacket[USB_PACKET_DATA+1];
    INPacket[USB_PACKET_DATA+2] = OUTPacket[USB_PACKET_DATA+2];
    INPacket[USB_PACKET_DATA+3] = OUTPacket[USB_PACKET_DATA+3];
    INPacket[USB_PACKET_DATA+4] = data[4];
    INPacket[USB_PACKET_LEN] = 7+4+1+16;
    i = 0;
    tempL = ((ulong)OUTPacket[USB_PACKET_DATA+0]) << 24;
    tempL |= (((ulong)OUTPacket[USB_PACKET_DATA+1]) << 16);
    tempL |= (((ulong)OUTPacket[USB_PACKET_DATA+2]) << 8);
    tempL |= (ulong)OUTPacket[USB_PACKET_DATA+3];
    pData = (byte *)tempL;
    while (i < 16) {
      INPacket[USB_PACKET_DATA+i+5] = *pData++;
      i++;
    }
    break;

  case USB_RESP_TEST:
    INPacket[USB_PACKET_LEN] = 7;
    break;

  default:
    break;
  } // switch()
  
  USBGenericInHandle = USBGenWrite(USBGEN_EP_NUM, INPacket, INPacket[USB_PACKET_LEN]);
} // UsbSendResp()


void Usb485Resp(void) {
  usb485String = mct485String[mct485TxQueueRd];
  usb485RxLen = 0;
  while (usb485RxLen < mct485RxIndex) {
    usb485RxBuffer[usb485RxLen] = mct485RxBuffer[usb485RxLen];
    usb485RxLen++;
  }
} // Usb485Resp()

// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void)
{
	//Example power saving code.  Insert appropriate code here for the desired
	//application behavior.  If the microcontroller will be put to sleep, a
	//process similar to that shown below may be used:

	//ConfigureIOPinsForLowPower();
	//SaveStateOfAllInterruptEnableBits();
	//DisableAllInterruptEnableBits();
	//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
	//Sleep();
	//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
	//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

	//IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is
	//cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause
	//things to not work as intended.


    #if defined(__C30__)
    #if 0
        U1EIR = 0xFFFF;
        U1IR = 0xFFFF;
        U1OTGIR = 0xFFFF;
        IFS5bits.USB1IF = 0;
        IEC5bits.USB1IE = 1;
        U1OTGIEbits.ACTVIE = 1;
        U1OTGIRbits.ACTVIF = 1;
        Sleep();
    #endif
    #endif
}


/******************************************************************************
 * Function:        void _USB1Interrupt(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the USB interrupt bit is set
 *					In this example the interrupt is only used when the device
 *					goes to sleep when it receives a USB suspend command
 *
 * Note:            None
 *****************************************************************************/
#if 0
void __attribute__ ((interrupt)) _USB1Interrupt(void)
{
    #if !defined(self_powered)
        if(U1OTGIRbits.ACTVIF)
        {
            IEC5bits.USB1IE = 0;
            U1OTGIEbits.ACTVIE = 0;
            IFS5bits.USB1IF = 0;

            //USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
            USBClearInterruptFlag(USBIdleIFReg,USBIdleIFBitNum);
            //USBSuspendControl = 0;
        }
    #endif
}
#endif

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *
 *					This call back is invoked when a wakeup from USB suspend
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void)
{
	// If clock switching or other power savings measures were taken when
	// executing the USBCBSuspend() function, now would be a good time to
	// switch back to normal full power run mode conditions.  The host allows
	// a few milliseconds of wakeup time, after which the device must be
	// fully back to normal, and capable of receiving and processing USB
	// packets.  In order to do this, the USB module must receive proper
	// clocking (IE: 48MHz clock must be available to SIE for full speed USB
	// operation).
}

/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void)
{
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

	// Typically, user firmware does not need to do anything special
	// if a USB error occurs.  For example, if the host sends an OUT
	// packet to your device, but the packet gets corrupted (ex:
	// because of a bad connection, or the user unplugs the
	// USB cable during the transmission) this will typically set
	// one or more USB error interrupt flags.  Nothing specific
	// needs to be done however, since the SIE will automatically
	// send a "NAK" packet to the host.  In response to this, the
	// host will normally retry to send the packet again, and no
	// data loss occurs.  The system will typically recover
	// automatically, without the need for application firmware
	// intervention.

	// Nevertheless, this callback function is provided, such as
	// for debugging purposes.
}


/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *****************************************************************************/
void USBCBCheckOtherReq(void)
{
}//end


/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *****************************************************************************/
void USBCBStdSetDscHandler(void)
{
    // Must claim session ownership if supporting this request
}//end


/******************************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This
 *					callback function should initialize the endpoints
 *					for the device's usage according to the current
 *					configuration.
 *
 * Note:            None
 *****************************************************************************/
void USBCBInitEP(void)
{
    USBEnableEndpoint(USBGEN_EP_NUM,USB_OUT_ENABLED|USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    USBGenericOutHandle = USBGenRead(USBGEN_EP_NUM,(BYTE*)&OUTPacket,USBGEN_EP_SIZE);
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function should only be called when:
 *
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.
 *
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            Interrupt vs. Polling
 *                  -Primary clock
 *                  -Secondary clock ***** MAKE NOTES ABOUT THIS *******
 *                   > Can switch to primary first by calling USBCBWakeFromSuspend()

 *                  The modifiable section in this routine should be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of 1-13 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at lest 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void)
{
    static WORD delay_count;

    USBResumeControl = 1;                // Start RESUME signaling

    delay_count = 1800U;                // Set RESUME line for 1-13 ms
    do
    {
        delay_count--;
    }while(delay_count);
    USBResumeControl = 0;
}


/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size)
{
    switch(event)
    {
        case EVENT_CONFIGURED:
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        case EVENT_TRANSFER:
            Nop();
            break;
        default:
            break;
    }
    return TRUE;
}
/** EOF main.c ***************************************************************/
