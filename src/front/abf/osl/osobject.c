/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#ifndef FEtalloc
#define FEtalloc(tag, size, status) FEreqmem((tag), (size), FALSE, (status))
#endif
#include	<afe.h>
#include	<ooclass.h>
#include	<abclass.h>
#include	<oodep.h>
#include        <acatrec.h> /* XXX this should use abfdbcat.h */
#include	<abfdbcat.h>
#include	<fdesc.h>
#include	<osglobs.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>

#define _STATIC

/**
** Name:	osobjects.c -	OSL Parser Object Catalogs Module.
**
** Description:
**	Contains routines that access the ABF application dependency information
**	for the OSL parser.  These routines maintain and access the symbol table
**	for the application frames and procedures, reading and writing the
**	information from and to files passed in by ABF.  Defines:
**
**	osobjinit()	initialize for the frame/procedure object.
**	osframesym()	return symbol table entry for a frame object.
**	osprocsym()	return symbol table entry for a procedure object.
**	osdepwrite()	write out dependency information to database.
**	osgl_dep()	add a application-object dependency on the fid.
**	osgl_fetch()	fetch a global variable from the application catalog.
**
** History:
**	Revision 5.1  86/10/17  16:14:06  wong
**	Modified for translator:  Map dependency type to internal type.
**	Modified 'osobjref()' and 'osobjtype()' since only constants
**	(identifier or strings) can have references or a possible type.
**
**	Revision 6.0  87/02  wong
**	Made frame and procedure symbol tables local to this module.
**	Modified to use new catalogs.
**
**	Revision 6.3/03/00  90/06 Mike S
**	Discriminate between OC_DTCALL and OC_DTRCALL dependencies based
**	on whether caller uses frame's (or procedure's) return value.  Don't
**	enter dependencies for system or ADF objects.  Eliminate special
**	handling for undefined callees.
**	
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures.
**		Deleted osobjtype and replaced osobjsym
**		by new functions osframesym and osprocsym;
**		modified osobjinit.
**	06/20/91 (emerson)
**		Bug fix in osobjinit.
**	08/04/91 (emerson)
**		In osobjinit: Changes in support of allowing the user
**		to enter the osl or oslsql command manually.
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added calls to routines to pass data across ABF/IAOM boundary:
**		 IIAMgfrGetIIamFrames(), IIAMgprGetIIamProcs() and
**		 IIAMgglGetIIamGlobals().
**		(See iaom/abfiaom.c for more information).
**
**	Revision 6.5
**	18-may-93 (davel)
**		Add owner argument to osgl_dep() and _add_dep(). Part of fix
**		for bugs 51700, 51721, and 51814.
**	10-jun-93 (davel)
**		Fix bug 52458 - make sure owner is not NULL in _add_dep
**		before issuing FEtalloc on it.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchins change for axp_osf
**              29-jan-93 (kchin)
**              Changed type of osAppid and osFid from OOID to long. This is
**              to make them tally to their declaration in osglobs.h
**      23-mar-1999 (hweho01)
**              Extended the changes for axp_osf to ris_u64 (AIX 64-bit
**              platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**              Extended the changes for axp_osf to axp_lnx (Alpha Linux)
**      06-Mar-2003 (hanje04)
**              Extend axp_osf changes to all 64bit platforms (LP64).
**	24-apr-2003 (somsa01)
**		Use i8 instead of long for LP64 case.
**    25-Oct-2005 (hanje04)
**        Add prototype for obj_look() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	23-Aug-2009 (kschendel) 121804
**	    Need afe.h to satisfy gcc 4.3.
**/
GLOBALREF char	*osAppname;	/* application name */

FUNC_EXTERN bool osckadfproc();

/* Frame Symbol Table */
static OSSYM	Frames ZERO_FILL;

/* Procedure Symbol Table */
static OSSYM	Procedures ZERO_FILL;

/* Frame/Procedure Object Catalog Structure */
static ACATREC	*Objp = NULL;

static VOID	_add_dep();
static VOID	_free_dep();
static ACATDEP *_check_dep();
static APPL_COMP *sys_object();
static STATUS obj_look(
			char	*name,
			OOID	general_class,
			OOID	*class,
			register DB_DATA_VALUE	*data_type,
			bool			*isSystem);

