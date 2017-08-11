/*
 * Module:	Text parser library
 * Author:	Lvjianfeng
 * Date:	2012.6
 */


#include "lib_string.h"

#include "lib_parser.h"


//Constant definition


//Type definition


//Private variable definition

static uint8 m_u8_TagDelimiter;
static uint8 m_u8_TokenGroupCount;
static uint8 m_u8_TokenPerGroup[LIB_PARSER_TOKEN_GROUP_COUNT_MAX];
static const lib_parser_token_profile *m_tp_TokenProfile
	[LIB_PARSER_TOKEN_GROUP_COUNT_MAX];


//Private function declaration

static void LibParser_Memset
(
	uint8 *u8p_Data,
	uint8 u8_Value,
	uint ui_Length
);
static void LibParser_Memcpy
(
	uint8 *u8p_Dst,
	const uint8 *u8p_Src,
	uint16 u16_Length
);
static uint LibParser_CompareToken
(
	const uint8 *u8p_SourceString,
	const uint8 *u8p_DestString,
	uint16 u16_DestStringLength
);


//Public function definition

uint LibParser_Initialize(void)
{
	m_u8_TagDelimiter = LIB_PARSER_TAG_DELIMITER_DEFAULT;
	m_u8_TokenGroupCount = LIB_PARSER_TOKEN_GROUP_COUNT_MAX;
	LibParser_Memset((uint8 *)m_u8_TokenPerGroup, 0, sizeof(m_u8_TokenPerGroup));
	LibParser_Memset((uint8 *)m_tp_TokenProfile, 0, sizeof(m_tp_TokenProfile));

	return FUNCTION_OK;
}


