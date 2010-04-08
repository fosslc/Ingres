/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <erg4.h>
# include "g4globs.h"
# include "rts.h"


/*
** Name:	g4chkobj.c - Check object for validity
**
** Description:
**	This file defines:
**
**	IIAG4chkobj 	- check object handle for validity
**
** History:
**	16-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need rts.h to satisfy gcc 4.3.
*/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN 	STATUS	IIARarrArrayRef();

/* static's */

/*{
** Name:	IIAG4chkobj      - check object handle for validity
**
** Description:
**	Given a handle passed in from a 3GL procedure, see if it refers to
**	a valid object.   Depending on the access type required, the object may
**	be required to be an array or a valid row of an array.
**
**	For ABF, the object handle is an index into an array of known objects,
**	managed in the g4utils.c routines.  We simply check that the specified
**	index is in range.
**
** Inputs:
**	handle		i4	object handle
**	access		i4	access required:	
**	index		i4	array index, if a row is being accessed
**	caller		i4	Who our caller is.  This affects how we
**				report errors.
**
** Outputs:
**	none
**
**	Returns:
**		OK	if object is valid
**
** Side Effects:
**	iiAG4savedObject is set if handle is valid.
**
** History:
**	16-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	08-apr-93 (davel)
**		Fix bug 51076 - pass iiarIarIsArray a DB_DATA_VALUE* rather
**		than a PTR.
**	07-sep-93 (donc) Bug 54518
**		Routine was calling IIARarrArrayRef with a miscast 1st
**		argument. Modified to call with a dbdv, not PTR.
*/
STATUS
IIAG4chkobj(i4 handle, i4  access, i4  index, i4  caller)
{
    PTR	optr;
    DB_DATA_VALUE  odbv;
    STATUS retval;
    i2     *state;
    G4ERRDEF errparms;

    retval = OK;

    /* 
    ** If we're called for GET ATTRIBUTE or SET ATTRIBUTE, initialize
    ** the error state, since we're being called directly from 3GL.
    */
    if (caller == G4GATTR_ID || caller == G4SATTR_ID)
    {
       CLEAR_ERROR;
    }

    /* See if it's a valid handle */
    if (IIAG4gkoGetKnownObject(handle, &optr) != OK)
    {
	if (caller > 0)
	{
	    errparms.errmsg = E_G42711_BADVALUE;
	    errparms.numargs = 2;
	    errparms.args[0] = (PTR)iiAG4routineNames[caller];
	    errparms.args[1] = (PTR)ERget(F_G40001_OBJECT);
	    IIAG4semSetErrMsg(&errparms, TRUE);
	}
	return E_G42711_BADVALUE;
    }

    /* OK, it's an object.  What else should we check? */
    switch (abs(access))
    {
	case G4OA_ARRAY:
	case G4OA_ROW:
	    /* make a temp DBV for the complex object */
	    IIAG4dbvFromObject(optr, &odbv);

	    /* It can't be the NULL object */
	    if (optr == NULL)
	    {
		retval = E_G42710_NULLOBJECT;
	    }

	    /* Also, it must be an array */
	    else if (!iiarIarIsArray(&odbv))
	    {
		if (caller > 0)
		{
		    errparms.errmsg = E_G42712_NOTARRAY;
		    errparms.numargs = 1;
		    errparms.args[0] = (PTR)iiAG4routineNames[caller];
		    IIAG4semSetErrMsg(&errparms, TRUE);
		}
		return E_G42712_NOTARRAY;
	    }
	    /* And, if we're checking rows, it must be a valid row */
	    else if (abs(access) == G4OA_ROW)
	    {
		i4 lindex = index;
		i4	flags = 0;	/* IIARarrArrayRef() flags - none */
		DB_DATA_VALUE targ;

		targ.db_datatype = DB_DMY_TYPE;
		targ.db_length = 4;
		targ.db_prec   = 0;

		if (IIARarrArrayRef( &odbv, lindex, &targ, flags ) != OK)
		{	
		    errparms.errmsg = E_G42713_BADINDEX;
		    errparms.numargs = 2;
		    errparms.args[0] = (PTR)iiAG4routineNames[caller];
		    errparms.args[1] = (PTR)&lindex;
		    IIAG4semSetErrMsg(&errparms, TRUE);
		    return E_G42713_BADINDEX;
		}
		optr = targ.db_data;
	    }
	    break;

	default:
	/* Nothing else to check */
	break;
    }

    /* 
    ** If our caller won't accept the
    ** NULL object, output an error.
    */
    if (retval == OK && optr == NULL && access > 0)
	retval = E_G42710_NULLOBJECT;

    if (retval == E_G42710_NULLOBJECT)
    {
	    errparms.errmsg = E_G42710_NULLOBJECT;
	    errparms.numargs = 1;
	    errparms.args[0] = (PTR)iiAG4routineNames[caller];
	    IIAG4semSetErrMsg(&errparms, TRUE);
	    return E_G42710_NULLOBJECT;
    }

    if (retval == OK)
    {
	iiAG4savedObject = optr;
        iiAG4savedAccess = abs(access);
    }

    return retval; 
}
