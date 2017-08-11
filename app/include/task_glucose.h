/*
 * Module:	Blood glucose meter task
 * Author:	Lvjianfeng
 * Date:	2012.8
 */

#ifndef _TASK_GLUCOSE_H_
#define _TASK_GLUCOSE_H_


#include "global.h"
#include "history.h"
#include "devos.h"


//Constant define

#define TASK_GLUCOSE_TEST_ENABLE			0

#define TASK_GLUCOSE_HISTORY_ITEM_COUNT		500
#define TASK_GLUCOSE_TEST_DATA_COUNT		100


//Type definition

typedef enum
{
	TASK_GLUCOSE_PARAM_HYPO = 0,
	TASK_GLUCOSE_PARAM_HYPER,
	TASK_GLUCOSE_PARAM_KETONE,
	TASK_GLUCOSE_PARAM_MEAL,
	TASK_GLUCOSE_PARAM_HISTORY_COUNT,
	TASK_GLUCOSE_PARAM_NBB_LIMIT,
	TASK_GLUCOSE_PARAM_EARLY_FILL_LIMIT,
	TASK_GLUCOSE_PARAM_FILL_LIMIT,
	TASK_GLUCOSE_PARAM_BG_LIMIT,
	TASK_GLUCOSE_PARAM_TEST,
	TASK_GLUCOSE_PARAM_RE,
	TASK_GLUCOSE_COUNT_PARAM
} task_glucose_parameter;

typedef enum
{
	TASK_GLUCOSE_FLAG_HYPO = 0,
	TASK_GLUCOSE_FLAG_HYPER,
	TASK_GLUCOSE_FLAG_KETONE,
	TASK_GLUCOSE_FLAG_BEFORE_MEAL,
	TASK_GLUCOSE_FLAG_AFTER_MEAL,
	TASK_GLUCOSE_FLAG_POUND,
	TASK_GLUCOSE_FLAG_CONTROL,
	TASK_GLUCOSE_FLAG_MMOL,
	TASK_GLUCOSE_FLAG_NO_CODE,
	TASK_GLUCOSE_COUNT_FLAG
} task_glucose_flag;

typedef struct
{
	history_date_time t_DateTime;
	uint16 u16_Flag;
	uint16 u16_Glucose;
	uint16 u16_Temperature;
	uint16 u16_Index;
	uint16 u16_Current;
	uint16 u16_Impedance;
} task_glucose_history_item;


//Function declaration

uint TaskGlucose_Initialize
(
	devos_task_handle t_TaskHandle
);
void TaskGlucose_Process
(
	devos_task_handle t_TaskHandle
);
uint TaskGlucose_SetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
uint TaskGlucose_GetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
void tran_re(uint8* t);

uint TaskGlucose_ResetHistory(void);
uint TaskGlucose_ReadHistory
(
	uint16 u16_Index,
	uint ui_Absolute,
	task_glucose_history_item *tp_Item
);

#if TASK_GLUCOSE_TEST_ENABLE == 1
void TaskGlucose_Test(void);
#endif

#endif
