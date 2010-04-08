/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		 
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<osfiles.h>
#include	<oserr.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<iltypes.h>
#include	<oslconf.h>
#include	<osfrs.h>
#include	<ilops.h>

/**
** Name:    osfrs.c -	OSL Translator Generate Set/Inquire FRS
**					Statement Module.
** Description:
**	Contains routines that interface with the EQUEL Set/Inquire FRS
**	interpretation sub-module to generate IL code to set and inquire FRS
**	parameters.  Defines:
**
**	frs_head()	generate frs set/inquire statement header il code.
**	frs_style()	determine style of set/inquire frs statement.
**	frs_iqset()	set frs inquire type.
**	osfrs_setval()	return reference to frs set value.
**	osfrs_constant()
**	frs_gen()	generate il code for frs set/inquire statement
**	frs_error()	equel set/inquire frs module error interface.
**	frs_insrow_with	set up for an insertrow with clause.
**
** History:
**	Revision 5.0  86/10/17	 16:40:15  wong
**	Initial revision
**
**	Revison 6.0  87/04  wong
**	Added support for `old' (3.0) style syntax.
**
**	Revison 6.4
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT
**
**	Revision 6.5
**	29-jul-92 (davel)
**		Added logic to set length and precision in osconstalloc() call.
**	03-feb-93 (davel)
**		Added frs_insrow_with() to support the new with clause in
**		insertrow and loadtable statments. Also hacked out special
**		(wrong) logic for row object type in frs_gen(). (bug 49379)
**	26-mar-93 (davel)
**		Fix bug 49379 - remove special (wrong) logic for 
**		row object type in frs_gen(). (this fix already in 6.5)
**	19-May-1993 (fredv)
**		Fix bug 51792 - fixed a coding error in frs_gen() and 
**			added code to generate constant. Otherwise, the
**			4gl statements like:
**				set_forms frs (map(frskey16) = 'controlL'); 
**			        set_forms frs (label(frskey16) = '^L');
**			would return "SET/INQUIRE MAP or LABEL" error messages.
**      09-Jan-96 (fanra01)
**          Changed extern to GLOBALREF for frs and added reference to DLL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need oslconf.h to satisfy gcc 4.3.
*/

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF FRS__	frs;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF FRS__	frs;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/*{
** Name:    frs_head() -	Generate FRS Set/Inquire Statement Header
**					IL Code.
** Description:
**	Generates header IL code for an FRS set/inquire statement.  This
**	routine interacts with the EQUEL Set/Inquire FRS interpretation module
**	through the structures set up by 'frs_object()' and 'frs_parentname()'.
**
** Side Effects:
**	Fills in 'frs' structure.
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Added backwards compatibility for old-style set/inquires.
*/

VOID
frs_head ()
{
    register FRS_OBJ	*fo = frs.f_obj;
    register FRS_NAME	*fn = &frs.f_row;
    register i4	i;
    ILALLOC		par[FRSmaxNAMES];

    if (frs.f_err)
	return;

    if (frs.f_old)
    { /* old-style */
	par[0] = par[1] = par[2] = 0;
	for (i = 0, fn = frs.f_pnames ; i < frs.f_argcnt ; ++i, ++fn)
	{
	    par[i] = ((fn->fn_var != NULL)
			? (ILALLOC)((OSSYM *)fn->fn_var)->s_ilref
			: IGsetConst(DB_CHA_TYPE, fn->fn_name)
		     );
	}
	IGgenStmt((frs.f_mode == FRSset ? IL_OSETFRS :IL_OINQFRS), (IGSID*)NULL,
	    4, par[0], par[1], par[2], IGsetConst(DB_CHA_TYPE, fo->fo_name)
	);
    }
    else
    { /* new-style */
	ILALLOC row = 0;

	frs.f_old = FALSE;

	/* Add row number - zero in most cases */

	if (fn->fn_var != NULL)
	    row = (ILALLOC)((OSSYM *)fn->fn_var)->s_ilref;
	else if (fn->fn_name != NULL && *fn->fn_name != '0')
	    row = IGsetConst(DB_INT_TYPE, fn->fn_name);

	par[0] = par[1] = 0;
	/* assert:  fo->fo_args == fo->f_argcnt  <==> no errors */
	for (i = 0, fn = frs.f_pnames ; i < fo->fo_args ; ++i, ++fn)
	{
	    par[i] = ((fn->fn_var != NULL)
			? (ILALLOC)((OSSYM *)fn->fn_var)->s_ilref
			: IGsetConst(DB_CHA_TYPE, fn->fn_name)
		     );
	}
	IGgenStmt((frs.f_mode == FRSset ? IL_SETFRS : IL_INQFRS), (IGSID *)NULL,
	    4, par[0], par[1], row, fo->fo_def /* add object constant */
	);
    }
}

