/*
 * Module:	History manager task
 * Author:	Lvjianfeng
 * Date:	2012.9
 */


#include "drv.h"
#include "button.h"
#include "glucose.h"

#include "task.h"
#include "task_device.h"
#include "task_glucose.h"
#include "task_history.h"


//Constant definition

#define TASK_HISTORY_CYCLE				100

#define LCD_DATA_LENGTH					3

#define DAY_PER_YEAR					365
#define MONTH_FEBRUARY					2

#define GLUCOSE_MG_TO_MMOL_NUMERATOR	10			
#define GLUCOSE_MG_TO_MMOL_DENOMINATOR	18			
#define MG_TO_MMOL(glucose)				(((((uint16)(glucose)) * \
										GLUCOSE_MG_TO_MMOL_NUMERATOR) + \
										(GLUCOSE_MG_TO_MMOL_DENOMINATOR / 2)) / \
										GLUCOSE_MG_TO_MMOL_DENOMINATOR)

#define HISTORY_STATUS_TEMPERATURE		0
#define HISTORY_STATUS_GLUCOSE			0
#define HISTORY_STATUS_FLAG				1


//Type definition

typedef enum
{
	TASK_HISTORY_STATE_HISTORY = 0,
	TASK_HISTORY_STATE_AVERAGE,
	TASK_HISTORY_STATE_RESET,
	TASK_HISTORY_STATE_QUIT,
	TASK_HISTORY_COUNT_STATE
} task_history_state;

typedef enum
{
	TASK_HISTORY_AVERAGE_MODE_ALL = 0,
	TASK_HISTORY_AVERAGE_MODE_BEFORE_MEAL,
	TASK_HISTORY_AVERAGE_MODE_AFTER_MEAL,
	TASK_HISTORY_COUNT_AVERAGE_MODE
} task_history_average_model;

typedef enum
{
	TASK_HISTORY_AVERAGE_DAY_7 = 0,
	TASK_HISTORY_AVERAGE_DAY_14,
	TASK_HISTORY_AVERAGE_DAY_30,
	TASK_HISTORY_AVERAGE_DAY_60,
	TASK_HISTORY_AVERAGE_DAY_90,
	TASK_HISTORY_COUNT_AVERAGE_DAY
} task_history_average_day;

typedef enum
{
	TASK_HISTORY_EVENT_GLUCOSE = 0,
	TASK_HISTORY_COUNT_EVENT
} task_history_event_type;


//Private variable definition

static uint m_ui_CurrentState = {0};
static uint m_ui_PreviousState = {0};
static uint m_ui_AverageMode = {0};
static uint m_ui_GlucoseUnit = {0};
static uint16 m_u16_LastItem = {0};
static uint16 m_u16_CurrentItem = {0};
static uint16 m_u16_HistoryCount = {0};
static uint m_ui_ButtonState[BUTTON_COUNT_ID] = {0};

static const uint16 m_u16_AverageDay[TASK_HISTORY_COUNT_AVERAGE_DAY] = 
{ 
	7, 14, 30, 60, 90
};
static const uint8 m_u8_DayPerMonth[] = 
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static const uint8 m_u8_ResetToken[] =
{
	DRV_LCD_CHARACTER_D,
	DRV_LCD_CHARACTER_E,
	DRV_LCD_CHARACTER_L
};


//Private function declaration

static uint16 TaskHistory_DateToDayCount
(
	uint8 u8_Year,
	uint8 u8_Month,
	uint8 u8_Day
);
static void TaskHistory_InitializeHistoryDisplay(void);
static void TaskHistory_InitializeAverageDisplay(void);
static void TaskHistory_DisplayVoid(void);
static void TaskHistory_DisplayReset(void);
static void TaskHistory_DisplayMeal(void);
static void TaskHistory_DisplayFlag
(
	uint16 u16_Flag
);
static void TaskHistory_DisplayReading
(
	uint16 u16_Reading
);
static void TaskHistory_DisplayHistory
(
	uint16 u16_Item
);
static void TaskHistory_DisplayAverage
(
	uint16 u16_Item
);
static void TaskHistory_DisplayItem
(
	uint16 u16_Item
);
static void TaskHistory_ProcessBrowse(void);
static void TaskHistory_ProcessMode(void);
static void TaskHistory_CancelReset(void);
static uint TaskHistory_CheckQuitButton(void);
static uint TaskHistory_ReadHistory
(
	uint16 u16_Index,
	history *tp_History
);


