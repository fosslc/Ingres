/*
** TC.C
**
** This file contains the NT versions of the TC CL routines.
** The TC module contains functions to operate on COMM-files, which are
** used for communication between INGRES front ends and the SEP test tool.
**
** On NT, TCFILES are implemented as plain files. A "writer" process
** process simply appends data to the file. A "reader" process keeps
** track of how many bytes it's read so far; when FindFirstFile() reports 
** that there are new bytes to be read (i.e. the file's gotten bigger,) 
** the process reads. 
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
**	26-mar-97 (mcgem01)
**	    Created for NT based upon the UNIX version.
**      03-Apr-97 (fanra01)
**          Modified TCempty to read info for the sep output file.
**      21-May-97 (fanra01)
**          When a grandchild process exits whilst running sep the child
**          process terminates.  The child process was incorrectly positioning
**          the file pointer reading an EOF.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      20-Sep-1999 (fanra01)
**          Plug handle leak noticed in the sep executable.
*/


#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iinap.h>
#include <me.h>
#include <lo.h>
#include <st.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>


#ifndef F_OK
#define F_OK 0
#endif

#define     TC_ESCAPE	    '\\'	    /* TC escape character */

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
**	26-mar-97 (mcgem01)
**	    Ported from UNIX.
**	    
*/

STATUS
TCopen(location, mode, desc)
LOCATION	 *location;
char		 *mode;
TCFILE		**desc;
{
	char	    *filename;
	int	     bytes_written;
	STATUS	     status;
	WIN32_FIND_DATA FindFileData;


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

	/* Open the file. */
	if(((*desc)->_id =  CreateFile ( filename, 
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
				0 )) == INVALID_HANDLE_VALUE )
	{
		MEfree( (PTR) *desc);
		return(FAIL);
	}
	else
	{
		if ( GetLastError() != ERROR_ALREADY_EXISTS)
			(*desc)->_flag |= _TCCREAT;
	}

	/* Initialize the buffer pointers. */
	(*desc)->_pos = (*desc)->_buf;

	if (*mode == 'r')
	{
		i4 ZERO = 0;
		char *dot;
                HANDLE  firstfile;

		/* Create synchronization file. */
		STcopy(filename, (*desc)->_sync_fname);
		if ( (dot = STrindex( (*desc)->_sync_fname, ".", 0) ) != NULL)
			STcopy(".syn", dot);
		else
			STcat( (*desc)->_sync_fname, ".syn");


	        if(((*desc)->_sync_id =  CreateFile ( (*desc)->_sync_fname,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                0 )) == INVALID_HANDLE_VALUE)
	        {
	                MEfree( (PTR) *desc);
	                return(FAIL);
	        }

		/* Initialiaze synch. file. */
		firstfile = FindFirstFile( (*desc)->_sync_fname,
				(LPWIN32_FIND_DATA)&FindFileData);

		if ((firstfile != INVALID_HANDLE_VALUE) &&
                    (FindFileData.nFileSizeLow == 0))
		{
			WriteFile( (*desc)->_sync_id, &ZERO, sizeof(ZERO),
					&bytes_written, NULL );
			FlushFileBuffers ((*desc)->_sync_id);
		}

		(*desc)->_flag |= _TCREAD;
		(*desc)->_end = (*desc)->_pos;
                if (firstfile != INVALID_HANDLE_VALUE)
                {
                    FindClose( firstfile );
                }
	}
	else  
	{
		/* Since we may not be the first writer to attach to this
		   TCFILE, we have to expliticly go to the end.
		*/
		SetFilePointer ( (*desc)->_id, 0, NULL, FILE_END);
		(*desc)->_flag |= _TCWRT;
		(*desc)->_end = (*desc)->_buf + TCBUFSIZE;
	}

	return(OK);
}  /* TCopen */



/*
**
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
**	25-mar-97 (mcgem01)
**	    Borrowed from UNIX.
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
}



/*
**
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
**	25-mar-97 (mcgem01)
**	    Created.
*/

STATUS
TCflush(desc)
TCFILE	*desc;
{
	i4 bytesToWrite;
	int bytes_written;

	if (!desc || !(desc->_flag & _TCWRT))
		return(FAIL);

	bytesToWrite = desc->_pos - desc->_buf;
	if (bytesToWrite > 0)
	{
		/* 
		** Since we might be sharing this TCFILE with another writer,
		** we have to make sure we're really at the end of the file.
		*/
		SetFilePointer ( desc->_id, 0, NULL, FILE_END);
		WriteFile(desc->_id, desc->_buf, bytesToWrite, 
					&bytes_written, NULL);
		if (bytes_written != bytesToWrite)
			return(FAIL);
		FlushFileBuffers (desc->_id);
	}

	desc->_pos = desc->_buf;
	return(OK);
}



/*
**
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
**	25-mar-97 (mcgem01)
**	    Created.
**      21-May-97 (fanra01)
**          The size read from the sync file refers to the size of interface
**          file.
**          Modified file position flag to be relative to the beginning of the
**          file rather than relative to current position.
**      20-Sep-1999 (fanra01)
**          Close the handle returned from FindFirstFile.
**	18-jul-2002 (somsa01)
**	    Instead of using PCsleep(), now we can use II_nap().
*/
 
