/*
 * Module:	Blood glucose meter
 * Author:	Lvjianfeng
 * Date:	2011.12
 */


#include "drv.h"

#include "glucose.h"


//Constant definition

#define GPIO_CHANNEL_PRESENT_N			(DRV_GPIO_PORTB| DRV_GPIO_PIN4)
#define GPIO_CHANNEL_TEST_BG			(DRV_GPIO_PORTI | DRV_GPIO_PIN0)
#define GPIO_CHANNEL_EN_BG				(DRV_GPIO_PORTI| DRV_GPIO_PIN1)
#define GPIO_CHANNEL_GND1				(DRV_GPIO_PORTF| DRV_GPIO_PIN4)
#define GPIO_CHANNEL_GND2				(DRV_GPIO_PORTI| DRV_GPIO_PIN3)
#define GPIO_CHANNEL_HCT1				(DRV_GPIO_PORTI| DRV_GPIO_PIN2)




#define GPIO_CHANNEL_ATC                (DRV_GPIO_PORTF| DRV_GPIO_PIN6)
#define GPIO_CHANNEL_RE2                (DRV_GPIO_PORTB| DRV_GPIO_PIN5)
#define GPIO_CHANNEL_RE3				(DRV_GPIO_PORTB| DRV_GPIO_PIN6)
#define GPIO_CHANNEL_RE6				(DRV_GPIO_PORTB| DRV_GPIO_PIN7)
#define GPIO_CHANNEL_RE8				(DRV_GPIO_PORTB| DRV_GPIO_PIN4)
#define GPIO_CHANNEL_RE10				(DRV_GPIO_PORTC| DRV_GPIO_PIN0)


#define HCT_WAVEFORM_FREQUENCY			3
#define HCT_WAVEFORM_RATIO				100
#define HCT_IMPEDANCE_SAMPLE_COUNT		16
#define HCT_IMPEDANCE_NUMERATOR			16500
#define HCT_IMPEDANCE_DENOMINATOR		1
#define HCT_IMPEDANCE_OFFSET			1650

#define GLUCOSE_REFERENCE_BG_DEFAULT	1005
#define GLUCOSE_REFERENCE_BG_MAX		1056
#define GLUCOSE_REFERENCE_BG_MIN		955
#define GLUCOSE_REFERENCE_HCT_DEFAULT	16500
#define GLUCOSE_REFERENCE_HCT_MAX		18150
#define GLUCOSE_REFERENCE_HCT_MIN		14850

#define GLUCOSE_RATIO_NUMERATOR			768
#define GLUCOSE_RATIO_DENOMINATOR		3768

#define TEMPERATURE_OFFSET				2732
#define TEMPERATURE_SLOPE				162

#define NTC_REFERENCE_RESISTANCE		33000
#define NTC_REFERENCE_RATIO				43

#define DATA_ADDRESS_REFERENCE_BG		((uint16)0x1200)
#define DATA_ADDRESS_REFERENCE_HCT		((uint16)0x1202)


//Type definition

typedef enum
{
	GLUCOSE_FLAG_ENABLE = 0,
	GLUCOSE_COUNT_FLAG
} glucose_flag;






//Private variable definition

static uint m_ui_Flag = {0};
static uint m_ui_Mode = {0};
static uint16 m_u16_ReferenceBG = {0};
static uint16 m_u16_ReferenceHCT = {0};
static const uint16 m_u16_NTC[] = 
{
	27219, 26179, 25140, 24100, 23061, 22021, 21202, 20383, 
	19564, 18745, 17926, 17275, 16625, 15974, 15324, 14674, 
	14155, 13636, 13118, 12599, 12081, 11664, 11248, 10832, 
	10416, 10000, 9663, 9326, 8989, 8652, 8315, 8041, 
	7768, 7495, 7221, 6948, 6725, 6502, 6279, 6056, 
	5834, 5650, 5467, 5284, 5100, 4917, 4755, 4597, 
	4446, 4301, 4161 
};
static const uint16 m_u16_HCT[] =
{
	40000, 39918, 39672, 39266, 38703, 37990, 37135, 36147, 
	35037, 33817, 32500, 31101, 29635, 28119, 26568, 25000, 
	23432, 21881, 20365, 18899, 17500, 16183, 14963, 13853, 
	12865, 12010, 11297, 10734, 10328, 10082, 10000, 10082, 
	10328, 10734, 11297, 12010, 12865, 13853, 14963, 16183, 
	17500, 18899, 20365, 21881, 23432, 25000, 26568, 28119, 
	29635, 31101, 32500, 33817, 35037, 36147, 37135, 37990, 
	38703, 39266, 39672, 39918, 
};

