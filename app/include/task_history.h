/*
 * Module:	History manager task
 * Author:	Lvjianfeng
 * Date:	2012.9
 */

#ifndef _TASK_HISTORY_H_
#define _TASK_HISTORY_H_


#include "global.h"
#include "devos.h"


//Constant define

#define TASK_HISTORY_TEST_ENABLE		0


//Type definition

typedef enum
{
	TASK_HISTORY_PARAM_DATE_TIME = 0,
	TASK_HISTORY_PARAM_HISTORY,
	TASK_HISTORY_COUNT_PARAM
} task_history_param;


//Function declaration

uint TaskHistory_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskHistory_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskHistory_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskHistory_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);

#if TASK_HISTORY_TEST_ENABLE == 1
void TaskHistory_Test(void);
#endif

#endif
