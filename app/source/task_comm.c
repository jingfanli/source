/*
 * Module:	Communication manager task
 * Author:	Lvjianfeng
 * Date:	2011.9
 */


#include "drv.h"
#include "mid.h"
#include "devcomm.h"
#include "lib_queue.h"
#include "lib_checksum.h"

#include "task.h"
#include "task_comm.h"


//Constant definition

#define TASK_COMM_BUFFER_SIZE_COMMAND		128
#define TASK_COMM_BUFFER_SIZE_MESSAGE		128
#define TASK_COMM_BUFFER_SIZE_DEVICE_EVENT	64
#define TASK_COMM_PACKET_LENGTH_MAX			32
#define TASK_COMM_PACKET_LENGTH_MASTER		16
#define TASK_COMM_PACKET_LENGTH_SLAVE		32
#define TASK_COMM_WRITE_DELAY_MASTER		20
#define TASK_COMM_WRITE_DELAY_SLAVE			20
#define TASK_COMM_SEND_RETRY				3
#define TASK_COMM_SEND_TIMEOUT				400
#define TASK_COMM_TARGET_ADDRESS_OFFSET		1


//Type definition

typedef enum
{
	TASK_COMM_DEVICE_SERIAL = 0,
	TASK_COMM_COUNT_DEVICE
} task_comm_device;

typedef enum
{
	TASK_COMM_FLAG_SWITCH = 0,
	TASK_COMM_COUNT_FLAG
} task_comm_flag;

typedef enum
{
	TASK_COMM_RF_STATE_IDLE,
	TASK_COMM_RF_STATE_BROADCAST,
	TASK_COMM_RF_STATE_CONNECTED,
	TASK_COMM_COUNT_RF_STATE
} task_comm_rf_state;

typedef struct
{
	uint8 u8_Address;
	uint8 u8_SourcePort;
	uint8 u8_TargetPort;
	uint8 u8_Mode;
	uint8 u8_Length;
} task_comm_message;


//Private variable definition

static uint m_ui_Flag[MESSAGE_COUNT_ADDRESS] = {0};
static uint m_ui_MessageID = {0};
static uint m_ui_ReadLength = {0};
static uint8 m_u8_RFState = {0};
static uint8 m_u8_BufferCommand[TASK_COMM_BUFFER_SIZE_COMMAND] = {0};
static uint8 m_u8_BufferMessage[MESSAGE_COUNT_ADDRESS][TASK_COMM_BUFFER_SIZE_MESSAGE] = {0};
static uint8 m_u8_BufferDeviceEvent[TASK_COMM_BUFFER_SIZE_DEVICE_EVENT];
static lib_queue_object m_t_QueueMessage[MESSAGE_COUNT_ADDRESS] = {0};
static lib_queue_object m_t_QueueDeviceEvent;
static const uint8 *m_u8p_ReadData = {0};

static const uint m_ui_ParameterSize[TASK_COMM_COUNT_PARAM] =
{
	sizeof(uint8),
	sizeof(uint8),
	sizeof(uint8),
	sizeof(uint)
};


//Private function declaration

static uint TaskComm_InitializeSerial(void);
static uint TaskComm_InitializeComm(void);
static uint TaskComm_InitializeOS
(
	devos_task_handle t_TaskHandle
);
static uint TaskComm_GetDevice
(
	devcomm_int t_Address,
	devcomm_int *tp_Device
);
static uint TaskComm_CheckState
(
	uint *uip_State
);
static void TaskComm_Memcpy
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	devcomm_int t_Length
);
static uint8 TaskComm_GetCRC8
(
	const uint8 *u8p_Data,
	devcomm_int t_Length,
	uint8 u8_Base
);
static uint16 TaskComm_GetCRC16
(
	const uint8 *u8p_Data,
	devcomm_int t_Length,
	uint16 u16_Base
);
static void TaskComm_Tick
(
	uint16 u16_TickTime
);
static uint TaskComm_SendCommand
(
	uint8 u8_Address,
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	const message_command *tp_Command,
	uint8 u8_Mode
);
static uint TaskComm_CheckCommandPending
(
	devcomm_int t_Address
);
static void TaskComm_SendMessage
(
	uint ui_MessageID
);
static void TaskComm_HandleMessage(void);
static uint TaskComm_HandleEvent
(
	devcomm_int t_Device,
	devcomm_int t_Address,
	devcomm_int t_SourcePort,
	devcomm_int t_TargetPort,
	devcomm_int t_Event
);
static uint TaskComm_WriteDevice
(
	devcomm_int t_Device,
	const uint8 *u8p_Data,
	devcomm_int t_Length
);
static uint TaskComm_ReadDevice
(
	devcomm_int t_Device,
	uint8 *u8p_Data,
	devcomm_int *tp_Length
);
static uint TaskComm_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
);


