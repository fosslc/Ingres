/*
** Copyright (c) 1997, 2009 Ingres Corporation
*/
# ifndef TBLINFO_H_INCLUDED
# define TBLINFO_H_INCLUDED
# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>

/**
** Name:	tblinfo.h - table information for RepServer
**
** Description:
**	Table information for RepServer.
**
** History:
**	20-jan-97 (joea)
**		Created based on tblinfo.sh in replicator library.
**	01-dec-97 (joea)
**		Add IIAPI_DESCRIPTOR to replace individual descriptive fields in
**		RS_COLDESC.  Eliminate IIAPI_DATAVALUE fields.  Use DB_MAXNAME
**		instead of literal 32.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	20-may-98 (joea)
**		Add dba_table member to RS_TBLDESC.
**	24-jun-98 (abbjo03)
**		Add has_blob, dlm_table_name, dlm_table_owner, shadow_table and
**		archive_table members to RS_TBLDESC. Add dlm_col_name member to
**		RS_COLDESC.
**	22-jul-98 (abbjo03)
**		Add rem_table_owner member to RS_TBLDESC. Add prototype for
**		RSaro_AdjRemoteOwner. Replace col_name by dlm_col_name.
**	13-aug-98 (abbjo03)
**		Add handles to RS_TBLDESC to support repeated queries in
**		IIRSgrd_GetRowData.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-oct-2002 (padni01) bug 106498
**		Added column row_width to RS_TBLDESC
**      22-apr-2009 (joea)
**          Add col_ident to support replication of identity columns.
**/

/*{
** Name:	RS_COLDESC - column description
**
** Description:
**	This structure describes a replicated column.
*/
typedef struct tagRS_COLDESC
{
	i4		col_sequence;
	char		col_name[DB_MAXNAME*2+3];	/* quoted colum name */
	i4		key_sequence;
        char            col_ident;       /* identity column generate type */
	IIAPI_DESCRIPTOR coldesc;
} RS_COLDESC;


/*{
** Name:	RS_TBLDESC - table description
**
** Description:
**	This structure describes a replicated table.
*/
typedef struct tagRS_TBLDESC
{
	i4		table_no;
	char		table_name[DB_MAXNAME+1];
	char		table_owner[DB_MAXNAME+1];
	bool		dba_table;
	bool		has_blob;
	short		num_regist_cols;
	short		num_key_cols;
	i2		cdds_no;
	char		cdds_lookup_table[DB_MAXNAME+1];
	char		prio_lookup_table[DB_MAXNAME+1];
	char		dlm_table_name[DB_MAXNAME*2+3];
	char		dlm_table_owner[DB_MAXNAME*2+3];
	char		rem_table_owner[DB_MAXNAME*2+3];
	char		shadow_table[DB_MAXNAME+1];
	char		archive_table[DB_MAXNAME+1];
	II_PTR		grd1_qryHandle;
	II_PTR		grd2_qryHandle;
	II_PTR		grd3_qryHandle;
	II_PTR		grd4_qryHandle;
	i4		num_inserts;
	i4		num_updates;
	i4		num_deletes;
	RS_COLDESC	*cols;
	i4		row_width;
} RS_TBLDESC;


FUNC_EXTERN void RSaro_AdjRemoteOwner(RS_TBLDESC *tbl, char *target_dba);

# endif
