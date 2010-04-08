/*
** Copyright (c) 2006, 2010 Ingres Corporation
*/

/* Name: gcdmsg.h
**
** Description:
**	Definitions for the Data Access Messaging Protocol (DAMP)
**	Transport (TL) and Message (MSG) layer protocols.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**      17-Sep-99 (rajus01)
**          Added connection parameters.   
**	22-Sep-99 (gordy)
**	    Added character set to TL connection parameters.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	26-Oct-99 (gordy)
**	    Added query types for cursor positioned DELETE/UPDATE.
**	 4-Nov-99 (gordy)
**	    Added timeout connection parameter.
**	 4-Nov-99 (gordy)
**	    Added TL packet for interrupt.
**	13-Dec-99 (gordy)
**	    Added parameters for cursor pre-fetch.
**	17-Jan-00 (gordy)
**	    Added REQUEST message for JDBC server internal requests.
**	19-May-00 (gordy)
**	    Added end-of-data result parameter: JDBC_RP_EOD.
**	14-Jun-00 (gordy)
**	    Changed the descriptor nullable member to flags to
**	    support procedure parameter extensions.  Added a
**	    REQUEST sub-type for procedure parameter names and
**	    message paramters for schema and procedure names.
**	    Added the procedure return value to the RESULT msg.
**	17-Oct-00 (gordy)
**	    Added connection parameter for autocommit mode and
**	    connection parameter values.
**	15-Nov-00 (gordy)
**	    Added result parameter for read-only cursors in addition
**	    to the fetch-limit.
**	20-Jan-01 (gordy)
**	    Added message protocol level 2 for changes since 19-May-00.
**	 7-Mar-01 (gordy)
**	    Added new REQUEST operation for dbmsinfo() (@ MSG_PROTO_2).
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions: begin and
**	    prepare transaction messages, XA transaction ID params.
**	 2-Apr-01 (gordy)
**	    Added JDBC_XP_ING_XID.
**	18-Apr-01 (gordy)
**	    Added support for distributed transaction management connections.
**	    Added JDBC_MSG_P_DTMC, JDBC_CP_II_XID, JDBC_CP_XA_FRMT,
**	    JDBC_CP_XA_GTID, JDBC_CP_XA_BQUAL, JDBC_REQ_2PC_XIDS,
**	    JDBC_RP_DB_NAME.
**	10-May-01 (gordy)
**	    Added functions to put/get UCS2 data.
**      13-Aug-01 (loera01) SIR 105521
**          Added new proc parameter flag JDBC_DSC_GTT to support global temp 
**          table parameters for db procedures.
**	20-Aug-01 (gordy)
**	    Added query flag parameter and READONLY flag.
**	20-May-2002 (hanje04)
**	    Backed out change 457149 (no comment), not needed in main.
**	01-Oct-2002 (wansh01)
**          Added JDBC_CP_LOGIN_TYPE, JDBC_CPV_LOGIN_LOCAL,
** 	    JDBC_CPV_LOGIN_REMOTE. 	
**	01-Nov-2002 (wansh01)
**          Added connection parameter value JDBC_CPV_LOGIN_USER.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	26-Dec-02 (gordy)
**	    Added TL protocol level 2, new packet header ID and connection
**	    parameter DAM_TL_CP_CHARSET_ID.  Added MSG protocol level 3,
**	    new message header ID and connection parameter MSG_CP_AUTO_STATE.
**	15-Jan-03 (gordy)
**	    Added DAM_TL_CP_MSG_ID, MSG_CP_CLIENT_USER, MSG_CP_CLIENT_HOST
**	    and MSG_CP_CLIENT_ADDR.  Renamed some messaging functions.
**	14-Feb-03 (gordy)
**	    Added savepoints: MSG_XACT_SAVEPOINT, MSG_XP_SAVEPOINT.  Added
**	    DONE parameters MSG_RP_TBLKEY, MSG_RP_OBJKEY.
**	 7-Jul-03 (gordy)
**	    Added connection parameters MSG_CP_TIMEZONE, MSG_CP_DECIMAL,
**	    MSG_CP_DATE_FRMT, MSG_CP_MNY_FRMT, MSG_CP_MNY_PREC.
**	20-Aug-03 (gordy)
**	    Result flags consolidated into single parameter: MSG_RP_FLAGS,
**	    MSG_RF_XACT_END, MSG_RF_READ_ONLY, MSG_RF_EOD, MSG_RF_CLOSED.
**	    Added query flag MSG_QF_AUTO_CLOSE.
**	 1-Oct-03 (gordy)
**	    Renamed DONE message to RESULT.  Added query flags
**	    MSG_QF_FETCH_FIRST, MSG_QF_FETCH_ALL.
**	15-Mar-04 (gordy)
**	    Added functions to handle i8 values.
**	16-Jun-04 (gordy)
**	    Added session mask to ML initialization parameters.
**	10-May-05 (gordy)
**	    Added MSG_PROTO_4 and MSG_XACT_ABORT.
**	21-Apr-06 (gordy)
**	    Added MSG_PROTO_5, messsage MSG_INFO.
**	31-May-06 (gordy)
**	    Added SQL_INTERVAL at protocol level 5.
**	28-Jun-06 (gordy)
**	    Added MSG_QRY_DESC, MSG_QF_DESC_OUTPUT, MSG_DSC_POUT.
**	14-Jul-06 (gordy)
**	    Added MSG_ET_XA, MSG_XACT_END, MSG_XP_XA_FLAGS 
**	    and XA flags values.
**	31-Aug-06 (gordy)
**	    Added MSG_REQ_PARAM_INFO.
**	 6-Nov-06 (gordy)
**	    Allow query text to be longer than 64K.
**	15-Nov-06 (gordy)
**	    Added support for LOB locators at protocol level 6.
**	 5-Apr-07 (gordy)
**	    Added support for scrollable cursors.
**	23-Jul-07 (gordy)
**	    Added connection parameter MSG_CP_DATE_ALIAS.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level.
**	26-Oct-09 (gordy)
**	    Added MSG_PROTO_7 and SQL_BOOLEAN.
**	25-Mar-10 (gordy)
**	    Added support for batch processing: message header flag
**	    MSG_HDR_EOB, batch message MSG_BATCH.
*/

