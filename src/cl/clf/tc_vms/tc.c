/*
**    Copyright (c) 1989, 2008 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <descrip.h>
#include <ssdef.h>
#include <iodef.h>
#include <dvidef.h>
#include <dcdef.h>
#include <efndef.h>
#include <iosbdef.h>
#include <lib$routines.h>
#include <starlet.h>
#include <me.h>
#include <lo.h>
#include <st.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>
#include <astjacket.h>

#define	MBXSIZE		2048
#define	TCBUFSIZE	512

static	i4	tc_files = 0;

/*
**++
**  NAME
**
**      TCopen()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Opens a COMM-file. 
**      COMM-files are files that can be shared by processes.
**      They provide a more flexible communication mechanism
**      between Ingres modules and testing tools.
**
**  FORMAL PARAMETERS:
**
**      location        - location of the file
**      mode            - access mode of the file, possible values are
**                        "w" - open file for writing
**                        "r" - open file for reading 
**      desc            - value to get file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**      OK              the file was properly opened,
**
**      TC_BAD_SYNTAX   illegal parameter was passed
**
**      FAIL            file couldn't be opened
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

STATUS
TCopen(location, mode, desc)
LOCATION    *location;
char	    *mode;
TCFILE	    **desc;
{
    STATUS			thestatus;
    struct dsc$descriptor_s	mbxname;
    i4				syserr,ast_value;
    u_i2			channel;
    char			modec;
    char			*name,*abuf;
    TCFILE			*afile;

    switch(modec = *mode)
    {
	case 'r':
	case 'w':
	    break;
	default:
	    return(TC_BAD_SYNTAX);
    }

    LOtos(location,&name);
    ast_value = sys$setast(0);
    thestatus = FAIL;

    for (;;)
    {
	if (name == NULL || *name == '\0')
	    break;
	
	/* check if reached max. number of opened files */

	if (tc_files > _COMMFILES)
	    break;

	/* allocate space for file structure */

	afile = (TCFILE *)MEreqmem(0,sizeof(*afile),TRUE,&thestatus);
	if (thestatus != OK)
	{
	    thestatus = FAIL;
	    break;
	}

	/* allocate space for file buffer */

	abuf = (char *)MEreqmem(0,TCBUFSIZE,TRUE,&thestatus);
	if (thestatus != OK)
	{
	    thestatus = FAIL;
	    break;
	}


	mbxname.dsc$w_length = STlength(name);
	mbxname.dsc$b_dtype = DSC$K_DTYPE_T;
	mbxname.dsc$b_class = DSC$K_CLASS_S;
	mbxname.dsc$a_pointer = name;

	syserr = sys$crembx( 0, &channel, MBXSIZE, MBXSIZE, 0, 0, &mbxname);
	if (syserr != SS$_NORMAL)
	{
	    MEfree((PTR)afile);
	    MEfree((PTR)abuf);
	    break;
	}

	afile->_id = channel;
	afile->_flag |= _TCOPEN;
	afile->_buf = afile->_pos = abuf;
	if (modec == 'r')
	{
	    afile->_flag |= _TCREAD;
	    afile->_end = afile->_pos;
	}
	else  
	{
	    afile->_flag |= _TCWRT;
	    afile->_end = afile->_buf + TCBUFSIZE;
	}

	*desc = afile;
	thestatus = OK;
	break;
    }
       
    if (ast_value == SS$_WASSET)
	sys$setast(1);
    return(thestatus);
}



/*
**++
**  NAME
**
**      TCputc()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Writes a character to a COMM-file. 
**      Data written to a COMM-file is sequenced in order of arrival.
**      Data will be appended to a buffer, after buffer limit is reached
**	data will be flushed to the file. 
**      Client code may make no assumptions about when and whether the TCputc
**      routine will block. An implementation is free to make TCputc block on
**      the second TCputc call if the previous character has not been read. 
**
**  FORMAL PARAMETERS:
**
**      achar           - char to be written to COMM-file
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       OK             char was properly appended to file,
**
**       FAIL           char could not be appended to file.
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
*/

STATUS
TCputc(achar,desc)
i4	achar;
TCFILE	*desc;
{
	u_i2 thechannel;
	i4 num_chars,status;
	char *cptr;

	if (!(desc->_flag & _TCWRT))
	    return(FAIL);

	if (desc->_pos >= desc->_end)
	{
	    TCflush(desc);
	}

	cptr = desc->_pos;
	*cptr = achar;
	desc->_pos = ++cptr;

	return(OK);
}



/*
**++
**  NAME
**
**      TCflush()
**
**  FUNCTIONAL DESCRIPTION:
**
**      sends write buffer to given COMM-file. 
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       OK             data in write buffer was flushed,
**
**       FAIL           flush could not be done.
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
*/