//Public function definition

uint TaskHistory_Initialize
(
	devos_task_handle t_TaskHandle
)
{

	return FUNCTION_OK;
}


void TaskHistory_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint8 u8_MessageData;
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;


	DEVOS_TASK_BEGIN

	DevOS_MessageWait(TASK_MESSAGE_ID_HISTORY, u8p_MessageData, 
		&t_MessageLength); 
	DevOS_MessageClear(TASK_MESSAGE_ID_SYSTEM, t_TaskHandle);
	TaskGlucose_GetConfig(TASK_GLUCOSE_PARAM_HISTORY_COUNT, 
		(uint8 *)&m_u16_HistoryCount, (uint *)0);
	TaskDevice_GetConfig(TASK_DEVICE_PARAM_GLUCOSE_UNIT, 
		(uint8 *)&m_ui_GlucoseUnit, (uint *)0);
	TaskHistory_InitializeHistoryDisplay();
	m_ui_CurrentState = TASK_HISTORY_STATE_HISTORY;
	m_ui_AverageMode = TASK_HISTORY_AVERAGE_MODE_ALL;

	if (m_u16_HistoryCount == 0)
	{
		m_u16_LastItem = 0;
		TaskHistory_DisplayVoid();
	}
	else
	{
		m_u16_LastItem = m_u16_HistoryCount - 1;
		TaskHistory_DisplayItem(m_u16_LastItem);
	}

	m_u16_CurrentItem = m_u16_LastItem;

	while (Button_Read(BUTTON_ID_CENTER) != BUTTON_STATE_RELEASE)
	{
		DevOS_TaskDelay(TASK_HISTORY_CYCLE);
	}

	while (1)
	{
		u8p_MessageData = DevOS_MessageReceive(TASK_MESSAGE_ID_SYSTEM,
			&t_MessageLength);	

		if (((u8p_MessageData != (uint8 *)0) &&
			(*u8p_MessageData == TASK_MESSAGE_DATA_QUIT)) ||
			(TaskHistory_CheckQuitButton() != FUNCTION_OK) ||
			(m_ui_CurrentState == TASK_HISTORY_STATE_QUIT))
		{
			DevOS_MessageClear(TASK_MESSAGE_ID_HISTORY, t_TaskHandle);
			u8_MessageData = TASK_MESSAGE_DATA_SLEEP;
			DevOS_MessageSend(TASK_MESSAGE_ID_DEVICE, &u8_MessageData,
				sizeof(u8_MessageData));
			break;
		}

		if (m_u16_HistoryCount > 0)
		{
			TaskHistory_ProcessBrowse();
			TaskHistory_ProcessMode();
		}

		DevOS_TaskDelay(TASK_HISTORY_CYCLE);
	}

	DEVOS_TASK_END
}


uint TaskHistory_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{

	return FUNCTION_OK;
}


uint TaskHistory_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_HISTORY_PARAM_DATE_TIME:
			break;

		case TASK_HISTORY_PARAM_HISTORY:

			if (uip_Length != (uint *)0)
			{
				*uip_Length = sizeof(history);
			}

			return TaskHistory_ReadHistory(*((uint16 *)u8p_Value),
					(history *)u8p_Value);

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


#if TASK_HISTORY_TEST_ENABLE == 1

void TaskHistory_Test(void)
{
}

#endif


//Private function definition

static uint16 TaskHistory_DateToDayCount
(
	uint8 u8_Year,
	uint8 u8_Month,
	uint8 u8_Day
)
{
	uint8 i;
	uint16 u16_DayCount;


	u16_DayCount = (uint16)u8_Year * DAY_PER_YEAR;

	for (i = 0; i < (u8_Month - 1); i++)
	{
		u16_DayCount += (uint16)m_u8_DayPerMonth[i];
	}

	u16_DayCount += (uint16)u8_Day - 1;

	//Add extra day for leap year
	u16_DayCount += ((uint16)u8_Year + 3) / 4;

	if ((u8_Month > MONTH_FEBRUARY) && ((u8_Year % 4) == 0))
	{
		u16_DayCount++;
	}	

	return u16_DayCount;
}


