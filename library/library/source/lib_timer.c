/*
 * Module:	Software timer library
 * Author:	Lvjianfeng
 * Date:	2011.8
 */


#include "lib_timer.h"


//Constant definition


//Type definition

typedef enum
{
	LIB_TIMER_STATE_VALID,
	LIB_TIMER_STATE_ACTIVE,
	LIB_TIMER_STATE_OVERFLOW,
} lib_timer_state;

typedef struct _lib_timer_object
{
	uint ui_State;
	uint16 u16_Tick;
	uint16 u16_Reload;
#if LIB_TIMER_LIST_ENABLE != 0
	struct _lib_timer_object *tp_NextTimer;
#endif
} lib_timer_object;


//Variable definition

static uint m_ui_TimerCount;
static lib_timer_object m_t_Timer[LIB_TIMER_COUNT_MAX];
#if LIB_TIMER_LIST_ENABLE != 0
static lib_timer_object *m_tp_TimerList;
#endif


//Private function declaration

static void LibTimer_Memset
(
	uint8 *u8p_Data,
	uint8 u8_Value,
	uint ui_Length
);
#if LIB_TIMER_LIST_ENABLE != 0
static void LibTimer_ReloadTimer
(
	lib_timer_object *tp_Timer
);
#endif


//Public function definition

uint LibTimer_Initialize(void)
{
	m_ui_TimerCount = 0;
	LibTimer_Memset((uint8 *)m_t_Timer, 0, sizeof(m_t_Timer));
#if LIB_TIMER_LIST_ENABLE != 0
	m_tp_TimerList = (lib_timer_object *)0;
#endif

	return FUNCTION_OK;
}


uint LibTimer_SetConfig
(
	uint ui_TimerChannel,
	uint ui_Parameter,
	const void *vp_Value
)
{
	//Check if timer channel is out of range
	if (ui_TimerChannel >= m_ui_TimerCount)
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Parameter)
	{
		case LIB_TIMER_PARAM_RELOAD:
			m_t_Timer[ui_TimerChannel].u16_Reload = *(uint16 *)vp_Value;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint LibTimer_GetConfig
(
	uint ui_TimerChannel,
	uint ui_Parameter,
	void *vp_Value
)
{
	//Check if timer channel is out of range
	if (ui_TimerChannel >= m_ui_TimerCount)
	{
		return FUNCTION_FAIL;
	}

	switch (ui_Parameter)
	{
		case LIB_TIMER_PARAM_TICK:
			*(uint16 *)vp_Value = m_t_Timer[ui_TimerChannel].u16_Reload;
			break;

		case LIB_TIMER_PARAM_OVERFLOW:

			if (REG_GET_BIT(m_t_Timer[ui_TimerChannel].ui_State, 
				LIB_TIMER_STATE_OVERFLOW) == 0)
			{
				*(uint *)vp_Value = 0;
			}
			else
			{
				*(uint *)vp_Value = 1;
				REG_CLEAR_BIT(m_t_Timer[ui_TimerChannel].ui_State, 
					LIB_TIMER_STATE_OVERFLOW);
			}

			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint LibTimer_Create
(
	uint *uip_TimerChannel
)
{
	uint i;
	lib_timer_object *tp_Timer;


	tp_Timer = &m_t_Timer[0];

	for (i = 0; i < LIB_TIMER_COUNT_MAX; i++)
	{
		if (REG_GET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_VALID) == 0)
		{
			if (m_ui_TimerCount < i + 1)
			{
				m_ui_TimerCount = i + 1;
			}

			*uip_TimerChannel = i;
			REG_SET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_VALID);
			return FUNCTION_OK;
		}

		tp_Timer++;
	}

	return FUNCTION_FAIL;
}


uint LibTimer_Delete
(
	uint ui_TimerChannel
)
{
	lib_timer_object *tp_Timer;


	//Check if timer channel is out of range
	if (ui_TimerChannel >= m_ui_TimerCount)
	{
		return FUNCTION_FAIL;
	}

	tp_Timer = &m_t_Timer[ui_TimerChannel];

	//Check if timer is active or not
	if (REG_GET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_ACTIVE) != 0)
	{
		if (LibTimer_Stop(ui_TimerChannel) != FUNCTION_OK)
		{
			return FUNCTION_FAIL;
		}
	}

	LibTimer_Memset((uint8 *)tp_Timer, 0, sizeof(lib_timer_object));

	return FUNCTION_OK;
}


uint LibTimer_Start
(
	uint ui_TimerChannel
)
{
	lib_timer_object *tp_Timer;


	//Check if timer channel is out of range
	if (ui_TimerChannel >= m_ui_TimerCount)
	{
		return FUNCTION_FAIL;
	}

	tp_Timer = &m_t_Timer[ui_TimerChannel];
	
	//Check if timer is valid or not
	if ((REG_GET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_VALID) == 0) ||
		(tp_Timer->u16_Reload == 0))
	{
		return FUNCTION_FAIL;
	}

#if LIB_TIMER_LIST_ENABLE != 0
	LibTimer_ReloadTimer(tp_Timer);
#endif

	REG_SET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_ACTIVE);

	return FUNCTION_OK;
}