static uint16 m_u16_Re[CHANNEL_RECONUT] = {0};

static uint16 m_u16_HCTWaveform[sizeof(m_u16_HCT) / sizeof(uint16)];


//Private function declaration

static void Glucose_EnableHCTWaveform(void);
static void Glucose_DisableHCTWaveform(void);
static uint16 Glucose_GetBGCurrent
(
	uint ui_Channel
);
static uint16 Glucose_GetImpedance(void);
static uint16 Glucose_GetTemperature(void);
static uint16 Glucose_GetNTC(void);
static uint16 Glucose_GetTestVoltage(void);


uint Glucose_re_initialize(void)
	{
		uint ui_Value;
		uint16 u16_Value;
		
		ui_Value = DRV_GPIO_MODE_INPUT;
	
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
	
		ui_Value = DRV_GPIO_MODE_OUTPUT;
		
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_MODE, 
				(const uint8 *)&ui_Value);
	
		ui_Value = 0;
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
	
		ui_Value = 0;
			DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_PULLUP, 
				(const uint8 *)&ui_Value);
	
		DrvGPIO_Clear(GPIO_CHANNEL_RE10);
	}

uint Glucose_re_back(void)
{
	uint ui_Value;
	uint16 u16_Value;

	ui_Value = DRV_GPIO_MODE_OUTPUT;


	DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);

	DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);

	DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);

	
	ui_Value = 1;

   DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	ui_Value = 0;

 
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
        	

	
    DrvGPIO_Set(GPIO_CHANNEL_ATC);

	
	DrvGPIO_Set(GPIO_CHANNEL_RE2);
	DrvGPIO_Set(GPIO_CHANNEL_RE3);
	DrvGPIO_Set(GPIO_CHANNEL_RE6);
	DrvGPIO_Set(GPIO_CHANNEL_PRESENT_N);
	DrvGPIO_Set(GPIO_CHANNEL_RE10);

	DrvGPIO_Clear(GPIO_CHANNEL_GND1);
 
      

}


uint Glucose_re8_back(void)
	{
		uint ui_Value;
		uint16 u16_Value;
	
		ui_Value = DRV_GPIO_MODE_OUTPUT;
	
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_MODE, 
				(const uint8 *)&ui_Value);
	

	
		
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
	

	
		
		
	
	
		ui_Value = 0;
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_ATC, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE2, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE3, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE6, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
	
		DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);

		
		DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_PULLUP, 
				(const uint8 *)&ui_Value);
	
		
		
		DrvGPIO_SetConfig(GPIO_CHANNEL_RE10, DRV_GPIO_PARAM_PULLUP, 
				(const uint8 *)&ui_Value);

		DrvGPIO_Clear(GPIO_CHANNEL_RE10);
		DrvGPIO_Set(GPIO_CHANNEL_ATC);
		DrvGPIO_Set(GPIO_CHANNEL_RE2);
		DrvGPIO_Set(GPIO_CHANNEL_RE3);
		DrvGPIO_Set(GPIO_CHANNEL_RE6);
		DrvGPIO_Set(GPIO_CHANNEL_PRESENT_N);
		DrvGPIO_Set(GPIO_CHANNEL_GND1);
	
	}


void re_cactu(uint16 *tab)
	{
		uint16 *u16_Value;
		uint i;
		uint32 temp;
		uint32 Val;
		u16_Value=&m_u16_Re[channel_RE10];
		Glucose_re8_back();
		DrvADC_Enable();
		DrvADC_Sample();
		while (DrvADC_Read(DRV_ADC_CHANNEL_RE8,u16_Value)!= FUNCTION_OK)
		{
			;
		}
		Glucose_re_back();
		
		DrvADC_Sample();
		while (DrvADC_Read(DRV_ADC_CHANNEL_RE2, (u16_Value)+1) != FUNCTION_OK)
		{
			;
		}
		while (DrvADC_Read(DRV_ADC_CHANNEL_RE3, (u16_Value+2)) != FUNCTION_OK)
		{
			;
		}
		while (DrvADC_Read(DRV_ADC_CHANNEL_RE6, (u16_Value+3)) != FUNCTION_OK)
		{
			;
		}
		while (DrvADC_Read(DRV_ADC_CHANNEL_RE8, (u16_Value+4)) != FUNCTION_OK)
		{
			;
		}
		temp=m_u16_Re[channel_RE10];
		temp=(3600*temp)/(4096-temp);
		temp=temp-20;
		*tab=(uint16)temp;
                temp=0;
		for(i=1;i<5;i++)
			{
				temp+=((4096-m_u16_Re[i])/18);
			}
                        
                    temp=temp/100;
			temp=temp*28;
		
			
			Val=m_u16_Re[channel_RE2];
			Val=1800*(Val-temp)/(4096-Val);
		*(tab+1)=(uint16)Val;
			Val=m_u16_Re[channel_RE3];
			Val=Val=1800*(Val-temp)/(4096-Val);
		*(tab+2)=Val;
			 Val=m_u16_Re[channel_RE6];
			Val=Val=1800*(Val-temp)/(4096-Val);
		*(tab+3)=Val;
			 Val=m_u16_Re[channel_RE8];
			Val=Val=1800*(Val-temp)/(4096-Val);
		*(tab+4)=Val;
	}




