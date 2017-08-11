/*
 * Module:	Setting manager task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "drv.h"
#include "button.h"
#include "bluetooth.h"

#include "device.h"

#include "task.h"
#include "task_device.h"
#include "task_glucose.h"
#include "task_setting.h"


//Constant definition

#define DISPLAY_BUFFER_SIZE				4
#define DISPLAY_OFFSET_VALUE_TENS		DRV_LCD_OFFSET_GLUCOSE_TENS
#define DISPLAY_OFFSET_VALUE_UNITS		DRV_LCD_OFFSET_GLUCOSE_UNITS
#define DISPLAY_OFFSET_VALUE_FRACTION	DRV_LCD_OFFSET_GLUCOSE_FRACTION
#define DISPLAY_VALUE_LENGTH_MAX		3

#define TASK_SETTING_CYCLE				100

#define MONTH_FEBRUARY					2
#define YEAR_BASE						2000

#define GLUCOSE_MG_TO_MMOL_NUMERATOR	10			
#define GLUCOSE_MG_TO_MMOL_DENOMINATOR	18			


//Type definition

typedef uint (*task_setting_save)
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);

typedef uint (*task_setting_load)
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);

typedef enum
{
	TASK_SETTING_FLAG_LEAP_YEAR = 0,
	TASK_SETTING_FLAG_MMOL,
	TASK_SETTING_FLAG_BLUETOOTH,
	TASK_SETTING_COUNT_FLAG
} task_setting_flag;

typedef enum
{
	TASK_SETTING_BLINK_OFF = 0,
	TASK_SETTING_BLINK_ON = 1,
	TASK_SETTING_COUNT_BLINK
} task_setting_blink;

typedef enum
{
	TASK_SETTING_ITEM_YEAR = 0,
	TASK_SETTING_ITEM_MONTH,
	TASK_SETTING_ITEM_DAY,
	TASK_SETTING_ITEM_HOUR,
	TASK_SETTING_ITEM_MINUTE,
	TASK_SETTING_ITEM_TIME_FORMAT,
	TASK_SETTING_ITEM_AUDIO,
	TASK_SETTING_ITEM_HYPO,
	TASK_SETTING_ITEM_HYPER,
	TASK_SETTING_ITEM_KETONE,
	TASK_SETTING_ITEM_BLUETOOTH,
	TASK_SETTING_ITEM_MEAL,
	TASK_SETTING_ITEM_ALARM_ID,
	TASK_SETTING_ITEM_ALARM_HOUR,
	TASK_SETTING_ITEM_ALARM_MINUTE,
	TASK_SETTING_ITEM_ALARM_SWITCH,
	TASK_SETTING_COUNT_ITEM
} task_setting_item;

typedef struct
{
	uint ui_Offset;
	uint ui_Length;
} task_setting_display;

typedef struct
{
	uint16 u16_LowerLimit;
	uint16 u16_UpperLimit;
} task_setting_limit;

typedef struct
{
	uint ui_Parameter;
	task_setting_save fp_SaveSetting;
	task_setting_load fp_LoadSetting;
} task_setting_operation;


//Private function declaration

static uint TaskSetting_CheckLeapYear
(
	uint16 u16_Year
);
static void TaskSetting_SetLabelBlink
(
	uint ui_Item,
	uint ui_Blink
);
static void TaskSetting_DisplayLabel
(
	uint ui_Item,
	uint16 u16_Value
);
static void TaskSetting_DisplayValue
(
	uint ui_Item,
	uint16 u16_Value
);
static void TaskSetting_DisplayItem
(
	uint ui_Item,
	uint16 u16_Value
);
static void TaskSetting_SelectItem
(
	uint ui_Item
);
static void TaskSetting_DeselectItem
(
	uint ui_Item
);
static void TaskSetting_ResetAllLabels
(
	uint ui_FirstItem,
	uint ui_LastItem
);
static void TaskSetting_ProcessSelect
(
	uint ui_FirstItem,
	uint ui_LastItem
);
static void TaskSetting_ProcessValueChange
(
	uint ui_ButtonID,
	uint ui_Item
);
static void TaskSetting_ProcessInput(void);
static void TaskSetting_SaveSetting
(
	uint ui_Item
);
static void TaskSetting_LoadSetting
(
	uint ui_Item
);
static uint TaskSetting_SetAlarmConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint TaskSetting_GetAlarmConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
static uint TaskSetting_GetDevice
(
	device *tp_Device
);


//Private variable definition

static uint m_ui_Flag = {0};
static uint m_ui_CurrentItem = {0};
static uint m_ui_ButtonState[BUTTON_COUNT_ID] = {0};
static uint m_u8_DisplayBuffer[DISPLAY_BUFFER_SIZE] = {0};
static uint16 m_u16_Value[TASK_SETTING_COUNT_ITEM] = {0};

static const uint8 m_u8_TokenSwitchOn[] = 
{
	DRV_LCD_CHARACTER_O,
	DRV_LCD_CHARACTER_N
};
static const uint8 m_u8_TokenSwitchOff[] = 
{
	DRV_LCD_CHARACTER_O,
	DRV_LCD_CHARACTER_F,
	DRV_LCD_CHARACTER_F
};
static const uint8 m_u8_Token12Hour[] = 
{
	DRV_LCD_CHARACTER_1,
	DRV_LCD_CHARACTER_2,
	DRV_LCD_CHARACTER_H
};
static const uint8 m_u8_Token24Hour[] = 
{
	DRV_LCD_CHARACTER_2,
	DRV_LCD_CHARACTER_4,
	DRV_LCD_CHARACTER_H
};
static const task_setting_limit m_t_MonthLimit[] = {{1, 12}};
static const task_setting_limit m_t_DayLimit[] = 
{
	{1, 31}, {1, 28}, {1, 31}, {1, 30}, {1, 31}, {1, 30},
	{1, 31}, {1, 31}, {1, 30}, {1, 31}, {1, 30}, {1, 31}, {1, 29}
};
static const task_setting_limit m_t_HourLimit[] = {{0, 23}};
static const task_setting_limit m_t_MinuteLimit[] = {{0, 59}};
static const task_setting_limit m_t_YearLimit[] = {{0, 50}};
static const task_setting_limit m_t_HypoLimit[] = {{19, 100}, {10, 56}};
static const task_setting_limit m_t_HyperLimit[] = {{119, 600}, {66, 333}};
static const task_setting_limit m_t_SwitchLimit[] = {{0, 1}};
static const task_setting_limit m_t_AlarmIDLimit[] = {{1, TASK_DEVICE_ALARM_COUNT}};
static const task_setting_limit *m_tp_Limit[TASK_SETTING_COUNT_ITEM] = 
{
	m_t_YearLimit,
	m_t_MonthLimit,
	m_t_DayLimit,
	m_t_HourLimit,
	m_t_MinuteLimit,
	m_t_SwitchLimit,
	m_t_SwitchLimit,
	m_t_HypoLimit,
	m_t_HyperLimit,
	m_t_SwitchLimit,
	m_t_SwitchLimit,
	m_t_SwitchLimit,
	m_t_AlarmIDLimit,
	m_t_HourLimit,
	m_t_MinuteLimit,
	m_t_SwitchLimit
};
static const task_setting_display m_t_LabelDisplay[TASK_SETTING_COUNT_ITEM] = 
{
	{DRV_LCD_OFFSET_MINUTE_TENS, 2},
	{DRV_LCD_OFFSET_MONTH_TENS, 2},
	{DRV_LCD_OFFSET_DAY_TENS, 2},
	{DRV_LCD_OFFSET_HOUR_TENS, 2},
	{DRV_LCD_OFFSET_MINUTE_TENS, 2},
	{DRV_LCD_OFFSET_PM, 2},
	{DRV_LCD_OFFSET_AUDIO, 1},
	{DRV_LCD_OFFSET_HYPO, 1},
	{DRV_LCD_OFFSET_HYPER, 1},
	{DRV_LCD_OFFSET_KETONE, 1},
	{DRV_LCD_OFFSET_BLUETOOTH, 1},
	{DRV_LCD_OFFSET_AFTER_MEAL, 1},
	{DRV_LCD_OFFSET_DAY_TENS, 2},
	{DRV_LCD_OFFSET_HOUR_TENS, 2},
	{DRV_LCD_OFFSET_MINUTE_TENS, 2},
	{DRV_LCD_OFFSET_ALARM, 1}
};
static const task_setting_display m_t_ValueDisplay = 
{
	DRV_LCD_OFFSET_GLUCOSE_TENS, 3
};
static const task_setting_operation m_t_Operation[TASK_SETTING_COUNT_ITEM] =
{
	{DRV_RTC_PARAM_CALENDAR_YEAR, DrvRTC_SetConfig, DrvRTC_GetConfig},
	{DRV_RTC_PARAM_CALENDAR_MONTH, DrvRTC_SetConfig, DrvRTC_GetConfig},
	{DRV_RTC_PARAM_CALENDAR_DAY, DrvRTC_SetConfig, DrvRTC_GetConfig},
	{DRV_RTC_PARAM_CALENDAR_HOUR, DrvRTC_SetConfig, DrvRTC_GetConfig},
	{DRV_RTC_PARAM_CALENDAR_MINUTE, DrvRTC_SetConfig, DrvRTC_GetConfig},
	{TASK_DEVICE_PARAM_TIME_FORMAT, TaskDevice_SetConfig, TaskDevice_GetConfig},
	{TASK_DEVICE_PARAM_AUDIO, TaskDevice_SetConfig, TaskDevice_GetConfig},
	{TASK_GLUCOSE_PARAM_HYPO, TaskGlucose_SetConfig, TaskGlucose_GetConfig},
	{TASK_GLUCOSE_PARAM_HYPER, TaskGlucose_SetConfig, TaskGlucose_GetConfig},
	{TASK_GLUCOSE_PARAM_KETONE, TaskGlucose_SetConfig, TaskGlucose_GetConfig},
	{TASK_DEVICE_PARAM_BLUETOOTH, TaskDevice_SetConfig, TaskDevice_GetConfig},
	{TASK_GLUCOSE_PARAM_MEAL, TaskGlucose_SetConfig, TaskGlucose_GetConfig},
	{0, 0, 0},
	{TASK_DEVICE_PARAM_ALARM_HOUR, TaskSetting_SetAlarmConfig, 
		TaskSetting_GetAlarmConfig},
	{TASK_DEVICE_PARAM_ALARM_MINUTE, TaskSetting_SetAlarmConfig, 
		TaskSetting_GetAlarmConfig},
	{TASK_DEVICE_PARAM_ALARM_SWITCH, TaskSetting_SetAlarmConfig, 
		TaskSetting_GetAlarmConfig}
};


//Public function definition

uint TaskSetting_Initialize
(
	devos_task_handle t_TaskHandle
)
{

	return FUNCTION_OK;
}


void TaskSetting_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint ui_FirstItem;
	static uint ui_LastItem;
	static uint8 u8_MessageData;
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;


	DEVOS_TASK_BEGIN

	DevOS_MessageWait(TASK_MESSAGE_ID_SETTING, u8p_MessageData, 
		&t_MessageLength); 

	if (t_MessageLength == 1)
	{
		if (*u8p_MessageData == TASK_MESSAGE_DATA_SYSTEM)
		{
			ui_FirstItem = TASK_SETTING_ITEM_YEAR;
			ui_LastItem = TASK_SETTING_ITEM_MEAL;
		}
		else if (*u8p_MessageData == TASK_MESSAGE_DATA_ALARM)
		{
			ui_FirstItem = TASK_SETTING_ITEM_ALARM_ID;
			ui_LastItem = TASK_SETTING_ITEM_ALARM_SWITCH;
		}
		else
		{
			ui_FirstItem = TASK_SETTING_COUNT_ITEM;
			ui_LastItem = TASK_SETTING_COUNT_ITEM;
		}
	}
	else
	{
		ui_FirstItem = TASK_SETTING_COUNT_ITEM;
		ui_LastItem = TASK_SETTING_COUNT_ITEM;
	}

	if ((ui_FirstItem < TASK_SETTING_COUNT_ITEM) &&
		(ui_LastItem < TASK_SETTING_COUNT_ITEM))
	{
		TaskSetting_ResetAllLabels(ui_FirstItem, ui_LastItem);
		m_ui_CurrentItem = ui_FirstItem;
		TaskSetting_SelectItem(m_ui_CurrentItem);
		DevOS_MessageClear(TASK_MESSAGE_ID_SYSTEM, t_TaskHandle);

		if (ui_FirstItem == TASK_SETTING_ITEM_YEAR)
		{
			while (Button_Read(BUTTON_ID_LEFT) != BUTTON_STATE_RELEASE)
			{
				DevOS_TaskDelay(TASK_SETTING_CYCLE);
			}
		}
		else if (ui_FirstItem == TASK_SETTING_ITEM_ALARM_ID)
		{
			while (Button_Read(BUTTON_ID_RIGHT) != BUTTON_STATE_RELEASE)
			{
				DevOS_TaskDelay(TASK_SETTING_CYCLE);
			}
		}

		while (1)
		{
			u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_SYSTEM,
				&t_MessageLength);	

			if (((u8p_MessageData != (uint8 *)0) &&
				(*u8p_MessageData == TASK_MESSAGE_DATA_QUIT)) ||
				(m_ui_CurrentItem > ui_LastItem))
			{
				DevOS_MessageClear(TASK_MESSAGE_ID_SETTING, t_TaskHandle);
				u8_MessageData = TASK_MESSAGE_DATA_SLEEP;
				DevOS_MessageSend(TASK_MESSAGE_ID_DEVICE, &u8_MessageData,
					sizeof(u8_MessageData));
				break;
			}

			TaskSetting_ProcessSelect(ui_FirstItem, ui_LastItem);
			TaskSetting_ProcessInput();

			DevOS_TaskDelay(TASK_SETTING_CYCLE);
		}
	}

	DEVOS_TASK_END
}


uint TaskSetting_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{

	return FUNCTION_OK;
}


uint TaskSetting_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_SETTING_PARAM_DEVICE:

			if (uip_Length != (uint *)0)
			{
				*uip_Length = sizeof(device);
			}

			return TaskSetting_GetDevice((device *)u8p_Value);

		default:
			return FUNCTION_FAIL;
	}
}


//Private function definition

static uint TaskSetting_CheckLeapYear
(
	uint16 u16_Year
)
{
	u16_Year += YEAR_BASE;

	if ((((u16_Year % 4) == 0) && ((u16_Year % 100) != 0)) ||
		((u16_Year % 400) == 0))
	{
		return FUNCTION_OK;
	}
	else
	{
		return FUNCTION_FAIL;
	}
}


static void TaskSetting_SetLabelBlink
(
	uint ui_Item,
	uint ui_Blink
)
{
	uint ui_Length;
	uint ui_Offset;


	ui_Length = m_t_LabelDisplay[ui_Item].ui_Length;
	ui_Offset = m_t_LabelDisplay[ui_Item].ui_Offset;

	while (ui_Length > 0)
	{
		DrvLCD_SetConfig(ui_Offset, DRV_LCD_PARAM_BLINK, 
			(const uint8 *)&ui_Blink);
		ui_Length--;
		ui_Offset++;
	}
}


static void TaskSetting_DisplayLabel
(
	uint ui_Item,
	uint16 u16_Value
)
{
	uint ui_Value;
	uint ui_Length;
	uint ui_Offset;
	const task_setting_display *tp_Display;


	if ((ui_Item == TASK_SETTING_ITEM_HOUR) ||
		(ui_Item == TASK_SETTING_ITEM_ALARM_HOUR))
	{
		TaskDevice_GetConfig(TASK_DEVICE_PARAM_TIME_FORMAT, (uint8 *)&ui_Value,
			(uint *)0);
		TaskDevice_DisplayHour((uint8)u16_Value, (uint8)ui_Value);
		return;
	}

	if (ui_Item == TASK_SETTING_ITEM_TIME_FORMAT)
	{
		TaskDevice_DisplayHour(m_u16_Value[TASK_SETTING_ITEM_HOUR], u16_Value);
		return;
	}

	if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
		(ui_Item == TASK_SETTING_ITEM_HYPER))
	{
		if (u16_Value != 0)
		{
			u16_Value = 1;
		}
	}
	else if (ui_Item == TASK_SETTING_ITEM_YEAR)
	{
		m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
		m_u8_DisplayBuffer[1] = DRV_LCD_SYMBOL_OFF;
		DrvLCD_Write(m_t_LabelDisplay[TASK_SETTING_ITEM_TIME_FORMAT].ui_Offset,
			m_u8_DisplayBuffer, 
			m_t_LabelDisplay[TASK_SETTING_ITEM_TIME_FORMAT].ui_Length);
	}
	else if (ui_Item == TASK_SETTING_ITEM_AUDIO)
	{
		if (u16_Value == 0)
		{
			u16_Value = 1;
		}
		else
		{
			u16_Value = 0;
		}
	}

	ui_Length = sizeof(m_u8_DisplayBuffer);
	TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, u16_Value);
	tp_Display = &m_t_LabelDisplay[ui_Item];

	if (ui_Length <= tp_Display->ui_Length)
	{
		ui_Offset = tp_Display->ui_Offset;

		if (ui_Item == TASK_SETTING_ITEM_DAY)
		{
			DrvLCD_Write(ui_Offset, m_u8_DisplayBuffer, ui_Length);
			ui_Offset++;
		}
		else
		{
			DrvLCD_Write(ui_Offset + tp_Display->ui_Length - ui_Length, 
				m_u8_DisplayBuffer, ui_Length);
		}

		ui_Length = tp_Display->ui_Length - ui_Length;

		if ((ui_Item == TASK_SETTING_ITEM_YEAR) ||
			(ui_Item == TASK_SETTING_ITEM_MINUTE) ||
			(ui_Item == TASK_SETTING_ITEM_ALARM_MINUTE))
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_0;
		}
		else
		{
			m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;
		}

		while (ui_Length > 0)
		{
			DrvLCD_Write(ui_Offset, m_u8_DisplayBuffer, 1);
			ui_Length--;
			ui_Offset++;
		}
	}
}


static void TaskSetting_DisplayValue
(
	uint ui_Item,
	uint16 u16_Value
)
{
	uint ui_Length;
	uint ui_Offset;


	switch (ui_Item)
	{
		case TASK_SETTING_ITEM_HYPO:
		case TASK_SETTING_ITEM_HYPER:

			if (u16_Value == 0)
			{
				ui_Length = sizeof(m_u8_TokenSwitchOff);
				Drv_Memcpy(m_u8_DisplayBuffer, m_u8_TokenSwitchOff, 
					sizeof(m_u8_TokenSwitchOff));
			}
			else
			{
				ui_Length = sizeof(m_u8_DisplayBuffer);
				TaskDevice_NumberToBCD(m_u8_DisplayBuffer, &ui_Length, u16_Value);
			}

			break;

		case TASK_SETTING_ITEM_TIME_FORMAT:

			if (u16_Value == m_tp_Limit[ui_Item]->u16_LowerLimit)
			{
				ui_Length = sizeof(m_u8_Token24Hour);
				Drv_Memcpy(m_u8_DisplayBuffer, m_u8_Token24Hour, 
					sizeof(m_u8_Token24Hour));
			}
			else
			{
				ui_Length = sizeof(m_u8_Token12Hour);
				Drv_Memcpy(m_u8_DisplayBuffer, m_u8_Token12Hour, 
					sizeof(m_u8_Token12Hour));
			}

			break;

		case TASK_SETTING_ITEM_AUDIO:
		case TASK_SETTING_ITEM_KETONE:
		case TASK_SETTING_ITEM_BLUETOOTH:
		case TASK_SETTING_ITEM_MEAL:
		case TASK_SETTING_ITEM_ALARM_SWITCH:

			if (u16_Value == m_tp_Limit[ui_Item]->u16_LowerLimit)
			{
				ui_Length = sizeof(m_u8_TokenSwitchOff);
				Drv_Memcpy(m_u8_DisplayBuffer, m_u8_TokenSwitchOff, 
					sizeof(m_u8_TokenSwitchOff));
			}
			else
			{
				ui_Length = sizeof(m_u8_TokenSwitchOn);
				Drv_Memcpy(m_u8_DisplayBuffer, m_u8_TokenSwitchOn, 
					sizeof(m_u8_TokenSwitchOn));
			}

			break;

		default:
			ui_Length = 0;
			break;
	}

	if (ui_Length <= m_t_ValueDisplay.ui_Length)
	{
		ui_Offset = m_t_ValueDisplay.ui_Offset;
		DrvLCD_Write(ui_Offset + m_t_ValueDisplay.ui_Length - ui_Length, 
			m_u8_DisplayBuffer, ui_Length);

		ui_Length = m_t_ValueDisplay.ui_Length - ui_Length;
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_ALL_OFF;

		while (ui_Length > 0)
		{
			DrvLCD_Write(ui_Offset, m_u8_DisplayBuffer, 1);
			ui_Length--;
			ui_Offset++;
		}
	}
}


static void TaskSetting_DisplayItem
(
	uint ui_Item,
	uint16 u16_Value
)
{
	uint i;


	TaskSetting_DisplayLabel(ui_Item, u16_Value);

	if ((ui_Item == TASK_SETTING_ITEM_HOUR) ||
		(ui_Item == TASK_SETTING_ITEM_ALARM_HOUR))
	{
		if (m_u16_Value[TASK_SETTING_ITEM_TIME_FORMAT] != 0)
		{
			if (u16_Value == 0)
			{
				u16_Value = 12;
			}
			else if (u16_Value > 12)
			{
				u16_Value -= 12;
			}
		}
	}

	if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
		(ui_Item == TASK_SETTING_ITEM_HYPER))
	{
		if (m_u16_Value[ui_Item] != 0)
		{
			TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_ON);
		}
		else
		{
			TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_OFF);
		}

	}

	TaskSetting_DisplayValue(ui_Item, u16_Value);

	if (ui_Item == TASK_SETTING_ITEM_ALARM_ID)
	{
		for (i = TASK_SETTING_ITEM_ALARM_HOUR; 
			i <= TASK_SETTING_ITEM_ALARM_SWITCH; i++)
		{
			TaskSetting_LoadSetting(i);
			TaskSetting_DisplayLabel(i, m_u16_Value[i]);
		}
	}
}


static void TaskSetting_SelectItem
(
	uint ui_Item
)
{
	if (ui_Item == TASK_SETTING_ITEM_YEAR)
	{
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_2;
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_TENS, &m_u8_DisplayBuffer[0], 
			sizeof(m_u8_DisplayBuffer[0]));
		m_u8_DisplayBuffer[0] = DRV_LCD_CHARACTER_0;
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_UNITS, &m_u8_DisplayBuffer[0], 
			sizeof(m_u8_DisplayBuffer[0]));
		m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_TIME_COLON, &m_u8_DisplayBuffer[0], 
			sizeof(m_u8_DisplayBuffer[0]));
	}

	TaskSetting_DisplayItem(ui_Item, m_u16_Value[ui_Item]);
	TaskSetting_SetLabelBlink(ui_Item, TASK_SETTING_BLINK_ON);
}


static void TaskSetting_DeselectItem
(
	uint ui_Item
)
{
	if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
		(ui_Item == TASK_SETTING_ITEM_HYPER))
	{
		if (m_u16_Value[ui_Item] != 0)
		{
			TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_OFF);
		}
	}

	if (ui_Item == TASK_SETTING_ITEM_YEAR)
	{
		TaskSetting_DisplayLabel(TASK_SETTING_ITEM_HOUR, 
			m_u16_Value[TASK_SETTING_ITEM_HOUR]);
		TaskSetting_DisplayLabel(TASK_SETTING_ITEM_MINUTE, 
			m_u16_Value[TASK_SETTING_ITEM_MINUTE]);
		m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_ON;
		DrvLCD_Write(DRV_LCD_OFFSET_TIME_COLON, &m_u8_DisplayBuffer[0], 
			sizeof(m_u8_DisplayBuffer[0]));
	}
	else if (ui_Item == TASK_SETTING_ITEM_AUDIO)
	{
		TaskSetting_DisplayLabel(ui_Item, 1);
	}
	else if ((ui_Item >= TASK_SETTING_ITEM_HYPO) && 
			(ui_Item <= TASK_SETTING_ITEM_MEAL))
	{
		TaskSetting_DisplayLabel(ui_Item, 0);
	}
	else
	{
		TaskSetting_DisplayLabel(ui_Item, m_u16_Value[ui_Item]);
	}

	TaskSetting_SetLabelBlink(ui_Item, TASK_SETTING_BLINK_OFF);
}


static void TaskSetting_ResetAllLabels
(
	uint ui_FirstItem,
	uint ui_LastItem
)
{
	uint i;
	uint ui_Value;


	TaskDevice_GetConfig(TASK_DEVICE_PARAM_GLUCOSE_UNIT, (uint8 *)&ui_Value,
		(uint *)0);

	if (ui_Value == 0)
	{
		REG_SET_BIT(m_ui_Flag, TASK_SETTING_FLAG_MMOL);
	}
	else
	{
		REG_CLEAR_BIT(m_ui_Flag, TASK_SETTING_FLAG_MMOL);
	}

	Bluetooth_GetConfig(BLUETOOTH_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_Value,
		(uint *)0);

	if (ui_Value == 0)
	{
		REG_SET_BIT(m_ui_Flag, TASK_SETTING_FLAG_BLUETOOTH);
	}
	else
	{
		REG_CLEAR_BIT(m_ui_Flag, TASK_SETTING_FLAG_BLUETOOTH);
	}

	if (ui_FirstItem == TASK_SETTING_ITEM_ALARM_ID)
	{
		m_u8_DisplayBuffer[0] = DRV_LCD_SYMBOL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_DATE_BAR, &m_u8_DisplayBuffer[0], 
			sizeof(m_u8_DisplayBuffer[0]));
		TaskSetting_LoadSetting(TASK_SETTING_ITEM_TIME_FORMAT);
	}

	for (i = ui_FirstItem; i <= ui_LastItem; i++)
	{
		if (i == TASK_SETTING_ITEM_ALARM_ID)
		{
			m_u16_Value[i] = 1;
		}
		else
		{
			TaskSetting_LoadSetting(i);
		}

		if (i == TASK_SETTING_ITEM_YEAR)
		{
			if (TaskSetting_CheckLeapYear(m_u16_Value[i]) == FUNCTION_OK)
			{
				REG_SET_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR);
			}
			else
			{
				REG_CLEAR_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR);
			}
		}
		else if (i == TASK_SETTING_ITEM_AUDIO)
		{
			TaskSetting_DisplayLabel(i, 1);
		}
		else if ((i >= TASK_SETTING_ITEM_HYPO) &&
			(i <= TASK_SETTING_ITEM_MEAL))
		{
			TaskSetting_DisplayLabel(i, 0);
		}
		else
		{
			TaskSetting_DisplayLabel(i, m_u16_Value[i]);
		}
	}
}


static void TaskSetting_ProcessSelect
(
	uint ui_FirstItem,
	uint ui_LastItem
)
{
	uint ui_ButtonState;


	ui_ButtonState = Button_Read(BUTTON_ID_CENTER);

	if ((ui_ButtonState == BUTTON_STATE_PRESS) && 
		(m_ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_RELEASE))
	{
		TaskSetting_DeselectItem(m_ui_CurrentItem);
		TaskSetting_SaveSetting(m_ui_CurrentItem);

		if ((m_ui_CurrentItem == TASK_SETTING_ITEM_KETONE) && 
			(REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_BLUETOOTH) == 0))
		{
			m_ui_CurrentItem += 2;
		}
		else
		{
			m_ui_CurrentItem++;
		}
		
		if (m_ui_CurrentItem <= ui_LastItem)
		{
			TaskSetting_SelectItem(m_ui_CurrentItem);
		}
	}

	m_ui_ButtonState[BUTTON_ID_CENTER] = ui_ButtonState;
}


static void TaskSetting_ProcessValueChange
(
	uint ui_ButtonID,
	uint ui_Item
)
{
	uint ui_ButtonState;
	const task_setting_limit *tp_Limit;


	ui_ButtonState = Button_Read(ui_ButtonID);

	if ((ui_ButtonState == BUTTON_STATE_LONG_PRESS) ||
		((ui_ButtonState == BUTTON_STATE_PRESS) && 
		(m_ui_ButtonState[ui_ButtonID] == BUTTON_STATE_RELEASE)))
	{
		if (((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER)) &&
			(REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_MMOL) != 0))
		{
				tp_Limit = m_tp_Limit[ui_Item] + 1;
		}
		else if (ui_Item == TASK_SETTING_ITEM_DAY)
		{
			//Check if it's February and leap year
			if ((m_u16_Value[TASK_SETTING_ITEM_MONTH] == MONTH_FEBRUARY) &&
				(REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR) != 0))
			{
				tp_Limit = m_tp_Limit[TASK_SETTING_ITEM_DAY] + 
					m_t_MonthLimit->u16_UpperLimit;
			}
			else
			{
				tp_Limit = m_tp_Limit[TASK_SETTING_ITEM_DAY] + 
					m_u16_Value[TASK_SETTING_ITEM_MONTH] - 1;
			}
		}
		else
		{
			tp_Limit = m_tp_Limit[ui_Item];
		}

		if (ui_ButtonID == BUTTON_ID_LEFT)
		{
			if (m_u16_Value[ui_Item] > tp_Limit->u16_LowerLimit)
			{
				m_u16_Value[ui_Item]--;
			}
			else
			{
				m_u16_Value[ui_Item] = tp_Limit->u16_UpperLimit;
			}
		}
		
		if (ui_ButtonID == BUTTON_ID_RIGHT)
		{
			if (m_u16_Value[ui_Item] <  tp_Limit->u16_UpperLimit)
			{
				if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
					(ui_Item == TASK_SETTING_ITEM_HYPER))
				{
					if (m_u16_Value[ui_Item] == 0)
					{
						m_u16_Value[ui_Item] = tp_Limit->u16_LowerLimit;
					}
				}

				m_u16_Value[ui_Item]++;
			}
			else
			{
				m_u16_Value[ui_Item] = tp_Limit->u16_LowerLimit;
			}
		}

		if (ui_Item == TASK_SETTING_ITEM_MONTH)
		{	
			//Check if it's February and leap year
			if ((m_u16_Value[TASK_SETTING_ITEM_MONTH] == MONTH_FEBRUARY) &&
				(REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR) != 0))
			{
				tp_Limit = &m_t_DayLimit[m_t_MonthLimit->u16_UpperLimit];
			}
			else
			{
				tp_Limit = &m_t_DayLimit[m_u16_Value[TASK_SETTING_ITEM_MONTH] - 1];
			}

			//Check if the current day exceeds the upper day limit of current month
			if (m_u16_Value[TASK_SETTING_ITEM_DAY] > tp_Limit->u16_UpperLimit)
			{
				m_u16_Value[TASK_SETTING_ITEM_DAY] = tp_Limit->u16_UpperLimit;
				TaskSetting_DisplayLabel(TASK_SETTING_ITEM_DAY, 
					m_u16_Value[TASK_SETTING_ITEM_DAY]);
				TaskSetting_SaveSetting(TASK_SETTING_ITEM_DAY);
			}
		}

		if (ui_Item == TASK_SETTING_ITEM_YEAR)
		{
			if (TaskSetting_CheckLeapYear(m_u16_Value[TASK_SETTING_ITEM_YEAR]) == 
				FUNCTION_OK)
			{
				REG_SET_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR);
			}
			else
			{
				REG_CLEAR_BIT(m_ui_Flag, TASK_SETTING_FLAG_LEAP_YEAR);

				//Check if the current day is the last day of February
				if ((m_u16_Value[TASK_SETTING_ITEM_MONTH] == MONTH_FEBRUARY) &&
					(m_u16_Value[TASK_SETTING_ITEM_DAY] > 
					m_t_DayLimit[MONTH_FEBRUARY - 1].u16_UpperLimit))
				{
					m_u16_Value[TASK_SETTING_ITEM_DAY] = 
						m_t_DayLimit[MONTH_FEBRUARY - 1].u16_UpperLimit;
					TaskSetting_DisplayLabel(TASK_SETTING_ITEM_DAY, 
						m_u16_Value[TASK_SETTING_ITEM_DAY]);
					TaskSetting_SaveSetting(TASK_SETTING_ITEM_DAY);
				}
			}
		}

		if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER))
		{
			if (m_u16_Value[ui_Item] == tp_Limit->u16_LowerLimit)
			{
				m_u16_Value[ui_Item] = 0;
			}
		}

		TaskSetting_DisplayItem(ui_Item, m_u16_Value[ui_Item]);
	}

	if ((ui_ButtonState == BUTTON_STATE_LONG_PRESS) && 
		(m_ui_ButtonState[ui_ButtonID] == BUTTON_STATE_PRESS))
	{
		TaskSetting_SetLabelBlink(ui_Item, TASK_SETTING_BLINK_OFF);
	}		

	if ((ui_ButtonState == BUTTON_STATE_RELEASE) && 
		(m_ui_ButtonState[ui_ButtonID] == BUTTON_STATE_LONG_PRESS))
	{
		TaskSetting_SetLabelBlink(ui_Item, TASK_SETTING_BLINK_ON);
	}		

	m_ui_ButtonState[ui_ButtonID] = ui_ButtonState;
}


static void TaskSetting_ProcessInput(void)
{
	TaskSetting_ProcessValueChange(BUTTON_ID_LEFT, m_ui_CurrentItem);
	TaskSetting_ProcessValueChange(BUTTON_ID_RIGHT, m_ui_CurrentItem);
}


static void TaskSetting_SaveSetting
(
	uint ui_Item
)
{
	uint ui_Value;
	uint16 u16_Value;
	const task_setting_operation *tp_Operation;


	if (ui_Item != TASK_SETTING_ITEM_ALARM_ID)
	{
		if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER))
		{
			u16_Value = m_u16_Value[ui_Item];
		}
		else
		{
			ui_Value = (uint)m_u16_Value[ui_Item];
		}

		tp_Operation = &m_t_Operation[ui_Item];
		
		if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER))
		{
			if (REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_MMOL) != 0)
			{
				u16_Value = ((u16_Value * GLUCOSE_MG_TO_MMOL_DENOMINATOR) + 
					(GLUCOSE_MG_TO_MMOL_NUMERATOR / 2)) / 
					GLUCOSE_MG_TO_MMOL_NUMERATOR;
			}

			tp_Operation->fp_SaveSetting(tp_Operation->ui_Parameter, 
				(const uint8 *)&u16_Value, sizeof(u16_Value));
		}
		else
		{
			tp_Operation->fp_SaveSetting(tp_Operation->ui_Parameter, 
				(const uint8 *)&ui_Value, sizeof(ui_Value));

			if ((ui_Item == TASK_SETTING_ITEM_HOUR) ||
				(ui_Item == TASK_SETTING_ITEM_MINUTE))
			{
				TaskDevice_UpdateAlarm();
			}
		}
	}
}


static void TaskSetting_LoadSetting
(
	uint ui_Item
)
{
	uint ui_Value;
	uint16 u16_Value;
	const task_setting_operation *tp_Operation;


	if (ui_Item != TASK_SETTING_ITEM_ALARM_ID)
	{
		tp_Operation = &m_t_Operation[ui_Item];
		
		if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER))
		{
			tp_Operation->fp_LoadSetting(tp_Operation->ui_Parameter, 
				(uint8 *)&u16_Value, (uint *)0);
		}
		else
		{
			tp_Operation->fp_LoadSetting(tp_Operation->ui_Parameter, 
				(uint8 *)&ui_Value, (uint *)0);
		}

		if ((ui_Item == TASK_SETTING_ITEM_HYPO) ||
			(ui_Item == TASK_SETTING_ITEM_HYPER))
		{
			if (REG_GET_BIT(m_ui_Flag, TASK_SETTING_FLAG_MMOL) != 0)
			{
				u16_Value = ((u16_Value * GLUCOSE_MG_TO_MMOL_NUMERATOR) + 
					(GLUCOSE_MG_TO_MMOL_DENOMINATOR / 2)) / 
					GLUCOSE_MG_TO_MMOL_DENOMINATOR;
			}

			m_u16_Value[ui_Item] = u16_Value;
		}
		else
		{
			m_u16_Value[ui_Item] = (uint16)ui_Value;
		}
	}
}


static uint TaskSetting_SetAlarmConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return TaskDevice_SetAlarm((uint)m_u16_Value[TASK_SETTING_ITEM_ALARM_ID] - 1,
		ui_Parameter, *((const uint*)u8p_Value));
}


static uint TaskSetting_GetAlarmConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return TaskDevice_GetAlarm((uint)m_u16_Value[TASK_SETTING_ITEM_ALARM_ID] - 1,
		ui_Parameter, (uint*)u8p_Value);
}


static uint TaskSetting_GetDevice
(
	device *tp_Device
)
{
	uint16 u16_VersionMajor;
	uint16 u16_VersionMinor;


	tp_Device->u8_Endian = DEVICE_ENDIAN_BIG;
	tp_Device->u8_Type = DEVICE_TYPE_METER;
	tp_Device->u32_Capacity = TASK_GLUCOSE_HISTORY_ITEM_COUNT;
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_ID, tp_Device->u8_ID, (uint *)0);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_MODEL, (uint8 *)&tp_Device->u32_Model,
		(uint *)0);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_VERSION_MAJOR, 
		(uint8 *)&u16_VersionMajor, (uint *)0);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_VERSION_MINOR, 
		(uint8 *)&u16_VersionMinor, (uint *)0);
	tp_Device->u32_Version = ((uint32)u16_VersionMajor << 16) & u16_VersionMinor;

	return FUNCTION_OK;
}