/*{
** Name:	osobjinit() -	Initialize For The Frame/Procedure Object.
**
** Description:
**	Sets up the frame/procedure object tables and retrieves the compilation
**
** Input:
**	name	{char *}  The frame/procedure object name.
**	form	{char *}  Form associated with frame/procedure object.
**	type	{nat}  Type of object:  OSFRAME or OSPROC.
**
** Returns:
**	{OSSYM *}  The symbol table entry for the frame/procedure object.
**
** Side Effects:
**	Sets 'osForm', 'osSymbol', and 'osFid'; enters symbol for object.
**
** History:
**	05/87 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures: return symbol table pointer
**		for frame/procedure instead of echoing the name of the form.
**		Also, I don't see how this function can ever be called
**		with name set to NULL, so I removed the logic that returned
**		immediately in that case.
**	06/20/91 (emerson)
**		Revise the logic in the case where yyerror was being called to
**		report a syntax error [which was happening in most cases where
**		the frame/procedure object (specified via the -a and -f flags
**		on the osl command line) couldn't be found in the ABF catalog]:
**		(1) If there's no frame/procedure object, report an internal
**		error (presumably the osl command line or ABF catalog is bad).
**		(2) If there's a frame/procedure object, but it's of the wrong
**		type, i.e. a frame when the source file defines a procedure
**		or vice versa, issue the new error E_OS000A_TypeMismatch,
**		and create and return an "undefined" symbol table entry
**		(before, garbage was being returned when yyeror was called -
**		mea culpa).
**	08/04/91 (emerson)
**		Changes in support of allowing the user to enter
**		the osl or oslsql command manually:
**		(1) Convert application name (if provided by -a flag)
**		    to application ID.  This allows the user to specify
**		    -a<name> instead of -a<id>; see also getargs.c.
**		(2) Any errors that probably result from a bad command line
**		    are now reported as user errors, not internal errors.
**		Also, treat the case of a missing (or zero) -a flag
**		as a fatal error.  (This routine always used to just return
**		in that case; I can't see why).
*/
OSSYM *
osobjinit ( name, form, type )
char	*name;
char	*form;
i4	type;
{
	OOID	oclass;
	OOID	xclass;
	OOID	class;
	OOID	id;
	STATUS	stat;
	OSSYM	*objsym;
	OSSYM	*parent;

#if defined(LP64)
	GLOBALREF i8	osAppid;
	GLOBALREF i8	osFid;
#else /* LP64 */
	GLOBALREF OOID	osAppid;
	GLOBALREF OOID	osFid;
#endif /* LP64 */
	GLOBALREF char	*osForm;

	char	*iiIG_string();

	/* Initialize Object Tables */

	Frames.s_name = ERx("%FRAME");
	Frames.s_kind = OSFRAME;
	Frames.s_brightness = OS_DARK;
	Procedures.s_name = ERx("%PROCEDURE");
	Procedures.s_kind = OSPROC;
	Procedures.s_brightness = OS_DARK;

	if ( type == OSFRAME )
	{
		parent = &Frames;
	}
	else
	{
		parent = &Procedures;
	}

	/*
	** If osAppid is 0 and osAppname is not NULL, we need to convert
	** the application name in osAppname to an application ID in osAppid.
	** If osAppid is 0 and osAppname *is* NULL, then -a0 (or no -a flag
	** at all) was specified on the osl or oslsql command line,
	** which is a fatal error.
	*/
	if ( osAppid == 0 )
	{
		if (  osAppname == NULL
		   || IIAMoiObjId( osAppname, (OOID)OC_APPL, (OOID)0,
				    &id, &class ) != OK
		   )
		{
			osuerr( E_OS000D_BadFrameOrProc, 0 );
			/* no return */
		}
		osAppid = id;
	}

	if ( type == OSFRAME )
	{
		oclass = OC_OSLFRAME;
		xclass = OC_OSLPROC;
	}
	else
	{
		oclass = OC_OSLPROC;
		xclass = OC_OSLFRAME;
	}

	stat = IIAMoiObjId( name, oclass, osAppid, &id, &class );

	if ( stat != OK )
	{
		oclass = xclass;
		stat = IIAMoiObjId( name, oclass, osAppid, &id, &class );
	}

	if ( stat != OK || IIAMcgCatGet( osAppid, id, &Objp ) != OK )
	{
		osuerr( E_OS000D_BadFrameOrProc, 0 );
		/* no return */
	}

	if ( oclass == xclass )
	{
		/*
		** Source file defines a procedure for what the ABF
		** catalog says is a frame, or vice versa.
		*/
		oscerr( E_OS000A_TypeMismatch, 0 );
		return ossymundef( name, parent );
	}

	{
		register ACATDEP	*dep;
		register ACATDEP	*next;
		DB_DATA_VALUE		rettype;
		ACATDEP			dummy;

		rettype.db_datatype = Objp->retadf_type;
		rettype.db_length = Objp->retlength;
		rettype.db_prec = Objp->retscale;

		objsym = ossymput(name, type, &rettype, parent, 0);

		osSymbol = STalloc(Objp->symbol);
		osStatic = STequal(Objp->arg[AXOF_STATIC], ERx("static"));

		/* Search for Form and FID and delete the compiler-generated
		** dependencies while we're at it.  
		** Note:  The walk along the list must
		** assign 'dep' from 'next', which must always be the `real'
		** next otherwise the next would be skipped.
		*/
		dummy.next = Objp->dep_on;
		dummy.class = dummy.deptype = OC_UNDEFINED;
		for ( dep = &dummy ; dep->next != NULL ; dep = next )
		{
			next = dep->next;

			switch (next->deptype)
			{
			    case OC_DTGLOB:
			    case OC_DTTYPE:
			    case OC_DTFORM_TYPE:
			    case OC_DTTABLE_TYPE:
			    case OC_DTTFLD_TYPE:
			    case OC_DTCALL:
			    case OC_DTRCALL:
				/* delete these. */
				next->class = OC_UNDEFINED;
				dep->next = next->next;

				/* put it on the freelist. */
				_free_dep(next);
				next = dep;
				break;

			    default:
				/* Assign IL and form dependencies.  */
				if ( next->class == OC_ILCODE )
				{
					osFid = next->id;
				}
				else if ( ( form == NULL || *form == EOS ) 
					&& ( next->class == OC_AFORMREF 
						|| next->class == OC_FORM )
					&& next->deptype == OC_DTDBREF )
				{
					osForm = form = iiIG_string(next->name);
				}
			}  /* end switch */
		} /* end for */
		Objp->dep_on = dummy.next;
	}
	return objsym;
}

