/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/* Name: gcdapi.h
**
** Description:
**	Definitions for the Data Access Server API interface.
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	14-Feb-03 (gordy)
**	    Added savepoint parm to PCB, gcd_api_savepoint(), and 
**	    table/object key results.
**	20-Aug-03 (gordy)
**	    Added statement closed result flag.
**	 1-Oct-03 (gordy)
**	    Added gcd_send_result(), gcd_fetch() to return rows with
**	    query results.  Removed total rows (row count sufficient).
**	21-Apr-06 (gordy)
**	    Added gcd_send_trace().
**	14-Jul-06 (gordy)
**	    Added support for XA requests.
**	15-Nov-06 (gordy)
**	    Query flags required at API Version 6 (LOB Locators).
**	 4-Apr-07 (gordy)
**	    Added support for scrollable cursors.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added gcd_api_relenv(). gcd_api_init() takes API version as
**	    input, returns status and API environment handle.
**	25-Mar-10 (gordy)
**	    Added batch processing support.
*/

#ifndef _GCD_API_INCLUDED_
#define _GCD_API_INCLUDED_

/*
** To permit nested requests, the single callback
** function is replaced with a callback stack and
** functions to push/pop callbacks.  The stack
** size is fixed.  Currently, only three stack
** levels are required: gcd_purge_stmt(),
** gcd_del_stmt(), gcd_api_cancel().
*/
#define	GCD_PCB_STACK	5

typedef	void (*GCD_PCB_CALLBACK)( PTR ptr );

typedef struct
{

    GCD_CCB		*ccb;		/* Connection control block */
    GCD_RCB		*rcb;		/* Request control block (output) */
    GCD_SCB		*scb;		/* Statement control block */

    u_i1		callbacks;
    GCD_PCB_CALLBACK	callback[ GCD_PCB_STACK ];

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
	    IIAPI_SAVEPTPARM	sp;
	    IIAPI_XASTARTPARM	start;
	    IIAPI_XAENDPARM	end;
	    IIAPI_XAPREPPARM	xaprep;
	    IIAPI_XACOMMITPARM	xacomm;
	    IIAPI_XAROLLPARM	xaroll;
	    IIAPI_PREPCMTPARM	prep;
	    IIAPI_COMMITPARM	commit;
	    IIAPI_ROLLBACKPARM	roll;
	    IIAPI_AUTOPARM	ac;
	    IIAPI_QUERYPARM	query;
	    IIAPI_BATCHPARM	batch;
	    IIAPI_SETDESCRPARM	sDesc;
	    IIAPI_PUTPARMPARM	pParm;
	    IIAPI_GETDESCRPARM	gDesc;
	    IIAPI_POSPARM	pos;
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
	    II_PTR		conn;
	} disc;

	struct
	{
	    II_PTR		savepoint;
	    u_i4		xa_flags;
	    IIAPI_TRAN_ID	distXID;
	} tran;

	struct
	{
	    IIAPI_QUERYTYPE	type;
	    char		*text;
	    bool		params;
	    II_ULONG		flags;
	} query;

	struct
	{
	    II_INT2		count;
	    IIAPI_DESCRIPTOR	*desc;
	} desc;

	struct
	{
	    II_UINT2		reference;
	    II_INT4		offset;
	    II_INT2		max_row;
	    II_INT2		row_cnt;
	    II_INT2		col_cnt;
	    bool		more_segments;
	    IIAPI_DATAVALUE	*data;
	} data;

    } data;

    struct
    {
	u_i2	flags;

#define	PCB_RSLT_STMT_ID	0x0001		/* Statement ID is set */
#define	PCB_RSLT_FETCH_LIMIT	0x0002		/* Fetch limit is set */
#define	PCB_RSLT_READ_ONLY	0x0004		/* READONLY cursor */
#define	PCB_RSLT_SCROLL		0x0008		/* Scrollable cursor */
#define	PCB_RSLT_ROW_STATUS	0x0010		/* Row status & pos are set */
#define	PCB_RSLT_ROW_COUNT	0x0040		/* Row count is set */
#define	PCB_RSLT_EOD		0x0080		/* End-of-data reached */
#define	PCB_RSLT_PROC_RET	0x0100		/* Procedure return is set */
#define	PCB_RSLT_TBLKEY		0x0200		/* Table-key is set */
#define	PCB_RSLT_OBJKEY		0x0400		/* Object-key is set */
#define	PCB_RSLT_CLOSED		0x0800		/* Statment closed */
#define	PCB_RSLT_XACT_END	0x1000		/* Transaction ended */

	u_i2		row_status;
	i4		row_position;
	i4		row_count;
	i4		fetch_limit;
	i4		proc_ret;
	STMT_ID		stmt_id;
	u_i1		tblkey[ IIAPI_TBLKEYSZ ];
	u_i1		objkey[ IIAPI_OBJKEYSZ ];

    } result;

} GCD_PCB;

