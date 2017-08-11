/*
 * Module:	LCD driver
 * Author:	Lvjianfeng
 * Date:	2012.8
 */


#include "stm8l15x.h"

#include "drv.h"


//Constant definition

#define CHARACTER_SEGMENT_COUNT			9
#define RAM_MAP_COM_HIGH				4
#define SYMBOL_COUNT					1
#define LCD_OFFSET_FIRST_SYMBOL			DRV_LCD_OFFSET_DATE_BAR
#define BLINK_CYCLE_ON					500
#define BLINK_CYCLE_OFF					500

#define LCD_RAM_PAGE_MASK				0xF0
#define LCD_RAM_DATA_COUNT				22


//Type definition

typedef enum
{
	DRV_LCD_FLAG_BLINK = 0,
	DRV_LCD_COUNT_FLAG
} drv_lcd_flag;

typedef struct
{
	uint8 u8_Address;
	uint8 u8_Offset;
} drv_lcd_ram_map;


//Private variable definition

static uint m_ui_BlinkToggle = {0};
static uint m_ui_LCDFlag[DRV_LCD_COUNT_OFFSET] = {0};
static uint8 m_u8_LCDData[DRV_LCD_COUNT_OFFSET] = {0};
static uint16 m_u16_BlinkTimer = {0};
static uint16 m_u16_BlinkCycle = {0};

