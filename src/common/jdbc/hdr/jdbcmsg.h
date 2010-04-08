/*
** Copyright (c) 2004 Ingres Corporation
*/

/* Name: jdbcmsg.h
**
** Description:
**	Definitions for the TL and MSG layer protocols.
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
*/

#ifndef _JDBC_MSG_INCLUDED_
#define _JDBC_MSG_INCLUDED_

#include <cvgcc.h>

/*
** The following values are copied from GCC header files.
*/

#define	SZ_NL_PCI		16	/* From gccpci.h */
#define TPDU_8K_SZ		0x0D	/* From gccpci.h */


/*
** Transport Layer message structures and definitions
*/

typedef struct	    /* TL message header */
{

    u_i4	tl_id;		/* Header ID */

#define	JDBC_TL_ID	0x4C54434A	/* Header ID: ascii "JCTL" */

    u_i2	msg_id;		/* Message ID */

#define	JDBC_TL_CR	0x5243		/* Connect request: ascii "CR" */
#define	JDBC_TL_CC	0x4343		/* Connect confirm: ascii "CC" */
#define	JDBC_TL_DT	0x5444		/* Data: ascii "DT" */
#define	JDBC_TL_DR	0x5244		/* Disconnect request: ascii "DR" */
#define	JDBC_TL_DC	0x4344		/* Disconnect confirm: ascii "DC" */
#define JDBC_TL_INT	0x5249		/* Interrupt: ascii "IR" */

} JDBC_TL_HDR;

#define	JDBC_TL_HDR_SZ		(CV_N_I4_SZ + CV_N_I2_SZ)

typedef struct		/* TL parameters */
{

    u_i1	param_id;	/* Parameter ID */
    u_i1	pl1;		/* Parameter length or 255 */
    u_i2	pl2;		/* Optional: present if param_len1 == 255 */
    u_i1	param[1];	/* Variable length: explicit */

} JDBC_TL_PARAM;

typedef struct	    /* TL CR/CC message */
{

    JDBC_TL_PARAM	params;

#define	JDBC_TL_CP_PROTO	0x01	/* Protocol level */

#define	JDBC_TL_PROTO_1		0x01
#define	JDBC_TL_DFLT_PROTO	JDBC_TL_PROTO_1

#define	JDBC_TL_CP_SIZE		0x02	/* Buffer size */

/*
** Our TL protocol should support buffer sizes upto
** 32K.  There is an understood limitation between
** GCC and the GCC CL of 8K.
*/
#define	JDBC_TL_PKT_MAX		0x0F		/* 32K packet size */
#define	JDBC_TL_PKT_DEF		TPDU_8K_SZ	/* GCC CL limitation */
#define	JDBC_TL_PKT_MIN		0x0A		/* 1K packet size */

#define	JDBC_TL_CP_MSG		0x03	/* MSG layer info */
#define	JDBC_TL_CP_CHRSET	0x04	/* Character set */

} JDBC_TL_CONN;

typedef struct	    /* TL DR/DC message */
{

    JDBC_TL_PARAM	params;		/* Optional */

#define	JDBC_TL_DP_ERR		0x01	/* Error code */

} JDBC_TL_DISC;

/*
** JDBC Message structures and definitions.
*/

#define	JDBC_MSG_P_PROTO	0x01	/* Parameter: protocol */
#define	JDBC_MSG_P_DTMC		0x02	/* Dist Trans Mgmt Connection */

#define	JDBC_MSG_PROTO_1	0x01	/* Protocol Levels */
#define	JDBC_MSG_PROTO_2	0x02
#define	JDBC_MSG_DFLT_PROTO	JDBC_MSG_PROTO_2

typedef struct	    /* MSG header */
{

    u_i4    hdr_id;

#define	JDBC_MSG_HDR_ID		0x4342444A	/* Header ID: 'JDBC' */	

    u_i2    msg_len;
    u_i1    msg_id;

#define	JDBC_MSG_UNUSED		0x00
#define	JDBC_MSG_CONNECT	0x01
#define	JDBC_MSG_DISCONN	0x02	
#define	JDBC_MSG_XACT		0x03
#define	JDBC_MSG_QUERY		0x04
#define	JDBC_MSG_CURSOR		0x05
#define	JDBC_MSG_DESC		0x06
#define	JDBC_MSG_DATA		0x07
#define	JDBC_MSG_ERROR		0x08
#define	JDBC_MSG_DONE		0x09
#define	JDBC_MSG_REQUEST	0x0A

#define	JDBC_MSG_CNT		11

    u_i1    msg_flags;

#define	JDBC_MSGFLG_EOD		0x01
#define	JDBC_MSGFLG_EOG		0x02

} JDBC_MSG_HDR;

#define	JDBC_MSG_HDR_LEN	(CV_N_I4_SZ + CV_N_I2_SZ + (CV_N_I1_SZ * 2))

/*
** Message parameters.
*/