static void TaskHistory_InitializeHistoryDisplay(void)
{
	uint8 u8_LCDData;


	TaskDevice_InitializeDisplay();

	u8_LCDData = DRV_LCD_SYMBOL_ON;
	DrvLCD_Write(DRV_LCD_OFFSET_HISTORY, &u8_LCDData, sizeof(u8_LCDData));
	TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_ON);
}


static void TaskHistory_InitializeAverageDisplay(void)
{
	uint8 u8_LCDData;


	TaskDevice_InitializeDisplay();

	u8_LCDData = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_TIME_COLON, &u8_LCDData, sizeof(u8_LCDData));
	DrvLCD_Write(DRV_LCD_OFFSET_DATE_BAR, &u8_LCDData, sizeof(u8_LCDData));
	u8_LCDData = DRV_LCD_SYMBOL_ON;
	DrvLCD_Write(DRV_LCD_OFFSET_DAY_AVG, &u8_LCDData, sizeof(u8_LCDData));
	DrvLCD_Write(DRV_LCD_OFFSET_READINGS, &u8_LCDData, sizeof(u8_LCDData));
}


static void TaskHistory_DisplayVoid(void)
{
	uint8 u8_LCDData[LCD_DATA_LENGTH];


	u8_LCDData[0] = DRV_LCD_CHARACTER_BAR;
	u8_LCDData[1] = DRV_LCD_CHARACTER_BAR;
	u8_LCDData[2] = DRV_LCD_CHARACTER_BAR;

	DrvLCD_Write(DRV_LCD_OFFSET_MONTH_UNITS, u8_LCDData, 1);
	DrvLCD_Write(DRV_LCD_OFFSET_DAY_TENS, u8_LCDData, 2);
	DrvLCD_Write(DRV_LCD_OFFSET_HOUR_TENS, u8_LCDData, 2);
	DrvLCD_Write(DRV_LCD_OFFSET_MINUTE_TENS, u8_LCDData, 2);
	DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, u8_LCDData, sizeof(u8_LCDData));
	TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_OFF);
}


static void TaskHistory_DisplayReset(void)
{
	uint ui_Value;


	TaskHistory_InitializeHistoryDisplay();
	TaskHistory_DisplayVoid();
	DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, m_u8_ResetToken, 
		sizeof(m_u8_ResetToken));
	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_GLUCOSE_TENS, DRV_LCD_PARAM_BLINK,
		(const uint8 *)&ui_Value);
	DrvLCD_SetConfig(DRV_LCD_OFFSET_GLUCOSE_UNITS, DRV_LCD_PARAM_BLINK,
		(const uint8 *)&ui_Value);
	DrvLCD_SetConfig(DRV_LCD_OFFSET_GLUCOSE_FRACTION, DRV_LCD_PARAM_BLINK,
		(const uint8 *)&ui_Value);
}


static void TaskHistory_DisplayMeal(void)
{
	uint8 u8_LCDData;


	u8_LCDData = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_BEFORE_MEAL, &u8_LCDData, sizeof(u8_LCDData));
	DrvLCD_Write(DRV_LCD_OFFSET_AFTER_MEAL, &u8_LCDData, sizeof(u8_LCDData));

	u8_LCDData = DRV_LCD_SYMBOL_ON;

	if (m_ui_AverageMode == TASK_HISTORY_AVERAGE_MODE_BEFORE_MEAL)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_BEFORE_MEAL, &u8_LCDData, 
			sizeof(u8_LCDData));
	}
	else if (m_ui_AverageMode == TASK_HISTORY_AVERAGE_MODE_AFTER_MEAL)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_AFTER_MEAL, &u8_LCDData, 
			sizeof(u8_LCDData));
	}
}


static void TaskHistory_DisplayFlag
(
	uint16 u16_Flag
)
{
	uint8 u8_LCDData;


	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_HYPO) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_HYPO, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_HYPER) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_HYPER, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_KETONE) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_KETONE, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_BEFORE_MEAL, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_AFTER_MEAL, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(u16_Flag, TASK_GLUCOSE_FLAG_POUND) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_POUND, &u8_LCDData, sizeof(u8_LCDData));
}


