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
#include	<oslconf.h>
#include	<ilops.h>
#include	<iltypes.h>

/**
** Name:	osquel.c -	OSL QUEL support module.
**
** Description:
**	Contains routines that process references to fields or columns of
**	a form, for QUEL only.  
**	
**	osformall()	get values for all fields on form.
**	ostabfall()	get values for table field row.
**	osfldval()	get value for field.
**	ostfval()	get value for table field column/row.
**
** History:
**	10/89 (billc) - split off from osreffrm.c.
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
**/


/*{
** Name:    osformall() -	Get Values for All Fields on Form.
**
** Description:
**	This routine generates IL code to access the values in each regular
**	field on a form (but not table fields or hidden fields.)  At the same
**	time, it also builds a node list of references for the field values and
**	returns this list.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**
** Returns:
**    {OSNODE *}	A list of field symbols for the values.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Modified to build and return node list.
*/
OSNODE *
osformall (form)
OSSYM	*form;
{
    register OSNODE	*list = NULL;

    if (form->s_kind == OSFORM && form->s_fields != NULL)
    { /* generate access for all fields on form */
    	register OSSYM	*fld;

    	form->s_ref |= OS_FLDREF;
    	for (fld = form->s_fields ; fld != NULL ; fld = fld->s_sibling)
    	{
	    U_ARG	symp;
	    U_ARG	nlist;

	    if (fld->s_kind == OSFIELD && (fld->s_ref & OS_FLDREF) == 0)
	    {
		fld->s_ref |= OS_FLDREF;
		IGgenStmt(IL_GETFORM, (IGSID *)NULL,
		    2, IGsetConst(DB_CHA_TYPE, fld->s_name), fld->s_ilref
		);
	    }
	    symp.u_symp = fld;
	    symp.u_nodep = osmknode(VALUE, &symp, (U_ARG *)NULL, (U_ARG *)NULL);
	    nlist.u_nodep = list;
	    list = osmknode(NLIST, &symp, &nlist, (U_ARG *)NULL);
    	}
    }
    return list;
}

/*{
** Name:	ostabfall() -	Get Values for Table Field Row.
**
** Description:
**	Builds a node list of references for the column/row values that will be
**	returned to the caller.  At the same time, it generates IL code to
**	access all the values in the columns for a table field row (but not
**	hidden columns) if a specific row is referenced or the table field
**	is being unloaded.  (Unreferenced table field access refers to the
**	row being unloaded when in an UNLOADTABLE loop.)
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form.
**	table	{char *}  The name of the table field.
**	expr	{OSNODE *}  The table field row reference expression.
**
** Returns:
**	{OSNODE *}	A list of table field symbols for the values.
**
** History:
**	09/86 (jhw) -- Written.
**	04/88 (jhw) -- Modified to not generate IL code to fetch values when
**			row not referenced in unload loop (which was a Bug!)
*/

OSNODE *
ostabfall (form, table, expr)
OSSYM	*form;
char	*table;
OSNODE	*expr;
{
	register OSNODE	*list = NULL;
	register OSSYM	*tfld;

	if ( (tfld = ossymlook(table, form)) == NULL || tfld->s_kind != OSTABLE )
		oscerr( OSNOTBLFLD, 2, table, form->s_name );
	else
	{ /* a table field ... */
		register OSSYM	*col;
		bool		fetch = expr != NULL || (tfld->s_ref & OS_TFUNLOAD) == 0;

		ostmpbeg();
		if ( fetch )
		{ /* will be fetching row ... */
			IGgenStmt(IL_GETROW, (IGSID *)NULL,
					2, IGsetConst(DB_CHA_TYPE, tfld->s_name),
						(expr == NULL ? 0 : osvalref(expr))
			);
		}
		ostmpend();
		for (col = tfld->s_columns ; col != NULL ; col = col->s_sibling)
		{ /* ... all visible columns */
			if ( col->s_kind == OSCOLUMN )
			{
				register OSNODE	*symn;
				U_ARG	symp;
				U_ARG	nlist;

				symn =
					ostvalnode(tfld, tfld->s_name, (OSNODE*)NULL, col->s_name);
				if ( fetch )
				{ /* fetch column value ... */
					IGgenStmt(IL_TL2ELM, (IGSID *)NULL,
					   2, symn->n_sym->s_ilref,
						IGsetConst(DB_CHA_TYPE, col->s_name)
					);
				}
				symp.u_nodep = symn;
				nlist.u_nodep = list;
				list = osmknode(NLIST, &symp, &nlist, (U_ARG *)NULL);
			}
		} /* end for */
		IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	}
	return list;
}

/*{
** Name:    osfldval() -	Get Value for Field.
**
** Description:
**	Generate IL code to access the value in a regular (form) field.  This
**	sets the reference bit in the symbol table structure to show that
**	a reference is active.
**
**	Note:  This routine should only be used for fields and variables,
**	although no IL code is generated for the latter.
**
** Input:
**	refn	{OSNODE *}  The reference node for the field (contains
**				 a reference to the symbol table entry.)
**
** History:
**	08/86 (jhw) -- Written.
*/

VOID
osfldval (refn)
register OSNODE	*refn;
{
    if (refn != NULL && refn->n_token == VALUE)
    {
	register OSSYM	*fld = refn->n_sym;

	if (fld->s_kind == OSFIELD && (fld->s_ref & OS_FLDREF) == 0)
	    IGgenStmt(IL_GETFORM, (IGSID *)NULL,
		2, IGsetConst(DB_CHA_TYPE, fld->s_name), fld->s_ilref
	    );

	fld->s_parent->s_ref |= OS_FLDREF;
	fld->s_ref |= OS_FLDREF;
    }
    else
    {
	osuerr(OSBUG, 1, ERx("osfldval"));
    }
}

/*{
** Name:    ostfval() -	Get Value for Table Field Column/Row.
**
** Description:
**	Generates IL code to access a column value for a table field row.
**	This may include code evaluating the expression representing the
**	row reference.
*/

ostfval (refn)
register OSNODE	*refn;
{
	register OSSYM	*sym;

	if (refn == NULL 
	  || refn->n_token != VALUE 
	  || refn->n_tfref == NULL)
	{
	    osuerr(OSBUG, 1, ERx("ostfval"));
	    return;
	}

	sym = refn->n_tfref;

	/*
	** Generate access only if reference is not to the current row
	** of the table field or the table field is not being unloaded.
	*/
	if (refn->n_sexpr != NULL || (sym->s_parent->s_ref & OS_TFUNLOAD) == 0)
	{
	    refn->n_sref = 0;
	    if (refn->n_sexpr != NULL)
	    { /* reference to specific row */
		refn->n_sref = osvalref(refn->n_sexpr);
		refn->n_sexpr = NULL;
	    }
	    IGgenStmt(IL_GETROW, (IGSID *)NULL,
		2, IGsetConst(DB_CHA_TYPE, sym->s_parent->s_name), refn->n_sref
	    );
	    IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, refn->n_sym->s_ilref,
			IGsetConst(DB_CHA_TYPE, sym->s_name)
	    );
	    IGgenStmt(IL_ENDLIST, (IGSID *)NULL, 0);
	}
}