/*{
** Name:	osframesym() -	Return Symbol Node for a Frame Object.
**
** Description:
**	Return a symbol table entry for a referenced frame object.
**
** Input:
**	name	{char *}  Name of the frame.
**	enter	{bool}  Force entry into the symbol table if undefined.
**
** Returns:
**	{OSSYM *}  Symbol node for object.
**
** History:
**	30-sep-1985	First written (agh).
**	07/86 (jhw)	Modified to check only "tkID" and "tkSCONST"
**			since only those have symbol references to set.
**	02/87 (jhw)	Modified to look-up reference in DB and return
**			symbol node for object.
**	04/07/91 (emerson)
**		Modifications for local procedures.
**		First parm is now name rather node containing name.
**		Old second parm (type) has been removed.
**		Renamed old osobjsym to osframesym;
**		procedures are now handled by the new osprocsym.
*/
static OSSYM	*objlook();	/* forward reference */

OSSYM *
osframesym ( name, enter )
char	*name;
bool	enter;
{
	return objlook(name, &Frames, OSFRAME, enter);
}

/*{
** Name:	osprocsym() -	Return Symbol Node for a Procedure Object.
**
** Description:
**	Return a symbol table entry for a referenced procedure object.
**	We search first for a local procedure declared in a visible scope
**	(i.e. a child of the current OSFORM or one of its OSFORM ancestors).
**
**	Backward compatibility note:
**
**	In searching for a local procedure, we don't "see" any other kind
**	of symbol table entry. This runs counter to the philosophy that
**	frames and procedures (on the one hand) and fields, variables,
**	and constants (on the other) are all in the same name space.
**	This philosophy has been enforced for global objects
**	for as long as we've had global variables and constants.
**
**	With the advent of local procedures, we'd like extend the enforcement
**	to local objects as well, but we have to make an exception.
**	We currently allow a local variable to be declared
**	with same name as a global frame or procedure.
**	There's nothing wrong with that, but on a CALLPROC,
**	the local variable does not hide the global procedure.
**	One could argue that it should, and that the CALLPROC
**	should be flagged as an error, but we have to allow for
**	backward compatibility.
**
** Input:
**	name	{char *}  Name of the procedure.
**	enter	{bool}  Force entry into the symbol table if undefined.
**
** Returns:
**	{OSSYM *}  Symbol node for object.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/

OSSYM *
osprocsym ( name, enter )
char	*name;
bool	enter;
{
	OSSYM	*sym;

	sym = ossymsearch(name, OS_DIM, OS_BRIGHT);
	if (sym != NULL)
	{
		return sym;
	}
	return objlook(name, &Procedures, OSPROC, enter);
}

/*
** Name:	objlook() -	Find Object in Symbol Table or DB.
**
** Description:
**	This routine looks up a procedure or frame either in its symbol table
**	or in the database.  If the symbol is in the DB or is not found, but
**	should be in the symbol table, it is entered as into the symbol table
**	(as undefined in the latter case.)
**
**	Note:  Symbol table entry is currently always forced.
**
** Input:
**	name	{char *}  The frame or procedure name.
**	parent	{OSSYM *}  Parent node for object.
**	type	{nat}  Object type.
**	enter	{bool}  Force entry into the symbol table when undefined.
**
** Returns:
**	{OSSYM *}  Symbol node for object.
**
** History:
**	02/87 (jhw) -- Written.
**	11/89 (jhw) -- Added 'type' parameter.
**	12/89 (jhw) -- Abstracted object look-up into 'obj_look()',
**		which supports system frames and procedures.
*/

