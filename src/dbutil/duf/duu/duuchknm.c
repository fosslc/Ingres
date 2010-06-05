/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <cm.h>

#include    <st.h>

/**
**
**  Name: DUUCHKNM.C -    routines to check the validity of names used
**			    by the system utilities.
**
**  Description:
**        This module contains routines used by the system utilities to
**	check the validity of various name types such as user's names
**	or database's names.
**
**          du_chk1_dbname()	- validate the syntax of a database name.
**	    du_chk2_usrname()	- validate the syntax of a user's name.
**	    du_chk3_locname()	- validate the syntax of a location name.
**
**
**  History:    
**      08-Sep-86 (ericj)
**          Initial creation.
**      24-Nov-87 (ericj)
**          Replace CH calls with CM calls so that this can be used on Unix.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	20-sep-2004 (devjo01)
**	    Remove du_chk3_locname.  Use cui_chk3_lockname from cuid.c instead.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: du_chk1_dbname() -	check the syntax of a database name.
**
** Description:
**        This routine checks the syntactical correctness of a database name.
**	A valid database name is not more than 9 characters long, begins
**	with an alphabetic character, and contains only alphanumerics.
**	Underscore is not allowed.  Note, this routine has the side effect
**	of lower-casing the string passed in.
**
** Inputs:
**      dbname                          Ptr to a buffer containing dbname.
**
** Outputs:
**	*dbname				Ptr to buffer containing lower-cased
**					dbname.
**	Returns:
**	    OK				If the name was valid.
**	    FAIL			If the name was invalid.
**
** Side Effects:
**	  Lower cases the database name being checked.
**
** History:
**      08-Sep-86 (ericj)
**          Initial creation.
**      16-Dec-88 (teg)
**          The CL routine CMnmchar changed its behavior and started returning
**	    TRUE for some non alphanumeric characters (@,$,!).  So, I modified
**	    this routine to stop using CMnmchar and to check explicately for
**	    alpha, digit and underscore.
**		24-May-99 (kitch01)
**		   Bug 91063. Function has been moved to cuid.c so that backend programs
**		   may also check the validity of database names.
[@history_line@]...
[@history_template@]...
*/
STATUS
du_chk1_dbname(dbname)
char               *dbname;
{
	return cui_chk1_dbname(dbname);
}



/*{
** Name: du_chk2_usrname() -	check user name syntax.
**
** Description:
**        This routine checks the validity of a username's syntax.  This
**	routine has the side effect of lower casing the user's name.
**
** Inputs:
**      uname                           Ptr to a buffer containing the user's
**					name.
**
** Outputs:
**      *uname                          Lower cased user's name.
**	Returns:
**	    OK			If a valid user name.
**	    FAIL			If an invalid user name.
**
** Side Effects:
**	      Lower cases the user's name.
**
** History:
**      08-Sep-86 (ericj)
**          Initial creation.
**	02-Jun-89 (teg)
**	    Fix bug 6379 (use same criteria as accessdb to check user name
**	    syntax)
** 	4-Nov-1993 (jpk)
**	    DELIM_IDENT: usernames can now be anything.  If delimited,
**	    "Mack The Knife" is legal.  Even <Larry "Bud" Melman> is legal.
**	    Only test we can perform is for size.  Fixed bug in that test:
**	    DU_USR_SIZE = sizeof (struct DB_OWN_NAME), could be greater
**	    than DB_MAXNAME, which is what we want.
**		
[@history_template@]...
*/

i4
du_chk2_usrname(uname)
register char	*uname;
{

    if ((STlength(uname) > DB_OWN_MAXNAME) || (STlength(uname) == 0))
	return(FAIL);

    return(OK);
}
