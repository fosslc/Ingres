/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef SAPIW_H_INCLUDED
# define SAPIW_H_INCLUDED
# include <compat.h>
# include <st.h>
# include <er.h>
# include <iiapi.h>

/**
** Name:	sapiw.h - Synchronous OpenIngres API wrapper interface
**
** Description:
**	Include file for functions to interface with the OpenIngres API.
**
** History:
**	16-apr-98 (joea)
**		Created.
**	17-apr-98 (joea)
**		Add procedure return parameter to IIsw_execProcedure.
**	04-may-98 (joea)
**		Add IIsw_dbConnect.
**	04-May-1998 (shero03)
**	    Added IIsw_execLoop
**	11-may-98 (joea)
**		Add tranHandle parameter to IIsw_connect and IIsw_dbConnect.
**	08-jul-98 (abbjo03)
**		Add SW_PARM_DESC_DATA_INIT macro and document the others. Add
**		new arguments to IIsw_selectSingleton, IIsw_query and
**		IIsw_getColumns to support blobs. Add define for
**		IISW_MORE_SEGMENTS. Replace IIsw_queryNP by macro that calls
**		IIsw_query.
**	30-jul-98 (abbjo03)
**		Add support for repeated queries.
**	23-dec-98 (abbjo03)
**		Replace IIsw_getEvent by IIsw_catchEvent, IIsw_waitEvent and
**		IIsw_getEventInfo.
**	02-mar-99 (abbjo03)
**		Upgrade IIsw_initialize to version 2. Replace II_PTR error
**		handle parameters by IIAPI_GETEINFOPARM error parameter blocks.
**	19-apr-99 (abbjo03)
**		Upgrade IIsw_terminate to version 2.
**	18-may-99 (abbjo03)
**		Add IIsw_cancelEvent.
**	24-sep-99 (abbjo03) bug 99005
**		Add IIsw_releaseXID.
**	25-nov-2003 (gupsh03)
**		Added IIsw_initialize to version 3. Add version as input 
**		param.
**      26-mar-2004 (gupsh01)
**              Added getQinfoParm as input parameter to II_sw functions.
**/

/* return value for IIsw_getColumns if more blob segments need to be fetched */

# define IISW_MORE_SEGMENTS		(IIAPI_STATUS)(-1)


/* macro to replace IIsw_queryNP function (which is now deprecated) */

# define IIsw_queryNP(ch, th, qt, sh, qih, eh)  \
	IIsw_query(ch, th, qt, 0, NULL, NULL, NULL, NULL, sh, qih, eh)


/* macros to initialize an IIAPI_DESCRIPTOR */

/*{
** Name:	SW_QRYDESC_INIT - initialize query descriptor
**
** Description:
**	This macro initializes a query descriptor for the most common case,
**	i.e., as a query parameter for a non-DECIMAL datatype.
**
** Inputs:
**	ds	- IIAPI_DESCRIPTOR to be initialized
**	t	- datatype (e.g., IIAPI_INT_TYPE)
**	l	- length of parameter
**	n	- is parameter nullable?
*/
# define SW_QRYDESC_INIT(ds, t, l, n) \
	(ds).ds_dataType = (t), (ds).ds_nullable = (n), (ds).ds_length = (l), \
	(ds).ds_precision = 0, (ds).ds_scale = 0, \
	(ds).ds_columnType = IIAPI_COL_QPARM, (ds).ds_columnName = ERx("")


/*{
** Name:	SW_PROCDESC_INIT - initialize procedure parameter descriptor
**
** Description:
**	This macro initializes a database procedure parameter descriptor for the
**	most common case, i.e., non-DECIMAL datatype, the length of the
**	parameter can be determined by using the sizeof operator (i.e., do not
**	pass a char * for a string [char array] parameter), and the parameter is
**	not nullable.
**
** Inputs:
**	ds	- IIAPI_DESCRIPTOR to be initialized
**	t	- datatype (e.g., IIAPI_INT_TYPE)
**	v	- data value
**	nm	- name of the procedure parameter
*/
# define SW_PROCDESC_INIT(ds, t, v, nm) \
	(ds).ds_dataType = (t), (ds).ds_nullable = FALSE, \
	(ds).ds_length = sizeof(v), (ds).ds_precision = 0, (ds).ds_scale = 0, \
	(ds).ds_columnType = IIAPI_COL_PROCPARM, (ds).ds_columnName = (nm)


/* macros to initialize an IIAPI_DATAVALUE */

/*{
** Name:	SW_COLDATA_INIT - initialize retrieval data descriptor
**
** Description:
**	This macro initializes a data value descriptor to retrieve column
**	values. The dv_null member is not initialized.
**
** Inputs:
**	dv	- IIAPI_DATAVALUE to be initialized
**	val	- variable or buffer to receive data
*/
# define SW_COLDATA_INIT(dv, val)	(dv).dv_length = sizeof(val), \
					(dv).dv_value = &(val)

