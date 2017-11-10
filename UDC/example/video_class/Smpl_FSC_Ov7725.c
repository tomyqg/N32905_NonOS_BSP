#include "wblib.h"
#include "W55FA93_VideoIn.h"
#include "W55FA93_GPIO.h"
#include "DrvI2C.h"
#include "demo.h"

typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	char* pszFileName;
}S_VIDEOIN_REAL;
typedef struct
{
	UINT32 u32Width;
	UINT32 u32Height;
	E_VIDEOIN_OUT_FORMAT eFormat;
	char* pszFileName;
}S_VIDEOIN_PACKET_FMT;

struct OV_RegValue
{
	UINT8	u8RegAddr;		//Register Address
	UINT8	u8Value;			//Register Data
};

struct OV_RegTable{
	struct OV_RegValue *sRegTable;
	UINT32 u32TableSize;
};




#define _REG_TABLE_SIZE(nTableName)	(sizeof(nTableName)/sizeof(struct OV_RegValue))

struct OV_RegValue g_sOV7725_RegValue[] = 
{
//NewKen sensor module initial for OV7725 20110613
		{0x12, 0x80},
//		{0x00, 0x00}, {0x00, 0x00}, {0x00, 0x64},	 	//Newken only
		{0x12, 0x00}, {0x3D, 0x03},
		{0x17, 0x22}, {0x18, 0xA4}, {0x19, 0x07}, {0x1A, 0xF0},
		{0x32, 0x02}, {0x29, 0xA0}, {0x2C, 0xF0},
		{0x2A, 0x02}, {0x65, 0x20}, {0x11, 0x01},
//		{0x15, 0x03},								//Newken olny
		{0x42, 0x7F}, {0x63, 0xE0}, {0x64, 0xFF},
		{0x66, 0x00},		
//		{0x66, 0xC0}, 								// For Video_In
		{0x67, 0x48}, {0x0D, 0x41},
		{0x0E, 0x01}, {0x0F, 0xC5}, {0x14, 0x11},
		{0x22, 0x7F}, {0x23, 0x03}, {0x24, 0x40},
		{0x25, 0x30}, {0x26, 0xA1}, {0x2B, 0x00},
		{0x6B, 0xAA}, {0x13, 0xEF}, {0x90, 0x05},
		{0x91, 0x01}, {0x92, 0x03}, {0x93, 0x00},
		{0x94, 0x90}, {0x95, 0x8A}, {0x96, 0x06},
		{0x97, 0x0B}, {0x98, 0x95}, {0x99, 0xA0},
		{0x9A, 0x1E}, {0x9B, 0x08}, {0x9C, 0x20},
		{0x9E, 0x81}, {0xA6, 0x04}, {0x7E, 0x0C},
		{0x7F, 0x24}, {0x80, 0x3A}, {0x81, 0x60},
		{0x82, 0x70}, {0x83, 0x7E}, {0x84, 0x8A},
		{0x85, 0x94}, {0x86, 0x9E}, {0x87, 0xA8},
		{0x88, 0xB4}, {0x89, 0xBE}, {0x8A, 0xCA},
		{0x8B, 0xD8}, {0x8C, 0xE2}, {0x8D, 0x28},
		{0x46, 0x05}, {0x47, 0x00}, {0x48, 0x00},
		{0x49, 0x12}, {0x4A, 0x00}, {0x4B, 0x13},
		{0x4C, 0x21}, 
		{0x0C, 0x10}, 
//		{0x0C, 0x00},								// For Video_In
		{0x09, 0x00},
		{0xFF, 0xFF}, {0xFF, 0xFF}	
};

static struct OV_RegTable g_OV_InitTable[] =
{//8 bit slave address, 8 bit data. 
	{0, 0},
	{0, 0},//{g_sOV6880_RegValue,	_REG_TABLE_SIZE(g_sOV6880_RegValue)},		
	{0, 0},//{g_sOV7648_RegValue,	_REG_TABLE_SIZE(g_sOV7648_RegValue)},
	{0, 0},//{g_sOV7670_RegValue,	_REG_TABLE_SIZE(g_sOV7670_RegValue)},
	{g_sOV7725_RegValue,	_REG_TABLE_SIZE(g_sOV7725_RegValue)},	
	{0, 0},//{g_sOV2640_RegValue,	_REG_TABLE_SIZE(g_sOV2640_RegValue)},	
	{0, 0},//{g_sOV9660_RegValue,	_REG_TABLE_SIZE(g_sOV9660_RegValue)},
	{0, 0}
};