static void TaskHistory_DisplayReading
(
	uint16 u16_Reading
)
{
	uint i;
	uint ui_Length;
	uint8 u8_LCDData[LCD_DATA_LENGTH];


	for (i = 0; i < sizeof(u8_LCDData); i++)
	{
		u8_LCDData[i] = DRV_LCD_CHARACTER_ALL_OFF;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_HOUR_UNITS, u8_LCDData, sizeof(u8_LCDData));
	ui_Length = sizeof(u8_LCDData);
	TaskDevice_NumberToBCD(u8_LCDData, &ui_Length, u16_Reading);

	if (ui_Length < sizeof(u8_LCDData))
	{
		DrvLCD_Write(DRV_LCD_OFFSET_MINUTE_TENS, u8_LCDData, ui_Length);
	}
	else
	{
		DrvLCD_Write(DRV_LCD_OFFSET_HOUR_UNITS, u8_LCDData, sizeof(u8_LCDData));
	}
}


static void TaskHistory_DisplayHistory
(
	uint16 u16_Item
)
{
	uint ui_Value;
	task_glucose_history_item t_HistoryItem;
	

	if (TaskGlucose_ReadHistory(u16_Item + 1, 0, &t_HistoryItem) == FUNCTION_OK)
	{
		TaskDevice_GetConfig(TASK_DEVICE_PARAM_TIME_FORMAT, (uint8 *)&ui_Value,
			(uint *)0);

		TaskDevice_DisplayMonth(t_HistoryItem.t_DateTime.u8_Month);
		TaskDevice_DisplayDay(t_HistoryItem.t_DateTime.u8_Day);
		TaskDevice_DisplayHour(t_HistoryItem.t_DateTime.u8_Hour, (uint8)ui_Value);
		TaskDevice_DisplayMinute(t_HistoryItem.t_DateTime.u8_Minute);

		if (m_ui_GlucoseUnit == 0)
		{
			TaskDevice_DisplayGlucose(MG_TO_MMOL(t_HistoryItem.u16_Glucose));
		}
		else
		{
			TaskDevice_DisplayGlucose(t_HistoryItem.u16_Glucose);
		}

		TaskHistory_DisplayFlag(t_HistoryItem.u16_Flag);
	}
}


static void TaskHistory_DisplayAverage
(
	uint16 u16_Item
)
{
	uint ui_Value;
	uint8 u8_LCDData[LCD_DATA_LENGTH];
	uint16 u16_DayCount;
	uint16 u16_DayCountBegin;
	uint16 u16_DayCountEnd;
	uint16 u16_HistoryIndex;
	uint16 u16_ReadingCount;
	uint32 u32_GlucoseAverage;
	task_glucose_history_item t_HistoryItem;


	//Read current date
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_YEAR, (uint8 *)&ui_Value,
		(uint *)0);
	u8_LCDData[0] = (uint8)ui_Value;
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MONTH, (uint8 *)&ui_Value,
		(uint *)0);
	u8_LCDData[1] = (uint8)ui_Value;
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_DAY, (uint8 *)&ui_Value,
		(uint *)0);
	u8_LCDData[2] = (uint8)ui_Value;

	//Calculate the number of days from year 2000
	u16_DayCountEnd = TaskHistory_DateToDayCount(u8_LCDData[0], u8_LCDData[1],
		u8_LCDData[2]);

	//Check if there are enough days from year 2000 to calculate average
	if (u16_DayCountEnd >= m_u16_AverageDay[u16_Item] - 1)
	{
		u16_DayCountBegin = u16_DayCountEnd - m_u16_AverageDay[u16_Item] + 1;
	}
	else
	{
		u16_DayCountBegin = u16_DayCountEnd + 1;
	}
	
	u16_HistoryIndex = m_u16_HistoryCount;
	u16_ReadingCount = 0;
	u32_GlucoseAverage = 0;

	while (u16_HistoryIndex > 0)
	{
		TaskGlucose_ReadHistory(u16_HistoryIndex, 0, &t_HistoryItem);
		u16_DayCount = TaskHistory_DateToDayCount(t_HistoryItem.t_DateTime.u8_Year,
			t_HistoryItem.t_DateTime.u8_Month, t_HistoryItem.t_DateTime.u8_Day);

		if ((REG_GET_BIT(t_HistoryItem.u16_Flag, 
			TASK_GLUCOSE_FLAG_POUND) == 0) && 
			(u16_DayCount >= u16_DayCountBegin) && 
			(u16_DayCount <= u16_DayCountEnd))
		{
			if (m_ui_AverageMode == TASK_HISTORY_AVERAGE_MODE_BEFORE_MEAL)
			{
				if (REG_GET_BIT(t_HistoryItem.u16_Flag, 
					TASK_GLUCOSE_FLAG_BEFORE_MEAL) != 0)
				{
					u32_GlucoseAverage += (uint32)t_HistoryItem.u16_Glucose;
					u16_ReadingCount++;
				}
			}
			else if (m_ui_AverageMode == TASK_HISTORY_AVERAGE_MODE_AFTER_MEAL)
			{
				if (REG_GET_BIT(t_HistoryItem.u16_Flag, 
					TASK_GLUCOSE_FLAG_AFTER_MEAL) != 0)
				{
					u32_GlucoseAverage += (uint32)t_HistoryItem.u16_Glucose;
					u16_ReadingCount++;
				}
			}
			else
			{
				u32_GlucoseAverage += (uint32)t_HistoryItem.u16_Glucose;
				u16_ReadingCount++;
			}
		}

		u16_HistoryIndex--;
	}

	if (u16_ReadingCount > 0)
	{
		u32_GlucoseAverage /= (uint32)u16_ReadingCount;

		if (m_ui_GlucoseUnit == 0)
		{
			TaskDevice_DisplayGlucose(MG_TO_MMOL((uint16)u32_GlucoseAverage));
		}
		else
		{
			TaskDevice_DisplayGlucose((uint16)u32_GlucoseAverage);
		}

		TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_ON);
	}
	else
	{
		u8_LCDData[0] = DRV_LCD_CHARACTER_BAR;
		u8_LCDData[1] = DRV_LCD_CHARACTER_BAR;
		u8_LCDData[2] = DRV_LCD_CHARACTER_BAR;
		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, u8_LCDData, sizeof(u8_LCDData));
		TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_OFF);
	}

	TaskHistory_DisplayReading(u16_ReadingCount);
	TaskDevice_DisplayDay(m_u16_AverageDay[u16_Item]);
}


