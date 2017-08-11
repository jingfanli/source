/*
 * Module:	Midware
 * Author:	Lvjianfeng
 * Date:	2011.10
 */


#include "drv.h"

#include "mid.h"


//Constant definition


//Type definition

typedef uint (*mid_set_config)
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);

typedef uint (*mid_get_config)
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);


//Private function declaration

static uint Mid_ButtonLeftSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint Mid_ButtonCenterSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint Mid_ButtonRightSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
);
static uint Mid_ButtonLeftGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
static uint Mid_ButtonCenterGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);
static uint Mid_ButtonRightGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
);


//Private variable definition

static const mid_set_config m_fp_SetConfig[MID_COUNT_PORT] =
{
	0,
	Glucose_SetConfig,
	Mid_ButtonLeftSetConfig,
	Mid_ButtonCenterSetConfig,
	Mid_ButtonRightSetConfig,
	Bluetooth_SetConfig
};
static const mid_get_config m_fp_GetConfig[MID_COUNT_PORT] =
{
	0,
	Glucose_GetConfig,
	Mid_ButtonLeftGetConfig,
	Mid_ButtonCenterGetConfig,
	Mid_ButtonRightGetConfig,
	Bluetooth_GetConfig
};


//Public function definition

uint Mid_Initialize(void)
{
	Button_Initialize();
	Glucose_Initialize();

	return FUNCTION_OK;
}


uint Mid_HandleEvent
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	uint8 u8_Event
)
{
	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_MIDWARE)
	{
		return FUNCTION_FAIL;
	}

	switch (u8_Event)
	{
		case MESSAGE_EVENT_SEND_DONE:
			break;

		case MESSAGE_EVENT_ACKNOWLEDGE:
			break;

		case MESSAGE_EVENT_TIMEOUT:
			break;

		case MESSAGE_EVENT_RECEIVE_DONE:
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint Mid_HandleCommand
(
	uint8 u8_SourcePort,
	uint8 u8_TargetPort,
	message_command *tp_Command
)
{
	uint ui_Length;
	uint ui_Acknowledge;
	uint8 u8_Port;


	if (MESSAGE_GET_MAJOR_PORT(u8_TargetPort) != MESSAGE_PORT_MIDWARE)
	{
		return FUNCTION_FAIL;
	}

	u8_Port = MESSAGE_GET_MINOR_PORT(u8_TargetPort);

	if (u8_Port >= MID_COUNT_PORT)
	{
		return FUNCTION_FAIL;
	}

	ui_Acknowledge = FUNCTION_FAIL;

	switch (tp_Command->u8_Operation)
	{
		case MESSAGE_OPERATION_SET:

			if (m_fp_SetConfig[u8_Port] != (mid_set_config)0)
			{
				ui_Acknowledge = 
					m_fp_SetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, (uint)tp_Command->u8_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8p_Data[0] = (uint8)ui_Acknowledge;
					tp_Command->u8_Operation = MESSAGE_OPERATION_ACKNOWLEDGE;
					tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
				}
			}

			break;

		case MESSAGE_OPERATION_GET:

			if (m_fp_GetConfig[u8_Port] != (mid_get_config)0)
			{
				ui_Length = (uint)tp_Command->u8_Length;
				
				ui_Acknowledge = 
					m_fp_GetConfig[u8_Port]((uint)tp_Command->u8_Parameter,
					tp_Command->u8p_Data, &ui_Length);

				if (ui_Acknowledge == FUNCTION_OK)
				{
					tp_Command->u8_Operation = MESSAGE_OPERATION_NOTIFY;
					tp_Command->u8_Length = (uint8)ui_Length;
				}
			}

			break;

		case MESSAGE_OPERATION_NOTIFY:
		case MESSAGE_OPERATION_ACKNOWLEDGE:
			ui_Acknowledge = FUNCTION_OK;
			tp_Command->u8p_Data[0] = (uint8)MESSAGE_EVENT_ACKNOWLEDGE;
			tp_Command->u8_Operation = MESSAGE_OPERATION_EVENT;
			tp_Command->u8_Length = sizeof(tp_Command->u8p_Data[0]);
			break;

		default:
			break;
	}
	
	return ui_Acknowledge;
}


//Private function definition

static uint Mid_ButtonLeftSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return Button_SetConfig(BUTTON_ID_LEFT, ui_Parameter, u8p_Value, ui_Length);
}


static uint Mid_ButtonCenterSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return Button_SetConfig(BUTTON_ID_CENTER, ui_Parameter, u8p_Value, ui_Length);
}


static uint Mid_ButtonRightSetConfig
(
	uint ui_Parameter,
	const uint8 *u8p_Value,
	uint ui_Length
)
{
	return Button_SetConfig(BUTTON_ID_RIGHT, ui_Parameter, u8p_Value, ui_Length);
}


static uint Mid_ButtonLeftGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return Button_GetConfig(BUTTON_ID_LEFT, ui_Parameter, u8p_Value, uip_Length);
}


static uint Mid_ButtonCenterGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return Button_GetConfig(BUTTON_ID_CENTER, ui_Parameter, u8p_Value, uip_Length);
}


static uint Mid_ButtonRightGetConfig
(
	uint ui_Parameter,
	uint8 *u8p_Value,
	uint *uip_Length
)
{
	return Button_GetConfig(BUTTON_ID_RIGHT, ui_Parameter, u8p_Value, uip_Length);
}
