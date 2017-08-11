/*
 * Module:	Hardware driver
 * Author:	jingfanli
 * Date:	2017.8
 */


#include "drv.h"

#include "drv_gpio.h"
#include "drv_voice.h"
#include "stm8l15x_tim4.h"

#define voice_data             (DRV_GPIO_PORTC| DRV_GPIO_PIN3)
#define voice_busy             (DRV_GPIO_PORTC| DRV_GPIO_PIN1)
#define voice_reset            (DRV_GPIO_PORTC| DRV_GPIO_PIN2)


static uint8 voice_flag=0;
static uint8 voice_reset_flag=0;



static uint16 voice_settime=0;
static uint8 voice_count=0;
static uint16 voice_runtime=0;
static uint8 voice_status=0;
static uint8 voice_dataflag=0;
static uint8 voice_merage1=15;




void VOICE_init(void)
{		uint ui_Value;
		
		ui_Value = DRV_GPIO_MODE_INPUT;
		DrvGPIO_SetConfig(voice_busy, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		ui_Value=DRV_GPIO_MODE_OUTPUT;
		DrvGPIO_SetConfig(voice_data, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(voice_reset, DRV_GPIO_PARAM_MODE, 
			(const uint8 *)&ui_Value);

		ui_Value=1;
		DrvGPIO_SetConfig(voice_data, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		DrvGPIO_SetConfig(voice_reset, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);
		ui_Value=0;
		DrvGPIO_SetConfig(voice_busy, DRV_GPIO_PARAM_PULLUP, 
			(const uint8 *)&ui_Value);

		DrvGPIO_Clear(voice_data);
		DrvGPIO_Clear(voice_reset);
	
}
/*
void VOICE_program
	(
		uint8 voicecount,
		uint16 settingtime
		 
		
	)
{

	
	switch(voicecount)
		{
			case strip_in :
				VOICE_Start(2,settingtime);
				break;
			case result_out:
				voice_merage1=result_out;

				break;
			case hight_bloodsugar:
				VOICE_Start(4,settingtime);
				break;
			case low_bloddsuguar:
				VOICE_Start(5,settingtime);
				break;
			case blood_ketone:
				VOICE_Start(6,settingtime);
				break;
			case power_off:
				VOICE_Start(7,settingtime);
				break;
			case error:
				VOICE_Start(8,settingtime);
				break;
			default : break;
				
		}

}
*/
void VOICE_Tick
(
	uint16 u16_TickTime
)
{
	if (voice_flag==1)
		{
			voice_runtime++;
			if(voice_reset_flag==0)
				{
					DrvGPIO_Set(voice_reset);
					if(voice_runtime>voice_settime)
						{
							DrvGPIO_Clear(voice_reset);
							voice_reset_flag=1;
							voice_runtime=0;
						}

				}
			if((voice_runtime>voice_settime)&&(voice_reset_flag==1)&&(voice_dataflag==0))
				{
							voice_runtime=0;
							voice_status=VOICE_ON;
							voice_dataflag=1;


		
				}
			if(voice_dataflag)
				{
			switch (voice_status)
				{
					case VOICE_ON:
						DrvGPIO_Set(voice_data);
						if(voice_runtime>voice_settime)
							{
								voice_status=VOICE_OFF;
								voice_runtime=0;
								
							}
						break;
					case VOICE_OFF:
						DrvGPIO_Clear(voice_data);
						if(voice_runtime>voice_settime)
							{
								
								voice_runtime=0;
								if(voice_count>0)
									{
										voice_count--;
										voice_status=VOICE_ON;
									}
								else 
									{
										voice_dataflag=0;
										voice_reset_flag=0;
										voice_flag=0;
										voice_runtime=0;
										DrvGPIO_Clear(voice_data);
										DrvGPIO_Clear(voice_reset);
										Voice_closed();
									}
								
							}
						break;

						default:break;
				}
				}
		}
}

void VOICE_Start
(
	
	uint8 voice_coun,
	uint16 settingtime
	
	
)
{
	
	voice_count	= voice_coun;
	voice_settime = settingtime;
	voice_flag = 1;
        
        TIM4_Cmd(ENABLE);
	
}


void VOICE_INIT(void)
{	
	disableInterrupts();
	VOICE_init();
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	TIM4_TimeBaseInit(TIM4_Prescaler_16, 5);
   	TIM4_ClearFlag(TIM4_FLAG_Update);
	TIM4_ITConfig(TIM4_IT_Update, ENABLE);
	
	enableInterrupts();
	//TIM4_Cmd(ENABLE);
	
}


void voice_merage(uint8 uivalue,uint16 input)
{
	switch(uivalue)
		{
			case strip_in :
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(0,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);

				Voice_closed();
				//voice_merage1=15;
				break;
			case result_out:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(2,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				number_to_voice(input);
				VOICE_Start(3,10);
				//voice_merage1=15;

				break;
			case hight_bloodsugar:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(4,10);
				break;
			case low_bloddsuguar:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(5,10);
				break;
			case blood_ketone:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(6,10);
				break;
			case power_off:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(7,10);
				break;
			case error:
				VOICE_Start(21,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(8,10);
				break;
			default :
				Voice_closed();
				break;
				
		}

}

uint8 get_vla(void)
{
	uint8 uival;
	uival=voice_merage1;
	return uival;
}

void Voice_closed(void)
{
	TIM4_Cmd(DISABLE);
	voice_count	= 0;
	voice_settime = 0;
	voice_flag = 0;
}

void voice_over(void)
{
	while(DrvGPIO_Read(voice_busy)==0);
	while(DrvGPIO_Read(voice_busy)==1);
	
}

void number_to_voice(uint16 input)
{
	uint8 ge;
	uint8 shi;
	uint8 xiao;
	if(input<100)
		{
			ge=input/10;
			xiao=input%10;
				VOICE_Start((ge+8),10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				VOICE_Start(19,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
				if(xiao==0)
					{
					VOICE_Start(20,10);
					while(DrvGPIO_Read(voice_busy)==0);
					while(DrvGPIO_Read(voice_busy)==1);
					}
				else 
					{
					VOICE_Start((xiao+8),10);
					while(DrvGPIO_Read(voice_busy)==0);
					while(DrvGPIO_Read(voice_busy)==1);
					}
				Voice_closed();
		}
	else 
		{
			shi=input/100;
			ge=(input/10)%10;
			xiao=input%100;
			VOICE_Start(18,10);
			while(DrvGPIO_Read(voice_busy)==0);
			while(DrvGPIO_Read(voice_busy)==1);
			if(ge==0)
				{
					;
				}
			else
				{
				VOICE_Start((ge+8),10);
			while(DrvGPIO_Read(voice_busy)==0);
			while(DrvGPIO_Read(voice_busy)==1);
				}
				VOICE_Start(19,10);
				while(DrvGPIO_Read(voice_busy)==0);
				while(DrvGPIO_Read(voice_busy)==1);
			if(xiao==0)
					{
					VOICE_Start(20,10);
					while(DrvGPIO_Read(voice_busy)==0);
					while(DrvGPIO_Read(voice_busy)==1);
					}
				else 
					{
					VOICE_Start((xiao+8),10);
					while(DrvGPIO_Read(voice_busy)==0);
					while(DrvGPIO_Read(voice_busy)==1);
					}
		}
}
