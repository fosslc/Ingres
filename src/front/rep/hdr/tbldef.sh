/*
** Copyright (c) 1996, 2009 Ingres Corporation
*/
# ifndef TBLDEF_H_INCLUDED
# define TBLDEF_H_INCLUDED

# include <compat.h>
# include <gl.h>
# include <iicommon.h>

/**
** Name:	tbldef.sh - Table definition
**
** History:
**	16-dec-96 (joea)
**		Created based on tbldef.sh in replicator library.
**	18-sep-97 (joea)
**		Add page_size to table definition.
**	23-jul-98 (abbjo03)
**		Replace DLMNAME_LENGTH by formula using DB_MAXNAME.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-apr-2009 (joea)
**          Add column_ident to COLDEF to support identity columns.
**/

/* for COLDEF column_ident */
#define COL_IDENT_ALWAYS           'A'
#define COL_IDENT_BYDEFAULT        'D'
#define COL_IDENT_NONE             ' '

EXEC SQL BEGIN DECLARE SECTION;

typedef struct _coldef
{
	char	column_name[DB_MAXNAME+1];
	char	dlm_column_name[DB_MAXNAME*2+3];
	char	column_datatype[DB_MAXNAME+1];
	i4	column_length;
	i4	column_scale;
	i4	column_sequence;
	char	column_nulls[2];
	char	column_defaults[2];
	char    column_ident;
	i4	key_sequence;
} COLDEF;


typedef struct _tbldef
{
	char	table_name[DB_MAXNAME+1];
	char	dlm_table_name[DB_MAXNAME*2+3];
	char	table_owner[DB_MAXNAME+1];
	char	dlm_table_owner[DB_MAXNAME*2+3];
	i4	table_no;
	i4	page_size;
	i2	cdds_no;
	i2	target_type;
	i4	ncols;		/* Number of registered columns */
	char	columns_registered[26];
	char	supp_objs_created[26];
	char	rules_created[26];
	char	cdds_lookup_table[DB_MAXNAME+1];
	char	prio_lookup_table[DB_MAXNAME+1];
	char	index_used[DB_MAXNAME+1];
	char	sha_name[DB_MAXNAME+1];
	char	arc_name[DB_MAXNAME+1];
	char	rem_ins_proc[DB_MAXNAME+1];
	char	rem_upd_proc[DB_MAXNAME+1];
	char	rem_del_proc[DB_MAXNAME+1];
	COLDEF	*col_p;
} TBLDEF;

EXEC SQL END DECLARE SECTION;


FUNC_EXTERN STATUS RMtbl_fetch(i4 table_no, bool force);

# endif