uint LibTimer_Stop
(
	uint ui_TimerChannel
)
{
	lib_timer_object *tp_Timer;
#if LIB_TIMER_LIST_ENABLE != 0
	lib_timer_object *tp_PrevTimer;
#endif


	//Check if timer channel is out of range
	if (ui_TimerChannel >= m_ui_TimerCount)
	{
		return FUNCTION_FAIL;
	}

	tp_Timer = &m_t_Timer[ui_TimerChannel];

	//Check if timer is active or not
	if (REG_GET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_ACTIVE) != 0)
	{
#if LIB_TIMER_LIST_ENABLE != 0
		tp_PrevTimer = m_tp_TimerList;

		if (tp_PrevTimer != tp_Timer)
		{
			//Find the previous timer before the stopped timer
			while (tp_PrevTimer != 0)
			{
				if (tp_PrevTimer->tp_NextTimer == tp_Timer)
				{
					break;
				}

				tp_PrevTimer = tp_PrevTimer->tp_NextTimer;
			}

			tp_PrevTimer->tp_NextTimer = tp_Timer->tp_NextTimer;
		}
		else
		{
			m_tp_TimerList = tp_Timer->tp_NextTimer;
		}

		tp_PrevTimer = tp_Timer->tp_NextTimer;

		//Update tick value of timer that after the stopped timer
		if (tp_PrevTimer != 0)
		{
			tp_PrevTimer->u16_Tick += tp_Timer->u16_Tick;
		}
#endif
		REG_CLEAR_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_ACTIVE);
	}

	return FUNCTION_OK;
}


void LibTimer_Tick
(
	uint16 u16_TickTime
)
{
	lib_timer_object *tp_Timer;


#if LIB_TIMER_LIST_ENABLE != 0
	if (m_tp_TimerList != (lib_timer_object *)0)
	{
		if (m_tp_TimerList->u16_Tick >= u16_TickTime)
		{
			m_tp_TimerList->u16_Tick -= u16_TickTime;
		}

		while (m_tp_TimerList->u16_Tick < u16_TickTime)
		{
			REG_SET_BIT(m_tp_TimerList->ui_State, LIB_TIMER_STATE_OVERFLOW);
			tp_Timer = m_tp_TimerList;
			m_tp_TimerList = tp_Timer->tp_NextTimer;
			LibTimer_ReloadTimer(tp_Timer);
		}
	}
#else
	uint i;


	tp_Timer = &m_t_Timer[0];

	for (i = 0; i < m_ui_TimerCount; i++)
	{
		if (REG_GET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_ACTIVE) != 0)
		{
			tp_Timer->u16_Tick += u16_TickTime;

			if (tp_Timer->u16_Tick >= tp_Timer->u16_Reload)
			{
				tp_Timer->u16_Tick = 0;
				REG_SET_BIT(tp_Timer->ui_State, LIB_TIMER_STATE_OVERFLOW);
			}
		}

		tp_Timer++;
	}
#endif
}


#if LIB_TIMER_TEST_ENABLE != 0

void LibTimer_Test(void)
{
}

#endif


//Private function definition

static void LibTimer_Memset
(
	uint8 *u8p_Data,
	uint8 u8_Value,
	uint ui_Length
)
{
	while (ui_Length > 0)
	{
		*u8p_Data++ = u8_Value;
		ui_Length--;
	}
}


#if LIB_TIMER_LIST_ENABLE != 0
static void LibTimer_ReloadTimer
(
	lib_timer_object *tp_Timer
)
{
	lib_timer_object *tp_PrevTimer;
	lib_timer_object *tp_NextTimer;
	lib_timer_object *tp_ExpireTimer;


	tp_ExpireTimer = tp_Timer;

	//Find all the expired timers
	while (tp_ExpireTimer->tp_NextTimer != (lib_timer_object *)0)
	{
		if ((tp_ExpireTimer->tp_NextTimer->u16_Tick == 0) &&
			(tp_ExpireTimer->u16_Reload == 
			tp_ExpireTimer->tp_NextTimer->u16_Reload))
		{
			REG_SET_BIT(tp_ExpireTimer->tp_NextTimer->ui_State, 
				LIB_TIMER_STATE_OVERFLOW);
			tp_ExpireTimer = tp_ExpireTimer->tp_NextTimer;
			m_tp_TimerList = tp_ExpireTimer->tp_NextTimer;
		}
		else
		{
			break;
		}
	}

	tp_Timer->u16_Tick = tp_Timer->u16_Reload;
	tp_ExpireTimer->tp_NextTimer = (lib_timer_object *)0;
	tp_NextTimer = m_tp_TimerList;

	//Check if active timer list is empty or not
	if (tp_NextTimer == (lib_timer_object *)0)
	{
		m_tp_TimerList = tp_Timer;
		return;
	}

	//Check if timer should be added to the head of active timer list
	if (tp_Timer->u16_Tick < tp_NextTimer->u16_Tick)
	{
		tp_ExpireTimer->tp_NextTimer = tp_NextTimer;
		tp_NextTimer->u16_Tick -= tp_Timer->u16_Tick;
		m_tp_TimerList = tp_Timer;
		return;
	}

	tp_PrevTimer = tp_NextTimer;

	//Insert timer into the active timer list
	while (tp_NextTimer != (lib_timer_object *)0)
	{
		if (tp_Timer->u16_Tick >= tp_NextTimer->u16_Tick)
		{
			tp_Timer->u16_Tick -= tp_NextTimer->u16_Tick;
			tp_PrevTimer = tp_NextTimer;
			tp_NextTimer = tp_NextTimer->tp_NextTimer;
		}
		else
		{
			break;
		}
	}

	tp_PrevTimer->tp_NextTimer = tp_Timer;
	tp_ExpireTimer->tp_NextTimer = tp_NextTimer;

	//Check if timer is the last one
	if (tp_NextTimer != (lib_timer_object *)0)
	{
		tp_NextTimer->u16_Tick -= tp_Timer->u16_Tick;
	}
}
#endif
