/*
 * Module:	Shell task
 * Author:	Lvjianfeng
 * Date:	2012.1
 */


#include "drv.h"

#include "lib_string.h"
#include "glucose.h"

#include "task.h"
#include "task_device.h"
#include "task_glucose.h"
#include "task_shell.h"


//Constant definition

#define OUTPUT_BUFFER_SIZE				6
#define YEAR_BASE						2000
#define ADC_DELAY						100

#define HISTORY_OUTPUT_TOKEN			'h'
#define INFO_OUTPUT_TOKEN				'i'
#define STRIP_OUTPUT_TOKEN				's'
#define TEST_OUTPUT_TOKEN				't'
#define TEST_RE_TOKEN                   'r'


//Type definition


//Private variable definition

static uint m_ui_Command = {0};
static uint8 m_u8_OutputBuffer[OUTPUT_BUFFER_SIZE] = {0};
static uint16 m_u16_OutputIndex = {0};
static uint16 m_u16_DelayTimer = {0};
static uint8  m_u8_reout[25]={0};


//Private function declaration

static void TaskShell_HexToAscii
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint ui_Length
);
static void TaskShell_OutputValue
(
	uint8 *u8p_Value,
	uint8 u8_Delimiter,
	uint ui_Length
);
static void TaskShell_OutputHistory
(
	uint16 u16_Index
);
static void TaskShell_OutputInfo(void);
static void TaskShell_OutputStrip
(
	uint16 u16_Index
);
static void TaskShell_OutputTest
(
	uint16 u16_Index
);
static void TaskShell_Tick
(
	uint16 u16_TickTime
);
static void TaskShell_HandleCommand
(
	uint ui_Command
);
static uint TaskShell_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
);


//Public function definition

uint TaskShell_Initialize
(
	devos_task_handle t_TaskHandle
)
{
	drv_uart_callback t_Callback;


	t_Callback.fp_HandleEvent = TaskShell_HandleDeviceEvent;
	DrvUART_SetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_CALLBACK, 
		(const uint8 *)&t_Callback, 0);

	return FUNCTION_OK;
}


void TaskShell_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;


	DEVOS_TASK_BEGIN

	DevOS_MessageWait(DEVOS_MESSAGE_COUNT_MAX, u8p_MessageData,
		&t_MessageLength);

	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_SHELL, 
		&t_MessageLength);

	if (u8p_MessageData != (const uint8 *)0)
	{
		DrvUART_HandleEvent(DRV_UART_DEVICE_ID_1, 
			DRV_UART_EVENT_INTERRUPT_WRITE_DONE, (const uint8 *)0, 0);
		DrvUART_Tick(DRV_UART_DEVICE_ID_1, ~0);
	}

	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_TICK, 
		&t_MessageLength);

	if (u8p_MessageData != (const uint8 *)0)
	{
		TaskShell_Tick((uint16)*u8p_MessageData);
	}

	DEVOS_TASK_END
}


uint TaskShell_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	uint ui_Value;


	switch (ui_Parameter)
	{
		case TASK_SHELL_PARAM_COMMAND:
			DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, 
				DRV_UART_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value, (uint *)0);

			if (ui_Value == 0)
			{
				TaskShell_HandleCommand(*((const uint *)u8p_Value));
			}

			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint TaskShell_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{

	return FUNCTION_OK;
}


//Private function definition

static void TaskShell_HexToAscii
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint ui_Length
)
{
	while (ui_Length > 0)
	{
		ui_Length--;

		if (u8p_Source[ui_Length] <= 9)
		{
			u8p_Target[ui_Length] = u8p_Source[ui_Length] + '0'; 
		}
		else if ((u8p_Source[ui_Length] >= 10) && (u8p_Source[ui_Length] <= 
			'Z' - 'A' + 10))
		{
			u8p_Target[ui_Length] = u8p_Source[ui_Length] - 10 + 'A';
		}
	}
}


static void TaskShell_OutputValue
(
	uint8 *u8p_Value,
	uint8 u8_Delimiter,
	uint ui_Length
)
{
	DrvUART_Write(DRV_UART_DEVICE_ID_1, u8p_Value, ui_Length);
	DrvUART_Write(DRV_UART_DEVICE_ID_1, &u8_Delimiter, sizeof(u8_Delimiter));
}


static void TaskShell_OutputHistory
(
	uint16 u16_Index
)
{
	uint ui_Length;
	uint8 u8_Return;
	uint16 u16_Value;
	task_glucose_history_item t_HistoryItem;
	

	if (TaskGlucose_ReadHistory(u16_Index, 0, &t_HistoryItem) == FUNCTION_OK)
	{
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Year + YEAR_BASE;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Month;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Day;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Hour;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Minute;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Second;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Glucose;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint8)Glucose_Round((sint32)t_HistoryItem.u16_Temperature, 
			GLUCOSE_TEMPERATURE_RATIO);
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Flag;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\r', ui_Length);
	}

	u8_Return = '\n';

	DrvUART_Write(DRV_UART_DEVICE_ID_1, &u8_Return, sizeof(u8_Return));
}


