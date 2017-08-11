/*
 * Module:	Beep driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _DRV_BEEP_H_
#define _DRV_BEEP_H_


#include "global.h"


//Constant definition

#define DRV_BEEP_TEST_ENABLE		0


//Type definition

typedef enum
{
	DRV_BEEP_PARAM_FREQUENCY = 0,
	DRV_BEEP_PARAM_SWITCH,
	DRV_BEEP_COUNT_PARAM
} drv_beep_param;

typedef enum
{
	DRV_BEEP_FREQUENCY_1000 = 0,
	DRV_BEEP_FREQUENCY_2000,
	DRV_BEEP_FREQUENCY_4000,
	DRV_BEEP_COUNT_FREQUENCY
} drv_beep_frequency;


//Function declaration

uint DrvBEEP_Initialize(void);
uint DrvBEEP_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvBEEP_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void DrvBEEP_Enable(void);
void DrvBEEP_Disable(void);
void DrvBEEP_Start
(
	uint ui_BeepCount,
	uint16 u16_OnInterval,
	uint16 u16_OffInterval
);
void DrvBEEP_Stop(void);
void DrvBEEP_Tick
(
	uint16 u16_TickTime
);

#if DRV_BEEP_TEST_ENABLE == 1
void DrvBEEP_Test(void);
#endif

#endif
