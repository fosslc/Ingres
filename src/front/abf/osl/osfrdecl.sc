/*
** Copyright (c) 1984, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#include	<afe.h>
#include	<oodep.h>
#include	<oocat.h>
EXEC SQL INCLUDE <ooclass.sh>;
#include	<fdesc.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osglobs.h>
#include     	<uigdata.h>
#include     	<iltypes.h>

/**
** Name:	osfrmdecl.sc -	OSL Translator Form Field Symbol Entry Routine.
**
** Description:
**	Contains routines used to enter form fields (both visible and hidden)
**	into the symbol table.  Defines:
**
**	osgetcurrentproc()	get the symbol table entry for the current
**				frame, procedure, or local procedure.
**	osdeclare()		enter variable, proc, or column in symbol table.
**	osdefineproc()		define a local procedure.
**	osendproc()		mark the end of a local procedure.
**	oschkundefprocs()	check for undefined procedures.
**	osformdecl()		enter form fields in symbol table.
**
** History:
**	Revision 6.4/02
**	04/27/92 (emerson)
**		Part of fix for bug 43771: created osgetcurrentproc.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Modified osformdecl and osdeclare;
**		created osdefineproc, osendproc, and oschkundefprocs.
**
**	Revision 6.1  88/11  wong
**	Modified to use forms owned by others only if one such form exists.
**	(For multi-developer environments.)
**
**	Revision 6.0  87/05  wong
**	Modified to support ADTs and NULLs with new form catalogs
**	and 5.0 compatibility support.
**
**	Revision 5.1  86/11/04  15:04:13  wong
**	Changed 'osdeclare()' to expect frame symbol table entry
**	rather than name.
**	(07/86)  Abstracted from 'osformtype()'.
**	12-sep-88 (kenl)
**		Removed EXEC SQL COMMITs.  Since the SQLized ABF normally
**		runs with autocommit on, we don't need them.
**	24-oct-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	osgetcurrentproc() -	Get symbol table entry for the current
**					frame, procedure, or local procedure.
** Returns:
**	Pointer to the symbol table entry for the current frame, procedure,
**	or local procedure; or NULL if we're not within one.
**
** History:
**	04/27/92 (emerson)
**		Written (Part of fix for bug 43771).
*/
static	OSSYM	*current_proc_sym = NULL;

OSSYM *
osgetcurrentproc()
{
	return current_proc_sym;
}

/*{
** Name:	osdeclare() - Enter Variable, Proc, or Column into Symbol Table.
**
** Description:
**	Enters a column of a table field, or a variable or local procedure
**	of the current form (which may itself be a local procedure),
**	into the symbol table.  This routine first parses the type
**	specifier and then enters the object in the symbol table.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the current form.
**	name	{char *}  The name of the field (or table field.)
**	attr	{char *}  The name of the table field attribute (column).
**	fmt	{char *}  The type name.
**	length	{char *}  The length (or precision) digit string.
**	scale	{char *}  The scale digit string.
**	null	{char *}  Null indicator (tri-valued logic, can't be bool).
**	flags	{nat}	flags, to be stashed in the runtime symbol table.
**	isproc	{bool}	Is this a declaration of a local procedure?
**
** Returns:
**	Pointer to new symbol table entry (NULL on error).
**
** History:
**	07/86 (jhw) -- Written.
**	01/87 (jhw) -- Modified to use ADF (via 'oschkdecl()'.)
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Allow forward declarations of local procedures.
**		Return pointer to new symbol table entry (instead of VOID).
**	23-sep-93 (essi)
**		Corrected typo in assignment.
*/