/*{
** Name:    frs_style() -	Determine Style of Set/Inquire FRS Statement.
**
** Description:
**	Determines if the semantics of a set/inquire FRS statement
**	are old (3.0) style or not.
**
** Side Effects:
**	Sets 'f_old' in FRS structure, 'frs'.
**
** History:
**	04/87 (jhw) -- Written.
*/
VOID
frs_style ()
{
    register FRS_OBJ	*fo = frs.f_obj;

    if (frs.f_err)
	return;

    /*
    ** Old-style ==> fo->fo_old && frs.f_argcnt > fo->fo_args
    ** New-style ==> frs.f_argcnt == fo->fo_args
    ** Error ==> (fo->fo_old && frs.f_argcnt < fo->fo_args) ||
    **		  (!fo->fo_old && frs.f_argcnt != fo->fo_args)
    */
    if (frs.f_argcnt > FRSmaxNAMES  ||
		(frs.f_argcnt != fo->fo_args &&
			(!fo->fo_old || frs.f_argcnt < fo->fo_args)) )
    { /* error! */
	char	buf[16];

	CVna( fo->fo_args, buf );
	frs_error( E_EQ0158_fsOBJARG, EQ_ERROR, 2, fo->fo_name, buf );
	frs.f_err = TRUE;
    }
    else if (fo->fo_old && frs.f_argcnt > fo->fo_args)
    { /* old-style */
	frs.f_old = TRUE;
	frs_error( E_EQ0078_grSTYUNSUPP, EQ_WARN, 1,
		frs.f_mode == FRSset ? ERx("SET_FORMS") : ERx("INQUIRE_FORMS")
	);
    }
    else
    { /* new-style */
	frs.f_old = FALSE;
    }
}

/*{
** Name:    osfrs_old() -	Return Whether Old-Style FRS Statement.
**
** Description:
**	Returns TRUE if the FRS set/inquire statement was recognized as
**	being in the old-style syntax.
**
** Returns:
**	{bool}	TRUE if old-style.
**
** History:
**	04/87 (jhw) -- Written.
*/
bool
osfrs_old ()
{
    return (bool)(frs.f_old);
}

/*{
** Name:    frs_iqset() -	Set FRS Inquire Type.
*/
VOID
frs_iqset (sym)
register OSSYM	*sym;
{
    frs_iqsave(sym->s_name, osfrs_basetype(sym->s_type));
}

/*{
** Name:    osfrs_setval() -	Return Reference to FRS Set Value.
**
** Description:
**	Checks FRS set value type and returns reference to the value.
**
**	Expects `new' style only.
**
** History:
**	08/86 (jhw) -- Written.
*/
ILALLOC
osfrs_setval (value)
register OSNODE *value;
{
    register FRS_CONST	*fc = frs.f_const;
    ILALLOC		ref = 0;
    OSSYM		*sym;

    char	*frsck_mode();
    char	*iiIG_string();
    OSSYM	*ostmpalloc();
    OSSYM	*osnodesym();

    if (frs.f_err)
	return ref;

    sym = osnodesym(value);

    if (osfrs_basetype(value->n_type) != fc->fc_type[FRSset] &&
		fc->fc_def != FsiMAP)
    { 
	/* error */
	bool	var = !tkIS_CONST_MACRO(value->n_token);
	char	buf[64];
	char	*cp;

	if (value->n_token == VALUE)
	{
	    if (value->n_tfref == NULL)
		cp = sym->s_name;
	    else
	    {
		cp = STpolycat(3, value->n_tfref->s_parent->s_name, ERx("."),
			value->n_tfref->s_name, buf);
	    }
	}
	else if (tkIS_CONST_MACRO(value->n_token))
	{
	    cp = value->n_const; 
	}
	else /* assume complex type. */
	{
		cp = sym->s_name;
	}

	frs_error(fc->fc_type[FRSset] == T_INT
			? (var ? E_EQ0061_grINTVAR : E_EQ0059_grINT)
			: (var ? E_EQ0067_grSTRVAR : E_EQ0066_grSTR),
		EQ_ERROR, 1, cp
	);
	ref = 0;
    }
    else if (value->n_token == VALUE 
      || value->n_token == ARRAYREF
      || value->n_token == DOT)
    {
	ref = sym->s_ilref;
    }
    else
    { /* constant */
	register char	*str;
	i4		type;
	i4		length;
	i2		prec = 0;
	if (fc->fc_def == FsiMAP)
	{
	    char	*valp;
	    char	buf[16];

	    if (value->n_token == tkID)
		valp = value->n_value;
	    else if (value->n_token == tkSCONST)
		valp = value->n_const;
	    else
		valp = NULL;
	    CVna(frsck_valmap(valp), buf);
	    str = iiIG_string(buf);
	    type = DB_INT_TYPE;
	    length = sizeof(i4);
	}
	else if (fc->fc_def == FsiFMODE)
	{
	    str = frsck_mode(ERx("SET_FORMS"), value->n_const);
	    type = DB_CHA_TYPE;
	    length = value->n_length;
	}
	else
	{
	    str = value->n_const;
	    type = value->n_type;	/* Note:  constant types are actual */
	    length = value->n_length;
	    prec = value->n_prec;
	}
	ref = (ILALLOC)osconstalloc(type, length, prec, str)->s_ilref;
    }
    ostrfree(value);

    return ref;
}

