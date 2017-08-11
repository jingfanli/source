/*
 * Module:	Device operating system
 * Author:	Lvjianfeng
 * Date:	2012.5
 */


#include "devos.h"


//Constant definition


//Type definition

typedef enum
{
	DEVOS_TASK_FLAG_VALID,
	DEVOS_COUNT_TASK_FLAG
} devos_task_flag;

typedef enum
{
	DEVOS_TASK_PENDING_DELAY,
#if (DEVOS_MESSAGE_COUNT_MAX > 0)
	DEVOS_TASK_PENDING_MESSAGE,
#endif
#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
	DEVOS_TASK_PENDING_SEMAPHORE,
#endif
	DEVOS_COUNT_TASK_PENDING
} devos_task_pending;

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
typedef enum
{
	DEVOS_MESSAGE_STATE_REGISTERED,
	DEVOS_MESSAGE_STATE_RECEIVED,
	DEVOS_COUNT_MESSAGE_STATE
} devos_message_state;
#endif

typedef struct _devos_task
{
	uint16 u16_Context;
	uint16 u16_DelayTime;
	devos_int t_TaskID;
	devos_int t_Flag;
	devos_int t_Pending;
	devos_int t_Priority;
#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
	devos_int t_SemaphoreID;
#endif
#if (DEVOS_MESSAGE_COUNT_MAX > 0)
	devos_int t_MessageID;
#endif
	devos_callback_task_process fp_TaskProcess;
	struct _devos_task *tp_PriorityTask;
#if (DEVOS_MESSAGE_COUNT_MAX > 0)
	uint8 u8_MessageState[DEVOS_MESSAGE_COUNT_MAX];
#endif
} devos_task;

typedef struct
{
	devos_int t_TotalTaskCount;
	devos_int t_ReadyTaskCount;
	devos_task *tp_HeadTask;
	devos_task *tp_ReadyTask;
} devos_priority_info;

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
typedef struct
{
	uint8 *u8p_Buffer;
	devos_int t_Length;
	devos_int t_LengthMax;
} devos_message;
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
typedef struct
{
	devos_int t_Count;
	devos_int t_CountMax;
} devos_semaphore;
#endif


//Private variable definition

static devos_int m_t_TaskIDCurrent = {0};
static devos_int m_t_TaskIDMax = {0};
static devos_int m_t_PriorityMax = {0};
static devos_task m_t_Task[DEVOS_TASK_COUNT_MAX] = {0};
static devos_priority_info m_t_PriorityInfo[DEVOS_PRIORITY_COUNT_MAX] = {0};
static devos_callback_enter_critical m_fp_EnterCritical;
static devos_callback_exit_critical m_fp_ExitCritical;

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
static devos_message m_t_Message[DEVOS_MESSAGE_COUNT_MAX] = {0};
#endif

#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
static devos_semaphore m_t_Semaphore[DEVOS_SEMAPHORE_COUNT_MAX] = {0};
#endif


//Private function declaration

static devos_task *DevOS_GetReadyTask(void);
static void DevOS_SetTaskPending
(
	devos_task *tp_Task,
	devos_int t_Pending
);
static void DevOS_ClearTaskPending
(
	devos_task *tp_Task,
	devos_int t_Pending
);


//Public function definition

void DevOS_Initialize
(
	devos_callback_enter_critical fp_EnterCritical,
	devos_callback_exit_critical fp_ExitCritical
)
{
	m_fp_EnterCritical = fp_EnterCritical;
	m_fp_ExitCritical = fp_ExitCritical;
}


void DevOS_Schedule(void)
{
	devos_task *tp_Task;


	tp_Task = DevOS_GetReadyTask();

	if (tp_Task != (devos_task *)0)
	{
		m_t_TaskIDCurrent = tp_Task->t_TaskID;
		tp_Task->fp_TaskProcess((devos_task_handle)(tp_Task->t_TaskID));
	}
}


