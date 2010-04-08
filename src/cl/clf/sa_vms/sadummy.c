/*
**    Copyright (c) 1994, 2008 Ingres Corporation
*/

/*
** Dummy SA routines, these will compile an any machine, when you have coded
** a version of SA for your platform add it's code to the list below
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <tm.h>
# include    <me.h>
# include    <st.h>
# include    <tr.h>
# include    <sacl.h>

/*
** Name: SA.C - contains all routines for the SA sub-library
**
** Description:
**	Routines for the SA sub-library, these routines are designed to
**	provide an interface to the operating system for the management of
**	the operating system audit trail. Initialy the only operations
**	supported will be:
**	    SAopen - Open an audit trail
**	    SAclose - Close an audit trail
**	    SAflush - Flush buffered records to the operating system audit trail**	    SAwrite - Write a record to the operating system audit trail.
** History:
**	7-jan-94 (stephenb)
**	    Initialy created as stubs.
**	28-feb-94 (stephenb)
**	    Update to current SA spec.
**	15-mar-94 (ajc)
**	    Added hp8_bls port specific stuff
**	9-may-94 (robf)
**          - Moved headers so sadummy.c would compile, need to have tm.h
**	      before sacl.h.
**	    - Added me.h, required for MEreqmem.
**	    - Correctly coerce the sa_desc values in STcompare, before we
**	      were trying to get the member (sa_desc) of a PTR, which doesn't
**	      exist.
**	21-jun-94 (wolf) 
**          ifdef in SAsupports_osaudit() is missing the endif.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
*/
static i4 SAtrim( char    *string,
		   i4     length);
STATUS SAflush();


/*
** Name: SAsupports_osaudit - does SA support writing to operating system audits
**
** Description:
**	This routine determins wether SA support writing of operating
**	system audit logs, in the simpilest case this routine can be hard coded
**	to return TRUE if all other SA routines have been coded and tested on
**	this platform or FALSE if they haven't.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	TRUE or FALSE
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation.
*/
bool
SAsupports_osaudit()
{
# ifdef SATEST
    return (TRUE);
# else
    return (FALSE);
# endif
}

/*
** Name: SAopen - open an operating system audit trail
**
** Description:
**	Open an operating system audit trail. Initialy this routine will
**	only support the writing of audit records (which are appended to
**	the end of the trail), thus the caller may only open the trail in
**	one mode, SA_WRITE. A descriptor will be returned from this routine
**	which may be used to refference the audit trail in future calls to
**	SA. The default operating system audit trail will be the only
**	supported trail, and this case the parameter aud_desc will be NULL.
**
** Inputs:
**	aud_desc		description of the audit trail to open
**	flags			flags to use when opening the audit trail
**
** Outputs:
**	aud_trail_d		descriptor to use for this audit trail
**	err_code		pointer to a variable used to return OS errors
**
** Returns:
**	OK              if operation succeeded
** 	SA_NOACCESS     user has no access to this audit trail
**	SA_BADPARAM     bad parameter values were passed to the routine
**	SA_NOPRIV       the user does not have privileges to read audit trails
**	SA_NOSUPP       the operation is not supported by this implimentation of**			SA.
**	FAIL            the operation failed
**
** History:
**	7-jan-94 (stephenb)
**	    Initialy created as a stub.
**	28-feb-94 (stephenb)
**	    Update to current SA spec.
*/
STATUS
SAopen( char            *aud_desc,
	i4         flags,
	PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    CL_CLEAR_ERR(err_code);
    /*
    ** Alocate Memory for descriptor
    */
    *aud_trail_d = MEreqmem(0, sizeof(SA_DESC), TRUE, (STATUS *) NULL);
    if (*aud_trail_d == NULL)
	return (SA_NOMEM);
    STcopy("OS", ((SA_DESC *)*aud_trail_d)->sa_desc);

    return (OK);
}