#ifndef _GCD_MSG_INCLUDED_
#define _GCD_MSG_INCLUDED_

#include <cvgcc.h>

/*
** Transport Layer (DAM-TL) message structures and definitions
*/

typedef struct	    /* TL message header */
{

    u_i4	tl_id;		/* Header ID */

#define	DAM_TL_ID_1	0x4C54434A	/* Header ID: ascii "JCTL" */
#define	DAM_TL_ID_2	0x4C544D44	/* Header ID: ascii "DMTL" */

    u_i2	msg_id;		/* Message ID */

#define	DAM_TL_CR	0x5243		/* Connect request: ascii "CR" */
#define	DAM_TL_CC	0x4343		/* Connect confirm: ascii "CC" */
#define	DAM_TL_DT	0x5444		/* Data: ascii "DT" */
#define	DAM_TL_DR	0x5244		/* Disconnect request: ascii "DR" */
#define	DAM_TL_DC	0x4344		/* Disconnect confirm: ascii "DC" */
#define DAM_TL_INT	0x5249		/* Interrupt: ascii "IR" */

} DAM_TL_HDR;

#define	DAM_TL_HDR_SZ		(CV_N_I4_SZ + CV_N_I2_SZ)

typedef struct		/* TL parameters */
{

    u_i1	param_id;	/* Parameter ID */
    u_i1	pl1;		/* Parameter length or 255 */
    u_i2	pl2;		/* Optional: present if param_len1 == 255 */
    u_i1	param[1];	/* Variable length: explicit */

} DAM_TL_PARAM;

typedef struct	    /* TL CR/CC message */
{

    DAM_TL_PARAM	params;

#define	DAM_TL_CP_PROTO		0x01	/* Protocol level */

#define	DAM_TL_PROTO_1		0x01
#define	DAM_TL_PROTO_2		0x02
#define	DAM_TL_DFLT_PROTO	DAM_TL_PROTO_2

#define	DAM_TL_CP_SIZE		0x02	/* Buffer size */

/*
** Our TL protocol should support buffer sizes upto
** 32K.  There is an understood limitation between
** GCC and the GCC CL of 8K.
*/
#define	DAM_TL_PKT_MAX		0x0F	/* 32K packet size */
#define	DAM_TL_PKT_DEF		0x0D	/* GCC CL limitation */
#define	DAM_TL_PKT_MIN		0x0A	/* 1K packet size */

#define	DAM_TL_CP_MSG		0x03	/* MSG layer info */
#define	DAM_TL_CP_CHRSET	0x04	/* Character set */
#define	DAM_TL_CP_CHRSET_ID	0x05	/* Character set ID */
#define	DAM_TL_CP_MSG_ID	0x06	/* MSG layer ID */

#define	DAM_TL_DMML_ID		0x4C4D4D44	/* DAM-ML ID: 'DMML' */	

} DAM_TL_CONN;