//Public function definition

uint Glucose_Initialize(void)
{
	uint ui_Value;
	uint16 u16_Value;


    
   Glucose_re_initialize();
	ui_Value = DRV_GPIO_MODE_INPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);

	ui_Value = DRV_GPIO_MODE_OUTPUT;
	DrvGPIO_SetConfig(GPIO_CHANNEL_TEST_BG, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_BG, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_HCT1, DRV_GPIO_PARAM_MODE, 
		(const uint8 *)&ui_Value);

	ui_Value = 0;
	DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	ui_Value = 1;
	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_TEST_BG, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_EN_BG, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);
	DrvGPIO_SetConfig(GPIO_CHANNEL_HCT1, DRV_GPIO_PARAM_PULLUP, 
		(const uint8 *)&ui_Value);

	DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_INTERRUPT, 
		(const uint8 *)&ui_Value);

	DrvGPIO_Clear(GPIO_CHANNEL_TEST_BG);
	DrvGPIO_Clear(GPIO_CHANNEL_EN_BG);
	DrvGPIO_Clear(GPIO_CHANNEL_HCT1);

	Glucose_re_initialize();

	u16_Value = HCT_WAVEFORM_FREQUENCY * (sizeof(m_u16_HCT) / sizeof(uint16));
	DrvDAC_SetConfig(DRV_DAC_PARAM_FREQUENCY, (const uint8 *)&u16_Value,
		sizeof(u16_Value));

	u16_Value = sizeof(m_u16_ReferenceBG);
	DrvFLASH_Read(DATA_ADDRESS_REFERENCE_BG, (uint8 *)&m_u16_ReferenceBG, 
		&u16_Value);

	if ((m_u16_ReferenceBG < GLUCOSE_REFERENCE_BG_MIN) ||
		(m_u16_ReferenceBG > GLUCOSE_REFERENCE_BG_MAX))
	{
		m_u16_ReferenceBG = GLUCOSE_REFERENCE_BG_DEFAULT;
	}

	u16_Value = sizeof(m_u16_ReferenceHCT);
	DrvFLASH_Read(DATA_ADDRESS_REFERENCE_HCT, (uint8 *)&m_u16_ReferenceHCT, 
		&u16_Value);

	if ((m_u16_ReferenceHCT < GLUCOSE_REFERENCE_HCT_MIN) ||
		(m_u16_ReferenceHCT > GLUCOSE_REFERENCE_HCT_MAX))
	{
		m_u16_ReferenceHCT = GLUCOSE_REFERENCE_HCT_DEFAULT;
	}

	return FUNCTION_OK;
}


