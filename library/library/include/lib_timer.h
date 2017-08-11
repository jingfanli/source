/*
 * Module:	Software timer library
 * Author:	Lvjianfeng
 * Date:	2011.8
 */

#ifndef _LIB_TIMER_H_
#define _LIB_TIMER_H_


#include "global.h"


//Constant define

#ifndef LIB_TIMER_COUNT_MAX
#define LIB_TIMER_COUNT_MAX			8
#endif

#ifndef LIB_TIMER_LIST_ENABLE
#define LIB_TIMER_LIST_ENABLE		0
#endif

#ifndef LIB_TIMER_TEST_ENABLE
#define LIB_TIMER_TEST_ENABLE		0
#endif


//Type definition

typedef enum
{
	LIB_TIMER_PARAM_OVERFLOW,
	LIB_TIMER_PARAM_TICK,
	LIB_TIMER_PARAM_RELOAD,
	LIB_TIMER_COUNT_PARAM
} lib_timer_param;


//Function declaration

uint LibTimer_Initialize(void);
uint LibTimer_SetConfig
(
	uint ui_TimerChannel,
	uint ui_Parameter,
	const void *vp_Value
);
uint LibTimer_GetConfig
(
	uint ui_TimerChannel,
	uint ui_Parameter,
	void *vp_Value
);
uint LibTimer_Create
(
	uint *uip_TimerChannel
);
uint LibTimer_Delete
(
	uint ui_TimerChannel
);
uint LibTimer_Start
(
	uint ui_TimerChannel
);
uint LibTimer_Stop
(
	uint ui_TimerChannel
);
void LibTimer_Tick
(
	uint16 u16_TickTime
);

#if LIB_TIMER_TEST_ENABLE == 1
void LibTimer_Test(void);
#endif

#endif