static OSSYM *
objlook ( name, parent, type, enter )
char	*name;
OSSYM	*parent;
i4	type;
bool	enter;
{
    register OSSYM  *sym;
    register ACATDEP *dep = NULL;
    bool isSystem;

#if defined(LP64)
    GLOBALREF i8	osAppid;
#else /* LP64 */
    GLOBALREF OOID	osAppid;
#endif /* LP64  */

    if ((sym = ossympeek(name, parent)) == NULL)
    { /* look-up in DB */
	OOID		class;
	OOID		oclass;
	DB_DATA_VALUE	dbtype;

	dbtype.db_datatype = DB_NODT;

	if ( parent->s_kind == OSFRAME )
		oclass = OC_OSLFRAME;
	else if ( parent->s_kind == OSPROC )
		oclass = OC_OSLPROC;

	/* Look up object class and type */
	if ( obj_look(name, oclass, &class, &dbtype, &isSystem) != OK )
	{ /* not found */
		type = OSUNDEF;
		class = oclass;		/* Close enough for dependency record */
	}

	if ( Objp != NULL )
	{
		/*
		** Add a dependency record, unless it's a system-defined 
		** object or an ADF function.  If we found it in the catalogs,
		** the user is overriding any system definition.
		*/

		if ((dep = _check_dep(name, (char *)NULL, OC_DTCALL)) == NULL)
		{
			if (isSystem)
				;	
			else if (OSUNDEF == type && osckadfproc(name))
				;
			else
			{
				/* Add all dependencies as OC_DTCALL now */
				_add_dep(name, (char *)NULL, class, OC_DTCALL);
			}
		}
	}

	/* Enter in symbol table if defined or desired. */
	if ( type != OSUNDEF || enter )
	    sym = ossymput(name, type, &dbtype, parent, 0);
    }
	
    return sym;
}

/*
** Name:	sys_object() -	Look up System Object.
**
** Description:
**	Returns the named system object if it is one.
**
** Input:
**	name	{char *}  The object name.
**	class	{OOID}  The generic class of the object.
**
** Returns:
**	{APPL_COMP *}   The system object if it is one;
**		 	otherwise NULL.
**
** History:
**	12/89 (jhw) -- Written.
*/
FUNC_EXTERN APPL_COMP	*IIAMgfrGetIIamFrames();	 
FUNC_EXTERN APPL_COMP	*IIAMgprGetIIamProcs();		 
FUNC_EXTERN APPL_COMP	*IIAMgglGetIIamGlobals();	 

static APPL_COMP *
sys_object ( name, class )
char	*name;
OOID	class;
{
	register APPL_COMP	*sysp;
	char			nlower[FE_MAXNAME+1];

	if ( class == OC_OSLFRAME )
		sysp = IIAMgfrGetIIamFrames();		 
	else if ( class == OC_OSLPROC )
		sysp = IIAMgprGetIIamProcs();		 
	else if ( class == OC_GLOBAL )
		sysp = IIAMgglGetIIamGlobals();		 
	else
		return NULL;	/* No global anything elses */

    	_VOID_ STlcopy(name, nlower, sizeof(nlower)-1);
    	CVlower(nlower);
	for ( ; sysp != NULL ; sysp = sysp->next )
	{
		if ( STequal(sysp->name, nlower) )
			break;
	}
	return sysp;
}