OSSYM *
osdeclare (form, name, attr, fmt, length, scale, null, flags, isproc)
register OSSYM	*form;
char		*name;
char		*attr;
register char	*fmt;
register char	*length;
char		*scale;
char		*null;
i4		flags;
bool		isproc;
{
	OSSYM		*tab, *sym, *asym;
	DB_DATA_VALUE   tdbv;
	register char   *chklen;
	OSSYM		*osrecdeclare();
	i4		status;

#ifdef OSL_TRACE
        {
                char *attr_tmp   = (attr == NULL ? "" : attr);
                char *fmt_tmp = (fmt == NULL ? "" : fmt);
                char *len_tmp = (length == NULL ? "" : length);
                char *scale_tmp = (scale == NULL ? "" : scale);
                char *null_tmp = (null == NULL ? "" : null);
        SIprintf("osdeclare: form=|%s|, name=|%s|, attr=|%s|, fmt=|%s|\n length=|%s|, scale=|%s|, nullable=|%s|\n",
                form, name, attr_tmp, fmt_tmp, len_tmp, scale_tmp, null_tmp
                );
        }
#endif /* OSL_TRACE */

	/*
	** Check for 'c' type with length specified or digits following
	** the 'c' (but not both) and 5.0 compatibility mode.
	** Then, convert this declaration to a text type.
	** Allow a type of 'none' if this is a procedure declaration.
	*/
	if ( isproc && STbcompare(fmt, 0, ERx("none"), 0, TRUE) == 0 )
	{
		tdbv.db_datatype = DB_NODT;
		tdbv.db_length = 0;
		tdbv.db_prec = 0;
		tdbv.db_data = ERx("none");
	}
	else if ( osCompat != 5 
	   || (*fmt != 'c' && *fmt != 'C') 
	   || (*(fmt+1) != EOS && (length != NULL || !CMdigit(fmt+1))) 
	   || *(chklen = (length == NULL ? fmt+1 : length)) == EOS )
	{ 
		/* no conversion */
		oschkdecl(name, fmt, length, scale, null, &tdbv);
	}
	else
	{ 
		/* converting 'c' type to 'text' */
		oschkdecl(name, ERx("text"), chklen, scale, null, &tdbv);
	}

	/* Check for 5.0 `i4' and `f8' Compatibility and convert length */
	if (osCompat == 5)
	{
		if (tdbv.db_datatype == DB_INT_TYPE)
			tdbv.db_length = sizeof(i4);
		else if (tdbv.db_datatype == DB_FLT_TYPE)
			tdbv.db_length = sizeof(f8);
	}

	/* Enter in Symbol Table */

	if (isproc)
	{
		if (attr != NULL)
		{
			/* tried to do "x.y = procedure returning ..."*/
			oscerr(E_OS0060_BadProcName, 2, name, attr);
			return (OSSYM *)NULL;
		}
		if (! (flags & FDF_LOCAL))
		{
			/* procedure declaration found among parameters */
			oscerr(E_OS0064_MisplacedProcDecl, 2,
				name, form->s_name);
		}

		/*
		** No two local procedures can have the same name,
		** no matter what parents they have.  Also, no local
		** procedure can have the same name as the frame's form
		** or the main procedure.
		**
		** Note that if we detect an error, we don't return;
		** we want to check for further errors. We also want to
		** create a symbol table entry to minimize future spurious errs.
		*/
		asym = ossymsearch(name, OS_DIM, OS_DARK);
		if (asym != NULL)
		{
			/* duplicate local procedure declaration */
			oscerr(E_OS0061_DupProcDecl, 1, name);
		}
	}
	else if (attr == NULL)
	{
		/*
		** We don't allow any variable declared in a local procedure
		** to have the same name as its parent local procedure
		** or the same name as any of that local procedure's ancestors.
		**
		** It would be nice to also disallow variables in a frame's
		** INITIALIZE statement from having the same name as the frame's
		** form, and to disallow variables in a main procedure
		** from having the same name as the main procedure,
		** but that presents a backwards compatibilty problem.
		** (Maybe we should issue a warning?)
		**
		** Note that if we detect an error, we don't return;
		** we want to check for further errors. We also want to
		** create a symbol table entry to minimize future spurious errs.
		*/
		asym = NULL;
		if (form->s_parent->s_kind == OSFORM)
		{
			asym = ossymsearch(name, OS_BRIGHT, OS_DARK);
		}
		if (asym != NULL)
		{
			/* variable name same as one of its ancestors */
			oscerr(E_OS0062_AmbiguousVar, 1, name);
		}
	}

	if (tdbv.db_datatype == DB_DMY_TYPE)
	{
		/* we have a complex object type. */
		if (isproc)
		{
			/* tried to declare procedure returning complex type */
			oscerr(E_OS0063_BadProcType, 1, name);
			tdbv.db_datatype = DB_NODT;
			tdbv.db_length = 0;
			tdbv.db_prec = 0;
			tdbv.db_data = ERx("none");
			flags &= ~(FDF_ARRAY | FDF_RECORD);
		}
		else
		{
			if (flags & FDF_ARRAY)
			{
				/* And it's an array. */
				tdbv.db_length = 1;
			}
			if (attr != NULL)
			{
				/* tried to do "x.y = record of ..." */
				oscerr(OSBADHIDCOL, 2, name, attr);
				return (OSSYM *)NULL;
			}
			return osrecdeclare(form, name, OSVAR, &tdbv);
		}
	}
	if (flags & FDF_ARRAY)
	{
		/* scalar type declared as an array. */
		oscerr(OSCANTARRAY, 1, fmt);
		flags &= ~FDF_ARRAY;
	}
	if (attr == NULL)
	{
		i4	kind = isproc ? OSFORM : OSVAR;

		sym = ossymput(name, kind, &tdbv, form, flags);
		if (sym == NULL)
		{
			/* duplicate variable or local procedure */
			if (asym == NULL)
			{
				oscerr(OSEHIDDEN, 2, name, form->s_name);
			}
			return (OSSYM *)NULL;
		}

		/*
		** If we're declaring a local procedure, we need to
		** allocate a SID for it (osdefineproc will set it).
		** We also need to mark its symbol table entry as "dim".
		** If we already found a duplicate "dim" procedure
		** of the same name, mark the duplicate symbol table entry
		** as defined to prevent spurious "undefined procedure" message.
		*/
		if (isproc)
		{
			sym->s_sid = (PTR)IGinitSID();
			sym->s_brightness = OS_DIM;
			if (asym != NULL)
			{
				asym->s_ref |= OS_PROCDEFINED;
			}
		}
		return sym;
	}
	tab = ostblcheck(form, name);
	if (tab == NULL)
	{
		return (OSSYM *)NULL;
	}
	sym = ossymput(attr, OSHIDCOL, &tdbv, tab, flags);
	if (sym == NULL)
	{
		/* duplicate table field column */
		oscerr(OSEHIDCOL, 3, attr, name, form->s_name);
	}
	return sym;
}

