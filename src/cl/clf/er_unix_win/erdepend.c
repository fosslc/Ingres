/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<lo.h>
#include	<st.h>
#include	<si.h>
#include	<er.h>
#include	<cs.h>
#include	"erloc.h"
#include	<errno.h>


/**
** Name:    ERdepend.c - The group of internal used machin dependent 
**			    routines in ER.
**
** Description:
**      This is a group of internal used machin dependent routines in 
**	ER compatibility library.
**	If you use these routine in your program, you must be included
**	'erloc.h' header file.
**
**	cer_close	close the file
**	cer_open	open the file
**	cer_getdata	get data from file
**	cer_dataset	read in block and set data in buffer
**	cer_sysgetmsg	get system message
**
** History:    
**      01-Oct-1986 (kobayashi) - first written
**	01-Oct-1990 (anton)
**	    Include cs.h for reentrancy support
**      05-Jun-96 (fanra01)
**          Modified cer_sysgetmsg to return NT specific system error messages.
**	15-nov-96 (mcgem01)
**	    Cleaned up code by removing all unneeded code.  Also ifdef'ed in
**	    an NT specific version of cer_sysgetmsg.
**	18-nov-96 (mcgem01)
**	    Fix up screw-up in previous submission.
**	30-nov-98 (stephenb)
**	    Replace direct error string comparison with strerror, it doesn't
**	    work under Solaris 7
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.  Move DESCRIPTOR definition to erloc.h.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**/

#define IOBLOCK	512

/*{
**  Name: cer_close - close the file
**
**  Description:
**	This function close the file that internal file id is parameter 'file'
**	->w_ifi.
**
**  Input:
**	ERFILE	*file		    the pointer of file descriptor.
**
**  Output:
**	none.
**	Return:
**	    STATUS  OK	
**		    FAIL
**
**  Side Effect:
**	none.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
*/
STATUS
cer_close(ERFILE *file)
{
    i4		st = OK;

    if (*file != (ERFILE)0)
    {
	/* close run time file */
	
	if (SIclose(*file) != OK)
	{
	    st = FAIL;
	}
	/* Clear file pointer in ERmulti table. */

	*file = 0;
    }
    return(st);
}

/*{
**  Name: cer_open - open the file
**
**  Description:
**	This function open the file that name is filename and fors is 
**	ER_FASTSIDE or ER_SLOWSIDE.
**
**  Input:
**	LOCATION *loc		location pointer of file.
**	ERFILE *file		pointer of file descriptor
**
**  Output:
**	CL_ERR_DESC *err_code	error code structure
**	Return:
**	    STATUS  OK
**		    ER_BADOPEN
**
**  Side Effect:
**	changes to err_code.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
**	24-jan-1989 (roger)
**	    Make use of CL_ERR_DESC.  Taking advantage of knowledge about
**	    how some CL routines are implemented on Unix- this should be
**	    fixed by adding a CL_ERR_DESC parameter to them and letting
**	    them set it.  Doing this badness because wherever you go,
**	    there's ER.
**
*/
STATUS
cer_open(LOCATION *loc, ERFILE *file, CL_ERR_DESC *err_code)
{
    i4  status;

    /* Open the message file */

    if ((status = SIfopen(loc,"r", SI_RACC,IOBLOCK,file)) != OK)
    {
	SETCLERR(err_code, 0, ER_fopen);    /* secret knowledge- tsk, tsk */
	return(ER_BADOPEN);
    }

    return(OK);
}

/*{
**  Name:   cer_getdata - get data from file.
**
**  Description:
**	This function gets data for 'size' from file to 'pbuf'.
**
**  Input:
**	char *pbuf	    pointer of buffer to set.
**	i4  size	    size of data to set.
**	i4  bkt	    baket number
**	ERFILE *file	    pointer of file descriptor
**
**  Output:
**	CL_ERR_DESC *err_code	error code structure
**	Return:
**	    STATUS	OK
**			ER_BADREAD
**
**  Side Effect:
**	Changes to err_code.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
**	5/90 (bobm) make bucket i4
**
*/
#define		SI_P_START	0

STATUS
cer_getdata(char *pbuf, i4 size, i4 bkt, ERFILE *file, CL_ERR_DESC *err_code)
{
    i4  status;
    i4	rcount;

    /* seek the message file until read part rearch */

    if ((status = SIfseek(*file,(i4)((bkt - 1)*IOBLOCK),SI_P_START)) != OK)
    {
	SETCLERR(err_code, 0, ER_fseek);    /* secret knowledge again */
	return(ER_BADREAD);
    }

    /* read data from message file */

    if ((status = SIread(*file,(i4)size,&rcount,pbuf)) != OK && status != ENDFILE)
    {
	SETCLERR(err_code, status, ER_fread);    /* secret knowledge thrice */
	return(ER_BADREAD);
    }

    /* if the count that read and the count that had read are differrence,return error code. */
    if (size != rcount)
    {
	SETCLERR(err_code, 0, 0);
	return(ER_BADREAD);
    }

    return(OK);
}