typedef struct	    /* TL DR/DC message */
{

    DAM_TL_PARAM	params;		/* Optional */

#define	DAM_TL_DP_ERR		0x01	/* Error code */

} DAM_TL_DISC;

/*
** Messaging Layer (DAM-ML) message structures and definitions.
*/

/* MSG protocol initialization parameters negotiated during TL connect */

#define	MSG_P_PROTO		0x01	/* Protocol Level */
#define	MSG_P_DTMC		0x02	/* Dist Trans Mgmt Connection */
#define	MSG_P_MASK		0x03	/* Session mask */

#define	MSG_PROTO_1		0x01	/* Protocol Levels */
#define	MSG_PROTO_2		0x02
#define	MSG_PROTO_3		0x03
#define	MSG_PROTO_4		0x04
#define	MSG_PROTO_5		0x05
#define	MSG_PROTO_6		0x06
#define	MSG_PROTO_7		0x07
#define	MSG_DFLT_PROTO		MSG_PROTO_7

typedef struct	    /* MSG header */
{

    u_i4    hdr_id;

#define	MSG_HDR_ID_1		0x4342444A	/* Header ID: 'JDBC' */	
#define	MSG_HDR_ID_2		0x4C4D4D44	/* Header ID: 'DMML' */	

    u_i2    msg_len;
    u_i1    msg_id;

#define	MSG_UNUSED		0x00
#define	MSG_CONNECT		0x01
#define	MSG_DISCONN		0x02
#define	MSG_XACT		0x03
#define	MSG_QUERY		0x04
#define	MSG_CURSOR		0x05
#define	MSG_DESC		0x06
#define	MSG_DATA		0x07
#define	MSG_ERROR		0x08
#define	MSG_RESULT		0x09
#define	MSG_REQUEST		0x0A
#define	MSG_INFO		0x0B
#define	MSG_BATCH		0x0C

#define	DAM_ML_MSG_CNT		13

    u_i1    msg_flags;

#define	MSG_HDR_EOD		0x01
#define	MSG_HDR_EOG		0x02
#define	MSG_HDR_EOB		0x04

} DAM_ML_HDR;

#define	MSG_HDR_LEN		(CV_N_I4_SZ + CV_N_I2_SZ + (CV_N_I1_SZ * 2))

/*
** Message parameters.
*/

typedef struct
{

    u_i2    param_id;
    u_i2    val_len;
    u_i1    value[ 1 ];		/* Variable length: explicit */

} DAM_ML_PM;

/*
** Connection message.
*/

typedef struct
{

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_CP_DATABASE		0x01
#define	MSG_CP_USERNAME		0x02
#define	MSG_CP_PASSWORD		0x03
#define	MSG_CP_DBUSERNAME	0x04
#define	MSG_CP_GROUP_ID		0x05
#define	MSG_CP_ROLE_ID		0x06
#define	MSG_CP_DBPASSWORD	0x07
#define MSG_CP_TIMEOUT		0x08
#define	MSG_CP_CONNECT_POOL	0x09
#define	MSG_CP_AUTO_MODE	0x0A
#define	MSG_CP_II_XID		0x0B
#define	MSG_CP_XA_FRMT		0x0C
#define	MSG_CP_XA_GTID		0x0D
#define	MSG_CP_XA_BQUAL		0x0E
#define	MSG_CP_LOGIN_TYPE   	0x0F
#define	MSG_CP_AUTO_STATE	0x10
#define	MSG_CP_CLIENT_USER	0x11
#define	MSG_CP_CLIENT_HOST	0x12
#define	MSG_CP_CLIENT_ADDR	0x13
#define	MSG_CP_TIMEZONE		0x14
#define	MSG_CP_DECIMAL		0x15
#define	MSG_CP_DATE_FRMT	0x16
#define	MSG_CP_MNY_FRMT		0x17
#define	MSG_CP_MNY_PREC		0x18
#define	MSG_CP_DATE_ALIAS	0x19

#define	MSG_CPV_POOL_OFF	0
#define	MSG_CPV_POOL_ON		1
#define	MSG_CPV_XACM_DBMS	0
#define	MSG_CPV_XACM_SINGLE	1
#define	MSG_CPV_XACM_MULTI	2
#define	MSG_CPV_LOGIN_LOCAL	0
#define	MSG_CPV_LOGIN_REMOTE	1
#define	MSG_CPV_LOGIN_USER  	2
#define	MSG_CPV_AUTO_OFF	0
#define	MSG_CPV_AUTO_ON		1

} DAM_ML_CONN;

