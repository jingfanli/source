/*
 * Module:	Device manager task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "drv.h"
#include "glucose.h"
#include "button.h"
#include "bluetooth.h"

#include "device.h"
#include "devfs.h"
#include "lib_string.h"

#include "task.h"
#include "task_device.h"
#include "drv_voice.h"

//Constant definition

#define TASK_DEVICE_CYCLE				100

#define SIGNAL_PRESENT_DELAY			500
#define BUTTON_LONG_PRESS_DELAY			2000
#define DISPLAY_RESET_DELAY				1000
#define DATE_TIME_UPDATE_DELAY			1000
#define ALARM_BEEP_DELAY				2000
#define ERROR_DELAY						TASK_CALIBRATE_TIME(5000)

#define DISPLAY_TIMEOUT					TASK_CALIBRATE_TIME(12000)
#define DISPLAY_BUFFER_SIZE				3

#define WEEKDAY_COUNT					7
#define HOUR_PER_DAY					24
#define MINUTE_PER_HOUR					60
#define ALARM_RETRY_MINUTE				5

#define WAKEUP_BEEP_COUNT				1	
#define WAKEUP_BEEP_ON_INTERVAL			100
#define WAKEUP_BEEP_OFF_INTERVAL		0	
#define ERROR_BEEP_COUNT				2	
#define ERROR_BEEP_ON_INTERVAL			200
#define ERROR_BEEP_OFF_INTERVAL			200	
#define ALARM_BEEP_COUNT				2	
#define ALARM_BEEP_ON_INTERVAL			200
#define ALARM_BEEP_OFF_INTERVAL			100	

#define SERIAL_NUMBER_OFFSET_COMMAND	16

#define DATA_ADDRESS_FACTORY_CONFIG		((uint16)0x1300)


//Type definition

typedef enum
{
	TASK_DEVICE_FLAG_ALARM_RETRY = 0,
	TASK_DEVICE_FLAG_ALARM_INTERRUPT,
	TASK_DEVICE_FLAG_GLUCOSE_INTERRUPT,
	TASK_DEVICE_COUNT_FLAG
} task_device_flag;

typedef enum
{
	TASK_DEVICE_STATE_SLEEP = 0,
	TASK_DEVICE_STATE_WAKEUP,
	TASK_DEVICE_STATE_ALARM,
	TASK_DEVICE_STATE_COMMUNICATION,
	TASK_DEVICE_STATE_ERROR,
	TASK_DEVICE_STATE_GLUCOSE,
	TASK_DEVICE_STATE_SETTING,
	TASK_DEVICE_STATE_HISTORY,
	TASK_DEVICE_COUNT_STATE
} task_device_state;

typedef enum
{
	TASK_DEVICE_TRIGGER_INVALID = 0,
	TASK_DEVICE_TRIGGER_BUTTON,
	TASK_DEVICE_TRIGGER_GLUCOSE,
	TASK_DEVICE_TRIGGER_ALARM,
	TASK_DEVICE_TRIGGER_COMMUNICATION,
	TASK_DEVICE_COUNT_TRIGGER
} task_device_trigger;

typedef enum
{
	TASK_DEVICE_SETTING_FLAG_AUDIO = 0,
	TASK_DEVICE_SETTING_FLAG_TIME_FORMAT,
	TASK_DEVICE_SETTING_FLAG_BLUETOOTH,
	TASK_DEVICE_COUNT_SETTING_FLAG
} task_device_setting_flag;

typedef struct
{
	uint8 u8_Weekday;
	uint8 u8_Hour;
	uint8 u8_Minute;
	uint8 u8_Switch;
} task_device_alarm;

typedef struct
{
	uint8 u8_FileID;
	uint8 u8_Flag;
	uint8 u8_Year;
	uint8 u8_Month;
	uint8 u8_Day;
	uint8 u8_Hour;
	uint8 u8_Minute;
	uint8 u8_Second;
} task_device_setting;

typedef struct
{
	uint16 u16_VersionMajor;
	uint16 u16_VersionMinor;
	uint8 u8_MealFlag;
	uint8 u8_GlucoseUnit;
	uint8 u8_NoCode;
	uint8 u8_Reserved;
	uint32 u32_Model;
	uint8 u8_ID[DEVICE_ID_SIZE];
} task_device_factory_config;


//Private variable definition

static uint m_ui_Flag = {0};
static uint m_ui_State = {0};
static uint m_ui_Trigger = {0};
static uint m_ui_PowerMode = {0};
static uint m_ui_AlarmOnCount = {0};
static uint m_u8_DisplayBuffer[DISPLAY_BUFFER_SIZE] = {0};
static uint16 m_u16_TimeoutTimer = {0};
static uint16 m_u16_DateTimeTimer = {0};
static uint16 m_u16_AlarmTimer = {0};
static uint16 m_u16_ErrorTimer = {0};
static task_device_setting m_t_Setting = {0};
static task_device_factory_config m_t_FactoryConfig = {0};
static task_device_alarm m_t_Alarm[TASK_DEVICE_ALARM_COUNT] = {0};

static uint8 m_u8_ATCommand[] = "AT+NAMEGoChek - 000000\r\n";


//Private function declaration

static void TaskDevice_AsciiToHex
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint ui_Length
);
static uint8 TaskDevice_24HourTo12Hour
(
	uint8 u8_Hour,
	uint8 *u8p_PM
);
static uint TaskDevice_ResetFile(void);
static void TaskDevice_TriggerButton(void);
static void TaskDevice_TriggerGlucose(void);
static void TaskDevice_TriggerAlarm(void);
static void TaskDevice_TriggerCommunication(void);
static uint TaskDevice_ProcessWakeup
(
	uint *uip_State,
	uint *uip_MessageID,
	uint8 *u8p_MessageData
);
static void TaskDevice_ProcessAlarm(void);
static void TaskDevice_ProcessCommunication(void);
static void TaskDevice_ProcessError(void);
static void TaskDevice_ProcessTimeout(void);
static void TaskDevice_ProcessSleep(void);
static void TaskDevice_DisplayDatetime(void);
static void TaskDevice_DisplayError(void);
static void TaskDevice_DisplayBluetooth(void);
static void TaskDevice_ReloadAlarmNext(void);
static void TaskDevice_ReloadAlarmRetry(void);


//Public function definition

uint TaskDevice_Initialize
(
	devos_task_handle t_TaskHandle
)
{
	uint i;
	uint ui_Value;
	uint16 u16_Value;
	devfs_int t_Length;
	drv_rtc_callback t_RTCCallback;
	drv_gpio_callback t_GPIOCallback;


	Button_Initialize();
	Glucose_Initialize();
	Bluetooth_Initialize();
	VOICE_INIT();

	t_RTCCallback.fp_Wakeup = (drv_rtc_callback_wakeup)0;
	t_RTCCallback.fp_Alarm = TaskDevice_TriggerAlarm;
	DrvRTC_SetConfig(DRV_RTC_PARAM_CALLBACK, (const uint8 *)&t_RTCCallback, 0);

	u16_Value = BUTTON_LONG_PRESS_DELAY;
	Button_SetConfig(BUTTON_ID_LEFT, BUTTON_PARAM_LONG_PRESS_DELAY,
		(const uint8 *)&u16_Value, sizeof(u16_Value));
	Button_SetConfig(BUTTON_ID_CENTER, BUTTON_PARAM_LONG_PRESS_DELAY,
		(const uint8 *)&u16_Value, sizeof(u16_Value));
	Button_SetConfig(BUTTON_ID_RIGHT, BUTTON_PARAM_LONG_PRESS_DELAY,
		(const uint8 *)&u16_Value, sizeof(u16_Value));
	t_GPIOCallback.fp_Interrupt = TaskDevice_TriggerButton;
	Button_SetConfig(BUTTON_ID_LEFT, BUTTON_PARAM_CALLBACK,
		(const uint8 *)&t_GPIOCallback, 0);
	Button_SetConfig(BUTTON_ID_CENTER, BUTTON_PARAM_CALLBACK,
		(const uint8 *)&t_GPIOCallback, 0);
	Button_SetConfig(BUTTON_ID_RIGHT, BUTTON_PARAM_CALLBACK,
		(const uint8 *)&t_GPIOCallback, 0);
	t_GPIOCallback.fp_Interrupt = TaskDevice_TriggerGlucose;
	Glucose_SetConfig(GLUCOSE_PARAM_CALLBACK, (const uint8 *)&t_GPIOCallback, 0);

	u16_Value = sizeof(m_t_FactoryConfig);
	DrvFLASH_Read(DATA_ADDRESS_FACTORY_CONFIG, (uint8 *)&m_t_FactoryConfig, 
		&u16_Value);
	Drv_Memcpy(m_u8_ATCommand + SERIAL_NUMBER_OFFSET_COMMAND, 
		m_t_FactoryConfig.u8_ID, sizeof(m_t_FactoryConfig.u8_ID));

	t_Length = sizeof(m_t_Setting);

	if (DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING,
		(uint8 *)&m_t_Setting, 0, &t_Length) != FUNCTION_OK)
	{
		TaskDevice_ResetFile();
	}
	else
	{
		//Check if the id of history file is matched or not
		if ((m_t_Setting.u8_FileID != TASK_FILE_ID_SETTING) ||
			(t_Length != sizeof(m_t_Setting)))
		{
			TaskDevice_ResetFile();
		}
		else
		{
			t_Length = sizeof(m_t_Alarm);
			DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING,
				(uint8 *)&m_t_Alarm, sizeof(m_t_Setting), &t_Length);

			for (i = 0; i < TASK_DEVICE_ALARM_COUNT; i++)
			{
				if (m_t_Alarm[i].u8_Switch != 0)
				{
					m_ui_AlarmOnCount++;
				}
			}

			TaskDevice_ReloadAlarmNext();

			if (m_ui_AlarmOnCount > 0)
			{
				ui_Value = 1;
				DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_ALARM, 
					(const uint8 *)&ui_Value, sizeof(ui_Value));
			}
			else
			{
				ui_Value = 0;
				DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_ALARM, 
					(const uint8 *)&ui_Value, sizeof(ui_Value));
			}
		}
	}

	if (REG_GET_BIT(m_t_Setting.u8_Flag, TASK_DEVICE_SETTING_FLAG_AUDIO) != 0)
	{
		ui_Value = 1;
	}
	else
	{
		ui_Value = 0;
	}

	DrvBEEP_SetConfig(DRV_BEEP_PARAM_SWITCH, (const uint8 *)&ui_Value, 
		sizeof(ui_Value));
	m_ui_State = TASK_DEVICE_STATE_SLEEP;
	Glucose_GetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value,
		(uint *)0);

	if (ui_Value == 0)
	{
		TaskDevice_TriggerGlucose();
		return FUNCTION_OK;
	}

	DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SIGNAL_PRESENT, 
		(uint8 *)&ui_Value, (uint *)0);

	if (ui_Value == 0)
	{
		TaskDevice_TriggerCommunication();
		return FUNCTION_OK;
	}

	for (i = BUTTON_ID_LEFT; i <= BUTTON_ID_RIGHT; i++)
	{
		if (Button_Read(i) == BUTTON_STATE_PRESS)
		{
			TaskDevice_TriggerButton();
			return FUNCTION_OK;
		}
	}

	ui_Value = DRV_SLEEP_MODE_HALT;
	Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
		sizeof(ui_Value));

	return FUNCTION_OK;
}


void TaskDevice_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint ui_Value;
	static uint ui_Bluetooth;
	static uint ui_MessageID;
	static uint8 u8_MessageData;
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;


	DEVOS_TASK_BEGIN

	DrvLCD_Tick(TASK_DEVICE_CYCLE);
	DrvBEEP_Tick(TASK_DEVICE_CYCLE);
	Button_Tick(TASK_DEVICE_CYCLE);
	Bluetooth_Tick(TASK_DEVICE_CYCLE);

	if (m_ui_State == TASK_DEVICE_STATE_WAKEUP)
	{
		if (TaskDevice_ProcessWakeup(&m_ui_State, &ui_MessageID, 
			&u8_MessageData) == FUNCTION_OK)
		{
			if ((ui_MessageID == TASK_MESSAGE_ID_DEVICE) &&
				(u8_MessageData == TASK_MESSAGE_DATA_SLEEP))
			{
				ui_Value = 1;
			}
			else
			{
				ui_Value = 0;
			}

			if (m_ui_State == TASK_DEVICE_STATE_GLUCOSE)
			{
				DevOS_TaskDelay(SIGNAL_PRESENT_DELAY);
				Glucose_GetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, 
					(uint8 *)&ui_Value, (uint *)0);
			}

			if (ui_Value == 0)
			{
				Drv_GetConfig(DRV_PARAM_POWER_MODE, (uint8 *)&m_ui_PowerMode,
					(uint *)0);
				ui_Value = 0;

				if (m_ui_PowerMode != DRV_POWER_MODE_OFF)
				{
					if (m_ui_State == TASK_DEVICE_STATE_GLUCOSE)
					{
						DevOS_MessageSend(ui_MessageID, &u8_MessageData, 
							sizeof(u8_MessageData));
					}

					if ((REG_GET_BIT(m_t_Setting.u8_Flag, 
						TASK_DEVICE_SETTING_FLAG_BLUETOOTH) != 0) &&
						((m_ui_State == TASK_DEVICE_STATE_HISTORY) ||
						 (m_ui_State == TASK_DEVICE_STATE_GLUCOSE)))
					{
						Bluetooth_GetConfig(BLUETOOTH_PARAM_SIGNAL_PRESENT,
							(uint8 *)&ui_Bluetooth, (uint *)0);

						if (ui_Bluetooth == 0)
						{
							ui_Bluetooth = 1;
						}
						else
						{
							ui_Bluetooth = 0;
						}
					}
				}

				Bluetooth_SetConfig(BLUETOOTH_PARAM_SWITCH, 
					(const uint8 *)&ui_Bluetooth, sizeof(ui_Bluetooth));
				DrvLCD_Enable();
				ui_Value = DRV_LCD_DATA_ALL_ON;
				DrvLCD_SetConfig(DRV_LCD_OFFSET_MONTH_TENS, DRV_LCD_PARAM_RESET, 
					(const uint8 *)&ui_Value);
				DrvBEEP_Start(WAKEUP_BEEP_COUNT, WAKEUP_BEEP_ON_INTERVAL,
					WAKEUP_BEEP_OFF_INTERVAL);
				DevOS_TaskDelay(DISPLAY_RESET_DELAY);
				
				if (ui_Bluetooth != 0)
				{
					ui_Value = 0;
					DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_FRAME,
						(const uint8 *)&ui_Value, sizeof(ui_Value));
					DrvUART_Write(DRV_UART_DEVICE_ID_2, m_u8_ATCommand, 
						sizeof(m_u8_ATCommand) - 1);
				}

				DrvBEEP_Stop();
				DevOS_TaskDelay(DISPLAY_RESET_DELAY);
				
				if (ui_Bluetooth != 0)
				{
					ui_Value = sizeof(m_u8_ATCommand) - 1;
					DrvUART_Read(DRV_UART_DEVICE_ID_2, (uint8 *)0, &ui_Value);
					ui_Value = 1;
					DrvUART_SetConfig(DRV_UART_DEVICE_ID_2, DRV_UART_PARAM_FRAME,
						(const uint8 *)&ui_Value, sizeof(ui_Value));

					if (m_ui_State == TASK_DEVICE_STATE_GLUCOSE)
					{
						ui_Bluetooth = 0;
						Bluetooth_SetConfig(BLUETOOTH_PARAM_SWITCH, 
							(const uint8 *)&ui_Bluetooth, sizeof(ui_Bluetooth));
					}
				}

				TaskDevice_InitializeDisplay();
				
				if (m_ui_PowerMode == DRV_POWER_MODE_OFF)
				{
					m_u16_DateTimeTimer = DATE_TIME_UPDATE_DELAY;
					m_ui_State = TASK_DEVICE_STATE_ERROR;
					TaskDevice_DisplayError();
				}
				else
				{
					if (ui_MessageID != TASK_MESSAGE_ID_DEVICE)
					{
						if (ui_MessageID != TASK_MESSAGE_ID_GLUCOSE)
						{
							DevOS_MessageSend(ui_MessageID, &u8_MessageData, 
								sizeof(u8_MessageData));
						}
					}
					else
					{
						DevOS_MessageClear(TASK_MESSAGE_ID_SYSTEM, t_TaskHandle);
					}
				}
			}
			else
			{
				TaskDevice_ProcessSleep();
			}
		}
	}

	if (m_ui_State != TASK_DEVICE_STATE_SETTING)
	{
		TaskDevice_DisplayBluetooth();
	}

	if (m_ui_State == TASK_DEVICE_STATE_ALARM)
	{
		TaskDevice_ProcessAlarm();
	}

	if (m_ui_State == TASK_DEVICE_STATE_COMMUNICATION)
	{
		TaskDevice_ProcessCommunication();
	}

	if (m_ui_State == TASK_DEVICE_STATE_ERROR)
	{
		TaskDevice_ProcessError();
	}

	if ((m_ui_State == TASK_DEVICE_STATE_SETTING) ||
		(m_ui_State == TASK_DEVICE_STATE_HISTORY) ||
		(m_ui_State == TASK_DEVICE_STATE_ALARM))
	{
		TaskDevice_ProcessTimeout();
	}

	if ((m_ui_State == TASK_DEVICE_STATE_ALARM) ||
		(m_ui_State == TASK_DEVICE_STATE_COMMUNICATION) ||
		(m_ui_State == TASK_DEVICE_STATE_ERROR) ||
		(m_ui_State == TASK_DEVICE_STATE_GLUCOSE))
	{
		TaskDevice_DisplayDatetime();
	}

	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_DEVICE, 
		&t_MessageLength);

	if (u8p_MessageData != (uint8 *)0)
	{
		if (*u8p_MessageData == TASK_MESSAGE_DATA_SLEEP)
		{
			Task_EnterCritical();

			if (REG_GET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_INTERRUPT) != 0)
			{
				REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_INTERRUPT);
				m_ui_State = TASK_DEVICE_STATE_WAKEUP;
				m_ui_Trigger = TASK_DEVICE_TRIGGER_ALARM;
				Task_ExitCritical();
			}
			else if (REG_GET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_GLUCOSE_INTERRUPT) != 0)
			{
				REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_GLUCOSE_INTERRUPT);
				m_ui_State = TASK_DEVICE_STATE_WAKEUP;
				m_ui_Trigger = TASK_DEVICE_TRIGGER_GLUCOSE;
				Task_ExitCritical();
			}
			else
			{
				Task_ExitCritical();
				TaskDevice_ProcessSleep();
			}
		}
	}	

	DevOS_TaskDelay(TASK_DEVICE_CYCLE);

	DEVOS_TASK_END
}


uint TaskDevice_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	uint ui_Value;


	ui_Value = *((const uint *)u8p_Value);

	switch (ui_Parameter)
	{
		case TASK_DEVICE_PARAM_AUDIO:
			REG_WRITE_FIELD(m_t_Setting.u8_Flag, TASK_DEVICE_SETTING_FLAG_AUDIO, 
				REG_MASK_1_BIT, ui_Value);
			DrvBEEP_SetConfig(DRV_BEEP_PARAM_SWITCH, u8p_Value, ui_Length);
			break;

		case TASK_DEVICE_PARAM_TIME_FORMAT:
			REG_WRITE_FIELD(m_t_Setting.u8_Flag, 
				TASK_DEVICE_SETTING_FLAG_TIME_FORMAT, REG_MASK_1_BIT, ui_Value);
			break;

		case TASK_DEVICE_PARAM_BLUETOOTH:
			REG_WRITE_FIELD(m_t_Setting.u8_Flag, 
				TASK_DEVICE_SETTING_FLAG_BLUETOOTH, REG_MASK_1_BIT, ui_Value);
			break;

		default:
			return FUNCTION_FAIL;
	}

	DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING, 
		(const uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting));

	return FUNCTION_OK;
}


uint TaskDevice_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_DEVICE_PARAM_AUDIO:
			*((uint *)u8p_Value) = (uint)REG_READ_FIELD(m_t_Setting.u8_Flag, 
				TASK_DEVICE_SETTING_FLAG_AUDIO, REG_MASK_1_BIT);
			break;

		case TASK_DEVICE_PARAM_TIME_FORMAT:
			*((uint *)u8p_Value) = (uint)REG_READ_FIELD(m_t_Setting.u8_Flag, 
				TASK_DEVICE_SETTING_FLAG_TIME_FORMAT, REG_MASK_1_BIT);
			break;

		case TASK_DEVICE_PARAM_BLUETOOTH:
			*((uint *)u8p_Value) = (uint)REG_READ_FIELD(m_t_Setting.u8_Flag, 
				TASK_DEVICE_SETTING_FLAG_BLUETOOTH, REG_MASK_1_BIT);
			break;

		case TASK_DEVICE_PARAM_VERSION_MAJOR:
			*((uint16 *)u8p_Value) = m_t_FactoryConfig.u16_VersionMajor;
			break;

		case TASK_DEVICE_PARAM_VERSION_MINOR:
			*((uint16 *)u8p_Value) = m_t_FactoryConfig.u16_VersionMinor;
			break;

		case TASK_DEVICE_PARAM_MEAL_FLAG:
			*((uint *)u8p_Value) = (uint)m_t_FactoryConfig.u8_MealFlag;
			break;

		case TASK_DEVICE_PARAM_GLUCOSE_UNIT:
			*((uint *)u8p_Value) = (uint)m_t_FactoryConfig.u8_GlucoseUnit;
			break;

		case TASK_DEVICE_PARAM_NO_CODE:
			*((uint *)u8p_Value) = (uint)m_t_FactoryConfig.u8_NoCode;
			break;

		case TASK_DEVICE_PARAM_ID:
			TaskDevice_AsciiToHex(u8p_Value, m_t_FactoryConfig.u8_ID,
				sizeof(m_t_FactoryConfig.u8_ID));
			break;

		case TASK_DEVICE_PARAM_MODEL:
			*((uint32 *)u8p_Value) = m_t_FactoryConfig.u32_Model;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint TaskDevice_SetAlarm
(
	uint ui_AlarmID,
	uint ui_Parameter,
	uint ui_Value
)
{
	task_device_alarm *tp_Alarm;


	if (ui_AlarmID >= TASK_DEVICE_ALARM_COUNT)
	{
		return FUNCTION_FAIL;
	}

	tp_Alarm = &(m_t_Alarm[ui_AlarmID]);

	switch (ui_Parameter)
	{
		case TASK_DEVICE_PARAM_ALARM_WEEKDAY:
			
			if ((ui_Value > 0) && (ui_Value <= WEEKDAY_COUNT))
			{
				tp_Alarm->u8_Weekday = 0x00;
				REG_SET_BIT(tp_Alarm->u8_Weekday, ui_Value - 1);
			}
			else
			{
				tp_Alarm->u8_Weekday = 0xFF;
			}

			break;
			
		case TASK_DEVICE_PARAM_ALARM_HOUR:
			tp_Alarm->u8_Hour = (uint8)ui_Value;
			break;
			
		case TASK_DEVICE_PARAM_ALARM_MINUTE:
			tp_Alarm->u8_Minute = (uint8)ui_Value;
			break;
			
		case TASK_DEVICE_PARAM_ALARM_SWITCH:

			if ((tp_Alarm->u8_Switch == 0) && (uint8)ui_Value != 0)
			{
				tp_Alarm->u8_Switch = (uint8)ui_Value;
				m_ui_AlarmOnCount++;
				
				if (m_ui_AlarmOnCount == 1)
				{
					ui_Value = 1;
					DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_ALARM, 
						(const uint8 *)&ui_Value, sizeof(ui_Value));
				}
			}

			if ((tp_Alarm->u8_Switch != 0) && (uint8)ui_Value == 0)
			{
				tp_Alarm->u8_Switch = (uint8)ui_Value;
				m_ui_AlarmOnCount--;
				
				if (m_ui_AlarmOnCount == 0)
				{
					ui_Value = 0;
					DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_ALARM, 
						(const uint8 *)&ui_Value, sizeof(ui_Value));
				}
			}

			break;
			
		default:
			return FUNCTION_FAIL;
	}

	if (REG_GET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY) == 0)
	{
		TaskDevice_ReloadAlarmNext();
	}

	DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING, 
		(const uint8 *)tp_Alarm, (sizeof(*tp_Alarm) * ui_AlarmID) + 
		sizeof(m_t_Setting), sizeof(*tp_Alarm));

	return FUNCTION_OK;
}


uint TaskDevice_GetAlarm
(
	uint ui_AlarmID,
	uint ui_Parameter,
	uint *uip_Value
)
{
	uint ui_Value;
	task_device_alarm *tp_Alarm;


	if (ui_AlarmID >= TASK_DEVICE_ALARM_COUNT)
	{
		return FUNCTION_FAIL;
	}

	tp_Alarm = &(m_t_Alarm[ui_AlarmID]);
	
	if (tp_Alarm->u8_Weekday == 0)
	{
		tp_Alarm->u8_Weekday = 0xFF;
	}

	switch (ui_Parameter)
	{
		case TASK_DEVICE_PARAM_ALARM_WEEKDAY:
			
			if (tp_Alarm->u8_Weekday == 0xFF)
			{
				*uip_Value = 0;
			}
			else
			{
				ui_Value = (uint)tp_Alarm->u8_Weekday;
				*uip_Value = 1;

				while ((ui_Value & 1) == 0)
				{
					ui_Value >>= 1;
					(*uip_Value)++;
				}
			}

			break;
			
		case TASK_DEVICE_PARAM_ALARM_HOUR:
			*uip_Value = (uint)tp_Alarm->u8_Hour;
			break;
			
		case TASK_DEVICE_PARAM_ALARM_MINUTE:
			*uip_Value = (uint)tp_Alarm->u8_Minute;
			break;
			
		case TASK_DEVICE_PARAM_ALARM_SWITCH:
			*uip_Value = (uint)tp_Alarm->u8_Switch;
			break;
			
		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void TaskDevice_UpdateAlarm(void)
{
	REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY);
	TaskDevice_ReloadAlarmNext();
}


void TaskDevice_NumberToBCD
(
	uint8 *u8p_BCD,
	uint *uip_Length,
	uint16 u16_Number
)
{
	uint ui_Length;


	if (LibString_NumberToString(u8p_BCD, uip_Length, u16_Number) == FUNCTION_OK)
	{
		ui_Length = *uip_Length;

		while (ui_Length > 0)
		{
			*u8p_BCD -= '0';
			u8p_BCD++;
			ui_Length--;
		}
	}
}


void TaskDevice_InitializeDisplay(void)
{
	uint ui_Value;


	ui_Value = DRV_LCD_DATA_ALL_OFF;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_MONTH_TENS, 
		DRV_LCD_PARAM_RESET, (const uint8 *)&ui_Value);

	m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_ON;
	DrvLCD_Write(DRV_LCD_OFFSET_TIME_COLON, m_u8_DisplayBuffer, 1);
	DrvLCD_Write(DRV_LCD_OFFSET_DATE_BAR, m_u8_DisplayBuffer, 1);

	if ((m_ui_State == TASK_DEVICE_STATE_GLUCOSE) ||
		(m_ui_State == TASK_DEVICE_STATE_ALARM) ||
		(m_ui_State == TASK_DEVICE_STATE_COMMUNICATION))
	{
		ui_Value = 1;
		DrvLCD_SetConfig(DRV_LCD_OFFSET_TIME_COLON, 
			DRV_LCD_PARAM_BLINK, (const uint8 *)&ui_Value);
		m_u16_DateTimeTimer = DATE_TIME_UPDATE_DELAY;
	}

	if (m_ui_State == TASK_DEVICE_STATE_ALARM)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_ALARM, m_u8_DisplayBuffer, 1);
		ui_Value = 1;
		DrvLCD_SetConfig(DRV_LCD_OFFSET_ALARM, DRV_LCD_PARAM_BLINK, 
			(const uint8 *)&ui_Value);
		m_u16_AlarmTimer = ALARM_BEEP_DELAY;
	}

	if (REG_GET_BIT(m_t_Setting.u8_Flag, TASK_DEVICE_SETTING_FLAG_AUDIO) == 0)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_AUDIO, m_u8_DisplayBuffer, 1);
	}

	if (m_ui_PowerMode != DRV_POWER_MODE_NORMAL)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_BATTERY, m_u8_DisplayBuffer, 1);
	}

	if (m_ui_State != TASK_DEVICE_STATE_SETTING)
	{
		TaskDevice_DisplayBluetooth();
	}

	if (m_ui_State == TASK_DEVICE_STATE_COMMUNICATION)
	{
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
		m_u8_DisplayBuffer[1] = DRV_LCD_CHARACTER_P;
		m_u8_DisplayBuffer[2] = DRV_LCD_CHARACTER_C;
		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, m_u8_DisplayBuffer, 
			sizeof(m_u8_DisplayBuffer));
	}
}


void TaskDevice_DisplayMonth
(
	uint8 u8_Month
)
{
	uint ui_Length;


	ui_Length = sizeof(m_u8_DisplayBuffer);
	TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, (uint16)u8_Month);

	if (ui_Length < 2)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_MONTH_UNITS, m_u8_DisplayBuffer, 1);
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_MONTH_TENS, m_u8_DisplayBuffer, 1);
	}
	else
	{
		DrvLCD_Write(DRV_LCD_OFFSET_MONTH_TENS, m_u8_DisplayBuffer, 2);
	}
}


void TaskDevice_DisplayDay
(
	uint8 u8_Day
)
{
	uint ui_Length;


	ui_Length = sizeof(m_u8_DisplayBuffer);
	TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, (uint16)u8_Day);

	if (ui_Length < 2)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_DAY_TENS, m_u8_DisplayBuffer, 1);
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_DAY_UNITS, m_u8_DisplayBuffer, 1);
	}
	else
	{
		DrvLCD_Write(DRV_LCD_OFFSET_DAY_TENS, m_u8_DisplayBuffer, 2);
	}
}


void TaskDevice_DisplayHour
(
	uint8 u8_Hour,
	uint8 u8_TimeFormat
)
{
	uint ui_Length;


	m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
	m_u8_DisplayBuffer[1] = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_PM, m_u8_DisplayBuffer, 2);

	//Check if 12 hour time format is selected
	if (u8_TimeFormat == 0)
	{
		ui_Length = sizeof(m_u8_DisplayBuffer);
		TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, (uint16)u8_Hour);
	}
	else
	{
		m_u8_DisplayBuffer[0] = TaskDevice_24HourTo12Hour(u8_Hour, 
			&m_u8_DisplayBuffer[1]);

		if (m_u8_DisplayBuffer[1] == 0)
		{
			m_u8_DisplayBuffer[1] = DRV_LCD_SYMBOL_ON;
			DrvLCD_Write(DRV_LCD_OFFSET_AM, &m_u8_DisplayBuffer[1], 1);
		}
		else
		{
			m_u8_DisplayBuffer[1] = DRV_LCD_SYMBOL_ON;
			DrvLCD_Write(DRV_LCD_OFFSET_PM, &m_u8_DisplayBuffer[1], 1);
		}


		ui_Length = sizeof(m_u8_DisplayBuffer);
		TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, 
			(uint16)m_u8_DisplayBuffer[0]);
	}

	if (ui_Length < 2)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_UNITS, m_u8_DisplayBuffer, 1);
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_TENS, m_u8_DisplayBuffer, 1);
	}
	else
	{
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_TENS, m_u8_DisplayBuffer, 2);
	}
}


void TaskDevice_DisplayMinute
(
	uint8 u8_Minute
)
{
	uint ui_Length;


	ui_Length = sizeof(m_u8_DisplayBuffer);
	TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, (uint16)u8_Minute);

	if (ui_Length < 2)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_MINUTE_UNITS, m_u8_DisplayBuffer, 1);
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_0;
		DrvLCD_Write(DRV_LCD_OFFSET_MINUTE_TENS, m_u8_DisplayBuffer, 1);
	}
	else
	{
		DrvLCD_Write(DRV_LCD_OFFSET_MINUTE_TENS, m_u8_DisplayBuffer, 2);
	}
}


void TaskDevice_DisplayGlucose
(
	uint16 u16_Glucose
)
{
	uint i;
	uint ui_Length;


	if (u16_Glucose >= TASK_DEVICE_ERROR_ID_BASE)
	{
		DrvBEEP_Start(ERROR_BEEP_COUNT, ERROR_BEEP_ON_INTERVAL, 
			ERROR_BEEP_OFF_INTERVAL);

		if (u16_Glucose < TASK_DEVICE_ERROR_ID_OVERFLOW)
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_E;
			m_u8_DisplayBuffer[1] = DRV_LCD_CHARACTER_BAR;
			m_u8_DisplayBuffer[2] = (uint8)(u16_Glucose - TASK_DEVICE_ERROR_ID_BASE);
		}
		else if (u16_Glucose == TASK_DEVICE_ERROR_ID_OVERFLOW)
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
			m_u8_DisplayBuffer[1] = DRV_LCD_CHARACTER_H;
			m_u8_DisplayBuffer[2] = DRV_LCD_CHARACTER_I;
		}
		else if (u16_Glucose == TASK_DEVICE_ERROR_ID_UNDERFLOW)
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
			m_u8_DisplayBuffer[1] = DRV_LCD_CHARACTER_L;
			m_u8_DisplayBuffer[2] = DRV_LCD_CHARACTER_O;
		}

		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, m_u8_DisplayBuffer,
			sizeof(m_u8_DisplayBuffer));	
	}
	else
	{
		for (i = 0; i < sizeof(m_u8_DisplayBuffer); i++)
		{
			m_u8_DisplayBuffer[i] = DRV_LCD_CHARACTER_ALL_OFF;
		}

		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, m_u8_DisplayBuffer, 
			sizeof(m_u8_DisplayBuffer));
		ui_Length = sizeof(m_u8_DisplayBuffer);
		TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, u16_Glucose);
		DrvLCD_Write((DRV_LCD_OFFSET_GLUCOSE_TENS + sizeof(m_u8_DisplayBuffer)) - 
			ui_Length, m_u8_DisplayBuffer, ui_Length);
	}
}


void TaskDevice_DisplayUnit
(
	uint8 u8_LCDData
)
{
	uint ui_LCDOffsetUnit;


	if (m_t_FactoryConfig.u8_GlucoseUnit == 0)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_POINT, &u8_LCDData, 
			sizeof(u8_LCDData));
		ui_LCDOffsetUnit = DRV_LCD_OFFSET_MMOL;
	}
	else
	{
		ui_LCDOffsetUnit = DRV_LCD_OFFSET_MG;
	}

	DrvLCD_Write(ui_LCDOffsetUnit, &u8_LCDData, sizeof(u8_LCDData));
}


uint TaskDevice_HandleDeviceEvent
(
	uint ui_DeviceID,
	uint ui_Event,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	if (ui_DeviceID == DRV_UART_DEVICE_ID_1)
	{
		switch (ui_Event)
		{
			case DRV_UART_EVENT_INTERRUPT_TRIGGER:
				TaskDevice_TriggerCommunication();
				break;

			default:
				return FUNCTION_FAIL;
		}
	}

	return FUNCTION_OK;
}


#if TASK_DEVICE_TEST_ENABLE == 1

void TaskDevice_Test(void)
{
}

#endif


//Private function definition

static void TaskDevice_AsciiToHex
(
	uint8 *u8p_Target,
	const uint8 *u8p_Source,
	uint ui_Length
)
{
	while (ui_Length > 0)
	{
		ui_Length--;

		if ((u8p_Source[ui_Length] >= '0') && (u8p_Source[ui_Length] <= '9'))
		{
			u8p_Target[ui_Length] = u8p_Source[ui_Length] - '0'; 
		}
		else if ((u8p_Source[ui_Length] >= 'a') && (u8p_Source[ui_Length] <= 'z'))
		{
			u8p_Target[ui_Length] = u8p_Source[ui_Length] - 'a' + 10;
		}
		else if ((u8p_Source[ui_Length] >= 'A') && (u8p_Source[ui_Length] <= 'Z'))
		{
			u8p_Target[ui_Length] = u8p_Source[ui_Length] - 'A' + 10;
		}
	}
}


static uint8 TaskDevice_24HourTo12Hour
(
	uint8 u8_Hour,
	uint8 *u8p_PM
)
{
	if (u8_Hour < 12)
	{
		*u8p_PM = 0;
	}
	else
	{
		*u8p_PM = 1;
	}

	if (u8_Hour == 0)
	{
		return 12;
	}
	else if ((u8_Hour > 0) && (u8_Hour <= 12))
	{
		return u8_Hour;
	}
	else
	{
		return u8_Hour - 12;
	}
}


static uint TaskDevice_ResetFile(void)
{
	uint i;
	uint ui_Value;


	//Delete the setting file
	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING,
		(const uint8 *)0, 0, 0) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	for (i = 0; i < (sizeof(m_t_Alarm) / sizeof(task_device_alarm)); i++)
	{
		m_t_Alarm[i].u8_Weekday = 0xFF;
	}

	REG_SET_BIT(m_t_Setting.u8_Flag, TASK_DEVICE_SETTING_FLAG_AUDIO);
	Bluetooth_GetConfig(BLUETOOTH_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value,
		(uint *)0);

	if (ui_Value == 0)
	{
		REG_SET_BIT(m_t_Setting.u8_Flag, TASK_DEVICE_SETTING_FLAG_BLUETOOTH);
	}

	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING,
		(const uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting)) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_SETTING,
		(const uint8 *)&m_t_Alarm, sizeof(m_t_Setting), sizeof(m_t_Alarm)) !=
		FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return DevFS_Synchronize(TASK_FILE_SYSTEM_VOLUME_ID);
}


static void TaskDevice_TriggerButton(void)
{
	uint ui_Value;
	

	if (m_ui_State == TASK_DEVICE_STATE_SLEEP)
	{
		m_ui_State = TASK_DEVICE_STATE_WAKEUP;
		m_ui_Trigger = TASK_DEVICE_TRIGGER_BUTTON;
		ui_Value = 1;
		DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		ui_Value = DRV_SLEEP_MODE_WAIT;
		Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
	}
}


static void TaskDevice_TriggerGlucose(void)
{
	uint ui_Value;
	uint8 u8_MessageData;
	

	if (m_ui_State == TASK_DEVICE_STATE_SLEEP)
	{
		m_ui_State = TASK_DEVICE_STATE_WAKEUP;
		m_ui_Trigger = TASK_DEVICE_TRIGGER_GLUCOSE;
		ui_Value = 1;
		DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		ui_Value = DRV_SLEEP_MODE_WAIT;
		Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
	}
	else
	{
		if ((m_ui_State == TASK_DEVICE_STATE_HISTORY) ||
			(m_ui_State == TASK_DEVICE_STATE_COMMUNICATION) ||
			(m_ui_State == TASK_DEVICE_STATE_ALARM))
		{
			REG_SET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_GLUCOSE_INTERRUPT);
			u8_MessageData = TASK_MESSAGE_DATA_QUIT;
			DevOS_MessageSend(TASK_MESSAGE_ID_SYSTEM, &u8_MessageData, 
				sizeof(u8_MessageData));
		}
	}
}


static void TaskDevice_TriggerAlarm(void)
{
	uint ui_Value;
	uint8 u8_MessageData;
	

	if (m_ui_State == TASK_DEVICE_STATE_SLEEP)
	{
		m_ui_State = TASK_DEVICE_STATE_WAKEUP;
		m_ui_Trigger = TASK_DEVICE_TRIGGER_ALARM;
		ui_Value = 1;
		DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		ui_Value = DRV_SLEEP_MODE_WAIT;
		Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
	}
	else
	{
		if ((m_ui_State == TASK_DEVICE_STATE_HISTORY) ||
			(m_ui_State == TASK_DEVICE_STATE_COMMUNICATION))
		{
			REG_SET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_INTERRUPT);
			u8_MessageData = TASK_MESSAGE_DATA_QUIT;
			DevOS_MessageSend(TASK_MESSAGE_ID_SYSTEM, &u8_MessageData, 
				sizeof(u8_MessageData));
		}
	}
}


static void TaskDevice_TriggerCommunication(void)
{
	uint ui_Value;
	

	ui_Value = 1;
	DrvUART_SetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SWITCH, 
		(const uint8 *)&ui_Value, sizeof(ui_Value));

	if (m_ui_State == TASK_DEVICE_STATE_SLEEP)
	{
		m_ui_State = TASK_DEVICE_STATE_WAKEUP;
		m_ui_Trigger = TASK_DEVICE_TRIGGER_COMMUNICATION;
		ui_Value = 1;
		DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		ui_Value = DRV_SLEEP_MODE_WAIT;
		Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
	}
}


static uint TaskDevice_ProcessWakeup
(
	uint *uip_State,
	uint *uip_MessageID,
	uint8 *u8p_MessageData
)
{
	uint i;
	uint ui_ButtonState[BUTTON_COUNT_ID];


	if (m_ui_Trigger == TASK_DEVICE_TRIGGER_BUTTON)
	{
		for (i = BUTTON_ID_LEFT; i <= BUTTON_ID_RIGHT; i++)
		{
			ui_ButtonState[i] = Button_Read(i);
		}

		if ((ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_RELEASE) &&
			(ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_RELEASE) &&
			(ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_RELEASE))
		{
			*uip_MessageID = TASK_MESSAGE_ID_DEVICE;
			*u8p_MessageData = TASK_MESSAGE_DATA_SLEEP;
			return FUNCTION_OK;
		}
		else if (ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_LONG_PRESS)
		{
			*uip_MessageID = TASK_MESSAGE_ID_SETTING;
			*u8p_MessageData = TASK_MESSAGE_DATA_SYSTEM;
			*uip_State = TASK_DEVICE_STATE_SETTING;
			return FUNCTION_OK;
		} 
		else if (ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_LONG_PRESS)
		{
			*uip_MessageID = TASK_MESSAGE_ID_HISTORY;
			*u8p_MessageData = TASK_MESSAGE_DATA_SYSTEM;
			*uip_State = TASK_DEVICE_STATE_HISTORY;
			return FUNCTION_OK;
		} 
		else if (ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_LONG_PRESS)
		{
			*uip_MessageID = TASK_MESSAGE_ID_SETTING;
			*u8p_MessageData = TASK_MESSAGE_DATA_ALARM;
			*uip_State = TASK_DEVICE_STATE_SETTING;
			return FUNCTION_OK;
		}
		else 
		{
			return FUNCTION_FAIL;
		}
	}
	else if (m_ui_Trigger == TASK_DEVICE_TRIGGER_GLUCOSE)
	{
		*uip_MessageID = TASK_MESSAGE_ID_GLUCOSE;
		*u8p_MessageData = TASK_MESSAGE_DATA_SYSTEM;
		*uip_State = TASK_DEVICE_STATE_GLUCOSE;
		return FUNCTION_OK;
	}
	else if (m_ui_Trigger == TASK_DEVICE_TRIGGER_ALARM)
	{
		*uip_MessageID = TASK_MESSAGE_ID_DEVICE;
		*u8p_MessageData = TASK_MESSAGE_DATA_ALARM;
		*uip_State = TASK_DEVICE_STATE_ALARM;
		return FUNCTION_OK;
	}
	else if (m_ui_Trigger == TASK_DEVICE_TRIGGER_COMMUNICATION)
	{
		*uip_MessageID = TASK_MESSAGE_ID_DEVICE;
		*u8p_MessageData = TASK_MESSAGE_DATA_SYSTEM;
		*uip_State = TASK_DEVICE_STATE_COMMUNICATION;
		return FUNCTION_OK;
	}
	else
	{
		*uip_MessageID = TASK_MESSAGE_ID_DEVICE;
		*u8p_MessageData = TASK_MESSAGE_DATA_SLEEP;
		return FUNCTION_OK;
	}
}


static void TaskDevice_ProcessAlarm(void)
{
	uint i;
	uint ui_Value;
	uint ui_ButtonState[BUTTON_COUNT_ID];
	uint8 *u8p_MessageData;
	devos_int t_MessageLength;


	m_u16_AlarmTimer += TASK_DEVICE_CYCLE;

	if (m_u16_AlarmTimer >= ALARM_BEEP_DELAY)
	{
		m_u16_AlarmTimer = 0;
		DrvBEEP_Start(ALARM_BEEP_COUNT, ALARM_BEEP_ON_INTERVAL,
			ALARM_BEEP_OFF_INTERVAL);
	}

	Glucose_GetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value, 
		(uint *)0);

	if (ui_Value == 0)
	{
		DrvBEEP_Stop();
		ui_Value = 0;
		DrvLCD_SetConfig(DRV_LCD_OFFSET_ALARM, DRV_LCD_PARAM_BLINK, 
			(const uint8 *)&ui_Value);
		m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_ALARM, m_u8_DisplayBuffer, 1);
		REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY);
		TaskDevice_ReloadAlarmNext();
		m_ui_State = TASK_DEVICE_STATE_WAKEUP;
		m_ui_Trigger = TASK_DEVICE_TRIGGER_GLUCOSE;
		return;
	}

	for (i = BUTTON_ID_LEFT; i <= BUTTON_ID_RIGHT; i++)
	{
		ui_ButtonState[i] = Button_Read(i);
		
		if (ui_ButtonState[i] == BUTTON_STATE_PRESS)
		{
			DrvBEEP_Stop();
			REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY);
			TaskDevice_ReloadAlarmNext();
			TaskDevice_ProcessSleep();
			return;
		}
	}

	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_SYSTEM, 
		&t_MessageLength);

	if (u8p_MessageData != (uint8 *)0)
	{
		if (*u8p_MessageData == TASK_MESSAGE_DATA_QUIT)
		{
			DrvBEEP_Stop();

			if (REG_GET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY) == 0)
			{
				REG_SET_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY);
				TaskDevice_ReloadAlarmRetry();
			}
			else
			{
				REG_CLEAR_BIT(m_ui_Flag, TASK_DEVICE_FLAG_ALARM_RETRY);
				TaskDevice_ReloadAlarmNext();
			}

			TaskDevice_ProcessSleep();
		}
	}	
}


static void TaskDevice_ProcessCommunication(void)
{
	uint ui_Value;
	uint8 u8_MessageData;
	uint8 *u8p_MessageData;
	devos_int t_MessageLength;


	u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_SYSTEM, 
		&t_MessageLength);

	if (u8p_MessageData != (uint8 *)0)
	{
		if (*u8p_MessageData == TASK_MESSAGE_DATA_QUIT)
		{
			u8_MessageData = TASK_MESSAGE_DATA_SLEEP;
			DevOS_MessageSend(TASK_MESSAGE_ID_DEVICE, &u8_MessageData,
				sizeof(u8_MessageData));
			return;
		}
	}	

	DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SIGNAL_PRESENT, 
		(uint8 *)&ui_Value, (uint *)0);

	if (ui_Value != 0)
	{
		TaskDevice_ProcessSleep();
	}
}


static void TaskDevice_ProcessError(void)
{
	m_u16_ErrorTimer += TASK_DEVICE_CYCLE;

	if (m_u16_ErrorTimer >= ERROR_DELAY)
	{
		TaskDevice_ProcessSleep();
	}
}


static void TaskDevice_ProcessTimeout(void)
{
	uint i;
	uint ui_ButtonState[BUTTON_COUNT_ID];
	uint8 u8_MessageData;


	for (i = BUTTON_ID_LEFT; i <= BUTTON_ID_RIGHT; i++)
	{
		ui_ButtonState[i] = Button_Read(i);
	}

	if ((ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_RELEASE) &&
		(ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_RELEASE) &&
		(ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_RELEASE))
	{
		m_u16_TimeoutTimer += TASK_DEVICE_CYCLE / 10;
	}
	else
	{
		m_u16_TimeoutTimer = 0;
	}

	if (m_u16_TimeoutTimer >= DISPLAY_TIMEOUT)
	{
		u8_MessageData = TASK_MESSAGE_DATA_QUIT;
		DevOS_MessageSend(TASK_MESSAGE_ID_SYSTEM, &u8_MessageData, 
			sizeof(u8_MessageData));
	}
}


static void TaskDevice_ProcessSleep(void)
{
	uint ui_Value;


	DevFS_Synchronize(TASK_FILE_SYSTEM_VOLUME_ID);
	m_u16_TimeoutTimer = 0;
	ui_Value = 0;
	Bluetooth_SetConfig(BLUETOOTH_PARAM_SWITCH, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	Drv_SetConfig(DRV_PARAM_PVD, (const uint8 *)&ui_Value, sizeof(ui_Value));
	ui_Value = DRV_LCD_DATA_ALL_OFF;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_MONTH_TENS, DRV_LCD_PARAM_RESET, 
		(const uint8 *)&ui_Value);
	DrvLCD_Disable();

	Task_EnterCritical();
	ui_Value = 0;
	DrvUART_SetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SWITCH, 
		(const uint8 *)&ui_Value, sizeof(ui_Value));
	DrvRTC_SetConfig(DRV_RTC_PARAM_SWITCH_WAKEUP, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	ui_Value = DRV_SLEEP_MODE_HALT;
	Drv_SetConfig(DRV_PARAM_SLEEP_MODE, (const uint8 *)&ui_Value, 
		sizeof(ui_Value));
	m_ui_State = TASK_DEVICE_STATE_SLEEP;
	DrvUART_GetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SIGNAL_PRESENT, 
		(uint8 *)&ui_Value, (uint *)0);

	if (ui_Value == 0)
	{
		TaskDevice_TriggerCommunication();
	}
	
	Task_ExitCritical();
}


static void TaskDevice_DisplayDatetime(void)
{
	uint ui_Value;


	m_u16_DateTimeTimer += TASK_DEVICE_CYCLE;

	if (m_u16_DateTimeTimer >= DATE_TIME_UPDATE_DELAY)
	{
		m_u16_DateTimeTimer = 0;

		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MONTH, (uint8 *)&ui_Value,
			(uint *)0);
		TaskDevice_DisplayMonth((uint8)ui_Value);
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_DAY, (uint8 *)&ui_Value,
			(uint *)0);
		TaskDevice_DisplayDay((uint8)ui_Value);
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_HOUR, (uint8 *)&ui_Value,
			(uint *)0);
		TaskDevice_DisplayHour((uint8)ui_Value, 
			REG_READ_FIELD(m_t_Setting.u8_Flag, 
			TASK_DEVICE_SETTING_FLAG_TIME_FORMAT, REG_MASK_1_BIT));
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MINUTE, (uint8 *)&ui_Value,
			(uint *)0);
		TaskDevice_DisplayMinute((uint8)ui_Value);
	}
}


static void TaskDevice_DisplayError(void)
{
	uint ui_Value;


	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_BATTERY, DRV_LCD_PARAM_BLINK,
		(const uint8 *)&ui_Value);
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_BATTERY);
	m_u16_ErrorTimer = 0;
}


static void TaskDevice_DisplayBluetooth(void)
{
	uint ui_Value;


	ui_Value = 1;
	DrvLCD_Read(DRV_LCD_OFFSET_BLUETOOTH, m_u8_DisplayBuffer, &ui_Value);
	Bluetooth_GetConfig(BLUETOOTH_PARAM_SWITCH, (uint8 *)&ui_Value, (uint *)0);

	if (ui_Value == 0)
	{
		if (m_u8_DisplayBuffer[0] != DRV_LCD_SYMBOL_OFF)
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
			DrvLCD_Write(DRV_LCD_OFFSET_BLUETOOTH, m_u8_DisplayBuffer, 1);
		}
	}
	else
	{
		if (m_u8_DisplayBuffer[0] != DRV_LCD_SYMBOL_ON)
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_ON;
			DrvLCD_Write(DRV_LCD_OFFSET_BLUETOOTH, m_u8_DisplayBuffer, 1);
		}

		Drv_GetConfig(DRV_PARAM_PVD, (uint8 *)&ui_Value, (uint *)0);	
	}

	DrvLCD_SetConfig(DRV_LCD_OFFSET_BLUETOOTH, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
}


static void TaskDevice_ReloadAlarmNext(void)
{
	uint i;
	uint ui_Hour;
	uint ui_Minute;
	uint ui_AlarmIndex;
	uint16 u16_CurrentTime;
	uint16 u16_NextTime;
	uint16 u16_AlarmTime;
	task_device_alarm *tp_Alarm;


	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_HOUR, (uint8 *)&ui_Hour, 
		(uint *)0);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MINUTE, (uint8 *)&ui_Minute, 
		(uint *)0);

	if (m_ui_AlarmOnCount > 0)
	{
		u16_CurrentTime = (uint16)ui_Hour * MINUTE_PER_HOUR + (uint16)ui_Minute;
		u16_NextTime = HOUR_PER_DAY * MINUTE_PER_HOUR * 2;
		ui_AlarmIndex = TASK_DEVICE_ALARM_COUNT;
		tp_Alarm = &m_t_Alarm[0];

		//Find the alarm time that is after current time and closest to current 
		//time
		for (i = 0; i < TASK_DEVICE_ALARM_COUNT; i++)
		{
			if (tp_Alarm->u8_Switch != 0)
			{
				u16_AlarmTime = (uint16)tp_Alarm->u8_Hour * MINUTE_PER_HOUR + 
					(uint16)tp_Alarm->u8_Minute;
				
				if (u16_AlarmTime <= u16_CurrentTime)
				{
					u16_AlarmTime += HOUR_PER_DAY * MINUTE_PER_HOUR;
				}

				if (u16_AlarmTime < u16_NextTime)
				{
					u16_NextTime = u16_AlarmTime;
					ui_AlarmIndex = i;
				}
			}

			tp_Alarm++;
		}

		if (ui_AlarmIndex < TASK_DEVICE_ALARM_COUNT)
		{
			ui_Hour = (uint)m_t_Alarm[ui_AlarmIndex].u8_Hour;
			ui_Minute = (uint)m_t_Alarm[ui_AlarmIndex].u8_Minute;
			DrvRTC_SetConfig(DRV_RTC_PARAM_ALARM_HOUR, (const uint8 *)&ui_Hour,
				sizeof(ui_Hour));
			DrvRTC_SetConfig(DRV_RTC_PARAM_ALARM_MINUTE, 
				(const uint8 *)&ui_Minute, sizeof(ui_Minute));
		}
	}
}


static void TaskDevice_ReloadAlarmRetry(void)
{
	uint ui_Hour;
	uint ui_Minute;


	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_HOUR, (uint8 *)&ui_Hour,
		(uint *)0);
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MINUTE, (uint8 *)&ui_Minute,
		(uint *)0);
	ui_Minute += ALARM_RETRY_MINUTE;

	if (ui_Minute >= MINUTE_PER_HOUR)
	{
		ui_Minute -= MINUTE_PER_HOUR;
		ui_Hour++;

		if (ui_Hour >= HOUR_PER_DAY)
		{
			ui_Hour = 0;
		}
	}

	DrvRTC_SetConfig(DRV_RTC_PARAM_ALARM_HOUR, (const uint8 *)&ui_Hour,
		sizeof(ui_Hour));
	DrvRTC_SetConfig(DRV_RTC_PARAM_ALARM_MINUTE, (const uint8 *)&ui_Minute,
		sizeof(ui_Minute));
}
