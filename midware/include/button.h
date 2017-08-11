/*
 * Module:	Button
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_


#include "global.h"


//Constant define

#define BUTTON_TEST_ENABLE			0


//Type definition

typedef enum
{
	BUTTON_PARAM_LONG_PRESS_DELAY,
	BUTTON_PARAM_CALLBACK,
	BUTTON_COUNT_PARAM
} button_param;

typedef enum
{
	BUTTON_ID_LEFT,
	BUTTON_ID_CENTER,
	BUTTON_ID_RIGHT,
	BUTTON_COUNT_ID
} button_id;

typedef enum
{
	BUTTON_STATE_INVALID,
	BUTTON_STATE_PRESS,
	BUTTON_STATE_LONG_PRESS,
	BUTTON_STATE_RELEASE,
	BUTTON_COUNT_STATE
} button_state;


//Function declaration

uint Button_Initialize(void);
uint Button_SetConfig
(
	uint ui_ButtonID,
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint Button_GetConfig
(
	uint ui_ButtonID,
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint Button_Read
(
	uint ui_ButtonID
);
void Button_Tick
(
	uint16 u16_TickTime
);

#if BUTTON_TEST_ENABLE == 1
void Button_Test(void);
#endif

#endif
