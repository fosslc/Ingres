/*
** Copyright (c) 2004 Ingres Corporation
*/

/* Name: jdbcapi.h
**
** Description:
**	Definitions for the JDBC API interface.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**      17-Sep-99 (rajus01)
**          Added support for connection parameters.   
**	29-Sep-99 (gordy)
**	    Added more_segments structure member for BLOB data support.
**	 4-Nov-99 (gordy)
**	    Added connection timeout.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetching.
**	 6-Jan-00 (gordy)
**	    Replaced connection data with Connection Information Block.
**	19-May-00 (gordy)
**	    Added additional row counters and EOD result indicator to PCB.
**	25-Aug-00 (gordy)
**	    Statement name was not needed other than as a local variable.
**	    Added procedure return value.
**	11-Oct-00 (gordy)
**	    Handle nested requests using a callback stack.
**	15-Nov-00 (gordy)
**	    Return the READ_ONLY status of a cursor.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions: register and
**	    release transaction ID, prepare (2PC) transaction.
**	18-Apr-01 (gordy)
**	    Added distributed transaction ID to connection parameters.
**       7-Jun-05 (gordy)
**          Added jdbc_xact_abort() to handle transaction aborts.
*/

#ifndef _JDBC_API_INCLUDED_
#define _JDBC_API_INCLUDED_

/*
** To permit nested requests, the single callback
** function is replaced with a callback stack and
** functions to push/pop callbacks.  The stack
** size is fixed.  Currently, only three stack
** levels are required: jdbc_purge_stmt(),
** jdbc_del_stmt(), jdbc_api_cancel().
*/
#define	JDBC_PCB_STACK	5

typedef	void (*JDBC_PCB_CALLBACK)( PTR ptr );

typedef struct
{

    JDBC_CCB		*ccb;		/* Connection control block */
    JDBC_RCB		*rcb;		/* Request control block (output) */
    JDBC_SCB		*scb;		/* Statement control block */

    u_i1		callbacks;
    JDBC_PCB_CALLBACK	callback[ JDBC_PCB_STACK ];

    IIAPI_STATUS	api_status;	/* API function status */
    STATUS		api_error;	/* API/DBMS error code */

    struct
    {
	char			*name;

	union
	{
	    IIAPI_SETCONPRMPARM scon;
	    IIAPI_CONNPARM	conn;
	    IIAPI_DISCONNPARM	disc;
	    IIAPI_ABORTPARM	abort;
	    IIAPI_PREPCMTPARM	prep;
	    IIAPI_COMMITPARM	commit;
	    IIAPI_ROLLBACKPARM	roll;
	    IIAPI_AUTOPARM	ac;
	    IIAPI_QUERYPARM	query;
	    IIAPI_SETDESCRPARM	sDesc;
	    IIAPI_PUTPARMPARM	pParm;
	    IIAPI_GETDESCRPARM	gDesc;
	    IIAPI_GETCOLPARM	gCol;
	    IIAPI_GETQINFOPARM	gqi;
	    IIAPI_CANCELPARM	cancel;
	    IIAPI_CLOSEPARM	close;
	} parm;
    } api;

    /*
    ** Data areas for various API operations.
    */
    union
    {
	struct
	{
	    i4			timeout;
	    char		*database;
	    char		*username;
	    char		*password;
	    IIAPI_TRAN_ID	distXID;
	    u_i2		param_cnt;
	    CONN_PARM		*parms;
	} conn;

	struct
	{
	    PTR		conn;
	} disc;

	struct
	{
	    IIAPI_TRAN_ID	distXID;
	} tran;

	struct
	{
	    IIAPI_QUERYTYPE	type;
	    bool		params;
	    char		*text;
	} query;

	struct
	{
	    II_INT2		count;
	    IIAPI_DESCRIPTOR	*desc;
	} desc;

	struct
	{
	    II_INT2		max_row;
	    II_INT2		tot_row;
	    II_INT2		row_cnt;
	    II_INT2		col_cnt;
	    bool		more_segments;
	    IIAPI_DATAVALUE	*data;
	} data;

    } data;

    struct
    {
	u_i2	flags;

#define	PCB_RSLT_ROWCOUNT	0x01
#define	PCB_RSLT_STMT_ID	0x02
#define	PCB_RSLT_FETCH_LIMIT	0x04
#define	PCB_RSLT_PROC_RET	0x08
#define	PCB_RSLT_XACT_END	0x10
#define	PCB_RSLT_EOD		0x20
#define	PCB_RSLT_READ_ONLY	0x40

	i4		rowcount;
	i4		fetch_limit;
	i4		proc_ret;
	STMT_ID		stmt_id;

    } result;

} JDBC_PCB;

FUNC_EXTERN bool	jdbc_push_callback( JDBC_PCB *, JDBC_PCB_CALLBACK );
FUNC_EXTERN void	jdbc_pop_callback( JDBC_PCB * );
FUNC_EXTERN STATUS	jdbc_api_init( void );
FUNC_EXTERN void	jdbc_api_term( void );
FUNC_EXTERN void	jdbc_api_connect( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_setConnParms( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_disconnect( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_abort( JDBC_PCB * );
FUNC_EXTERN bool	jdbc_api_regXID( JDBC_CCB *, IIAPI_TRAN_ID * );
FUNC_EXTERN bool	jdbc_api_relXID( JDBC_CCB * );
FUNC_EXTERN void	jdbc_api_prepare( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_commit( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_rollback( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_autocommit( JDBC_PCB *, bool );
FUNC_EXTERN void	jdbc_api_query( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_setDesc( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_putParm( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_getDesc( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_getCol( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_gqi( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_cancel( JDBC_PCB * );
FUNC_EXTERN void	jdbc_api_close( JDBC_PCB *, PTR );
FUNC_EXTERN STATUS	jdbc_api_format( JDBC_CCB *, 
					 IIAPI_DESCRIPTOR *, 
					 IIAPI_DATAVALUE *,
					 IIAPI_DESCRIPTOR *, 
					 IIAPI_DATAVALUE * );

FUNC_EXTERN JDBC_PCB *	jdbc_new_pcb( JDBC_CCB *, bool );
FUNC_EXTERN void	jdbc_del_pcb( JDBC_PCB * );

FUNC_EXTERN JDBC_SCB *	jdbc_new_stmt( JDBC_CCB *, JDBC_PCB *, bool );
FUNC_EXTERN void	jdbc_del_stmt( JDBC_PCB *, JDBC_SCB * );
FUNC_EXTERN void	jdbc_purge_stmt( JDBC_PCB * );
FUNC_EXTERN bool	jdbc_active_stmt( JDBC_CCB * );
FUNC_EXTERN JDBC_SCB *	jdbc_find_stmt( JDBC_CCB *, STMT_ID * );
FUNC_EXTERN JDBC_SCB *	jdbc_find_cursor( JDBC_CCB *, char * );

FUNC_EXTERN void        jdbc_xact_abort( JDBC_PCB * );
FUNC_EXTERN void	jdbc_xact_check( JDBC_PCB *, u_i1 );

FUNC_EXTERN void	jdbc_send_done( JDBC_CCB *, JDBC_RCB **, JDBC_PCB * );
FUNC_EXTERN void	jdbc_send_error( JDBC_RCB **, 
					 u_i1, STATUS, char *, u_i2, char * );

#endif /* _JDBC_API_INCLUDED_ */

