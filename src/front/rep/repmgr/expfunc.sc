/*
** Copyright (c) 1997, 2009 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cm.h>
# include <lo.h>
# include <si.h>
# include <me.h>
# include <nm.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <rpu.h>
# include <tblobjs.h>
# include <tbldef.h>
# include "errm.h"

/**
** Name:	expfunc.sc - expand tokens
**
** Description:
**	Defines
**		expand_ddl	- expand column data definition token
**		expand_ddk	- expand key column data definition token
**		expand_jon	- expand join token
**		expand_iocol	- expand table column tokens
**		expand_key	- expand key columns token
**		expand_cll	- expand column list assignment token
**		expand_tblobj	- expand table object token
**		expand_odt	- expand owner qualified table name token
**		expand_oiv	- expand overriding identity value token
**		expand_vps	- expand variable page size token
**		prefix_name	- prefix a name
**		column_alias	- generate alphanumeric column alias
**
** History:
**	09-jan-97 (joea)
**		Created based on expfunc.sc in replicator library.
**	05-feb-97 (joea)
**		Remove unused functions. Document token meanings.
**	17-mar-97 (joea) bug 77354
**		In expand_key, list the index columns in the same order as the
**		underlying index or storage structure.
**	10-apr-97 (joea) bug 77354
**		Apply the same change to expand_cky as above for expand_key.
**	23-apr-97 (joea) bugs 81736 81737
**		For DECIMAL datatypes, use a larger buffer in expand_cdl to
**		account for the decimal point and the possible negative sign.
**		Also, in expand_ddl add the decimal scale to the specification.
**	18-jun-97 (joea)
**		Define/undef ii_BLOB in expand_ccc to agree with 16-jan-97
**		change to qryfuncs.tlp.
**	23-jul-97 (joea) bug 81741
**		Add flag parameter to expand_key to deal with DECIMAL key
**		columns. In expand_ddk, add decimal scale to the specification.
**		In expand_iocol and expand_cjn, support DECIMAL key columns.
**		Consolidate table object name expansion into expand_tblobj.
**		Eliminate unused options in expand_tbl and expand_own.
**	17-sep-97 (joea) bug 85560
**		In expand_ccc and expand_fbb only expand text if the blob
**		column is being replicated.
**	18-sep-97 (joea)
**		Add expand_vps to expand variable page size token.  Remove
**		ddba_messageit(5, ...) calls since they are no-ops.
**	04-dec-97 (joea) bug 85560
**		Add parentheses in expand_ccc and expand_fbb to apply the
**		17-sep-97 fix to both LONG VARCHAR and LONG BYTE.
**	23-jul-98 (abbjo03)
**		Remove processing of tokens made obsolete by generic RepServer.
**	25-sep-98 (abbjo03)
**		Replace ddba_messageit with IIUGerr. Add cases for object_key
**		and table_key in expand_ddl and expand_ddk.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Oct-2006 (hweho01)
**          Added ingresdate to the list for data type processing.
**      05-nov-2009 (joea)
**          Add tests for "boolean" in expand_ddl and expand_ddk.
**	29-Sep-2010 (drivi01)
**	    Remove unused defines for hardcoded buffers.
**	    Remove hardcoded value for DLNM_SIZE and replace it 
**	    with DB_MAX_DELIMID+1.
**/

# define MAX_INDEX_COLS		31
# define DLNM_SIZE		DB_MAX_DELIMID+1
# define COL_ALIAS_PREFIX	ERx("col")
# define COL_ALIAS_NDIGITS	3
# define BLANK			' '


GLOBALREF TBLDEF *RMtbl;


static char *prefix_name(char *prefix, char *name, char *prefixed_name);
static char *column_alias(i4 column_num, char *alias);

static char	indent_string[100];
static char	Prefixed_Name_Buf[DLNM_SIZE];
static char	Column_Alias_Buf[DLNM_SIZE];