static const drv_lcd_ram_map m_t_CharacterRAMMap[LCD_OFFSET_FIRST_SYMBOL][CHARACTER_SEGMENT_COUNT] =
{
	//Month tens
	{
		{19, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Month units
	{
		{15, 0x03}, {17, 0x03}, {19, 0x03}, {21, 0x03}, 
		{15, 0x02}, {17, 0x02}, {21, 0x02}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Day tens
	{
		{15, 0x01}, {17, 0x01}, {19, 0x01}, {21, 0x01}, 
		{15, 0x00}, {17, 0x00}, {21, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Day units
	{
		{3, 0x03}, {6, 0x07}, {10, 0x03}, {13, 0x07}, 
		{3, 0x02}, {6, 0x06}, {13, 0x06}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Hour tens
	{
		{LCD_RAM_DATA_COUNT, 0x00}, {14, 0x07}, {16, 0x07}, {20, 0x07}, 
		{14, 0x06}, {16, 0x06}, {20, 0x06}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Hour units
	{
		{14, 0x05}, {16, 0x05}, {18, 0x05}, {20, 0x05}, 
		{14, 0x04}, {16, 0x04}, {20, 0x04}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Minute tens
	{
		{2, 0x02}, {5, 0x06}, {9, 0x02}, {12, 0x06}, 
		{2, 0x03}, {5, 0x07}, {12, 0x07}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Minute units
	{
		{15, 0x07}, {17, 0x07}, {19, 0x07}, {21, 0x07}, 
		{1, 0x04}, {5, 0x00}, {12, 0x00}, {LCD_RAM_DATA_COUNT, 0x00}, 
		{LCD_RAM_DATA_COUNT, 0x00}
	},
	//Glucose Tens
	{
		{18, 0x03}, {14, 0x03}, {0, 0x01}, {7, 0x01}, 
		{3, 0x05}, {0, 0x02}, {7, 0x02}, {16, 0x03}, 
		{3, 0x06}
	},
	//Glucose units
	{
		{7, 0x03}, {0, 0x03}, {0, 0x04}, {7, 0x04}, 
		{4, 0x00}, {0, 0x05}, {7, 0x05}, {3, 0x07},
		{4, 0x01}
	},
	//Glucose fraction
	{
		{7, 0x06}, {0, 0x06}, {0, 0x07}, {7, 0x07}, 
		{4, 0x03}, {1, 0x00}, {8, 0x00}, {4, 0x02},
		{4, 0x04}
	}
};
static const drv_lcd_ram_map m_t_SymbolRAMMap[DRV_LCD_COUNT_OFFSET - LCD_OFFSET_FIRST_SYMBOL] =
{
	{19, 0x02}, {18, 0x04}, {11, 0x02}, {8, 0x04}, 
	{9, 0x03}, {10, 0x02}, {11, 0x07}, {11, 0x06},
	{18, 0x06}, {8, 0x02}, {4, 0x06}, {1, 0x02}, 
	{1, 0x03}, {4, 0x07}, {8, 0x03}, {8, 0x01},
	{4, 0x05}, {1, 0x01}, {11, 0x05}, {20, 0x03}, 
	{10, 0x05}, {10, 0x06}, {11, 0x03}, {11, 0x04}
};
static const uint16 m_u16_CharacterBitmap[DRV_LCD_COUNT_CHARACTER] =
{
	//Character "0"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "1"
	(1 << 5) | (1 << 6) | (1 << 8),
	//Character "2"
	(1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "3"
	(1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 8),
	//Character "4"
	(1 << 0) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "5"
	(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 8),
	//Character "6"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 8),
	//Character "7"
	(1 << 3) | (1 << 5) | (1 << 6) | (1 << 8),
	//Character "8"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "9"
	(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "A" 
	(1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "b"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 8),
	//Character "C"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 7),
	//Character "d"
	(1 << 1) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "E"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 7),
	//Character "F"
	(1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 7),
	//Character "G"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8),
	//Character "H"
	(1 << 0) | (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "I"
	(1 << 0) | (1 << 1) | (1 << 7),
	//Character "J"
	(1 << 2) | (1 << 5) | (1 << 6) | (1 << 8),
	//Character "L"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 7),
	//Character "n"
	(1 << 1) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 8),
	//Character "O"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "P"
	(1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "q"
	(1 << 0) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "r"
	(1 << 1) | (1 << 4) | (1 << 7),
	//Character "t"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 4) | (1 << 7),
	//Character "U"
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "y"
	(1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8),
	//Character "-"
	(1 << 4),
	//Character all off
	0,
	//Character all on
	(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8)
};


//Private function declaration

static void DrvLCD_ResetLCD
(
	uint8 u8_Data
);
static void DrvLCD_WriteLCDRAM
(
	uint8 u8_Data,
	const drv_lcd_ram_map *tp_RAMMap
);
static void DrvLCD_WriteCharacter
(
	uint8 u8_Index,
	uint8 u8_Data	
);


//Public function definition

uint DrvLCD_Initialize(void)
{
	uint i;


	for (i = 0; i < LCD_OFFSET_FIRST_SYMBOL; i++)
	{
		m_u8_LCDData[i] = DRV_LCD_CHARACTER_ALL_OFF;
	}

	for (i = LCD_OFFSET_FIRST_SYMBOL; i < DRV_LCD_COUNT_OFFSET; i++)
	{
		m_u8_LCDData[i] = DRV_LCD_SYMBOL_OFF;
	}

	/* Enable LCD clock */
	CLK->PCKENR2 |= CLK_PCKENR2_LCD;

	/* Initialize the LCD */
	LCD->FRQ &= ~LCD_FRQ_PS;    /* Clear the prescaler bits */
	LCD->FRQ |= 0x20;
	LCD->CR1 &= ~LCD_CR1_B2;    /* Clear the B2 bit */
	LCD->CR1 &= ~LCD_CR1_DUTY;  /* Clear the prescaler bits */
	LCD->CR1 |= 0x06;
	LCD->CR2 &= ~LCD_CR2_VSEL;  /* Clear the voltage source bit */
	LCD->CR2 &= ~LCD_CR2_CC;    /* Clear the contrast bits  */
	LCD->CR2 |= 0x0A;           /* Select the maximum voltage value Vlcd */
	LCD->CR2 |= LCD_CR2_HD;     /* Permanently enable low resistance divider */

	/* Mask register*/
	LCD->PM[0] = 0xFE;
	LCD->PM[1] = 0x1F;
	LCD->PM[2] = 0x0C;
	LCD->PM[3] = 0x8C;
	LCD->PM[4] = 0xFF;
	LCD->PM[5] = 0x08;

	return FUNCTION_OK;
}


uint DrvLCD_SetConfig
(
	uint ui_Offset,
	uint ui_Parameter,
	const uint8 *u8p_Value
)
{
	if (ui_Offset >= DRV_LCD_COUNT_OFFSET)
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Parameter)
	{
		case DRV_LCD_PARAM_BLINK:
			
			if (*((uint *)u8p_Value) != 0)
			{
				REG_SET_BIT(m_ui_LCDFlag[ui_Offset], DRV_LCD_FLAG_BLINK);
			}
			else
			{
				REG_CLEAR_BIT(m_ui_LCDFlag[ui_Offset], DRV_LCD_FLAG_BLINK);
				
				if (m_ui_BlinkToggle == 0)
				{
					if (ui_Offset < LCD_OFFSET_FIRST_SYMBOL)
					{
						DrvLCD_WriteCharacter(ui_Offset, m_u8_LCDData[ui_Offset]);
					}
					else
					{
						DrvLCD_WriteLCDRAM(m_u8_LCDData[ui_Offset], 
							&m_t_SymbolRAMMap[ui_Offset - LCD_OFFSET_FIRST_SYMBOL]);
					}
				}
			}

			break;

		case DRV_LCD_PARAM_RESET:
			DrvLCD_ResetLCD(*((const uint8 *)u8p_Value));
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvLCD_GetConfig
(
	uint ui_Offset,
	uint ui_Parameter,
	uint8 *u8p_Value
)
{
	if (ui_Offset >= DRV_LCD_COUNT_OFFSET)
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Parameter)
	{
		case DRV_LCD_PARAM_BLINK:
			
			if (REG_GET_BIT(m_ui_LCDFlag[ui_Offset], DRV_LCD_FLAG_BLINK) != 0)
			{
				*((uint *)u8p_Value) = 1;
			}
			else
			{
				*((uint *)u8p_Value) = 0;
			}

			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint DrvLCD_Write
(
	uint ui_Offset,
	const uint8 *u8p_Data,
	uint ui_Length
)
{
	uint i;
	uint ui_OffsetEnd;


	ui_OffsetEnd = ui_Offset + ui_Length;

	if (ui_OffsetEnd > DRV_LCD_COUNT_OFFSET)
	{
		return FUNCTION_FAIL;
	}

	for (i = ui_Offset; i < ui_OffsetEnd; i++)
	{
		m_u8_LCDData[i] = *u8p_Data;

		if (i < LCD_OFFSET_FIRST_SYMBOL)
		{
			DrvLCD_WriteCharacter(i, *u8p_Data);
		}
		else
		{
			DrvLCD_WriteLCDRAM(*u8p_Data, &m_t_SymbolRAMMap[i - 
				LCD_OFFSET_FIRST_SYMBOL]);
		}

		u8p_Data++;
	}

	return FUNCTION_OK;
}


uint DrvLCD_Read
(
	uint ui_Offset,
	uint8 *u8p_Data,
	uint *uip_Length
)
{
	uint i;
	uint ui_OffsetEnd;


	ui_OffsetEnd = ui_Offset + *uip_Length;

	if (ui_OffsetEnd > DRV_LCD_COUNT_OFFSET)
	{
		if (ui_Offset < DRV_LCD_COUNT_OFFSET)
		{
			*uip_Length = DRV_LCD_COUNT_OFFSET - ui_Offset;
			ui_OffsetEnd = DRV_LCD_COUNT_OFFSET;
		}
		else
		{
			return FUNCTION_FAIL;
		}
	}

	for (i = ui_Offset; i < ui_OffsetEnd; i++)
	{
		*u8p_Data = m_u8_LCDData[i];
		u8p_Data++;
	}

	return FUNCTION_OK;
}


void DrvLCD_Enable(void)
{
	LCD->CR3 |= LCD_CR3_LCDEN;
}


void DrvLCD_Disable(void)
{
	LCD->CR3 &= ~LCD_CR3_LCDEN;
}


void DrvLCD_Interrupt(void)
{
}


void DrvLCD_Tick
(
	uint16 u16_TickTime
)
{
	uint i;


	m_u16_BlinkTimer += u16_TickTime;

	if (m_u16_BlinkTimer >= m_u16_BlinkCycle)
	{
		m_u16_BlinkTimer = 0;
		m_ui_BlinkToggle = ~m_ui_BlinkToggle;

		for (i = 0; i < DRV_LCD_COUNT_OFFSET; i++)
		{
			if (REG_GET_BIT(m_ui_LCDFlag[i], DRV_LCD_FLAG_BLINK) != 0)
			{
				if (m_ui_BlinkToggle != 0)
				{
					if (i < LCD_OFFSET_FIRST_SYMBOL)
					{
						DrvLCD_WriteCharacter(i, m_u8_LCDData[i]);
					}
					else
					{
						DrvLCD_WriteLCDRAM(DRV_LCD_SYMBOL_ON, 
							&m_t_SymbolRAMMap[i - LCD_OFFSET_FIRST_SYMBOL]);
					}

					m_u16_BlinkCycle = BLINK_CYCLE_ON;
				}
				else
				{
					if (i < LCD_OFFSET_FIRST_SYMBOL)
					{
						DrvLCD_WriteCharacter(i, DRV_LCD_CHARACTER_ALL_OFF);
					}
					else
					{
						DrvLCD_WriteLCDRAM(DRV_LCD_SYMBOL_OFF, 
							&m_t_SymbolRAMMap[i - LCD_OFFSET_FIRST_SYMBOL]);
					}

					m_u16_BlinkCycle = BLINK_CYCLE_OFF;
				}
			}
		}
	}
}


#if DRV_LCD_TEST_ENABLE == 1

void DrvLCD_Test(void)
{
	uint ui_Value;
	uint8 u8_Data;

/*	ui_Value = 1;
	DrvLCD_SetConfig(DRV_LCD_OFFSET_DAY_TENS, DRV_LCD_PARAM_BLINK, 
		(const void *)&ui_Value);
	DrvLCD_SetConfig(DRV_LCD_OFFSET_DAY_UNITS, DRV_LCD_PARAM_BLINK, 
		(const void *)&ui_Value);*/
	u8_Data = DRV_LCD_CHARACTER_E;
	DrvLCD_Write(DRV_LCD_OFFSET_MONTH_TENS, &u8_Data, sizeof(u8_Data));
	u8_Data = DRV_LCD_CHARACTER_5;
	DrvLCD_Write(DRV_LCD_OFFSET_DAY_TENS, &u8_Data, sizeof(u8_Data));
}

#endif


//Private function definition

static void DrvLCD_ResetLCD
(
	uint8 u8_Data
)
{
	uint i;


	for (i = 0; i < LCD_RAM_DATA_COUNT; i++)
	{
		LCD->RAM[i] = u8_Data;
	}

	for (i = 0; i < DRV_LCD_COUNT_OFFSET; i++)
	{
		REG_CLEAR_BIT(m_ui_LCDFlag[i], DRV_LCD_FLAG_BLINK);
	}

	if (u8_Data == DRV_LCD_DATA_ALL_OFF)
	{
		for (i = 0; i < LCD_OFFSET_FIRST_SYMBOL; i++)
		{
			m_u8_LCDData[i] = DRV_LCD_CHARACTER_ALL_OFF;
		}

		for (i = LCD_OFFSET_FIRST_SYMBOL; i < DRV_LCD_COUNT_OFFSET; i++)
		{
			m_u8_LCDData[i] = DRV_LCD_SYMBOL_OFF;
		}
	}
	else if (u8_Data == DRV_LCD_DATA_ALL_ON)
	{
		for (i = 0; i < LCD_OFFSET_FIRST_SYMBOL; i++)
		{
			m_u8_LCDData[i] = DRV_LCD_CHARACTER_ALL_ON;
		}

		for (i = LCD_OFFSET_FIRST_SYMBOL; i < DRV_LCD_COUNT_OFFSET; i++)
		{
			m_u8_LCDData[i] = DRV_LCD_SYMBOL_ON;
		}
	}
}


static void DrvLCD_WriteLCDRAM
(
	uint8 u8_Data,
	const drv_lcd_ram_map *tp_RAMMap
)
{
	if (tp_RAMMap->u8_Address < LCD_RAM_DATA_COUNT)
	{	
		if (REG_GET_BIT(u8_Data, 0) != 0)
		{
			REG_SET_BIT(LCD->RAM[tp_RAMMap->u8_Address], 
				tp_RAMMap->u8_Offset);
		}
		else
		{
			REG_CLEAR_BIT(LCD->RAM[tp_RAMMap->u8_Address], 
				tp_RAMMap->u8_Offset);
		}
	}
}


static void DrvLCD_WriteCharacter
(
	uint8 u8_Index,
	uint8 u8_Data	
)
{
	uint i;
	uint16 u16_Bitmap;


	if (u8_Data < DRV_LCD_COUNT_CHARACTER)
	{
		if (u8_Index == DRV_LCD_OFFSET_MONTH_TENS)
		{
			if ((u8_Data == DRV_LCD_CHARACTER_1) || 
				(u8_Data == DRV_LCD_CHARACTER_ALL_ON))
			{
				DrvLCD_WriteLCDRAM(DRV_LCD_SYMBOL_ON, 
					&m_t_CharacterRAMMap[u8_Index][0]);
			}
			else
			{
				DrvLCD_WriteLCDRAM(DRV_LCD_SYMBOL_OFF, 
					&m_t_CharacterRAMMap[u8_Index][0]);
			}
		}
		else
		{
			u16_Bitmap = m_u16_CharacterBitmap[u8_Data];

			for (i = 0; i < CHARACTER_SEGMENT_COUNT; i++)
			{
				DrvLCD_WriteLCDRAM((uint8)u16_Bitmap, 
					&m_t_CharacterRAMMap[u8_Index][i]);
				u16_Bitmap >>= 1;
			}
		}
	}
}
