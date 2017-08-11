/*
 * Module:	Blood glucose meter task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#include "stm8l15x.h"
#include "stm8l15x_usart.h"
#include "drv.h"
#include "button.h"
#include "bluetooth.h"
#include "glucose.h"

#include "lib_checksum.h"
#include "devfs.h"

#include "task.h"
#include "task_device.h"
#include "task_shell.h"
#include "task_glucose.h"
#include "drv_voice.h"




//Constant definition

#define DATA_COUNT_HCT_TEST				10
#define DATA_COUNT_BG1_TEST				50
#define DATA_COUNT_BG2_TEST				30
#define DATA_COUNT_BG2_DELAY			5
#define DATA_COUNT_FILL_DETECT			3
#define DATA_COUNT_ABOVE_BG2_LIMIT		3
#define DATA_COUNT_HCT_CHECK			5
#define DATA_COUNT_HCT_AVERAGE			3

#define COUNT_DOWN_BG1_TEST				5

#define DELAY_MODE_SWITCH				200
#define DELAY_NBB_TEST					1000
#define DELAY_EARLY_FILL_TEST			100
#define DELAY_FILL_DETECT				100
#define DELAY_BG2_TEST					100
#define DELAY_BG1_TEST					100
#define DELAY_HCT_TEST					100
#define DELAY_CONNECTION_CHECK			200
#define DELAY_TEST_FINISH				100
#define DELAY_ERROR						TASK_CALIBRATE_TIME(5000)

#define TIMEOUT_EARLY_FILL				1000
#define TIMEOUT_COUNT_DOWN				1000
#define TIMEOUT_FILL_DETECT				TASK_CALIBRATE_TIME(12000)
#define TIMEOUT_TEST_FINISH				TASK_CALIBRATE_TIME(12000)

#define KETONE_LIMIT_DEFAULT			300
#define GLUCOSE_UPPER_LIMIT_DEFAULT		600
#define GLUCOSE_LOWER_LIMIT_DEFAULT		20
#define TEMPERATURE_UPPER_LIMIT_DEFAULT	450
#define TEMPERATURE_LOWER_LIMIT_DEFAULT	50
#define HCT_NBB_LIMIT_DEFAULT			50000
#define HCT_BIAS_LIMIT_DEFAULT			1000

#define BLOOD_BEEP_COUNT				1	
#define BLOOD_BEEP_ON_INTERVAL			500
#define BLOOD_BEEP_OFF_INTERVAL			0	
#define READING_BEEP_COUNT				1	
#define READING_BEEP_ON_INTERVAL		500
#define READING_BEEP_OFF_INTERVAL		0	

#define BG_CURRENT_MAX					2000
#define SIGNAL_PRESENT_COUNT_MAX		3
#define GLUCOSE_CURRENT_RATIO			100
#define COEFFICIENT_RATIO				100000
#define GLUCOSE_MG_TO_MMOL_NUMERATOR	10			
#define GLUCOSE_MG_TO_MMOL_DENOMINATOR	18			
#define HCT_FACTOR_RATIO				1000
#define COUNT_LOG_TABLE					100

#define CODE_CARD_ADDRESS				0x1000


//Type definition

typedef enum
{
	TASK_GLUCOSE_SWITCH_HYPO = 0,
	TASK_GLUCOSE_SWITCH_HYPER,
	TASK_GLUCOSE_SWITCH_KETONE,
	TASK_GLUCOSE_SWITCH_MEAL,
	TASK_GLUCOSE_COUNT_SWITCH
} task_glucose_switch;

typedef struct
{
	uint8 u8_FileID;
	uint8 u8_Switch;
	uint16 u16_Hypo;
	uint16 u16_Hyper;
	uint16 u16_HistoryCount;
	uint16 u16_HistoryIndex;
} task_glucose_setting;

typedef struct
{
	uint16 u16_CodeNumber;
	uint16 u16_Temperature;
	uint16 u16_NBBLimit;
	uint16 u16_EarlyFillLimit;
	uint16 u16_FillLimit;
	uint16 u16_BG2Limit;
	sint32 s32_GlucoseCode[5];
	sint32 s32_CurrentTemperatureCode[3][2];
	uint16 u16_Reserved[35];
	uint16 u16_Checksum;
} task_glucose_code_glucose;

typedef struct
{
	sint32 s32_ImpedanceTemperatureCode[2][3];
	sint32 s32_ImpedanceRangeCode[4];
	sint32 s32_HCTFactor[2][3];
	sint32 s32_HCTCode[3];
	sint32 s32_HCTPercentage;
	uint16 u16_Reserved[23];
	uint16 u16_Checksum;
} task_glucose_code_hct;

typedef struct
{
	task_glucose_code_glucose t_CodeGlucose;
	task_glucose_code_hct t_CodeHCT;
} task_glucose_code;

typedef struct
{
	uint16 u16_Temperature;
	uint16 u16_DataNBB;
	uint16 u16_DataEarlyFill;
	uint16 u16_DataFillDetectLast;
	uint16 u16_DataBG2Test[DATA_COUNT_BG2_TEST];
	uint16 u16_DataBG1Test[DATA_COUNT_BG1_TEST];
	uint16 u16_DataBG1Last;
	uint16 u16_DataGlucose;
	uint16 u16_DataFillDetect[DATA_COUNT_FILL_DETECT];
	uint16 u16_Reserved[1];
	uint16 u16_DataHCTTest[DATA_COUNT_HCT_TEST];
} task_glucose_test_data;


//Private variable definition

static uint m_ui_SignalPresentCount = {0};
static uint m_ui_ButtonState[BUTTON_COUNT_ID] = {0};
static uint m_u16_Flag = {0};
static uint16 m_u16_KetoneLimit = {0};
static uint16 m_u16_GlucoseUpperLimit = {0};
static uint16 m_u16_GlucoseLowerLimit = {0};
static uint16 m_u16_TemperatureUpperLimit = {0};
static uint16 m_u16_TemperatureLowerLimit = {0};
static task_glucose_setting m_t_Setting = {0};
static task_glucose_code m_t_Code = {0};
static task_glucose_test_data m_t_TestData = {0};
static const uint16 m_u16_LogTable[COUNT_LOG_TABLE + 1] = 
{
	0, 0, 3010, 4771, 6021, 6990, 7782, 8451, 
	9031, 9542, 0, 414, 792, 1139, 1461, 1761, 
	2041, 2304, 2553, 2788, 3010, 3222, 3424, 3617, 
	3802, 3979, 4150, 4314, 4472, 4624, 4771, 4914, 
	5051, 5185, 5315, 5441, 5563, 5682, 5798, 5911, 
	6021, 6128, 6232, 6335, 6435, 6532, 6628, 6721, 
	6812, 6902, 6990, 7076, 7160, 7243, 7324, 7404, 
	7482, 7559, 7634, 7709, 7782, 7853, 7924, 7993, 
	8062, 8129, 8195, 8261, 8325, 8388, 8451, 8513, 
	8573, 8633, 8692, 8751, 8808, 8865, 8921, 8976, 
	9031, 9085, 9138, 9191, 9243, 9294, 9345, 9395, 
	9445, 9494, 9542, 9590, 9638, 9685, 9731, 9777, 
	9823, 9868, 9912, 9956, 10000
};

static  uint8 m_u8_retable[25]={0};


//Private function declaration

static uint TaskGlucose_ResetFile(void);
static uint TaskGlucose_DisplayGlucose
(
	uint16 u16_Glucose
);
static void TaskGlucose_DisplayCountDown
(
	uint ui_CountDown
);
static void TaskGlucose_DisplayStrip(void);
static void TaskGlucose_DisplayFilling(void);
static void TaskGlucose_DisplayCodeError(void);
static void TaskGlucose_DisplayNBBError(void);
static void TaskGlucose_DisplayTemperatureError(void);
static void TaskGlucose_DisplayBG2Error(void);
static void TaskGlucose_DisplayHCTError(void);
static void TaskGlucose_DisplayStripError(void);
static void TaskGlucose_DisplayControl(void);
static void TaskGlucose_DisplayPound(void);
static void TaskGlucose_DisplayMeal(void);
static uint TaskGlucose_CheckCodeCard(void);
static uint TaskGlucose_CheckButtonControl(void);
static uint TaskGlucose_CheckButtonPound(void);
static uint TaskGlucose_CheckButtonMeal(void);
static uint TaskGlucose_CheckSignalPresent(void);
static void TaskGlucose_EnableGlucose(void);
static void TaskGlucose_DisableGlucose(void);
static uint16 TaskGlucose_CalculateGlucose
(
	uint16 u16_BGCurrent,
	sint32 s32_HCTFactor,
	uint16 u16_Temperature
);
static sint32 TaskGlucose_CalculateHCTFactor
(
	uint16 *u16p_HCTImpedance,
	sint32 s32_TemperatureDelta
);
static void TaskGlucose_SaveHistory
(
	uint16 u16_Index
);
static uint TaskGlucose_LoadHistory
(
	uint16 u16_Index,
	task_glucose_history_item *tp_Item
);
static void TaskGlucose_QuitTask
(
	devos_task_handle t_TaskHandle
);
static uint32 TaskGlucose_Log
(
	uint32 u32_Value
);
static sint32 TaskGlucose_Polynomial
(
	sint32 *s32p_Coefficient,
	sint32 s32_Value,
	uint16 u16_Gain,
	uint ui_Power
);
static void TaskGlucose_Multiply64Bit
(
	uint32 *u32p_Operand
);
static void TaskGlucose_Divide64Bit
(
	uint32 *u32p_Operand,
	uint16 u16_Divisor
);
static void TaskGlucose_retransit(uint16 *tab);
static void uart2_sendbyte(uint8_t data);
static char chartohex(unsigned char hexdata);

static int HtoD_4(unsigned int adata);



//Public function definition

uint TaskGlucose_Initialize
(
	devos_task_handle t_TaskHandle
)
{
	devfs_int t_Length;


	m_u16_KetoneLimit = KETONE_LIMIT_DEFAULT;
	m_u16_GlucoseUpperLimit = GLUCOSE_UPPER_LIMIT_DEFAULT;
	m_u16_GlucoseLowerLimit = GLUCOSE_LOWER_LIMIT_DEFAULT;
	m_u16_TemperatureUpperLimit = TEMPERATURE_UPPER_LIMIT_DEFAULT;
	m_u16_TemperatureLowerLimit = TEMPERATURE_LOWER_LIMIT_DEFAULT;

	t_Length = sizeof(m_t_Setting);

	if (DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY,
		(uint8 *)&m_t_Setting, 0, &t_Length) != FUNCTION_OK)
	{
		return TaskGlucose_ResetFile();
	}
	else
	{
		//Check if the id of history file is matched or not
		if ((m_t_Setting.u8_FileID != TASK_FILE_ID_HISTORY) ||
			(t_Length != sizeof(m_t_Setting)))
		{
			return TaskGlucose_ResetFile();
		}
	}

	t_Length = sizeof(m_t_TestData);

	return DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_TEST,
		(uint8 *)&m_t_TestData, 0, &t_Length);
}


void TaskGlucose_Process
(
	devos_task_handle t_TaskHandle
)
{
	static uint ui_Value;
	static uint ui_TestDataIndex;
	static uint ui_TestDataCount;
	static uint16 u16_Timer;
	static sint32 s32_HCTFactor;
	static uint8 *u8p_MessageData;
	static devos_int t_MessageLength;
	static task_glucose_code_glucose *tp_CodeGlucose;
	uint16 table[20]={0};
	static uint16  V_value;
	static uint8  V_flag=0;


	DEVOS_TASK_BEGIN

	DevOS_MessageWait(TASK_MESSAGE_ID_GLUCOSE, u8p_MessageData, 
		&t_MessageLength); 

	do 
	{

		re_cactu(&table[0]);
                
        Glucose_re_initialize();
		TaskGlucose_retransit(&table[0]);
		/*VOICE_Start(21,10);
        	DevOS_TaskDelay(270);
        	Voice_closed();
        	VOICE_Start(5,10);
        	DevOS_TaskDelay(500);
		Voice_closed();*/
		//voice_merage(4,189);
		
        Drv_EnablePower();
		TaskGlucose_EnableGlucose();
		ui_Value = GLUCOSE_MODE_HCT;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		ui_Value = 1;
		Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_NBB_TEST / 2);
		Glucose_Sample();
		m_t_TestData.u16_DataNBB = Glucose_Read(GLUCOSE_CHANNEL_HCT);
		ui_Value = 0;
		Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		ui_Value = GLUCOSE_MODE_BG2;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_NBB_TEST / 2);
		TaskDevice_GetConfig(TASK_DEVICE_PARAM_NO_CODE, (uint8 *)&ui_Value, 
			(uint *)0);
         
		//voice_merage(0);

		if (ui_Value != 0)
		{
			REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE);
		}
		else
		{
			REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE);
		}

		if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
		{
			DrvEEPROM_Enable();
		}

		Glucose_Sample();
		tp_CodeGlucose = &m_t_Code.t_CodeGlucose;

		//Check code card
		if (TaskGlucose_CheckCodeCard() != FUNCTION_OK)
		{
			if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
			{
				DrvEEPROM_Disable();
			}

			Drv_DisablePower();
			DevOS_TaskDelay(DELAY_NBB_TEST);
			TaskGlucose_DisplayCodeError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}
		else
		{
			if (tp_CodeGlucose->u16_CodeNumber >= TASK_DEVICE_ERROR_ID_BASE)
			{
				if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
				{
					DrvEEPROM_Disable();
				}

				Drv_DisablePower();
				DevOS_TaskDelay(DELAY_NBB_TEST);
				TaskGlucose_DisplayCodeError();
				DevOS_TaskDelay(DELAY_ERROR);
				break;
			}
		}

		m_t_TestData.u16_Temperature = Glucose_Read(GLUCOSE_CHANNEL_NTC);

		if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
		{
			DrvEEPROM_Disable();
		}

		Drv_DisablePower();

		//Check if temperature is out of limit
		if ((m_t_TestData.u16_Temperature > m_u16_TemperatureUpperLimit) ||
			(m_t_TestData.u16_Temperature < m_u16_TemperatureLowerLimit))
		{
			DevOS_TaskDelay(DELAY_NBB_TEST);
			TaskGlucose_DisplayTemperatureError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		//Check if NBB is out of limit
		if (m_t_TestData.u16_DataNBB < HCT_NBB_LIMIT_DEFAULT)
		{
			DevOS_TaskDelay(DELAY_NBB_TEST);
			TaskGlucose_DisplayNBBError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		//Sample no blood baseline (NBB)
		m_t_TestData.u16_DataNBB = Glucose_Read(GLUCOSE_CHANNEL_BG);

		//Check if NBB is out of limit
		if (m_t_TestData.u16_DataNBB > tp_CodeGlucose->u16_NBBLimit)
		{
			DevOS_TaskDelay(DELAY_NBB_TEST);
			TaskGlucose_DisplayNBBError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		ui_Value = GLUCOSE_MODE_BG1;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_NBB_TEST);
		Glucose_Sample();
		m_t_TestData.u16_DataNBB = Glucose_Read(GLUCOSE_CHANNEL_BG);

		//Check if NBB is out of limit
		if (m_t_TestData.u16_DataNBB > tp_CodeGlucose->u16_NBBLimit)
		{
			TaskGlucose_DisplayNBBError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		u16_Timer = 0;

		//Detect early filling of blood
		while (u16_Timer < TIMEOUT_EARLY_FILL)
		{
			m_t_TestData.u16_DataEarlyFill = Glucose_Read(GLUCOSE_CHANNEL_BG);

			if (m_t_TestData.u16_DataEarlyFill > 
				tp_CodeGlucose->u16_EarlyFillLimit)
			{
				break;
			}

			u16_Timer += DELAY_EARLY_FILL_TEST;
			DevOS_TaskDelay(DELAY_EARLY_FILL_TEST);
			Glucose_Sample();
		}

		//Check if early filling of blood is detected
		if (u16_Timer < TIMEOUT_EARLY_FILL)
		{
			TaskGlucose_DisplayNBBError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		TaskGlucose_DisplayStrip();
		TaskGlucose_DisplayFilling();
		TaskDevice_GetConfig(TASK_DEVICE_PARAM_GLUCOSE_UNIT, (uint8 *)&ui_Value,
			(uint *)0);
		m_u16_Flag = 0;

		if (ui_Value == 0)
		{
			REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_MMOL);
		}
		else
		{
			REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_MMOL);
		}

		u16_Timer = 0;
		ui_TestDataIndex = 0;
		m_ui_SignalPresentCount = 0;

		//Detect filling of blood
		while (u16_Timer < TIMEOUT_FILL_DETECT)
		{
			if (TaskGlucose_CheckButtonControl() == FUNCTION_OK)
			{
				u16_Timer = 0;
			}

			TaskGlucose_DisplayControl();

			m_t_TestData.u16_DataFillDetect[ui_TestDataIndex] = 
				Glucose_Read(GLUCOSE_CHANNEL_BG);

			if (m_t_TestData.u16_DataFillDetect[ui_TestDataIndex] > 
				tp_CodeGlucose->u16_FillLimit)
			{
				ui_TestDataIndex++;
			}
			else
			{
				ui_TestDataIndex = 0;
			}

			if (ui_TestDataIndex >= DATA_COUNT_FILL_DETECT)
			{
				m_t_TestData.u16_DataFillDetectLast = 
					m_t_TestData.u16_DataFillDetect[DATA_COUNT_FILL_DETECT - 1];
				DevOS_TaskDelay(DELAY_FILL_DETECT);
				Glucose_Sample();
				break;
			}

			u16_Timer += (DELAY_FILL_DETECT / 10);

			if (TaskGlucose_CheckSignalPresent() != FUNCTION_OK)
			{
				break;
			}
			
			voice_merage(0,189);
			DevOS_TaskDelay(DELAY_FILL_DETECT);
			Glucose_Sample();
		}

		//Check if timeout of filling detection occurs
		if ((m_ui_SignalPresentCount > SIGNAL_PRESENT_COUNT_MAX) ||
			(u16_Timer >= TIMEOUT_FILL_DETECT))
		{
			break;
		}

		DrvBEEP_Start(BLOOD_BEEP_COUNT, BLOOD_BEEP_ON_INTERVAL, 
			BLOOD_BEEP_OFF_INTERVAL);
		ui_Value = GLUCOSE_MODE_BG2;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_MODE_SWITCH);
		Glucose_Sample();
		ui_TestDataCount = 0;
		ui_TestDataIndex = 0;
		m_ui_SignalPresentCount = 0;

		//Test BG2
		while (ui_TestDataIndex < DATA_COUNT_BG2_TEST)
		{
			m_t_TestData.u16_DataBG2Test[ui_TestDataIndex] = 
				Glucose_Read(GLUCOSE_CHANNEL_BG);

			if (ui_TestDataIndex >= DATA_COUNT_BG2_DELAY)
			{
				if (m_t_TestData.u16_DataBG2Test[ui_TestDataIndex] > 
					tp_CodeGlucose->u16_BG2Limit)
				{
					ui_TestDataCount++;
				}
				else
				{
					ui_TestDataCount = 0;
				}
			}

			ui_TestDataIndex++;

			if (ui_TestDataIndex >= DATA_COUNT_BG2_DELAY)
			{
				if (ui_TestDataCount >= DATA_COUNT_ABOVE_BG2_LIMIT)
				{
					Drv_Memset((uint8 *)(&m_t_TestData.u16_DataBG2Test[ui_TestDataIndex]),
						0, sizeof(uint16) * (DATA_COUNT_BG2_TEST - 
						ui_TestDataIndex));
					DevOS_TaskDelay(DELAY_BG2_TEST);
					Glucose_Sample();
					break;
				}
			}

			if (TaskGlucose_CheckSignalPresent() != FUNCTION_OK)
			{
				break;
			}

			DevOS_TaskDelay(DELAY_BG2_TEST);
			Glucose_Sample();
		}

		//Check if enough blood is applied on the strip
		if ((m_ui_SignalPresentCount > SIGNAL_PRESENT_COUNT_MAX) ||
			(ui_TestDataCount < DATA_COUNT_ABOVE_BG2_LIMIT))
		{
			TaskGlucose_DisplayBG2Error();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}
		V_flag=0;
		ui_Value = GLUCOSE_MODE_BG1;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_MODE_SWITCH);
		Glucose_Sample();
		ui_TestDataIndex = 0;
		ui_TestDataCount = COUNT_DOWN_BG1_TEST + 1;
		u16_Timer = TIMEOUT_COUNT_DOWN;
		m_ui_SignalPresentCount = 0;

		//Test BG1
		while (ui_TestDataIndex < DATA_COUNT_BG1_TEST)
		{
			m_t_TestData.u16_DataBG1Test[ui_TestDataIndex] = 
				Glucose_Read(GLUCOSE_CHANNEL_BG);
			u16_Timer += DELAY_BG1_TEST;

			if (u16_Timer >= TIMEOUT_COUNT_DOWN)
			{
				if (ui_TestDataCount <= COUNT_DOWN_BG1_TEST)
				{
					TaskGlucose_DisplayCountDown(ui_TestDataCount);
				}

				ui_TestDataCount--;
				u16_Timer = 0;
			}

			ui_TestDataIndex++;

			if (TaskGlucose_CheckSignalPresent() != FUNCTION_OK)
			{
				break;
			}

			DevOS_TaskDelay(DELAY_BG1_TEST);
			Glucose_Sample();
		}
		
		if (m_ui_SignalPresentCount > SIGNAL_PRESENT_COUNT_MAX)
		{
			TaskGlucose_DisplayStripError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		TaskGlucose_DisplayCountDown(ui_TestDataCount);
		m_t_TestData.u16_DataBG1Last = Glucose_Read(GLUCOSE_CHANNEL_BG);
		ui_Value = GLUCOSE_MODE_HCT;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		Drv_EnablePower();
		DevOS_TaskDelay(DELAY_MODE_SWITCH);
		ui_Value = 1;
		Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		DevOS_TaskDelay(DELAY_MODE_SWITCH);
		ui_TestDataIndex = 0;
		m_ui_SignalPresentCount = 0;

		//Test HCT
		while (ui_TestDataIndex < DATA_COUNT_HCT_TEST)
		{
			m_t_TestData.u16_DataHCTTest[ui_TestDataIndex] = 
				Glucose_Read(GLUCOSE_CHANNEL_HCT);
			ui_TestDataIndex++;

			if (TaskGlucose_CheckSignalPresent() != FUNCTION_OK)
			{
				break;
			}

			DevOS_TaskDelay(DELAY_HCT_TEST);
		}

		ui_Value = 0;
		Glucose_SetConfig(GLUCOSE_PARAM_HCT_WAVEFORM, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		ui_Value = GLUCOSE_MODE_OFF;
		Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value, 
			sizeof(ui_Value));
		Drv_DisablePower();
		
		if (m_ui_SignalPresentCount > SIGNAL_PRESENT_COUNT_MAX)
		{
			TaskGlucose_DisplayStripError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		m_t_TestData.u16_Temperature = Glucose_Read(GLUCOSE_CHANNEL_NTC);
		TaskGlucose_DisableGlucose();

		//Calculate HCT factor and blood glucose
		s32_HCTFactor = 
			TaskGlucose_CalculateHCTFactor(m_t_TestData.u16_DataHCTTest, 
			(sint32)m_t_TestData.u16_Temperature - 
			(sint32)tp_CodeGlucose->u16_Temperature);

		if (s32_HCTFactor == 0)
		{
			TaskGlucose_DisplayHCTError();
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		m_t_TestData.u16_DataGlucose = 
			TaskGlucose_CalculateGlucose(m_t_TestData.u16_DataBG1Last,
			s32_HCTFactor, m_t_TestData.u16_Temperature);
		ui_Value = TASK_SHELL_COMMAND_OUTPUT_TEST;
		TaskShell_SetConfig(TASK_SHELL_PARAM_COMMAND, (const uint8 *)&ui_Value,
			sizeof(ui_Value));

		if (TaskGlucose_DisplayGlucose(m_t_TestData.u16_DataGlucose) != 
			FUNCTION_OK)
		{
			DevOS_TaskDelay(DELAY_ERROR);
			break;
		}

		if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_CONTROL) != 0)
		{
			REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_POUND);
			TaskGlucose_DisplayPound();
		}
		else
		{
			TaskDevice_GetConfig(TASK_DEVICE_PARAM_BLUETOOTH, (uint8 *)&ui_Value,
				(uint *)0);
			
			if (ui_Value != 0)
			{
				Bluetooth_GetConfig(BLUETOOTH_PARAM_SIGNAL_PRESENT,
					(uint8 *)&ui_Value, (uint *)0);

				if (ui_Value == 0)
				{
					ui_Value = 1;
				}
				else
				{
					ui_Value = 0;
				}
			}

			Bluetooth_SetConfig(BLUETOOTH_PARAM_SWITCH, (const uint8 *)&ui_Value,
				sizeof(ui_Value));
			TaskGlucose_SaveHistory(0);
		}
		
		DrvBEEP_Start(READING_BEEP_COUNT, READING_BEEP_ON_INTERVAL, 
			READING_BEEP_OFF_INTERVAL);
		u16_Timer = 0;
		m_ui_SignalPresentCount = 0;

		while (u16_Timer < TIMEOUT_TEST_FINISH)
		{
			if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_CONTROL) == 0)
			{
				if (TaskGlucose_CheckButtonPound() == FUNCTION_OK)
				{
					u16_Timer = 0;
					TaskGlucose_SaveHistory(m_t_Setting.u16_HistoryIndex);
				}

				if (TaskGlucose_CheckButtonMeal() == FUNCTION_OK)
				{
					u16_Timer = 0;
					TaskGlucose_SaveHistory(m_t_Setting.u16_HistoryIndex);
				}

				TaskGlucose_DisplayPound();
				TaskGlucose_DisplayMeal();
			}

			if (TaskGlucose_CheckSignalPresent() != FUNCTION_OK)
			{
				break;
			}

			u16_Timer += (DELAY_TEST_FINISH / 10);
			if(V_flag==0)
				{
			V_value=m_t_TestData.u16_DataGlucose;
			V_value=(uint16)Glucose_Round(((sint32)V_value * 
			GLUCOSE_MG_TO_MMOL_NUMERATOR), GLUCOSE_MG_TO_MMOL_DENOMINATOR);
			if(REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPO)!=0)
			voice_merage(3,V_value);
			else if(REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPER)!=0)
			voice_merage(2,V_value);
			else if(REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_KETONE)!=0)
			voice_merage(4,V_value);
			else
			voice_merage(1,V_value);
			V_flag=1;
				}
			DevOS_TaskDelay(DELAY_TEST_FINISH);
		}
	}
	while(0); 

	TaskGlucose_QuitTask(t_TaskHandle);
	
	DEVOS_TASK_END
}