/*{
** Name:	SW_PARMDATA_INIT - initialize parameter data descriptor
**
** Description:
**	This macro initializes a data value descriptor to pass data, as in a
**	WHERE clause.
**
** Inputs:
**	dv	- IIAPI_DATAVALUE to be initialized
**	val	- address of variable or buffer to receive data
**	l	- size of variable or length of buffer
**	n	- is parameter NULL?
*/
# define SW_PARMDATA_INIT(dv, val, l, n)	(dv).dv_null = (n), \
						(dv).dv_length = (l), \
						(dv).dv_value = (val)

/*{
** Name:	SW_PROCDATA_INIT - initialize dbproc parameter data descriptor
**
** Description:
**	This macro initializes a data value descriptor to pass as a database
**	procedure parameter for the most common case, i.e., the length of the
**	parameter can be determined by using the sizeof operator and the
**	parameter is not NULL.
**
** Inputs:
**	dv	- IIAPI_DATAVALUE to be initialized
**	val	- parameter variable or buffer
*/
# define SW_PROCDATA_INIT(dv, val)		(dv).dv_null = FALSE, \
						(dv).dv_length = sizeof(val), \
						(dv).dv_value = (II_PTR)&(val)


/*{
** Name:	SW_PARM_DESC_DATA_INIT - initialize parameter descriptor
**
** Description:
**	This macro initializes both a parameter descriptor and a data value
**	descriptor to pass data, as in a WHERE clause.
**
** Inputs:
**	ds	- IIAPI_DESCRIPTOR to be initialized
**	dv	- IIAPI_DATAVALUE to be initialized
**	val	- parameter variable or buffer
**	t	- datatype (e.g., IIAPI_INT_TYPE)
*/
# define SW_PARM_DESC_DATA_INIT(dsc, dv, t, val) \
	SW_QRYDESC_INIT((dsc), (t), sizeof((val)), FALSE); \
	SW_PARMDATA_INIT((dv), &(val), sizeof((val)), FALSE)


/*{
** Name:	SW_PROC_DESC_DATA_INIT - initialize dbproc parameter descriptor
**
** Description:
**	This macro initializes both a parameter descriptor and a data value
**	descriptor for a database procedure parameter.
**
** Inputs:
**	ds	- IIAPI_DESCRIPTOR to be initialized
**	dv	- IIAPI_DATAVALUE to be initialized
**	nm	- name of the procedure parameter
**	t	- datatype (e.g., IIAPI_INT_TYPE)
**	val	- parameter variable or buffer
*/
# define SW_PROC_DESC_DATA_INIT(dsc, dv, nm, t, val) \
	SW_PROCDESC_INIT((dsc), (t), (val), (nm)); SW_PROCDATA_INIT((dv), (val))


/* macros to terminate strings after retrieval */

# define SW_CHA_TERM(s, ds)		(s)[(ds).dv_length] = EOS, \
					STtrmwhite((s))
# define SW_VCH_TERM(s)			*((char *)(s) + sizeof(short) + \
						*(short *)(s)) = EOS


/* structure for specifying a repeated query ID */

typedef struct
{
	II_INT	int_id[2];
	II_CHAR	*char_id;
} SW_REPEAT_QUERY_ID;

/* database event text structure */

typedef struct
{
	short	len;
	char	text[256];
} SW_EVT_TEXT;

/* typedef for pointer to callback function */

typedef II_VOID (II_FAR II_CALLBACK *IISW_CALLBACK)(II_PTR closure,
	II_PTR parmBlock);


/* API initialize/terminate functions */

FUNC_EXTERN IIAPI_STATUS IIsw_initialize(II_PTR *envHandle, i4 version);
FUNC_EXTERN IIAPI_STATUS IIsw_terminate(II_PTR envHandle); 

/* connection functions */

