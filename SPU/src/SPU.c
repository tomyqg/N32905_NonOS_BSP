/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
 
#include "string.h"
#include "stdlib.h"
#include "w55fa93_reg.h"
#include "spu.h"
#include "DrvSPU.h"

//#define DBG_PRINTF(...)
//#define DBG_PRINTF printf

/* buffer */
UINT8	*_pucPlayAudioBuff;

__align(256) UINT8 playbuffer[FRAG_SIZE];

void SPU_SET_SAMPLE_RATE(UINT32 u32sysclk, UINT32 u32SampleRate)
{	
	DrvSPU_SetSampleRate(u32sysclk, u32SampleRate);
}

void SPU_SET_BASE_ADDRESS(UINT32 u32BaseAddress)
{
	DrvSPU_SetBaseAddress(0, u32BaseAddress);
	DrvSPU_SetBaseAddress(1, u32BaseAddress);			
}

UINT32 SPU_GET_BASE_ADDRESS(void)
{
	return DrvSPU_GetBaseAddress(0);
}

void SPU_SET_TH1_ADDRESS(UINT32 u32TH1Address)
{
	DrvSPU_SetThresholdAddress(0, u32TH1Address);	
	DrvSPU_SetThresholdAddress(1, u32TH1Address);		
}

UINT32 SPU_GET_TH1_ADDRESS(void)
{
	return DrvSPU_GetThresholdAddress(0);
}

void SPU_SET_TH2_ADDRESS(UINT32 u32TH2Address)
{
	DrvSPU_SetEndAddress(0, u32TH2Address);	
	DrvSPU_SetEndAddress(1, u32TH2Address);		
}

UINT32 SPU_GET_TH2_ADDRESS(void)
{
	return DrvSPU_GetEndAddress(0);
}

UINT32 SPU_GET_CUR_ADDRESS(void)
{
	return DrvSPU_GetCurrentAddress(0);
}

void SPU_STEREO(void)
{
	DrvSPU_SetSrcType(0, 0x06);		// Stereo PCM16 left	
	DrvSPU_SetSrcType(1, 0x07);		// Stereo PCM16 Right			
}

void SPU_MONO(void)
{
	DrvSPU_SetSrcType(0, 0x05);		// Mono PCM16	
}

BOOL SPU_ISMONO(void)
{
	return 0;
}

void SPU_SET_VOLUME(UINT16 u16CHRVolume, UINT16 u16CHLVolume)
{
	UINT16 u16Voluem;
	
	// u16CHRVolume: 0x0(Mute) ~ 0x3F(Maximum)
		u16Voluem = (u16CHRVolume & 0x3F) | ((u16CHLVolume & 0x3F) << 8);
		DrvSPU_SetVolume(u16Voluem);
}

UINT8 SPU_GET_VOLUME(UINT8 u8Channel)
{
	return DrvSPU_GetChannelVolume(u8Channel);
}

void SPU_SET_POWER_DOWN(UINT16 u16PowerDown)
{
	outp32(REG_SPU_DAC_VOL, (inp32(REG_SPU_DAC_VOL) & ~ANA_PD) | ((u16PowerDown & 0xFFF)<<16));
}

UINT16 SPU_GET_POWER_DOWN(void)
{
	return (inp32(REG_SPU_DAC_VOL) & ~ANA_PD) >> 16;
}

UINT8 DacOnOffLevel;

//must use this function before calling spuStartPlay()
VOID spuDacOn(UINT8 level)
{
	DacOnOffLevel = level;	
	
	outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | 0x30);		//disable
	if(level == 3)
		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & ~0x30);	//delay time, p0=3s
	else if(level == 1)
		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & ~0x20);	//delay time, p0=0.5-1s
	else if(level == 2)
		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) & ~0x10);	//delay time, p0=2s
	else 	
	{	
		outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x03FF0000);	//P7
		outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | 0x30);		//disable
		return;
	}

	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x0800000);	//P7	
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x0400000);	//P6
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x01e0000);	//P1-4
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x0200000);	//P5	
	sysDelay(1);
	
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) & ~0x00010000);	//P0			
	
	if(level == 3)		//modify this delay time to meet the product request
		sysDelay(220);
	else if(level == 1)
		sysDelay(70);
	else if(level == 2)
		sysDelay(30);
		
}

