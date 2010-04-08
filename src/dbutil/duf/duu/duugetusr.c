/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>

#include    <cs.h>
#include    <lk.h>
#include    <dudbms.h>
#include    <duenv.h>

#include    <cv.h>
#include    <id.h>
#include    <si.h>
#include    <lo.h>
#include    <nm.h>
#include    <st.h>


/**
**
**  Name: DUUGETUSR.C -	routines used to access the user file and specific
**			records in that file.
**
**  Description:
**        This file contains all of the routines necessary to access
**	the users file, "user", located in ING_DUILITY.
**
**          du_get_usrname() -	get a user record from the user file.
**
**
**  History:    $Log-for RCS$
**      28-Sep-86 (ericj)
**          Initial creation.
**	10-apr-90 (greg)
**          Integrate fix to prevent overwriting end of usr_name
**	25-Apr-90 (teg)
**	    Back out above change, as it introduces a bug that truncates the
**	    24th character of 24 char long usernames.  Also, the 
**	    input/output sections were incorrect, so I corrected them.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	09-nov-93 (rblumer)
**	    Take out CVlower call in du_get_usrname, since we now allow
**	    upper-case and mixed-case user names.
**      31-jan-94 (mikem)
**          Added include of <cs.h> before <lk.h>.  <lk.h> now references
**          a CS_SID type.
**      21-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

/*
[@forward_type_references@]
*/


/*
**  Forward and/or External function references.
*/
/*
[@function_reference@]...
*/



/*
**  Defines of other constants.
*/

/*
**      Constants dealing with parsing a line from the "users" file.
*/
/* {@fix_me@} */
#define                 DU_NUSRFIELDS   4
#define                 DU_UFNAME	0
#define                 DU_UFGID 	1
#define                 DU_UFMID 	2
#define                 DU_UFSTAT	3

/*
[@group_of_defined_constants@]...
*/

/*
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: du_brk_usr()	-   initialize a DU_USR struct from a line obtained
**			    from the "users" file.
**
** Description:
**        This routine takes a buffer containing a line read from the "users"
**	file and breaks it up into its component parts initializing a
**	DU_USR struct which describes the user.
**
** Inputs:
**      ubuf                            Ptr to a character buffer containing
**					the line from the "users" file.  This
**					buffer should be null-terminated.
**	usr_rec				Ptr to user record to be initialized.
**
** Outputs:
**      *usr_rec
**	    .du_uname			This is set to the users name.
**	    .du_ugid			GID of the user.
**	    .du_umid			MID of the user.
**	    .du_ustatus			User status.
**	Returns:
**	    OK				Completed successfully.
**	    FAIL			Failure.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-Sep-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
STATUS
du_brk_usr(ubuf, usr_rec)
char               ubuf[];
DU_USR             *usr_rec;
{
    char                *usr_field[DU_NUSRFIELDS];
    register	i4	i;
    register	char	*p;

    for (i = 1, usr_field[0] = p = ubuf; p && i < DU_NUSRFIELDS;)
	if ((p = STchr(p, '!')) || (p = STchr(p, '\0')))
	{
	    *p++	= EOS;
	    usr_field[i++]	= p;
	}

    if (i < DU_NUSRFIELDS)
	return(FAIL);

    CVlower(usr_field[DU_UFNAME]);
    STcopy(usr_field[DU_UFNAME], usr_rec->du_uname);

    CVol(usr_field[DU_UFGID], &usr_rec->du_ugid);
    CVol(usr_field[DU_UFMID], &usr_rec->du_umid);
    CVol(usr_field[DU_UFSTAT], &usr_rec->du_ustatus);
    
    return(OK);    
}



/*
**
** Name: du_find_usr() - get line from user file based on name
**
** Description:
**	  Given a user name as a string, this routine returns the
**	corresponding line from the users file from the ING_DUILITY
**	location.
**
** Inputs:
**      name                            Name of the user.
**	ubuf				a buf to dump the line in (declare as
**					char buf[DU_MAXLINE + 1]
**
**	Returns:
**	    OK				if successful.
**	    FAIL			Failure (user not found or file couldn't
**					be opened.
**
** Side effects:
**	none
**
** History:
**	28-Sep-86 (ericj)
**	    Initial creation.
**	30-sep-91 (teresa)
**	    pick up ingres63p change 263259 from 6.4 line:
**		01-aug-91 (kirke)
**		Changed ubuf size implementation to match interface description.
*/

i4
du_find_usr(uname, ubuf)
char	*uname;
char	ubuf[];
{
    FILE		*userf;
    i2			ret_val = -1;
    register char	*bp;

    CVlower(uname);

    /* {@fix_me@} */
    /* The following call to NMf_open should be replaced with a call ot
    ** NMfu_open().  This procedure will be used on files located in
    ** ING_DUILITY, instead of NMf_open.  For every NMfu_open(), there
    ** must be an accompaning NMfu_close().
    */
#ifdef MVS
    /*
    **	The MVS version of LO and NM requires that filenames have a prefix
    **	and a suffix.
    */
    if (NMf_open("r", "users.data", &userf))
#else
    if (NMf_open("r", "users", &userf))
#endif
	ret_val = 1;

    while (ret_val < 0)
    {
	bp = ubuf;

	if ((SIgetrec(ubuf, DU_MAXLINE + 1, userf)) != OK)
	{
	    ret_val = 1;
	}
	else
	{
	    /* Skip blank lines */
	    if (ubuf[0] == EOS)
		continue;

	    if ((bp = STchr(ubuf, '!')) == NULL)
	    {
		ret_val = 1;
	    }
	    else
	    {
		*bp = '\0';
		CVlower(ubuf);
		if (STcompare(ubuf, uname) == 0)
		{
		    *bp = '!';

		    /* the code does not expect a newline. */
		    if (bp = STchr(ubuf, '\n'))
		    {
			*bp = EOS;
		    }
		    ret_val = 0;
		}
	    }
	}
    }	    /* end of while stmt */

    return(ret_val == OK ? OK : FAIL);
}


/*{
** Name: du_get_usrname()  - get a user name from the environment.
**
** Description:
**        This routine gets the Ingres user name from the process
**	running this procedure.
**
** Inputs:
**      usr_name                        Character buffer that will contain name
**					of user to be looked up. (declare as
**					char buf[DU_USR_SIZE + 1]
**
** Outputs:
**	usr_name			Puts null terminated string with user
**					name into user supplied buffer.
**	Returns:
**	    VOID
**
** Side Effects:
**	    none.
**
** History:
**      28-Sep-86 (ericj)
**          Initial creation.
**	10-apr-90 (greg)
**          Integrate fix to prevent overwriting end of usr_name
**	25-Apr-90 (teg)
**	    Back out above change, as it introduces a bug that truncates the
**	    24th character of 24 char long usernames.  Also, the 
**	    input/output sections were incorrect, so I corrected them.
**	09-nov-93 (rblumer)
**	    Take out CVlower call, since we now allow upper-case and mixed-case
**	    user names.
[@history_template@]...
**/
VOID
du_get_usrname(usr_name)
char		    *usr_name;
{

    IDname(&usr_name);

    usr_name[DU_USR_SIZE]   = EOS;

    (VOID) STtrmwhite(usr_name);

    return;
}
