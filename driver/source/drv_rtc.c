/*
 * Module:	RTC driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

#define DRV_RTC_UNLOCK_KEY1			0xCA
#define DRV_RTC_UNLOCK_KEY2			0x53
#define DRV_RTC_LOCK_KEY			0xFF

#define DRV_RTC_TICK_CYCLE_DEFAULT	10
#define DRV_RTC_YEAR_DEFAULT		12

#define GET_WAKEUP_COUNTER(cycle)	(((uint32)((cycle) * (32768 / 16) / 1000)) - 1)


//Type definition

typedef enum
{
	DRV_RTC_FLAG_WAKEUP_ENABLE = 0,
	DRV_RTC_FLAG_ALARM_ENABLE,
	DRV_RTC_FLAG_TIME_FORMAT,
	DRV_RTC_COUNT_FLAG
} drv_rtc_flag;


//Variable definition

static uint m_ui_Flag = {0};
static uint16 m_u16_TickCycle = {0};
static drv_rtc_callback m_t_Callback = {0};


//Private function declaration

static void DrvRTC_Synchronize(void);
static void DrvRTC_BeginCalendarInit(void);
static void DrvRTC_EndCalendarInit(void);
static uint8 DrvRTC_ByteToBCD
(
	uint8 u8_Byte
);
static uint8 DrvRTC_BCDToByte
(
	uint8 u8_BCD
);
static void DrvRTC_SetCallback
(
	const drv_rtc_callback *tp_Callback
);


//Public function definition

uint DrvRTC_Initialize(void)
{
	uint ui_Value;


	m_u16_TickCycle = DRV_RTC_TICK_CYCLE_DEFAULT;

	CLK->PCKENR2 |= CLK_PCKENR2_RTC;

	/* Disable the write protection for RTC registers */
	RTC->WPR = DRV_RTC_UNLOCK_KEY1;
	RTC->WPR = DRV_RTC_UNLOCK_KEY2;

	RTC->CR1 = 0;
	RTC->CR2 = 0;

	/* Enable alarm for hour and minute */
	RTC->ALRMAR1 = 0;
	RTC->ALRMAR2 &= ~RTC_ALRMAR2_MSK2;
	RTC->ALRMAR3 &= ~RTC_ALRMAR3_MSK3;
	RTC->ALRMAR4 |= RTC_ALRMAR4_MSK4;

	/* Enable wake up and alarm unit Interrupt */
	RTC->CR2 |= (uint8)((RTC_IT_WUT | RTC_IT_ALRA) & 0x00F0);
	RTC->TCR1 |= (uint8)((RTC_IT_WUT | RTC_IT_ALRA) & RTC_TCR1_TAMPIE);

	/* Wait until WUTWF flag is set */
	while (((RTC->ISR1 & RTC_ISR1_WUTWF) == RESET) ||
		((RTC->ISR1 & RTC_ISR1_ALRAWF) == RESET))	
	{
		;
	}

	/* Configure the Wakeup Timer counter */
	RTC->WUTRH = (uint8)(GET_WAKEUP_COUNTER(DRV_RTC_TICK_CYCLE_DEFAULT) >> 8);
	RTC->WUTRL = (uint8)(GET_WAKEUP_COUNTER(DRV_RTC_TICK_CYCLE_DEFAULT));

	/* Enable the write protection for RTC registers */
	RTC->WPR = DRV_RTC_LOCK_KEY; 

	DrvRTC_Synchronize();
	DrvRTC_GetConfig(DRV_RTC_PARAM_CALENDAR_YEAR, (uint8 *)&ui_Value, (uint *)0);

	if (ui_Value < DRV_RTC_YEAR_DEFAULT)
	{
		ui_Value = DRV_RTC_YEAR_DEFAULT;
		DrvRTC_SetConfig(DRV_RTC_PARAM_CALENDAR_YEAR, (const uint8 *)&ui_Value,
			sizeof(ui_Value));
	}

	return FUNCTION_OK;
}