/*{
** Name:	cer_dataset - read data to use on VMS
**
** Description:
**	This procedure reads data from message file on VMS. Offset,datasize,
**	pointer of dataarea and internal stream id is set this procedure, then
**	data is set to dataarea from message file for datasize. Why this 
**	procedure is created? Because read on VMS is fix length block, unlike
**	UNIX. On UNIX,there is 'lseek', but on VMS, not.
**
**  Input:
**	i4 offset	    offset of header and class message in messagefile.
**	i4 size	    the size of dataarea to want set
**	char *dataarea	    the pointer of dataarea
**	ERFILE *file	    pointer of file descriptor
**
**  Output:
**	Return
**	    OK		    scucess
**	    ER_BADREAD	    fail sys$read system call
**
**	Exception
**	    none.
**  
**  Sideeffect:
**	none.
**
**  History:
**	01-Oct-1986 (kobayashi) - first written
**
*/
STATUS
cer_dataset(i4 offset, i4 size, char *parea, ERFILE *file)
{
    i4 status;
    i4 rcount;

    /*
    **	move file pointer to offset of file by SIfseek procedure.
    */

    if ((status = SIfseek(*file,(i4)offset,SI_P_START)) != OK)
    {
	return(ER_BADREAD);
    }

    /* read data from the message file. */

    if ((status = SIread(*file,(i4)size,&rcount,parea)) != OK && status != ENDFILE)
    {
	return(ER_BADREAD);
    }

    /* if size and read_count are differrence,return error status. */

    if (size != rcount)
    {
	return(ER_BADREAD);
    }

    return(OK);
}

/*{
**  Name: cer_sysgetmsg	    - get system error message 
**
**  Description:
**	Get the message for the system error described by `error'.
**	Called by ERlookup().
**
**  Input:
**	serr		The CL_ERR_DESC describing the error.
**	msg_desc	The pointer of message descriptor.
**
**  Output:
**	length		The message length
**	err		system error if one occurs here
**	Return:
**	    OK
**	    ER_TRUNCATED
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
**  Side effects:
**	none
**
**  History:
**	24-NOV-1986 (kobayashi)
**	    Created new for 5.0 KANJI
**	08-jul-1988 (roger)
**	    Changed first parameter to CL_ERR_DESC* to increase
**	    information hiding.
**	    Changed to return "Unknown error" for errno >= sys_nerr.
**	    See perror(3).
**	    Fixed bug: `length' was not getting set.
**	    Changed code that assumed `error' member of CL_ERR_DESC.
**      05-Jun-96 (fanra01)
**          Changed to provide system error messages from the system message
**          string table.
**	04-Dec-2001 (hanje04)
**	    Removed declaration of sys_errlist and sys_nerr as they have been
**	    replaced by strerror.
**
*/
STATUS
cer_sysgetmsg(CL_ERR_DESC *serr, i4 *length, DESCRIPTOR *msg_desc, CL_ERR_DESC *err)

# ifdef NT_GENERIC
{
STATUS status = OK;
char *msgp;
DWORD dwChrs = 0;
DWORD i;

    SETCLERR(err, 0, 0);    /* not used */

    if (serr->errnum < 0)
	return (ER_BADPARAM);

    dwChrs = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER ,
                           NULL,        /* no module handle                  */
                           (DWORD)serr->errnum,
                           MAKELANGID(0, SUBLANG_ENGLISH_US),
                           (LPTSTR) &msgp,
                           200,         /* minimum size of memory to allocate*/
                           NULL);       /* no argumemt list                  */
    if (dwChrs)
    {
        /* replace line breaks with spaces */
        for(i=0; i < dwChrs; i++)
            msgp[i] = ((msgp[i]=='\r')||(msgp[i]=='\n')) ? ' ':msgp[i];
    }
    else
    {                                   /* error not found                   */
        msgp = ERx("Unknown error");
    }

    if ((*length = (i4) dwChrs) > msg_desc->desc_length - 1)
    {
	*length = msg_desc->desc_length - 1;
	status = ER_TRUNCATED;
    }

    /* copy the message to msg descriptor */
    STncpy( msg_desc->desc_value, msgp, *length);
    msg_desc->desc_value[ *length ] = EOS;

    if (dwChrs)
        LocalFree((HLOCAL)msgp);

    return (status);
}
# else /* NT_GENERIC */
{
    STATUS status = OK;
    char *msgp;

    SETCLERR(err, 0, 0);    /* not used */

    if (serr->errnum < 0)
	return (ER_BADPARAM);
    
#if defined( usl_us5)
    msgp = strerror(serr->errnum);
#else
    msgp = strerror(serr->errnum) ? strerror(serr->errnum) :
	ERx("Unknown error");
#endif /* usl_us5 */

   if ((*length = STlength(msgp)) > msg_desc->desc_length - 1)
    {
	*length = msg_desc->desc_length - 1;
	status = ER_TRUNCATED;
    }

    /* copy the message to msg descriptor */
    STncpy(msg_desc->desc_value, msgp, *length);
    msg_desc->desc_value[ *length ] = EOS;

    return (status);
}
# endif