uint TaskGlucose_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_GLUCOSE_PARAM_HYPO:
			
			if (*((uint16 *)u8p_Value) == 0)
			{
				REG_CLEAR_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPO);
			}
			else
			{
				REG_SET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPO);
			}

			m_t_Setting.u16_Hypo = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_HYPER:
			
			if (*((uint16 *)u8p_Value) == 0)
			{
				REG_CLEAR_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPER);
			}
			else
			{
				REG_SET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPER);
			}

			m_t_Setting.u16_Hyper = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_KETONE:
			REG_WRITE_FIELD(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_KETONE, 
				REG_MASK_1_BIT, *((uint *)u8p_Value));
			break;

		case TASK_GLUCOSE_PARAM_MEAL:
			REG_WRITE_FIELD(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_MEAL, 
				REG_MASK_1_BIT, *((uint *)u8p_Value));
			break;

		case TASK_GLUCOSE_PARAM_NBB_LIMIT:
			m_t_Code.t_CodeGlucose.u16_NBBLimit = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_EARLY_FILL_LIMIT:
			m_t_Code.t_CodeGlucose.u16_EarlyFillLimit = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_FILL_LIMIT:
			m_t_Code.t_CodeGlucose.u16_FillLimit = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_BG_LIMIT:
			m_t_Code.t_CodeGlucose.u16_BG2Limit = *((uint16 *)u8p_Value);
			break;

		case TASK_GLUCOSE_PARAM_TEST:
			break;

		default:
			return FUNCTION_FAIL;
	}

	if ((ui_Parameter == TASK_GLUCOSE_PARAM_HYPO) ||
		(ui_Parameter == TASK_GLUCOSE_PARAM_HYPER) ||
		(ui_Parameter == TASK_GLUCOSE_PARAM_MEAL) ||
		(ui_Parameter == TASK_GLUCOSE_PARAM_KETONE))
	{
		return DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY, 
			(const uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting));
	}

	return FUNCTION_OK;
}


