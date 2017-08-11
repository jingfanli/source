/*
 * Module:	Hardware driver
 * Author:	Lvjianfeng
 * Date:	2011.8
 */

#ifndef _DRV_VOICE_H_
#define _DRV_VOICE_H_


#include "global.h"
#include "message.h"

#include "drv_adc.h"
#include "drv_beep.h"
#include "drv_dac.h"
#include "drv_eeprom.h"
#include "drv_gpio.h"
#include "drv_lcd.h"
#include "drv_rtc.h"
#include "drv_uart.h"
#include "drv_flash.h"



void VOICE_init(void);
void VOICE_Tick
(
	uint16 u16_TickTime
);

void VOICE_Start
(
	
	uint8 voice_coun,
	uint16 settingtime
	
	
);

void VOICE_INIT(void);

/*void VOICE_program
	(
		uint8 voicecount,
		uint16 settingtime 
		
	);*/


void voice_merage(uint8 uivalue,uint16 input);

uint8 get_vla(void);
void voice_over(void);
void Voice_closed(void);
void number_to_voice(uint16 input);





typedef enum
{
	VOICEMODE_ON=0,
	VOICPWM_ON,
	VOICEMODE_COUNT
}VOICE_MODE;


typedef enum
{

	VOICE_ON=0,
	VOICE_OFF,


}VOICE_STAUS;

typedef enum
{
	strip_in=0,
	result_out,
	hight_bloodsugar,
	low_bloddsuguar,
	blood_ketone,
	power_off,
	error,
	voice_amount
	
	
}VOICE_SWITC;




#endif
