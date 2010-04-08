/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fdesc.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>

/**
** Name:	ossymchk.c -	OSL Parser Symbol Check Module.
**
** Description:
**	Routines to return values of, or check, symbol table entries.
**	This file defines:
**
**	ostblcheck()	check and return table field symbol entry.
**	os_ta_check()	check and return table-field or array symbol entry.
**	oscolcheck()	table field column symbol check.
**	osfldcheck()	field symbol check.
**	osqryobjchk()	check and return query object symbol entry.
**	osqryidchk()	check id for query target list.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Part of fix for bugs 34846 and 39582 in osqryidchk.
**
**	Revision 6.4
**	11/14/90 (emerson)
**		Check for global variables (OSGLOB)
**		as well as local variables (OSVAR).  (Bug 34415).
**	04/07/91 (emerson)
**		Modifications for local procedures (osqryobjchk).
**
**	Revision 6.0  87/02  wong
**	Pass types as DB_DATA_VALUEs, 02/87.
**
**	Revision 5.1  86/11/04  15:31:14  wong
**	Modified from compiler version.
**/

/*{
** Name:	ostblcheck() -	Check and Return Table Field Symbol Entry.
**
** Description:
**	Returns the symbol table entry for a named table field.  If the
**	table field does not exists, an error message is generated.
**
** Input:
**	form		{OSSYM *}  The symbol table entry of the form.
**	tfldname	{char *}  The name of the table field.
**
** Returns:
**	{OSSYM *}	The symbol table entry, if found.  NULL otherwise.
**
** History:
**	09/86 (jhw) -- Modified.
*/
OSSYM *
ostblcheck (form, tfldname)
OSSYM	*form;
char	*tfldname;
{
	register OSSYM	*tbl;

	if ((tbl = ossympeek(tfldname, form)) == NULL || tbl->s_kind != OSTABLE)
	{
		oscerr(OSNOTBLFLD, 2, tfldname, form->s_name);
		tbl = NULL;
	}
	return tbl;
}

/*{
** Name:	os_ta_check() -	Check and Return Table Field or Array Symbol.
**
** Description:
**	Returns the symbol table entry for a named table field or array.  If the
**	table field or array does not exist, an error message is generated.
**
** Input:
**	form		{OSSYM *}  The symbol table entry of the form.
**	node		{OSNODE *}  The identifier of the table field or array.
**
** Returns:
**	{OSSYM *}	The symbol table entry, if found.  NULL otherwise.
**
** History:
**	10/89 (billc) -- Written.
*/

OSNODE *
os_ta_check (form, node)
OSSYM	*form;
OSNODE	*node;
{
	register OSSYM	*sym;

	if (osiserr(node))
		return NULL;

	sym = osnodesym(node);
	if ( sym->s_kind != OSTABLE && (sym->s_flags & FDF_ARRAY) == 0 )
	{
		node->n_flags |= N_ERROR;
		oscerr(OSNOTARRAY, 1, sym->s_name);
	}
	return node;
}