uint LibParser_SetConfig
(
	uint ui_Parameter,
	const void *vp_Value
)
{
	switch (ui_Parameter)
	{
		case LIB_PARSER_PARAM_TOKEN_GROUP_COUNT:
			
			if (*((uint8 *)vp_Value) > LIB_PARSER_TOKEN_GROUP_COUNT_MAX)
			{
				return FUNCTION_FAIL;
			}

			m_u8_TokenGroupCount = *((uint8 *)vp_Value);
			break;

		case LIB_PARSER_PARAM_TAG_DELIMITER:
			m_u8_TagDelimiter = *((uint8 *)vp_Value);
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint LibParser_GetConfig
(
	uint ui_Parameter,
	void *vp_Value
)
{
	switch (ui_Parameter)
	{
		case LIB_PARSER_PARAM_TOKEN_GROUP_COUNT:
			*((uint8 *)vp_Value) = m_u8_TokenGroupCount;
			break;

		case LIB_PARSER_PARAM_TAG_DELIMITER:
			*((uint8 *)vp_Value) = m_u8_TagDelimiter;
			break;

		default:
			return FUNCTION_FAIL;
	}

	return FUNCTION_OK;
}


uint LibParser_AddToken
(
	uint8 u8_TokenGroup,
	uint8 u8_TokenPerGroup,
	const lib_parser_token_profile *tp_TokenProfile
)
{
	if (u8_TokenGroup >= m_u8_TokenGroupCount)
	{
		return FUNCTION_FAIL;
	}

	m_tp_TokenProfile[u8_TokenGroup] = tp_TokenProfile;
	m_u8_TokenPerGroup[u8_TokenGroup] = u8_TokenPerGroup;

	return FUNCTION_OK;
}


uint LibParser_ClearToken
(
	uint8 u8_TokenGroup
)
{
	if (u8_TokenGroup >= m_u8_TokenGroupCount)
	{
		return FUNCTION_FAIL;
	}

	m_tp_TokenProfile[u8_TokenGroup] = (lib_parser_token_profile *)0;
	m_u8_TokenPerGroup[u8_TokenGroup] = 0;

	return FUNCTION_OK;
}


uint LibParser_TextToToken
(
	const uint8 *u8p_Text,
	uint8 *u8p_Token,
	uint16 u16_TextLength,
	uint16 *u16p_TokenLength
)
{
	uint8 i;
	uint8 u8_TokenGroup;
	uint16 u16_TokenStringLength;
	uint16 u16_ParseLength;
	const uint8 *u8p_TokenString;


	if (*u16p_TokenLength < ((uint16)m_u8_TokenGroupCount + 
		(sizeof(m_u8_TokenGroupCount) + sizeof(u16_TokenStringLength))))
	{
		return FUNCTION_FAIL;
	}

	//Remove heading delimiter
	while (((*u8p_Text) == m_u8_TagDelimiter) && (u16_TextLength > 0))
	{
		u8p_Text++;
		u16_TextLength--;
	}

	*u8p_Token = m_u8_TokenGroupCount;
	u8p_Token++;
	u8_TokenGroup = 0;
	u8p_TokenString = u8p_Text;
	u16_TokenStringLength = 0;
	u16_ParseLength = 0;

	while (u16_ParseLength < u16_TextLength)
	{
		//Check if token delimiter is found
		if ((*u8p_Text) == m_u8_TagDelimiter)
		{
			//Check if token string should be parsed
			if ((u16_TokenStringLength > 0) && 
				(u8_TokenGroup < m_u8_TokenGroupCount))
			{
				if (m_tp_TokenProfile[u8_TokenGroup] == 
					(const lib_parser_token_profile *)0)
				{
					return FUNCTION_FAIL;
				}

				//Search for matched token in token group
				for (i = 0; i < m_u8_TokenPerGroup[u8_TokenGroup]; i++)
				{
					if (LibParser_CompareToken(m_tp_TokenProfile[u8_TokenGroup][i].
						u8_TokenString, u8p_TokenString, u16_TokenStringLength) == 
						FUNCTION_OK)
					{
						*u8p_Token = m_tp_TokenProfile[u8_TokenGroup][i].u8_TokenID;
						u8p_Token++;
						u16_TokenStringLength = 0;
						break;
					}
				}

				//Return if no match token is found
				if (u16_TokenStringLength != 0)
				{
					return FUNCTION_FAIL;
				}
			}
		}
		else
		{
			//Check if it is the first character of token string
			if (u16_TokenStringLength == 0) 
			{
				//Check if it it the first character of parsing
				if (u16_ParseLength != 0)
				{
					u8p_TokenString = u8p_Text;
					u8_TokenGroup++;
				}
			}

			if (u8_TokenGroup == m_u8_TokenGroupCount)
			{
				break;
			}

			u16_TokenStringLength++;
		}

		u16_ParseLength++;
		u8p_Text++;
	}

	//Check if there is any extra data left in the text
	if (u8_TokenGroup < m_u8_TokenGroupCount)
	{
		if (m_tp_TokenProfile[u8_TokenGroup] == 
			(const lib_parser_token_profile *)0)
		{
			return FUNCTION_FAIL;
		}

		//Search for matched token in token group
		for (i = 0; i < m_u8_TokenPerGroup[u8_TokenGroup]; i++)
		{
			if (LibParser_CompareToken(m_tp_TokenProfile[u8_TokenGroup][i].
				u8_TokenString, u8p_TokenString, u16_TokenStringLength) == 
				FUNCTION_OK)
			{
				*u8p_Token = m_tp_TokenProfile[u8_TokenGroup][i].u8_TokenID;
				break;
			}
		}

		u8p_Token += m_u8_TokenGroupCount - u8_TokenGroup;

		if (i >= m_u8_TokenPerGroup[u8_TokenGroup])
		{
			//Return if no token is found
			if ((u8_TokenGroup == 0) && (u16_TokenStringLength != 0))
			{
				return FUNCTION_FAIL;
			}

			*u16p_TokenLength -= (uint16)m_u8_TokenGroupCount + 
				(sizeof(m_u8_TokenGroupCount) + sizeof(u16_TokenStringLength));

			//Check if extra data length is out of range
			if (u16_TokenStringLength > *u16p_TokenLength) 
			{
				return FUNCTION_FAIL;
			}

			*((uint16 *)u8p_Token) = u16_TokenStringLength;
			u8p_Token += sizeof(u16_TokenStringLength);
			
			//Copy extra data to token buffer
			LibParser_Memcpy(u8p_Token, u8p_TokenString, u16_TokenStringLength);
		}
		else
		{
			*((uint16 *)u8p_Token) = 0;
			u16_TokenStringLength = 0;
		}
	}
	else
	{
		*u16p_TokenLength -= (uint16)m_u8_TokenGroupCount + 
			(sizeof(m_u8_TokenGroupCount) + sizeof(u16_TokenStringLength));

		//Check if extra data length is out of range
		if ((u16_TextLength - u16_ParseLength) > *u16p_TokenLength) 
		{
			return FUNCTION_FAIL;
		}

		u16_TokenStringLength = u16_TextLength - u16_ParseLength;
		*((uint16 *)u8p_Token) = u16_TokenStringLength;
		u8p_Token += sizeof(u16_TokenStringLength);
		
		//Copy extra data to token buffer
		LibParser_Memcpy(u8p_Token, u8p_Text, u16_TokenStringLength);
	}

	*u16p_TokenLength = u16_TokenStringLength + (uint16)m_u8_TokenGroupCount + 
		(sizeof(m_u8_TokenGroupCount) + sizeof(u16_TokenStringLength));

	return FUNCTION_OK;
}


uint LibParser_TokenToText
(
	const uint8 *u8p_Token,
	uint8 *u8p_Text,
	uint16 u16_TokenLength,
	uint16 *u16p_TextLength
)
{

	return FUNCTION_OK;
}


//Private function definition

static void LibParser_Memset
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


static void LibParser_Memcpy
(
	uint8 *u8p_Dst,
	const uint8 *u8p_Src,
	uint16 u16_Length
)
{
	while (u16_Length > 0)
	{
		*u8p_Dst++ = *u8p_Src++;
		u16_Length--;
	}
}


static uint LibParser_CompareToken
(
	const uint8 *u8p_SourceString,
	const uint8 *u8p_DestString,
	uint16 u16_DestStringLength
)
{
	uint16 u16_CompareLength;


	u16_CompareLength = 0;

	while ((u16_CompareLength < u16_DestStringLength) && 
		(u16_CompareLength < LIB_PARSER_TOKEN_STRING_SIZE_MAX))
	{
		if ((*u8p_SourceString) != (*u8p_DestString))
		{
			break;
		}

		u8p_DestString++;
		u8p_SourceString++;
		u16_CompareLength++;
	}

	if (u16_CompareLength >= u16_DestStringLength)
	{
		//When destination string is shorter than source string, check if there is 
		//any charater in sourece string left that is not being compared
		if ((u16_CompareLength == LIB_PARSER_TOKEN_STRING_SIZE_MAX) || 
			(!((u16_CompareLength < LIB_PARSER_TOKEN_STRING_SIZE_MAX) && 
			((*u8p_SourceString) != m_u8_TagDelimiter))))
		{
			//Match token is found
			return FUNCTION_OK;
		}
	}

	return FUNCTION_FAIL;
}