/*{
** Name:	osdefineproc() - Define a Local Procedure.
**
** Description:
**	This function is called to update the symbol table
**	when a local procedure definition is encountered.
**	(A *definition* is the full body of the local procedure).
**	A forward *declaration* of the local procedure should have already
**	been encountered.  If not, an error message is written,
**	a declaration of PROCEDURE RETURNING NONE is assumed,
**	and the local procedure is made a child of the default parent.
**
**	The OSFORM symbol table entry for the local procedure
**	is marked as "defined" and "bright",
**	and all its OSFORM ancestors are also marked as "bright".
**
** Input:
**	name		{char *}   The name of the local procedure being defined
**	default_parent	{OSSYM *}  The symbol table entry for the parent the
**				   local procedure should be made a child of,
**				   if the local procedure hasn't been declared.
** Returns:
**	{OSSYM *}  The local procedure symbol table entry.
**
** History:
**	04/07/91 (emerson)
**		Written (for local procedures).
**	04/27/92 (emerson)
**		Set current_proc_sym (Part of fix for bug 43771).
*/

OSSYM *
osdefineproc (name, default_parent)
char		*name;
OSSYM		*default_parent;
{
	OSSYM	*sym, *asym;

	sym = ossymsearch(name, OS_DIM, OS_DARK);
	if (sym == NULL)
	{
		/* local procedure hasn't been declared */
		oscerr(E_OS0065_ProcNotDecl, 1, name);
		sym = osdeclare(default_parent, name,
				(char *)NULL, ERx("none"),
				(char *)NULL, (char *)NULL,
				(char *)NULL, FDF_LOCAL,
				(bool)TRUE);
	}
	else if (sym->s_ref & OS_PROCDEFINED)
	{
		/* local procedure has already been defined */
		oscerr(E_OS0066_DupProcDefn, 1, name);
	}
	sym->s_ref |= OS_PROCDEFINED;
	IGsetSID((IGSID *)sym->s_sid);

	for (asym = sym; asym->s_kind == OSFORM; asym = asym->s_parent)
	{
		asym->s_brightness = OS_BRIGHT;
	}

	current_proc_sym = sym;

	return sym;
}

