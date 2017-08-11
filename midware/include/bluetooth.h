/*
 * Module:	Bluetooth
 * Author:	Lvjianfeng
 * Date:	2014.7
 */

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_


#include "global.h"


//Constant define

#define BLUETOOTH_TEST_ENABLE			0


//Type definition

typedef enum
{
	BLUETOOTH_PARAM_SWITCH = 0,
	BLUETOOTH_PARAM_SIGNAL_PRESENT,
	BLUETOOTH_PARAM_CONNECTION,
	BLUETOOTH_PARAM_CALLBACK,
	BLUETOOTH_COUNT_PARAM
} bluetooth_param;


//Function declaration

uint Bluetooth_Initialize(void);
uint Bluetooth_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint Bluetooth_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void Bluetooth_Tick
(
	uint16 u16_TickTime
);

#if BLUETOOTH_TEST_ENABLE == 1
void Bluetooth_Test(void);
#endif

#endif