static bool doescape = TRUE;

i4
TCgetc(desc,seconds)
TCFILE	*desc;
i4	seconds;
{
	i4	toreturn;
	i4	bytes_written;
	i4	bytes_read;
	WIN32_FIND_DATA FindFileData;
        DWORD dwError;

	if (!(desc->_flag & _TCREAD))
		return(TC_EOF);

	if (desc->_pos < desc->_end)
		toreturn = *desc->_pos++;
	else
	{
		int bytesToRead;
		i4 start_time, now_time, cur_size;

		/* 
		** Retrieve the last i4 written to the synch file. This
		** tells the file address of the last byte read from the
		** TCFILE.
		*/

		SetFilePointer (desc->_sync_id, -(i4)(sizeof(i4)), NULL, 
				FILE_END);
		ReadFile (desc->_sync_id, &cur_size, sizeof(i4), &bytes_read,
				NULL );
		/* If we aren't the most recent reader, seek to the first
		** unread byte.
		*/
		if (cur_size != desc->_size)
		{
			desc->_size = cur_size;
			SetFilePointer(desc->_id, cur_size, NULL, FILE_BEGIN);
		}
		start_time = TMsecs();
		while (TRUE)
		{
                        HANDLE firstfile = NULL;

			if ((firstfile=FindFirstFile( desc->_fname,
					(LPWIN32_FIND_DATA)&FindFileData))
				== INVALID_HANDLE_VALUE)
				return(TC_EOF);
			if ((int)(FindFileData.nFileSizeLow) > desc->_size)
                        {
                                FindClose( firstfile );
				break;
                        }
			now_time = TMsecs();
			if ( (seconds > 0)
			  && ( (now_time - start_time) > seconds) )
                        {
                                FindClose( firstfile );
				return(TC_TIMEDOUT);
                        }
                        FindClose( firstfile );
			II_nap(1000);
		}
		bytesToRead = FindFileData.nFileSizeLow - desc->_size;
		if (bytesToRead > TCBUFSIZE)
			bytesToRead = TCBUFSIZE;
		ReadFile(desc->_id, desc->_buf, bytesToRead, &bytes_read, NULL);
		if (bytes_read <= 0)
                {
                    dwError = GetLastError();
			return(TC_EOF);
                }
		if (bytes_read != bytesToRead)
			SIprintf("TCgetc: tried to read %d bytes, but read() returned %d.\n",
			  bytesToRead, bytes_read);
		desc->_end = desc->_buf + bytes_read;
		desc->_pos = desc->_buf + 1;
		desc->_size += bytes_read;
		/* Update the synch file. */
		WriteFile (desc->_sync_id, &desc->_size, sizeof(i4),
				&bytes_written, NULL );
		FlushFileBuffers (desc->_sync_id);
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
**
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
**	25-mar-97 (mcgem01)
**	    Created - based upon the UNIX version.
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
		    if ( (dot = STrindex(desc->_sync_fname, ".", 0) ) == NULL)
			STcat(desc->_sync_fname, ".syn");
		    else
			STcopy(".syn", dot);
		    DeleteFile(desc->_sync_fname);
		}
	}
	FlushFileBuffers(desc->_id);
	CloseHandle(desc->_id);

	if (desc->_flag & _TCREAD)
	{
		FlushFileBuffers(desc->_sync_id);
		CloseHandle(desc->_sync_id);
		if (desc->_flag & _TCCREAT)
			DeleteFile(desc->_sync_fname);
	}

	if (desc->_flag & _TCCREAT)
		DeleteFile(desc->_fname);

	MEfree((PTR)desc);

	return(OK);
}



/*
**
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
**	25-mar-97 (mcgem01)
**	    Ported from UNIX.
**      03-Apr-97 (fanra01)
**          Changed from _sync_fname to _fname.
**      20-Sep-1999 (fanra01)
**          Close the handle returned from FindFirstFile.
*/

bool
TCempty(desc)
TCFILE	*desc;
{
	i4 bytes_read;
	WIN32_FIND_DATA FindFileData;
	HANDLE firstfile = NULL;

	if (!(desc->_flag & _TCREAD))
		return(TRUE);

	if (desc->_pos < desc->_end)
		return(FALSE);
	else
	{

		if ((firstfile = FindFirstFile( desc->_fname,
				(LPWIN32_FIND_DATA)&FindFileData))
				== INVALID_HANDLE_VALUE)
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
		if ((int)(FindFileData.nFileSizeLow) > desc->_size)
		{
			SetFilePointer(desc->_sync_id, -(i4)(sizeof(i4)),
					NULL, FILE_END);
			ReadFile(desc->_sync_id, &desc->_size, sizeof(i4),
				&bytes_read, NULL );
		}
                FindClose( firstfile );
		return((int)(FindFileData.nFileSizeLow) == desc->_size);
	}
}
