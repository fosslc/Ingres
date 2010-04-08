/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <erg4.h>
# include       "rts.h"
# include       "g4globs.h"
# include       <st.h>


/**
** Name:	g4inqset.c	- EXEC 4GL INQUIRE and SET statements
**
** Description:
**	This file defines:
**
**	IIAG4i4Inq4GL		EXEC 4GL INQUIRE_4GL
**	IIAG4s4Set4GL		EXEC 4GL SET_4GL
**
** History:
**	22-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    iiarCcnClassName isn't bool.  Fix.
**/

/* # define's */
# define MSGMASK 0xffff		/* Mask out message class */

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	STATUS	iiarArAllCount();

/* static's */


/*{
** Name:	IIAG4i4Inq4GL	- EXEC 4GL INQUIRE_4GL
**
** Description:
**	Return data for the various possible inquiries.  Note that some 
**	parameters are unused for some inquiries.
**
** Inputs:
**	i4	isvar		data passed by reference?
**	type	i4		ADF type of user's variable
**	length	i4		size of user's variable
**	i4	object		object to inquire about
**	code	i4		Which inquire to perform
**
** Outputs:
**	ind	i2 *		User's null indicator
**	data	PTR		User's data buffer
**
**	Returns:
**		STATUS
**
** History:
**	22-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	25-Aug-1993 (fredv)
**		Included <st.h>.
*/
STATUS
IIAG4i4Inq4GL
(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, i4 object, i4  code)
{
    STATUS		status;
    G4ERRDEF		g4errdef;
    i4			access;
    DB_DATA_VALUE	dbv, odbv;
    i4			intout;
    i4		count, dcount;
    AB_TYPENAME		buf;

    /* Check object, if it applies */
    switch(code)
    {
	case G4IQallrows:
	case G4IQlastrow:
	case G4IQfirstrow:
	    access = G4OA_ARRAY;
	    break;

	case G4IQisarray:
	case G4IQclassname:
	    access = G4OA_OBJECT;
	    break;

	default:
	    access = G4OA_INVALID;
    }
    if (access != G4OA_INVALID)
    {
	if ((status = IIAG4chkobj(object, access, 0, G4INQUIRE_ID)) != OK)
	    return status;
    }

    /* save the object in DBV form */
    IIAG4dbvFromObject(iiAG4savedObject, &odbv);

    /* Set up the output  DBV.  Almost everything is integer */
    switch (code)
    {
	case G4IQclassname:
	case G4IQerrtext:
	    dbv.db_datatype = DB_CHA_TYPE;
	    dbv.db_prec = 0;
	    break;

	default:
	    dbv.db_datatype = DB_INT_TYPE;
	    dbv.db_prec = 0;
	    dbv.db_length = 4;
	    dbv.db_data = (PTR)&intout;
	    break;
    }

    /* Do the right thing for each inquiry */
    switch (code)
    {
	case G4IQerrtext:
	    dbv.db_data = iiAG4errtext;
	    dbv.db_length = STlength(iiAG4errtext);
	    break;

	case G4IQerrno:
	    intout = iiAG4errno & MSGMASK;
	    break;
	
	case G4IQallrows:
	    (void) iiarArAllCount( &odbv, &count, &dcount );
	    intout = count + dcount;
	    break;

	case G4IQlastrow:
	    (void) iiarArAllCount( &odbv, &count, &dcount );
	    intout = count;
	    break;

	case G4IQfirstrow:
	    (void) iiarArAllCount( &odbv, &count, &dcount );
	    intout = 1 - dcount;
	    break;

	case G4IQisarray:
	    intout = (iiarIarIsArray( &odbv ) ? 1 : 0);
	    break;

	case G4IQclassname:
	    iiarCcnClassName( &odbv, buf, FALSE );
	    dbv.db_data = (PTR)buf;
	    dbv.db_length = STlength(buf);
	    break;
    }

    if ((status = IIAG4get_data(&dbv, ind, type, length, data)) != OK)
    {
	g4errdef.errmsg = status;
	g4errdef.numargs = 3;
	g4errdef.args[0] = iiAG4routineNames[G4INQUIRE_ID];
	g4errdef.args[1] = iiAG4routineNames[G4INQUIRE_ID];
	g4errdef.args[2] = iiAG4inqtypes[code];
	IIAG4semSetErrMsg(&g4errdef, TRUE);
    }

    return status;
}

/*{
** Name:	IIAG4s4Set4GL	- EXEC 4GL SET
**
** Description:
**	Return data for the various possible sets.  Note that some 
**	parameters are unused for some inquiries.
**
** Inputs:
**	object	i4		object to inquire about
**	code	i4		Which inquire to perform
**	ind	*i2 		User's null indicator
**	isvar	i4		data passed by reference	
**	type	i4		ADF type of user's variable
**	length	i4		size of user's variable
**	data	PTR		User's data buffer
**
**	Returns:
**		STATUS
**
** History:
**	22-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4s4Set4GL
(i4 object, i4  code, i2 *ind, i4  isvar, i4  type, i4  length, PTR data)
{
    STATUS status;
    G4ERRDEF g4errdef;
    DB_DATA_VALUE dbv;
    i4 intin;

    /* 
    ** Currently, the object parameter is never used.  If, in the future,
    ** some types of SET require it, it should be checked here.
    */

    /* Set up our DBV.  */
    switch (code)
    {
	case G4STmessages:
	    dbv.db_datatype = DB_INT_TYPE;
	    dbv.db_prec = 0;
	    dbv.db_length = 4;
	    dbv.db_data = (PTR)&intin;
	    break;
    }

    /*
    ** Now that we have a DBV, fill it from 3GL.  Set common g4errdef info
    ** first; IIAG4set_data may reset this information for certain errors.
    */
    g4errdef.numargs = 3;
    g4errdef.args[0] = iiAG4routineNames[G4SET_ID];
    g4errdef.args[1] = iiAG4routineNames[G4SET_ID];
    g4errdef.args[2] = iiAG4settypes[code];
    if ((status = IIAG4set_data( &dbv, ind, isvar, type, length, data, 
					&g4errdef)) != OK)
    {
	g4errdef.errmsg = status;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return status;
    }

    /* Do the right thing for each set */
    switch (code)
    {
	case G4STmessages:
	    iiAG4showMessages = intin;
	    break;
    }

    return status;
}
