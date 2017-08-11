/*
 * Module:	DAC driver
 * Author:	Lvjianfeng
 * Date:	2014.4
 */

#ifndef _DRV_DAC_H_
#define _DRV_DAC_H_


#include "global.h"


//Constant define

#define DRV_DAC_RESOLUTION				12

#define DRV_DAC_TEST_ENABLE				1


//Type definition

typedef enum
{
	DRV_DAC_PARAM_REFERENCE = 0,
	DRV_DAC_PARAM_FREQUENCY,
	DRV_DAC_COUNT_PARAM
} drv_dac_param;


//Function declaration

uint DrvDAC_Initialize(void);
uint DrvDAC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvDAC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void DrvDAC_Enable
(
	uint16 *u16p_Data,
	uint ui_Length
);
void DrvDAC_Disable(void);

#if DRV_DAC_TEST_ENABLE == 1
void DrvDAC_Test(void);
#endif

#endif