void DevOS_Tick
(
	uint16 u16_TickTime
)
{
	devos_int i;
	devos_task *tp_Task;


	tp_Task = &m_t_Task[0];

	//Update wait time of tasks
	for (i = 0; i <= m_t_TaskIDMax; i++)
	{
		if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) != 0)
		{
			if (tp_Task->u16_DelayTime > 0)
			{
				m_fp_EnterCritical();

				if (tp_Task->u16_DelayTime > u16_TickTime)
				{
					tp_Task->u16_DelayTime -= u16_TickTime;
				}
				else
				{
					tp_Task->u16_DelayTime = 0;
				}

				if (tp_Task->u16_DelayTime == 0)
				{
					DevOS_ClearTaskPending(tp_Task, DEVOS_TASK_PENDING_DELAY);
				}

				m_fp_ExitCritical();
			}
		}

		tp_Task++;
	}
}


uint DevOS_TaskCreate
(
	devos_task_handle *tp_TaskHandle,
	const devos_task_profile *tp_TaskProfile
)
{
	devos_int i;
	devos_task *tp_Task;
	devos_priority_info *tp_PriorityInfo;


	//Check if priority is out of range
	if (tp_TaskProfile->t_Priority >= DEVOS_PRIORITY_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Task = &m_t_Task[0];

	//Search for invalid task in the buffer
	for (i = 0; i < DEVOS_TASK_COUNT_MAX; i++)
	{
		if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) == 0)
		{
			break;
		}

		tp_Task++;
	}

	//Check if no invalid task is found
	if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) != 0)
	{
		return FUNCTION_FAIL;
	}

	tp_Task->u16_Context = 0;
	tp_Task->t_TaskID = i;
	tp_Task->t_Pending = 0;
	tp_Task->u16_DelayTime = 0;
	tp_Task->t_Priority = tp_TaskProfile->t_Priority;
	tp_Task->fp_TaskProcess = tp_TaskProfile->fp_TaskProcess;
	tp_Task->tp_PriorityTask = (devos_task *)0;

	*tp_TaskHandle = (devos_task_handle)i;
	tp_PriorityInfo = &m_t_PriorityInfo[tp_TaskProfile->t_Priority];

	//Add task to the tail of priority list
	if (tp_PriorityInfo->t_TotalTaskCount > 0)
	{
		tp_Task = tp_PriorityInfo->tp_HeadTask;

		//Locate tail of task list in the same priority
		while (tp_Task->tp_PriorityTask != (devos_task *)0)
		{
			tp_Task = tp_Task->tp_PriorityTask;
		}
		
		tp_Task->tp_PriorityTask = &m_t_Task[i];
	}
	else
	{
		tp_PriorityInfo->tp_HeadTask = tp_Task;
		tp_PriorityInfo->tp_ReadyTask = tp_Task;
	}

	tp_Task = &m_t_Task[i];
	tp_PriorityInfo->t_TotalTaskCount++;
	tp_PriorityInfo->t_ReadyTaskCount++;

	//Update max priority
	if (tp_TaskProfile->t_Priority > m_t_PriorityMax)
	{
		m_t_PriorityMax = tp_TaskProfile->t_Priority;
	}
	
	//Update max task ID
	if (i > m_t_TaskIDMax)
	{
		m_t_TaskIDMax = i;
	}

	REG_SET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID);

	return FUNCTION_OK;
}


