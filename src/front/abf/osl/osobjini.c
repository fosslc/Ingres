/*
** Copyright (c) 1984, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#include	<afe.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:    osobjinit.c -	OSL Translator Generate Interpreted Object
**					Intermediate Language Prologue Module.
** Description:
**	Contains routines to perform initialization for OSL objects including
**	generation of intermediate language code for object (frame or procedure)
**	prologues.
**
** Description:
**	ostabsinit()	generate form table fields initialization IL code.
**	osinittab()	generate initialize table field IL code.
**
** History:
**	Revision 6.4/02
**	10/02/91 (emerson)
**		Removed #include of unused header file osloop.h
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Removed functions osfrminit and osprocinit
**		(Their logic is now in the grammar).
**
**	Revision 5.1  86/07/18  09:52:33  wong
**	Initial revision.  (Modified from OSL compiler "osobjinit.c".)
**/

VOID	oshidcol();

/*
** Name:    ostabsinit() -	Generate Form Table Fields
**					Initialization IL Code.
** Description:
**	Generates `initialize table' IL code for all table fields in the
**	form for the frame being translated by calling 'osinittab()' for
**	each table field.
**
** Input:
**	form	{OSSYM *}  Symbol table entry for the form.
**
** History:
**	07/86 (jhw) -- Modified from OSL compiler 'osinittabs()'.
*/

VOID
ostabsinit (form)
OSSYM	*form;
{
    register OSSYM	*tblfld;

    VOID	osinittab();

    if (form == NULL || form->s_kind != OSFORM)
	return;		/* ERROR */
    for (tblfld = form->s_tables ; tblfld != NULL ; tblfld = tblfld->s_sibling)
	osinittab(form, tblfld->s_name);
}

/*{
** Name:    osinittab() -	Generate Initialize Table Field IL Code.
**
** Description:
**	Generates the IL code for initializing a table field, if it exists:
**
**		ILINITTAB
**		possible hidden column declarations
**		ILENDLIST
**
** Input:
**	form		{char *}
**		Name of the form.
**
**	name		{char *}
**		Name of the table field.
**
** History:
**	07/86 (jhw) -- Written.
*/

VOID
osinittab (form, table)
OSSYM	*form;
char	*table;
{
    if (ostblcheck(form, table) != NULL)
    {
	IGgenStmt(IL_INITTAB, (IGSID *)NULL,
		2, IGsetConst(DB_CHA_TYPE, table),(ILREF)0
	);
	oshidcol(form, table);
	IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
    }
}

/*{
** Name:	oshidcol() -	Generate Hidden Column Declarations for
**					IL Table Field Initialization Code.
** Description:
**	Generate IL code to declare any hidden columns in the given table field:
**
**		ILTL2ELM	column	type
**		...
**
** Parameters:
**	form	{OSSYM *}  Symbol table entry for the form.
**	name	{char *}  Name of the table field.
**
** History:
**	07/86 (jhw) -- Modified from OSL compiler 'oshidcol()'.
**	04/87 (jhw) -- Modified to use AFE to get type specification.
**	14-jun-90 (mgw) Bug #30781
**		Don't try to generate any IL code here unless "name" really is
**		a table field.
*/
VOID
oshidcol (form, name)
OSSYM	*form;
char	*name;
{
    OSSYM		*tblfld;

    if ( (tblfld = ossympeek(name, form)) != NULL && tblfld->s_kind == OSTABLE )
    {
	register OSSYM	*col;

	for (col = tblfld->s_columns; col != NULL; col = col->s_sibling)
	{
	    if ( col->s_kind == OSHIDCOL && !(col->s_flags & FDF_READONLY) )
	    {
		DB_USER_TYPE	buf;
		DB_DATA_VALUE	dbv;

		dbv.db_datatype = col->s_type;
		dbv.db_length = col->s_length;
		dbv.db_prec = col->s_scale;
		ostypeout(&dbv, &buf);
		IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2,
			IGsetConst(DB_CHA_TYPE, col->s_name),
			IGsetConst(DB_CHA_TYPE, buf.dbut_name)
		);
	    }
	} /* end for */
    }
}