uint TaskGlucose_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case TASK_GLUCOSE_PARAM_HYPO:
			*((uint16 *)u8p_Value) = m_t_Setting.u16_Hypo;
			break;

		case TASK_GLUCOSE_PARAM_HYPER:
			*((uint16 *)u8p_Value) = m_t_Setting.u16_Hyper;
			break;

		case TASK_GLUCOSE_PARAM_KETONE:
			*((uint *)u8p_Value) = REG_READ_FIELD(m_t_Setting.u8_Switch, 
				TASK_GLUCOSE_SWITCH_KETONE, REG_MASK_1_BIT);
			break;

		case TASK_GLUCOSE_PARAM_MEAL:
			*((uint *)u8p_Value) = REG_READ_FIELD(m_t_Setting.u8_Switch, 
				TASK_GLUCOSE_SWITCH_MEAL, REG_MASK_1_BIT);
			break;

		case TASK_GLUCOSE_PARAM_HISTORY_COUNT:
			*((uint16 *)u8p_Value) = m_t_Setting.u16_HistoryCount;
			break;

		case TASK_GLUCOSE_PARAM_NBB_LIMIT:
			*((uint16 *)u8p_Value) = m_t_Code.t_CodeGlucose.u16_NBBLimit;
			break;

		case TASK_GLUCOSE_PARAM_EARLY_FILL_LIMIT:
			*((uint16 *)u8p_Value) = m_t_Code.t_CodeGlucose.u16_EarlyFillLimit;
			break;

		case TASK_GLUCOSE_PARAM_FILL_LIMIT:
			*((uint16 *)u8p_Value) = m_t_Code.t_CodeGlucose.u16_FillLimit;
			break;

		case TASK_GLUCOSE_PARAM_BG_LIMIT:
			*((uint16 *)u8p_Value) = m_t_Code.t_CodeGlucose.u16_BG2Limit;
			break;

		case TASK_GLUCOSE_PARAM_TEST:
			*((uint16 *)u8p_Value) = (uint16)&m_t_TestData; 
			break;
		case TASK_GLUCOSE_PARAM_RE:
			*u8p_Value = (uint8)&m_u8_retable;
                        break;
		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint TaskGlucose_ResetHistory(void)
{
	return TaskGlucose_ResetFile();
}


