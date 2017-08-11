/*
 * Module:	Task manager
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _TASK_H_
#define _TASK_H_


#include "global.h"
#include "message.h"


//Constant define

#define TASK_FILE_SYSTEM_VOLUME_ID		0

#define TASK_CALIBRATE_TIME(time)		(uint16)(((uint32)(time) * 100) / 98)


//Type definition

typedef enum
{
	TASK_FILE_ID_SETTING = 0,
	TASK_FILE_ID_HISTORY,
	TASK_FILE_ID_TEST,
	TASK_COUNT_FILE_ID
} task_file_id;

typedef enum
{
	TASK_MESSAGE_ID_SYSTEM = 0,
	TASK_MESSAGE_ID_DEVICE,
	TASK_MESSAGE_ID_GLUCOSE,
	TASK_MESSAGE_ID_SETTING,
	TASK_MESSAGE_ID_HISTORY,
	TASK_MESSAGE_ID_SHELL,
	TASK_MESSAGE_ID_TICK,
	TASK_COUNT_MESSAGE_ID
} task_message_id;

typedef enum
{
	TASK_MESSAGE_DATA_INVALID = 0,
	TASK_MESSAGE_DATA_QUIT,
	TASK_MESSAGE_DATA_SLEEP,
	TASK_MESSAGE_DATA_ALARM,
	TASK_MESSAGE_DATA_SYSTEM,
	TASK_COUNT_MESSAGE_DATA
} task_message_data;

typedef enum
{
	TASK_PORT_SETTING = 0,
	TASK_PORT_COMM,
	TASK_PORT_SHELL,
	TASK_PORT_GLUCOSE,
	TASK_PORT_DEVICE,
	TASK_PORT_HISTORY,
	TASK_COUNT_PORT
} task_port;


//Function declaration

uint Task_Initialize(void);
uint Task_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
);
uint Task_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
);
void Task_Process(void);
uint Task_ObtainMessageID
(
	uint *uip_MessageID
);
void Task_EnterCritical(void);
void Task_ExitCritical(void);

#endif
