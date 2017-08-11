/*
 * Module:	Task manager
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "drv.h"
#include "devos.h"
#include "devfs.h"

#include "task.h"
#include "task_device.h"
#include "task_comm.h"
#include "task_glucose.h"
#include "task_setting.h"
#include "task_history.h"
#include "task_shell.h"


//Constant definition

#define TASK_PRIORITY_DEVICE			0
#define TASK_PRIORITY_COMM				2
#define TASK_PRIORITY_SHELL				2
#define TASK_PRIORITY_GLUCOSE			1
#define TASK_PRIORITY_SETTING			1
#define TASK_PRIORITY_HISTORY			1
#define TASK_PRIORITY_IDLE				3


//Type definition

typedef enum
{
	TASK_ID_DEVICE,
	TASK_ID_COMM,
	TASK_ID_SHELL,
	TASK_ID_GLUCOSE,
	TASK_ID_SETTING,
	TASK_ID_HISTORY,
	TASK_ID_IDLE,
	TASK_COUNT_ID
} task_id;

typedef uint (*task_set_config)
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);

typedef uint (*task_get_config)
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);


//Private function declaration

static void Task_InitializeFileSystem(void);
static void Task_Tick
(
	uint16 u16_TickTime
);
static void Task_ProcessIdle
(
	devos_task_handle t_TaskHandle
);


//Private variable definition

static uint m_ui_MessageID = {0};
static devos_task_handle m_t_TaskHandle[TASK_COUNT_ID];
static uint8 m_u8_MessageData[TASK_COUNT_MESSAGE_ID];

static const task_set_config m_fp_SetConfig[TASK_COUNT_PORT] =
{
	TaskSetting_SetConfig,
	TaskComm_SetConfig,
	TaskShell_SetConfig,
	TaskGlucose_SetConfig,
	TaskDevice_SetConfig,
	TaskHistory_SetConfig
};
static const task_get_config m_fp_GetConfig[TASK_COUNT_PORT] =
{
	TaskSetting_GetConfig,
	TaskComm_GetConfig,
	TaskShell_GetConfig,
	TaskGlucose_GetConfig,
	TaskDevice_GetConfig,
	TaskHistory_GetConfig
};
static const devos_task_profile m_t_TaskProfile[TASK_COUNT_ID] =
{
	{
		TASK_PRIORITY_DEVICE,
		TaskDevice_Process
	},
	{
		TASK_PRIORITY_COMM,
		TaskComm_Process
	},
	{
		TASK_PRIORITY_SHELL,
		TaskShell_Process
	},
	{
		TASK_PRIORITY_GLUCOSE,
		TaskGlucose_Process
	},
	{
		TASK_PRIORITY_SETTING,
		TaskSetting_Process
	},
	{
		TASK_PRIORITY_HISTORY,
		TaskHistory_Process
	},
	{
		TASK_PRIORITY_IDLE,
		Task_ProcessIdle
	},
};


//Public function definition

uint Task_Initialize(void)
{
	drv_rtc_callback t_Callback;


	Drv_DisableInterrupt();

	m_ui_MessageID = TASK_COUNT_MESSAGE_ID;

	DevOS_Initialize(Task_EnterCritical, Task_ExitCritical);

	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_DEVICE], 
		&m_t_TaskProfile[TASK_ID_DEVICE]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_SHELL], 
		&m_t_TaskProfile[TASK_ID_SHELL]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_COMM], 
		&m_t_TaskProfile[TASK_ID_COMM]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_GLUCOSE], 
		&m_t_TaskProfile[TASK_ID_GLUCOSE]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_SETTING], 
		&m_t_TaskProfile[TASK_ID_SETTING]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_HISTORY], 
		&m_t_TaskProfile[TASK_ID_HISTORY]);
	DevOS_TaskCreate(&m_t_TaskHandle[TASK_ID_IDLE], 
		&m_t_TaskProfile[TASK_ID_IDLE]);

	DevOS_MessageInitialize(TASK_MESSAGE_ID_SYSTEM, 
		&m_u8_MessageData[TASK_MESSAGE_ID_SYSTEM], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_SYSTEM]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_DEVICE, 
		&m_u8_MessageData[TASK_MESSAGE_ID_DEVICE], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_DEVICE]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_GLUCOSE, 
		&m_u8_MessageData[TASK_MESSAGE_ID_GLUCOSE], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_GLUCOSE]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_SETTING, 
		&m_u8_MessageData[TASK_MESSAGE_ID_SETTING], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_SETTING]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_HISTORY, 
		&m_u8_MessageData[TASK_MESSAGE_ID_HISTORY], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_HISTORY]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_SHELL, 
		&m_u8_MessageData[TASK_MESSAGE_ID_SHELL], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_SHELL]));
	DevOS_MessageInitialize(TASK_MESSAGE_ID_TICK, 
		&m_u8_MessageData[TASK_MESSAGE_ID_TICK], 
		sizeof(m_u8_MessageData[TASK_MESSAGE_ID_TICK]));

	DevOS_MessageRegister(TASK_MESSAGE_ID_DEVICE, 
		m_t_TaskHandle[TASK_ID_DEVICE]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_GLUCOSE, 
		m_t_TaskHandle[TASK_ID_GLUCOSE]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_SETTING, 
		m_t_TaskHandle[TASK_ID_SETTING]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_HISTORY, 
		m_t_TaskHandle[TASK_ID_HISTORY]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_SHELL, 
		m_t_TaskHandle[TASK_ID_SHELL]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_SYSTEM, 
		m_t_TaskHandle[TASK_ID_DEVICE]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_SYSTEM, 
		m_t_TaskHandle[TASK_ID_SETTING]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_SYSTEM, 
		m_t_TaskHandle[TASK_ID_HISTORY]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_TICK, 
		m_t_TaskHandle[TASK_ID_COMM]);
	DevOS_MessageRegister(TASK_MESSAGE_ID_TICK, 
		m_t_TaskHandle[TASK_ID_SHELL]);

	Task_InitializeFileSystem();

	TaskDevice_Initialize(m_t_TaskHandle[TASK_ID_DEVICE]);
	TaskComm_Initialize(m_t_TaskHandle[TASK_ID_COMM]);
	TaskGlucose_Initialize(m_t_TaskHandle[TASK_ID_GLUCOSE]);
	TaskSetting_Initialize(m_t_TaskHandle[TASK_ID_SETTING]);
	TaskHistory_Initialize(m_t_TaskHandle[TASK_ID_HISTORY]);
	TaskShell_Initialize(m_t_TaskHandle[TASK_ID_SHELL]);

	t_Callback.fp_Wakeup = Task_Tick;
	t_Callback.fp_Alarm = (drv_rtc_callback_alarm)0;
	DrvRTC_SetConfig(DRV_RTC_PARAM_CALLBACK, (const uint8 *)&t_Callback,
		0);

	Drv_EnableInterrupt();

	return FUNCTION_OK;
}


uint Task_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
)
{
	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_TASK)
	{
		return FUNCTION_FAIL;
	}

	switch (u8_Event)
	{
		case MESSAGE_EVENT_SEND_DONE:
			break;

		case MESSAGE_EVENT_ACKNOWLEDGE:
			break;

		case MESSAGE_EVENT_TIMEOUT:
			break;

		case MESSAGE_EVENT_RECEIVE_DONE:
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Task_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
)
{
	uint ui_Length;
	uint ui_Acknowledge;
	uint8 u8_Port;


	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_TASK)
	{
		return FUNCTION_FAIL;
	}

	u8_Port = MESSAGE_GET_MINOR_PORT(u8_TargetPort);

	if (u8_Port >= TASK_COUNT_PORT)
	{
		return FUNCTION_FAIL;
	}

	ui_Acknowledge = FUNCTION_FAIL;

	switch (tp_Command->u8_Operation)
	{
		case MESSAGE_OPERATION_SET:

			if (m_fp_SetConfig[u8_Port] != (task_set_config)0)
			{
				ui_Acknowledge = 
					m_fp_SetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, (uint)tp_Command->u8_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8p_Data[0] = (uint8)ui_Acknowledge;
					tp_Command->u8_Operation = MESSAGE_OPERATION_ACKNOWLEDGE;
					tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
				}
			}

			break;

		case MESSAGE_OPERATION_GET:

			if (m_fp_GetConfig[u8_Port] != (task_get_config)0)
			{
				ui_Length = (uint)tp_Command->u8_Length;
				
				ui_Acknowledge = 
					m_fp_GetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, &ui_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8_Operation = MESSAGE_OPERATION_NOTIFY;
					tp_Command->u8_Length = (uint8)ui_Length;
				}
			}

			break;

		case MESSAGE_OPERATION_NOTIFY:
		case MESSAGE_OPERATION_ACKNOWLEDGE:

			if (tp_Command->u8_Operation == MESSAGE_OPERATION_NOTIFY)
			{
				if ((u8_Port == TASK_PORT_COMM) &&
					(tp_Command->u8_Parameter == TASK_COMM_PARAM_RF_STATE) &&
					(m_fp_SetConfig[u8_Port] != (task_set_config)0))
				{
					m_fp_SetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
						tp_Command->u8p_Data, (uint)tp_Command->u8_Length);
				}
			}

			ui_Acknowledge = FUNCTION_OK;
			tp_Command->u8p_Data[0] = (uint8)MESSAGE_EVENT_ACKNOWLEDGE;
			tp_Command->u8_Operation = MESSAGE_OPERATION_EVENT;
			tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
			break;

		default:
			break;
	}
	
	return ui_Acknowledge;
}


void Task_Process(void)
{
	while (1)
	{
		DevOS_Schedule();
	}
}


uint Task_ObtainMessageID
(
	uint *uip_MessageID
)
{
	if (m_ui_MessageID >= DEVOS_MESSAGE_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}

	*uip_MessageID = m_ui_MessageID;
	m_ui_MessageID++;

	return FUNCTION_OK;
}


void Task_EnterCritical(void)
{
	Drv_DisableInterrupt();
}


void Task_ExitCritical(void)
{
	Drv_EnableInterrupt();
}


//Private function definition

static void Task_InitializeFileSystem(void)
{
	devfs_volume_param t_VolumeParameter;
	devfs_callback t_Callback;


	t_VolumeParameter.t_Address = DRV_FLASH_ADDRESS_PROGRAM_USER;
	t_VolumeParameter.t_ErasedData = DRV_FLASH_ERASED_DATA;
	t_VolumeParameter.t_BlockSize = DEVFS_BLOCK_SIZE_MAX;
	t_VolumeParameter.t_BlockCount = ((DRV_FLASH_ADDRESS_PROGRAM_END - 
		DRV_FLASH_ADDRESS_PROGRAM_USER + 1) / DEVFS_BLOCK_SIZE_MAX);
	t_Callback.fp_Memcpy = Drv_Memcpy;
	t_Callback.fp_Write = DrvFLASH_Write;
	t_Callback.fp_Read = DrvFLASH_Read;
	t_Callback.fp_Erase = DrvFLASH_Erase;

	if (DevFS_Mount(TASK_FILE_SYSTEM_VOLUME_ID, &t_VolumeParameter, 
		&t_Callback) != FUNCTION_OK)
	{
		DevFS_Format(TASK_FILE_SYSTEM_VOLUME_ID, &t_VolumeParameter, 
			&t_Callback);
		DevFS_Mount(TASK_FILE_SYSTEM_VOLUME_ID, &t_VolumeParameter, 
			&t_Callback);
	}
}


static void Task_Tick
(
	uint16 u16_TickTime
)
{
	uint8 u8_TickTime;


	DevOS_Tick(u16_TickTime);
	u8_TickTime = (uint8)u16_TickTime;
	DevOS_MessageSend(TASK_MESSAGE_ID_TICK, &u8_TickTime, sizeof(u8_TickTime));
}


static void Task_ProcessIdle
(
	devos_task_handle t_TaskHandle
)
{
	//Drv_RefreshWatchdog();
	Drv_Sleep();
}
