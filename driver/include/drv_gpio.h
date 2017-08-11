/*
 * Module:	General purpose IO driver
 * Author:	Lvjianfeng
 * Date:	2011.8
 */

#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_


#include "global.h"


//Constant define

#define DRV_GPIO_PIN_COUNT			8

#define DRV_GPIO_MASK_PORT			0xF0
#define DRV_GPIO_MASK_PIN			0x0F

#define DRV_GPIO_PORTA				0x00
#define DRV_GPIO_PORTB				0x10
#define DRV_GPIO_PORTC				0x20
#define DRV_GPIO_PORTD				0x30
#define DRV_GPIO_PORTE				0x40
#define DRV_GPIO_PORTF				0x50
#define DRV_GPIO_PORTG				0x60
#define DRV_GPIO_PORTH				0x70
#define DRV_GPIO_PORTI				0x80

#define DRV_GPIO_PIN0				0x00
#define DRV_GPIO_PIN1				0x01
#define DRV_GPIO_PIN2				0x02
#define DRV_GPIO_PIN3				0x03
#define DRV_GPIO_PIN4				0x04
#define DRV_GPIO_PIN5				0x05
#define DRV_GPIO_PIN6				0x06
#define DRV_GPIO_PIN7				0x07
#define DRV_GPIO_PINALL				0x0F

#define DRV_GPIO_TEST_ENABLE		0


//Type definition

typedef enum
{
	DRV_GPIO_PARAM_MODE,
	DRV_GPIO_PARAM_PULLUP,
	DRV_GPIO_PARAM_INTERRUPT,
	DRV_GPIO_PARAM_CALLBACK,
	DRV_GPIO_COUNT_PARAM
} drv_gpio_param;

typedef enum
{
	DRV_GPIO_MODE_INPUT,
	DRV_GPIO_MODE_OUTPUT,
	DRV_GPIO_COUNT_MODE
} drv_gpio_mode;

typedef void (*drv_gpio_callback_interrupt)(void);

typedef struct
{
	drv_gpio_callback_interrupt fp_Interrupt;
} drv_gpio_callback;


//Function declaration

uint DrvGPIO_Initialize(void);
uint DrvGPIO_SetConfig
(
	uint ui_Channel,
	uint ui_Parameter,
	const uint8 *u8p_Value
);
uint DrvGPIO_GetConfig
(
	uint ui_Channel,
	uint ui_Parameter,
	uint8 *u8p_Value
);
uint DrvGPIO_Set
(
	uint ui_Channel
);
uint DrvGPIO_Clear
(
	uint ui_Channel
);
uint DrvGPIO_Toggle
(
	uint ui_Channel
);
uint DrvGPIO_Write
(
	uint ui_Channel,
	uint ui_Value
);
uint DrvGPIO_Read
(
	uint ui_Channel
);
void DrvGPIO_Interrupt
(
	uint ui_Pin
);

#if DRV_GPIO_TEST_ENABLE == 1
void DrvGPIO_Test(void);
#endif

#endif
