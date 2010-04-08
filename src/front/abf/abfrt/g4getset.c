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
# include       <erg4.h>
# include       "g4globs.h"


/*
** Name:	g4getset.c - Basic get and set attribute routines
**
** Description:
**	This file defines:
**
**	IIAG4gaGetAttr		Get attribute
**	IIAG4saSetAttr		Set attribute
**
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	STATUS IIARdotDotRef();

/* static's */
static STATUS get_attr_dbv
	(char *name, i4  caller, DB_DATA_VALUE *targ);

/*{
** Name:	IIAG4gaGetAttr - Get attribute
**
** Description:
**	Get an attribute.  The object we're getting the attribute for is
**	in iiAG4savedObject.
**
** Inputs:
**	isvar	i4		Pass by reference (ignored)
**	type	i4		ADF type of user's variable
**	length	i4		length of user's variable
**	name	char *		name of attribute
**
** Outputs:
**	ind	i2 *		NULL indicator
**	data	PTR		User's data area.
**
**	Returns:
**		STATUS
**
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4gaGetAttr(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, char *name)
{
    STATUS status;
    DB_DATA_VALUE dbv;
    G4ERRDEF g4errdef;

    /* First, get the attribute descriptor */
    if ((status = get_attr_dbv(name, G4GATTR_ID, &dbv)) != OK)
	return status;

    /* 
    ** Now that we have the data in a DBV, send it to 3GL.
    */
    if ((status = IIAG4get_data(&dbv, ind, type, length, data)) != OK)
    {
	g4errdef.numargs = 3;
	g4errdef.errmsg = status;
	g4errdef.args[0] = iiAG4routineNames[G4GATTR_ID];
	g4errdef.args[1] = ERget(F_G40005_ATTRIBUTE);
	g4errdef.args[2] = name;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
    }

    return status;
}

/*{
** Name:	IIAG4saSetAttr - Set attribute
**
** Description:
**	Set an attribute.  The object we're setting the attribute for is
**	in iiAG4savedObject.
**
** Inputs:
**	name	char *		name of attribute
**	ind	i2 *		NULL indicator
**	isvar	i4		Pass by reference
**	type	i4		ADF type of user's variable
**	length	i4		length of user's variable
**	data	PTR		User's data area.
**
**	Returns:
**		STATUS
**
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4saSetAttr(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data)
{
    STATUS status;
    DB_DATA_VALUE dbv;
    G4ERRDEF g4errdef;

    /* First, get the attribute descriptor */
    if ((status = get_attr_dbv(name, G4SATTR_ID, &dbv)) != OK)
	return status;

    /* 
    ** Now that we have a DBV, fill it from 3GL.  Set common g4errdef info 
    ** first; IIAG4set_data may reset this information for certain errors.
    */
    g4errdef.numargs = 3;
    g4errdef.args[0] = iiAG4routineNames[G4SATTR_ID];
    g4errdef.args[1] = ERget(F_G40005_ATTRIBUTE);
    g4errdef.args[2] = name;
    if ((status = IIAG4set_data(&dbv, ind, isvar, type, length, data, 
				&g4errdef))
	    != OK)
    {
	g4errdef.errmsg = status;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return status;
    }
	
    return status;
}

/*
** get_attr_dbv
**
** Get attribute descriptor
*/
static STATUS 
get_attr_dbv(char *name, i4  caller, DB_DATA_VALUE *targ)
{
    G4ERRDEF g4errdef;
    DB_DATA_VALUE	src;

    /* save the object in DBV form */
    IIAG4dbvFromObject(iiAG4savedObject, &src);

    /* fill target DBV. Note this will also handle the _state and _record
    ** attributes.
    */
    if ( IIARgtaGetAttr( &src, name, targ) != OK)
    {
	g4errdef.errmsg = E_G4271F_BAD_ATTR;
	g4errdef.numargs = 3;
	g4errdef.args[0] = iiAG4routineNames[caller];
	g4errdef.args[1] = name;
	g4errdef.args[2] = ERx("object");
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G4271F_BAD_ATTR;
    }
    else
    {
	return OK;
    }
}
