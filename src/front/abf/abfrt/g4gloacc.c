/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<erg4.h>
# include	<ooclass.h>
# include	"g4globs.h"


/**
** Name:	g4gloacc.c	- Access to 4GL globals from 3GL
**
** Description:
**	This file defines:
**
**	IIAG4ggGetGlobal	- Get a global constant or variable
**	IIAG4sgSetGlobal	- Set a global variable
**
** History:
**	22-dec-92 (davel)
**		Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	STATUS IIARgglGetGlobal ( char *name, DB_DATA_VALUE **gdbv,
					  i4  *kind );
/* static's */


/*{
** Name:	IIAG4ggGetGlobal - Get a global variable
**
** Description:
**	Look up the variable name, and, if it exists, move it to
**	the user's variable.  If the global is a complex object, then
**	IIAG4get_data() will add it to the Known Object stack.
**
** Inputs:
**	i4		isvar		Passed by reference (ignored)
**	type		i4		ADF type of user variable
**	length		i4		data size of user variable
**	name		char *		variable or constant name
**	gtype		i4		Is it a variable or a constant?
**
** Outputs:
**	ind		i2 *		NULL indicator
**	data		PTR		user variable pointer
**
**	Returns:
**		
**
** History:
**	22-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4ggGetGlobal 
(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, char *name, i4  gtype)
{
    DB_DATA_VALUE *dbdv;
    STATUS status;
    i4  caller = (gtype == G4GT_CONST ? G4GETCONST_ID : G4GETVAR_ID);
    G4ERRDEF g4errdef;
    i4  kind;

    CLEAR_ERROR;

    /* First, look up the name.  */
    if ((status = IIARgglGetGlobal(name, &dbdv, &kind)) != OK)
    {
	g4errdef.errmsg = E_G42717_NO_GLOBAL;
	g4errdef.numargs = 3;
	g4errdef.args[0] = iiAG4routineNames[caller];
	g4errdef.args[1] = ERget((caller == G4GETCONST_ID) ? 
				F_G40003_CONSTANT : 
				F_G40004_VARIABLE);
	g4errdef.args[2] = name;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	status =  E_G42717_NO_GLOBAL;
    }
    else if ( (kind == OC_CONST && gtype == G4GT_CONST)
	   || (kind == OC_GLOBAL && gtype == G4GT_VAR)
	    )
    {
	/* we found the right king of global; try to convert to the user's
	** variables.
	*/
	status = IIAG4get_data(dbdv, ind, type, length, data);

	if (status != OK)
	{
	    g4errdef.errmsg = status;
	    g4errdef.numargs = 3;
	    g4errdef.args[0] = iiAG4routineNames[caller];
	    g4errdef.args[1] = ERget((caller == G4GETCONST_ID) ? 
				F_G40003_CONSTANT : 
				F_G40004_VARIABLE);
	    g4errdef.args[2] = name;
	    IIAG4semSetErrMsg(&g4errdef, TRUE);
	}
    }
    else
    {
	/* We found the wrong kind of global.  Issue an error. */
	g4errdef.errmsg = E_G42718_WRONG_GLOBAL_TYPE;
	g4errdef.numargs = 4;
	g4errdef.args[0] = iiAG4routineNames[caller];
	g4errdef.args[1] = name;
	g4errdef.args[2] = ERget((caller == G4GETCONST_ID) ?
				    F_G40004_VARIABLE : 
				    F_G40003_CONSTANT);
	g4errdef.args[3] = ERget((caller == G4GETCONST_ID) ? 
				    F_G40003_CONSTANT : 
				    F_G40004_VARIABLE);
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	status = E_G42718_WRONG_GLOBAL_TYPE;
    }

    return status;
}

/*{
** Name:	IIAG4sgSetGlobal - Set a global variable
**
** Description:
**	Look up the variable name, and, if it exists, move data to it from
**	the user's variable.  If it's an object, loook it up in the bag of
**	known objects.
**
** Inputs:
**	name		char *		variable or constant name
**	ind		i2		NULL indicator
**	isvar		i4		passed by reference
**	type		i4		ADF type of user variable
**	length		i4		data size of user variable
**	data		PTR		user variable pointer
**
**	Returns:
**		
**
** History:
**	22-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4sgSetGlobal(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data)
{
    DB_DATA_VALUE *dbdv;
    STATUS status;
    G4ERRDEF g4errdef;
    i4  kind;

    CLEAR_ERROR;

    /* First, look up the name.  */
    if ((status = IIARgglGetGlobal(name, &dbdv, &kind)) != OK)
    {
	g4errdef.errmsg = E_G42717_NO_GLOBAL;
	g4errdef.numargs = 3;
	g4errdef.args[0] = iiAG4routineNames[G4SETVAR_ID];
	g4errdef.args[1] = ERget(F_G40004_VARIABLE);
	g4errdef.args[2] = name;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	status =  E_G42717_NO_GLOBAL;
    }
    else if ( kind == OC_GLOBAL)
    {
	/* we found the right king of global; try to convert to the user's
	** variables. Set common g4errdef info first; IIAG4set_data may reset 
 	** this information for certain errors.
	*/
	g4errdef.numargs = 3;
	g4errdef.args[0] = iiAG4routineNames[G4SETVAR_ID];
	g4errdef.args[1] = ERget(F_G40004_VARIABLE);
	g4errdef.args[2] = name;
	status = IIAG4set_data(dbdv, ind, isvar, type, length, data, &g4errdef);
	if (status != OK)
	{
	    g4errdef.errmsg = status;
	    IIAG4semSetErrMsg(&g4errdef, TRUE);
	}
    }
    else
    {
	/* We found the wrong kind of global.  Issue an error. */
	g4errdef.errmsg = E_G42718_WRONG_GLOBAL_TYPE;
	g4errdef.numargs = 4;
	g4errdef.args[0] = iiAG4routineNames[G4SETVAR_ID];
	g4errdef.args[1] = name;
	/* infer that the global was a constant and not a variable */
	g4errdef.args[2] = ERget(F_G40003_CONSTANT);
	g4errdef.args[3] = ERget(F_G40004_VARIABLE);
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	status = E_G42718_WRONG_GLOBAL_TYPE;
    }

    return status;
}
