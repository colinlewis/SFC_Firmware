/*********************************************************************
 *
 *                  NVM Program Library Source
 *
 *********************************************************************
 * FileName:        nvm_program_lib.c
 * Dependencies:
 * Processor:       PIC32
 *
 * Complier:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology Inc..
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PIC32 Microcontroller is intended
 * and supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PIC32 Microcontroller products.
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * $Id: NVM.h,v 1.1 2006/10/16 20:30:29 C10737 Exp $
 * $Name: $
 *
 ********************************************************************/
#include <peripheral/nvm.h>


/*********************************************************************
 * Function:        unsigned int NVMProgram(void* address, const void* data, unsigned int size)
 *
 * Description:
 *
 * PreCondition:    None
 *
 * Inputs:          address:  Destination address to start writing from.
 *                  data:  Location of data to write.
 *                  size:  Number of bytes to write.
 *
 * Output:          '0' if operation completed successfully.
 *
 * Example:         NVMProgram((void*) 0xBD000000, (const void*) 0xA0000000, 1024)
 ********************************************************************/
unsigned int NVMProgram(void * address, const void * data, unsigned int size, void* pagebuff)
{
    unsigned int startAddr, endAddr;
    unsigned int numBefore, numAfter, numLeft;
    unsigned int index;

	// 1. Make sure that the size is multiple of 4
	if(size && size & 0x03)
		return 1;

    while(size)
    {
        // 2. Get Page Start address
        startAddr = (unsigned int)address & (~(BYTE_PAGE_SIZE - 1));

        // 3. Calculate the number of bytes that need to be stored before the address.
    	numBefore = (unsigned int)address - startAddr;

    	// 4. Make a copy of original data, if there is any
    	if(numBefore)
    		memcpy(pagebuff, (void *)startAddr, numBefore);

        // 5. Determine how many can copy form user buffer
        numLeft = BYTE_PAGE_SIZE - numBefore;

        if(size <= numLeft)
        {
            // Copy all of it
            memcpy((pagebuff + numBefore), (void *)data, size);

            // Calculate the number of bytes that need to be stored after the address.
            numAfter = (startAddr + BYTE_PAGE_SIZE) - ((unsigned int)address + size);

            // Copy whats left
    	    if(numAfter)
    		    memcpy((pagebuff + numBefore + size), (void *)((unsigned int)address + size), numAfter);

            size = 0;
        }
        else
        {
            // Copy numLeft of it
            memcpy((pagebuff + numBefore), (void *)data, numLeft);

            // decrement size
            size -= numLeft;
            // Increment address
            address += numLeft;
        }

        // Erase the Page
        NVMErasePage((void *)startAddr);

        // Program the Page
        for(index = 0; index < NUM_ROWS_PAGE; index++)
        {
            NVMWriteRow((void *)(startAddr + (index * BYTE_ROW_SIZE)), (void *)(pagebuff + (index * BYTE_ROW_SIZE)));
        }
    }

    return 0;
}
