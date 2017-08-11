/*
 * Module:	Device manager task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _TASK_DEVICE_H_
#define _TASK_DEVICE_H_


#include "global.h"
#include "devos.h"


//Constant define

#define TASK_DEVICE_TEST_ENABLE				0

#define TASK_DEVICE_ALARM_COUNT				10

#define TASK_DEVICE_ERROR_ID_BASE			1000
#define TASK_DEVICE_ERROR_ID_CODE			(TASK_DEVICE_ERROR_ID_BASE + 1)
#define TASK_DEVICE_ERROR_ID_NBB			(TASK_DEVICE_ERROR_ID_BASE + 2)
#define TASK_DEVICE_ERROR_ID_BG2			(TASK_DEVICE_ERROR_ID_BASE + 3)
#define TASK_DEVICE_ERROR_ID_STRIP			(TASK_DEVICE_ERROR_ID_BASE + 4)
#define TASK_DEVICE_ERROR_ID_TEMPERATURE	(TASK_DEVICE_ERROR_ID_BASE + 5)
#define TASK_DEVICE_ERROR_ID_BATTERY		(TASK_DEVICE_ERROR_ID_BASE + 6)
#define TASK_DEVICE_ERROR_ID_HCT			(TASK_DEVICE_ERROR_ID_BASE + 7)
#define TASK_DEVICE_ERROR_ID_OVERFLOW		(TASK_DEVICE_ERROR_ID_BASE + 8)
#define TASK_DEVICE_ERROR_ID_UNDERFLOW		(TASK_DEVICE_ERROR_ID_BASE + 9)


//Type definition

typedef enum
{
	TASK_DEVICE_PARAM_AUDIO = 0,
	TASK_DEVICE_PARAM_TIME_FORMAT,
	TASK_DEVICE_PARAM_BLUETOOTH,
	TASK_DEVICE_PARAM_VERSION_MAJOR,
	TASK_DEVICE_PARAM_VERSION_MINOR,
	TASK_DEVICE_PARAM_MEAL_FLAG,
	TASK_DEVICE_PARAM_GLUCOSE_UNIT,
	TASK_DEVICE_PARAM_NO_CODE,
	TASK_DEVICE_PARAM_ALARM_WEEKDAY,
	TASK_DEVICE_PARAM_ALARM_HOUR,
	TASK_DEVICE_PARAM_ALARM_MINUTE,
	TASK_DEVICE_PARAM_ALARM_SWITCH,
	TASK_DEVICE_PARAM_ID,
	TASK_DEVICE_PARAM_MODEL,
	TASK_DEVICE_COUNT_PARAM
} task_device_param;


//Function declaration

uint TaskDevice_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskDevice_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskDevice_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskDevice_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint TaskDevice_SetAlarm
(
	uint ui_AlarmID,
	uint ui_Parameter,
	uint ui_Value
);
uint TaskDevice_GetAlarm
(
	uint ui_AlarmID,
	uint ui_Parameter,
	uint *uip_Value
);
void TaskDevice_UpdateAlarm(void);
void TaskDevice_NumberToBCD
(
	uint8 *u8p_BCD,
	uint *uip_Length,
	uint16 u16_Number
);
void TaskDevice_InitializeDisplay(void);
void TaskDevice_DisplayMonth
(
	uint8 u8_Month
);
void TaskDevice_DisplayDay
(
	uint8 u8_Day
);
void TaskDevice_DisplayHour
(
	uint8 u8_Hour,
	uint8 u8_TimeFormat
);
void TaskDevice_DisplayMinute
(
	uint8 u8_Minute
);
void TaskDevice_DisplayGlucose
(
	uint16 u16_Glucose
);
void TaskDevice_DisplayUnit
(
	uint8 u8_LCDData
);
uint TaskDevice_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
);

#if TASK_DEVICE_TEST_ENABLE == 1
void TaskDevice_Test(void);
#endif

#endif