uint TaskGlucose_ReadHistory
(
	uint16 u16_Index,
	uint ui_Absolute,
	task_glucose_history_item *tp_Item
)
{
	if (ui_Absolute != 0)
	{
		return TaskGlucose_LoadHistory(u16_Index, tp_Item);
	}

	if (u16_Index > m_t_Setting.u16_HistoryCount)
	{
		u16_Index = m_t_Setting.u16_HistoryCount;
	}

	u16_Index += m_t_Setting.u16_HistoryIndex;

	if (u16_Index > m_t_Setting.u16_HistoryCount)
	{
		u16_Index -= m_t_Setting.u16_HistoryCount;
	}

	if (u16_Index == 0)
	{
		return FUNCTION_FAIL;
	}
	else
	{
		return TaskGlucose_LoadHistory(u16_Index, tp_Item);
	}
}


#if TASK_GLUCOSE_TEST_ENABLE == 1

#include "lib_string.h"

#define UART_DEVICE_ID					DRV_UART_DEVICE_ID_2

void TaskGlucose_Test(void)
{
	uint i;
	uint ui_Length;
	uint16 *u16p_Data;
	uint8 u8_String[8];
	const uint8 u8_Tab[] = "\t";
	const uint8 u8_Return[] = "\r\n";


	u16p_Data = (uint16 *)&m_t_TestData;

	for (i = 0; i < sizeof(task_glucose_test_data) / sizeof(uint16); i++)
	{
		ui_Length = 8;
		LibString_NumberToString(u8_String, &ui_Length, *u16p_Data);

		while (DrvUART_Write(DRV_UART_DEVICE_ID_1, u8_String, ui_Length) != 
			FUNCTION_OK)
		{
			;
		}

		while (DrvUART_Write(DRV_UART_DEVICE_ID_1, u8_Tab, sizeof(u8_Tab) - 1) != 
			FUNCTION_OK)
		{
			;
		}
		
		u16p_Data++;
	}

	while (DrvUART_Write(DRV_UART_DEVICE_ID_1, u8_Return, sizeof(u8_Return) - 1) != 
		FUNCTION_OK)
	{
		;
	}
}

#endif


//Private function definition