/*{
** Name:	expand_ddl - expand column data definition token
**
** Description:
**	expand the $ddl token
**
** Inputs:
**	table_no	- table number
**	p1		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_ddl(
i4	table_no,
char	*p1,
i4	indent,
char	*expand_string)
{
	COLDEF	*col_p;
	char	tmp[30];
	i4	column_number;

	(void)RMtbl_fetch(table_no, FALSE);

	*expand_string = EOS;
	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		if (col_p->column_sequence)	/* All registered columns */
		{
			if (*expand_string != EOS)
			{
				STcat(expand_string, ERx(",\n"));
				STprintf(indent_string, ERx("%*c"), indent,
					BLANK);
				STcat(expand_string, indent_string);
			}
			STcat(expand_string, prefix_name(p1,
				col_p->column_name, NULL));
			STcat(expand_string, ERx(" "));
			STcat(expand_string, col_p->column_datatype);
			if (STequal(col_p->column_datatype, ERx("date")) ||
			    STequal(col_p->column_datatype, ERx("ansidate")) ||
			    STequal(col_p->column_datatype, ERx("ingresdate")) ||
			    STequal(col_p->column_datatype,
						ERx("time without time zone")) ||
			    STequal(col_p->column_datatype,
						ERx("time with time zone")) ||
			    STequal(col_p->column_datatype,
						ERx("time with local time zone")) ||
			    STequal(col_p->column_datatype,
						ERx("timestamp without time zone")) ||
			    STequal(col_p->column_datatype,
						ERx("timestamp with time zone")) ||
			    STequal(col_p->column_datatype,
					    ERx("timestamp with local time zone")) ||
			    STequal(col_p->column_datatype,
					    ERx("interval year to month")) ||
			    STequal(col_p->column_datatype,
					    ERx("interval day to second")) ||
			    STequal(col_p->column_datatype, ERx("money")) ||
                            STequal(col_p->column_datatype, "boolean") ||
			    STequal(col_p->column_datatype,
						ERx("object_key")) ||
			    STequal(col_p->column_datatype,
						ERx("table_key")))
				STcopy(ERx(" "), tmp);
			else if (STequal(col_p->column_datatype, ERx("integer")) ||
				STequal(col_p->column_datatype, ERx("float")))
				STprintf(tmp, ERx("%d "), col_p->column_length);
			else if (STequal(col_p->column_datatype, ERx("decimal")))
				STprintf(tmp, ERx("(%d, %d) "),
					col_p->column_length,
					col_p->column_scale);
			else
				STprintf(tmp, ERx("(%d) "), col_p->column_length);
			STcat(expand_string, tmp);
			if (CMcmpcase(col_p->column_nulls, ERx("Y")) == 0)
			{
				STcat(expand_string, ERx("WITH NULL"));
			}
			else
			{
				if (CMcmpcase(col_p->column_defaults, ERx("Y")) == 0)
					STcat(expand_string,
						ERx("NOT NULL WITH DEFAULT"));
				else
					STcat(expand_string,
						ERx("NOT NULL NOT DEFAULT"));
			}
		}
	}
}


