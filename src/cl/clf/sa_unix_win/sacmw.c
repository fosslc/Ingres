/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** This is Sun CMW specific code, only compile if we are on that port
** (otherwhise we will use the routines in sadummy.c unless another SA has 
** been written).
*/
# include <bzarch.h>
# ifdef su4_cmw

# ifdef _REENTRANT
# error Changes for reentrancy have not been tested (tutto01)
# endif

# include    <pwd.h>
# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <tm.h>
# include    <sacl.h>
# include    <st.h>
# include    <me.h>
# include    <cs.h>
# include    <bsm/audit.h>
# include    <bsm/audit_stat.h>
# include    <bsm/auditwrite.h>

/* External variables */

/*
** Name: SA.C - contains all routines for the SA module
**
** Description:
**	Routines for the SA module, these routines are designed to
**	provide an interface to the operating system for the management of
**	the operating system audit trail. Initialy the only operations
**	supported will be:
**
**	    SAopen - Open an audit trail
**	    SAclose - Close an audit trail
**	    SAflush - Flush buffered records to the operating system audit trail**	    SAwrite - Write a record to the operating system audit trail.
** History:
**	7-jan-94 (stephenb)
**	    Initialy created as stubs.
**	9-feb-94 (stephenb)
**	    Written for CMW.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static i4  SAtrim( char    *string,
		   i4      length);

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
**	9-feb-94 (stephenb)
**	    Coded for Sun CMW.
**	28-feb-94 (stephenb)
**	    Removed auditstat() we will know if auditing is realy not working
**	    properly from the first auditwrite().
*/
bool
SAsupports_osaudit()
{
    	return (TRUE);
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
**	On Sun CMW we do not need to open the audit trail for writing, but we 
**	will need to return an audit trail descriptor. Since SA_WRITE is 
**	currently the only valid flag, and we only support writing to one audit
**	trail we will just fill the descriptor with the string "OS_CURRENT".
**	At this time we will also turn on queueing of audit records.
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
**	SA_NOMEM	No memory available for SA.
**	SA_NOOPEN	Could not open the audit trail.
**
** History:
**	7-jan-94 (stephenb)
**	    Initialy created as a stub.
**	9-feb-94 (stephenb)
**	    Coded for Sun CMW.
*/
STATUS
SAopen( char            *aud_desc,
	i4         flags,
	PTR	        *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    i4	syserr;
    u_i4	qsize;
    char	*qstring;

    err_code->errnum = SA_OK;
    /*
    ** Check flags
    */
    if (flags & ~SA_WRITE)
	return (SA_BADPARAM);

    /*
    ** Turn on audit queueing, each audit record is approximately
    ** 650 bytes maximum, the current system imposed maximum queue size 
    ** appears to be 32K bytes, so we'll set it to that as a default allowing
    ** 50 records to be written before the queue fills and flushes. This value
    ** will be left configurable with the use of II_AUDIT_QSIZE just incase
    ** it needs to be changed, but ***BEWARE*** seting this value too low will
    ** cause this routine to continualy block the server to flush records, and
    ** server threads will appear to hang.
    */
    NMgtAt("II_AUDIT_QSIZE", &qstring);
    if (qstring == NULL || *qstring == NULL)
	qsize = 32768;
    else if (CVan(qstring, &qsize) != OK)
	qsize = 32768;
    syserr = auditwrite(AW_QUEUE, qsize, AW_END);
    if (syserr)
    {
	_VOID_ TRdisplay(
		"SAopen: Cannot initiate audit queueing, system error = %d\n", 
		aw_errno);
	err_code->errnum = aw_errno;
	return (SA_NOOPEN);
    }
    /*
    ** Turn on the trusted server option, we will be constructing the
    ** subject and label fields ourselves
    */
    syserr = auditwrite(AW_SERVER, AW_END);
    if (syserr)
    {
	_VOID_ TRdisplay("SAopen: Cannot enable trusted server option, system error = %d\n", aw_errno);
	err_code->errnum = aw_errno;
	return (SA_NOOPEN);
    }
    /*
    ** Allocate memory for descriptor
    */
	
    *aud_trail_d = MEreqmem(0, sizeof(SA_DESC), TRUE, (STATUS *) NULL);
    if (*aud_trail_d == NULL)
	return (SA_NOMEM);
    STcopy("OS_CURRENT", ((SA_DESC *)*aud_trail_d)->sa_desc);

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
**	On Sun CMW no open or close of the current audit trail is required,
**	since this is the only audit trail we currently support we just
**	check that we have the descriptor for this audit trail (set by SAopen)
**	and the release the memory and return. We will also turn off
**	audit record queueing and trusted server options at this point.
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
**	SA_MFREE	Unable to free allocated memory.
**	SA_NOCLOSE	Unable to close the audit trail
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation as a stub
**	9-feb-94 (stephenb)
**	    Coded for Sun CMW.
**	28-feb-94 (stephenb)
**	    Remove call to SAflush(), the auditwrite() call will flush the
**	    queue anyway.
*/
STATUS
SAclose(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    STATUS	cl_stat;
    i4	syserr;

    err_code->errnum = SA_OK;
    /*
    ** Check parameters
    */
    if (*aud_trail_d == NULL)
	return (SA_BADPARAM);
    /*
    ** Check that the audit trail is open
    */
    if (STcompare(((SA_DESC *)*aud_trail_d)->sa_desc, "OS_CURRENT"))
	return (SA_NOOPEN);

    /*
    ** Turn off audit record queueing and trusted server options
    ** NOTE: the AW_NOQUEUE operation will automatically flush the current
    ** queue so in this implimentation there is no need to call SAflush().
    */
    syserr = auditwrite(AW_NOQUEUE, AW_END);
    if (syserr)
    {
	_VOID_ TRdisplay("SAclose: cannot disable audit queueing, system error = %d\n", aw_errno);
	err_code->errnum = aw_errno;
	return (SA_NOCLOSE);
    }
    syserr = auditwrite(AW_NOSERVER, AW_END);
    if (syserr)
    {
	_VOID_ TRdisplay("SAclose: Cannot disable trsuted server option, system error = %d\n", aw_errno);
	err_code->errnum = aw_errno;
	return (SA_NOCLOSE);
    }
    /*
    ** Free memory
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
**	The Sun CMW implimentation of this routine will use the queueing
**	mechanism provided with auditwrite() to impliment an asynchrounous
**	routine. Calls to this routine will simply place the audit record
**	on the queue (which exists in the callers address space) and return.
**	A subsequent call to SAflush will flush the queue to the kernel.
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
**	SA_NOWRITE      Failed to write to the audit trail.
**	SA_BADPARAM     bad parameter values were passed to the routine
**	SA_NOSUPP       the operation is not supported by this implimentation of
**			SA.
**	SA_NOMEM	Memory could not be allocated for SA.
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation using TRdisplay for testing.
**	10-feb-94 (stephenb)
**	    Coded for Sun CMW.
*/
STATUS
SAwrite(PTR             *aud_trail_d,
	SA_AUD_REC      *aud_rec,
	CL_ERR_DESC     *err_code)
{
    i4		syserr;
    u_i4		egid;
    u_i4		euid;
    char		stat;
    STATUS		local_stat = OK;
    struct passwd	*ruserinfo;
    char            	pwnam_buf[sizeof(struct passwd)];
    struct passwd	*euserinfo;
    char		*inst_code;
    char		temp[GL_MAXNAME+1]; /* temp buffer used to turn blank
					    ** pad strings into null terminated
					    ** strings
					    */

    err_code->errnum = SA_OK;
    /*
    ** Check parameters.
    */
    if (*aud_trail_d == NULL)
	return (SA_BADPARAM);
    /*
    ** Check that the audit trail is open.
    */
    if (STcompare(((SA_DESC *)*aud_trail_d)->sa_desc, "OS_CURRENT"))
	return (SA_NOOPEN);

    for (;;) /* something to break out of */
    {
	temp[GL_MAXNAME] = NULL;
	MEcopy(aud_rec->sa_ruserid, GL_MAXNAME, temp);
	_VOID_ SAtrim(temp, GL_MAXNAME); 
	/*
	** Construct subject information
	*/
        ruserinfo = iiCLgetpwnam(temp, &pwnam_buf, sizeof(struct passwd));
	if (ruserinfo == NULL)
	{
	    /* can't write audit without this */
	    local_stat = SA_NOWRITE;
	    break;
	}
	
	MEcopy(aud_rec->sa_euserid, GL_MAXNAME, temp);
	_VOID_ SAtrim(temp, GL_MAXNAME);
        euserinfo = iiCLgetpwnam(temp, &pwnam_buf, sizeof(struct passwd));
	if (euserinfo == NULL)
	{
	    /* 
	    ** Not fatal, we could be using an effective ingres user that is
	    ** not an OS user
	    */
	    egid = 0;
	    euid = 0;
	}
	else
	{
	    egid = euserinfo->pw_gid;
	    euid = euserinfo->pw_uid;
	}
	if (aud_rec->sa_status)
		stat = -1;
	else
		stat = 0;
	/*
	** Get installation code
	*/
	NMgtIngAt("II_INSTALLATION", &inst_code);
	
	/*
	** Write the audit record:
	**
	** Due to an apparant CMW bug we have to use a multi-shot write,
	** appending each field to the record with another auditwrite() call
	** (messy) and then writing the record at the end.
	*/
    	/*
	** The blank filled strings need to be null terminated for CMW
	** but extra spaces don't matter, so we just copy all
	** GL_MAXNAME characters into a temp buffer which we'll null
	** terminate.
    	*/
	for (;;) /* to break from */
	{
	    au_termid_t		dummy_term;

	    syserr = auditwrite(
	    	AW_EVENT, /* Manditory CMW event, make this an Ingres event */
		    "ACMW_ingres",
	    	AW_TEXT, /* Installation code */
		    inst_code,
	    	AW_APPEND, AW_END);
	    if (syserr)
		break;
	    syserr = auditwrite(
	    	AW_SUBJECT, /* user info */
		    (au_id_t) ruserinfo->pw_uid, euid, egid, ruserinfo->pw_uid,
		    ruserinfo->pw_gid, 0, 0, &dummy_term,
	    	AW_APPEND, AW_END);
	    if (syserr)
		break;
	    if (aud_rec->sa_sec_label)
	    {
	    	syserr = auditwrite(
	    	    AW_SLABEL, /* Sensitivity label */
		    &aud_rec->sa_sec_label->cmw_label.bcl_sensitivity_label,
		    AW_APPEND, AW_END);
	    	if (syserr)
		    break;
	    	syserr = auditwrite(
	    	    AW_ILABEL, /* information label */
		    &aud_rec->sa_sec_label->cmw_label.bcl_information_label,
		    AW_APPEND, AW_END);
	    	if (syserr)
		    break;
	    }
	    syserr = auditwrite(
	    	AW_TEXT, /* access type */
		    aud_rec->sa_accesstype,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    syserr = auditwrite(
	    	AW_TEXT, /* type of event */
		    aud_rec->sa_eventtype,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    syserr = auditwrite(
	    	AW_TEXT, /* audit message */
		    aud_rec->sa_messtxt,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    MEcopy(aud_rec->sa_dbname, GL_MAXNAME, temp);
	    syserr = auditwrite(
	    	AW_TEXT, /* database name */
		    temp,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    syserr = auditwrite(
	    	AW_RETURN, /* status of action */
		    stat, 0,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    MEcopy(aud_rec->sa_userpriv, GL_MAXNAME, temp);
	    syserr = auditwrite(
	    	AW_TEXT, /* user privileges */
		    temp,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    MEcopy(aud_rec->sa_objpriv, GL_MAXNAME, temp);
	    syserr = auditwrite(
	    	AW_TEXT, /* object privileges */
		    temp,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    MEcopy(aud_rec->sa_objowner, GL_MAXNAME, temp);
	    syserr = auditwrite(
	    	AW_TEXT, /* object owner */
		    temp,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    MEcopy(aud_rec->sa_objname, GL_MAXNAME, temp);
	    syserr = auditwrite(
	    	AW_TEXT, /* object name */
		    temp,
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    if (aud_rec->sa_detail_txt)
	    {
	        syserr = auditwrite(
	    	    AW_TEXT, /* additional detail text */
		    	aud_rec->sa_detail_txt,
	   	    AW_APPEND, AW_END);
	        if (syserr)
		    break;
	    }
	    syserr = auditwrite(
	    	AW_DATA, /* additional detail integer */
		AWD_DECIMAL, AWD_INT, 1, &(aud_rec->sa_detail_int),
		AW_APPEND, AW_END);
	    if (syserr)
		break;
	    if (aud_rec->sa_sess_id)
	    {
	    	syserr = auditwrite(
	    	    AW_TEXT, /* Ingres session ID */
		    	aud_rec->sa_sess_id,
		    AW_APPEND, AW_END);
	    	if (syserr)
		    break;
	    }
	    syserr = auditwrite(AW_WRITE, AW_END);
	    break;
	}
	if (syserr)
	{
	    TRdisplay("SAwrite: Failed to write audit record due to system error %d\n", aw_errno);
	    err_code->errnum = aw_errno;
	    local_stat = SA_NOWRITE;
	    break;
	}

    	break;
    }
    /*
    ** Handle errors
    */
    if (local_stat != OK)
	return (local_stat);
    else
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
**	On Sun CMW this will just call auditwrite() with the flush option
**	to flush the queue.
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
**		SA_NOFLUSH	Unable to flush audit records.
**
** History:
**	11-jan-94 (stephenb)
**	    Initial creation as a stub.
**	11-feb-94 (stephenb)
**	    Coded for Sun CMW.
*/
STATUS
SAflush(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
    i4	syserr;

    err_code->errnum = SA_OK;
    /*
    ** Check parameters.
    */
    if (*aud_trail_d == NULL)
	return (SA_BADPARAM);
    /*
    ** Check that the audit trail is open
    */
    if (STcompare(((SA_DESC *)*aud_trail_d)->sa_desc, "OS_CURRENT"))
	return (SA_NOOPEN);
    /*
    ** Flush the outstanding records
    */
    syserr = auditwrite(AW_FLUSH, AW_END);
    if (syserr)
    {
	TRdisplay("SAflush: Unable to flush the audit queue due to system error %d\n",aw_errno);
	err_code->errnum = aw_errno;
	return (SA_NOFLUSH);
    }

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
**	    Updated to search backwars until first non-blank, thus allowing
**	    strings to have embeded blanks.
*/
i4
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
# endif /* su4_cmw */