static uint TaskGlucose_ResetFile(void)
{
	//Delete the history file
	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY,
		(const uint8 *)0, 0, 0) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	//Delete the test data file
	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_TEST,
		(const uint8 *)0, 0, 0) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	Drv_Memset((uint8 *)&m_t_TestData, 0, sizeof(m_t_TestData));
	Drv_Memset((uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting));
	m_t_Setting.u8_FileID = TASK_FILE_ID_HISTORY;
	REG_SET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_KETONE);

	if (DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY,
		(const uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting)) != FUNCTION_OK)
	{
		return FUNCTION_FAIL;
	}

	return DevFS_Synchronize(TASK_FILE_SYSTEM_VOLUME_ID);
}


static uint TaskGlucose_DisplayGlucose
(
	uint16 u16_Glucose
)
{
	uint8 u8_LCDData;


	u8_LCDData = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_BLOOD, &u8_LCDData, sizeof(u8_LCDData));
	DrvLCD_Write(DRV_LCD_OFFSET_STRIP, &u8_LCDData, sizeof(u8_LCDData));

	if (u16_Glucose > m_u16_GlucoseUpperLimit)
	{
		TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_OVERFLOW);
		return FUNCTION_FAIL;
	}
	else if (u16_Glucose < m_u16_GlucoseLowerLimit)
	{
		TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_UNDERFLOW);
		return FUNCTION_FAIL;
	}

	REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPO);
	REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPER);
	REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_KETONE);
	u8_LCDData = DRV_LCD_SYMBOL_ON;

	if ((u16_Glucose < m_t_Setting.u16_Hypo) && 
		(REG_GET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPO) != 0))
	{
		REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPO);
		DrvLCD_Write(DRV_LCD_OFFSET_HYPO, &u8_LCDData, sizeof(u8_LCDData));
	}

	if ((u16_Glucose > m_t_Setting.u16_Hyper) &&
		(REG_GET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_HYPER) != 0))
	{
		REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_HYPER);
		DrvLCD_Write(DRV_LCD_OFFSET_HYPER, &u8_LCDData, sizeof(u8_LCDData));
	}

	if ((u16_Glucose > m_u16_KetoneLimit) && 
		(REG_GET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_KETONE) != 0))
	{
		REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_KETONE);
		DrvLCD_Write(DRV_LCD_OFFSET_KETONE, &u8_LCDData, sizeof(u8_LCDData));
	}

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_MMOL) != 0)
	{
		u16_Glucose = (uint16)Glucose_Round(((sint32)u16_Glucose * 
			GLUCOSE_MG_TO_MMOL_NUMERATOR), GLUCOSE_MG_TO_MMOL_DENOMINATOR);

		if (u16_Glucose < 10)
		{
			u8_LCDData = DRV_LCD_CHARACTER_0;
			DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_UNITS, &u8_LCDData, 
				sizeof(u8_LCDData));
		}
	}
	
	TaskDevice_DisplayGlucose(u16_Glucose);
	TaskDevice_DisplayUnit(DRV_LCD_SYMBOL_ON);

	return FUNCTION_OK;
}


static void TaskGlucose_DisplayCountDown
(
	uint ui_CountDown
)
{
	uint ui_Value;
	uint8 u8_LCDData;


	if (ui_CountDown == COUNT_DOWN_BG1_TEST)
	{
		u8_LCDData = DRV_LCD_CHARACTER_ALL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_TENS, &u8_LCDData, 
			sizeof(u8_LCDData));
		DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_FRACTION, &u8_LCDData, 
			sizeof(u8_LCDData));
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
		DrvLCD_Write(DRV_LCD_OFFSET_CODE, &u8_LCDData, sizeof(u8_LCDData));
		ui_Value = 0;
		DrvLCD_SetConfig(DRV_LCD_OFFSET_BLOOD, DRV_LCD_PARAM_BLINK, 
			(const uint8 *)&ui_Value);
	}

	u8_LCDData = (uint8)ui_CountDown;
	DrvLCD_Write(DRV_LCD_OFFSET_GLUCOSE_UNITS, &u8_LCDData, sizeof(u8_LCDData));
}


static void TaskGlucose_DisplayStrip(void)
{
	uint ui_Value;
	uint8 u8_LCDData;


	u8_LCDData = DRV_LCD_SYMBOL_ON;
	DrvLCD_Write(DRV_LCD_OFFSET_STRIP, &u8_LCDData, sizeof(u8_LCDData));
	ui_Value = 0;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_STRIP, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
}


static void TaskGlucose_DisplayFilling(void)
{
	uint ui_Value;
	uint8 u8_LCDData;


	u8_LCDData = DRV_LCD_SYMBOL_ON;
	DrvLCD_Write(DRV_LCD_OFFSET_BLOOD, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
	{
		DrvLCD_Write(DRV_LCD_OFFSET_CODE, &u8_LCDData, sizeof(u8_LCDData));
		TaskDevice_DisplayGlucose(m_t_Code.t_CodeGlucose.u16_CodeNumber);
	}
	
	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_BLOOD, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
}


static void TaskGlucose_DisplayCodeError(void)
{
	uint ui_Value;


	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_CODE, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_CODE);
}


static void TaskGlucose_DisplayNBBError(void)
{
	uint ui_Value;


	TaskGlucose_DisplayStrip();
	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_STRIP, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_NBB);
}


static void TaskGlucose_DisplayTemperatureError(void)
{
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_TEMPERATURE);
}


static void TaskGlucose_DisplayBG2Error(void)
{
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_BG2);
}


static void TaskGlucose_DisplayHCTError(void)
{
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_HCT);
}


static void TaskGlucose_DisplayStripError(void)
{
	uint ui_Value;
	uint8 u8_LCDData;


	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_STRIP, DRV_LCD_PARAM_BLINK, 
		(const uint8 *)&ui_Value);
	u8_LCDData = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_BLOOD, &u8_LCDData, sizeof(u8_LCDData));
	TaskDevice_DisplayGlucose(TASK_DEVICE_ERROR_ID_STRIP);
}


static void TaskGlucose_DisplayControl(void)
{
	uint8 u8_LCDData;


	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_CONTROL) == 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_CONTROL, &u8_LCDData, sizeof(u8_LCDData));
}


static void TaskGlucose_DisplayPound(void)
{
	uint8 u8_LCDData;


	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_POUND) == 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_OFF;
	}
	else
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
	}

	DrvLCD_Write(DRV_LCD_OFFSET_POUND, &u8_LCDData, sizeof(u8_LCDData));
}


static void TaskGlucose_DisplayMeal(void)
{
	uint8 u8_LCDData;


	u8_LCDData = DRV_LCD_SYMBOL_OFF;
	DrvLCD_Write(DRV_LCD_OFFSET_BEFORE_MEAL, &u8_LCDData, sizeof(u8_LCDData));
	DrvLCD_Write(DRV_LCD_OFFSET_AFTER_MEAL, &u8_LCDData, sizeof(u8_LCDData));

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
		DrvLCD_Write(DRV_LCD_OFFSET_BEFORE_MEAL, &u8_LCDData, 
			sizeof(u8_LCDData));
	}

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL) != 0)
	{
		u8_LCDData = DRV_LCD_SYMBOL_ON;
		DrvLCD_Write(DRV_LCD_OFFSET_AFTER_MEAL, &u8_LCDData, 
			sizeof(u8_LCDData));
	}
}