uint DevOS_TaskDelete
(
	devos_task_handle t_TaskHandle
)
{
	devos_int i;
	devos_task *tp_CurrentTask;
	devos_task *tp_PreviousTask;
	devos_priority_info *tp_PriorityInfo;


	//Check if task handle is out of range
	if ((devos_int)t_TaskHandle > m_t_TaskIDMax)
	{
		return FUNCTION_FAIL;
	}
	
	tp_CurrentTask = &m_t_Task[(devos_int)t_TaskHandle];

	//Check if task is valid or not
	if (REG_GET_BIT(tp_CurrentTask->t_Flag, DEVOS_TASK_FLAG_VALID) == 0)
	{
		return FUNCTION_OK;
	}

#if (DEVOS_MESSAGE_COUNT_MAX > 0)
	//Unregister all the message
	for (i = 0; i < DEVOS_MESSAGE_COUNT_MAX; i++)
	{
		tp_CurrentTask->u8_MessageState[i] = 0;
	}
#endif

	tp_CurrentTask->t_Flag = 0;
	tp_PriorityInfo = &m_t_PriorityInfo[tp_CurrentTask->t_Priority];
	tp_PreviousTask = tp_PriorityInfo->tp_HeadTask;

	//Check if task to be deleted is the head task or not
	if (tp_PreviousTask == tp_CurrentTask)
	{
		tp_PriorityInfo->tp_HeadTask = tp_CurrentTask->tp_PriorityTask;
	}
	else
	{
		//Locate task to be deleted
		while (tp_PreviousTask->tp_PriorityTask != tp_CurrentTask)
		{
			tp_PreviousTask = tp_PreviousTask->tp_PriorityTask;
		}

		//Delete task in the priority list
		tp_PreviousTask->tp_PriorityTask = tp_CurrentTask->tp_PriorityTask;
	}

	//Check if task to be deleted is being scheduled
	if (tp_PriorityInfo->tp_ReadyTask == tp_CurrentTask)
	{
		//Check if there is still any task in this priority
		if (tp_PriorityInfo->tp_HeadTask == (devos_task *)0)
		{
			tp_PriorityInfo->tp_ReadyTask = (devos_task *)0;
		}
		else
		{
			tp_PriorityInfo->tp_ReadyTask = tp_PreviousTask;
		}
	}

	if (tp_CurrentTask->t_Pending == 0)
	{
		tp_PriorityInfo->t_ReadyTaskCount--;
	}

	tp_PriorityInfo->t_TotalTaskCount--;

	//Check if max priority should be updated
	if ((tp_CurrentTask->t_Priority >= m_t_PriorityMax ) && 
		(tp_PriorityInfo->t_TotalTaskCount == 0))
	{
		for (; m_t_PriorityMax > 0; m_t_PriorityMax--)
		{
			if (m_t_PriorityInfo[m_t_PriorityMax].t_TotalTaskCount != 0)
			{
				break;
			}
		}
	}

	//Check if max task ID should be updated
	if (tp_CurrentTask->t_TaskID >= m_t_TaskIDMax) 
	{
		for (; m_t_TaskIDMax > 0; m_t_TaskIDMax--)
		{
			if (REG_GET_BIT(m_t_Task[m_t_TaskIDMax].t_Flag, 
				DEVOS_TASK_FLAG_VALID) != 0)
			{
				break;
			}
		}
	}

	return FUNCTION_OK;
}


#if (DEVOS_MESSAGE_COUNT_MAX > 0)
uint DevOS_MessageInitialize
(
	devos_int t_MessageID,
	uint8 *u8p_Buffer,
	devos_int t_Length
)
{
	//Check if message ID is valid or not
	if (t_MessageID >= DEVOS_MESSAGE_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}

	if (t_Length == 0)
	{
		m_t_Message[t_MessageID].u8p_Buffer = (uint8 *)~0;
	}
	else
	{
		m_t_Message[t_MessageID].u8p_Buffer = u8p_Buffer;
	}

	m_t_Message[t_MessageID].t_LengthMax = t_Length;

	return FUNCTION_OK;
}


uint DevOS_MessageRegister
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
)
{
	devos_int t_TaskID;
	devos_task *tp_Task;


	t_TaskID = (devos_int)t_TaskHandle;

	//Check if message ID and task handle are valid or not
	if ((t_MessageID >= DEVOS_MESSAGE_COUNT_MAX) ||
		(t_TaskID > m_t_TaskIDMax))
	{
		return FUNCTION_FAIL;
	}

	tp_Task = &m_t_Task[t_TaskID];

	if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) == 0)
	{
		return FUNCTION_FAIL;
	}

	m_fp_EnterCritical();
	REG_SET_BIT(tp_Task->u8_MessageState[t_MessageID], 
		DEVOS_MESSAGE_STATE_REGISTERED);
	m_fp_ExitCritical();

	return FUNCTION_OK;
}