//Public function definition

uint TaskComm_Initialize
(
	devos_task_handle t_TaskHandle
)
{
	if (TaskComm_InitializeOS(t_TaskHandle) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (TaskComm_InitializeComm() != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (TaskComm_InitializeSerial() != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void TaskComm_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;


	DEVOS_TASK_BEGIN
	
	DevOS_MessageWait(DEVOS_MESSAGE_COUNT_MAX, u8p_MessageData,
		&t_MessageLength);

	TaskComm_HandleMessage();

	DEVOS_TASK_END
}


uint TaskComm_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	if (ui_Length != m_ui_ParameterSize[ui_Parameter])
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Parameter)
	{
		case TASK_COMM_PARAM_RF_STATE:

			if (*u8p_Value < TASK_COMM_COUNT_RF_STATE)
			{
				m_u8_RFState = *u8p_Value;
			}
			else
			{
				return FUNCTION_FAIL;
			}

			break;

		case TASK_COMM_PARAM_ENABLE:

			if (*u8p_Value < MESSAGE_COUNT_ADDRESS)
			{
				REG_SET_BIT(m_ui_Flag[*u8p_Value], TASK_COMM_FLAG_SWITCH);
			}
			else
			{
				return FUNCTION_FAIL;
			}

			break;

		case TASK_COMM_PARAM_DISABLE:

			if (*u8p_Value < MESSAGE_COUNT_ADDRESS)
			{
				REG_CLEAR_BIT(m_ui_Flag[*u8p_Value], TASK_COMM_FLAG_SWITCH);
			}
			else
			{
				return FUNCTION_FAIL;
			}

			break;

		default:
			return FUNCTION_FAIL;
	}
	
	return FUNCTION_OK;
}


uint TaskComm_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_COMM_PARAM_RF_STATE:
			*((uint8 *)u8p_Value) = m_u8_RFState;
			break;

		case TASK_COMM_PARAM_BUSY:

			if (TaskComm_CheckState((uint *)u8p_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}
			
			break;

		default:
			return FUNCTION_FAIL;
	}

	if (uip_Length != (uint *)0)
	{
		*uip_Length = m_ui_ParameterSize[ui_Parameter];
	}

	return FUNCTION_OK;
}


