/*
 * Module:	Device operating system
 * Author:	Lvjianfeng
 * Date:	2012.5
 */

#ifndef _DEVOS_H_
#define _DEVOS_H_


#include "global.h"


//Constant definition

#ifndef DEVOS_COOPPERATIVE
#define DEVOS_COOPPERATIVE				1
#endif

#ifndef DEVOS_TASK_COUNT_MAX			
#define DEVOS_TASK_COUNT_MAX			8
#endif

#ifndef DEVOS_PRIORITY_COUNT_MAX
#define DEVOS_PRIORITY_COUNT_MAX		8
#endif

#ifndef DEVOS_MESSAGE_COUNT_MAX
#define DEVOS_MESSAGE_COUNT_MAX			8
#endif

#ifndef DEVOS_SEMAPHORE_COUNT_MAX
#define DEVOS_SEMAPHORE_COUNT_MAX		0
#endif


//Type definition

#ifndef devos_int
#define devos_int 						uint
#endif

typedef devos_int devos_task_handle;

typedef void (*devos_callback_enter_critical)(void);

typedef void (*devos_callback_exit_critical)(void);

typedef uint (*devos_callback_task_initialize)(void);

typedef void (*devos_callback_task_process)
(
	devos_task_handle t_TaskHandle
);

typedef struct
{
	devos_int t_Priority;
	devos_callback_task_process fp_TaskProcess;
} devos_task_profile;


//Function declaration

void DevOS_Initialize
(
	devos_callback_enter_critical fp_EnterCritical,
	devos_callback_exit_critical fp_ExitCritical
);
void DevOS_Schedule(void);
void DevOS_Tick
(
	uint16 u16_TickTime
);

uint DevOS_TaskCreate
(
	devos_task_handle *tp_TaskHandle,
	const devos_task_profile *tp_TaskProfile
);
uint DevOS_TaskDelete
(
	devos_task_handle t_TaskHandle
);

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
uint DevOS_MessageInitialize
(
	devos_int t_MessageID,
	uint8 *u8p_Buffer,
	devos_int t_Length
);
uint DevOS_MessageRegister
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
);
uint DevOS_MessageUnregister
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
);
uint DevOS_MessageClear
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
);
uint DevOS_MessageSend
(
	devos_int t_MessageID,
	const uint8 *u8p_Data,
	devos_int t_Length
);
uint8* DevOS_MessageReceive
(
	devos_int t_MessageID,
	devos_int *tp_Length
);
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
uint DevOS_SemaphoreInitialize
(
	devos_int t_SemaphoreID,
	devos_int t_Count
);
uint DevOS_SemaphoreObtain
(
	devos_int t_SemaphoreID
);
uint DevOS_SemaphorePost
(
	devos_int t_SemaphoreID
);
#endif

void DevOS_CoreTaskDelay
(
	uint16 u16_Time
);

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
uint8* DevOS_CoreMessageWait
(
	devos_int t_MessageID,
	devos_int *tp_Length
);
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
devos_int DevOS_CoreSemaphorePend
(
	devos_int t_SemaphoreID
);
#endif

#if DEVOS_COOPPERATIVE != 0
void DevOS_CoreTaskSetContext
(
	uint16 u16_Context
);
uint16 DevOS_CoreTaskGetContext(void);
#endif


//Macro definition

#if DEVOS_COOPPERATIVE != 0

#define DEVOS_TASK_BEGIN				switch (DevOS_CoreTaskGetContext()) \
										{ \
										case 0:
#define DEVOS_TASK_SCHEDULE				DevOS_CoreTaskSetContext(__LINE__); \
										return; \
										case __LINE__:
#define DEVOS_TASK_END					DevOS_CoreTaskSetContext((uint16)(~0)); \
										return; \
										}

#define DevOS_TaskYield()				DEVOS_TASK_SCHEDULE;
#define DevOS_TaskDelay(time)			DevOS_CoreTaskDelay(time); \
										DEVOS_TASK_SCHEDULE;

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
#define DevOS_MessageWait(id,data,len)	data = DevOS_CoreMessageWait(id,len); \
										while (data == (uint8 *)0) \
										{ \
											DEVOS_TASK_SCHEDULE; \
											data = DevOS_MessageReceive(id,len); \
											break; \
										} 
										
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
#define DevOS_SemaphorePend(id)			while (DevOS_CoreSemaphorePend(id) == 0) \
										{ \
											DEVOS_TASK_SCHEDULE; \
											break; \
										} 
										
#endif

#else

#define DEVOS_TASK_BEGIN				
#define DEVOS_TASK_SCHEDULE				
#define DEVOS_TASK_END					

#define DevOS_TaskYield()				
#define DevOS_TaskDelay(time)			DevOS_CoreTaskDelay(time)

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
#define DevOS_MessageWait(id,data,len)	data = DevOS_CoreMessageWait(id,len);
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
#define DevOS_SemaphorePend(id)			DevOS_CoreSemaphorePend(id);
#endif

#endif

#endif