/*
** Name:	obj_look() -	Look up Global Object.
**
** Description:
**	Looks for the named object in the database or in the system tables.
**	The class and the type description for the object will be returned
**	by reference if the object is found.
**
** Inputs:
**	name	{char *}  The object name.
**	generic	{OOID}  The generic class of the object.
**
** Outputs:
**	class	{OOID *}  The class of the object.
**	data_type	{DB_DATA_VALUE *}  The data type of the object.
**	isSystem	{bool *}	Did we find a system object
**
** Returns:
**	{STATUS}  OK if found.
**
** History:
**	12/89 (jhw) -- Abstracted from 'objlook()' and modified to support
**		system objects.
*/
static STATUS
obj_look ( name, general_class, class, data_type, isSystem )
char			*name;
OOID			general_class;
OOID			*class;
register DB_DATA_VALUE	*data_type;
bool			*isSystem;
{
	ACATREC		*ap;
	OOID		objid;
	STATUS		rval;

#if defined(LP64)
	GLOBALREF i8	osAppid;
#else /* LP64 */
	GLOBALREF OOID	osAppid;
#endif /* LP64 */

	*isSystem = FALSE;
	data_type->db_datatype = DB_NODT;

	/* Get object catalog description */
	if ( Objp == NULL
		|| (rval = IIAMoiObjId(name, general_class, osAppid, &objid,
					class)) != OK
			|| (rval = IIAMcgCatGet(osAppid, objid, &ap)) != OK )
	{ /* not in DB */
		register APPL_COMP	*sysp;

		sysp = sys_object(name, general_class);

		if ( sysp == NULL )
			return ( rval == OK ) ? FAIL : rval;
		else
		{ /* system frame, procedure, or global */
			register DB_DATA_DESC	*ret_type;

			*isSystem = TRUE;
			switch ( *class = sysp->class )
			{
			  case OC_HLPROC:
				ret_type = &((HLPROC *)sysp)->return_type;
				break;
			  case OC_OSLPROC:
				ret_type = &((_4GLPROC *)sysp)->return_type;
				break;
			  case OC_DBPROC:
				ret_type = &((DBPROC *)sysp)->return_type;
				break;

			  case OC_GLOBAL:
				ret_type = &((GLOBVAR *)sysp)->data_type;
				break;

			  case OC_CONST:
				ret_type = &((CONSTANT *)sysp)->data_type;
				break;

			  case OC_OSLFRAME:
			  case OC_OLDOSLFRAME:
			  case OC_MUFRAME:
			  case OC_APPFRAME:
			  case OC_UPDFRAME:
			  case OC_BRWFRAME:
				ret_type = &((USER_FRAME *)sysp)->return_type;
				break;

			  default:
			  case OC_RWFRAME:
			  case OC_QBFFRAME:
			  case OC_GRFRAME:
			  case OC_GBFFRAME:
				ret_type = NULL;
				break;
			}
			if ( ret_type != NULL )
			{
				data_type->db_datatype = ret_type->db_datatype;
				data_type->db_length = ret_type->db_length;
				data_type->db_prec = ret_type->db_scale;
				data_type->db_data = ret_type->db_data;
			}
		}
	}
	else
	{
		data_type->db_datatype = ap->retadf_type;
		data_type->db_length = ap->retlength;
		data_type->db_prec = ap->retscale;
		/* save class type names; otherwise ignore */
		data_type->db_data = ( data_type->db_datatype == DB_DMY_TYPE )
					? STalloc(ap->rettype) : NULL;

		*class = ap->class;

		IIUGtagFree(ap->tag);
	}

	return OK;
}

/*{
** Name:	osckadfproc() -	Check for an ADF procedure.
**
** Description:
**
** Input:
**	name	{char *}  name of the function to check.
**
** Returns:
**	{bool}  is it an ADF function.
**
** History:
*/

bool
osckadfproc (name)
char	*name;
{
	if ( !IIAFckFuncName(name) )
		return FALSE;
	return TRUE;
}