/*
** Name: SAclose - close an operating system audit trail
**
** Description:
**	This rouitne closes an operating system audit trail previously opened
**	by SAopen(). The routine should check that the given trail has been
**	opened before attempting to close it, it will also call SAflush()
**	to flush all outstanding audit records to the audit trail.
**
** Inputs:
**	aud_trail_d	descriptor that identifies this audit trail
**
** Outputs:
**	err_code	pointer to a variable used to return OS errors
**
** Returns:
**	OK              if operation succeeded
**	SA_NOOPEN       the audit trail decsribed by this descriptor has not
**			been opened
**	SA_BADPARAM     bad parameter values were passed to the routine
**	SA_NOSUPP       the operation is not supported by this implimentation of
**			SA.
**	FAIL            the operation failed
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation as a stub
**	28-feb-94 (stephenb)
**	    Updated to current SA spec.
*/
STATUS
SAclose(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    STATUS	cl_stat;

    CL_CLEAR_ERR(err_code);
    /*
    ** Check that the audit trail is open, this is just a dummy check
    ** that the stub routine SAopen() has been called.
    */
    if (STcompare(((SA_DESC*)*aud_trail_d)->sa_desc, "OS"))
	return (SA_NOOPEN);
    /*
    ** Call SAflush() to flush outstanding records
    */
    cl_stat = SAflush(*aud_trail_d, err_code);
    if (cl_stat != OK)
	return (cl_stat);

    /*
    ** Re-claim memory
    */
    cl_stat = MEfree(*aud_trail_d);
    if (cl_stat != OK)
	return (SA_MFREE);
    else
	return (OK);
}

/*
** Name - SAwrite - write a record to the operating system audit trail
**
** Description:
**	This routine writes an audit record to the operating system audit trail.
**	It is preffereable though not compulsory for this routine to return 
**	asynchronously, a subsequent call to SAflush() will hand all 
**	outstanding audits to the operating system for auditing, if SAwrite() 
**	is synchronous then SAflush() will be a no-op routine. SAwrite() may 
**	buffer a number of audit records for performance before writing, if 
**	the caller wishes to guarantee writes a subsequent call to SAflush() 
**	must be used. SAwrite() should attempt, if possible, to write an
**	additional field, other than those passed in SA_AUD_REC, to the 
**	operating system audit trail, that will uniquely identify the record 
**	as an Ingres audit record. The audit trail will be identified by it's 
**	descriptior passed back	from SAopen().
**	
** Inputs:
**	aud_trail_d     a descriptor that identifies this audit trail
**	aud_rec         the audit record to be written
**
** Outputs:
**	err_code        pointer to a variable used to return OS errors.
**
** Returns:
**	OK              if operation succeeded
**	SA_NOOPEN       the audit trail decsribed by this descriptor has not
**			been opened
**	SA_NOWRITE      this audit trail may not be written to
**	SA_BADPARAM     bad parameter values were passed to the routine
**	SA_NOSUPP       the operation is not supported by this implimentation of
**			SA.
**	FAIL            the operation failed
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation using TRdisplay for testing.
**	28-feb-94 (stephenb)
**	    Updated to current SA spec.
*/
STATUS
SAwrite(PTR             *aud_trail_d,
	SA_AUD_REC      *aud_rec,
	CL_ERR_DESC     *err_code)
{
    /*
    ** This is a "quick and dirty routine" which just re-formats
    ** the audit record and TRdisplay's it
    */
    SYSTIME	stime;
    struct	TMhuman	time;
    char	*date;

    CL_CLEAR_ERR(err_code);
    /*
    ** Check that the audit trail is open, this is just a dummy check
    ** that the stub routine SAopen() has been called.
    */
    if (STcompare(((SA_DESC*)*aud_trail_d)->sa_desc, "OS"))
	return (SA_NOOPEN);

    /*
    ** Get a human readable date string
    */
    TMnow(&stime);
    _VOID_ TMbreak(&stime, &time);
    _VOID_ STpolycat(7, time.wday, time.day, time.month, time.year,
	time.hour, time.mins, time.sec, &date);
    /*
    ** add null terminators to blank filled strings  (this will fail
    ** if the string already uses all 32 charaters).
    */
     _VOID_ SAtrim(aud_rec->sa_ruserid, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_euserid, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_dbname, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_userpriv, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_objpriv, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_objowner, GL_MAXNAME);
     _VOID_ SAtrim(aud_rec->sa_objname, GL_MAXNAME);
    /*
    ** Send the lot to TRdisplay, not all fields are displayed, just
    ** enough to get the idea
    */
    TRdisplay("%s | %s | %s | %s | %s | %s\n", &date,
	aud_rec->sa_accesstype, aud_rec->sa_eventtype, aud_rec->sa_objname, 
	aud_rec->sa_ruserid, aud_rec->sa_dbname);
    
    return (OK);
}