/*
** Transaction message.
*/

typedef struct
{

    u_i2	xact_op;

#define	MSG_XACT_COMMIT		0x01
#define	MSG_XACT_ROLLBACK	0x02
#define	MSG_XACT_AC_ENABLE	0x03
#define	MSG_XACT_AC_DISABLE	0x04
#define	MSG_XACT_BEGIN		0x05
#define	MSG_XACT_PREPARE	0x06
#define	MSG_XACT_SAVEPOINT	0x07
#define	MSG_XACT_ABORT		0x08
#define	MSG_XACT_END		0x09

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_XP_II_XID		0x01
#define	MSG_XP_XA_FRMT		0x02
#define	MSG_XP_XA_GTID		0x03
#define	MSG_XP_XA_BQUAL		0x04
#define	MSG_XP_SAVEPOINT	0x05
#define	MSG_XP_XA_FLAGS		0x06

#define	MSG_XA_JOIN		0x00200000
#define	MSG_XA_FAIL		0x20000000
#define	MSG_XA_1PC		0x40000000

} DAM_ML_TRAN;

/*
** Request message.
*/

typedef struct
{
    u_i2	req_id;

#define	MSG_REQ_CAPABILITY	0x01	/* DBMS capabilities */
#define	MSG_REQ_PARAM_NAMES	0x02	/* Parameter names */
#define	MSG_REQ_DBMSINFO	0x03	/* dbmsinfo() */
#define	MSG_REQ_2PC_XIDS	0x04	/* Two-phase Commit Transaction IDs */
#define	MSG_REQ_PARAM_INFO	0x05	/* Parameter info */

#define	DAM_ML_REQ_MAX		0x05

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_RQP_SCHEMA_NAME	0x01	/* Schema name */
#define	MSG_RQP_PROC_NAME	0x02	/* Procedure name */
#define MSG_RQP_INFO_ID		0x03	/* dbmsinfo( INFO_ID ) */
#define	MSG_RQP_DB_NAME		0x04	/* Database name */

} DAM_ML_RQST;

/*
** Query message.
*/

typedef struct
{
    u_i2	query_type;

#define	MSG_QRY_EXQ		0x01	/* Execute a query */
#define	MSG_QRY_OCQ		0x02	/* Open cursor for query */
#define	MSG_QRY_PREP		0x03	/* Prepare a statement */
#define	MSG_QRY_EXS		0x04	/* Execute a statement */
#define	MSG_QRY_OCS		0x05	/* Open cursor for a statement */
#define	MSG_QRY_EXP		0x06	/* Execute a procedure */
#define	MSG_QRY_CDEL		0x07	/* Cursor delete */
#define	MSG_QRY_CUPD		0x08	/* Cursor update */
#define	MSG_QRY_DESC		0x09	/* Describe a statement */

#define	DAM_ML_QRY_MAX		0x09

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_QP_QTXT		0x01	/* Query text */
#define	MSG_QP_CRSR_NAME	0x02	/* Cursor name */
#define	MSG_QP_STMT_NAME	0x03	/* Statement name */
#define	MSG_QP_SCHEMA_NAME	0x04	/* Procedure schema */
#define	MSG_QP_PROC_NAME	0x05	/* Procedure name */
#define	MSG_QP_FLAGS		0x06	/* Query flags */

#define	MSG_QF_READONLY		0x0001	/* READONLY cursor */
#define	MSG_QF_AUTO_CLOSE	0x0002	/* Close statement at EOF */
#define	MSG_QF_FETCH_FIRST	0x0004	/* Fetch First block of rows */
#define	MSG_QF_FETCH_ALL	0x0008	/* Fetch All rows */
#define	MSG_QF_DESC_OUTPUT	0x0010	/* Describe Output */
#define	MSG_QF_LOCATORS		0x0020	/* LOB Locators requested */
#define	MSG_QF_SCROLL		0x0040	/* Scrollable cursor */

} DAM_ML_QRY;