/*{
** Name:	expand_ddk - expand key column data definition token
**
** Description:
**	expand the $ddk token
**
** Inputs:
**	table_no	- table number
**	p1		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
** History:
**	05-oct-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/
void
expand_ddk(
i4	table_no,
char	*p1,
i4	indent,
char	*expand_string)
{
	COLDEF	*col_p;
	char	tmp[30];
	i4	column_number;

	(void)RMtbl_fetch(table_no, FALSE);

	*expand_string = EOS;
	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		if (col_p->key_sequence > 0)
		{
			if (*expand_string != EOS)
			{
				STcat(expand_string, ERx(",\n"));
				STprintf(indent_string, ERx("%*c"), indent, BLANK);
				STcat(expand_string, indent_string);
			}
			STcat(expand_string, prefix_name(p1,
				col_p->column_name, NULL));
			STcat(expand_string, ERx(" "));
			STcat(expand_string, col_p->column_datatype);
			if (STequal(col_p->column_datatype, ERx("date")) ||
				STequal(col_p->column_datatype, ERx("money")) ||
				STequal(col_p->column_datatype,
						ERx("object_key")) ||
				STequal(col_p->column_datatype,
						ERx("ingresdate")) ||
				STequal(col_p->column_datatype,
						ERx("ansidate")) ||
				STequal(col_p->column_datatype,
						ERx("time without time zone")) ||
				STequal(col_p->column_datatype,
						ERx("time with time zone")) ||
				STequal(col_p->column_datatype,
						ERx("time with local time zone")) ||
				STequal(col_p->column_datatype,
						ERx("timestamp without time zone")) ||
				STequal(col_p->column_datatype,
						ERx("timestamp with time zone")) ||
				STequal(col_p->column_datatype,
					    ERx("timestamp with local time zone")) ||
				STequal(col_p->column_datatype,
					    ERx("interval year to month")) ||
				STequal(col_p->column_datatype,
					    ERx("interval day to second")) ||
                                STequal(col_p->column_datatype, "boolean") ||
				STequal(col_p->column_datatype,
						ERx("table_key")))
				STcopy(ERx(" "), tmp);
			else if (STequal(col_p->column_datatype, ERx("integer")) ||
				STequal(col_p->column_datatype, ERx("float")))
				STprintf(tmp, ERx("%d "), col_p->column_length);
			else if (STequal(col_p->column_datatype, ERx("decimal")))
				STprintf(tmp, ERx("(%d, %d) "),
					col_p->column_length,
					col_p->column_scale);
			else
				STprintf(tmp, ERx("(%d) "), col_p->column_length);
			STcat(expand_string, tmp);
			if (CMcmpcase(col_p->column_nulls, ERx("Y")) == 0)
			{
				STcat(expand_string, ERx("WITH NULL"));
			}
			else
			{
				if (CMcmpcase(col_p->column_defaults, ERx("Y")) == 0)
					STcat(expand_string,
						ERx("NOT NULL WITH DEFAULT"));
				else
					STcat(expand_string,
						ERx("NOT NULL NOT DEFAULT"));
			}
		}
	}
}


/*{
** Name:	expand_jon - expand join token
**
** Description:
**	expand the $jon token
**
** Inputs:
**	table_no	- table number
**	keys_only	-
**	p1		-
**	p2		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_jon(
i4	table_no,
bool	keys_only,
char	*p1,
char	*p2,
i4	indent,
char	*expand_string)
{
	COLDEF	*col_p;
	i4	column_number = 0;
	bool	first_column = TRUE;

	(void)RMtbl_fetch(table_no, FALSE);

	*expand_string = EOS;
	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		if (!keys_only || col_p->key_sequence > 0)	/* key field */
		{
			if (first_column)
			{
				first_column = FALSE;
			}
			else
			{
				STcat(expand_string, ERx(" AND\n"));
				STprintf(indent_string, ERx("%*c"), indent,
					BLANK);
				STcat(expand_string, indent_string);
			}
			STcat(expand_string, prefix_name(p1,
				col_p->column_name, NULL));
			STcat(expand_string, ERx(" = "));
			STcat(expand_string, prefix_name(p2,
				col_p->column_name, NULL));
		}
	}
}


/*{
** Name:	expand_iocol - expand table column tokens
**
** Description:
**	Expand $col token
**
** Inputs:
**	table_no	- table number
**	p1		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_iocol(
i4	table_no,
char	*p1,
i4	indent,
char	*expand_string)
{
	bool	first_column = TRUE;
	i4	column_number;
	COLDEF	*col_p;

	(void)RMtbl_fetch(table_no, FALSE);

	*expand_string = EOS;
	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		if (col_p->key_sequence > 0)	/* Get key columns */
		{
			if (first_column)
			{
				first_column = FALSE;
			}
			else
			{
				STcat(expand_string, ERx(",\n"));
				STprintf(indent_string, ERx("%*c"), indent,
					BLANK);
				STcat(expand_string, indent_string);
			}
			STcat(expand_string, prefix_name(p1, col_p->column_name,
				NULL));
		}
	}

	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		/* nonkey columns */
		if (col_p->column_sequence && col_p->key_sequence == 0)
		{
			STcat(expand_string, ERx(",\n"));
			STprintf(indent_string, ERx("%*c"), indent, BLANK);
			STcat(expand_string, indent_string);
			STcat(expand_string, prefix_name(p1, col_p->column_name,
				NULL));
		}
	}
}