FUNC_EXTERN bool	gcd_push_callback( GCD_PCB *, GCD_PCB_CALLBACK );
FUNC_EXTERN void	gcd_pop_callback( GCD_PCB * );
FUNC_EXTERN STATUS	gcd_api_init( u_i2, PTR * );
FUNC_EXTERN void	gcd_api_term( PTR );
FUNC_EXTERN void	gcd_api_connect( GCD_PCB * );
FUNC_EXTERN void	gcd_api_setConnParms( GCD_PCB * );
FUNC_EXTERN void	gcd_api_disconnect( GCD_PCB * );
FUNC_EXTERN void	gcd_api_abort( GCD_PCB * );
FUNC_EXTERN void	gcd_api_savepoint( GCD_PCB * );
FUNC_EXTERN bool	gcd_api_regXID( GCD_CCB *, IIAPI_TRAN_ID * );
FUNC_EXTERN bool	gcd_api_relXID( GCD_CCB * );
FUNC_EXTERN void	gcd_api_xaStart( GCD_PCB * );
FUNC_EXTERN void	gcd_api_xaEnd( GCD_PCB * );
FUNC_EXTERN void	gcd_api_xaPrepare( GCD_PCB * );
FUNC_EXTERN void	gcd_api_xaCommit( GCD_PCB * );
FUNC_EXTERN void	gcd_api_xaRollback( GCD_PCB * );
FUNC_EXTERN void	gcd_api_prepare( GCD_PCB * );
FUNC_EXTERN void	gcd_api_commit( GCD_PCB * );
FUNC_EXTERN void	gcd_api_rollback( GCD_PCB * );
FUNC_EXTERN void	gcd_api_autocommit( GCD_PCB *, bool );
FUNC_EXTERN void	gcd_api_query( GCD_PCB * );
FUNC_EXTERN void	gcd_api_batch( GCD_PCB * );
FUNC_EXTERN void	gcd_api_setDesc( GCD_PCB * );
FUNC_EXTERN void	gcd_api_putParm( GCD_PCB * );
FUNC_EXTERN void	gcd_api_getDesc( GCD_PCB * );
FUNC_EXTERN void	gcd_api_position( GCD_PCB * );
FUNC_EXTERN void	gcd_api_getCol( GCD_PCB * );
FUNC_EXTERN void	gcd_api_gqi( GCD_PCB * );
FUNC_EXTERN void	gcd_api_cancel( GCD_PCB * );
FUNC_EXTERN void	gcd_api_close( GCD_PCB *, PTR );
FUNC_EXTERN STATUS	gcd_api_format( GCD_CCB *, 
					 IIAPI_DESCRIPTOR *, 
					 IIAPI_DATAVALUE *,
					 IIAPI_DESCRIPTOR *, 
					 IIAPI_DATAVALUE * );

FUNC_EXTERN GCD_PCB *	gcd_new_pcb( GCD_CCB * );
FUNC_EXTERN void	gcd_del_pcb( GCD_PCB * );

FUNC_EXTERN GCD_SCB *	gcd_new_stmt( GCD_CCB *, GCD_PCB * );
FUNC_EXTERN void	gcd_del_stmt( GCD_PCB *, GCD_SCB * );
FUNC_EXTERN void	gcd_purge_stmt( GCD_PCB * );
FUNC_EXTERN bool	gcd_active_stmt( GCD_CCB * );
FUNC_EXTERN GCD_SCB *	gcd_find_stmt( GCD_CCB *, STMT_ID * );
FUNC_EXTERN GCD_SCB *	gcd_find_cursor( GCD_CCB *, char * );

FUNC_EXTERN void	gcd_xact_abort( GCD_PCB * );
FUNC_EXTERN void	gcd_xact_check( GCD_PCB *, u_i1 );

FUNC_EXTERN void	gcd_fetch( GCD_CCB *, GCD_SCB *, GCD_PCB *, u_i4 );

FUNC_EXTERN void	gcd_send_done( GCD_CCB *, GCD_RCB **, GCD_PCB * );
FUNC_EXTERN void	gcd_send_result( GCD_CCB *, 
					GCD_RCB **, GCD_PCB *, u_i1 );
FUNC_EXTERN void	gcd_send_error( GCD_CCB *, GCD_RCB **, 
					u_i1, STATUS, char *, u_i2, char * );
FUNC_EXTERN void	gcd_send_trace( GCD_CCB *, u_i2, char * );

#endif /* _GCD_API_INCLUDED_ */

