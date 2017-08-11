/*
 * Module:	Text parser library
 * Author:	Lvjianfeng
 * Date:	2012.6
 */

#ifndef _LIB_PARSER_H_
#define _LIB_PARSER_H_


#include "global.h"


//Constant definition

#ifndef LIB_PARSER_TOKEN_STRING_SIZE_MAX
#define LIB_PARSER_TOKEN_STRING_SIZE_MAX	8
#endif

#ifndef LIB_PARSER_TOKEN_GROUP_COUNT_MAX
#define LIB_PARSER_TOKEN_GROUP_COUNT_MAX	4
#endif

#ifndef LIB_PARSER_TAG_DELIMITER_DEFAULT
#define LIB_PARSER_TAG_DELIMITER_DEFAULT	' '
#endif


//Type definition

typedef enum
{
	LIB_PARSER_PARAM_TOKEN_GROUP_COUNT,
	LIB_PARSER_PARAM_TAG_DELIMITER,
	LIB_PARSER_COUNT_PARAM
} lib_parser_param;

typedef struct
{
	uint8 u8_TokenID;
	uint8 u8_TokenString[LIB_PARSER_TOKEN_STRING_SIZE_MAX];
} lib_parser_token_profile; 


//Function declaration

uint LibParser_Initialize(void);
uint LibParser_SetConfig
(
	uint ui_Parameter,
	const void *vp_Value
);
uint LibParser_GetConfig
(
	uint ui_Parameter,
	void *vp_Value
);
uint LibParser_AddToken
(
	uint8 u8_TokenGroup,
	uint8 u8_TokenPerGroup,
	const lib_parser_token_profile *tp_TokenProfile
);
uint LibParser_ClearToken
(
	uint8 u8_TokenGroup
);
uint LibParser_TextToToken
(
	const uint8 *u8p_Text,
	uint8 *u8p_Token,
	uint16 u16_TextLength,
	uint16 *u16p_TokenLength
);
uint LibParser_TokenToText
(
	const uint8 *u8p_Token,
	uint8 *u8p_Text,
	uint16 u16_TokenLength,
	uint16 *u16p_TextLength
);


#endif