FUNC_EXTERN IIAPI_STATUS IIsw_setConnParam(II_PTR *connHandle, II_LONG paramID,
	II_CHAR *paramValue, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_connect(II_CHAR *target, II_CHAR *username,
	II_CHAR *password, II_PTR *connHandle, II_PTR *tranHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_dbConnect(II_CHAR *target, II_CHAR *dbUser,
	II_CHAR *dbPassword, II_PTR *connHandle, II_PTR *tranHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_disconnect(II_PTR *connHandle,
	IIAPI_GETEINFOPARM *errParm);

/* transaction control functions */

FUNC_EXTERN IIAPI_STATUS IIsw_commit(II_PTR *tranHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_rollback(II_PTR *tranHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_registerXID(II_UINT4 lowTran, II_UINT4 highTran,
	II_CHAR *tranName, II_PTR *tranHandle);
FUNC_EXTERN IIAPI_STATUS IIsw_prepareCommit(II_PTR *tranHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_releaseXID(II_PTR *tranHandle);

/* query functions */

/*
** Note:  The ppCallback function in IIsw_query will be called BEFORE calling
** IIapi_putParms, i.e., it's not the completion callback. Instead it should
** prepare the blob buffer for handing it off to IIapi_putParms.
*/
FUNC_EXTERN IIAPI_STATUS IIsw_query(II_PTR connHandle,
	II_PTR *tranHandle, II_CHAR *queryText, II_INT2 parmCount,
	IIAPI_DESCRIPTOR *parmDesc, IIAPI_DATAVALUE *parmData,
	IISW_CALLBACK ppCallback, II_PTR ppClosure, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_queryRepeated(II_PTR connHandle,
	II_PTR *tranHandle, II_CHAR *queryText, II_INT2 parmCount,
	IIAPI_DESCRIPTOR *parmDesc, IIAPI_DATAVALUE *parmData,
	SW_REPEAT_QUERY_ID *qryId, II_PTR *qryHandle, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_selectSingleton(II_PTR connHandle,
	II_PTR *tranHandle, II_CHAR *queryText, II_INT2 parmCount,
	IIAPI_DESCRIPTOR *parmDesc, IIAPI_DATAVALUE *parmData,
	II_INT2 columnCount, IIAPI_DATAVALUE *columnData,
	IISW_CALLBACK gcCallback, II_PTR gcClosure, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_selectSingletonRepeated(II_PTR connHandle,
	II_PTR *tranHandle, II_CHAR *queryText, II_INT2 parmCount,
	IIAPI_DESCRIPTOR *parmDesc, IIAPI_DATAVALUE *parmData,
	SW_REPEAT_QUERY_ID *qryId, II_PTR *qryHandle, II_INT2 columnCount,
	IIAPI_DATAVALUE *columnData, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_selectLoop(II_PTR connHandle, II_PTR *tranHandle,
	II_CHAR *queryText, II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_DATAVALUE *parmData, II_INT2 columnCount,
	IIAPI_DATAVALUE *columnData, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_selectLoopRepeated(II_PTR connHandle,
	II_PTR *tranHandle, II_CHAR *queryText, II_INT2 parmCount,
	IIAPI_DESCRIPTOR *parmDesc, IIAPI_DATAVALUE *parmData,
	SW_REPEAT_QUERY_ID *qryId, II_PTR *qryHandle, II_INT2 columnCount,
	IIAPI_DATAVALUE *columnData, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);

/* dbproc function */

FUNC_EXTERN IIAPI_STATUS IIsw_execProcedure(II_PTR connHandle,
	II_PTR *tranHandle, II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_DATAVALUE *parmData, II_LONG *procReturn, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_execProcDesc(II_PTR connHandle,
	II_PTR *tranHandle, II_INT2 parmCount, IIAPI_DESCRIPTOR *parmDesc,
	IIAPI_DATAVALUE *parmData, II_LONG *columnCount,
	IIAPI_DESCRIPTOR **columnDesc, II_PTR *stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm,
	IIAPI_GETEINFOPARM *errParm);

/* dbevent functions */

FUNC_EXTERN IIAPI_STATUS IIsw_catchEvent(II_PTR connHandle,
	IIAPI_CATCHEVENTPARM *catchParm, II_VOID (II_FAR II_CALLBACK *callback)
	(II_PTR closure, II_PTR parmBlock), II_PTR closure, II_PTR *evtHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_waitEvent(II_PTR connHandle, II_LONG timeout,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_getEventInfo(II_PTR evtHandle,
	SW_EVT_TEXT *eventInfo, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_cancelEvent(II_PTR evtHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_raiseEvent(II_PTR connHandle, II_PTR *tranHandle,
	II_CHAR *evtName, II_CHAR *evtText, II_PTR *stmtHandle,
	IIAPI_GETEINFOPARM *errParm);

/* miscellaneous functions */

FUNC_EXTERN IIAPI_STATUS IIsw_getColumns( II_PTR stmtHandle,
	II_INT2	*rowCount, II_INT2 colCount, IIAPI_DATAVALUE *colData,
	IISW_CALLBACK callback, II_PTR closure, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_getQueryInfo(II_PTR stmtHandle,
	IIAPI_GETQINFOPARM *qinfoParm, IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN II_LONG IIsw_getRowCount(IIAPI_GETQINFOPARM *qinfoParm);
FUNC_EXTERN II_VOID IIsw_getErrorInfo(II_PTR errHandle,
	IIAPI_GETEINFOPARM *errParm);
FUNC_EXTERN IIAPI_STATUS IIsw_close(II_PTR stmtHandle,
	IIAPI_GETEINFOPARM *errParm);

# endif
