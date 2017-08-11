/*
 * Module:	Communication manager task
 * Author:	Lvjianfeng
 * Date:	2011.9
 */

#ifndef _TASK_COMM_H_
#define _TASK_COMM_H_


#include "global.h"
#include "message.h"
#include "devos.h"


//Constant define

#define TASK_COMM_TEST_ENABLE			1


//Type definition

typedef enum
{
	TASK_COMM_PARAM_RF_STATE = 0,
	TASK_COMM_PARAM_ENABLE,
	TASK_COMM_PARAM_DISABLE,
	TASK_COMM_PARAM_BUSY,
	TASK_COMM_COUNT_PARAM
} task_comm_parameter;


//Function declaration

uint TaskComm_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskComm_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskComm_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskComm_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
uint TaskComm_Send
(
	uint8 u8_Address,
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	const message_command *tp_Command,
	uint8 u8_Mode
);
uint TaskComm_Receive
(
	uint8 u8_Address,
	uint8 *u8p_SourcePort,
	uint8 *u8p_TargetPort,
	message_command *tp_Command,
	uint8 *u8p_Mode
);

#if TASK_COMM_TEST_ENABLE == 1
void TaskComm_Test(void);
#endif

#endif
