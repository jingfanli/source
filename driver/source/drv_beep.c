/*
 * Module:	Beep driver 
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

#define BEEP_CLOCK		32768


//Type definition

typedef enum
{
	DRV_BEEP_STATE_IDLE = 0,
	DRV_BEEP_STATE_ON,
	DRV_BEEP_STATE_OFF,
	DRV_BEEP_STATE_MUTE,
	DRV_BEEP_COUNT_STATE
} drv_beep_state;


//Private variable definition

static uint m_ui_State = {0};
static uint m_ui_Frequency = {0};
static uint m_ui_BeepCount = {0};
static uint16 m_u16_OnInterval = {0};
static uint16 m_u16_OffInterval = {0};
static uint16 m_u16_Timer = {0};


//Private function declaration


//Public function definition

uint DrvBEEP_Initialize(void)
{
	uint ui_Frequency;


	CLK->PCKENR1 |= CLK_PCKENR1_BEEP;
	CLK->CBEEPR = 0x04;

	while ((CLK->CBEEPR & CLK_CBEEPR_BEEPSWBSY) != 0)
	{
		;
	}

	CLK->ICKCR |= CLK_ICKCR_BEEPAHALT;

	ui_Frequency = DRV_BEEP_FREQUENCY_1000;
	DrvBEEP_SetConfig(DRV_BEEP_PARAM_FREQUENCY, (const uint8 *)&ui_Frequency,
		sizeof(ui_Frequency));

	return FUNCTION_OK;
}


uint DrvBEEP_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_BEEP_PARAM_FREQUENCY:

			/* Set a default calibration value if no calibration is done */
			if ((BEEP->CSR2 & BEEP_CSR2_BEEPDIV) == BEEP_CSR2_BEEPDIV)
			{
				BEEP->CSR2 &= ~BEEP_CSR2_BEEPDIV;
				BEEP->CSR2 |= (BEEP_CLOCK / 8000) - 1;
			}
			
			BEEP->CSR2 &= ~BEEP_CSR2_BEEPSEL;

			switch (*((const uint *)u8p_Value))
			{
				case DRV_BEEP_FREQUENCY_1000:
					BEEP->CSR2 |= 0x00;
					break;
				case DRV_BEEP_FREQUENCY_2000:
					BEEP->CSR2 |= 0x40;
					break;
				case DRV_BEEP_FREQUENCY_4000:
					BEEP->CSR2 |= 0x80;
					break;

				default:
					return FUNCTION_FAIL;
			}

			m_ui_Frequency = *((const uint *)u8p_Value);
			break;
			
		case DRV_BEEP_PARAM_SWITCH:
			
			if (*((const uint *)u8p_Value) == 0)
			{
				if (m_ui_State != DRV_BEEP_STATE_MUTE)
				{
					DrvBEEP_Stop();
					m_ui_State = DRV_BEEP_STATE_MUTE;
				}
			}
			else
			{
				if (m_ui_State == DRV_BEEP_STATE_MUTE)
				{
					m_ui_State = DRV_BEEP_STATE_IDLE;
				}
			}

			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvBEEP_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case DRV_BEEP_PARAM_FREQUENCY:
			*((uint *)u8p_Value) = m_ui_Frequency;
			break;
			
		case DRV_BEEP_PARAM_SWITCH:

			if (m_ui_State == DRV_BEEP_STATE_MUTE)
			{
				*((uint *)u8p_Value) = 0;
			}
			else
			{
				*((uint *)u8p_Value) = 1;
			}

			break;
			
		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


void DrvBEEP_Start
(
	uint ui_BeepCount,
	uint16 u16_OnInterval,
	uint16 u16_OffInterval
)
{
	if ((ui_BeepCount == 0) || (u16_OnInterval == 0) ||
		(m_ui_State == DRV_BEEP_STATE_MUTE))
	{
		return;
	}

	m_ui_BeepCount = ui_BeepCount;
	m_u16_OnInterval = u16_OnInterval;
	m_u16_OffInterval = u16_OffInterval;

	if (m_ui_State == DRV_BEEP_STATE_IDLE)
	{
		m_u16_Timer = 0;
		m_ui_State = DRV_BEEP_STATE_ON;
		BEEP->CSR2 |= BEEP_CSR2_BEEPEN;
	}
}


void DrvBEEP_Stop(void)
{
	if ((m_ui_State == DRV_BEEP_STATE_ON) ||
		(m_ui_State == DRV_BEEP_STATE_OFF))
	{
		m_ui_State = DRV_BEEP_STATE_IDLE;
		BEEP->CSR2 &= ~BEEP_CSR2_BEEPEN;
	}
}


void DrvBEEP_Tick
(
	uint16 u16_TickTime
)
{
	switch (m_ui_State)
	{
		case DRV_BEEP_STATE_ON:
			m_u16_Timer += u16_TickTime;

			if (m_u16_Timer >= m_u16_OnInterval)
			{
				m_u16_Timer = 0;

				if (m_u16_OffInterval == 0)
				{
					m_ui_State = DRV_BEEP_STATE_IDLE;
				}
				else
				{
					m_ui_State = DRV_BEEP_STATE_OFF;
				}

				BEEP->CSR2 &= ~BEEP_CSR2_BEEPEN;
			}

			break;

		case DRV_BEEP_STATE_OFF:
			m_u16_Timer += u16_TickTime;

			if (m_u16_Timer >= m_u16_OffInterval)
			{
				m_u16_Timer = 0;
				m_ui_BeepCount--;

				if (m_ui_BeepCount > 0)
				{
					m_ui_State = DRV_BEEP_STATE_ON;
					BEEP->CSR2 |= BEEP_CSR2_BEEPEN;
				}
				else
				{
					m_ui_State = DRV_BEEP_STATE_IDLE;
				}
			}

			break;

		default:
			break;
	}
}


#if DRV_BEEP_TEST_ENABLE == 1


void DrvBEEP_Test(void)
{

    BEEP->CSR2 |= BEEP_CSR2_BEEPEN;

/*	while (1)
	{
    	BEEP->CSR2 |= BEEP_CSR2_BEEPEN;
		for (i = 0; i < 60000; i++);
		BEEP->CSR2 &= ~BEEP_CSR2_BEEPEN;
		for (i = 0; i < 60000; i++);
	}*/
}

#endif


//Private function definition
