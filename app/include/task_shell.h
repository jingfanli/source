/*
 * Module:	Shell task
 * Author:	Lvjianfeng
 * Date:	2012.1
 */

#ifndef _TASK_SHELL_H_
#define _TASK_SHELL_H_


#include "global.h"
#include "devos.h"


//Constant definition


//Type definition

typedef enum
{
	TASK_SHELL_PARAM_COMMAND = 0,
	TASK_SHELL_COUNT_PARAM
} task_shell_parameter;

typedef enum
{
	TASK_SHELL_COMMAND_OUTPUT_HISTORY = 0,
	TASK_SHELL_COMMAND_OUTPUT_INFO,
	TASK_SHELL_COMMAND_OUTPUT_STRIP,
	TASK_SHELL_COMMAND_OUTPUT_TEST,
	TASK_SHELL_COMMAND_OUTPUT_RE,
	TASK_SHELL_COUNT_COMMAND
} task_shell_command;


//Function declaration

uint TaskShell_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskShell_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskShell_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskShell_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);

#endif