/*{
** Name:	expand_key - expand key columns token
**
** Description:
**	Expand the $key token.
**
** Inputs:
**	table_no	- surrogate id for registered table
**	p1		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_key(
i4	table_no,
char	*p1,
i4	indent,
char	*expand_string)
{
	i4	i;
	COLDEF	*col_p;
	i4	index_cols[MAX_INDEX_COLS];

	(void)RMtbl_fetch(table_no, FALSE);
	
	for (i = 0; i < MAX_INDEX_COLS; ++i)
		index_cols[i] = -1;

	for (i = 0, col_p = RMtbl->col_p; i < RMtbl->ncols; ++i, *col_p++)
		if (col_p->key_sequence > 0 &&
				col_p->key_sequence <= MAX_INDEX_COLS)
			index_cols[col_p->key_sequence - 1] = i;

	*expand_string = EOS;
	for (i = 0; index_cols[i] != -1 && i < MAX_INDEX_COLS; ++i)
	{
		if (i > 0)
		{
			STcat(expand_string, ERx(",\n"));
			STprintf(indent_string, ERx("%*c"), indent, BLANK);
			STcat(expand_string, indent_string);
		}
		col_p = &RMtbl->col_p[index_cols[i]];
		STcat(expand_string, prefix_name(p1, col_p->column_name, NULL));
	}
}


/*{
** Name:	expand_cll - expand column list assignment token
**
** Description:
**	expand the $cll token
**
** Inputs:
**	table_no	- surrogate key for registered table
**	p1		-
**	p2		-
**	indent		- number of spaces to indent
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_cll(
i4	table_no,
char	*p1,
char	*p2,
i4	indent,
char	*expand_string)
{
	COLDEF	*col_p;
	i4	column_number = 0;
	bool	first_column = TRUE;

	(void)RMtbl_fetch(table_no, FALSE);

	*expand_string = EOS;
	for (column_number = 0, col_p = RMtbl->col_p;
		column_number < RMtbl->ncols; column_number++, *col_p++)
	{
		if (col_p->column_sequence)	/* denotes replicated column */
		{
			if (first_column)
			{
				first_column = FALSE;
			}
			else
			{
				STcat(expand_string, ERx(",\n"));
				STprintf(indent_string, ERx("%*c"), indent,
					BLANK);
				STcat(expand_string, indent_string);
			}
			STcat(expand_string, prefix_name(p1,
				col_p->column_name, NULL));
			STcat(expand_string, ERx(" = "));
			STcat(expand_string, prefix_name(p2,
				col_p->column_name, NULL));
		}
	}
}


/*{
** Name:	expand_tblobj - expand the table object tokens
**
** Description:
**	Expand the table object tokens: $sha, $sx1, $arc, $rip, $rup and $rdp.
**
** Inputs:
**	table_no	- table number
**	obj_type	- table object type
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_tblobj(
i4	table_no,
i4	obj_type,
char	*expand_string)
{
	char	tblobj_name[DB_MAXNAME+1];

	(void)RMtbl_fetch(table_no, FALSE);
	*tblobj_name = EOS;

	if (RPtblobj_name(RMtbl->table_name, RMtbl->table_no, obj_type,
			tblobj_name))
		IIUGerr(E_RM00EB_Err_gen_obj_name, UG_ERR_ERROR, 2,
			RMtbl->table_owner, RMtbl->table_name);

	STcopy(tblobj_name, expand_string);
}


/*{
** Name:	expand_odt - expand owner qualified table name token
**
** Description:
**	Expand the $odt token.  If the current server supports schemas,
**	$odt expands to "table_owner.table_name", delimited.
**
** Inputs:
**	table_name	- table name
**	table_owner	- table owner
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_odt(
char	*table_name,
char	*table_owner,
char	*expand_string)
{
	STcopy(RPedit_name(EDNM_DELIMIT, table_owner, NULL), expand_string);
	STcat(expand_string, ERx("."));
	STcat(expand_string, RPedit_name(EDNM_DELIMIT, table_name, NULL));
}


/*{
** Name:	expand_oiv - expand overriding identity value token
**
** Description:
**	Expand the $oiv token.  If the table has an identity column
**	use OVERRIDING SYSTEM/USER VALUE, depending on the GENERATED
**	(ALWAYS/BY DEFAULT) clause used in the identiy column.
**
** Inputs:
**	table_no	- table number
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_oiv(
i4	table_no,
char	*expand_string)
{
    COLDEF *col_p;
    i4 i;

    RMtbl_fetch(table_no, FALSE);

    *expand_string = EOS;
    for (i = 0, col_p = RMtbl->col_p; i < RMtbl->ncols; ++i, ++col_p)
    {
	if (col_p->column_ident == COL_IDENT_ALWAYS)
	{
	    STcopy("OVERRIDING SYSTEM VALUE", expand_string);
	    break;
	}
	else if (col_p->column_ident == COL_IDENT_BYDEFAULT)
	{
	    STcopy("OVERRIDING USER VALUE", expand_string);
	    break;
	}
    }
}


/*{
** Name:	expand_vps - expand the page size clause
**
** Description:
**	Expand the page size clause token.
**
** Inputs:
**	table_no	- table number
**
** Outputs:
**	expand_string	- the expanded string
**
** Returns:
**	none
*/
void
expand_vps(
i4	table_no,
char	*expand_string)
{
	(void)RMtbl_fetch(table_no, FALSE);

	STprintf(expand_string, ERx("%d"), RMtbl->page_size);
}