uint TaskComm_Send
(
	uint8 u8_Address,
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	const message_command *tp_Command,
	uint8 u8_Mode
)
{
	uint ui_Value;
	devcomm_int t_State;
	task_comm_message t_Message;


	if (u8_Address >= MESSAGE_COUNT_ADDRESS)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Query(TASK_COMM_DEVICE_SERIAL, (devcomm_int)u8_Address, 
		DEVCOMM_INFO_STATE, &t_State) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (t_State == DEVCOMM_STATE_BUSY)
	{
		if (LibQueue_GetConfig(&m_t_QueueMessage[u8_Address], 
			LIB_QUEUE_PARAM_BUFFER_SPACE, (void *)&ui_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		t_Message.u8_Address = u8_Address;
		t_Message.u8_SourcePort = u8_SourcePort;
		t_Message.u8_TargetPort = u8_TargetPort;
		t_Message.u8_Mode = u8_Mode;

		if (tp_Command == (const message_command *)0)
		{
			if (ui_Value < sizeof(t_Message))
			{
				return FUNCTION_FAIL;
			}

			t_Message.u8_Length = 0;
			ui_Value = sizeof(t_Message);

			if (LibQueue_PushTail(&m_t_QueueMessage[u8_Address], 
				(const uint8 *)&t_Message, &ui_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}
		}
		else
		{
			if (ui_Value < sizeof(t_Message) + MESSAGE_COUNT_COMMAND_OFFSET + 
				tp_Command->u8_Length)
			{
				return FUNCTION_FAIL;
			}

			t_Message.u8_Length = MESSAGE_COUNT_COMMAND_OFFSET + 
				tp_Command->u8_Length;
			ui_Value = sizeof(t_Message);

			if (LibQueue_PushTail(&m_t_QueueMessage[u8_Address], 
				(const uint8 *)&t_Message, &ui_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}

			ui_Value = MESSAGE_COUNT_COMMAND_OFFSET;

			if (LibQueue_PushTail(&m_t_QueueMessage[u8_Address], 
				(const uint8 *)tp_Command, &ui_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}

			ui_Value = (uint)tp_Command->u8_Length;

			if (LibQueue_PushTail(&m_t_QueueMessage[u8_Address], 
				tp_Command->u8p_Data, &ui_Value) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}
		}
	}
	else
	{
		return TaskComm_SendCommand(u8_Address, u8_SourcePort, u8_TargetPort,
			tp_Command, u8_Mode);
	}

	return FUNCTION_OK;
}


uint TaskComm_Receive
(
	uint8 u8_Address,
	uint8 *u8p_SourcePort,
	uint8 *u8p_TargetPort,
	message_command *tp_Command,
	uint8 *u8p_Mode
)
{
	devcomm_int t_Device;
	devcomm_int t_Length;
	devcomm_int t_SourcePort;
	devcomm_int t_TargetPort;
	devcomm_int t_Mode;


	if (TaskComm_GetDevice((devcomm_int)u8_Address, &t_Device) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag[u8_Address], TASK_COMM_FLAG_SWITCH) == 0)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Receive(t_Device, (devcomm_int)u8_Address, &t_SourcePort, 
		&t_TargetPort, m_u8_BufferCommand, &t_Length, &t_Mode) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (t_Length < MESSAGE_COUNT_COMMAND_OFFSET)
	{
		return FUNCTION_FAIL;
	}

	*u8p_SourcePort = (uint8)t_SourcePort;
	*u8p_TargetPort = (uint8)t_TargetPort;
	*u8p_Mode = (uint8)t_Mode;
	tp_Command->u8_Operation = 
		m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_OPERATION];
	tp_Command->u8_Parameter = 
		m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_PARAMETER];
	tp_Command->u8_Length = (uint8)t_Length - MESSAGE_COUNT_COMMAND_OFFSET;
	tp_Command->u8p_Data = &m_u8_BufferCommand[MESSAGE_COUNT_COMMAND_OFFSET];

	return FUNCTION_OK;
}


#if TASK_COMM_TEST_ENABLE == 1

void TaskComm_Test(void)
{
}

#endif


//Private function definition

static uint TaskComm_InitializeSerial(void)
{
	uint ui_Value;
	drv_uart_callback t_Callback;


	if (LibQueue_Initialize(&m_t_QueueDeviceEvent, m_u8_BufferDeviceEvent, 
		sizeof(m_u8_BufferDeviceEvent)) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	t_Callback.fp_HandleEvent = TaskComm_HandleDeviceEvent;

	if (DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_CALLBACK, 
		(const uint8 *)&t_Callback, sizeof(t_Callback)) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	ui_Value = 1;

	return DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_FRAME,
		(const uint8 *)&ui_Value, sizeof(ui_Value));
}


static uint TaskComm_InitializeComm(void)
{
	uint ui_Address;
	devcomm_profile t_Profile;
	devcomm_callback t_Callback;


	t_Profile.t_Address = MESSAGE_ADDRESS_MASTER;
	t_Profile.t_PacketLengthMax = TASK_COMM_PACKET_LENGTH_MAX;
	t_Profile.u16_Retry = TASK_COMM_SEND_RETRY;
	t_Profile.u16_Timeout = TASK_COMM_SEND_TIMEOUT;

	t_Callback.fp_HandleEvent = TaskComm_HandleEvent;
	t_Callback.fp_WriteDevice = TaskComm_WriteDevice;
	t_Callback.fp_ReadDevice = TaskComm_ReadDevice;
	t_Callback.fp_Memcpy = TaskComm_Memcpy;
	t_Callback.fp_GetCRC8 = TaskComm_GetCRC8;
	t_Callback.fp_GetCRC16 = TaskComm_GetCRC16;
	t_Callback.fp_EnterCritical = (devcomm_callback_enter_critical)0;
	t_Callback.fp_ExitCritical = (devcomm_callback_exit_critical)0;

	if (DevComm_Initialize(TASK_COMM_DEVICE_SERIAL, &t_Profile, &t_Callback) !=
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Link(TASK_COMM_DEVICE_SERIAL, MESSAGE_ADDRESS_SLAVE, 
		TASK_COMM_PACKET_LENGTH_SLAVE) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Link(TASK_COMM_DEVICE_SERIAL, MESSAGE_ADDRESS_MASTER, 
		TASK_COMM_PACKET_LENGTH_MASTER) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	REG_SET_BIT(m_ui_Flag[MESSAGE_ADDRESS_SLAVE], TASK_COMM_FLAG_SWITCH);
	REG_SET_BIT(m_ui_Flag[MESSAGE_ADDRESS_MASTER], TASK_COMM_FLAG_SWITCH);

	for (ui_Address = 0; ui_Address < MESSAGE_COUNT_ADDRESS; ui_Address++)
	{
		LibQueue_Initialize(&m_t_QueueMessage[ui_Address], 
			m_u8_BufferMessage[ui_Address], 
			sizeof(m_u8_BufferMessage[ui_Address]));
	}

	return FUNCTION_OK;
}


static uint TaskComm_InitializeOS
(
	devos_task_handle t_TaskHandle
)
{
	if (Task_ObtainMessageID(&m_ui_MessageID) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevOS_MessageInitialize((devos_int)m_ui_MessageID, (uint8 *)0, 0) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevOS_MessageRegister((devos_int)m_ui_MessageID, t_TaskHandle) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static uint TaskComm_GetDevice
(
	devcomm_int t_Address,
	devcomm_int *tp_Device
)
{
	switch (t_Address)
	{
		case MESSAGE_ADDRESS_MASTER:
		case MESSAGE_ADDRESS_SLAVE:
			*tp_Device = TASK_COMM_DEVICE_SERIAL;
			break;

		default:
			return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag[t_Address], TASK_COMM_FLAG_SWITCH) == 0)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static uint TaskComm_CheckState
(
	uint *uip_State
)
{
	uint ui_Event;
	uint ui_Length;
	devcomm_int t_State;


	*uip_State = 0;

	if (DevComm_Query(TASK_COMM_DEVICE_SERIAL, MESSAGE_ADDRESS_MASTER, 
		DEVCOMM_INFO_STATE, &t_State) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (t_State == DEVCOMM_STATE_BUSY)			
	{
		*uip_State = 1;
	}

	if (DevComm_Query(TASK_COMM_DEVICE_SERIAL, MESSAGE_ADDRESS_SLAVE, 
		DEVCOMM_INFO_STATE, &t_State) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}	

	if (t_State == DEVCOMM_STATE_BUSY) 
	{
		*uip_State = 1;
	}

	ui_Length = sizeof(ui_Event);

	if (LibQueue_PeekHead(&m_t_QueueDeviceEvent, (uint8 *)&ui_Event, 
		&ui_Length) == FUNCTION_OK)
	{
		*uip_State = 1;
	}

	if (*uip_State == 0)
	{
		if (DrvUART_GetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_BUSY, 
			(uint8 *)uip_State, (uint *)0) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	return FUNCTION_OK;
}


static void TaskComm_Memcpy
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	devcomm_int t_Length
)
{
	Drv_Memcpy(u8p_Target, u8p_Source, (uint)t_Length);
}


static uint8 TaskComm_GetCRC8
(
	const uint8 *u8p_Data,
	devcomm_int t_Length,
	uint8 u8_Base
)
{
	return LibChecksum_GetChecksumPartial8Bit(u8p_Data, (uint16)t_Length, 
		u8_Base);
}


static uint16 TaskComm_GetCRC16
(
	const uint8 *u8p_Data,
	devcomm_int t_Length,
	uint16 u16_Base
)
{
	u16_Base = ((u16_Base << 8) & 0xFF00) | ((u16_Base >> 8) & 0x00FF);
	u16_Base = LibChecksum_GetChecksumPartial16Bit(u8p_Data, (uint16)t_Length, 
		u16_Base);

	return ((u16_Base << 8) & 0xFF00) | ((u16_Base >> 8) & 0x00FF);
}


static void TaskComm_Tick
(
	uint16 u16_TickTime
)
{
	DevComm_Tick(TASK_COMM_DEVICE_SERIAL, u16_TickTime);
	DrvUART_Tick(DRV_UART_DEVICE_ID_2, u16_TickTime);
}


static uint TaskComm_SendCommand
(
	uint8 u8_Address,
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	const message_command *tp_Command,
	uint8 u8_Mode
)
{
	devcomm_int t_Device;


	if (TaskComm_GetDevice((devcomm_int)u8_Address, &t_Device) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag[u8_Address], TASK_COMM_FLAG_SWITCH) == 0)
	{
		return FUNCTION_FAIL;
	}

	if (tp_Command != (const message_command *)0)
	{
		m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_OPERATION] = 
			tp_Command->u8_Operation;
		m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_PARAMETER] = 
			tp_Command->u8_Parameter;
		Drv_Memcpy(m_u8_BufferCommand + MESSAGE_COUNT_COMMAND_OFFSET, 
			tp_Command->u8p_Data, (uint)tp_Command->u8_Length);	

		if (DevComm_Send(t_Device, (devcomm_int)u8_Address, 
			(devcomm_int)u8_SourcePort, (devcomm_int)u8_TargetPort,
			m_u8_BufferCommand, (devcomm_int)(tp_Command->u8_Length + 
			MESSAGE_COUNT_COMMAND_OFFSET), (devcomm_int)u8_Mode) != 
			FUNCTION_OK)
		{
			return FUNCTION_FAIL; 
		}
	}
	else
	{
		if (DevComm_Send(t_Device, (devcomm_int)u8_Address, 
			(devcomm_int)u8_SourcePort, (devcomm_int)u8_TargetPort,
			(const uint8 *)0, 0, (devcomm_int)u8_Mode) != FUNCTION_OK)
		{
			return FUNCTION_FAIL; 
		}
	}

	return FUNCTION_OK;
}


static uint TaskComm_CheckCommandPending
(
	devcomm_int t_Address
)
{
	uint ui_Value;
	devcomm_int t_State;
	devcomm_int t_Device;
	task_comm_message t_Message;


	if (TaskComm_GetDevice(t_Address, &t_Device) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Query(t_Device, t_Address, DEVCOMM_INFO_STATE, &t_State) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (t_State == DEVCOMM_STATE_BUSY)
	{
		return FUNCTION_FAIL;
	}

	ui_Value = sizeof(t_Message);

	if (LibQueue_PopHead(&m_t_QueueMessage[t_Address], (uint8 *)&t_Message, 
		&ui_Value) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (ui_Value < sizeof(t_Message))
	{
		return FUNCTION_FAIL;
	}

	ui_Value = (uint)t_Message.u8_Length;

	if (LibQueue_PopHead(&m_t_QueueMessage[t_Address], m_u8_BufferCommand, 
		&ui_Value) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag[t_Address], TASK_COMM_FLAG_SWITCH) == 0)
	{
		return FUNCTION_FAIL;
	}

	if (DevComm_Send(t_Device, t_Address, (devcomm_int)t_Message.u8_SourcePort, 
		(devcomm_int)t_Message.u8_TargetPort, m_u8_BufferCommand, 
		(devcomm_int)t_Message.u8_Length, (devcomm_int)t_Message.u8_Mode) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL; 
	}

	return FUNCTION_OK;
}


static void TaskComm_SendMessage
(
	uint ui_MessageID
)
{
	DevOS_MessageSend((devos_int)ui_MessageID, (const uint8 *)0, 0);
}


static void TaskComm_HandleMessage(void)
{
	uint ui_Event;
	uint ui_Length;
	uint8 *u8p_MessageData;
	devos_int t_MessageLength;


	u8p_MessageData = DevOS_MessageReceive((devos_int)m_ui_MessageID, 
		&t_MessageLength);

	if (u8p_MessageData != (const uint8 *)0)
	{
		ui_Length = sizeof(ui_Event);

		Task_EnterCritical();

		while (LibQueue_PopHead(&m_t_QueueDeviceEvent, (uint8 *)&ui_Event, 
			&ui_Length) == FUNCTION_OK)
		{
			Task_ExitCritical();

			DrvUART_HandleEvent(DRV_UART_DEVICE_ID_2, ui_Event, m_u8p_ReadData, 
				m_ui_ReadLength);

			if ((REG_GET_BIT(m_ui_Flag[MESSAGE_ADDRESS_MASTER], 
				TASK_COMM_FLAG_SWITCH) != 0) ||
				(REG_GET_BIT(m_ui_Flag[MESSAGE_ADDRESS_SLAVE], 
				TASK_COMM_FLAG_SWITCH) != 0))
			{
				switch (ui_Event)
				{
					case DRV_UART_EVENT_WRITE_DONE:
						DevComm_WriteDeviceDone(TASK_COMM_DEVICE_SERIAL);
						break;

					case DRV_UART_EVENT_READ_DONE:
						DevComm_ReadDeviceDone(TASK_COMM_DEVICE_SERIAL, 
							m_u8p_ReadData, (devcomm_int)m_ui_ReadLength);
						break;

					default:
						break;
				}
			}

			Task_EnterCritical();
		}

		Task_ExitCritical();

		TaskComm_CheckCommandPending(MESSAGE_ADDRESS_SLAVE);
		TaskComm_CheckCommandPending(MESSAGE_ADDRESS_MASTER);
	}

	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_TICK, 
		&t_MessageLength);

	if (u8p_MessageData != (const uint8 *)0)
	{
		TaskComm_Tick((uint16)*u8p_MessageData);
	}
}


static uint TaskComm_HandleEvent
(
	devcomm_int t_Device,
	devcomm_int t_Address,
	devcomm_int t_SourcePort,
	devcomm_int t_TargetPort,
	devcomm_int t_Event
)
{
	uint ui_Value;
	uint ui_Acknowledge;
	uint8 u8_Port;
	devcomm_int t_Length;
	devcomm_int t_Mode;
	message_command t_Command;


	if (TaskComm_GetDevice(t_Address, &t_Device) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag[t_Address], TASK_COMM_FLAG_SWITCH) == 0)
	{
		return FUNCTION_FAIL;
	}

	u8_Port = MESSAGE_GET_MAJOR_PORT((uint8)t_TargetPort);

	switch (u8_Port)
	{
		case MESSAGE_PORT_TASK:
			Task_HandleEvent((uint8)t_SourcePort, (uint8)t_TargetPort, 
				(uint8)t_Event);
			break;

		case MESSAGE_PORT_MIDWARE:
			Mid_HandleEvent((uint8)t_SourcePort, (uint8)t_TargetPort, 
				(uint8)t_Event);
			break;

		case MESSAGE_PORT_DRIVER:
			Drv_HandleEvent((uint8)t_SourcePort, (uint8)t_TargetPort, 
				(uint8)t_Event);
			break;

		default:
			return FUNCTION_FAIL;
	}

	if (t_Event == MESSAGE_EVENT_RECEIVE_DONE)
	{
		t_Length = sizeof(m_u8_BufferCommand);

		if (DevComm_Receive(t_Device, t_Address, &t_SourcePort, &t_TargetPort,
			m_u8_BufferCommand, &t_Length, &t_Mode) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}

		if (t_Length < MESSAGE_COUNT_COMMAND_OFFSET)
		{
			return FUNCTION_FAIL;
		}

		t_Command.u8_Operation = 
			m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_OPERATION];
		t_Command.u8_Parameter = 
			m_u8_BufferCommand[MESSAGE_COMMAND_OFFSET_PARAMETER];
		t_Command.u8_Length = (uint8)t_Length - MESSAGE_COUNT_COMMAND_OFFSET;
		t_Command.u8p_Data = m_u8_BufferCommand + MESSAGE_COUNT_COMMAND_OFFSET;
		ui_Acknowledge = FUNCTION_FAIL;

		switch (u8_Port)
		{
			case MESSAGE_PORT_TASK:
				ui_Acknowledge = Task_HandleCommand((uint8)t_SourcePort, 
					(uint8)t_TargetPort, &t_Command);
				break;

			case MESSAGE_PORT_MIDWARE:
				ui_Acknowledge = Mid_HandleCommand((uint8)t_SourcePort, 
					(uint8)t_TargetPort, &t_Command);
				break;

			case MESSAGE_PORT_DRIVER:
				ui_Acknowledge = Drv_HandleCommand((uint8)t_SourcePort, 
					(uint8)t_TargetPort, &t_Command);
				break;

			default:
				break;
		}

		if (ui_Acknowledge == FUNCTION_OK)
		{
			if ((t_Command.u8_Operation == MESSAGE_OPERATION_NOTIFY) || 
				(t_Command.u8_Operation == MESSAGE_OPERATION_ACKNOWLEDGE))
			{
				return TaskComm_Send((uint8)t_Address, (uint8)t_TargetPort, 
					(uint8)t_SourcePort, &t_Command, 
					MESSAGE_MODE_ACKNOWLEDGEMENT);
			}
			else
			{
				if (LibQueue_GetConfig(&m_t_QueueMessage[t_Address], 
					LIB_QUEUE_PARAM_BUFFER_SPACE, (void *)&ui_Value) == 
					FUNCTION_OK)
				{
					if ((ui_Value >= sizeof(m_u8_BufferMessage[t_Address])) &&
						(t_Command.u8_Operation == MESSAGE_OPERATION_EVENT) &&
						(t_Mode == MESSAGE_MODE_ACKNOWLEDGEMENT))
					{
						return TaskComm_Send((uint8)t_Address, 
							(uint8)t_TargetPort, (uint8)t_SourcePort, 
							&t_Command, MESSAGE_MODE_NO_ACKNOWLEDGEMENT);
					}
				}
			}
		}

		return ui_Acknowledge;
	}
	else
	{
		switch (t_Event)
		{
			case MESSAGE_EVENT_SEND_DONE:
				break;

			case MESSAGE_EVENT_ACKNOWLEDGE:
				break;

			case MESSAGE_EVENT_TIMEOUT:
				TaskComm_SendMessage(m_ui_MessageID);
				break;

			default:
				return FUNCTION_FAIL;
		}

		return FUNCTION_OK;
	}
}


static uint TaskComm_WriteDevice
(
	devcomm_int t_Device,
	const uint8 *u8p_Data,
	devcomm_int t_Length
)
{
	uint16 u16_Value;


	switch (*(u8p_Data + TASK_COMM_TARGET_ADDRESS_OFFSET))
	{
		case MESSAGE_ADDRESS_MASTER:
			u16_Value = TASK_COMM_WRITE_DELAY_MASTER;
			break;

		case MESSAGE_ADDRESS_SLAVE:
			u16_Value = TASK_COMM_WRITE_DELAY_SLAVE;
			break;

		default:
			u16_Value = 0;
			break;
	}

	DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_WRITE_DELAY,
		(const uint8 *)&u16_Value, sizeof(u16_Value));

	if (DrvUART_Write(DRV_UART_DEVICE_ID_2, u8p_Data, (uint)t_Length) != 
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static uint TaskComm_ReadDevice
(
	devcomm_int t_Device,
	uint8 *u8p_Data,
	devcomm_int *tp_Length
)
{
	uint ui_Length;
	uint ui_Return;

	ui_Length = (uint)(*tp_Length);
	ui_Return = DrvUART_Read(DRV_UART_DEVICE_ID_2, u8p_Data, &ui_Length);
	*tp_Length = (devcomm_int)ui_Length;

	return ui_Return;
}


static uint TaskComm_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	uint ui_Value;


	ui_Value = sizeof(ui_Event);

	Task_EnterCritical();
	ui_Value = LibQueue_PushTail(&m_t_QueueDeviceEvent, (const uint8 *)&ui_Event, 
		&ui_Value);
	Task_ExitCritical();
   
	if (ui_Value == FUNCTION_OK)
	{
		if (ui_Event == DRV_UART_EVENT_READ_DONE)
		{
			m_u8p_ReadData = u8p_Data;
			m_ui_ReadLength = ui_Length;
		}
	}

	TaskComm_SendMessage(m_ui_MessageID);

	return ui_Value;
}
