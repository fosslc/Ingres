/*
** Copyright (c) 2004 Ingres Corporation
*/





/*
** Name: aitsym.h
**
** Description:
**	API symbol tables.
**
** History:
**	24-Mar-95 (gordy)
**	    Extracted initializations to aitsym.c.
**	10-May-95 (feige)
**	    Added macro definition of MAX_CP_SYMSTR and MAX_CP_SYMINT.
**	    Updated macro definition of IS_PRMID_CHAR and IS_PRMID_LONG.
**	    Added GLOBALREF to CP_SYMSTRTBL and CP_SYMINT_TBL.
**	14-Jan-96 (emmag)
**	    Integration of Jasmine changes.
**	 9-Sep-96 (gordy)
**	    Added IIAPI_CP_DBMS_PASSWORD and IIAPI_CP_CENTURY_BOUNDARY.
**	13-Mar-97 (gordy)
**	    API Version 2: connection types.
**	19-Aug-98 (rajus01)
**	    Added macro definitions and table reference definitions for 
**	    IIapi_setEnvParam() function testing.
**	03-Feb-98 (rajus01)
**	    Added support for handling callback functions in 
**	    IIAPI_SETENVPRMPARM. Added AIT_SYMFUNC structure.
**	15-Mar-04 (gordy)
**	    Added support for 8 byte integers: IIAPI_CP_INT8_WIDTH,
**	    IIAPI_EP_INT8_WIDTH.
**      07-Sep-5 (crogr01)
**	    Completed support for MULTINATIONAL4 and ISO4 date formats.
*/





# ifndef __AITTERM_H__
# define __AITTERM_H__




# define	MAX_in_version				3
# define	MAX_sc_paramID				38
# define	MAX_CP_FLOAT4_STYLE			4
# define	MAX_CP_NUMERIC_TREATMENT		2
# define	MAX_CP_MONEY_LORT			2
# define	MAX_CP_MATH_EXCP			3
# define	MAX_CP_DATE_FORMAT			10	
# define	MAX_CP_SECONDARY_INX			8
# define	MAX_CP_SERVER_TYPE			2
# define	MAX_co_apiLevel				2
# define	MAX_co_type				2
# define	MAX_qy_queryType			51
# define	MAX_DT_ID				17
# define	MAX_ds_columnType			5
# define	MAX_gq_mask				7
# define	MAX_gp_status				9
# define	MAX_ti_type				2
# define	MAX_ge_type				3
# define	MAX_gq_flags				13
# define	MAX_se_paramID				28
# define	MAX_EP_FLOAT4_STYLE		MAX_CP_FLOAT4_STYLE
# define	MAX_EP_NUMERIC_TREATMENT	MAX_CP_NUMERIC_TREATMENT
# define	MAX_EP_MONEY_LORT		MAX_CP_MONEY_LORT
# define	MAX_EP_MATH_EXCP		MAX_CP_MATH_EXCP
# define	MAX_EP_DATE_FORMAT		MAX_CP_DATE_FORMAT

# define	MAX_CP_SYMSTR			MAX_CP_FLOAT4_STYLE +      \
						MAX_CP_NUMERIC_TREATMENT + \
						MAX_CP_MATH_EXCP +         \
						MAX_CP_SECONDARY_INX

# define	MAX_EP_SYMSTR			MAX_EP_FLOAT4_STYLE +      \
						MAX_EP_NUMERIC_TREATMENT + \
						MAX_EP_MATH_EXCP

# define	MAX_CP_SYMINT			MAX_CP_MONEY_LORT +	   \
						MAX_CP_DATE_FORMAT +       \
						MAX_CP_SERVER_TYPE

# define	MAX_EP_SYMINT			MAX_EP_MONEY_LORT +	   \
						MAX_EP_DATE_FORMAT 

# define	MAX_EP_FUNC			3

