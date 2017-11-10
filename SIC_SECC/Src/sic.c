/*-----------------------------------------------------------------------------------*/
/* Nuvoton Technology Corporation confidential                                       */
/*                                                                                   */
/* Copyright (c) 2008 by Nuvoton Technology Corporation                              */
/* All rights reserved                                                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
/****************************************************************************
 * 
 * FILENAME
 *     sic.c
 *
 * VERSION
 *     1.0
 *
 * DESCRIPTION
 *     This file contains SIC library APIs.
 *
 * DATA STRUCTURES
 *     None
 *
 * FUNCTIONS
 *     None
 *
 * HISTORY
  *     10/11/07      Create Ver 1.0
 *
 * REMARK
 *     None
 **************************************************************************/
/* Header files */
#include <stdio.h>
#include <string.h>

#include "wblib.h"
#include "w55fa93_sic.h"

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicOpen                                                                         */
/*                                                                                   */
/* Parameters:                                                                       */
/*   None				                                                             */
/*                                                                                   */                                                                           
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function reset the DMAC and SIC interface and enable interrupt.            */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
static _fmi_init_flag = FALSE;
void sicOpen(VOID)
{
	if (!_fmi_init_flag)
	{
		fmiInitDevice();
		_fmi_init_flag = TRUE;
	}
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicOpen                                                                         */
/*                                                                                   */
/* Parameters:                                                                       */
/*   None				                                                             */
/*                                                                                   */                                                                           
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function close the DMAC and SIC engine clock                               */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

void sicClose(VOID)
{
	outpw(REG_AHBCLK, inpw(REG_AHBCLK) & (~(SD_CKE | SIC_CKE)));
	sysDisableInterrupt(IRQ_SIC);
	_fmi_init_flag = FALSE;
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicIoctl                                                                        */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicFeature		SIC_SET_CLOCK, SIC_SET_CALLBACK.								 */
/*   sicArg0		Depend on feature setting.										 */
/*   sicArg1		Depend on feature setting.										 */
/*   sicArg2		Depend on feature setting.										 */
/*                                                                                   */                                                                           
/* Returns:                                                                          */
/*   None.                                                                           */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function set the FMI engine clock. Card insert/remove callback             */
/*	SIC_SET_CLOCK : sicArg0 used to set clock by KHz								 */
/*                                                                                   */
/*	SIC_SET_CALLBACK : sicArg0 used to select card type (FMI_SD_CARD0)				 */
/*	SIC_SET_CALLBACK : sicArg1 remove function pointer								 */
/*	SIC_SET_CALLBACK : sicArg2 insert function pointer								 */
/*-----------------------------------------------------------------------------------*/

VOID sicIoctl(INT32 sicFeature, INT32 sicArg0, INT32 sicArg1, INT32 sicArg2)
{
	switch(sicFeature)
	{
		case SIC_SET_CLOCK:
			fmiSetFMIReferenceClock(sicArg0);
			break;
		
		case SIC_SET_CALLBACK:
			fmiSetCallBack(sicArg0, (PVOID)sicArg1, (PVOID)sicArg2);
			break;			
		case SIC_GET_CARD_STATUS:
			*(UINT32 *)sicArg0 = (inpw(REG_SDISR) & 0x10000) ? FALSE : TRUE;			
			break;
	}
}

/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicSdRead                                                                       */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicSdPortNo	Select SD port 0 or port 1.										 */
/*   sdSectorNo		Sector No. to get the data from.								 */
/*   sdSectorCount	Sector count of this access.									 */
/*   sdTargetAddr	The address which data upload to SDRAM.							 */
/*                                                                                   */                                                                           
/* Returns:                                                                          */
/*   0					Success                                                      */
/*   FMI_TIMEOUT        Access timeout                                               */
/*   FMI_NO_SD_CARD     Card removed                                                 */
/*   FMI_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMI_SD_CRC16_ERROR Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to get the data from SD card					             */
/*-----------------------------------------------------------------------------------*/

INT sicSdRead(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
	if (fmiSD_CardSel(0))
		return FMI_NO_SD_CARD;

	return fmiSD_Read(sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sicSdRead0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
	if (fmiSD_CardSel(0))
		return FMI_NO_SD_CARD;
		
	return fmiSD_Read(sdSectorNo, sdSectorCount, sdTargetAddr);	
}

INT sicSdRead1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
	if (fmiSD_CardSel(1))
		return FMI_NO_SD_CARD;

//	return fmiSD_Read(sdSectorNo, sdSectorCount, sdTargetAddr);	
	outpw(REG_FMICR, FMI_SD_EN);
	return fmiSD_Read_in(pSD1, sdSectorNo, sdSectorCount, sdTargetAddr);
}

INT sicSdRead2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdTargetAddr)
{
	if (fmiSD_CardSel(2))
		return FMI_NO_SD_CARD;

//	return fmiSD_Read(sdSectorNo, sdSectorCount, sdTargetAddr);	
	outpw(REG_FMICR, FMI_SD_EN);
	return fmiSD_Read_in(pSD2, sdSectorNo, sdSectorCount, sdTargetAddr);

}


/*-----------------------------------------------------------------------------------*/
/* Function:                                                                         */
/*   sicSdWrite                                                                      */
/*                                                                                   */
/* Parameters:                                                                       */
/*   sicSdPortNo	Select SD port 0 or port 1.										 */
/*   sdSectorNo		Sector No. to get the data from.								 */
/*   sdSectorCount	Sector count of this access.									 */
/*   sdSourcetAddr	The address which data download data from SDRAM.				 */
/*                                                                                   */                                                                           
/* Returns:                                                                          */
/*   0					Success                                                      */
/*   FMI_TIMEOUT        Access timeout                                               */
/*   FMI_NO_SD_CARD     Card removed                                                 */
/*   FMI_SD_CRC7_ERROR  Command/Response error                                       */
/*   FMI_SD_CRC_ERROR   Data transfer error                                          */
/*                                                                                   */
/* Side effects:                                                                     */
/*   None.                                                                           */
/*                                                                                   */
/* Description:                                                                      */
/*   This function used to write the data to SD card					             */
/*-----------------------------------------------------------------------------------*/

INT sicSdWrite(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
	if (fmiSD_CardSel(0))
		return FMI_NO_SD_CARD;

	return fmiSD_Write(sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite0(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
	if (fmiSD_CardSel(0))
		return FMI_NO_SD_CARD;

	return fmiSD_Write(sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite1(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
	if (fmiSD_CardSel(1))
		return FMI_NO_SD_CARD;

//	return fmiSD_Write(sdSectorNo, sdSectorCount, sdSourceAddr);
	
	outpw(REG_FMICR, FMI_SD_EN);
	return fmiSD_Write_in(pSD1, sdSectorNo, sdSectorCount, sdSourceAddr);
}

INT sicSdWrite2(INT32 sdSectorNo, INT32 sdSectorCount, INT32 sdSourceAddr)
{
	if (fmiSD_CardSel(2))
		return FMI_NO_SD_CARD;

//	return fmiSD_Write(sdSectorNo, sdSectorCount, sdSourceAddr);
	outpw(REG_FMICR, FMI_SD_EN);
	return fmiSD_Write_in(pSD2, sdSectorNo, sdSectorCount, sdSourceAddr);
}