/*
** Batch message.
*/

typedef struct
{
    u_i2	batch_type;

#define	MSG_BAT_EXQ		0x01	/* Execute a query */
#define	MSG_BAT_EXS		0x04	/* Execute a statement */
#define	MSG_BAT_EXP		0x06	/* Execute a procedure */

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_BP_QTXT		0x01	/* Query text */
#define	MSG_BP_STMT_NAME	0x03	/* Statement name */
#define	MSG_BP_SCHEMA_NAME	0x04	/* Procedure schema */
#define	MSG_BP_PROC_NAME	0x05	/* Procedure name */
#define	MSG_BP_FLAGS		0x06	/* Query flags */

#define	MSG_BF_REPEAT		0x0080	/* Repeat previous query */

} DAM_ML_BATCH;

/*
** Cursor message.
*/

typedef struct
{
    u_i2	cursor_op;

#define	MSG_CUR_CLOSE		0x01
#define	MSG_CUR_FETCH		0x02

    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_CUR_STMT_ID		0x01
#define	MSG_CUR_PRE_FETCH	0x02
#define	MSG_CUR_POS_ANCHOR	0x03
#define	MSG_CUR_POS_OFFSET	0x04

#define	MSG_POS_BEGIN		0x01
#define	MSG_POS_END		0x02
#define	MSG_POS_CURRENT		0x03

} DAM_ML_CUR;

/*
** Data descriptors
*/

typedef struct
{
    i2		sql_type;
    i2		dbms_type;
    u_i2	length;
    u_i1	precision;
    u_i1	scale;
    u_i1	flags;

#define		MSG_DSC_NULL		0x01	/* Nullable */
#define		MSG_DSC_PIN 		0x02	/* Procedure param: IN */
#define		MSG_DSC_PIO		0x04	/* Procedure param: INOUT */
#define		MSG_DSC_GTT		0x08	/* Proc param: global tmp tbl */
#define		MSG_DSC_POUT		0x10	/* Procedure param: OUT */

    u_i2	name_len;
    char	name[ 1 ];	/* Variable length: explicit */

} DAM_ML_DS;

/*
** Descriptor message.
*/

typedef struct
{
    u_i2	count;
    DAM_ML_DS	desc[ 1 ];	/* Variable length: explicit */

} DAM_ML_DSC;

/*
** Error message.
*/

typedef struct
{

    u_i4	err_code;
    char	sqlState[ 5 ];
    u_i1	msg_type;

#define	MSG_ET_MSG		0x00	/* User message */
#define	MSG_ET_WRN		0x01	/* Warning message */
#define	MSG_ET_ERR		0x02	/* Error message */
#define	MSG_ET_XA		0x03	/* XA Error message */

    u_i2	msg_len;
    char	message[ 1 ];		/* Variable length: explicit */

} DAM_ML_ERR;

/*
** Result message.
*/

typedef struct
{
    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_RP_ROWCOUNT		0x01	/* Row count */
#define	MSG_RP_XACT_END		0x02	/* -- deprecated (see flags) -- */
#define	MSG_RP_STMT_ID		0x03	/* Statement ID */
#define MSG_RP_FETCH_LIMIT	0x04	/* Fetch row limit */
#define	MSG_RP_EOD		0x05	/* -- deprecated (see flags) -- */
#define	MSG_RP_PROC_RESULT	0x06	/* Procedure return value */
#define	MSG_RP_READ_ONLY	0x07	/* -- deprecated (see flags) -- */
#define	MSG_RP_TBLKEY		0x08	/* Table key */
#define	MSG_RP_OBJKEY		0x09	/* Object key */
#define	MSG_RP_FLAGS		0x0A	/* Result Flags */
#define	MSG_RP_ROW_STATUS	0x0B	/* Row status */
#define	MSG_RP_ROW_POS		0x0C	/* Row position */

#define	MSG_RF_XACT_END		0x01	/* Transaction completed */
#define	MSG_RF_READ_ONLY	0x02	/* Read-only cursor */
#define	MSG_RF_EOD		0x04	/* End-of-data */
#define	MSG_RF_CLOSED		0x08	/* Statement closed */
#define	MSG_RF_SCROLL		0x10	/* Scrollable cursor */

} DAM_ML_RESULT;