/*{
** Name:	osgl_dep() -	Enter an object-code dependency. 
**
** Description:
**	Enter a dependency on a global application component.
**
** Input:
**	name	{char *}	name of dependee object.
**	owner	{char *}	owner of dependee object.
**	class	{OOID}		the class of the dependee.
**	deptype	{OOID}		the type of the dependency.
**
** Returns:
**
** History:
**	07/89 (billc) -- Written.
**	18-may-93 (davel)
**		Add owner argument.
*/

VOID
osgl_dep (name, owner, class, deptype)
char *name;
char *owner;
OOID class;
OOID deptype;
{
	if ( Objp != NULL && _check_dep(name, owner, deptype) == NULL)
	{
		_add_dep(name, owner, class, deptype); 
	}
}

/*{
** Name:    osdepwrite() -	Write out Dependency Information to Database.
**
** Description:
**	Write out the dependency information for the frame/procedure object
**	to the object catalogs in the database.
**
** History:
**	30-sep-1985	First written (agh).
**	05/87 (jhw)  Modified to use 'IIAMcuCatUpd()'.
*/

VOID
osdepwrite ()
{
#if defined(LP64)
    GLOBALREF i8	osAppid;
#else /* LP64 */
    GLOBALREF OOID	osAppid;
#endif /* LP64 */

    if (Objp != NULL)
    {
	register ACATDEP *dep;
	register ACATDEP **p_dep;
	OO_OBJECT object;
	bool	removed;

	p_dep = &Objp->dep_on;
	dep = *p_dep;
	while (dep != NULL)
	{
	    removed = FALSE;	/* Assume we'll keep this one */

	    /* Change any OC_DTCALLs to OC_DTRCALL where necessary */
	    if (dep->deptype == OC_DTCALL)
	    {
	        OSSYM *sym;

	        sym = ossympeek(dep->name, &Frames);
	        if (sym == NULL) sym = ossympeek(dep->name, &Procedures);
	        if (sym != NULL && (sym->s_ref & OS_OBJREF) != 0)
	        {
	    	    dep->deptype = OC_DTRCALL;
	        }
	        else
	        {
		    register ACATDEP *mdep;

		    /* 
		    ** If there's a menu dependency with the same name, 
		    ** remove the call dependency.
		    */
		    for (mdep = Objp->dep_on; mdep != NULL; mdep = mdep->next)
		    {
		    	if (mdep->deptype == OC_DTMREF &&
			    STequal(dep->name, mdep->name))
			{
			    /* Remove the call dependency from the list */
			    *p_dep = dep->next;
			    removed = TRUE;
			    break;
			}
		    }
		}
	    }

	    if (!removed)
		p_dep = &dep->next;
	    dep = *p_dep;
	}

	IIUIbeginXaction();
	iiuicw1_CatWriteOn();
	object.ooid = Objp->ooid;
	if ( IIAMdpDeleteDeps( &object, OC_UNDEFINED, OC_UNDEFINED, 
				OC_UNDEFINED) != OK 
	  || IIAMadlAddDepList(Objp->dep_on, osAppid, Objp->ooid) != OK )
	{
		iiuicw0_CatWriteOff();
		IIUIabortXaction();
		osuerr(OSBUG, 1, ERx("osdepwrite()"));
	}
	iiuicw0_CatWriteOff();
	IIUIendXaction();
    }
}

/*{
** Name:	osgl_fetch() -	find global variables/constants
**
** Description:
**	Find a global variable/constant in database
**
** Input:
**	parent {OSSYM*} Symbol table to put them in.
**	applid {OOID} 	The application.
**
** Returns:
**	The new symbol.
**
** History:
**	8/89 (billc) -- Written.
**	12/89 (jhw) -- Modified to call 'obj_look()', which includes support
**		for system globals.
*/