uint DevOS_MessageUnregister
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
)
{
	devos_int t_TaskID;
	devos_task *tp_Task;


	t_TaskID = (devos_int)t_TaskHandle;

	//Check if message ID and task handle are valid or not
	if ((t_MessageID >= DEVOS_MESSAGE_COUNT_MAX) ||
		(t_TaskID > m_t_TaskIDMax))
	{
		return FUNCTION_FAIL;
	}

	tp_Task = &m_t_Task[t_TaskID];

	if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) == 0)
	{
		return FUNCTION_FAIL;
	}

	m_fp_EnterCritical();

	if ((tp_Task->t_MessageID == t_MessageID) ||
		(tp_Task->t_MessageID >= DEVOS_MESSAGE_COUNT_MAX))
	{
		DevOS_ClearTaskPending(tp_Task, DEVOS_TASK_PENDING_MESSAGE);
	}

	tp_Task->u8_MessageState[t_MessageID] = 0; 
	m_fp_ExitCritical();

	return FUNCTION_OK;
}


uint DevOS_MessageClear
(
	devos_int t_MessageID,
	devos_task_handle t_TaskHandle
)
{
	devos_int t_TaskID;
	devos_task *tp_Task;


	t_TaskID = (devos_int)t_TaskHandle;

	//Check if message ID and task handle are valid or not
	if ((t_MessageID >= DEVOS_MESSAGE_COUNT_MAX) ||
		(t_TaskID > m_t_TaskIDMax))
	{
		return FUNCTION_FAIL;
	}

	tp_Task = &m_t_Task[t_TaskID];

	if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) == 0)
	{
		return FUNCTION_FAIL;
	}

	m_fp_EnterCritical();
	REG_CLEAR_BIT(tp_Task->u8_MessageState[t_MessageID], 
		DEVOS_MESSAGE_STATE_RECEIVED);
	m_fp_ExitCritical();

	return FUNCTION_OK;
}


uint DevOS_MessageSend
(
	devos_int t_MessageID,
	const uint8 *u8p_Data,
	devos_int t_Length
)
{
	uint8 *u8p_Buffer;
	devos_int i;
	devos_task *tp_Task;
	devos_message *tp_Message;


	tp_Message = &m_t_Message[t_MessageID];

	//Check if message ID and message data size are valid or not
	if ((t_MessageID >= DEVOS_MESSAGE_COUNT_MAX) ||
		(t_Length > tp_Message->t_LengthMax))
	{
		return FUNCTION_FAIL;
	}

	tp_Message->t_Length = t_Length;
	u8p_Buffer = tp_Message->u8p_Buffer;

	while (t_Length > 0)
	{
		*u8p_Buffer = *u8p_Data;
		u8p_Buffer++;
		u8p_Data++;
		t_Length--;
	}

	tp_Task = &m_t_Task[0];

	//Inform all the registered tasks that new message has been received
	for (i = 0; i <= m_t_TaskIDMax; i++)
	{
		if (REG_GET_BIT(tp_Task->u8_MessageState[t_MessageID],
			DEVOS_MESSAGE_STATE_REGISTERED) != 0)
		{
			m_fp_EnterCritical();

			if ((tp_Task->t_MessageID == t_MessageID) ||
				(tp_Task->t_MessageID >= DEVOS_MESSAGE_COUNT_MAX))
			{
				DevOS_ClearTaskPending(tp_Task, DEVOS_TASK_PENDING_MESSAGE);
			}

			REG_SET_BIT(tp_Task->u8_MessageState[t_MessageID], 
				DEVOS_MESSAGE_STATE_RECEIVED);
			m_fp_ExitCritical();
		}

		tp_Task++;
	}
	
	return FUNCTION_OK;
}