/*
** Info message.
*/

typedef struct
{
    DAM_ML_PM	params[ 1 ];	/* Variable length: implicit */

#define	MSG_IP_TRACE		0x01	/* Trace message */

} DAM_ML_INFO;


/*
** SQL Datatypes.  
**
** These definitions match the values in the JDBC/ODBC Types.
*/

#define	SQL_BOOLEAN		16
#define	SQL_BIT			(-7)
#define	SQL_TINYINT		(-6)
#define	SQL_SMALLINT		5
#define	SQL_INTEGER		4
#define	SQL_BIGINT		(-5)
#define	SQL_FLOAT		6
#define	SQL_REAL		7
#define	SQL_DOUBLE		8
#define	SQL_DECIMAL		3
#define	SQL_NUMERIC		2
#define	SQL_CHAR		1
#define	SQL_VARCHAR		12
#define	SQL_LONGVARCHAR		(-1)
#define	SQL_BINARY		(-2)
#define	SQL_VARBINARY		(-3)
#define	SQL_LONGVARBINARY	(-4)
#define	SQL_DATE		91
#define	SQL_TIME		92
#define	SQL_TIMESTAMP		93
#define	SQL_INTERVAL		10
#define	SQL_NULL		0
#define	SQL_OTHER		1111

FUNC_EXTERN void	gcd_msg_process( GCD_RCB * );
FUNC_EXTERN void	gcd_msg_pause( GCD_CCB *, bool );
FUNC_EXTERN void	gcd_msg_xoff( GCD_CCB *, bool );
FUNC_EXTERN void	gcd_msg_flush( GCD_CCB * );

FUNC_EXTERN bool	gcd_msg_begin( GCD_CCB *, GCD_RCB **, u_i1, u_i2 );
FUNC_EXTERN void	gcd_msg_end( GCD_RCB **, bool );
FUNC_EXTERN void	gcd_msg_complete( GCD_RCB **, u_i1 );
FUNC_EXTERN bool	gcd_msg_split( GCD_RCB ** );

FUNC_EXTERN void	gcd_msg_connect( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_disconnect( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_xact( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_request( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_query( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_batch( GCD_CCB *, bool );
FUNC_EXTERN void	gcd_msg_desc( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_data( GCD_CCB * );
FUNC_EXTERN void	gcd_msg_cursor( GCD_CCB * );
FUNC_EXTERN u_i2	gcd_msg_version ( u_i2 );

FUNC_EXTERN bool	gcd_expand_qtxt( GCD_CCB *, u_i4, bool );

FUNC_EXTERN bool	gcd_put_i1( GCD_RCB **, u_i1 );
FUNC_EXTERN bool	gcd_put_i1p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_i2( GCD_RCB **, u_i2 );
FUNC_EXTERN bool	gcd_put_i2p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_i4( GCD_RCB **, u_i4 );
FUNC_EXTERN bool	gcd_put_i4p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_i8( GCD_RCB **, u_i8 );
FUNC_EXTERN bool	gcd_put_i8p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_f4p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_f8p( GCD_RCB **, u_i1 * );
FUNC_EXTERN bool	gcd_put_bytes( GCD_RCB **, u_i2, u_i1 * );
FUNC_EXTERN bool	gcd_put_ucs2( GCD_RCB **, u_i2, u_i1 * );

FUNC_EXTERN bool	gcd_get_i1( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_i2( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_i4( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_i8( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_f4( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_f8( GCD_CCB *, u_i1 * );
FUNC_EXTERN bool	gcd_get_bytes( GCD_CCB *, u_i2 , u_i1 * );
FUNC_EXTERN bool	gcd_get_ucs2( GCD_CCB *, u_i2 , u_i1 * );
FUNC_EXTERN bool	gcd_skip( GCD_CCB *, u_i2 );

#endif /* _GCD_MSG_INCLUDED_ */