/*{
** Name:	prefix_name - prefix a name
**
** Description:
**	Prefix a name as required for the expansion of various tokens.  If the
**	current server supports delimited names, delimit the name as well.
**
** Inputs:
**	prefix		- name prefix of a form like:
**				""	- no prefix
**				":"	- host variable declarator
**				"old_"	- prefix
**				":old_"	- declarator + prefix
**				"c."	- correlation variable
**				"new."	- correlation variable
**				"c.old_"- correlation + prefix
**	name		- the name to prefix
**
** Outputs:
**	prefixed_name	- if supplied, this buffer is filled with the prefixed
**			  name; if NULL, a global buffer is used and the
**			  caller should do an STcopy() upon return.
**
** Returns:
**	pointer to the prefixed name
*/
static char *
prefix_name(
char	*prefix,
char	*name,
char	*prefixed_name)
{
	char	tmp_prefix[DLNM_SIZE];
	char	tmp_suffix[DLNM_SIZE];
	char	tmp_name[DLNM_SIZE];
	char	*pn;
	char	*p;

	if (prefixed_name != NULL)
		pn = prefixed_name;
	else
		pn = Prefixed_Name_Buf;

	STlcopy(prefix, tmp_prefix, DLNM_SIZE);
	*tmp_suffix = EOS;

	if ((p = STrchr(tmp_prefix, '.')) != NULL ||	/* "c.old_" */
		(p = STrchr(tmp_prefix, ':')) != NULL)	/* ":old_" */
	{
		CMnext(p);
		STcat(tmp_suffix, p);
		*p = EOS;
	}
	else
	{
		STcat(tmp_suffix, tmp_prefix);			/* "old_" */
		*tmp_prefix = EOS;
	}

	STcat(tmp_suffix, name);

	STcopy(tmp_prefix, tmp_name);
	STcat(tmp_name, RPedit_name(EDNM_DELIMIT, tmp_suffix, tmp_suffix));

	STcopy(tmp_name, pn);
	return (pn);
}


/*{
** Name:	column_alias - generate alphanumeric column alias
**
** Description:
**	Generates an alphanumeric column alias from a column sequence number.
**
** Inputs:
**	column_num	- column sequence number
**
** Outputs:
**	alias		- if supplied, this buffer is filled with the column
**			  alias; if NULL, a global buffer is used and the
**			  caller should do an STcopy() upon return.
**
** Returns:
**	pointer to the column alias
*/
static char *
column_alias(
i4	column_num,
char	*alias)
{
	char	tmp_name[DLNM_SIZE];
	char	*ca;

	if (alias != NULL)
		ca = alias;
	else
		ca = Column_Alias_Buf;

	STprintf(tmp_name, ERx("%s%0*d"), COL_ALIAS_PREFIX, COL_ALIAS_NDIGITS,
		column_num);

	STcopy(tmp_name, ca);
	return (ca);
}