static void TaskHistory_DisplayItem
(
	uint16 u16_Item
)
{
	if (m_ui_CurrentState == TASK_HISTORY_STATE_HISTORY)
	{
		TaskHistory_DisplayHistory(u16_Item);
	}
	else if (m_ui_CurrentState == TASK_HISTORY_STATE_AVERAGE)
	{
		TaskHistory_DisplayAverage(u16_Item);
	}
}


static void TaskHistory_ProcessBrowse(void)
{
	uint ui_ButtonState;


	ui_ButtonState = Button_Read(BUTTON_ID_LEFT);

	if (m_ui_CurrentState == TASK_HISTORY_STATE_RESET)
	{
		if ((ui_ButtonState == BUTTON_STATE_PRESS) && 
			(m_ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_RELEASE))
		{
			TaskHistory_CancelReset();
		}
	}
	else
	{
		if ((ui_ButtonState == BUTTON_STATE_LONG_PRESS) || 
			((ui_ButtonState == BUTTON_STATE_PRESS) && 
			(m_ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_RELEASE)))
		{
			if (m_u16_CurrentItem > 0)
			{
				m_u16_CurrentItem--;
			}
			else
			{
				m_u16_CurrentItem = m_u16_LastItem;
			}

			TaskHistory_DisplayItem(m_u16_CurrentItem);
		}
	}

	m_ui_ButtonState[BUTTON_ID_LEFT] = ui_ButtonState;

	ui_ButtonState = Button_Read(BUTTON_ID_RIGHT);

	if (m_ui_CurrentState == TASK_HISTORY_STATE_RESET)
	{
		if ((ui_ButtonState == BUTTON_STATE_PRESS) && 
			(m_ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_RELEASE))
		{
			TaskHistory_CancelReset();
		}
	}
	else
	{
		if ((ui_ButtonState == BUTTON_STATE_LONG_PRESS) || 
			((ui_ButtonState == BUTTON_STATE_PRESS) && 
			(m_ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_RELEASE)))
		{
			if (m_u16_CurrentItem < m_u16_LastItem)
			{
				m_u16_CurrentItem++;
			}
			else
			{
				m_u16_CurrentItem = 0;
			}

			TaskHistory_DisplayItem(m_u16_CurrentItem);
		}
	}

	m_ui_ButtonState[BUTTON_ID_RIGHT] = ui_ButtonState;
}