static void TaskShell_OutputInfo(void)
{
	uint ui_Length;
	uint ui_Value;
	uint8 u8_Return;
	uint16 u16_Value;


	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_YEAR, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, 
		(uint16)ui_Value + YEAR_BASE);
	TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MONTH, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, (uint16)ui_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_DAY, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, (uint16)ui_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_HOUR, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, (uint16)ui_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MINUTE, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, (uint16)ui_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_SECOND, (uint8 *)&ui_Value,
		(uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, (uint16)ui_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	Drv_GetConfig(DRV_PARAM_VOLTAGE_VDD, (uint8 *)&u16_Value, (uint *)0);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	u16_Value = Glucose_Read(GLUCOSE_CHANNEL_TEMPERATURE);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	u16_Value = Glucose_Read(GLUCOSE_CHANNEL_NTC);
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_VERSION_MAJOR, (uint8 *)&u16_Value,
		(uint *)0);
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '.', ui_Length);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_VERSION_MINOR, (uint8 *)&u16_Value,
		(uint *)0);
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
	TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_ID, m_u8_OutputBuffer, (uint *)0);
	TaskShell_HexToAscii(m_u8_OutputBuffer, m_u8_OutputBuffer, 
		sizeof(m_u8_OutputBuffer));
	ui_Length = OUTPUT_BUFFER_SIZE;
	TaskShell_OutputValue(m_u8_OutputBuffer, '\r', ui_Length);

	u8_Return = '\n';

	DrvUART_Write(DRV_UART_DEVICE_ID_1, &u8_Return, sizeof(u8_Return));
}


static void TaskShell_OutputStrip
(
	uint16 u16_Index
)
{
	uint ui_Length;
	uint8 u8_Return;
	uint16 u16_Value;
	task_glucose_history_item t_HistoryItem;
	

	if (TaskGlucose_ReadHistory(u16_Index, 0, &t_HistoryItem) == FUNCTION_OK)
	{
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Year + YEAR_BASE;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Month;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '-', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Day;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Hour;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Minute;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, ':', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = (uint16)t_HistoryItem.t_DateTime.u8_Second;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Glucose;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Temperature;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Current;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
		ui_Length = OUTPUT_BUFFER_SIZE;
		u16_Value = t_HistoryItem.u16_Impedance;
		LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, u16_Value);
		TaskShell_OutputValue(m_u8_OutputBuffer, '\r', ui_Length);
	}

	u8_Return = '\n';

	DrvUART_Write(DRV_UART_DEVICE_ID_1, &u8_Return, sizeof(u8_Return));
}


static void TaskShell_OutputTest
(
	uint16 u16_Index
)
{
	uint ui_Length;
	uint8 u8_Return;
	uint16 u16_Value;
	uint16 *u16p_TestData;


	ui_Length = sizeof(u16_Value);
	TaskGlucose_GetConfig(TASK_GLUCOSE_PARAM_TEST, (uint8 *)&u16_Value, 
		&ui_Length);
	u16p_TestData = (uint16 *)u16_Value;	
	u16p_TestData += TASK_GLUCOSE_TEST_DATA_COUNT - u16_Index;
	ui_Length = OUTPUT_BUFFER_SIZE;
	LibString_NumberToString(m_u8_OutputBuffer, &ui_Length, *u16p_TestData);
	
	if (u16_Index > 1)
	{
		TaskShell_OutputValue(m_u8_OutputBuffer, '\t', ui_Length);
	}
	else
	{
		TaskShell_OutputValue(m_u8_OutputBuffer, '\r', ui_Length);
		u8_Return = '\n';
		DrvUART_Write(DRV_UART_DEVICE_ID_1, &u8_Return, sizeof(u8_Return));
	}
}