static uint TaskGlucose_CheckCodeCard(void)
{
	uint ui_Value;
	uint16 u16_Value;
	uint16 u16_Checksum;
	task_glucose_code_glucose *tp_CodeGlucose;
	task_glucose_code_hct *tp_CodeHCT;


	tp_CodeGlucose = &m_t_Code.t_CodeGlucose;

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
	{
		ui_Value = sizeof(m_t_Code.t_CodeGlucose);

		if (DrvEEPROM_Read(0, (uint8 *)tp_CodeGlucose, &ui_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}
	else
	{
		u16_Value = sizeof(m_t_Code.t_CodeGlucose);

		if (DrvFLASH_Read(CODE_CARD_ADDRESS, (uint8 *)tp_CodeGlucose, 
			&u16_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	u16_Checksum = LibChecksum_GetChecksum16Bit((const uint8 *)tp_CodeGlucose,
		sizeof(m_t_Code.t_CodeGlucose) - sizeof(tp_CodeGlucose->u16_Checksum));

	if (u16_Checksum != tp_CodeGlucose->u16_Checksum)
	{
		return FUNCTION_FAIL;
	}

	tp_CodeHCT = &m_t_Code.t_CodeHCT;

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_NO_CODE) == 0)
	{
		ui_Value = sizeof(m_t_Code.t_CodeHCT);

		if (DrvEEPROM_Read(sizeof(m_t_Code.t_CodeGlucose), (uint8 *)tp_CodeHCT, 
			&ui_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}
	else
	{
		u16_Value = sizeof(m_t_Code.t_CodeHCT);

		if (DrvFLASH_Read(CODE_CARD_ADDRESS + sizeof(m_t_Code.t_CodeGlucose), 
			(uint8 *)tp_CodeHCT, &u16_Value) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	u16_Checksum = LibChecksum_GetChecksum16Bit((const uint8 *)tp_CodeHCT,
		sizeof(m_t_Code.t_CodeHCT) - sizeof(tp_CodeHCT->u16_Checksum));

	if (u16_Checksum != tp_CodeHCT->u16_Checksum)
	{
		return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


static uint TaskGlucose_CheckButtonControl(void)
{
	uint ui_Return;
	uint ui_ButtonState;


	ui_Return = FUNCTION_FAIL;
	ui_ButtonState = Button_Read(BUTTON_ID_CENTER);

	if ((ui_ButtonState == BUTTON_STATE_PRESS) &&
		(m_ui_ButtonState[BUTTON_ID_CENTER] == BUTTON_STATE_RELEASE))
	{
		ui_Return = FUNCTION_OK;
		REG_REVERSE_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_CONTROL);
	}

	m_ui_ButtonState[BUTTON_ID_CENTER] = ui_ButtonState;

	return ui_Return;
}


static uint TaskGlucose_CheckButtonPound(void)
{
	uint ui_Return;
	uint ui_ButtonState;


	ui_Return = FUNCTION_FAIL;
	ui_ButtonState = Button_Read(BUTTON_ID_RIGHT);

	if ((ui_ButtonState == BUTTON_STATE_PRESS) &&
		(m_ui_ButtonState[BUTTON_ID_RIGHT] == BUTTON_STATE_RELEASE))
	{
		ui_Return = FUNCTION_OK;
		REG_REVERSE_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_POUND);
	}

	m_ui_ButtonState[BUTTON_ID_RIGHT] = ui_ButtonState;

	return ui_Return;
}


static uint TaskGlucose_CheckButtonMeal(void)
{
	uint ui_Return;
	uint ui_ButtonState;


	ui_Return = FUNCTION_FAIL;

	if (REG_GET_BIT(m_t_Setting.u8_Switch, TASK_GLUCOSE_SWITCH_MEAL) != 0)
	{
		ui_ButtonState = Button_Read(BUTTON_ID_LEFT);

		if ((ui_ButtonState == BUTTON_STATE_PRESS) &&
			(m_ui_ButtonState[BUTTON_ID_LEFT] == BUTTON_STATE_RELEASE))
		{
			ui_Return = FUNCTION_OK;

			if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL) != 0)
			{
				REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL);
				REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL);
			}
			else if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL) != 0)
			{
				REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL);
				REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL);
			}
			else
			{
				REG_SET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_BEFORE_MEAL);
				REG_CLEAR_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_AFTER_MEAL);
			}
		}

		m_ui_ButtonState[BUTTON_ID_LEFT] = ui_ButtonState;
	}

	return ui_Return;
}


static uint TaskGlucose_CheckSignalPresent(void)
{
	uint ui_Value;
	uint ui_SignalPresent;

	ui_Value = 1;
	Glucose_SetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	Glucose_GetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (uint8 *)&ui_SignalPresent, 
		(uint *)0);
	ui_Value = 0;
	Glucose_SetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (const uint8 *)&ui_Value,
		sizeof(ui_Value));

	if (ui_SignalPresent != 0)
	{
		m_ui_SignalPresentCount++;
	}
	else
	{
		m_ui_SignalPresentCount = 0;
	}
	
	if (m_ui_SignalPresentCount > SIGNAL_PRESENT_COUNT_MAX)
	{
		return FUNCTION_FAIL;
	}
	else
	{
		return FUNCTION_OK;
	}
}


static void TaskGlucose_EnableGlucose(void)
{
	uint ui_Value;


	//Enable glucose circuit and disable strip present signal
	ui_Value = 0;
	Glucose_SetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	Glucose_Enable();
}