uint8* DevOS_MessageReceive
(
	devos_int t_MessageID,
	devos_int *tp_Length
)
{
	devos_task *tp_Task;
	devos_message *tp_Message;


	//Check if message ID is valid or not
	if (t_MessageID >= DEVOS_MESSAGE_COUNT_MAX)
	{
		*tp_Length = 0;
		return (uint8 *)0;
	}
	
	tp_Task = &m_t_Task[m_t_TaskIDCurrent];

	//Check if there is any new message for current task
	if ((REG_GET_BIT(tp_Task->u8_MessageState[t_MessageID], 
		DEVOS_MESSAGE_STATE_REGISTERED) == 0) ||
		(REG_GET_BIT(tp_Task->u8_MessageState[t_MessageID],
		DEVOS_MESSAGE_STATE_RECEIVED) == 0))
	{
		*tp_Length = 0;
		return (uint8 *)0;
	}

	m_fp_EnterCritical();
	REG_CLEAR_BIT(tp_Task->u8_MessageState[t_MessageID], 
		DEVOS_MESSAGE_STATE_RECEIVED);
	m_fp_ExitCritical();

	tp_Message = &m_t_Message[t_MessageID];
	*tp_Length = tp_Message->t_Length;

	return tp_Message->u8p_Buffer;
}
#endif


#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
uint DevOS_SemaphoreInitialize
(
	devos_int t_SemaphoreID,
	devos_int t_Count
)
{
	//Check if semaphore ID is valid or not
	if (t_SemaphoreID >= DEVOS_SEMAPHORE_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}

	m_t_Semaphore[t_SemaphoreID].t_Count = t_Count;
	m_t_Semaphore[t_SemaphoreID].t_CountMax = t_Count;

	return FUNCTION_OK;
}


uint DevOS_SemaphoreObtain
(
	devos_int t_SemaphoreID
)
{
	//Check if semaphore ID are valid or not
	if (t_SemaphoreID >= DEVOS_SEMAPHORE_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}
	
	if (m_t_Semaphore[t_SemaphoreID].t_Count == 0)
	{
		return FUNCTION_FAIL;
	}

	m_t_Semaphore[t_SemaphoreID].t_Count--;

	return FUNCTION_OK;
}


uint DevOS_SemaphorePost
(
	devos_int t_SemaphoreID
)
{
	devos_int i;
	devos_int t_TaskID;
	devos_int t_Priority;
	devos_task *tp_Task;
	devos_semaphore *tp_Semaphore;


	//Check if semaphore ID is valid or not
	if (t_SemaphoreID >= DEVOS_SEMAPHORE_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}

	tp_Semaphore = &m_t_Semaphore[t_SemaphoreID];

	//Check if count of semaphore is valid or not
	if (tp_Semaphore->t_Count >= tp_Semaphore->t_CountMax)
	{
		return FUNCTION_FAIL;
	}

	t_TaskID = 0;
	tp_Task = &m_t_Task[0];
	t_Priority = m_t_PriorityMax;

	//Search for the tasks that are pending for the semaphore
	for (i = 0; i <= m_t_TaskIDMax; i++)
	{
		if (REG_GET_BIT(tp_Task->t_Pending, DEVOS_TASK_PENDING_SEMAPHORE) != 0)
		{
			if ((tp_Task->t_SemaphoreID == t_SemaphoreID) ||
				(tp_Task->t_SemaphoreID >= DEVOS_SEMAPHORE_COUNT_MAX))
			{
				//Check if the task has higher priority
				if (tp_Task->t_Priority < t_Priority)
				{
					t_TaskID = i;
					t_Priority = tp_Task->t_Priority;
				}
			}
		}

		tp_Task++;
	}
	
	tp_Task = &m_t_Task[t_TaskID];
	m_fp_EnterCritical();

	//Check if any pending task is found
	if (REG_GET_BIT(tp_Task->t_Pending, DEVOS_TASK_PENDING_SEMAPHORE) != 0)
	{
		//Release the pending task with highest priority
		DevOS_ClearTaskPending(tp_Task, DEVOS_TASK_PENDING_SEMAPHORE);
	}
	else
	{
		tp_Semaphore->t_Count++;
	}

	m_fp_ExitCritical();

	return FUNCTION_OK;
}
#endif