static void TaskHistory_ProcessMode(void)
{
	uint ui_ButtonState;


	ui_ButtonState = Button_Read(BUTTON_ID_CENTER);

	if ((ui_ButtonState == BUTTON_STATE_PRESS) && 
		(m_ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_RELEASE))
	{
		if (m_ui_CurrentState == TASK_HISTORY_STATE_HISTORY)
		{
			m_u16_CurrentItem = TASK_HISTORY_AVERAGE_DAY_7;
			m_u16_LastItem = TASK_HISTORY_AVERAGE_DAY_90;
			m_ui_CurrentState = TASK_HISTORY_STATE_AVERAGE;
			TaskHistory_InitializeAverageDisplay();
		}
		else if (m_ui_CurrentState == TASK_HISTORY_STATE_AVERAGE)
		{
			m_ui_AverageMode++;

			if (m_ui_AverageMode >= TASK_HISTORY_COUNT_AVERAGE_MODE)
			{
				m_ui_CurrentState = TASK_HISTORY_STATE_QUIT;
			}

			TaskHistory_DisplayMeal();
		}
		else if (m_ui_CurrentState == TASK_HISTORY_STATE_RESET)
		{
			TaskGlucose_ResetHistory();
			m_u16_HistoryCount = 0;
			TaskHistory_InitializeHistoryDisplay();
			TaskHistory_DisplayVoid();
		}

		TaskHistory_DisplayItem(m_u16_CurrentItem);
	}

	m_ui_ButtonState[BUTTON_ID_CENTER] = ui_ButtonState;

	if ((m_ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_LONG_PRESS) &&
		(m_ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_LONG_PRESS))
	{
		if (m_ui_CurrentState != TASK_HISTORY_STATE_RESET)
		{
			TaskHistory_DisplayReset();
			m_ui_PreviousState = m_ui_CurrentState;
			m_ui_CurrentState = TASK_HISTORY_STATE_RESET;
		}
	}
}


static void TaskHistory_CancelReset(void)
{
	m_ui_CurrentState = m_ui_PreviousState;
	
	if (m_ui_CurrentState == TASK_HISTORY_STATE_HISTORY)
	{
		TaskHistory_InitializeHistoryDisplay();
	}
	else if (m_ui_CurrentState == TASK_HISTORY_STATE_AVERAGE)
	{
		TaskHistory_InitializeAverageDisplay();
		TaskHistory_DisplayMeal();
	}

	TaskHistory_DisplayItem(m_u16_CurrentItem);
}


static uint TaskHistory_CheckQuitButton(void)
{
	if (m_u16_HistoryCount == 0)
	{
		if ((Button_Read(BUTTON_ID_CENTER) != BUTTON_STATE_RELEASE) ||
			(Button_Read(BUTTON_ID_LEFT) != BUTTON_STATE_RELEASE) ||
			(Button_Read(BUTTON_ID_RIGHT) != BUTTON_STATE_RELEASE))
		{
			return FUNCTION_FAIL;
		}
	}

	return FUNCTION_OK;
}


static uint TaskHistory_ReadHistory
(
	uint16 u16_Index,
	history *tp_History
)
{
	task_glucose_history_item t_HistoryItem;


	if (TaskGlucose_ReadHistory(u16_Index, 1, &t_HistoryItem) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	Drv_Memset((uint8 *)tp_History, 0, sizeof(*tp_History));
	Drv_Memcpy((uint8 *)&tp_History->t_DateTime, 
		(const uint8 *)&t_HistoryItem.t_DateTime, 
		sizeof(t_HistoryItem.t_DateTime));
	tp_History->t_Status.u8_Parameter[HISTORY_STATUS_TEMPERATURE] = 
		(uint8)Glucose_Round((sint32)t_HistoryItem.u16_Temperature, 
		GLUCOSE_TEMPERATURE_RATIO);
	tp_History->t_Status.u8_Parameter[HISTORY_STATUS_FLAG] = 
		(uint8)t_HistoryItem.u16_Flag;
	tp_History->t_Status.u16_Parameter[HISTORY_STATUS_GLUCOSE] =
		t_HistoryItem.u16_Glucose;
	tp_History->t_Event.u16_Index = t_HistoryItem.u16_Index;
	tp_History->t_Event.u8_Port = TASK_PORT_GLUCOSE;
	tp_History->t_Event.u8_Type = TASK_HISTORY_EVENT_GLUCOSE;
	tp_History->t_Event.u8_Urgency = HISTORY_URGENCY_NOTIFICATION;

	return FUNCTION_OK;
}