static void TaskGlucose_DisableGlucose(void)
{
	uint ui_Value;


	//Disable glucose circuit and enable strip present signal
	ui_Value = GLUCOSE_MODE_OFF;
	Glucose_SetConfig(GLUCOSE_PARAM_MODE, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	ui_Value = 1;
	Glucose_SetConfig(GLUCOSE_PARAM_SIGNAL_PRESENT, (const uint8 *)&ui_Value,
		sizeof(ui_Value));
	Glucose_Disable();
}


static uint16 TaskGlucose_CalculateGlucose
(
	uint16 u16_BGCurrent,
	sint32 s32_HCTFactor,
	uint16 u16_Temperature
)
{
	uint ui_Sign;
	sint32 s32_TemperatureDelta;
	sint32 s32_FinalCurrent;
	sint32 s32_Glucose;
	task_glucose_code_glucose *tp_CodeGlucose;


	if (u16_BGCurrent > BG_CURRENT_MAX)
	{
		return ~0;
	}

	tp_CodeGlucose = &m_t_Code.t_CodeGlucose;

	if (REG_GET_BIT(m_u16_Flag, TASK_GLUCOSE_FLAG_CONTROL) == 0)
	{
		s32_TemperatureDelta = (sint32)u16_Temperature - 
			(sint32)tp_CodeGlucose->u16_Temperature;

		if (s32_TemperatureDelta >= 0)
		{
			ui_Sign = 1;
		}
		else
		{
			ui_Sign = 0;
		}

		s32_FinalCurrent = Glucose_Round((sint32)u16_BGCurrent * 
			COEFFICIENT_RATIO, s32_HCTFactor);
		s32_FinalCurrent = Glucose_Round((s32_FinalCurrent *
			COEFFICIENT_RATIO) - (s32_TemperatureDelta * 
			tp_CodeGlucose->s32_CurrentTemperatureCode[ui_Sign][1] * 
			(GLUCOSE_CURRENT_RATIO / GLUCOSE_TEMPERATURE_RATIO)), 
			(s32_TemperatureDelta * 
			tp_CodeGlucose->s32_CurrentTemperatureCode[ui_Sign][0] / 
			GLUCOSE_TEMPERATURE_RATIO) + COEFFICIENT_RATIO);
	}
	else
	{
		s32_FinalCurrent = Glucose_Round(((sint32)u16_BGCurrent * 
			COEFFICIENT_RATIO), ((tp_CodeGlucose->s32_CurrentTemperatureCode[2][0] * 
			(sint32)u16_Temperature) / GLUCOSE_TEMPERATURE_RATIO) + 
			tp_CodeGlucose->s32_CurrentTemperatureCode[2][1]);
	}

	s32_Glucose = TaskGlucose_Polynomial(tp_CodeGlucose->s32_GlucoseCode, 
		s32_FinalCurrent, GLUCOSE_CURRENT_RATIO, 
		sizeof(tp_CodeGlucose->s32_GlucoseCode) / sizeof(sint32));
	s32_Glucose = Glucose_Round(s32_Glucose, COEFFICIENT_RATIO);

	if (s32_Glucose < 0)
	{
		s32_Glucose = 0;
	}

	return (uint16)s32_Glucose;
}


static sint32 TaskGlucose_CalculateHCTFactor
(
	uint16 *u16p_HCTImpedance,
	sint32 s32_TemperatureDelta
)
{
	uint i;
	uint ui_Sign;
	sint32 s32_HCTDelta;
	sint32 s32_HCTFactor;
	uint32 u32_Operand[2];
	task_glucose_code_hct *tp_CodeHCT;


	s32_HCTDelta = u16p_HCTImpedance[DATA_COUNT_HCT_TEST - DATA_COUNT_HCT_CHECK];
	s32_HCTFactor = s32_HCTDelta;

	//Check if there is any invalid impedance value in the sampling data 
	for (i = DATA_COUNT_HCT_TEST - DATA_COUNT_HCT_CHECK; 
		i < DATA_COUNT_HCT_TEST; i++)
	{
		if (u16p_HCTImpedance[i] == (uint16)~0)
		{
			return 0;
		}
		
		if (s32_HCTDelta > (sint32)u16p_HCTImpedance[i])
		{
			s32_HCTDelta = (sint32)u16p_HCTImpedance[i];
		}
		
		if (s32_HCTFactor < (sint32)u16p_HCTImpedance[i])
		{
			s32_HCTFactor = (sint32)u16p_HCTImpedance[i];
		}
	}

	s32_HCTDelta = (s32_HCTDelta - s32_HCTFactor) * (COEFFICIENT_RATIO / 10);
	s32_HCTDelta = Glucose_Round(s32_HCTDelta, s32_HCTFactor);

	if (s32_HCTDelta > HCT_BIAS_LIMIT_DEFAULT)
	{
		return 0;
	}

	s32_HCTDelta = 0;

	for (i = DATA_COUNT_HCT_TEST - DATA_COUNT_HCT_AVERAGE;
		i < DATA_COUNT_HCT_TEST; i++)
	{
		s32_HCTDelta += (sint32)u16p_HCTImpedance[i];
	}

	s32_HCTDelta = Glucose_Round(s32_HCTDelta, DATA_COUNT_HCT_AVERAGE);

	tp_CodeHCT = &m_t_Code.t_CodeHCT;

	if (s32_TemperatureDelta >= 0)
	{
		ui_Sign = 1;
	}
	else
	{
		ui_Sign = 0;
	}

	s32_HCTFactor = 
		TaskGlucose_Polynomial(tp_CodeHCT->s32_ImpedanceTemperatureCode[ui_Sign], 
		s32_TemperatureDelta, GLUCOSE_TEMPERATURE_RATIO, 
		sizeof(tp_CodeHCT->s32_ImpedanceTemperatureCode[ui_Sign]) / 
		sizeof(sint32));

	if (s32_HCTFactor <= 0)
	{
		return 0;
	}

	u32_Operand[0] = (uint32)s32_HCTFactor;
	u32_Operand[1] = (uint32)s32_HCTDelta;
	TaskGlucose_Multiply64Bit(u32_Operand);
	TaskGlucose_Divide64Bit(u32_Operand, COEFFICIENT_RATIO / 10);
	s32_HCTDelta = Glucose_Round(u32_Operand[1], 10);

	if (s32_HCTDelta <= tp_CodeHCT->s32_ImpedanceRangeCode[0])
	{
		return 0;
	}
	else if ((s32_HCTDelta > tp_CodeHCT->s32_ImpedanceRangeCode[0]) &&
		(s32_HCTDelta <= tp_CodeHCT->s32_ImpedanceRangeCode[2]))
	{
		s32_HCTDelta = tp_CodeHCT->s32_ImpedanceRangeCode[2];
	}
	else if ((s32_HCTDelta > tp_CodeHCT->s32_ImpedanceRangeCode[3]) &&
		(s32_HCTDelta < tp_CodeHCT->s32_ImpedanceRangeCode[1]))
	{
		s32_HCTDelta = tp_CodeHCT->s32_ImpedanceRangeCode[3];
	}
	else if (s32_HCTDelta >= tp_CodeHCT->s32_ImpedanceRangeCode[1])
	{
		return 0;
	}

	s32_HCTDelta = TaskGlucose_Log((uint32)s32_HCTDelta);
	s32_HCTDelta = TaskGlucose_Polynomial(tp_CodeHCT->s32_HCTCode, s32_HCTDelta, 
		m_u16_LogTable[COUNT_LOG_TABLE], sizeof(tp_CodeHCT->s32_HCTCode) / 
		sizeof(sint32));
	s32_HCTDelta -= tp_CodeHCT->s32_HCTPercentage;

	if (s32_HCTDelta >= 0)
	{
		ui_Sign = 1;
		u32_Operand[0] = (uint32)s32_HCTDelta;
	}
	else
	{
		ui_Sign = 0;
		u32_Operand[0] = (uint32)(-s32_HCTDelta);
	}

	s32_HCTFactor = 
		TaskGlucose_Polynomial(tp_CodeHCT->s32_HCTFactor[ui_Sign], 
		s32_TemperatureDelta, GLUCOSE_TEMPERATURE_RATIO, 
		sizeof(tp_CodeHCT->s32_HCTFactor[ui_Sign]) / sizeof(sint32));

	if (s32_HCTFactor >= 0)
	{
		u32_Operand[1] = (uint32)s32_HCTFactor;
	}
	else
	{
		u32_Operand[1] = (uint32)(-s32_HCTFactor);

		if (ui_Sign > 0)
		{
			ui_Sign = 0;
		}
		else
		{
			ui_Sign = 1;
		}
	}

	TaskGlucose_Multiply64Bit(u32_Operand);
	TaskGlucose_Divide64Bit(u32_Operand, COEFFICIENT_RATIO / 10);
	s32_HCTFactor = Glucose_Round(u32_Operand[1], 10);
	
	if (ui_Sign == 0)
	{
		s32_HCTFactor *= -1;
	}

	s32_HCTFactor += COEFFICIENT_RATIO;

	return s32_HCTFactor;
}


static void TaskGlucose_SaveHistory
(
	uint16 u16_Index
)
{
	uint i;
	uint ui_Value;
	sint32 s32_Impedance;
	devfs_int t_Offset;
	task_glucose_history_item t_HistoryItem;


	if ((u16_Index > m_t_Setting.u16_HistoryCount) ||
		(u16_Index == 0))
	{
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_YEAR, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Year = (uint8)ui_Value;
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MONTH, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Month = (uint8)ui_Value;
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_DAY, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Day = (uint8)ui_Value;
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_HOUR, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Hour = (uint8)ui_Value;
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_MINUTE, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Minute = (uint8)ui_Value;
		DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_SECOND, (uint8 *)&ui_Value,
			(uint *)0);
		t_HistoryItem.t_DateTime.u8_Second = (uint8)ui_Value;
		t_HistoryItem.u16_Glucose = m_t_TestData.u16_DataGlucose;
		t_HistoryItem.u16_Temperature = m_t_TestData.u16_Temperature;
		t_HistoryItem.u16_Current = m_t_TestData.u16_DataBG1Last;
		s32_Impedance = 0;

		for (i = DATA_COUNT_HCT_TEST - DATA_COUNT_HCT_AVERAGE;
			i < DATA_COUNT_HCT_TEST; i++)
		{
			s32_Impedance += (sint32)m_t_TestData.u16_DataHCTTest[i];
		}

		t_HistoryItem.u16_Impedance = (uint16)Glucose_Round(s32_Impedance, 
			DATA_COUNT_HCT_AVERAGE);
		m_t_Setting.u16_HistoryIndex++;

		if (m_t_Setting.u16_HistoryIndex > TASK_GLUCOSE_HISTORY_ITEM_COUNT)
		{
			m_t_Setting.u16_HistoryIndex = 1;
		}

		if (m_t_Setting.u16_HistoryCount < TASK_GLUCOSE_HISTORY_ITEM_COUNT)
		{
			m_t_Setting.u16_HistoryCount++;
		}

		t_HistoryItem.u16_Index = m_t_Setting.u16_HistoryIndex;
		u16_Index = m_t_Setting.u16_HistoryIndex;
		DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY, 
			(const uint8 *)&m_t_Setting, 0, sizeof(m_t_Setting));
		DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_TEST, 
			(const uint8 *)&m_t_TestData, 0, sizeof(m_t_TestData));
	}
	else
	{
		TaskGlucose_LoadHistory(u16_Index, &t_HistoryItem);
	}

	t_HistoryItem.u16_Flag = m_u16_Flag;
	t_Offset = (sizeof(t_HistoryItem) * (u16_Index - 1)) + sizeof(m_t_Setting);
	DevFS_Write(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY, 
		(const uint8 *)&t_HistoryItem, t_Offset, sizeof(t_HistoryItem));
}


static uint TaskGlucose_LoadHistory
(
	uint16 u16_Index,
	task_glucose_history_item *tp_Item
)
{
	devfs_int t_Length;


	if ((m_t_Setting.u16_HistoryCount == 0) ||
		(u16_Index > m_t_Setting.u16_HistoryCount))
	{
		Drv_Memset((uint8 *)tp_Item, 0, sizeof(*tp_Item));
		tp_Item->u16_Index = u16_Index;
		return FUNCTION_OK;
	}
	
	if (u16_Index == 0)
	{
		u16_Index = m_t_Setting.u16_HistoryIndex;
	}

	t_Length = sizeof(*tp_Item);

	return DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_HISTORY,
		(uint8 *)tp_Item, (sizeof(*tp_Item) * (u16_Index - 1)) + 
		sizeof(m_t_Setting), &t_Length);
}