typedef struct
{

    u_i2    param_id;
    u_i2    val_len;
    u_i1    value[ 1 ];		/* Variable length: explicit */

} JDBC_MSG_PM;

/*
** Connection message.
*/
typedef struct
{

    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_CP_DATABASE	0x01
#define	JDBC_CP_USERNAME	0x02
#define	JDBC_CP_PASSWORD	0x03
#define	JDBC_CP_DBUSERNAME	0x04
#define	JDBC_CP_GROUP_ID	0x05
#define	JDBC_CP_ROLE_ID		0x06
#define	JDBC_CP_DBPASSWORD	0x07
#define JDBC_CP_TIMEOUT		0x08
#define	JDBC_CP_CONNECT_POOL	0x09
#define	JDBC_CP_AUTO_MODE	0x0A
#define	JDBC_CP_II_XID		0x0B
#define	JDBC_CP_XA_FRMT		0x0C
#define	JDBC_CP_XA_GTID		0x0D
#define	JDBC_CP_XA_BQUAL	0x0E

#define	JDBC_CP_MAX		14	/* Number of parameters */

#define	JDBC_CPV_POOL_OFF	0
#define	JDBC_CPV_POOL_ON	1
#define	JDBC_CPV_XACM_DBMS	0
#define	JDBC_CPV_XACM_SINGLE	1
#define	JDBC_CPV_XACM_MULTI	2

} JDBC_MSG_CONN;

/*
** Transaction message.
*/

typedef struct
{

    u_i2	xact_op;

#define	JDBC_XACT_COMMIT	0x01
#define	JDBC_XACT_ROLLBACK	0x02
#define	JDBC_XACT_AC_ENABLE	0x03
#define	JDBC_XACT_AC_DISABLE	0x04
#define	JDBC_XACT_BEGIN		0x05
#define	JDBC_XACT_PREPARE	0x06

    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_XP_II_XID		0x01
#define	JDBC_XP_XA_FRMT		0x02
#define	JDBC_XP_XA_GTID		0x03
#define	JDBC_XP_XA_BQUAL	0x04

} JDBC_MSG_TRAN;

/*
** Request message.
*/

typedef struct
{
    u_i2	req_id;

#define	JDBC_REQ_CAPABILITY	0x01	/* DBMS capabilities */
#define	JDBC_REQ_PARAM_NAMES	0x02	/* Parameter names */
#define	JDBC_REQ_DBMSINFO	0x03	/* dbmsinfo() */
#define	JDBC_REQ_2PC_XIDS	0x04	/* Two-phase Commit Transaction IDs */

#define	JDBC_REQ_MAX		0x04

    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_RQP_SCHEMA_NAME	0x01	/* Schema name */
#define	JDBC_RQP_PROC_NAME	0x02	/* Procedure name */
#define JDBC_RQP_INFO_ID	0x03	/* dbmsinfo( INFO_ID ) */
#define	JDBC_RQP_DB_NAME	0x04	/* Database name */

} JDBC_MSG_RQST;

/*
** Query message.
*/

typedef struct
{
    u_i2	query_type;

#define	JDBC_QRY_EXQ		0x01	/* Execute a query */
#define	JDBC_QRY_OCQ		0x02	/* Open cursor for query */
#define	JDBC_QRY_PREP		0x03	/* Prepare a statement */
#define	JDBC_QRY_EXS		0x04	/* Execute a statement */
#define	JDBC_QRY_OCS		0x05	/* Open cursor for a statement */
#define	JDBC_QRY_EXP		0x06	/* Execute a procedure */
#define	JDBC_QRY_CDEL		0x07	/* Cursor delete */
#define	JDBC_QRY_CUPD		0x08	/* Cursor update */

#define	JDBC_QRY_MAX		0x08

    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_QP_QTXT		0x01	/* Query text */
#define	JDBC_QP_CRSR_NAME	0x02	/* Cursor name */
#define	JDBC_QP_STMT_NAME	0x03	/* Statement name */
#define	JDBC_QP_SCHEMA_NAME	0x04	/* Procedure schema */
#define	JDBC_QP_PROC_NAME	0x05	/* Procedure name */
#define	JDBC_QP_FLAGS		0x06	/* Query flags */

#define	JDBC_QF_READONLY	0x0001	/* READONLY cursor */

} JDBC_MSG_QRY;

/*
** Cursor message.
*/

typedef struct
{
    u_i2	cursor_op;

#define	JDBC_CUR_CLOSE		0x01
#define	JDBC_CUR_FETCH		0x02

    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_CUR_STMT_ID	0x01
#define	JDBC_CUR_PRE_FETCH	0x02

} JDBC_MSG_CUR;

/*
** Data descriptors
*/