uint DrvRTC_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_RTC_PARAM_TICK_CYCLE:
			m_u16_TickCycle = *((uint16 *)u8p_Value);
			RTC->WPR = DRV_RTC_UNLOCK_KEY1;
			RTC->WPR = DRV_RTC_UNLOCK_KEY2;

			if (REG_GET_BIT(m_ui_Flag, DRV_RTC_FLAG_WAKEUP_ENABLE) != 0)
			{
				RTC->CR2 &= ~RTC_CR2_WUTE;

				while ((RTC->ISR1 & RTC_ISR1_WUTWF) == RESET)
				{
					;
				}

				RTC->WUTRH = (uint8)(GET_WAKEUP_COUNTER(m_u16_TickCycle) >> 8);
				RTC->WUTRL = (uint8)(GET_WAKEUP_COUNTER(m_u16_TickCycle));
				RTC->CR2 |= RTC_CR2_WUTE;
			}
			else
			{
				RTC->WUTRH = (uint8)(GET_WAKEUP_COUNTER(m_u16_TickCycle) >> 8);
				RTC->WUTRL = (uint8)(GET_WAKEUP_COUNTER(m_u16_TickCycle));
			}

			RTC->WPR = DRV_RTC_LOCK_KEY; 
			break;

		case DRV_RTC_PARAM_CALLBACK:
			DrvRTC_SetCallback((const drv_rtc_callback *)u8p_Value);
			break;

		case DRV_RTC_PARAM_SWITCH_WAKEUP:

			if (*((uint *)u8p_Value) != 0)
			{
				if (REG_GET_BIT(m_ui_Flag, DRV_RTC_FLAG_WAKEUP_ENABLE) == 0)
				{				
					RTC->WPR = DRV_RTC_UNLOCK_KEY1;
					RTC->WPR = DRV_RTC_UNLOCK_KEY2;
					RTC->CR2 |= RTC_CR2_WUTE;
					RTC->WPR = DRV_RTC_LOCK_KEY; 

					REG_SET_BIT(m_ui_Flag, DRV_RTC_FLAG_WAKEUP_ENABLE);
					DrvRTC_Synchronize();
				}
			}
			else
			{
				if (REG_GET_BIT(m_ui_Flag, DRV_RTC_FLAG_WAKEUP_ENABLE) != 0)
				{				
					RTC->WPR = DRV_RTC_UNLOCK_KEY1;
					RTC->WPR = DRV_RTC_UNLOCK_KEY2;
					RTC->CR2 &= ~RTC_CR2_WUTE;

					while ((RTC->ISR1 & RTC_ISR1_WUTWF) == RESET)
					{
						;
					}

					RTC->WPR = DRV_RTC_LOCK_KEY; 
					REG_CLEAR_BIT(m_ui_Flag, DRV_RTC_FLAG_WAKEUP_ENABLE);
				}
			}

			break;

		case DRV_RTC_PARAM_SWITCH_ALARM:
			RTC->WPR = DRV_RTC_UNLOCK_KEY1;
			RTC->WPR = DRV_RTC_UNLOCK_KEY2;

			if (*((uint *)u8p_Value) != 0)
			{
				RTC->CR2 |= RTC_CR2_ALRAE;
				REG_SET_BIT(m_ui_Flag, DRV_RTC_FLAG_ALARM_ENABLE);
			}
			else
			{
				RTC->CR2 &= ~RTC_CR2_ALRAE;

				/* Wait until ALRxWF flag is set */
				while ((RTC->ISR1 & RTC_ISR1_ALRAWF) == RESET)
				{
					;
				}

				REG_CLEAR_BIT(m_ui_Flag, DRV_RTC_FLAG_ALARM_ENABLE);
			}

			RTC->WPR = DRV_RTC_LOCK_KEY; 
			break;

		case DRV_RTC_PARAM_CALENDAR_YEAR:
		case DRV_RTC_PARAM_CALENDAR_MONTH:
		case DRV_RTC_PARAM_CALENDAR_DAY:
		case DRV_RTC_PARAM_CALENDAR_HOUR:
		case DRV_RTC_PARAM_CALENDAR_MINUTE:
		case DRV_RTC_PARAM_CALENDAR_SECOND:
			DrvRTC_BeginCalendarInit();
			
			switch (ui_Parameter)
			{
				case DRV_RTC_PARAM_CALENDAR_YEAR:
					RTC->DR3 = DrvRTC_ByteToBCD((uint8)(*(const uint *)u8p_Value));
					break;

				case DRV_RTC_PARAM_CALENDAR_MONTH:
					REG_WRITE_FIELD(RTC->DR2, 0, REG_MASK_5_BIT, 
						DrvRTC_ByteToBCD((uint8)(*(const uint *)u8p_Value)));
					break;

				case DRV_RTC_PARAM_CALENDAR_DAY:
					RTC->DR1 = DrvRTC_ByteToBCD((uint8)(*(const uint *)u8p_Value));
					break;

				case DRV_RTC_PARAM_CALENDAR_HOUR:
					REG_WRITE_FIELD(RTC->TR3, 0, REG_MASK_6_BIT, 
						DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value)));
					break;

				case DRV_RTC_PARAM_CALENDAR_MINUTE:
					RTC->TR2 = DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value));
					break;

				case DRV_RTC_PARAM_CALENDAR_SECOND:
					RTC->TR1 = DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value));
					break;
					
				default:
					break;
			}

			DrvRTC_EndCalendarInit();
			break;

		case DRV_RTC_PARAM_ALARM_WEEKDAY:
		case DRV_RTC_PARAM_ALARM_HOUR:
		case DRV_RTC_PARAM_ALARM_MINUTE:

			RTC->WPR = DRV_RTC_UNLOCK_KEY1;
			RTC->WPR = DRV_RTC_UNLOCK_KEY2;

			if (REG_GET_BIT(m_ui_Flag, DRV_RTC_FLAG_ALARM_ENABLE) != 0)
			{
				RTC->CR2 &= ~RTC_CR2_ALRAE;

				while ((RTC->ISR1 & RTC_ISR1_ALRAWF) == RESET)
				{
					;
				}
			}

			switch (ui_Parameter)
			{
				case DRV_RTC_PARAM_ALARM_WEEKDAY:
					REG_WRITE_FIELD(RTC->ALRMAR4, 0, REG_MASK_4_BIT, 
						DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value)));
					break;

				case DRV_RTC_PARAM_ALARM_HOUR:
					REG_WRITE_FIELD(RTC->ALRMAR3, 0, REG_MASK_6_BIT, 
						DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value)));
					break;

				case DRV_RTC_PARAM_ALARM_MINUTE:
					REG_WRITE_FIELD(RTC->ALRMAR2, 0, REG_MASK_7_BIT, 
						DrvRTC_ByteToBCD((uint8)*((const uint *)u8p_Value)));
					break;

				default:
					break;
			}

			if (REG_GET_BIT(m_ui_Flag, DRV_RTC_FLAG_ALARM_ENABLE) != 0)
			{
				RTC->CR2 |= RTC_CR2_ALRAE;
			}

			RTC->WPR = DRV_RTC_LOCK_KEY; 
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvRTC_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_RTC_PARAM_RESET:

			if ((RTC->ISR1 & RTC_ISR1_INITS) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		case DRV_RTC_PARAM_TICK_CYCLE:
			*((uint16 *)u8p_Value) = m_u16_TickCycle;
			break;

		case DRV_RTC_PARAM_CALENDAR_YEAR:
			(void)RTC->TR1;
			*((uint *)u8p_Value) = (uint)DrvRTC_BCDToByte(RTC->DR3);
			break;

		case DRV_RTC_PARAM_CALENDAR_MONTH:
			(void)RTC->TR1;
			*((uint *)u8p_Value) = 
				(uint)DrvRTC_BCDToByte(REG_READ_FIELD(RTC->DR2, 0, 
				REG_MASK_5_BIT));
			(void)RTC->DR3;
			break;

		case DRV_RTC_PARAM_CALENDAR_DAY:
			(void)RTC->TR1;
			*((uint *)u8p_Value) = (uint)DrvRTC_BCDToByte(RTC->DR1);
			(void)RTC->DR3;
			break;

		case DRV_RTC_PARAM_CALENDAR_HOUR:
			(void)RTC->TR1;
			*((uint *)u8p_Value) = 
				(uint)DrvRTC_BCDToByte(REG_READ_FIELD(RTC->TR3, 0, 
				REG_MASK_6_BIT));
			(void)RTC->DR3;
			break;

		case DRV_RTC_PARAM_CALENDAR_MINUTE:
			(void)RTC->TR1;
			*((uint *)u8p_Value) = (uint)DrvRTC_BCDToByte(RTC->TR2);
			(void)RTC->DR3;
			break;

		case DRV_RTC_PARAM_CALENDAR_SECOND:
			*((uint *)u8p_Value) = (uint)DrvRTC_BCDToByte(RTC->TR1);
			(void)RTC->DR3;
			break;

		case DRV_RTC_PARAM_ALARM_WEEKDAY:
			*((uint *)u8p_Value) = 
				(uint)DrvRTC_BCDToByte(REG_READ_FIELD(RTC->ALRMAR4, 0, 
				REG_MASK_4_BIT));
			break;

		case DRV_RTC_PARAM_ALARM_HOUR:
			*((uint *)u8p_Value) = 
				(uint)DrvRTC_BCDToByte(REG_READ_FIELD(RTC->ALRMAR3, 0, 
				REG_MASK_6_BIT));
			break;

		case DRV_RTC_PARAM_ALARM_MINUTE:
			*((uint *)u8p_Value) = 
				(uint)DrvRTC_BCDToByte(REG_READ_FIELD(RTC->ALRMAR2, 0, 
				REG_MASK_7_BIT));
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void DrvRTC_Interrupt(void)
{
	uint8 u8_ISR2;


	u8_ISR2 = RTC->ISR2;

	if ((u8_ISR2 & (uint8)(RTC_IT_WUT >> 4)) != RESET)
	{
		RTC->ISR2 = ~((uint8)(RTC_IT_WUT >> 4));

		if (m_t_Callback.fp_Wakeup != (drv_rtc_callback_wakeup)0)
		{
			m_t_Callback.fp_Wakeup(m_u16_TickCycle);
		}
	}
	
	if ((u8_ISR2 & (uint8)(RTC_IT_ALRA >> 4)) != RESET)
	{
		RTC->ISR2 = ~((uint8)(RTC_IT_ALRA >> 4));

		if (m_t_Callback.fp_Alarm != (drv_rtc_callback_alarm)0)
		{
			m_t_Callback.fp_Alarm();
		}
	}
}


#if DRV_RTC_TEST_ENABLE == 1

void DrvRTC_Test(void)
{
}

#endif


//Private function definition

static void DrvRTC_Synchronize(void)
{
	RTC->WPR = DRV_RTC_UNLOCK_KEY1;
	RTC->WPR = DRV_RTC_UNLOCK_KEY2;

	/* Clear RSF flag by writing 0 in RSF bit  */
	RTC->ISR1 &= ~(RTC_ISR1_RSF | RTC_ISR1_INIT);

	/* Wait the registers to be synchronised */
	while ((RTC->ISR1 & RTC_ISR1_RSF) == RESET)
	{
		;
	}

	/* Enable the write protection for RTC registers */
	RTC->WPR = DRV_RTC_LOCK_KEY; 
}


static void DrvRTC_BeginCalendarInit(void)
{
	/* Disable the write protection for RTC registers */
	RTC->WPR = DRV_RTC_UNLOCK_KEY1;
	RTC->WPR = DRV_RTC_UNLOCK_KEY2;

	/* Check if the Initialization mode is set */
	if ((RTC->ISR1 & RTC_ISR1_INITF) == RESET)
	{
		/* Set the Initialization mode */
		RTC->ISR1 = (uint8_t)RTC_ISR1_INIT;

		/* Wait until INITF flag is set */
		while ((RTC->ISR1 & RTC_ISR1_INITF) == RESET)
		{
			;
		}
	}
}


static void DrvRTC_EndCalendarInit(void)
{
	/* Exit Initialization mode */
	RTC->ISR1 &= (uint8_t)~RTC_ISR1_INIT;

	/* Enable the write protection for RTC registers */
	RTC->WPR = DRV_RTC_LOCK_KEY; 

	DrvRTC_Synchronize();
}


static uint8 DrvRTC_ByteToBCD
(
	uint8 u8_Byte
)
{
	uint8 u8_BCDTens = 0;

	while (u8_Byte >= 10)
	{
		u8_BCDTens++;
		u8_Byte -= 10;
	}

	return  ((u8_BCDTens << 4) | u8_Byte);
}


static uint8 DrvRTC_BCDToByte
(
	uint8 u8_BCD
)
{
	uint8 u8_Byte = 0;

	u8_Byte = ((u8_BCD & 0xF0) >> 4) * 10;

	return (u8_Byte + (u8_BCD & 0x0F));
}


static void DrvRTC_SetCallback
(
	const drv_rtc_callback *tp_Callback
)
{
	if (tp_Callback->fp_Wakeup != (drv_rtc_callback_wakeup)0)
	{
		m_t_Callback.fp_Wakeup = tp_Callback->fp_Wakeup;
	}

	if (tp_Callback->fp_Alarm != (drv_rtc_callback_alarm)0)
	{
		m_t_Callback.fp_Alarm = tp_Callback->fp_Alarm;
	}
}