uint Glucose_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	uint ui_Value;


	switch (ui_Parameter)
	{
		case GLUCOSE_PARAM_MODE:

			switch (*((const uint*)u8p_Value))
			{
				case GLUCOSE_MODE_OFF:
					DrvGPIO_Clear(GPIO_CHANNEL_HCT1);
					DrvGPIO_Clear(GPIO_CHANNEL_EN_BG);
					ui_Value = DRV_GPIO_MODE_OUTPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					break;

				case GLUCOSE_MODE_BG1:
					DrvGPIO_Clear(GPIO_CHANNEL_HCT1);
					DrvGPIO_Set(GPIO_CHANNEL_EN_BG);
					ui_Value = DRV_GPIO_MODE_INPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					ui_Value = DRV_GPIO_MODE_OUTPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					break;

				case GLUCOSE_MODE_BG2:
					DrvGPIO_Clear(GPIO_CHANNEL_HCT1);
					DrvGPIO_Set(GPIO_CHANNEL_EN_BG);
					ui_Value = DRV_GPIO_MODE_OUTPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					ui_Value = DRV_GPIO_MODE_INPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					break;

				case GLUCOSE_MODE_HCT:
					DrvGPIO_Set(GPIO_CHANNEL_HCT1);
					DrvGPIO_Clear(GPIO_CHANNEL_EN_BG);
					ui_Value = DRV_GPIO_MODE_INPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND1, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					ui_Value = DRV_GPIO_MODE_OUTPUT;
					DrvGPIO_SetConfig(GPIO_CHANNEL_GND2, DRV_GPIO_PARAM_MODE, 
						(const void *)&ui_Value);
					break;

				default:
					break;
			}

			m_ui_Mode = *((const uint *)u8p_Value);
			break;

		case GLUCOSE_PARAM_SIGNAL_PRESENT:
			DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_PULLUP, 
				u8p_Value);
			break;

		case GLUCOSE_PARAM_GLUCOSE_CHANNEL:
			
			if (*((const uint *)u8p_Value) != 0)
			{
				DrvGPIO_Set(GPIO_CHANNEL_TEST_BG);
			}
			else
			{
				DrvGPIO_Clear(GPIO_CHANNEL_TEST_BG);
			}

			break;

		case GLUCOSE_PARAM_REFERENCE_BG:

			if ((*((const uint16 *)u8p_Value) < GLUCOSE_REFERENCE_BG_MIN) ||
				(*((const uint16 *)u8p_Value) > GLUCOSE_REFERENCE_BG_MAX))
			{
				return FUNCTION_FAIL;
			}

			m_u16_ReferenceBG = *((const uint16 *)u8p_Value);
			DrvFLASH_Erase(DATA_ADDRESS_REFERENCE_BG, sizeof(m_u16_ReferenceBG));
			DrvFLASH_Write(DATA_ADDRESS_REFERENCE_BG, 
				(const uint8 *)&m_u16_ReferenceBG, sizeof(m_u16_ReferenceBG));
			break;

		case GLUCOSE_PARAM_REFERENCE_HCT:

			if ((*((const uint16 *)u8p_Value) < GLUCOSE_REFERENCE_HCT_MIN) ||
				(*((const uint16 *)u8p_Value) > GLUCOSE_REFERENCE_HCT_MAX))
			{
				return FUNCTION_FAIL;
			}

			m_u16_ReferenceHCT = *((const uint16 *)u8p_Value);
			DrvFLASH_Erase(DATA_ADDRESS_REFERENCE_HCT, sizeof(m_u16_ReferenceHCT));
			DrvFLASH_Write(DATA_ADDRESS_REFERENCE_HCT, 
				(const uint8 *)&m_u16_ReferenceHCT, sizeof(m_u16_ReferenceHCT));
			break;

		case GLUCOSE_PARAM_HCT_WAVEFORM:

			if (*((const uint *)u8p_Value) != 0)
			{
				Glucose_EnableHCTWaveform();
			}
			else
			{
				Glucose_DisableHCTWaveform();
			}

			break;

		case GLUCOSE_PARAM_CALLBACK:
			DrvGPIO_SetConfig(GPIO_CHANNEL_PRESENT_N, DRV_GPIO_PARAM_CALLBACK, 
				u8p_Value);
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Glucose_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	switch (ui_Parameter)
	{
		case GLUCOSE_PARAM_MODE:
			*(uint *)u8p_Value = m_ui_Mode;
			break;

		case GLUCOSE_PARAM_SWITCH:
			
			if (REG_GET_BIT(m_ui_Flag, GLUCOSE_FLAG_ENABLE) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		case GLUCOSE_PARAM_SIGNAL_PRESENT:
			*((uint *)u8p_Value) = DrvGPIO_Read(GPIO_CHANNEL_PRESENT_N);
			break;

		case GLUCOSE_PARAM_REFERENCE_BG:
			*((uint16 *)u8p_Value) = m_u16_ReferenceBG;
			break;

		case GLUCOSE_PARAM_REFERENCE_HCT:
			*((uint16 *)u8p_Value) = m_u16_ReferenceHCT;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint16 Glucose_Read
(
	uint ui_Channel
)
{
	switch (ui_Channel)
	{
		case GLUCOSE_CHANNEL_BG:
			return Glucose_GetBGCurrent(DRV_ADC_CHANNEL_BG);

		case GLUCOSE_CHANNEL_HCT:
			return Glucose_GetImpedance();

		case GLUCOSE_CHANNEL_TEMPERATURE:
			return Glucose_GetTemperature();

		case GLUCOSE_CHANNEL_NTC:
			return Glucose_GetNTC();

		case GLUCOSE_CHANNEL_TEST:
			return Glucose_GetTestVoltage();

		default:
			return 0;
	}
}


void Glucose_Enable(void)
{
	DrvADC_Enable();
	REG_SET_BIT(m_ui_Flag, GLUCOSE_FLAG_ENABLE);
}


void Glucose_Disable(void)
{
	DrvADC_Disable();
	REG_CLEAR_BIT(m_ui_Flag, GLUCOSE_FLAG_ENABLE);
}


void Glucose_Sample(void)
{
	DrvADC_Sample();
}


sint32 Glucose_Round
(
	sint32 s32_Value,
	sint32 s32_Base
)
{
	if (s32_Value < 0)
	{
		return (s32_Value - (s32_Base / 2)) / s32_Base;
	}
	else if (s32_Value > 0)
	{
		return (s32_Value + (s32_Base / 2)) / s32_Base;
	}
	else
	{
		return 0;
	}
}


#if GLUCOSE_TEST_ENABLE == 1

void Glucose_Test(void)
{
}

#endif


//Private function definition

static void Glucose_EnableHCTWaveform(void)
{
	uint i;
	uint16 u16_Value;

	DrvADC_Sample();
	Drv_GetConfig(DRV_PARAM_VOLTAGE_VDD, (uint8 *)&u16_Value, (uint *)0);

	if (u16_Value == 0)
	{
		return;
	}

	for (i = 0; i < sizeof(m_u16_HCT) / sizeof(uint16); i++)
	{
		m_u16_HCTWaveform[i] = (uint16)(((uint32)m_u16_HCT[i] * 
			((1 << DRV_DAC_RESOLUTION) - 1)) / ((uint32)u16_Value * 
			HCT_WAVEFORM_RATIO));
	}

	DrvDAC_Enable(m_u16_HCTWaveform, sizeof(m_u16_HCTWaveform) / sizeof(uint16));
}


static void Glucose_DisableHCTWaveform(void)
{
	DrvDAC_Disable();
}


static uint16 Glucose_GetBGCurrent
(
	uint ui_Channel
)
{
	uint16 u16_ValueRef;
	uint16 u16_ValueBG;
	uint16 u16_VoltageRef;
	uint32 u32_CurrentBG;
	uint32 u32_Reference;


	while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_ValueRef) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(ui_Channel, &u16_ValueBG) != FUNCTION_OK)
	{
		;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_VoltageRef,
		(uint *)0);
	u32_CurrentBG = (uint32)u16_ValueBG * GLUCOSE_RATIO_DENOMINATOR;
	u32_Reference = (uint32)u16_ValueRef * GLUCOSE_RATIO_NUMERATOR;

	if (u32_CurrentBG > u32_Reference)
	{
		u32_CurrentBG = u32_CurrentBG - u32_Reference;

		if (u32_CurrentBG < (~0 / (uint32)u16_VoltageRef))
		{
			u32_CurrentBG = (u32_CurrentBG * (uint32)u16_VoltageRef) / 
				((uint32)u16_ValueRef * GLUCOSE_RATIO_DENOMINATOR);
		}
		else
		{
			u32_CurrentBG = (((u32_CurrentBG / (uint32)u16_ValueRef) * 
				(uint32)u16_VoltageRef) / GLUCOSE_RATIO_DENOMINATOR);
		}
	}
	else
	{
		u32_CurrentBG = 0;
	}

	u32_CurrentBG = u32_CurrentBG * GLUCOSE_REFERENCE_BG_DEFAULT / 
		(uint32)m_u16_ReferenceBG;

	if (u32_CurrentBG > (uint32)((uint16)~0))
	{
		return (uint16)~0;
	}
	else
	{
		return (uint16)u32_CurrentBG;
	}
}