STATUS
TCflush(desc)
TCFILE	*desc;
{
    u_i2 thechannel, writemode;
    i4 num_chars,status;
    char *cptr;
    IOSB iosb;

    if (!(desc->_flag & _TCWRT))
	return(FAIL);

    thechannel = desc->_id;
    num_chars = desc->_pos - desc->_buf;	    
    cptr = desc->_buf;

    if (num_chars)
    {
	writemode = IO$_WRITEVBLK|IO$M_NOFORMAT|IO$M_NOW;
	status = sys$qiow(EFN$C_ENF, thechannel, writemode, &iosb, 0,
			    0, cptr, num_chars, 0, 0, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
	    return(FAIL);
    }

    desc->_pos = desc->_buf;
    return(OK);

}



/*
**++
**  NAME
**
**      TCgetc()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Reads a character from a COMM-file. If the COMM-file is empty,
**      it will wait until at least one byte of data is available.
**      If the process writing to the file closes it, TCgetc will return
**      TC_EOF.
**      The TCgetc routine supports a timeout feature. If a given number of 
**      seconds has passed while the TCgetc routine is waiting for data to
**      be available, the TC_TIMEDOUT code will be returned by TCgetc.
**      If the given number of seconds is < 0, TCgetc will wait until data
**      is available.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**      seconds         - number of seconds to wait for data
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       next character available in the COMM-file, or
**
**       TC_EOF if process writing to the file closes it, or
**
**       TC_TIMEDOUT if TCgetc has been waiting for a given number of seconds
**                   ans still no data is available
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
*/
 
TCgetc(desc,seconds)
TCFILE	*desc;
i4	seconds;
{
        i4     status,achar,time1,time2,num_chars;
	u_i2	thechannel, readmode;
	char	*cptr;
        IOSB    iosb;

	if (!(desc->_flag & _TCREAD))
	    return(0);

	thechannel = desc->_id;
	cptr = desc->_pos;

	if (cptr < desc->_end)
	{
	    achar = *cptr;
	    desc->_pos = ++cptr;
	}
	else
	{
	    cptr = desc->_buf;
	    readmode = IO$_READVBLK;
	    if (seconds > 0)
	    {
		readmode |= IO$M_NOW;
		time1 = TMsecs();
	    }
	    for (;;)
	    {
		status = sys$qiow(EFN$C_ENF, thechannel, readmode, &iosb, 0, 0,
                        cptr, TCBUFSIZE, 0, 0, 0, 0);
		if ((status & 1) == 0)
		    lib$signal(status);

		num_chars = iosb.iosb$w_bcnt;
		if (iosb.iosb$w_status == SS$_ENDOFFILE && iosb.iosb$l_pid)
		{
		    achar = TC_EOF;
		    break;
		}
		if (seconds < 1 || num_chars)
		{
		    achar = *cptr;
		    desc->_pos = ++cptr;
		    desc->_end = desc->_buf + num_chars;
		    break;
		}
		if (((time2 = TMsecs()) - time1) > seconds)
		{
		    achar = TC_TIMEDOUT;
		    break;
		}
		PCsleep(1000);
	    }
	}

	
	return(achar);

}



/*
**++
**  NAME
**
**      TCclose()
**
**  FUNCTIONAL DESCRIPTION:
**
**      closes a COMM-file.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**      OK      file was properly closed,
**
**      FAIL    couldn't close file.
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
*/

STATUS
TCclose(desc)
TCFILE	*desc;
{
	u_i2 thechannel,writemode;
	i4  status,num_chars;
	char abuffer[5];
	TCFILE *temp;
	IOSB iosb;

	if (!(desc->_flag & _TCOPEN))
	    return(FAIL);

	thechannel = desc->_id;

	/* close the mailbox by sending EOF character (if write TCFILE) */

	if (desc->_flag & _TCWRT)
	{
	    TCflush(desc);
	    abuffer[0] = '\0';
	    num_chars = 0;
	    writemode = IO$_WRITEOF|IO$M_NOW;
	    status = sys$qiow(EFN$C_ENF, thechannel, writemode, &iosb, 0,
			    0, abuffer, num_chars, 0, 0, 0, 0);
	    if (status & 1)
	        status = iosb.iosb$w_status;
	    if ((status & 1) == 0)
		return(FAIL);
	}

	/* update local variables */

	desc->_flag &= ~_TCOPEN;
	tc_files--;

	/* free memory taken */

	MEfree((PTR)desc->_buf);
	MEfree((PTR)desc);

	/* deassign channel */

	if ((status = sys$dassgn(thechannel)) != SS$_NORMAL)
	    return(FAIL);
	else
	    return(OK);

}



/*
**++
**  NAME
**
**      TCempty()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Check if there is unread data in a COMM-file.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**      TRUE    there is no unread data in the COMM-file,
**
**      FALSE   there is unread data in the file.
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**  (may-89) created by Eduardo
*/

bool
TCempty(desc)
TCFILE	*desc;
{
        i4     status,num_chars;
	u_i2	thechannel, readmode;
	char	*cptr;
        IOSB    iosb;

	if (!(desc->_flag & _TCREAD))
	    return(TRUE);

	thechannel = desc->_id;
	cptr = desc->_pos;

	if (cptr < desc->_end)
	{
	    return(FALSE);
	}
	else
	{
	    cptr = desc->_buf;
	    readmode = IO$_READVBLK|IO$M_NOW;

	    status = sys$qiow(EFN$C_ENF, thechannel, readmode, &iosb, 0, 0,
			    cptr, TCBUFSIZE, 0, 0, 0, 0);
	    if ((status & 1) == 0)
		lib$signal(status);

	    num_chars = iosb.iosb$w_bcnt;
	    if (iosb.iosb$w_status == SS$_ENDOFFILE)
	    {
		if (iosb.iosb$l_pid)
		{
		    *cptr = TC_EOF;
		    num_chars = 1;
		}
		else
		    return(TRUE);
	    }
	    desc->_pos = cptr;
	    desc->_end = desc->_buf + num_chars;
	    return(FALSE);
	    
	}

	

}
