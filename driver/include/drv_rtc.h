/*
 * Module:	RTC driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _DRV_RTC_H_
#define _DRV_RTC_H_


#include "global.h"


//Constant define

#define DRV_RTC_TEST_ENABLE			0


//Type definition

typedef enum
{
	DRV_RTC_PARAM_RESET = 0,
	DRV_RTC_PARAM_TICK_CYCLE,
	DRV_RTC_PARAM_CALLBACK,
	DRV_RTC_PARAM_SWITCH_WAKEUP,
	DRV_RTC_PARAM_SWITCH_ALARM,
	DRV_RTC_PARAM_CALENDAR_YEAR,
	DRV_RTC_PARAM_CALENDAR_MONTH,
	DRV_RTC_PARAM_CALENDAR_DAY,
	DRV_RTC_PARAM_CALENDAR_HOUR,
	DRV_RTC_PARAM_CALENDAR_MINUTE,
	DRV_RTC_PARAM_CALENDAR_SECOND,
	DRV_RTC_PARAM_ALARM_WEEKDAY,
	DRV_RTC_PARAM_ALARM_HOUR,
	DRV_RTC_PARAM_ALARM_MINUTE,
	DRV_RTC_COUNT_PARAM
} drv_rtc_param;

typedef enum
{
	DRV_RTC_SWITCH_OFF,
	DRV_RTC_SWITCH_ON
} drv_rtc_switch;

typedef void (*drv_rtc_callback_wakeup)
(
	uint16 u16_TickTime
);

typedef void (*drv_rtc_callback_alarm)(void);

typedef struct
{
	drv_rtc_callback_wakeup fp_Wakeup;
	drv_rtc_callback_alarm fp_Alarm;
} drv_rtc_callback;


//Function declaration

uint DrvRTC_Initialize(void);
uint DrvRTC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint DrvRTC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void DrvRTC_Interrupt(void);

#if DRV_RTC_TEST_ENABLE == 1
void DrvRTC_Test(void);
#endif

#endif