OSSYM *
osgl_fetch (parent, name)
OSSYM *parent;
char  *name ;
{
	OOID		class;
	OSSYM		*sym;
	i4		gltype;
	i4		flags = 0;
	DB_DATA_VALUE	dbdv;
	OSSYM		*osrecdeclare();
	bool		isSystem;

	if ( Objp == NULL || 
	     obj_look(name, OC_GLOBAL, &class, &dbdv, &isSystem) != OK )
	{
		return NULL;
	}

	gltype = ( class == OC_GLOBAL ) ? OSGLOB : OSACONST;

	if ( dbdv.db_datatype == DB_DMY_TYPE )
	{
		STATUS osreclook();

		if (osreclook(dbdv.db_data) != OK)
		{
			oscerr(OSBADGLOB, 2, name, dbdv.db_data);
			return ossymundef(name, parent);
		}
		sym = osrecdeclare(parent, name, gltype, &dbdv);
	}
	else
	{
		sym = ossymput(name, gltype, &dbdv, parent, flags);
	}

	/* create a dependency on the global/const */
	if (!isSystem)
		osgl_dep(name, (char *)NULL, class, OC_DTGLOB);
	return sym;
}

/*{
** Name:	_check_dep() -	See if a given dependency already exists.
**
** Description:
**	Lookup a dependency, return TRUE if found.
**
** Input:
**	name	{char *}	name of dependee object.
**	deptype	{OOID}		the type of the dependency.
**
** Returns:
**	TRUE if found, FALSE otherwise.
**
** History:
**	xx/xx (xxxx) -- Written.
**	18-may-93 (davel)
**		Add owner argument.  Also put in conditional logic for 
**		determining a dependency match based on deptype.
*/
static ACATDEP *
_check_dep(name, owner, deptype)
char	*name;
char	*owner;
OOID	deptype;
{
	register ACATDEP	*dep;


	/* Search dependencies */
	for (dep = Objp->dep_on ; dep != NULL ; dep = dep->next)
	{
		if (dep->deptype == deptype)
		{
		    switch(deptype)
		    {
			/* goofiness. We only care about an owner match when the
			** deptype is OC_DTTABLE_TYPE; also in that case, we 
			** should do a case-sensitive compare of both the 
			** owner and the name.
			*/
			case OC_DTTABLE_TYPE:
			    if ( STcompare(name, dep->name) == 0
		  	      && ( ( owner != NULL && dep->owner != NULL 
			 		&& STcompare(owner, dep->owner) == 0 
		       		   )
		      		  || (owner == NULL && dep->owner == NULL)
		     		 )
			       )
			    {
				return dep;
			    }
			    break;

			default:
			    if ( STbcompare(name, 0, dep->name, 0, TRUE) == 0 )
			    {
				return dep;
			    }
			    break;
		    }
		}
	}
	return NULL;
}

static ACATDEP *_FreeDeps = NULL;

/*{
** Name:	_add_dep() -	Add a dependency to our internal list.
**
** Description:
**	Enter a dependency.
**
** Input:
**	name	{char *}	name of dependee object.
**	owner	{char *}	owner of dependee object.
**	class	{OOID}		the class of the dependee.
**	deptype	{OOID}		the type of the dependency.
*/
static  VOID
_add_dep(name, owner, class, deptype)
char	*name;
char	*owner;
OOID	class;
OOID	deptype;
{
	register ACATDEP	*dp;
	STATUS			stat;
	char			nlower[FE_MAXNAME+1];

	/* force lower, unless the object name is a backend object */
	if (deptype != OC_DTTABLE_TYPE)
	{
		(VOID) STlcopy(name, nlower, sizeof(nlower)-1);
		CVlower(nlower);
		name = nlower;
	}

	if (_FreeDeps != NULL)
	{
		dp = _FreeDeps;
		_FreeDeps = _FreeDeps->next;
	}
	else if ((dp = (ACATDEP*)FEtalloc(Objp->tag, sizeof(*dp), &stat)) 
		== NULL)
	{
		osuerr(OSNOMEM, 1, ERx("_add_dep()"));
	}

	/* setup dependency structure */
	dp->id = Objp->ooid;
	dp->name = FEtsalloc(Objp->tag, name);
	dp->owner = (owner == NULL) ? NULL : FEtsalloc(Objp->tag, owner);
	dp->class = class;

	/* Note:  Use return type to determine dependency type
	**	since it is an error to use the object otherwise.
	*/
	dp->deptype = deptype;

	/* insert */
	dp->deplink = ERx("");
	dp->deporder = 0;
	dp->next = Objp->dep_on;
	Objp->dep_on = dp;
}

/*{
** Name:	_free_dep() -	Free a dependency.
**
** Input:
**	dep	{ACATDEP *}	name of dependee object.
*/

static VOID
_free_dep(dep)
register ACATDEP	*dep;
{
	dep->next = _FreeDeps;
	_FreeDeps = dep;
}