/*{
** Name:    osfrs_constant() -	Make FRS Constant Node.
**
** Description:
**	Creates an FRS constant node for a set/inquire constant specification.
**
** Input:
**	constname	{char *}  The name of the FRS constant.
**	object		{OSNODE *}  Node describing object for which the
**				    constant is to be applied.
**
** Returns:
**	{OSNODE *}	The FRS constant node.
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Added support for `old' (3.0) style syntax.
*/
OSNODE *
osfrs_constant (constname, object)
char		*constname;
register OSNODE *object;
{
    register OSNODE	*np;

    OSNODE	*ostralloc();

    if (frs.f_old)
    { /* Old (3.0) style */
	np = osmkconst(tkSCONST, constname);
    }
    else
    { /* New style */
	if (frs.f_err)
	    np = NULL;
	else
	{
	    i4	    type;
	    char    *objname;
	    bool    var;
    
	    if ((np = ostralloc()) == NULL)
	    {
		osuerr(OSNOMEM, 1, ERx("osfrs_constant"));
		return (OSNODE *)NULL;
	    }
    
	    if (object == NULL)
	    {
		type = T_NONE;
		objname = NULL;
		var = FALSE;
	    }
	    else
	    {
		type = osfrs_basetype(object->n_type);
		if (object->n_token == tkID)
		    objname = object->n_value;
		else if (object->n_token == tkSCONST ||
				object->n_token == tkICONST)
		    objname = object->n_const;
		else
		    objname = NULL;
		var = object->n_token == VALUE;
	    }
    
	    np->n_token = FRSCONST;
	    np->n_type = DB_NODT;
	    np->n_frscode =
		frsck_constant(constname, objname, type, var, &np->n_frsval);
	    np->n_objref =
		(np->n_frsval >= 0 || object == NULL) ? 0 : osvalref(object);
    
	}
	ostrfree(object);
    }
    return np;
}

/*{
** Name:    frs_gen() -	Generate IL Code for FRS Set/Inquire Statement
**					List Element.
** Description:
**	Generate IL code for an FRS constant node.
**
** Input:
**	frsp	{OSNODE *}  FRS constant ("FRSCONST") node.  (Or, "TLASSIGN"
**				for `old' style support.)
**
** History:
**	09/86 (jhw) -- Written.
**	04/87 (jhw) -- Added support for `old' (3.0) style syntax.
*/

VOID
frs_gen (frsp)
register OSNODE *frsp;
{
    char	*iiIG_string();

    if (frsp->n_token == TLASSIGN)
    { /* Old (3.0) style */
	IGgenStmt(IL_TL2ELM, (IGSID *)NULL, 2, frsp->n_coln, frsp->n_tlexpr);
    }
    else if (frsp->n_token == FRSCONST)
    { /* New style */
	ILREF	ref;

	ref = ((frs.f_mode == FRSset) ? frsp->n_setref : frsp->n_inqvar->s_ilref);
        if (frsp->n_frsval >= 0)
        {
           char        buf[16];
 
           CVna(frsp->n_frsval, buf);
           IGgenStmt(IL_INQELM, (IGSID *)NULL, 4, ref, 0,
                   IGsetConst(DB_INT_TYPE, iiIG_string(buf)),
                   frsp->n_frscode);
        }
	else
	   IGgenStmt(IL_INQELM, (IGSID *)NULL,
		4, ref, frsp->n_objref, 0, frsp->n_frscode);
    }
    else
	osuerr(OSBUG, 1, ERx("osfrs"));

    ostrfree(frsp);
}

/*{
** Name:    frs_error() -	EQUEL Set/Inquire FRS Module Error Interface.
**
** Description:
**	This routine provides an error reporting interface for the EQUEL
**	Set/Inquire FRS module interpretation sub-module.
**
** Parameters:
**	errnum		{ER_MSGID}  The EQUEL error number.
**	severity	{nat}  The error severity.
**	nargs		{nat}  The number of arguments.
**	er0 ... er4	{PTR}  The arguments.
**
** Side Effects:
**	Increments warning or error counts.
**
** History:
**	08/86 (jhw) -- Written.
**	07/89 (jhw) -- Modified to call 'osReportErr()'.
*/

VOID
frs_error (errnum, severity, nargs, er0, er1, er2, er3, er4)
ER_MSGID	errnum;					 
i4		severity;
i4		nargs;
PTR		er0, er1, er2, er3, er4;
{
	GLOBALREF i4	osErrcnt;
	GLOBALREF i4	osWarncnt;
	GLOBALREF bool	osWarning;

	ER_MSGID	er_sever;

	if ( severity != EQ_WARN )
		++osErrcnt;
	else if ( !osWarning )
		return;
	else
		++osWarncnt;

	if ( severity == EQ_WARN )
		er_sever = _WarningSeverity;
	else if ( severity == EQ_ERROR )
		er_sever = _ErrorSeverity;
	else if ( severity == EQ_FATAL )
		er_sever = _FatalSeverity;

	osReportErr(errnum, er_sever, nargs, er0, er1, er2, er3, er4);
}
VOID
frs_insrow_with(void)
{
	frs_inqset(FRSset);
	frs_object(ERx("row"));
	frs.f_old = FALSE;
	frs.f_err = 0;
	return;
}
