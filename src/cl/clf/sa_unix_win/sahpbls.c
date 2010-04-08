/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** This is HP BLS specific code, only compile if we are on that port
** (otherwhise we will use the routines in sadummy.c unless another SA has 
** been written).
*/
# include <bzarch.h>
# ifdef hp8_bls

# include	<pwd.h>
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<tm.h>
# include	<sacl.h>
# include	<st.h>
# include	<me.h>
# include 	<gv.h>
# include	<sys/security.h>
# include	<sys/audit.h>

/*
** Cache the installation code rather than getting it al time from SAwrite
*/

static char		*inst_code;
static char		*audit_string;

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
**	    SAflush - Flush buffered records to the operating system 
**                    audit trail
**	    SAwrite - Write a record to the operating system audit trail.
** History:
**	01-mar-94 (ajc)
**	     Created. 
**	20-apr-98 (mcgem01)
**	    Product name change to Ingres.
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
**      01-mar-94 (ajc)
**           Created.
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
**	On HP BLS we do not need to open the audit trail for writing, but we 
**	will need to return an audit trail descriptor. Since SA_WRITE is 
**	currently the only valid flag, and we only support writing to one audit
**	trail we will just fill the descriptor with the string "OS_CURRENT".
**	At this time we will also turn on queueing of audit records. Also
**	have to formulate dummy argc and argv for set_auth_parameters().
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
**	SA_MFREE	Unable to free allocated memory.
**
** History:
**      01-mar-94 (ajc)
**           Created.
*/
STATUS
SAopen( char            *aud_desc,
	i4         flags,
	PTR	        *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
	char *audit_argv[2];
	int audit_argc = 0;
	int cl_stat = 0;

	/*
	** Formulate dummy argc and argv for set_auth_parameters()
        */
	audit_argv[0] = MEreqmem(0, 100, TRUE, (STATUS *) NULL);
	if (audit_argv[0] == NULL)
                return (SA_NOMEM);

	STprintf(audit_argv[0], 
		"\nIngres/Enhanced Security %s %s\tAHPBLS_ingres\n",
		GV_ENV, GV_VER);
	audit_argc++;

	audit_argv[1] = MEreqmem(0, 100, TRUE, (STATUS *) NULL);
	if (audit_argv[1] == NULL)
                return (SA_NOMEM);

	STcopy("iidbms server", audit_argv[1]);
        audit_argc++;

	set_auth_parameters(audit_argc, audit_argv);

	/*
	** Allocate memory for descriptor
	*/

	*aud_trail_d = MEreqmem(0, sizeof(SA_DESC), TRUE, (STATUS *) NULL);
	if (*aud_trail_d == NULL)
		return (SA_NOMEM);
	
	STcopy("OS_CURRENT", ((SA_DESC *)*aud_trail_d)->sa_desc);

	/*
	** Get installation code
	*/
	NMgtIngAt("II_INSTALLATION", &inst_code);

	/*
	** Get the memory audit string.
	*/
	audit_string = MEreqmem(0, 1024, TRUE, (STATUS *) NULL);
	if (audit_string == NULL)
                return (SA_NOMEM);

	cl_stat = MEfree(audit_argv[0]);
	if (cl_stat != OK)
		return (SA_MFREE);

	cl_stat = MEfree(audit_argv[1]);
        if (cl_stat != OK)
                return (SA_MFREE);

		
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
**	On HP BLS no open or close of the current audit trail is required,
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
**      01-mar-94 (ajc)
**           Created.
*/
STATUS
SAclose(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
	STATUS	cl_stat;
	i4	syserr;

	err_code->errno = SA_OK;
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
	** Free memory
	*/

	cl_stat = MEfree(*aud_trail_d);
	if (cl_stat != OK)
		return (SA_MFREE);

	cl_stat = MEfree(audit_string);
        if (cl_stat != OK)
		return (SA_MFREE);

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
**	The HP BLS implementation of this routine will construct 
**	the audit string then use the audit_subsystem(3) call to write
**	the message to /dev/auditw which will pass on the message to the
**	audit daemon. This is almost asynchronous (saves using a slave).
**	
** Inputs:
**	aud_trail_d     a descriptor that identifies this audit trail
**	aud_rec         the audit record to be written
**
** Reentrant:
**	No, uses a static buffer for holding audit record.
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
**      01-mar-94 (ajc)
**           Created.
**	30-may-95 (tutto01)
**	     Changed getpwnam call to the reentrant iiCLgetpwnam.
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
	char		pwnam_buf[sizeof(struct passwd)];
	struct passwd	*euserinfo;
	char		temp[5][GL_MAXNAME+1]; /* temp buffer used to turn blank
					    ** pad strings into null terminated
					    ** strings
					    */
	static char 	audit_string[1024];

	err_code->errno = SA_OK;

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
	
	temp[0][GL_MAXNAME] = NULL;
	temp[1][GL_MAXNAME] = NULL;
	temp[2][GL_MAXNAME] = NULL;
	temp[3][GL_MAXNAME] = NULL;
	temp[4][GL_MAXNAME] = NULL;

	MEcopy(aud_rec->sa_ruserid, GL_MAXNAME, temp);
	SAtrim(temp[0], GL_MAXNAME); 
	/*
	** Construct subject information
	*/
	ruserinfo = iiCLgetpwnam(temp[0], &pwnam_buf, sizeof(struct passwd));
	if (ruserinfo == NULL)
	{
		/* can't write audit without this */
		return SA_NOWRITE;
	}
	
	MEcopy(aud_rec->sa_euserid, GL_MAXNAME, temp);
	_VOID_ SAtrim(temp[0], GL_MAXNAME);

	if (aud_rec->sa_status)
		stat = -1;
	else
		stat = 0;

	/*
	** Construct Audit string then output it.
	*/

# define HPBLS_AUDIT_INTRO "\nAHPBLS_ingres\tInstallation: %s\tUser id: %s\n"

	STprintf(audit_string, HPBLS_AUDIT_INTRO, inst_code, temp);

	if (aud_rec->sa_sec_label) 
	{
		STprintf(audit_string, "%s%s%s\n", audit_string, 
			 "Security Level: ", 
			 mand_ir_to_er(&aud_rec->sa_sec_label->bls_label, 0));
	}

	STprintf(audit_string,"%s%s%s\t%s%s\n%s%s\n",
		 audit_string, "Access Type: ", aud_rec->sa_accesstype, 
		 "Event Type: ", aud_rec->sa_eventtype,
		 "Audit message:-- ", aud_rec->sa_messtxt);

	MEcopy(aud_rec->sa_dbname, GL_MAXNAME, temp);
	SAtrim(temp[0], GL_MAXNAME);
	MEcopy(aud_rec->sa_userpriv, GL_MAXNAME, temp[1]);
        SAtrim(temp[1], GL_MAXNAME);
	MEcopy(aud_rec->sa_objpriv, GL_MAXNAME, temp[2]);
        SAtrim(temp[2], GL_MAXNAME);
	MEcopy(aud_rec->sa_objowner, GL_MAXNAME, temp[3]);
        SAtrim(temp[3], GL_MAXNAME);
	MEcopy(aud_rec->sa_objname, GL_MAXNAME, temp[4]);
        SAtrim(temp[4], GL_MAXNAME);

	
	STprintf(audit_string,"%s%s%s\t%s%d\n%s%s\n%s%s\n%s%s\t%s%s\n", 
		 audit_string, "Database: ", temp, "Status: ", (int) stat, 
		 "User Privs: ", temp[1], "Object Privs: ", temp[2], 
		 "Object Owner: ", temp[3], "Object Name: ", temp[4]);

	if (aud_rec->sa_detail_txt) 
	{
		STprintf(audit_string, "%s%s%s\n%s%d\n",
			 audit_string, "Additional Message: -- ", 
			 aud_rec->sa_detail_txt, "Additional Event :",
			 aud_rec->sa_detail_int);
	}

	/*
	** Call HP specific call audit function
	*/

	if (aud_rec->sa_status)
		audit_subsystem(audit_string, "Failure", ET_SUBSYSTEM);
	else
		audit_subsystem(audit_string, "Success", ET_SUBSYSTEM);

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
**	On HP BLS this will just return OK as flush is not required.
**
** Inputs:
**	aud_trail_d	a descriptor that identifies this audit trail
**
** Outputs:
**	err_code	pointer to a variable used to return OS errors
**
** Returns:
**	        OK
** History:
**      01-mar-94 (ajc)
**           Created.
*/
STATUS
SAflush(PTR             *aud_trail_d,
	CL_ERR_DESC     *err_code)
{
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
**	0 if blank was found, string length otherwise.
** History:
**      01-mar-94 (ajc)
**           Created.
**	11-may-94 (stephenb)
**	    Updated to search backwards for first non-blank thus allowing
**	    embeded blanks in the string.
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
# endif /* hp8_bls */