static void TaskGlucose_QuitTask
(
	devos_task_handle t_TaskHandle
)
{
	uint8 u8_MessageData;
	devfs_int t_Length;


	t_Length = sizeof(m_t_TestData);

	if (DevFS_Read(TASK_FILE_SYSTEM_VOLUME_ID, TASK_FILE_ID_TEST,
		(uint8 *)&m_t_TestData, 0, &t_Length) != FUNCTION_OK)
	{
		Drv_Memset((uint8 *)&m_t_TestData, 0, sizeof(m_t_TestData));
	}

	TaskGlucose_DisableGlucose();
	DevOS_MessageClear(TASK_MESSAGE_ID_GLUCOSE, t_TaskHandle);
	u8_MessageData = TASK_MESSAGE_DATA_SLEEP;
	DevOS_MessageSend(TASK_MESSAGE_ID_DEVICE, &u8_MessageData,
		sizeof(u8_MessageData));
}


static uint32 TaskGlucose_Log
(
	uint32 u32_Value
)
{
	uint32 u32_Power;
	uint32 u32_Integer;
	uint32 u32_Fraction;


	u32_Power = 0;
	u32_Integer = u32_Value;
	u32_Fraction = 1;

	while (u32_Integer >= COUNT_LOG_TABLE)
	{
		u32_Power++;
		u32_Integer /= 10;
		u32_Fraction *= 10;
	}

	if (u32_Integer >= 10)
	{
		u32_Power++;
	}

	u32_Fraction = (uint32)Glucose_Round((sint32)((u32_Value - (u32_Integer * 
		u32_Fraction)) * (uint32)(m_u16_LogTable[u32_Integer + 1] - 
		m_u16_LogTable[u32_Integer])),  (sint32)u32_Fraction) + 
		(uint32)m_u16_LogTable[u32_Integer];
	
	return (u32_Power * (uint32)m_u16_LogTable[COUNT_LOG_TABLE] + u32_Fraction);
}


static sint32 TaskGlucose_Polynomial
(
	sint32 *s32p_Coefficient,
	sint32 s32_Value,
	uint16 u16_Gain,
	uint ui_Power
)
{
	uint i;
	uint j;
	sint32 s32_Monomial;
	sint32 s32_Polynomial;
	sint32 s32_Sign;
	uint32 u32_Operand[2];


	s32_Polynomial = s32p_Coefficient[0];

	for (i = 1; i < ui_Power; i++)
	{
		s32_Monomial = s32p_Coefficient[i];

		for (j = 0; j < i; j++)
		{
			if (s32_Monomial < 0)
			{
				u32_Operand[0] = (uint32)(-s32_Monomial);
				s32_Sign = -1;
			}
			else
			{
				u32_Operand[0] = (uint32)s32_Monomial;
				s32_Sign = 1;
			}

			if (s32_Value < 0)
			{
				u32_Operand[1] = (uint32)(-s32_Value);
				s32_Sign *= -1;
			}
			else
			{
				u32_Operand[1] = (uint32)s32_Value;
			}

			TaskGlucose_Multiply64Bit(u32_Operand);
			TaskGlucose_Divide64Bit(u32_Operand, u16_Gain);
			s32_Monomial = (sint32)u32_Operand[1] * s32_Sign;
		}
		
		s32_Polynomial += s32_Monomial;
	}

	return s32_Polynomial;
}


static void TaskGlucose_Multiply64Bit
(
	uint32 *u32p_Operand
)
{
	uint16 *u16p_Operand;
	uint32 u32_Product;
	uint32 u32_High;
	uint32 u32_Low;


	u16p_Operand = (uint16 *)u32p_Operand;
	u32_Product = ((uint32)u16p_Operand[0] * (uint32)u16p_Operand[3]) + 
		((uint32)u16p_Operand[1] * (uint32)u16p_Operand[2]);
	u32_High = ((uint32)u16p_Operand[0] * (uint32)u16p_Operand[2]) +
		(u32_Product >> 16);
	u32_Low = ((uint32)u16p_Operand[1] * (uint32)u16p_Operand[3]) +
		(u32_Product << 16);
	
	if ((uint32)(~0) - (u32_Product << 16) < ((uint32)u16p_Operand[1] * 
		(uint32)u16p_Operand[3]))
	{
		u32_High++;
	}

	u32p_Operand[0] = u32_High;
	u32p_Operand[1] = u32_Low;
}


static void TaskGlucose_Divide64Bit
(
	uint32 *u32p_Operand,
	uint16 u16_Divisor
)
{
	uint i;
	uint16 *u16p_Operand;
	uint32 u32_Remainder;


	u16p_Operand = (uint16 *)u32p_Operand;
	u32_Remainder = (uint32)u16p_Operand[0];

	for (i = 0; i < 3; i++)
	{
		u16p_Operand[i] = (uint16)(u32_Remainder / (uint32)u16_Divisor);
		u32_Remainder = (uint32)u32_Remainder % (uint32)u16_Divisor;
		u32_Remainder <<= 16;
		u32_Remainder += (uint32)u16p_Operand[i + 1];
	}

	u16p_Operand[3] = (uint16)(u32_Remainder / (uint32)u16_Divisor);
}


static void TaskGlucose_retransit(uint16 *tab )
{

	uint ui_Value;
	uint16 temp;
	uint16 m_u16_Restring[5]={0};
	uint8 ledtabl[25]={0};
	
	uint i;
	
    ui_Value = 1;
	DrvUART_SetConfig(DRV_UART_DEVICE_ID_1, DRV_UART_PARAM_SWITCH, 
		(const uint8 *)&ui_Value, sizeof(ui_Value));

	for(i=0;i<5;i++)
		{
			m_u16_Restring[i] = HtoD_4(*(tab+i));
		}

	
	temp=(m_u16_Restring[0])>>12;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[0]=temp;

	temp=(m_u16_Restring[0])>>8;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[1]=temp;

	temp=(m_u16_Restring[0])>>4;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[2]=temp;
	
	temp=(m_u16_Restring[0])>>0;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[3]=temp;

	ledtabl[4]=0x0a;

	temp=(m_u16_Restring[1])>>12;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[5]=temp;

	temp=(m_u16_Restring[1])>>8;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[6]=temp;

	temp=(m_u16_Restring[1])>>4;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[7]=temp;
	
	temp=(m_u16_Restring[1])>>0;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[8]=temp;

	ledtabl[9]=0x0a;

	temp=(m_u16_Restring[2])>>12;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[10]=temp;

	temp=(m_u16_Restring[2])>>8;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[11]=temp;

	temp=(m_u16_Restring[2])>>4;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[12]=temp;
	
	temp=(m_u16_Restring[2])>>0;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[13]=temp;

	ledtabl[14]=0x0a;

	temp=(m_u16_Restring[3])>>12;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[15]=temp;

	temp=(m_u16_Restring[3])>>8;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[16]=temp;

	temp=(m_u16_Restring[3])>>4;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[17]=temp;
	
	temp=(m_u16_Restring[3])>>0;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[18]=temp;

	ledtabl[19]=0x0a;
	temp=(m_u16_Restring[4])>>12;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[20]=temp;

	temp=(m_u16_Restring[4])>>8;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[21]=temp;

	temp=(m_u16_Restring[4])>>4;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[22]=temp;
	
	temp=(m_u16_Restring[4])>>0;
	temp=temp&0x0f;
	temp=chartohex(temp);
	ledtabl[23]=temp;

	ledtabl[24]=0x0a;
	
	
 
	for(i=0;i<25;i++)
        {
           m_u8_retable[i]=ledtabl[i]; 
       }
	

	
}
static void uart2_sendbyte(uint8_t data)
{
	
	USART1->DR = data;
	while((USART1->SR&0x80)==0);

	

}

static char chartohex(unsigned char hexdata)
{
	if(hexdata<=9&&hexdata>=0)
		{
			hexdata+=0x30;
		}
	else if(hexdata<=15&&hexdata>=10)
		{
			hexdata+=0x37;
		}
	else
		{
			hexdata=0xff;
		}
	return hexdata;

}

static int HtoD_4(unsigned int adata)
{
    unsigned char  i,xbuf[4]; 
    unsigned int  dd,div;
    dd=1000;
    for (i=0;i<4;i++)                 
    {                                
        xbuf[i]=adata/dd;
        adata%=dd;
        dd/=10;
     }
    div=xbuf[0];
    div<<=4;
    div|=xbuf[1];
    div<<=4;
    div|=xbuf[2];
    div<<=4;
    div|=xbuf[3];
    return div;
}

void tran_re(uint8* t)
{
	uint i;
	for(i=0;i<25;i++)
		{
			*(t+i)=m_u8_retable[i];
		}
}
