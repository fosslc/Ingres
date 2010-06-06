/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef REPSERV_H_INCLUDED
# define REPSERV_H_INCLUDED
# include <compat.h>
# include <lo.h>
# include <si.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include "conn.h"
# include "tblinfo.h"
# include "distq.h"

/**
** Name:	repserv.h - Replicator server include
**
** Description:
**	Include file for miscellaneous defines, globals and functions that
**	deal mostly with reading the distribution queue, transmitting the
**	transactions, and setting the server parameters.
**
** History:
**	16-dec-96 (joea)
**		Created based on flags.sh in replicator library.
**	03-feb-97 (joea)
**		Add RS_TBLDESC and DQ_ROW arguments to d_insert, d_update,
**		d_delete and RScommit.
**	26-mar-97 (joea)
**		Include distq.h before records.h. Temporarily use TBL_ENTRY
**		instead of RS_TBLDESC in d_insert, d_update and d_delete.
**		Eliminate trim_node.
**	22-aug-97 (joea)
**		Rename structures for consistency.
**	09-dec-97 (joea)
**		Rename read_q flags to avoid name problems on MVS. Add
**		RS_ROW_BUF_LEN_DFLT. Replace d_insert/update/delete by RSinsert/
**		update/delete. Add prototypes for server utility routines.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	27-may-98 (joea)
**		Change arguments to RSkeys_check_unique (and rename it). Add
**		prototype for RScreate_key_clauses.
**	09-jul-98 (abbjo03)
**		Add prototypes for RSpdb_PrepDescBuffers, RSfdb_FreeDescBuffers
**		and RSiiq_InsertInputQueue. Add RS_BLOB for blob buffers and
**		blob callback prototypes. Remove sha_name argument to
**		RScreate_key_clauses.
**	22-jul-98 (abbjo03)
**		Allow for delimited identifiers in paramNames parameter to
**		RSpdp_PrepDbprocParams.
**	24-feb-98 (abbjo03)
**		Change paramCount parameter to II_INT2.
**	23-aug-99 (abbjo03)
**		Remove QEP flag which is no longer used.
**	31-aug-99 (abbjo03) bug 98604
**		Add RSdtrans_id_reregister.
**	27-sep-99 (abbjo03) bug 98604
**		Replace RSdtrans_id_reregister by RSdtrans_id_get_last.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-Oct-2001 (inifa01) bug 105555 INGREP 97.
**		Defined new function pop_list_bottom() defined in list.c
**	06-Nov-2003 (inifa01) INGREP 108 bug 106498
**		Added RSmem_allocate() and RSmem_free().
**      22-Aug-2005 (hweho01)
**              Increased the default row buffer size to be DB_MAXSTRING+8192. 
**      24-May-2010 (stial01)
**              Added buf5 param to RSmem_free()
**/

# define READ_Q_MUST_BREAK_DFLT		5000
# define READ_Q_TRY_BREAK_DFLT		4000

# define RS_ROW_BUF_LEN_DFLT		DB_MAXSTRING + 8192  


/* LONG VARCHAR / LONG BYTE buffer */

typedef struct tagRS_BLOB
{
	short		len;			/* varchar length indicator */
	char		buf[DB_MAXTUP-2];	/* segment buffer */
	i4		size;			/* overall blob length */
	char		filename[MAX_LOC+1];	/* temp file path and name */
	FILE		*fp;			/* temp file pointer */
} RS_BLOB;


/* break after reading this many from the queue */

GLOBALREF i4	RSqbm_read_must_break;

/* break after a transaction change after reading this many from the queue */

GLOBALREF i4	RSqbt_read_try_to_break;


FUNC_EXTERN STATUS read_q(i2 server_no);
FUNC_EXTERN STATUS RStransmit(void);
FUNC_EXTERN STATUS RScommit(RS_TARGET *target, RS_TRANS_ROW *row);
FUNC_EXTERN STATUS RSdtrans_id_register(RS_TARGET *target);
FUNC_EXTERN II_PTR RSdtrans_id_get_last(void);
FUNC_EXTERN STATUS RSerror_handle(RS_TARGET *target, i4 dberror);

FUNC_EXTERN STATUS RSinsert(RS_TARGET *target, RS_TRANS_ROW *row);
FUNC_EXTERN STATUS RSupdate(RS_TARGET *target, RS_TRANS_ROW *row);
FUNC_EXTERN STATUS RSdelete(RS_TARGET *target, RS_TRANS_ROW *row);
FUNC_EXTERN STATUS RScollision(RS_TARGET *target, RS_TRANS_ROW *row,
	RS_TBLDESC *tbl, bool *collision_processed);

FUNC_EXTERN void init_list(void);
FUNC_EXTERN RLIST *new_node(void);
FUNC_EXTERN RLIST *next_node(void);
FUNC_EXTERN RLIST *list_bottom(void);
FUNC_EXTERN RLIST *list_top(void);
FUNC_EXTERN RLIST *pop_list_bottom(void);

FUNC_EXTERN void file_flags(void);
FUNC_EXTERN void com_flags(i4 argc, char *argv[]);
FUNC_EXTERN void set_flags(char *flag);
FUNC_EXTERN bool check_flags(void);

FUNC_EXTERN STATUS RSkeys_check_unique(RS_CONN *conn);

FUNC_EXTERN II_VOID II_FAR II_CALLBACK RSblob_gcCallback(II_PTR closure,
	II_PTR parmBlock);
FUNC_EXTERN II_VOID II_FAR II_CALLBACK RSblob_ppCallback(II_PTR closure,
	II_PTR parmBlock);

FUNC_EXTERN STATUS RScreate_key_clauses(RS_CONN *conn, RS_TBLDESC *tbl,
	RS_TRANS_KEY *trans, char *where_clause, char *name_clause,
	char *insert_value_clause, char *update_clause);

FUNC_EXTERN STATUS RSfdb_FreeDescBuffers(RS_TRANS_ROW *row);

FUNC_EXTERN STATUS RSiiq_InsertInputQueue(RS_CONN *conn, RS_TBLDESC *tbl,
	i2 trans_type, RS_TRANS_KEY *rep_key, RS_TRANS_KEY *old_rep_key,
	i2 cdds_no, i2 priority, char *trans_time, bool check_collision);

FUNC_EXTERN STATUS RSist_InsertShadowTable(RS_CONN *conn, RS_TBLDESC *tbl,
	i2 trans_type, RS_TRANS_KEY *rep_key, RS_TRANS_KEY *old_rep_key,
	i2 cdds_no, i2 priority, i1 in_archive, i2 new_key, char *trans_time,
	char *distrib_time, char *col_list, char *val_list,
	IIAPI_DESCRIPTOR pdesc[], IIAPI_DATAVALUE pdata[]);

FUNC_EXTERN STATUS RSpdb_PrepDescBuffers(RS_TRANS_ROW *row);

FUNC_EXTERN STATUS RSpdp_PrepDbprocParams(char *proc_name, RS_TARGET *target,
	RS_TBLDESC *tbl, RS_TRANS_ROW *row, char (*paramName)[DB_MAX_DELIMID+1],
	II_INT2 *paramCount, IIAPI_DESCRIPTOR *paramDesc,
	IIAPI_DATAVALUE *paramValue);

FUNC_EXTERN char * RSmem_allocate(i4 row_width, i4 num_cols,i4 colname_space,i4 overhead);
FUNC_EXTERN void RSmem_free(char *buf1, char *buf2, char *buf3, char *buf4, char *buf5);
# endif
