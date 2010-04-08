/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef DISTQ_H_INCLUDED
# define DISTQ_H_INCLUDED
# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "tblinfo.h"

/**
** Name:	distq.h - Distribution Queue API
**
** Description:
**	Defines structures and prototypes for the DQ API.
**
** History:
**	03-feb-97 (joea)
**		Created based on distq.h in replicator library.
**	26-mar-97 (joea)
**		Move RLIST here from records.sh.  Nest DQ_ROW inside RLIST.
**	24-jun-97 (joea)
**		Add cdds_no parameter to IIRSrlt_ReleaseTrans.  Uncomment
**		prototypes.
**	05-sep-97 (joea)
**		Rework some of the return values.  Rename structures for
**		consistency.  Add RS_TARGET structure.  Change API calling
**		parameters and add IIRSabt_AbortTrans and IIRSclq_CloseQueue.
**	02-dec-97 (joea)
**		Add RS_INTERNAL_ERR status value. Add RS_TBLDESC pointer to
**		RS_TRANS_ROW. Replace undescribed row_data and key_data buffers
**		by arrays of IIAPI_DESCRIPTORs and IIAPI_DATAVALUEs. Add
**		RS_TARGET to RLIST structure. Update API prototypes to rev 2.
**	26-may-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	25-jun-98 (abbjo03)
**		Add TRANS_KEY_DESC_INIT macro.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* return values */

# define RS_NO_TRANS		1
# define RS_NO_ROWS		1
# define RS_REREAD		2
# define RS_DBMS_ERR		3	/* RepServer will shutdown */
# define RS_MEM_ERR		4	/* RepServer will shutdown */
# define RS_INTERNAL_ERR	5

/* transaction types */
# define RS_INSERT		1
# define RS_UPDATE		2
# define RS_DELETE		3


/* macro to initialize API descriptors for a transaction key */

# define TRANS_KEY_DESC_INIT(ds, dv, k) \
	SW_PARM_DESC_DATA_INIT(*(ds), *(dv), IIAPI_INT_TYPE, (k).src_db); \
	SW_PARM_DESC_DATA_INIT(*(ds + 1), *(dv + 1), IIAPI_INT_TYPE, \
		(k).trans_id); \
	SW_PARM_DESC_DATA_INIT(*(ds + 2), *(dv + 2), IIAPI_INT_TYPE, \
		(k).seq_no)


/* replicated transaction key (replication key) */

typedef struct tagTRANS_KEY
{
	i2	src_db;		/* source database number */
	i4	trans_id;	/* DBMS transaction ID */
	i4	seq_no;		/* sequence within transaction */
} RS_TRANS_KEY;


/* replicated row */

typedef struct tagTRANS_ROW
{
	RS_TRANS_KEY	rep_key;		/* source replication key */
	i4		table_no;
	i2		trans_type;
	char		trans_time[26];
	i2		priority;		/* priority for collision */
	RS_TRANS_KEY	old_rep_key;		/* previous replication key */
	char		old_trans_time[26];
	i2		old_cdds_no;
	i2		old_priority;
	i2		new_key;
	u_i4		data_len;
	u_i4		key_len;
	RS_TBLDESC		*tbl_desc;
	IIAPI_DESCRIPTOR	*row_desc;
	IIAPI_DATAVALUE		*row_data;
	IIAPI_DESCRIPTOR	*key_desc;
	IIAPI_DATAVALUE		*key_data;
} RS_TRANS_ROW;


/* replication target */

typedef struct tagTARGET
{
	i2	db_no;
	i2	cdds_no;
	i2	type;
	i4	conn_no;
	i2	collision_mode;
	i2	error_mode;
} RS_TARGET;


/* transaction record list */

typedef struct rlist
{
	RS_TRANS_ROW	row;
	RS_TARGET	target;
	struct rlist	*link;
	struct rlist	*blink;
} RLIST;


/* API prototypes */

FUNC_EXTERN STATUS IIRSgnt_GetNextTrans(i2 server_no, RS_TARGET *target,
	RS_TRANS_KEY *trans);
FUNC_EXTERN STATUS IIRSgnr_GetNextRowInfo(i2 server_no, RS_TARGET *target,
	RS_TRANS_ROW *trans_row);
FUNC_EXTERN STATUS IIRSgrd_GetRowData(i2 server_no, RS_TARGET *target,
	RS_TRANS_ROW *trans_row);
FUNC_EXTERN STATUS IIRSrlt_ReleaseTrans(i2 server_no, RS_TARGET *target,
	RS_TRANS_KEY *trans);
FUNC_EXTERN STATUS IIRSabt_AbortTrans(i2 server_no, RS_TARGET *target,
	RS_TRANS_KEY *trans);
FUNC_EXTERN STATUS IIRSclq_CloseQueue(i2 server_no);

# endif