static UINT8 g_uOvDeviceID[]= 
{
	0x00,		// not a device ID
	0xc0,		// ov6680
	0x42,		// ov7648
	0x42,		// ov7670
	0x42,		// ov7725
	0x60,		// ov2640
	0x60,		// 0v9660
	0x00		// not a device ID
};

#ifdef __3RD_PORT__
/*
	Sensor power down and reset may default control on sensor daughter board.
	Reset by RC.
	Sensor alway power on (Keep low)

*/
static void SnrReset(void)
{
/* GPB02 reset:	H->L->H 	*/				
	//gpio_open(GPIO_PORTB);					//GPIOB2 as GPIO		
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB2));
	
	gpio_setportval(GPIO_PORTB, 1<<2, 1<<2);	//GPIOB 2 set high default
	gpio_setportpull(GPIO_PORTB, 1<<2, 1<<2);	//GPIOB 2 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<2, 1<<2);	//GPIOB 2 output mode 
	Delay(1000);			
	gpio_setportval(GPIO_PORTB, 1<<2, 0<<2);	//GPIOB 2 set low
	Delay(1000);				
	gpio_setportval(GPIO_PORTB, 1<<2, 1<<2);	//GPIOb 2 set high

}

static void SnrPowerDown(BOOL bIsEnable)
{/* GPB3 power down, HIGH for power down */

	//gpio_open(GPIO_PORTB);						//GPIOB as GPIO
	outp32(REG_GPBFUN, inp32(REG_GPBFUN) & (~MF_GPB3));
	
	gpio_setportval(GPIO_PORTB, 1<<3, 1<<3);		//GPIOB 3 set high default
	gpio_setportpull(GPIO_PORTB, 1<<3, 1<<3);		//GPIOB 3 pull-up 
	gpio_setportdir(GPIO_PORTB, 1<<3, 1<<3);		//GPIOB 3 output mode 				
	if(bIsEnable)
		gpio_setportval(GPIO_PORTB, 1<<3, 1<<3);	//GPIOB 3 set high
	else				
		gpio_setportval(GPIO_PORTB, 1<<3, 0);		//GPIOB 3 set low
}
#endif 


VOID OV7725_Init(UINT32 nIndex)
{
	UINT32 u32Idx;
	UINT32 u32TableSize;
	UINT8  u8DeviceID;
	UINT8 u8ID;
	struct OV_RegValue *psRegValue;
	DBG_PRINTF("Sensor ID = %d\n", nIndex);
	if ( nIndex >= (sizeof(g_uOvDeviceID)/sizeof(UINT8)) )
		return;
	videoIn_Open(48000, 24000);								/* For sensor clock output */	
#ifdef __3RD_PORT__
	SnrPowerDown(FALSE);
	SnrReset();		  	 										
#endif		
	u32TableSize = g_OV_InitTable[nIndex].u32TableSize;
	psRegValue = g_OV_InitTable[nIndex].sRegTable;
	u8DeviceID = g_uOvDeviceID[nIndex];
	DBG_PRINTF("Device Slave Addr = 0x%x\n", u8DeviceID);
// mask by inchu for I2C read sequence can progressive
//	if ( psRegValue == 0 )
//		return;	
#ifdef __GPIO_PIN__		
	gpio_open(GPIO_PORTB, 13);				//GPIOB 13 as GPIO
	gpio_open(GPIO_PORTB, 14);				//GPIOB 14 as GPIO
#else	
	gpio_open(GPIO_PORTB);				//GPIOB as GPIO
	
#endif 	
	DrvI2C_Open(eDRVGPIO_GPIOB, 					
				eDRVGPIO_PIN13, 
				eDRVGPIO_GPIOB,
				eDRVGPIO_PIN14, 
				(PFN_DRVI2C_TIMEDELY)Delay);
									
	for(u32Idx=0;u32Idx<u32TableSize; u32Idx++, psRegValue++)
	{
		I2C_Write_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, (psRegValue->u8RegAddr), (psRegValue->u8Value));	
		if ((psRegValue->u8RegAddr)==0x12 && (psRegValue->u8Value)==0x80)
		{	
			Delay(1000);		
			DBG_PRINTF("Delay A loop\n");
		}				
	}
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x0A);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x0B);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1C);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x1D);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);

	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0xD7);
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	u8ID = I2C_Read_8bitSlaveAddr_8bitReg_8bitData(u8DeviceID, 0x6A);	
	DBG_PRINTF("Sensor ID0 = 0x%x\n", u8ID);
	
	DrvI2C_Close();	
}