# define  IS_PRMID_LONG	(setc->sc_paramID == IIAPI_CP_CHAR_WIDTH)        || \
                        (setc->sc_paramID == IIAPI_CP_TXT_WIDTH)         || \
                        (setc->sc_paramID == IIAPI_CP_INT1_WIDTH)        || \
                        (setc->sc_paramID == IIAPI_CP_INT2_WIDTH)        || \
                        (setc->sc_paramID == IIAPI_CP_INT4_WIDTH)        || \
                        (setc->sc_paramID == IIAPI_CP_INT8_WIDTH)        || \
                        (setc->sc_paramID == IIAPI_CP_FLOAT4_WIDTH)      || \
                        (setc->sc_paramID == IIAPI_CP_FLOAT8_WIDTH)      || \
                        (setc->sc_paramID == IIAPI_CP_FLOAT4_PRECISION)  || \
                        (setc->sc_paramID == IIAPI_CP_FLOAT8_PRECISION)  || \
                        (setc->sc_paramID == IIAPI_CP_MONEY_PRECISION)   || \
                        (setc->sc_paramID == IIAPI_CP_NATIVE_LANG_CODE)  || \
                        (setc->sc_paramID == IIAPI_CP_APPLICATION)	 || \
			(setc->sc_paramID == IIAPI_CP_CENTURY_BOUNDARY)

# define IS_EPRMID_LONG	(sete->se_paramID == IIAPI_EP_CHAR_WIDTH) || \
			(sete->se_paramID == IIAPI_EP_TXT_WIDTH)         || \
			(sete->se_paramID == IIAPI_EP_INT1_WIDTH)        || \
			(sete->se_paramID == IIAPI_EP_INT2_WIDTH)        || \
			(sete->se_paramID == IIAPI_EP_INT4_WIDTH)        || \
			(sete->se_paramID == IIAPI_EP_INT8_WIDTH)        || \
			(sete->se_paramID == IIAPI_EP_FLOAT4_WIDTH)      || \
			(sete->se_paramID == IIAPI_EP_FLOAT8_WIDTH)      || \
			(sete->se_paramID == IIAPI_EP_FLOAT4_PRECISION)  || \
			(sete->se_paramID == IIAPI_EP_FLOAT8_PRECISION)  || \
			(sete->se_paramID == IIAPI_EP_MONEY_PRECISION)   || \
			(sete->se_paramID == IIAPI_EP_NATIVE_LANG_CODE)  || \
			(sete->se_paramID == IIAPI_EP_CENTURY_BOUNDARY)  || \
			(sete->se_paramID == IIAPI_EP_MAX_SEGMENT_LEN)

# define IS_PRMID_CHAR  (setc->sc_paramID == IIAPI_CP_FLOAT8_STYLE)      || \
                        (setc->sc_paramID == IIAPI_CP_MONEY_SIGN)        || \
                        (setc->sc_paramID == IIAPI_CP_DECIMAL_CHAR)      || \
                        (setc->sc_paramID == IIAPI_CP_STRING_TRUNC)      || \
                        (setc->sc_paramID == IIAPI_CP_TIMEZONE)          || \
                        (setc->sc_paramID == IIAPI_CP_RESULT_TBL)        || \
                        (setc->sc_paramID == IIAPI_CP_NATIVE_LANG)       || \
                        (setc->sc_paramID == IIAPI_CP_APP_ID)            || \
                        (setc->sc_paramID == IIAPI_CP_GROUP_ID)          || \
                        (setc->sc_paramID == IIAPI_CP_EFFECTIVE_USER)    || \
                      	(setc->sc_paramID == IIAPI_CP_MISC_PARM)         || \
                        (setc->sc_paramID == IIAPI_CP_GATEWAY_PARM)	 || \
			(setc->sc_paramID == IIAPI_CP_DBMS_PASSWORD)

# define IS_EPRMID_CHAR (sete->se_paramID == IIAPI_EP_FLOAT8_STYLE)      || \
                        (sete->se_paramID == IIAPI_EP_MONEY_SIGN)        || \
                        (sete->se_paramID == IIAPI_EP_DECIMAL_CHAR)      || \
                        (sete->se_paramID == IIAPI_EP_STRING_TRUNC)      || \
                        (sete->se_paramID == IIAPI_EP_NATIVE_LANG)       || \
                        (sete->se_paramID == IIAPI_EP_TIMEZONE)

# define IS_PRMID_BOOL  (setc->sc_paramID == IIAPI_CP_EXCLUSIVE_SYS_UPDATE) || \
                        (setc->sc_paramID == IIAPI_CP_SHARED_SYS_UPDATE)    || \
                        (setc->sc_paramID == IIAPI_CP_EXCLUSIVE_LOCK)       || \
                        (setc->sc_paramID == IIAPI_CP_WAIT_LOCK)