void DevOS_CoreTaskDelay
(
	uint16 u16_Time
)
{
	devos_task *tp_Task;


	if (u16_Time != 0)
	{
		tp_Task = &m_t_Task[m_t_TaskIDCurrent];

		if (REG_GET_BIT(tp_Task->t_Flag, DEVOS_TASK_FLAG_VALID) != 0)
		{
			m_fp_EnterCritical();
			tp_Task->u16_DelayTime = u16_Time;
			DevOS_SetTaskPending(tp_Task, DEVOS_TASK_PENDING_DELAY);
			m_fp_ExitCritical();
		}
	}
}


#if (DEVOS_MESSAGE_COUNT_MAX > 0)
uint8*  DevOS_CoreMessageWait
(
	devos_int t_MessageID,
	devos_int *tp_Length
)
{
	devos_int i;
	uint8 *u8p_Data;
	devos_task *tp_Task;


	tp_Task = &m_t_Task[m_t_TaskIDCurrent];
	tp_Task->t_MessageID = t_MessageID;
	*tp_Length = 0;
	u8p_Data = (uint8 *)0;

	if (t_MessageID >= DEVOS_MESSAGE_COUNT_MAX)
	{
		m_fp_EnterCritical();

		for (i = 0; i < DEVOS_MESSAGE_COUNT_MAX; i++)
		{
			if (REG_GET_BIT(tp_Task->u8_MessageState[i], 
				DEVOS_MESSAGE_STATE_RECEIVED) != 0)
			{
				break;
			}
		}

		if (i >= DEVOS_MESSAGE_COUNT_MAX)
		{
			DevOS_SetTaskPending(tp_Task, DEVOS_TASK_PENDING_MESSAGE);
		}

		m_fp_ExitCritical();
	}
	else
	{		
		if (REG_GET_BIT(tp_Task->u8_MessageState[t_MessageID], 
			DEVOS_MESSAGE_STATE_REGISTERED) != 0)
		{
			m_fp_EnterCritical();

			//Check if there is any new message for current task
			if (REG_GET_BIT(tp_Task->u8_MessageState[t_MessageID],
				DEVOS_MESSAGE_STATE_RECEIVED) != 0)
			{
				REG_CLEAR_BIT(tp_Task->u8_MessageState[t_MessageID], 
					DEVOS_MESSAGE_STATE_RECEIVED);
				*tp_Length = m_t_Message[t_MessageID].t_Length;
				u8p_Data = m_t_Message[t_MessageID].u8p_Buffer;
			}
			else
			{
				DevOS_SetTaskPending(tp_Task, DEVOS_TASK_PENDING_MESSAGE);
			}

			m_fp_ExitCritical();
		}
	}

	return u8p_Data;
}
#endif


#if (DEVOS_SEMAPHORE_COUNT_MAX > 0)
devos_int DevOS_CoreSemaphorePend
(
	devos_int t_SemaphoreID
)
{
	devos_int t_Count;
	devos_task *tp_Task;


	tp_Task = &m_t_Task[m_t_TaskIDCurrent];

	m_fp_EnterCritical();

	//Check if semaphore ID are valid or not
	if (t_SemaphoreID >= DEVOS_SEMAPHORE_COUNT_MAX)
	{
		t_Count = 0;
	}
	else
	{
		t_Count = m_t_Semaphore[t_SemaphoreID].t_Count;
	}

	if (t_Count > 0)
	{
		m_t_Semaphore[t_SemaphoreID].t_Count--;
	}
	else
	{
		tp_Task->t_SemaphoreID = t_SemaphoreID;
		DevOS_SetTaskPending(tp_Task, DEVOS_TASK_PENDING_SEMAPHORE);
	}

	m_fp_ExitCritical();

	return t_Count;
}
#endif