/*{
** Name:	oscolcheck() -	Table Field Column Symbol Check.
**
** Description:
**	Check that the specified column exists for the specified table field
**	on the specified form.
**
** Input:
**	form	The symbol table entry for the form.
**	table	The name of the table field.
**	col	The name of the column.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
oscolcheck (form, table, col)
OSSYM	*form;
char	*table;
char	*col;
{
	register OSSYM	*tsym;
	register OSSYM	*csym;

	if ((tsym = ossympeek(table, form)) == NULL || tsym->s_kind != OSTABLE)
		oscerr(OSNOTBLFLD, 2, table, form->s_name);
	else if ((csym = ossympeek(col, tsym)) == NULL || csym->s_kind != OSCOLUMN)
		oscerr(OSNOCOL, 3, form->s_name, table, col);
}

/*{
** Name:	osfldcheck() -	Field Symbol Check.
**
** Description:
**	Check that the specified (visible) field exists in the specified form.
**
** Input:
**	form	The symbol table entry for the form.
**	fld	The name of the field.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
osfldcheck (form, fld)
OSSYM	*form;
char	*fld;
{
	register OSSYM	*symp;

	if ((symp = ossympeek(fld, form)) == NULL ||
			(symp->s_kind != OSFIELD && symp->s_kind != OSTABLE))
		oscerr(OSRHSUNDEF, 1, fld);
}

/*{
** Name:	osqryobjchk() -	Check and Return Query Object Symbol Entry.
**
** Description:
**	This routine checks that the input name (plus attribute) identify
**	a valid attached query object for the input form.  A valid query
**	object is either the form, itself, or a table field that exists
**	on the form.  The symbol table entry for the query object will be
**	returned.
**
** Input:
**	form	{OSSYM *}  Symbol table entry for current form.
**	obj	{OSNODE *}  the query object (or query object parent.)
**
** Returns:
**	{OSSYM *}  Reference to query object symbol table entry.
**		   NULL if invalid.
**
** History:
**	06/87 (jhw) -- Written.
**	10/89 (billc) -- Modified for new value nodes to support complex objects
**	04/07/91 (emerson)
**		Modifications for local procedures: don't allow a local proc
**		as a query object.
*/
OSSYM *
osqryobjchk (form, obj)
OSSYM	*form;
OSNODE	*obj;
{
	if ( obj->n_token == DOT 
	  || obj->n_token == ARRAYREF 
	  || obj->n_token == VALUE )
	{
		register OSSYM	*sym = osnodesym(obj);

		if ( sym->s_kind == OSTABLE
			|| ( sym->s_kind == OSFORM
					&& sym->s_parent->s_kind != OSFORM )
			|| ( ( sym->s_kind == OSVAR
					|| sym->s_kind == OSGLOB
					|| sym->s_kind == OSRATTR )
				&& (sym->s_flags & (FDF_RECORD|FDF_ARRAY))
					!= 0 ) )
		{
			return sym;
		}
	}
	/* isn't in FORM.TABLE, FORM, TABLE, CLASS or ARRAY format */
	return NULL;
}

/*{
** Name:	osqryidchk() -	Check Identifier for Query Target List.
**
** Description:
**	This routine checks that the input name identifies a field or column
**	belonging to the input form object.
**
** Input:
**	formobj	{OSSYM *}  The symbol table entry for the form object.
**			   Must not be NULL.
**	id	{char *}   The identifier.
**
** Returns:
**	A pointer to a symbol table entry (possibly an "undefined" symbol)
**	representing the child of the form object identified by the identifier.
**
** History:
**	09/86 (jhw) -- Written.
**	10/90 (jhw) -- Added complex object support.
**	09/20/92 (emerson)
**		Part of fix for bugs 34846 and 39582:
**		Removed the logic that returned NULL if formobj was NULL;
**		the caller must now ensure that formobj is not NULL.
**		Return an "undefined" symbol instead of NULL in the case
**		where id is not an attribute of a record or array, as we
**		already were doing when id is not a child of a form or
**		tablefield.  (This simplifies life for the caller).
**		Also simplified the logic.
*/
OSSYM *
osqryidchk (formobj, id)
register OSSYM	*formobj;
char		*id;
{
	register OSSYM	*sym = ossymlook(id, formobj);

	if (sym == NULL)
	{
		if (formobj->s_kind == OSFORM)
		{
			oscerr(OSNOFLD, 2, id, formobj->s_name);
		}
		else if (formobj->s_kind == OSTABLE)
		{
			oscerr( OSNOTFCOL, 3, id, formobj->s_name,
						formobj->s_parent->s_name );
		}
		else	/* formobj is a record or array */
		{
			oscerr(OSNOTMEM, 2, id, formobj->s_name);
		}
		sym = ossymundef(id, formobj);
	}
	else if (formobj->s_kind == OSFORM)
	{
		if (sym->s_kind == OSTABLE)
		{
			oscerr(OSQRYTFLD, 1, id);
		}
		else if ( ( sym->s_kind != OSFIELD
				&& sym->s_kind != OSVAR
				&& sym->s_kind != OSGLOB
				&& sym->s_kind != OSUNDEF )
			|| ( sym->s_flags & (FDF_RECORD|FDF_ARRAY) ) )
		{
			oscerr(OSNOFLD, 2, id, formobj->s_name);
		}
	}
	else if ( formobj->s_kind != OSTABLE )
	{	/* formobj is a record or array; sym->s_kind == OSRATTR */
		if ( sym->s_flags & (FDF_RECORD|FDF_ARRAY) )
		{
			oscerr(OSQRYTFLD, 1, id);
		}
	}
	return sym;
}
