/*
 * Module:	Setting manager task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _TASK_SETTING_H_
#define _TASK_SETTING_H_


#include "global.h"
#include "devos.h"


//Constant definition


//Type definition

typedef enum
{
	TASK_SETTING_PARAM_DEVICE = 0,
	TASK_SETTING_COUNT_PARAM
} task_setting_param;


//Function declaration

uint TaskSetting_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskSetting_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskSetting_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskSetting_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);

#endif