typedef struct
{
    i2		jdbc_type;
    i2		dbms_type;
    u_i2	length;
    u_i1	precision;
    u_i1	scale;
    u_i1	flags;

#define		JDBC_DSC_NULL		0x01	/* Nullable */
#define		JDBC_DSC_PIN 		0x02	/* Procedure param: input */
#define		JDBC_DSC_POUT		0x04	/* Procedure param: output */
#define		JDBC_DSC_GTT		0x08	/* Proc param: global tmp tbl */

    u_i2	name_len;
    char	name[ 1 ];	/* Variable length: explicit */

} JDBC_MSG_DS;

/*
** Descriptor message.
*/

typedef struct
{
    u_i2	count;
    JDBC_MSG_DS	desc[ 1 ];	/* Variable length: explicit */

} JDBC_MSG_DSC;

/*
** Error message.
*/

typedef struct
{

    u_i4	err_code;
    char	sqlState[ 5 ];
    u_i1	msg_type;

#define	JDBC_ET_MSG		0x00	/* User message */
#define	JDBC_ET_WRN		0x01	/* Warning message */
#define	JDBC_ET_ERR		0x02	/* Error message */

    u_i2	msg_len;
    char	message[ 1 ];		/* Variable length: explicit */

} JDBC_MSG_ERR;

/*
** Result (done) message.
*/

typedef struct
{
    JDBC_MSG_PM	params[ 1 ];	/* Variable length: implicit */

#define	JDBC_RP_ROWCOUNT	0x01	/* Row count */
#define	JDBC_RP_XACT_END	0x02	/* Transaction completed */
#define	JDBC_RP_STMT_ID		0x03	/* Statement ID */
#define JDBC_RP_FETCH_LIMIT	0x04	/* Fetch row limit */
#define	JDBC_RP_EOD		0x05	/* End-of-data */
#define	JDBC_RP_PROC_RESULT	0x06	/* Procedure return value */
#define	JDBC_RP_READ_ONLY	0x07	/* Read-only cursor */

} JDBC_MSG_RESULT;

/*
** SQL Datatypes.  
**
** These definitions match the values in the
** JDBC class Types.
*/

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
#define	SQL_NULL		0
#define	SQL_OTHER		1111

FUNC_EXTERN bool	jdbc_msg_notify( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_pause( JDBC_CCB *, bool );
FUNC_EXTERN void	jdbc_msg_xoff( JDBC_CCB *, bool );
FUNC_EXTERN void	jdbc_flush_msg( JDBC_CCB * );

FUNC_EXTERN void	jdbc_msg_connect( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_disconnect( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_xact( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_request( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_query( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_desc( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_data( JDBC_CCB * );
FUNC_EXTERN void	jdbc_msg_cursor( JDBC_CCB * );

FUNC_EXTERN bool	jdbc_expand_qtxt( JDBC_CCB *, u_i2, bool );

FUNC_EXTERN bool	jdbc_msg_begin( JDBC_RCB **, u_i1, u_i2 );
FUNC_EXTERN void	jdbc_msg_end( JDBC_RCB *, bool );
FUNC_EXTERN bool	jdbc_msg_split( JDBC_RCB ** );

FUNC_EXTERN bool	jdbc_put_i1( JDBC_RCB **, u_i1 );
FUNC_EXTERN bool	jdbc_put_i1p( JDBC_RCB **, u_i1 * );
FUNC_EXTERN bool	jdbc_put_i2( JDBC_RCB **, u_i2 );
FUNC_EXTERN bool	jdbc_put_i2p( JDBC_RCB **, u_i1 * );
FUNC_EXTERN bool	jdbc_put_i4( JDBC_RCB **, u_i4 );
FUNC_EXTERN bool	jdbc_put_i4p( JDBC_RCB **, u_i1 * );
FUNC_EXTERN bool	jdbc_put_f4p( JDBC_RCB **, u_i1 * );
FUNC_EXTERN bool	jdbc_put_f8p( JDBC_RCB **, u_i1 * );
FUNC_EXTERN bool	jdbc_put_bytes( JDBC_RCB **, u_i2, u_i1 * );
FUNC_EXTERN bool	jdbc_put_ucs2( JDBC_RCB **, u_i2, u_i1 * );

FUNC_EXTERN bool	jdbc_get_i1( JDBC_CCB *, u_i1 * );
FUNC_EXTERN bool	jdbc_get_i2( JDBC_CCB *, u_i1 * );
FUNC_EXTERN bool	jdbc_get_i4( JDBC_CCB *, u_i1 * );
FUNC_EXTERN bool	jdbc_get_f4( JDBC_CCB *, u_i1 * );
FUNC_EXTERN bool	jdbc_get_f8( JDBC_CCB *, u_i1 * );
FUNC_EXTERN bool	jdbc_get_bytes( JDBC_CCB *, u_i2 , u_i1 * );
FUNC_EXTERN bool	jdbc_get_ucs2( JDBC_CCB *, u_i2 , u_i1 * );
FUNC_EXTERN bool	jdbc_skip( JDBC_CCB *, u_i2 );

#endif /* _JDBC_MSG_INCLUDED_ */