/*{
** Name:	osendproc() - Mark the End of a Local Procedure.
**
** Description:
**	This function is called to update the symbol table
**	when a local procedure definition has been completely processed.
**	(A definition is the full body of the local procedure).
**
**	The OSFORM symbol entry entry for the local procedure
**	and all its OSFORM ancestors are marked as "dim".
**	(Any other OSFORM symbol table entries were already marked as "dim").
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the local procedure
**			   that was just defined.
** Returns:
**	VOID
**
** History:
**	04/07/91 (emerson)
**		Written (for local procedures).
**	04/27/92 (emerson)
**		Clear current_proc_sym (Part of fix for bug 43771).
*/

VOID
osendproc (form)
OSSYM	*form;
{
	OSSYM	*asym;

	for (asym = form; asym->s_kind == OSFORM; asym = asym->s_parent)
	{
		asym->s_brightness = OS_DIM;
	}

	current_proc_sym = NULL;
}

/*{
** Name:	oschkundefprocs() - Check for Undefined Procedures.
**
** Description:
**	This function is called at the end of the 4GL source file
**	to check for local procedures that were declared but never defined.
**
** Input:
**	form	{OSSYM *}  The symbol table entry for the form or main procedure
**
** Returns:
**	VOID
**
** History:
**	04/07/91 (emerson)
**		Written (for local procedures).
*/

VOID
oschkundefprocs (form)
OSSYM	*form;
{
	OSSYM	*sym;

	for (sym = form->s_subprocs; sym != NULL; sym = sym->s_sibling)
	{
		if (sym->s_ref & OS_PROCDEFINED)
		{
			oschkundefprocs(sym);
		}
		else
		{
			/* local procedure was never defined */
			oscerr(E_OS0067_ProcNotDefn, 1, sym->s_name);
		}
	}
}

GLOBALREF char	*osDatabase;	/* database name */
GLOBALREF i4	osCompat;	/* 5.0 text compatibility */

/* Constants from the forms catalogs. */
#define		FREGULAR	0
#define		FTABLE	1
#define		FCOLUMN	2

/*{
** Name:	osformdecl() -	Enter Form Fields in Symbol Table.
**
** Description:
**	Enter a form and the fields defined for the form (i.e., those that are
**	visible) into the symbol table.  This routine is called once per 4GL
**	source file (even when the 4GL source file is for a global procedure).
**
** Input:
**	form	{char *}  The name of the form/procedure object
**			  for which to enter fields.
**	fields	{bool}	  Whether fields are to be entered for the form.
**	dbdtype {DB_DATA_VALUE *} The type returned by the form's frame
**				  or the procedure.
** Returns:
**	{OSSYM *}  The form/procedure object symbol table entry.
**
** History:
**	?/85 (agh) -- Written.
**	07/86 (jhw) -- Removed type declaration and hidden field/column entry
**			into separate routines.
**	02/87 (jhw) -- Modified to use ADF and ...
**	25-aug-88 (kenl)
**		Removed trim() from SQL SELECT and added STtrmwhite() to
**		SELECT loop.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added dbdtype parameter.
**		Add new dummy symbol Forms; make top OSFORM its only child.
**		Set s_ref and s_brightness in new top OSFORM.
**	04/27/92 (emerson)
**		Set current_proc_sym (Part of fix for bug 43771).
*/

/* Form Symbol Table */
static	OSSYM	Forms ZERO_FILL;