static void TaskShell_Tick
(
	uint16 u16_TickTime
)
{
	uint ui_Value;
	uint8 u8_Token;


	do
	{
		if (m_u16_DelayTimer > u16_TickTime)
		{
			m_u16_DelayTimer -= u16_TickTime;
			break;
		}

		if (m_u16_DelayTimer > 0)
		{
			m_u16_DelayTimer = 0;
			DrvADC_Sample();
			TaskShell_OutputInfo();
			DrvADC_Disable();
			Drv_DisablePower();
			break;
		}

		if (DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_BUSY, 
			(uint8 *)&ui_Value, (uint *)0) != FUNCTION_OK)
		{
			break;
		}

		if ((ui_Value != 0) || (m_u16_OutputIndex > 0))
		{
			break;
		}

		ui_Value = sizeof(u8_Token);

		if (DrvUART_Read(DRV_UART_DEVICE_ID_1, &u8_Token, &ui_Value) !=
			FUNCTION_OK)
		{
			break;
		}

		if (DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, 
			DRV_UART_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value, (uint *)0) != 
			FUNCTION_OK)
		{
			break;
		}

		if (ui_Value != 0)
		{
			break;
		}

		if (Glucose_GetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value,
			(uint *)0) != FUNCTION_OK)
		{
			break;
		}

		if (ui_Value == 0)
		{
			break;
		}

		switch (u8_Token)
		{
			case HISTORY_OUTPUT_TOKEN:
				TaskShell_HandleCommand(TASK_SHELL_COMMAND_OUTPUT_HISTORY);
				break;

			case INFO_OUTPUT_TOKEN:
				TaskShell_HandleCommand(TASK_SHELL_COMMAND_OUTPUT_INFO);
				break;

			case STRIP_OUTPUT_TOKEN:
				TaskShell_HandleCommand(TASK_SHELL_COMMAND_OUTPUT_STRIP);
				break;

			case TEST_OUTPUT_TOKEN:
				TaskShell_HandleCommand(TASK_SHELL_COMMAND_OUTPUT_TEST);
				break;
			case TEST_RE_TOKEN:
				TaskShell_HandleCommand(TASK_SHELL_COMMAND_OUTPUT_RE);
				break;
			default:
				break;
		}
	}
	while (0);
}


static void TaskShell_HandleCommand
(
	uint ui_Command
)
{
  
    uint8 ui_value[25];
	uint8 i;
	switch (ui_Command)
	{
		case TASK_SHELL_COMMAND_OUTPUT_HISTORY:
			TaskGlucose_GetConfig(TASK_GLUCOSE_PARAM_HISTORY_COUNT, 
				(uint8 *)&m_u16_OutputIndex, (uint *)0);

			if (m_u16_OutputIndex > 0)
			{
				TaskShell_OutputHistory(m_u16_OutputIndex);
				m_u16_OutputIndex--;
			}

			break;

		case TASK_SHELL_COMMAND_OUTPUT_INFO:
			Drv_EnablePower();
			DrvADC_Enable();
			m_u16_DelayTimer = ADC_DELAY;
			break;

		case TASK_SHELL_COMMAND_OUTPUT_STRIP:
			TaskGlucose_GetConfig(TASK_GLUCOSE_PARAM_HISTORY_COUNT, 
				(uint8 *)&m_u16_OutputIndex, (uint *)0);

			if (m_u16_OutputIndex > 0)
			{
				TaskShell_OutputStrip(m_u16_OutputIndex);
				m_u16_OutputIndex--;
			}

			break;

		case TASK_SHELL_COMMAND_OUTPUT_TEST:
			m_u16_OutputIndex = TASK_GLUCOSE_TEST_DATA_COUNT;
			TaskShell_OutputTest(m_u16_OutputIndex);
			m_u16_OutputIndex--;
			break;

		case TASK_SHELL_COMMAND_OUTPUT_RE:
			tran_re(&m_u8_reout[0]);
		   for(i=0;i<25;i++)
		   	{
		   		ui_value[i]=m_u8_reout[i];
		   	}
		   DrvUART_Write(DRV_UART_DEVICE_ID_1,&ui_value[0], 25);
				
		default:
			break;
	}

	m_ui_Command = ui_Command;
}


static uint TaskShell_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	uint ui_Value;


	switch (ui_Event)
	{
		case DRV_UART_EVENT_WRITE_DONE:

			if (DrvUART_GetConfig(ui_DeviceID, DRV_UART_PARAM_BUSY, 
				(uint8 *)&ui_Value, (uint *)0) != FUNCTION_OK)
			{
				return FUNCTION_FAIL;
			}

			if ((ui_Value == 0) && (m_u16_OutputIndex > 0))
			{
				switch (m_ui_Command)
				{
					case TASK_SHELL_COMMAND_OUTPUT_HISTORY:
						TaskShell_OutputHistory(m_u16_OutputIndex);
						break;

					case TASK_SHELL_COMMAND_OUTPUT_STRIP:
						TaskShell_OutputStrip(m_u16_OutputIndex);
						break;

					case TASK_SHELL_COMMAND_OUTPUT_TEST:
						TaskShell_OutputTest(m_u16_OutputIndex);
						break;

					default:
						break;
				}

				m_u16_OutputIndex--;
			}

			break;

		case DRV_UART_EVENT_INTERRUPT_WRITE_DONE:
			DevOS_MessageSend(TASK_MESSAGE_ID_SHELL, (const uint8 *)0, 0);
			break;

		default:
			return TaskDevice_HandleDeviceEvent(ui_DeviceID, ui_Event,
				u8p_Data, ui_Length);
	}

	return FUNCTION_OK;
}