/*===================================================================
	LCD dimension = (OPT_LCD_WIDTH, OPT_LCD_HEIGHT)	
	Packet dimension = (OPT_PREVIEW_WIDTH*OPT_PREVIEW_HEIGHT)	
	Stride should be LCM resolution  OPT_LCD_WIDTH.
	Packet frame start address = VPOST frame start address + (OPT_LCD_WIDTH-OPT_PREVIEW_WIDTH)/2*2 	
=====================================================================*/
UINT32 Smpl_OV7725(UINT8* pu8FrameBuffer0, UINT8* pu8FrameBuffer1)
{
	PFN_VIDEOIN_CALLBACK pfnOldCallback;
	UINT32 u32GCD;
	
	
	#ifdef __3RD_PORT__
		// GPIOD2 pull high
		gpio_setportval(GPIO_PORTD, 0x04, 0x04);    //GPIOD2 high to enable Amplifier 
		gpio_setportpull(GPIO_PORTD, 0x04, 0x04);	//GPIOD2 pull high
		gpio_setportdir(GPIO_PORTD, 0x04, 0x04);	//GPIOD2 output mode
	#endif 

#ifdef __1ST_PORT__	
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_SNR_CCIR601);
#endif
#ifdef __2ND_PORT__
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_2ND_SNR_CCIR601);	
#endif	
#ifdef __3RD_PORT__
	videoIn_Init(TRUE, 0, 24000, eVIDEOIN_3RD_SNR_CCIR601);	
#endif
	outp32(REG_AHBCLK, inp32(REG_AHBCLK) | HCLK4_CKE);
	OV7725_Init(4);	
			
	videoIn_Open(48000, 24000);		
	videoIn_EnableInt(eVIDEOIN_VINT);
	
	videoIn_InstallCallback(eVIDEOIN_VINT, 
						(PFN_VIDEOIN_CALLBACK)VideoIn_InterruptHandler,
						&pfnOldCallback	);	//Frame End interrupt
						
	videoIn_SetPacketFrameBufferControl(FALSE, FALSE);	
	
	videoinIoctl(VIDEOIN_IOCTL_SET_POLARITY,
				TRUE,
				FALSE,							//Polarity.	
				TRUE);							
												
	videoinIoctl(VIDEOIN_IOCTL_ORDER_INFMT_OUTFMT,								
				eVIDEOIN_IN_UYVY, 			//Input Order 
				eVIDEOIN_IN_YUV422	,		//Intput format
				eVIDEOIN_OUT_YUV422);		//Output format for packet 														
		
	videoinIoctl(VIDEOIN_IOCTL_SET_CROPPING_START_POSITION,				
				0,							//Vertical start position
				0,							//Horizontal start position	
				0);							//Useless
	videoinIoctl(VIDEOIN_IOCTL_CROPPING_DIMENSION,				
				OPT_CROP_HEIGHT,							//UINT16 u16Height, 
				OPT_CROP_WIDTH,							//UINT16 u16Width;	
				0);							//Useless
	u32GCD = GCD(OPT_PREVIEW_HEIGHT, OPT_CROP_HEIGHT);						 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PACKET,			//272/480
				480/u32GCD,
				OPT_CROP_HEIGHT/u32GCD);		
	u32GCD = GCD(OPT_PREVIEW_WIDTH, OPT_CROP_WIDTH);																
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PACKET,			//364/640
				640/u32GCD,
				OPT_CROP_WIDTH/u32GCD);		
	u32GCD = GCD(480, 480);						 							 
	videoinIoctl(VIDEOIN_IOCTL_VSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				480/u32GCD,
				480/u32GCD);		
	u32GCD = GCD(640, 640);																
	videoinIoctl(VIDEOIN_IOCTL_HSCALE_FACTOR,
				eVIDEOIN_PLANAR,			//640/640
				640/u32GCD,
				640/u32GCD);
	
		/* Set Pipes stride */				
	videoinIoctl(VIDEOIN_IOCTL_SET_STRIDE,										
				OPT_STRIDE,				
				640,   //OPT_ENCODE_WIDTH,
				0);
	/* Set Packet Buffer Address */					
	videoinIoctl(VIDEOIN_IOCTL_SET_BUF_ADDR,
				eVIDEOIN_PACKET,			
				0, 							//Packet buffer addrress 0	
				(UINT32)((UINT32)pu8FrameBuffer0 + (OPT_STRIDE-OPT_PREVIEW_WIDTH)/2*2) );	
	
	videoinIoctl(VIDEOIN_IOCTL_SET_PIPE_ENABLE,
				TRUE, 						// Engine enable ?
				eVIDEOIN_PACKET,			// which packet was enable. 											
				0 );							//Useless		
							
	sysSetLocalInterrupt(ENABLE_IRQ);						
										
														
	return Successful;			
}	