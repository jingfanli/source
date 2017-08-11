/*
 * Module:	Blood glucose meter 
 * Author:	Lvjianfeng
 * Date:	2011.12
 */

#ifndef _GLUCOSE_H_
#define _GLUCOSE_H_


#include "global.h"


//Constant definition

#define GLUCOSE_TEST_ENABLE			0

#define GLUCOSE_TEMPERATURE_RATIO	10


//Type definition

typedef enum
{
	GLUCOSE_PARAM_MODE = 0,
	GLUCOSE_PARAM_SWITCH,
	GLUCOSE_PARAM_SIGNAL_PRESENT,
	GLUCOSE_PARAM_GLUCOSE_CHANNEL,
	GLUCOSE_PARAM_REFERENCE_BG,
	GLUCOSE_PARAM_REFERENCE_HCT,
	GLUCOSE_PARAM_HCT_WAVEFORM,
	GLUCOSE_PARAM_CALLBACK,
	GLUCOSE_COUNT_PARAM
} glucose_param;

typedef enum
{
	GLUCOSE_MODE_OFF = 0,
	GLUCOSE_MODE_BG1,
	GLUCOSE_MODE_BG2,
	GLUCOSE_MODE_HCT,
	GLUCOSE_COUNT_MODE
} glucose_mode;

typedef enum
{
	GLUCOSE_CHANNEL_BG = 0,
	GLUCOSE_CHANNEL_HCT,
	GLUCOSE_CHANNEL_TEMPERATURE,
	GLUCOSE_CHANNEL_NTC,
	GLUCOSE_CHANNEL_TEST,
	GLUCOSE_COUNT_CHANNEL
} glucose_channel;

typedef enum
{
	channel_RE10=0,
	channel_RE2,
	channel_RE3,
	channel_RE6,
	channel_RE8,
	CHANNEL_RECONUT
} channel_re;




//Function declaration
uint Glucose_re_Initialize(void);
uint Glucose_re8_back(void);

void  re_cactu(uint16 *tab);

uint Glucose_Initialize(void);
uint Glucose_re_back(void);

uint Glucose_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint Glucose_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint16 Glucose_Read
(
	uint ui_Channel
);
void Glucose_Enable(void);
void Glucose_Disable(void);
void Glucose_Sample(void);
sint32 Glucose_Round
(
	sint32 s32_Value,
	sint32 s32_Base
);

#if GLUCOSE_TEST_ENABLE == 1
void Glucose_Test(void);
#endif

#endif