# define IS_EPRMID_FUNC (sete->se_paramID == IIAPI_EP_TRACE_FUNC) 	|| \
			(sete->se_paramID == IIAPI_EP_PROMPT_FUNC)	|| \
			(sete->se_paramID == IIAPI_EP_EVENT_FUNC)	





/*
** typedef
*/

typedef struct	_strlongtbl
{

    char	*symbol;
    II_LONG	longVal;

} AIT_SYMINT;

typedef struct	_strstrtbl
{

    char	*symbol;
    char	*strVal;

} AIT_SYMSTR;


typedef struct	_strfunctbl
{

    char	*symbol;
    II_VOID	(*func)();

} AIT_SYMFUNC;



/*
** External variables
*/

GLOBALREF AIT_SYMINT	in_version_TBL[MAX_in_version];
GLOBALREF AIT_SYMINT	sc_paramID_TBL[MAX_sc_paramID];
GLOBALREF AIT_SYMINT	co_apiLevel_TBL[MAX_co_apiLevel];
GLOBALREF AIT_SYMINT	co_type_TBL[MAX_co_type];
GLOBALREF AIT_SYMINT	CP_MONEY_LORT_TBL[MAX_CP_MONEY_LORT];
GLOBALREF AIT_SYMINT	CP_DATE_FORMAT_TBL[MAX_CP_DATE_FORMAT];
GLOBALREF AIT_SYMINT	CP_SERVER_TYPE_TBL[MAX_CP_SERVER_TYPE];
GLOBALREF AIT_SYMINT	qy_queryType_TBL[MAX_qy_queryType];
GLOBALREF AIT_SYMINT	DT_ID_TBL[MAX_DT_ID];
GLOBALREF AIT_SYMINT	ds_columnType_TBL[MAX_ds_columnType];
GLOBALREF AIT_SYMINT	gq_mask_TBL[MAX_gq_mask];
GLOBALREF AIT_SYMINT	gp_status_TBL[MAX_gp_status];
GLOBALREF AIT_SYMINT	ti_type_TBL[MAX_ti_type];
GLOBALREF AIT_SYMINT	ge_type_TBL[MAX_ge_type];
GLOBALREF AIT_SYMINT	gq_flags_TBL[MAX_gq_flags];
GLOBALREF AIT_SYMINT    CP_SYMINT_TBL[MAX_CP_SYMINT];
GLOBALREF AIT_SYMSTR	CP_FLOAT4_STYLE_TBL[MAX_CP_FLOAT4_STYLE];
GLOBALREF AIT_SYMSTR	CP_NUMERIC_TREATMENT_TBL[MAX_CP_NUMERIC_TREATMENT];
GLOBALREF AIT_SYMSTR	CP_MATH_EXCP_TBL[MAX_CP_MATH_EXCP];
GLOBALREF AIT_SYMSTR	CP_SECONDARY_INX_TBL[MAX_CP_SECONDARY_INX];
GLOBALREF AIT_SYMSTR	CP_SYMSTR_TBL[MAX_CP_SYMSTR];


GLOBALREF AIT_SYMINT	se_paramID_TBL[MAX_se_paramID];
GLOBALREF AIT_SYMSTR	EP_SYMSTR_TBL[MAX_EP_SYMSTR];
GLOBALREF AIT_SYMSTR	EP_FLOAT4_STYLE_TBL[MAX_CP_FLOAT4_STYLE];
GLOBALREF AIT_SYMSTR	EP_MATH_EXCP_TBL[MAX_EP_MATH_EXCP];
GLOBALREF AIT_SYMSTR	EP_NUMERIC_TREATMENT_TBL[MAX_EP_NUMERIC_TREATMENT];
GLOBALREF AIT_SYMINT	EP_SYMINT_TBL[MAX_EP_SYMINT];
GLOBALREF AIT_SYMINT	EP_MONEY_LORT_TBL[MAX_EP_MONEY_LORT];
GLOBALREF AIT_SYMINT	EP_DATE_FORMAT_TBL[MAX_EP_DATE_FORMAT];

GLOBALREF AIT_SYMFUNC	EP_FUNC_TBL[MAX_EP_FUNC];

# endif			/* __AITTERM_H__ */