static uint16 Glucose_GetImpedance(void)
{
	uint i;
	uint16 u16_Result;
	uint16 u16_Reference;
	uint32 u32_Impedance;


	u16_Reference = 0;
	u32_Impedance = 0;

	for (i = 0; i < HCT_IMPEDANCE_SAMPLE_COUNT; i++)
	{
		DrvADC_Sample();

		while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_Result) != FUNCTION_OK)
		{
			;
		}

		u16_Reference += u16_Result;

		while (DrvADC_Read(DRV_ADC_CHANNEL_HCT, &u16_Result) != FUNCTION_OK)
		{
			;
		}

		u32_Impedance += (uint32)u16_Result;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_Result,
		(uint *)0);
	u32_Impedance = (uint32)DRV_ADC_GET_VOLTAGE(u16_Result, u16_Reference, 
		u32_Impedance);

	if (u32_Impedance < (HCT_IMPEDANCE_OFFSET / HCT_IMPEDANCE_DENOMINATOR))
	{
		u32_Impedance = (u32_Impedance * HCT_IMPEDANCE_NUMERATOR) / 
			(HCT_IMPEDANCE_OFFSET - (u32_Impedance * HCT_IMPEDANCE_DENOMINATOR));

		if (u32_Impedance < (uint32)((uint16)~0))
		{
			u32_Impedance = (u32_Impedance * GLUCOSE_REFERENCE_HCT_DEFAULT) /
				(uint32)m_u16_ReferenceHCT;
		}
		else
		{
			u32_Impedance = (uint32)~0;
		}
	}
	else
	{
		u32_Impedance = (uint32)~0;
	}

	return (uint16)u32_Impedance;
}


