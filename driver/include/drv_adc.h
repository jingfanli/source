/*
 * Module:	ADC driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _DRV_ADC_H_
#define _DRV_ADC_H_


#include "global.h"


//Constant define

#define DRV_ADC_RESOLUTION				12

#define DRV_ADC_TEST_ENABLE				0

#define DRV_ADC_GET_VOLTAGE(vref, adref, advalue)	(uint16)(((uint32)(vref) * \
													(uint32)(advalue)) / (uint32)(adref))


//Type definition

typedef enum
{
	DRV_ADC_PARAM_REFERENCE = 0,
	DRV_ADC_COUNT_PARAM
} drv_adc_param;

typedef enum
{
	DRV_ADC_CHANNEL_HCT = 0,
	DRV_ADC_CHANNEL_RE6,
	DRV_ADC_CHANNEL_RE3,
	DRV_ADC_CHANNEL_RE2,
	DRV_ADC_CHANNEL_RE8,
	DRV_ADC_CHANNEL_BG,
	DRV_ADC_CHANNEL_VDD,
	DRV_ADC_CHANNEL_NTC,
	DRV_ADC_CHANNEL_REF,
	DRV_ADC_CHANNEL_TEMP,
	DRV_ADC_COUNT_CHANNEL


} drv_adc_channel;


//Function declaration

uint DrvADC_Initialize(void);
uint DrvADC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvADC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint DrvADC_Read
(
	uint ui_Channel,
	uint16 *u16p_Value
);
void DrvADC_Enable(void);
void DrvADC_Disable(void);
void DrvADC_Interrupt(void);
void DrvADC_Sample(void);

#if DRV_ADC_TEST_ENABLE == 1
void DrvADC_Test(void);
#endif

#endif
