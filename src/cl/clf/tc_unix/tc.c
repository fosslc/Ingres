/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** TC.C
**
** This file contains Unix versions of the TC CL routines.
** The TC module contains functions to operate on COMM-files, which are
** used for communication between INGRES front ends and the SEP test tool.
**
** On Unix, TCFILES are implemented as plain files. A "writer" process
** process simply appends data to the file. A "reader" process keeps
** track of how many bytes it's read so far; when fstat() reports that there
** are new bytes to be read (i.e. the file's gotten bigger,) the process
** reads. As a performance enhancement, this scheme of polling on fstat()
** will probably change soon.
**
** Also, the current SEP mainline implicitly requires that TCFILEs be able to
** support multiple processes on one end. Synchronization files are used to
** work around this problem. There's a synchronization file for each TCFILE.
** Currently, a "reader" must execute 3 steps: (1) check the synchronization
** file to find the file address of the last bytes read; (2) seek to the first
** unread byte; (3) read; and (4) update the synchronization file. The SEP
** mainline will have this restriction removed in a matter of dayts, at which
** time the significant performance hit from synchronization files will be
** removed.
**
** Routines in this file:
**	TCopen()
**	TCputc()
**	TCflush()
**	TCgetc()
**	TCclose()
**	TCempty()
**
** History:
**	22-may-89 (mca) - Initial version.
**	 1-aug-89 (mca) - Fixed Sun-specific timer sleep problem in TCgetc.
**			  On a heavily-loaded system, signal was occasionally
**			  delivered before sigpause() was called. Made changes
**			  so that this isn't a problem. Also, changed the
**			  length of the sleep.
**	23-Aug-1989 (fredv)
**		Using cl headers instead of sys headers.
**		Removed clsecret.h because include clconfig.h is enough.
**	06-Dec-1989 (fredv)
**		Replaced sun_u42 defines with xCL_030_SETITIMER_EXISTS
**		so that we can use itmer for those systems support this
**		function.
**	12-dec-1989 (mca)
**		Merged tests/sep/sepcl/tcsep.c with this file. Added the
**		IITCmaster bool, which SEP will set to TRUE but which
**		INGRES frontends will leave FALSE. When it's TRUE, TCclose()
**		will unlink sync files. Also, have TCgetc() return TC_EOF
**		if file isn't open for reading; previously, it returned 0,
**		which is a valid character. (Thanks, DaveB.)
**	05-apr-1990 (kathryn) (integrated into ingres63p by rog)
**		Remove references to SETITIMER use II_nap mechanism. 
**	16-apr-90 (kimman)
**		Integrating in the latest changes from sepcl/tcsep.c
**		into tc.c.
**	21-aug-90 (rog)
**		Portability changes so that TCgetc handles -1 correctly.
**	27-aug-90 (rog)
**		Move TC_ESCAPE into this file so that it isn't globally
**		visible.
**	28-aug-1990 (rog)
**		Back out the above two changes because they need to go through
**		the proper review process.
**	08-oct-90 (rog)
**		Escape out-of-band data, e.g., negative numbers -- especially
**		-1, in TCputc() and TCgetc().
**      11-feb-93 (smc)
**              Added time.h to define tm for axp_osf.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	25-may-95 (emmag)
**	    Map II_nap to Sleep() API call on NT.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	06-mar-96 (emmag)
**	    Check for NULL pointer.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to tcdata.c.
**	23-mar-97 (mcgem01)
**	    Desktop platforms are renowned for their tendancy to cache
**	    disk i/o.  Force all writes to disk with a commit.
**	12-jun-1998 (kosma01)
**	    On sgi_us5, replaced the lseeks that use a negative offset to seek
**	    backwards 4 bytes from the end of the syn file with fstat.
**	    Sep was failing by reading the same byte from the comm file.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Mar-2001 (wansh01)
**	    On dgi_us5, replaced the lseeks that use a negative offset to seek
**	    backwards 4 bytes from the end of the syn file with fstat.
**	    Sep was failing by reading the same byte from the comm file.
**	23-Sep-2003 (hanje04)
**	    On int_lnx with largefile support lseek must be called with
**        offset of type off_t and not int otherwise we get some crazy
**        file pointer behavior.
**	02-Jun-2005 (hanje04)
**	    MAC OS X also needs lseek to be called with type off_t.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/


#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <sys/stat.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include <fcntl.h>
#endif
#ifdef xCL_007_SYS_FILE_H_EXISTS
#include <sys/file.h>
#endif
#ifdef axp_osf
#include <time.h>
#endif
#include <me.h>
#include <lo.h>
#include <st.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>
#include <seek.h>


#ifndef F_OK
#define F_OK 0
#endif

#define     TC_ESCAPE	    '\\'	    /* TC escape character */

#ifdef NT_GENERIC
#define	    II_nap(A)	    Sleep(A)
#endif

/*
**
**  NAME
**
**      TCopen()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Opens a COMM-file.
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
**      	OK              the file was properly opened,
**      	TC_BAD_SYNTAX   illegal parameter was passed
**      	FAIL            file couldn't be opened
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
**      30-Aug-95 (fanra01)
**          Added NT_GENERIC sections for file creation and open in binary mode.
**          File types are important on the desktop platforms.  Using text
**          mode does not guarentee the number of bytes written to the file
**          because of carriage return insertion.
*/

STATUS
TCopen(location, mode, desc)
LOCATION	 *location;
char		 *mode;
TCFILE		**desc;
{
	char	    *filename;
	int	     fileflags;
	STATUS	     status;

	if ( (*mode != 'r') && (*mode != 'w') )
	{
		return(TC_BAD_SYNTAX);
	}

	LOtos(location,&filename);

	if (filename == NULL || *filename == '\0')
		return(FAIL);

	/* allocate space for file structure */

	*desc = (TCFILE *) MEreqmem(0, sizeof(**desc), TRUE, &status);
	if (status != OK)
		return(FAIL);

	STcopy(filename, (*desc)->_fname);

	(*desc)->_flag = _TCOPEN;
	(*desc)->_size = 0;

	/* Test for existence of the file. If we end up being the one who
	   created it, we'll have to unlink it later.
	*/
	if (access(filename, F_OK) != 0)
		(*desc)->_flag |= _TCCREAT;

	/* Open the file. */
# if defined(NT_GENERIC)
	fileflags = (*mode == 'r' ? O_RDONLY : O_WRONLY) | O_CREAT | O_BINARY;
# else
	fileflags = (*mode == 'r' ? O_RDONLY : O_WRONLY) | O_CREAT;
# endif  /* NT_GENERIC */
	(*desc)->_id = open(filename, fileflags, 0666);
	if ((*desc)->_id < 0)
	{
		MEfree( (PTR) *desc);
		return(FAIL);
	}

	/* Initialize the buffer pointers. */
	(*desc)->_pos = (*desc)->_buf;

	if (*mode == 'r')
	{
		struct stat st;
		i4 ZERO = 0;
		char *dot;

		/* Create synchronization file. */
		STcopy(filename, (*desc)->_sync_fname);
		if ( (dot = STrchr( (*desc)->_sync_fname, '.') ) != NULL)
			STcopy(".syn", dot);
		else
			STcat( (*desc)->_sync_fname, ".syn");
# if defined(NT_GENERIC)
		(*desc)->_sync_id = open((*desc)->_sync_fname, O_RDWR|O_CREAT|O_BINARY,
		  0666);
# else
		(*desc)->_sync_id = open((*desc)->_sync_fname, O_RDWR|O_CREAT,
		  0666);
# endif /* NT_GENERIC */
		if ((*desc)->_sync_id < 0)
		{
			MEfree( (PTR) *desc);
			return(FAIL);
		}
		/* Initialiaze synch. file. */
		fstat( (*desc)->_sync_id, &st);
		if (st.st_size == 0)
		{
			write( (*desc)->_sync_id, &ZERO, sizeof(ZERO) );
# ifdef DESKTOP
			_commit ((*desc)->_sync_id);
# endif
		}

		(*desc)->_flag |= _TCREAD;
		(*desc)->_end = (*desc)->_pos;
	}
	else  
	{
		/* Since we may not be the first writer to attach to this
		   TCFILE, we have to expliticly go to the end.
		*/
		lseek( (*desc)->_id, 0, L_XTND);
		(*desc)->_flag |= _TCWRT;
		(*desc)->_end = (*desc)->_buf + TCBUFSIZE;
	}

	return(OK);
}  /* TCopen */



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
**	22-may-89 (mca)		Initial version.
*/

STATUS
TCputc(achar,desc)
i4	achar;
TCFILE	*desc;
{
	if (!(desc->_flag & _TCWRT))
	    return(FAIL);

	if (desc->_pos >= desc->_end)
	{
	    TCflush(desc);
	}

	if ((achar != (i4) ((uchar) achar)) || (achar == TC_ESCAPE))
	{
		*desc->_pos++ = TC_ESCAPE;

		if (desc->_pos >= desc->_end)
		{
		    TCflush(desc);
		}
	}
	*desc->_pos++ = (uchar) achar;

	return(OK);
} /* TCputc */



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
**	22-may-89 (mca)		Initial version.
*/

STATUS
TCflush(desc)
TCFILE	*desc;
{
	i4 bytesToWrite;

	if (!desc || !(desc->_flag & _TCWRT))
		return(FAIL);

	bytesToWrite = desc->_pos - desc->_buf;
	if (bytesToWrite > 0)
	{
		/* Since we might be sharing this TCFILE with another writer,
		   we have to make sure we're really at the end of the file.
		*/
		lseek(desc->_id, 0, L_XTND);
		if (write(desc->_id, desc->_buf, bytesToWrite) != bytesToWrite)
			return(FAIL);
	}

# ifdef DESKTOP
	_commit (desc->_id);
# endif
	desc->_pos = desc->_buf;
	return(OK);
}  /* TCflush */



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
**	22-may-89 (mca)		Initial version.
*/
 
static bool doescape = TRUE;

i4
TCgetc(desc,seconds)
TCFILE	*desc;
i4	seconds;
{
	i4	toreturn;

	if (!(desc->_flag & _TCREAD))
		return(TC_EOF);

	if (desc->_pos < desc->_end)
		toreturn = *desc->_pos++;
	else
	{
		int bytesToRead, ret_val;
		i4 start_time, now_time, cur_size;
		struct stat statbuf;

		/* Retrieve the last i4  written to the synch file. This
		   tells the file address of the last byte read from the
		   TCFILE.
		*/
#if defined(sgi_us5) || defined(dgi_us5) || defined(LNX) || \
    defined(a64_sol) || defined(OSX)
{
		off_t  real_offset;
		real_offset = (-1 * (off_t)sizeof(i4));
		lseek(desc->_sync_id, real_offset, L_XTND);
}
#else
		lseek(desc->_sync_id, sizeof(i4)*-1, L_XTND);
#endif
		read(desc->_sync_id, &cur_size, sizeof(i4) );
		/* If we aren't the most recent reader, seek to the first
		   unread byte.
		*/
		if (cur_size != desc->_size)
		{
			desc->_size = cur_size;
			lseek(desc->_id, (off_t)cur_size, L_SET);
		}
		start_time = TMsecs();
		while (1)
		{
			if (fstat(desc->_id, &statbuf) < 0)
				return(TC_EOF);
			if (statbuf.st_size > desc->_size)
				break;
			now_time = TMsecs();
			if ( (seconds > 0)
			  && ( (now_time - start_time) > seconds) )
				return(TC_TIMEDOUT);
			II_nap(1000);
		}
		bytesToRead = statbuf.st_size - desc->_size;
		if (bytesToRead > TCBUFSIZE)
			bytesToRead = TCBUFSIZE;
		if ((ret_val = read(desc->_id, desc->_buf, bytesToRead)) <= 0)
			return(TC_EOF);
		if (ret_val != bytesToRead)
			SIprintf("TCgetc: tried to read %d bytes, but read() returned %d.\n",
			  bytesToRead, ret_val);
		desc->_end = desc->_buf + ret_val;
		desc->_pos = desc->_buf + 1;
		desc->_size += ret_val;
		/* Update the synch file. */
		write(desc->_sync_id, &desc->_size, sizeof(i4) );
# ifdef DESKTOP
		_commit(desc->_sync_id);
# endif
		toreturn = *desc->_buf;
	}
	if (doescape && toreturn == TC_ESCAPE)
	{
	    doescape = FALSE;
	    switch(toreturn = TCgetc(desc, 0))
	    {
		case TC_ESCAPE:
		    break;
		default:
		    toreturn = -1 - ( (i4)((uchar)-1) - ((uchar)toreturn));
		    break;
	    }
	    doescape = TRUE;
	}
	return(toreturn);
}  /* TCgetc */



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
**	22-may-89 (mca)		Initial version.
**	05-sep-95 (allst01/smiba01)
**		MUST check for NULL ptr
*/

/* This global will always be FALSE in INGRES frontends. In SEP, it will
** immediately be set to TRUE at startup.
*/
GLOBALREF bool IITCmaster;

STATUS
TCclose(desc)
TCFILE	*desc;
{
	if (!desc || !(desc->_flag & _TCOPEN))
		return(FAIL);


	if (desc->_flag & _TCWRT)
	{
		TCputc(TC_EOF, desc);
		TCflush(desc);
		if (IITCmaster)
		{
		    char *dot;

		    STcopy(desc->_fname, desc->_sync_fname);
		    if ( (dot = STrchr(desc->_sync_fname, '.') ) == NULL)
			STcat(desc->_sync_fname, ".syn");
		    else
			STcopy(".syn", dot);
		    unlink(desc->_sync_fname);
		}
	}
	close(desc->_id);

	if (desc->_flag & _TCREAD)
	{
		close(desc->_sync_id);
		if (desc->_flag & _TCCREAT)
			unlink(desc->_sync_fname);
	}

	if (desc->_flag & _TCCREAT)
		unlink(desc->_fname);

	MEfree((PTR)desc);

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
**	22-may-89 (mca)		Initial version.
*/

bool
TCempty(desc)
TCFILE	*desc;
{
	if (!(desc->_flag & _TCREAD))
		return(TRUE);

	if (desc->_pos < desc->_end)
		return(FALSE);
	else
	{
		struct stat statbuf;

		if (fstat(desc->_id, &statbuf) < 0)
		{
			desc->_pos = desc->_buf;
			*desc->_pos = TC_EOF;
			desc->_end = desc->_pos + 1;
			return(FALSE);
		}
		/* If it appears that there are new bytes to read in the
		   TCFILE, check the snch file to make sure someone else
		   hasn't already read them.
		*/
		if (statbuf.st_size > desc->_size)
		{
#if defined(sgi_us5) || defined(dgi_us5) || defined(a64_sol) || \
    defined(LNX) || defined(OSX)
{
			off_t  real_offset;
			real_offset = (-1 * (off_t)sizeof(i4));
			lseek(desc->_sync_id, real_offset, L_XTND);
}
#else
			lseek(desc->_sync_id, sizeof(i4)*-1, L_XTND);
#endif
			read(desc->_sync_id, &desc->_size, sizeof(i4) );
		}
		return(statbuf.st_size == desc->_size);
	}
}
