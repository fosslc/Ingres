/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>

/**
** Name:    osformrefs.c -	OSL Parser Symbol Reference Module.
**
** Description:
**	Contains routines used to set references for symbol table entries.
**
**	osmarkunld()	mark table field symbol for unload.
**	osclrrefs()
**	osclrunld()	clear table field symbol for unload.
**	osmarkput()	mark symbol for display.
**	osgenputs()	generate code to display values in marked fields.
**	osunldobj()	get the current UNLOADTABLE object.
**
** History:
**	Revision 5.1  86/10/17  16:14:38  wong
**	Extracted from "osgetform.c" (07/17.)
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in osclrrefs).
**
**	Revision 6.5
**	30-jul-92 (davel)
**		removed trigraph sequences for ANSI compilers.
**/

static OSSYM *UnldSym = NULL;

/*{
** Name:    osmarkunld() -	Mark the Table Field for Unload.
**
** Description:
**	Set the reference member in the symbol table entry of the named
**	table field for unloading and mark its columns as being referenced
**	as well.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**	sym	{OSSYM *}    The object to unload.
**
** History:
**	?/? (jrc) - Written.
**	10/87 (jhw) - Merged 'osmarktfld()' into this routine.
*/

osmarkunld (form, sym)
OSSYM	*form;
OSSYM	*sym;
{
    /* no action on array, here. */

    if (sym->s_kind == OSTABLE)
    {
	register OSSYM	*col;

	form->s_ref |= OS_TFLDREF;
	sym->s_ref |= OS_TFLDREF | OS_TFUNLOAD;
	for (col = sym->s_columns ; col != NULL ; col = col->s_sibling)
	    col->s_ref |= OS_TFCOLREF;
    }

    /* This symbol, if it's an array, doesn't have an ilref yet. */

    UnldSym = sym;
}

/*{
** Name:    osunldobj() -	get the current UNLOADTABLE object.
**
** Description:
**
** History:
**	10/89 (billc)	Written.
*/
OSSYM *
osunldobj()
{
	return UnldSym;
}

/*{
** Name:    osclrrefs() -	Clear Reference Flags for Form Fields.
**
** Description:
**	Reset the s_ref member in the symbol table entries for a form and
**	all its fields and table fields.
**	Called at the end of an OSL statement.
**
** History:
**	08/86 (jhw)	Clear table field columns as well.
**	30-sep-1985	Do not clear s_ref member for symbol table entries
**			of kinds OSFRAME or OSPROC.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Only clear the OS_REFMASK bits in s_ref field.
**		Also, removed check for symbol table entries
**		of kinds OSFRAME or OSPROC; they do not now appear
**		in the s_fields list (and I'm not clearing the OS_OBJREF
**		bit anymore, anyway).
*/
VOID
osclrrefs (form)
register OSSYM	*form;
{
    register OSSYM	*fld;
    register OSSYM	*tfld;

    if (form != NULL)
    {
	form->s_ref &= ~OS_REFMASK;
	for (fld = form->s_fields ; fld != NULL ; fld = fld->s_sibling)
	{
	    fld->s_ref &= ~OS_REFMASK;
	}
	for (tfld = form->s_tables ; tfld != NULL ; tfld = tfld->s_sibling)
	{
	    register OSSYM	*col;

	    tfld->s_ref &= OS_TFUNLOAD;
	    for (col = tfld->s_columns ; col != NULL ; col = col->s_sibling)
	    {
		col->s_ref &= ~OS_REFMASK;
	    }
	}
    }
}

/*{
** Name:    osclrunld() -	Clear Table Field Unloadtable State.
**
** Description:
**	Clear the OS_TFUNLOAD bit in the s_ref member in the
**	symbol table entry for a table field when it is no longer
**	being unloadtable-d.
**
** Input:
**	tblfld	{OSSYM *}  The symbol node for the table field.
*/
VOID
osclrunld (tblfld)
OSSYM	*tblfld;
{
    	if (tblfld->s_kind == OSTABLE)
		tblfld->s_ref &= ~OS_TFUNLOAD;
    	UnldSym = NULL;
}

static OSNODE	*_Putlist = NULL;

/*{
** Name:    osmarkput() -	Mark Symbol for Output.
**
** Description:
**	A field or table field column has been referenced as an LVALUE 
**	of a target list, and will need a `putform (putrow)' done for it.
**	Similarly, for a field or table field column referenced BYREF (VALUE
**	node.)  This routine adds the symbol table entry for the referenced
**	field to the static list '_Putlist'.
**
** Input:
**	fldn	{OSNODE *}  A VALUE node.
*/
osmarkput (fldn)
register OSNODE	*fldn;
{
    U_ARG	nodep;
    U_ARG	listp;

    if ( !(fldn->n_flags & N_VISIBLE) )
	return;

    if (fldn->n_token != VALUE)
	osuerr(OSBUG, 1, ERx("osmarkput(field)"));

    nodep.u_nodep = fldn;

    listp.u_nodep = _Putlist;
    _Putlist = osmknode(NLIST, &nodep, &listp, (U_ARG *)NULL);
}

/*{
** Name:    osgenputs() -      Generate Code to Display Values in Marked Fields.
**
** Description:
**	Generate putforms (for regular fields) and putrows
**	(for table field columns) when needed.
**	Traverses the list returned by 'osputlist()'.
*/
VOID
osgenputs ()
{
    register OSNODE	*node;
    OSNODE		*list;

    OSNODE	*osputlist();

    list = _Putlist;
    _Putlist = NULL;
    for (node = list ; node != NULL ; node = node->n_next)
    {
	osvardisplay(node->n_ele);
    }
    ostrfree(list);
}