//must use this function after calling spuStopPlay()
VOID spuDacOff(VOID)
{		
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | 0x10000);	//P0
	
	if(DacOnOffLevel == 3)		//modify this delay time to meet the product request
		sysDelay(150);
	else if(DacOnOffLevel == 1)
		sysDelay(70);
	else if(DacOnOffLevel == 2)
		sysDelay(40);
		
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | 0x200000);	//P5
	sysDelay(1);
				
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | 0x1e0000);	//P1-4
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | 0x400000);	//P6
	sysDelay(1);
	outp32(REG_SPU_DAC_VOL, inp32(REG_SPU_DAC_VOL) | 0x800000);	//P7	
	sysDelay(1);

	outp32(REG_SPU_DAC_PAR, inp32(REG_SPU_DAC_PAR) | 0x30);  //disable
}

VOID spuStartPlay(PFN_DRVSPU_CB_FUNC *fnCallBack, UINT8 *data)
{	
	DrvSPU_EnableInt(eDRVSPU_CHANNEL_0, DRVSPU_THADDRESS_INT, (PFN_DRVSPU_CB_FUNC*) fnCallBack);
	DrvSPU_EnableInt(eDRVSPU_CHANNEL_0, DRVSPU_ENDADDRESS_INT, (PFN_DRVSPU_CB_FUNC*) fnCallBack);	
	
	memcpy(playbuffer, data, FRAG_SIZE);

	DrvSPU_StartPlay();	
}

VOID spuStopPlay(VOID)
{
	int ii;     
    for (ii=0; ii<32; ii++)
    {
 		DrvSPU_DisableInt(ii, DRVSPU_ENDADDRESS_INT); 
        DrvSPU_DisableInt(ii, DRVSPU_THADDRESS_INT);
    }
	DrvSPU_StopPlay();	
	sysDisableInterrupt(IRQ_SPU);	
}

VOID spuIoctl(UINT32 cmd, UINT32 arg0, UINT32 arg1)
{
	switch(cmd)
	{		
		case SPU_IOCTL_SET_VOLUME:
			SPU_SET_VOLUME(arg0, arg1);
			break;
			
		case SPU_IOCTL_SET_MONO:
			SPU_MONO();
			break;
			
		
		case SPU_IOCTL_SET_STEREO:
			SPU_STEREO();
			break;			
			
		case SPU_IOCTL_GET_FRAG_SIZE:
			*((UINT32 *)arg0) = FRAG_SIZE;
			break;
			
		default:
			break;
	}		
}

VOID spuOpen(UINT32 u32SampleRate)
{
	UINT32 u32SystemClock, u32ExtClock;
	
	_pucPlayAudioBuff = (UINT8 *)((UINT32)playbuffer | 0x80000000);
	memset(_pucPlayAudioBuff, 0, FRAG_SIZE);

	SPU_DISABLE();	// SPU must be disabled before to enable again
	
	//sysGetClockFreq(&pllcfg);
	u32ExtClock= sysGetExternalClock();
	if (inp32(REG_CHIPCFG) & 0x01)
		u32SystemClock = sysGetPLLOutputKhz(eSYS_UPLL, u32ExtClock)*1000;
	else
		u32SystemClock = sysGetPLLOutputKhz(eSYS_UPLL, u32ExtClock)*1000;

	// 1.Check I/O pins. If I/O pins are used by other IPs, return error code.
	// 2.Enable IP��s clock
	// 3.Reset IP			
	
	DrvSPU_Open();	
	
	DrvSPU_SetPAN(0, 0x1f1f);	
	
	// Channel volume 0x4F for earphone and 0x3F for speaker
	DrvSPU_SetChannelVolume(0, 0x4F);	//	
	DrvSPU_SetChannelVolume(1, 0x4F);	//0408
	
	DrvSPU_ChannelOpen(0);		
	
	DrvSPU_SetSampleRate(u32SystemClock, u32SampleRate);

	DrvSPU_SetSrcType(0, 0x06);		// Stereo PCM16 left

	SPU_SET_BASE_ADDRESS((UINT32)_pucPlayAudioBuff);
	SPU_SET_TH1_ADDRESS((UINT32)_pucPlayAudioBuff + HALF_FRAG_SIZE);	
	SPU_SET_TH2_ADDRESS((UINT32)_pucPlayAudioBuff + FRAG_SIZE);

	sysInstallISR(IRQ_LEVEL_1, IRQ_SPU, (PVOID)DrvSPU_IntHandler);	
	sysSetLocalInterrupt(ENABLE_IRQ);
	sysEnableInterrupt(IRQ_SPU);
	
}

VOID spuClose (VOID)
{
	sysDisableInterrupt(IRQ_SPU);
	DrvSPU_Close();
}

VOID spuEqOpen (E_DRVSPU_EQ_BAND eEqBand, E_DRVSPU_EQ_GAIN eEqGain)
{
	DrvSPU_EqOpen(eEqBand, eEqGain);
}

VOID spuEqClose (VOID)
{
	DrvSPU_EqClose();
}