static uint16 Glucose_GetTemperature(void)
{
	uint16 u16_Reference;
	uint16 u16_Temperature;
	uint16 u16_VoltageRef;


	while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_Reference) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(DRV_ADC_CHANNEL_TEMP, &u16_Temperature) != FUNCTION_OK)
	{
		;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_VoltageRef,
		(uint *)0);
	u16_Temperature = (uint16)((((uint32)u16_VoltageRef * 
		(100 * GLUCOSE_TEMPERATURE_RATIO)) * (uint32)u16_Temperature) / 
		((uint32)u16_Reference * TEMPERATURE_SLOPE) - TEMPERATURE_OFFSET);

	return u16_Temperature;
}


static uint16 Glucose_GetNTC(void)
{
	uint i;
	uint16 u16_VoltageVDD;
	uint16 u16_NTC;


	while (DrvADC_Read(DRV_ADC_CHANNEL_VDD, &u16_VoltageVDD) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(DRV_ADC_CHANNEL_NTC, &u16_NTC) != FUNCTION_OK)
	{
		;
	}

	u16_NTC *= GLUCOSE_TEMPERATURE_RATIO;
	u16_NTC = (uint16)((NTC_REFERENCE_RESISTANCE * (uint32)u16_NTC) / 
		(uint32)(u16_VoltageVDD * NTC_REFERENCE_RATIO - u16_NTC));

	if (u16_NTC >= m_u16_NTC[0])
	{
		u16_NTC = 0;
	}
	else if (u16_NTC <= m_u16_NTC[(sizeof(m_u16_NTC) / sizeof(uint16)) - 1])
	{
		u16_NTC = ((sizeof(m_u16_NTC) / sizeof(uint16)) - 1) * 
			GLUCOSE_TEMPERATURE_RATIO;
	}
	else
	{
		for (i = 0; i < (sizeof(m_u16_NTC) / sizeof(uint16)) - 1; i++)
		{
			if ((u16_NTC < m_u16_NTC[i]) && (u16_NTC >= m_u16_NTC[i + 1]))
			{
				u16_NTC = ((m_u16_NTC[i] - u16_NTC) * GLUCOSE_TEMPERATURE_RATIO) / 
					(m_u16_NTC[i] - m_u16_NTC[i + 1]) + 
					(i * GLUCOSE_TEMPERATURE_RATIO);
				break;
			}
		}
	}

	return u16_NTC;
}


static uint16 Glucose_GetTestVoltage(void)
{
	uint16 u16_Reference;
	uint16 u16_VoltageRef;
	uint16 u16_VoltageBG;


	while (DrvADC_Read(DRV_ADC_CHANNEL_REF, &u16_Reference) != FUNCTION_OK)
	{
		;
	}

	while (DrvADC_Read(DRV_ADC_CHANNEL_BG, &u16_VoltageBG) != FUNCTION_OK)
	{
		;
	}

	DrvADC_GetConfig(DRV_ADC_PARAM_REFERENCE, (uint8 *)&u16_VoltageRef,
		(uint *)0);

	return DRV_ADC_GET_VOLTAGE(u16_VoltageRef, u16_Reference, u16_VoltageBG);
}