/*
** Name: SAflush - flush buffered audit records to the OS audit trail
**
** Description:
**	This routine ensures that all buffered audits for a specific audit 
**	trail are handed to the operating system for audit, the routine should 
**	wait for all system audit calls to be completed before returning. If 
**	SAwrite() returns synchronously then this routine will be a no-op since
**	there is never anything to do. The audit trail will be identified by 
**	it's descriptor passed back from SAopen().
**
** Inputs:
**	aud_trail_d	a descriptor that identifies this audit trail
**
** Outputs:
**	err_code	pointer to a variable used to return OS errors
**
** Returns:
**	        OK              if the operation succeeded
**		SA_NOOPEN       the audit trail decsribed by this descriptor 
**				has not been opened
**		SA_BADPARAM     bad parameter values were passed to the routine
**		SA_NOSUPP       the operation is not supported by this 
**				implimentation of SA.
**		FAIL            the operation failed
**
** History:
**	11-jan-94 (stephenb)
**	    Initial creation as a stub.
**	28-feb-94 (stephenb)
**	    Updated to current SA spec.
*/
STATUS
SAflush(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    CL_CLEAR_ERR(err_code);
    /*
    ** Since SAwrite() just uses TRdisplay() currently, this routine
    ** has no function
    */
    /*
    ** Check that the audit trail is open, this is just a dummy check
    ** that the stub routine SAopen() has been called.
    */
    if (STcompare(((SA_DESC*)*aud_trail_d)->sa_desc, "OS"))
	return (SA_NOOPEN);

    return (OK);
}

/*
** Name: SAtrim - Internal routine used to trim white space from a char string
**
** Description:
**	This routine takes a single word char string (single word being a
**	number of non-space, non-null printable characters terminated by a
**	space or spaces) and null terminates it. The difference between this
**	and the routines in ST is that the passed string will have a fixed
**	maximum length, and will not necesarily be null terminated. If the end
**	of the string is reached before a blank is found the string length
**	is returned and no NULL terminator is placed on the string.
**
** Inputs:
**	string		- single word char string
**	length		- maximum length of the string
**
** Outputs:
**	string		- null terminated word
**
** Returns:
**	0 if blank was found, string length otherwhise.
** History:
**	19-jan-94 (stephenb)
**	    Initial creation.
**	11-may-94 (stephenb)
**	    Updated to search backwards from the end of the string for the
**	    first non-blank, thus allowing embeded blanks.
**	16-aug-1996 (kamal03)  integrated UNIX changes
**		12-mar-1996 (sweeney)
**	    Make static to match its declaration.
*/
static i4
SAtrim( char	*string,
	i4	length)
{
    i4		i;
    char	*x;

    for (i = 0, x = string + length -1; i < length; i++, x--)
    {
	if (*x != ' ')
	{
	    *(x + 1) = NULL;
	    break;
	}
    }
    if (i >= length -1)
	return (length);
    else
	return (0);
}