static	STATUS	frm_fields();

OSSYM *
osformdecl (form, fields, dbdtype)
char		*form;
bool		fields;
DB_DATA_VALUE   *dbdtype;
{
	OSSYM	*fsymp;

	if (form == NULL || *form == EOS)
		osuerr(OSNOFRMNAME, 0);	/* exit! */
	/*
	** Add frame/form/procedure to the symbol table.
	*/
	Forms.s_name = ERx("%FORM");
	Forms.s_kind = OSDUMMY;
	Forms.s_brightness = OS_DARK;

	current_proc_sym =
		fsymp = ossymput(form, OSFORM, dbdtype, &Forms, FDF_LOCAL);
	fsymp->s_ref |= OS_PROCDEFINED | OS_ADOPT_CHILDREN_OF_BRIGHT;
	fsymp->s_brightness = OS_BRIGHT;

	if (fields)
	{
		OOID	frid;		/* form ID */
		char	dummy[OODATESIZE+1];
		STATUS	IIAMfoiFormInfo();

		if (osDatabase == NULL)
			osuerr(OSNODB, 0);	/* exit! */

		/* Make sure the form exists. */
		if (IIAMfoiFormInfo(form, dummy, &frid) != OK 
		  || frid == OC_UNDEFINED ) 
		{
			osuerr(OSNOFORM, 1, form);	/* exit! */
		}

		/*
		** Step through the form retrieving all the fields
		** entering each into the symbol table.
		*/

		_VOID_ IIAMforFormRead(form, frid, (PTR) fsymp, frm_fields);
	}
	return fsymp;
}

static STATUS
frm_fields(fname, fldname, p, kind, dbv)
char 	*fname;
char 	*fldname;
PTR	p;
i4	kind;
DB_DATA_VALUE	*dbv;
{
	register char   *name;
	DB_DATA_VALUE	ldbv;
	OSSYM		*tabsym;
	OSSYM		*fsymp = (OSSYM *) p;
	char		*iiIG_string();

	name = iiIG_string(fldname);

	if (kind != FTABLE && osCompat == 5)
	{ 
		/* 5.0 Compatibility Conversions */
		if (dbv->db_datatype == DB_INT_TYPE)
			dbv->db_length = sizeof(i4);
		else if (dbv->db_datatype == DB_FLT_TYPE)
			dbv->db_length = sizeof(f8);
		else if (dbv->db_datatype == DB_CHR_TYPE)
		{
			dbv->db_datatype = DB_TXT_TYPE;
			dbv->db_length += DB_CNTSIZE;
		}
	}
	
	switch (kind)
	{
	  case  FTABLE:
		/*
		** If the field is a table then it should be
		** entered into the symbol table, and saved on
		** a list so its columns can be extracted later.
		*/
		if ((tabsym = ossymput(name, OSTABLE, 
			(DB_DATA_VALUE*)NULL, fsymp, 0)) == NULL )
		{
			osuerr(OSE2TFLD, 2, name, fname);
		}

		/* Add specials */
		ldbv.db_datatype = DB_INT_TYPE;
		ldbv.db_length = sizeof(i4);
		ldbv.db_data = NULL;
		ldbv.db_prec = 0;
		_VOID_ ossymput(ERx("_RECORD"), OSHIDCOL, 
				&ldbv, tabsym, FDF_READONLY
			);
		_VOID_ ossymput(ERx("_STATE"), OSHIDCOL, 
				&ldbv, tabsym, FDF_READONLY
			);
		break;

	  case FCOLUMN:
		tabsym = fsymp->s_tables;
		if (ossymput(name, OSCOLUMN, dbv, tabsym, 0) == NULL)
		{
			osuerr(OSE2COL, 2, name, tabsym->s_name);
		}
		break;

	  case FREGULAR:
		/*
		** A regular field is entered into the symbol table.
		*/
		if (ossymput(name, OSFIELD, dbv, fsymp, 0) == NULL)
			osuerr(OSE2FLD, 2, name, fname);
		break;
	}	/* end switch */

	return OK;
}

