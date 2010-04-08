/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<afe.h>
# include	<abfcnsts.h>

/**
** Name:	abinqset.c	- run-time support of INQUIRE_4GL and SET_4GL
**				  statements.
**
** Description:
**	This file defines:
**
**	IIARt4gTranslate4GLConstant	Translate constant name to code.
**	IIARi4gInquire4GL 		INQUIRE_4GL support
**	IIARs4gSet4GL			SET_4GL support
**
** History:
**	10-feb-93 (davel)
**		Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN DB_STATUS	afe_cvinto();
FUNC_EXTERN i4		IIARictInquireClearTarget();
FUNC_EXTERN void	IIARsctSetClearTarget();

/* static's */

/*{
** Name:	IIARt4gTranslate4GLConstant - 	translate SET & INQUIRE_4GL 
**						constant names.
** Description:
**	Return internal INQUIRE_4GL integer constant for the specified 
**	constant name.  This is used for both INQUIRE_4GL and SET_4GL.
**
** Inputs:
**	char*		attr		name of attribute
**
** Outputs:
**
** Returns:
**	i4	SET/INQUIRE_4GL code (see abfcnsts.h for code defines)
**
** History:
**	10-feb-93 (davel)
**		Initial version.
**	09-mar-93 (connie)
**		complete the error messages from E_AR0086 & E_AR0087
**	09-jul-93 (mgw)
**		Make the constant name (attr) comparison case insensitive.
*/
i4
IIARt4gTranslate4GLConstant (attr)
char *attr;
{
    i4		code = AB_INQ4GL_invalid;

    if (STbcompare(attr, 0, ERx("clear_on_no_rows"), 0, TRUE) == 0)
    {
	code = AB_INQ4GL_clear_on_no_rows;
    }

    return code;
}

/*{
** Name:	IIARi4gInquire4GL	- run-time support for INQUIRE_4GL
**
** Description:
**	Return data for the various possible inquiries.
**
** Inputs:
**	char*		attr		name of attribute
**	DB_DATA_VALUE*	dbv		DBV to place the result.
**			.db_datatype
**			.db_length
**			.db_prec
** Outputs:
**	DB_DATA_VALUE*	dbv		DBV to place the result.
**			.db_data
**
** Returns:
**
** History:
**	10-feb-93 (davel)
**		Initial version.
*/
void
IIARi4gInquire4GL (attr, dbv)
char		*attr;
DB_DATA_VALUE	*dbv;
{
    STATUS		status;
    i4			code;
    DB_DATA_VALUE	ldbv;
    i4			intout;
    ADF_CB 		*cb = FEadfcb();

    /* translate name into the INQ4GL constant */
    code = IIARt4gTranslate4GLConstant(attr);

    /* setup local DBV based on the code (almost everything is integer) */
    switch(code)
    {
	case AB_INQ4GL_invalid:
	    /* raise an error */
	    IIUGerr(E_AR0086_Invalid_Inq4gl_cnst, 0, 2, attr,
		ERx("INQUIRE_4GL") );
	    return;

	case AB_INQ4GL_clear_on_no_rows:
	default:
	    ldbv.db_datatype = DB_INT_TYPE;
	    ldbv.db_prec = 0;
	    ldbv.db_length = 4;
	    ldbv.db_data = (PTR)&intout;
	    break;
    }

    /* Do the right thing for each inquiry */
    switch (code)
    {
	case AB_INQ4GL_clear_on_no_rows:
	    intout = IIARictInquireClearTarget();
	    break;

	default:
	    /* raise an error */
	    IIUGerr(E_AR0086_Invalid_Inq4gl_cnst, 0, 2, attr,
		ERx("INQUIRE_4GL") );
	    return;
    }

    /* convert our local dbv to the result dbv */
    if ((status = afe_cvinto(cb, &ldbv, dbv)) != OK)
    {
	/* raise an error */
	IIUGerr(E_AR0087_Unmatched_datatype, 0, 3,
		ERx("field"), attr, ERx("INQUIRE_4GL") );
    }

    return;
}

/*{
** Name:	IIARs4gSet4GL	- run-time support for SET_4GL
**
** Description:
**	Set various ABF run-time behaviours.
**
** Inputs:
**	char*		attr		name of attribute
**	DB_DATA_VALUE*	dbv		DBV which contains the value to set
**
** Outputs:
**
** Returns:
**
** Side-effects:
**	may set various run-time settings (e.g. the "clear_on_no_rows" run-time
**	behavior setting).
**
** History:
**	10-feb-93 (davel)
**		Initial version.
*/
void
IIARs4gSet4GL (attr, dbv)
char		*attr;
DB_DATA_VALUE	*dbv;
{
    STATUS		status;
    i4			code;
    DB_DATA_VALUE	ldbv;
    i4			intout;
    ADF_CB 		*cb = FEadfcb();

    /* translate name into the INQ4GL constant */
    code = IIARt4gTranslate4GLConstant(attr);

    /* setup local DBV based on the code (almost everything is integer) */
    switch(code)
    {
	case AB_INQ4GL_invalid:
	    /* raise an error */
	    IIUGerr(E_AR0086_Invalid_Inq4gl_cnst, 0, 2, attr,
		ERx("SET_4GL") );
	    return;

	case AB_INQ4GL_clear_on_no_rows:
	default:
	    ldbv.db_datatype = DB_INT_TYPE;
	    ldbv.db_prec = 0;
	    ldbv.db_length = 4;
	    ldbv.db_data = (PTR)&intout;
	    break;
    }

    /* convert the specified value into our local dbv */
    if ((status = afe_cvinto(cb, dbv, &ldbv)) != OK)
    {
	/* raise an error */
	IIUGerr(E_AR0087_Unmatched_datatype, 0, 3,
		ERx("value"), attr, ERx("SET_4GL") );
	return;
    }

    /* Do the right thing for each inquiry */
    switch (code)
    {
	case AB_INQ4GL_clear_on_no_rows:
	    IIARsctSetClearTarget(intout);
	    break;

	default:
	    /* raise an error */
	    IIUGerr(E_AR0086_Invalid_Inq4gl_cnst, 0, 2, attr,
		ERx("SET_4GL") );
	    return;
    }

    return;
}