#if DEVOS_COOPPERATIVE != 0
void DevOS_CoreTaskSetContext
(
	uint16 u16_Context
)
{
	devos_task *tp_Task;


	tp_Task = &m_t_Task[m_t_TaskIDCurrent];

	//Check if it is the end of the task
	if (u16_Context == (uint16)(~0))
	{
		tp_Task->u16_Context = 0;
	}
	else
	{
		tp_Task->u16_Context = u16_Context;
	}
}


uint16 DevOS_CoreTaskGetContext(void)
{
	return m_t_Task[m_t_TaskIDCurrent].u16_Context;
}
#endif


//Private function definition

static devos_task *DevOS_GetReadyTask(void)
{
	devos_int i;
	devos_int t_TotalTaskCount;
	devos_task *tp_ReadyTask;
	devos_priority_info *tp_PriorityInfo;


	tp_PriorityInfo = &m_t_PriorityInfo[0];

	//Search for highest priority that is ready for running
	for (i = 0; i <= m_t_PriorityMax; i++)
	{
		if (tp_PriorityInfo->t_ReadyTaskCount > 0)
		{
			break;
		}

		tp_PriorityInfo++;
	}

	tp_ReadyTask = tp_PriorityInfo->tp_ReadyTask;
	t_TotalTaskCount = tp_PriorityInfo->t_TotalTaskCount;

	//Search for ready task in the same priority
	for (i = 0; i < t_TotalTaskCount; i++)
	{
		if (tp_ReadyTask->t_Pending == 0)
		{
			break;
		}
		else
		{
			//Check if tail of priority list is reached
			if (tp_ReadyTask->tp_PriorityTask == (devos_task *)0)
			{
				tp_ReadyTask = tp_PriorityInfo->tp_HeadTask;
			}
			else
			{
				tp_ReadyTask = tp_ReadyTask->tp_PriorityTask;
			}
		}
	}

	//Check if ready task is found
	if (i < t_TotalTaskCount)
	{
		//Check if tail of priority list is reached and move to next ready task
		if (tp_ReadyTask->tp_PriorityTask == (devos_task *)0)
		{
			tp_PriorityInfo->tp_ReadyTask = tp_PriorityInfo->tp_HeadTask;
		}
		else
		{
			tp_PriorityInfo->tp_ReadyTask = tp_ReadyTask->tp_PriorityTask;
		}

		return tp_ReadyTask;
	}

	return (devos_task *)0;
}


static void DevOS_SetTaskPending
(
	devos_task *tp_Task,
	devos_int t_Pending
)
{
	devos_priority_info *tp_PriorityInfo;

	
	tp_PriorityInfo = &m_t_PriorityInfo[tp_Task->t_Priority];

	if ((tp_Task->t_Pending == 0) && (tp_PriorityInfo->t_ReadyTaskCount > 0))
	{
		tp_PriorityInfo->t_ReadyTaskCount--;	
	}

	REG_SET_BIT(tp_Task->t_Pending, t_Pending);
}


static void DevOS_ClearTaskPending
(
	devos_task *tp_Task,
	devos_int t_Pending
)
{
	devos_priority_info *tp_PriorityInfo;

	
	if (tp_Task->t_Pending != 0)
	{
		REG_CLEAR_BIT(tp_Task->t_Pending, t_Pending);

		tp_PriorityInfo = &m_t_PriorityInfo[tp_Task->t_Priority];

		if (tp_Task->t_Pending == 0)
		{
			tp_PriorityInfo->t_ReadyTaskCount++;	
		}
	}
}
