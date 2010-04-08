/*
** Copyright (c) 1983, 2005 Ingres Corporation
**
*/

# include	<compat.h>
# include       <gl.h>
# include	<rms.h>
# include	<lo.h>
# include	<st.h>
# include	<me.h>
# include	<si.h>
# include	<devdef.h>
# include	<descrip.h>
# include	<ssdef.h>
# include	<rmsdef.h>
# include	<starlet.h>
# include	"silocal.h"
# include       <astjacket.h>

/**
** Name:	si.c -	VAX/VMS Standard I/O Routines.
**
** Purpose:
**	Provides a portable standard i/o library for RTI programs.
**
** Defines:
**
**	status = SIisopen(file)
**	status = SIclose(file)
**	status = SIflush(file)
**	status = SIopen(loc, mode, &file)
**	status = SIread(file, numofbytes, &count, buf)
**	status = SIputrec(buf, file)
**	status = SIgetrec(file, n, buf)
**	status = SIfseek(file,offset,position)
**	status = SIftell(file,offset)
**	status = SIcat(loc, file)
**	
** History:
**		25-jan-1984	(djf)	Fixed up for 2.2CL
**		07-apr-84 	(fhc)	fixed $flush to correctly process full
**					buffers.
**		9/27/84 (lichtman) --   made SIfopen return message numbers as
**					defined in erdef.msg, because ERreport
**					says "NORMAL SUCCESSFUL COMPLETION" when
**					called with FAIL
**		17-jan-85 (fhc)	-- changed all calls to sys$get() to rms_get(),
**					and sys$put() to rms_put().  This was
**					done to get around a VMS 4.0 bug,
**					wherein control C's as we process them
**					do not work.
**		16-apr-86 (jhw) -- Both 'stderr' and 'stdout' are now flushed
**				on input from 'stdin'.
**		13-feb-90 (dkh) - Added missing second parameter for call
**				  to $flush() from SIflush().
**              4/91 (Mike S) -- Optimize SIfseek; if not changing records,
**                               just adjust the offset.
**                               Replace SI_iob with SI_iobptr.
**              7/91 (Mike S) -- Allocate FILEX structures dynamically
**              9/91 (alan)   -- Fix calculation of number of bytes in buffer
**                               on SIfopen of RACC file. (bug #39786)
**              12/91 (Mike S)-- Implement CL committe proposal allowing
**                              SIgetc and SIputc for RACC files.  Changes in
**                              _filbuf and _flsbuf
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	25-apr-94 (rganski)
**	    SIfopen(): set fab$w_mrs to rec_length for SI_TXT files. Return
**	    FAIL if rec_length > SI_MAX_TXT_REC. As per CL spec.
**	30-mar-95 (albany)
**	    Integrate changes from VAX CL:
**		31-Jan-1995 (canor01)
**	    	Implement Rewind for a text file. (Used by ABF runtime).
**	    	Other positioning of text file still returns error.
**	26-jul-95 (duursma)
**	    Fixes for bugs 68699/69765:
**	    When opening a terminal device, do not set the DEV$M_CR
**	    in the FAB, which would cause the terminal to act as a
**	    carriage controlled device, making it impossible to do
**	    things like prompt messages.  
**
**	    In $flush, added code to deal with terminal devices,
**	    which had been treated as carriage control devices up to now;
**	    this made it impossible to output a message to the terminal
**	    without a newline (eg. for a password prompt).
**
**	27-jul-95 (duursma)
**	    Now that terminal devices no longer have the FAB$M_CR bit set,
**	    must do an additional check for DEV$M_TRM being 0 before deciding
**	    to handle the case of variable length record text files.
**	29-aug-95 (albany)
**	    We now flush only writable (output) buffers during an SIflush() 
**	    since flushing input buffers dumps the output buffer to the screen.
**	8 Sep 1995 (dougb) bug 65193
**	    Ensure that files are not opened exclusively.  Corrects a
**	    problem with sharing of files opened for logging.
**	8 Apr 1996 (mckba01)
**	    Change implementation of change #412711 (25-apr-94 (rganski)). 
**	    Original method caused 3 bugs :- 73743, 71092, 72768. 
**	21 Oct 1996 (rosga02)
**          Init is_term to 0, to fix bug #75975.
**          Also, Improve performance for fixed record size record by
**          writing them directly from from the record buffer.
**	09-mar-1998 (kinte01)
**          Integrated VAX CL change 275230 from ingres63p:
**              15-may-95 (mckba01)
**                  Changes for GCN optimization. New file type
**                  #def  GCN_RACC_FILE.
**                  Addition of new #def for _flags in FILE structure
**                  _SI_RACC_OPT.
**          Back out last change to _FILE structure per change 275491:
**                      11/95 (mckba01) - Remove buffered record flag _usebuf
**                      from _FILE struct, after disaster at JPM.  Also it's
**                      non-aligned on Alpha anyway.
**	11-dec-1998 (kinte01)
**		In SIftell, for SI_RACC_OPT files, an offset was calculated
**		for fx but fx was never assigned a value. Found by the 
**		DEC C 6.0 compiler.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function declarations
**      15-Aug-2000 (horda03)
**              Allow the position of the next character to be obtained from
**              a record to be repositioned within the record. When a record is
**              read, it is stored in a buffer. A pointer is maintained indicating
**              the next character to be read. Thus little change is required to
**              SIfseek to reposition the cursor within the buffer. This will
**              allow BLOB data to be copied in ASCII format.
**              (72137)
**      21-Aug-2000 (horda03)
**              The IIACP.LOG, IIRCP.LOG and II_DBMS_LOG files are not opened
**              with shared READ access. The first two are because incorrect
**              filenames where being checked. The latter was never checked.
**              Because the ACP and RCP log files can have the node name
**              included need to check the start of the filename. Because the
**              II_DBMS_LOG is a logical which may contain a "%p" (process ID)
**              only files of the form <node>::<dir>II_DBMS... will be opened
**              shared. (102372)
**     07-Jul-2000 (loera01 & horda03) Bug 102029
**	    For SIfopen(), in the rare (some might say impossible) case that 
**          SYS$CREATE fails due to %RMS-E-FLK error, go into wait/retry loop.
**          Added routine log_error_message() to write any severe open errors
**          to the error log.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	28-feb-01 (kinte01)
**	    Use VMS provided header file (devdef.h) instead of Piccolo
**	    modified version (fabdev.h)
**     11-Jul-2001 (bolke01)
**              The IICSP.LOG files is not opened with shared READ access because 
**              incorrect filename is being checked. 
**              Because the CSP log file can have the node name included need to 
**              check the start of the filename. 
**              (105951)
**	27-jul-2004 (abbjo03)
**	    Rename si.h to silocal.h to avoid conflict with CL hdr!hdr file.
**	21-jul-2005 (abbjo03)
**	    Split SIfopen/SIopen to siopen.c.
**	27-jul-2005 (abbjo03)
**	    Split SIwrite to siwrite.c.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**      30-Dec-2009 (horda03) Bug 123091/123092
**          Open all TEXT files with SHARED read.
**          For TEXT filescw created with SHARED read, indicate that SIflush
**          calls will only flush the stream periodically. This dramatically
**          improves the performance of VMS tracing output.
*/


int	$fill(FILE *);
int	$flush(FILE *, int);

FUNC_EXTERN i4 rms_get(RABDEF *);
FUNC_EXTERN i4 rms_put(RABDEF *);


/*
**	SIclose	- close an open file
**
**	Parameters:
**		fp	- the file pointer
**
**	Returns:
**		0	-- okay
**		FAIL	-- error
**
**      History:
**          30-Dec-2009 (horda03) Bug 123091/123092
**             If the stream has a pending flush, then
**             do it before closing the file.
*/

STATUS
SIclose(fp)
FILE	*fp;
{
	int	ast_status;
        FILEX   *filex;

	if (fp < SI_iobptr || fp >= &SI_iobptr[_NFILE] || ((fp->_flag & _IOPEN) == 0))
	    return (FAIL);

	ast_status = sys$setast(0);

        filex = (FILEX *)fp->_file;

        if (filex->flush_pending)
        {
#ifdef OS_THREADS_USED
           /* On an OS threaded system, so need to prevent the
           ** flush timer scanning the flush list until the
           ** entry has been removed.
           */
           if (CS_is_mt())
              CSp_semaphore( SEM_EXCL, &SI_pending_mutex );

           if (filex->flush_pending)
           {
#endif
              filex->link.next->link.prev = filex->link.prev;
              filex->link.prev->link.next = filex->link.next;

#ifdef OS_THREADS_USED
           }
           if (CS_is_mt())
              CSv_semaphore( &SI_pending_mutex );
#endif
        }

	if ((fp->_flag & _IOWRT) && !(fp->_flag & _SI_RACC_OPT))
	{
	    if (fp->_flag & _IORACC)
	    {
		/* back patch the file size before closing */

		SIfseek(fp,0L,SI_P_START);
		*((i4 *)(fp->_ptr - 4)) = *((i4 *)(fp->_base - 8));
		$flush(fp, TRUE);
	    }
	    else
            {
		$flush(fp, TRUE);
            }
	}

	MEfree((PTR)((FILEX *)fp->_file)->fx_fab.fab$l_ctx);
	sys$disconnect(&((FILEX *)fp->_file)->fx_rab);
	sys$close(&((FILEX *)fp->_file)->fx_fab);
	fp->_flag = 0;

	if (ast_status == SS$_WASSET)
		sys$setast(1);

	return (OK);
}

/*
**	SIisopen	- Is a file still open
**
**/

bool
SIisopen(fp)
FILE	*fp;
{
	if (fp < SI_iobptr || fp >= &SI_iobptr[_NFILE] || ((fp->_flag & _IOPEN) == 0))
		return (FALSE);
	return (TRUE);
}

/*
**	SIgetrec - read next record from file
**
**	Parameters:
**		buf	- the buffer
**		n	- the number of characters to read
**		fp	- the file pointer
**
**	Returns:
**		OK	- success
**		FAIL	- error
*/

STATUS
SIgetrec(buf, n, fp)
char	*buf;			/* put record here */
i4	n;			/* no more than this many characters */
FILE	*fp;			/* file to read from */
{
	int	len;
	int	left;
	int	i;

        if(fp->_flag & _SI_RACC_OPT)
                return(FAIL);

	len = n - 1;

	if ((left = fp->_cnt) == 0)
	{
		if ((i = $fill(fp)) != OK)
			return (i);
		left = fp->_cnt;
	}
	if (len >= left)
	{
		MEcopy(fp->_ptr, left, buf);
		fp->_ptr += left;
		fp->_cnt = 0;
		buf[left] = 0;
	}
	else
	{
		MEcopy(fp->_ptr, len, buf);
		fp->_ptr += len;
		fp->_cnt -= len;
		buf[len] = 0;
	}
	return (OK);
}

/*
**	SIputrec	- write a NULL terminated string to a file
**
**	Parameters:
**		buf	- the buffer to write from
**		fp	- the stream
**
**	Returns:
**		OK	- success
**		FAIL	- error
*/

STATUS
SIputrec(buf, fp)
char	*buf;
FILE	*fp;
{
	i4	dummy;		/* a dummy variable */

        if(fp->_flag & _SI_RACC_OPT)
                return(FAIL);

	return(SIwrite(STlength(buf), buf, &dummy, fp));
}

/*
**      SIflush_pending - flush pending buffers
**
**      Description:
**         Scan the list of  buffers waiting to be flushed.
**         If there are no threads in a critical section, then
**         flush the buffer, otherwise requeue the flush.
**
**         This function runs within an AST.
**
**      Parameters:
**              None
**
**      Returns:
**              None
**
**      History:
**          30-Dec-2009 (horda03) Bug 123091/123092
**              Created.
*/
/* Flush at 10 second intervals */

static i4 delay_time [2] = { -1 * 10 * 10 * 1000 * 1000, -1 };

volatile i4 SI_flush_timer_set = 0;

FILEX_LINK SI_pending_list = { &SI_pending_list, &SI_pending_list };

#ifdef OS_THREADS_USED
CS_SEMAPHORE SI_pending_mutex;
i4 SI_init_pending_mutex = 1;
#endif

static void
SIflush_pending()
{
   FILEX *filex;
   FILEX *next;

#ifdef OS_THREADS_USED
   /* In an OS threaded process, running in an AST
   ** isn't sufficient to protect the pending list,
   ** so take a mutex.
   */
   if (CS_is_mt())
      CSp_semaphore( SEM_EXCL, &SI_pending_mutex );
#endif

   /* Traverse the queue, once we've got the first buffer,
   ** the queue header is cleared.
   */
   for (filex = SI_pending_list.next, 
           SI_pending_list.next = SI_pending_list.prev = &SI_pending_list; 
        filex != &SI_pending_list; filex = next)
   {
      next = filex->link.next;

#ifdef OS_THREADS_USED
      if (CS_is_mt())
      {
         if (filex->busy_buf)
         {
            /* Don't flush now, do it next time by adding to the
            ** pending queue once more.
            **
            ** busy_buf only set when mutex set, rather than
            ** wait for the mutex move on to the next one.
            */
            filex->link.next = SI_pending_list.prev->link.next;
            filex->link.prev = SI_pending_list.prev;
            filex->link.prev->link.next = SI_pending_list.prev = filex;

            continue;
         }

         CSp_semaphore( SEM_EXCL, (CS_SEMAPHORE *)fp->flush_mutex );

         /* filex->busy_buf is now clear */
      }
#endif
      /* For internal threads, AST will be disabled when the Queue is being
      ** amended, or the buffer is "busy".
      */

      sys$flush( &filex->fx_rab );

      filex->link.next = NULL;

      filex->flush_pending = 0;

#ifdef OS_THREADS_USED
      if (CS_is_mt())
         CSv_semaphore( (CS_SEMAPHORE *) fp->flush_mutex );
#endif
   }

   if (SI_pending_list.next != &SI_pending_list)
   {
      /* 1 or more buffers not flushed, so requeue. */
      sys$setimr( 0,  &delay_time , SIflush_pending, &SI_pending_list, 0);
   }
   else
   {
      /* No buffers waiting to be flushed, so timer inactive */
      SI_flush_timer_set = 0;
   }

#ifdef OS_THREADS_USED
   if (CS_is_mt())
      CSv_semaphore( &SI_pending_mutex );
#endif
}


/*
**	SIflush	- flush buffer
**
**	Parameters:
**		fp	- a valid file pointer
**
**	Returns:
**		OK	- success
**		FAIL	- error
**
**	History:
**	    29-aug-95 (albany)
**		We now flush only writable (output) buffers during an 
**		SIflush() since flushing input buffers dumps the output 
**		buffer to the screen.
**          30-Dec-2009 (horda03) Bug 123091/123092
**              Delay the sys$flush call if possible.
*/

STATUS
SIflush(fp)
FILE	*fp;
{
	STATUS	status;

        /*
        **      This is a No-Op since all writes
        **      are actual writes, nothing
        **      is held in buffers.
        */

        if(fp->_flag & _SI_RACC_OPT)
        {
                return(OK);
        }

	if (fp < SI_iobptr || fp >= &SI_iobptr[_NFILE] || ((fp->_flag & _IOPEN) == 0))
		return (FAIL);

	/*
	** seek to current position for RACC files.  $flush by itself
	** destroys file position.  Seek must flush to work.
	*/

	if ((fp->_flag & _IORACC) != 0)
	return (SIfseek(fp,0L,SI_P_CURRENT));

	/*  Flush buffer to disk and force file header. */

	/*
	** We only want to flush output buffers, since flushing
	** input buffers (such as stdin) dumps the output buffer
	** to the screen.  If the _IOWRT bit is set in _flag, it's 
	** an output buffer.
	*/
	if ( _IOWRT == ( _IOWRT & fp->_flag ) )
	{
            FILEX *filex = (FILEX *)fp->_file;
            i4    ast_status;

	    status = $flush(fp, FALSE);
	    if (status < 0)
            {
		return (FAIL);
            }

            if (filex->delay_flush)
            {
#ifdef OS_THREADS_USED
               if (CS_is_mt())
               {
                  CSp_semaphore( SEM_EXCL, (CS_SEMAPHORE *)fp->flush_mutex);
               }
               else
#endif
                  ast_status = sys$setast(0);

               if (!filex->flush_pending)
               {
#ifdef OS_THREADS_USED
                  if (CS_is_mt())
                  {
                     CSv_semaphore( (CS_SEMAPHORE *) fp->flush_mutex);

                     CSp_semaphore( SEM_EXCL, &SI_pending_mutex );
                  }
#endif
                  filex->flush_pending++;

                  filex->link.next = SI_pending_list.prev->link.next;
                  filex->link.prev = SI_pending_list.prev;
                  filex->link.prev->link.next = SI_pending_list.prev = filex;

                  if ( !SI_flush_timer_set )
                  {
                     SI_flush_timer_set++;

                     sys$setimr( 0,  &delay_time , SIflush_pending, &SI_pending_list, 0);
                  }

#ifdef OS_THREADS_USED
                  if (CS_is_mt())
                  {
                     CSv_semaphore( &SI_pending_mutex );
                  }
                  else
#endif
                     if (ast_status == SS$_WASSET)
                            sys$setast(1);

                  return OK;
               }

#ifdef OS_THREADS_USED
               else if (CS_is_mt())
               {
                  CSv_semaphore( (CS_SEMAPHORE *) fp->flush_mutex);
               }
               else
#endif
                  if (ast_status == SS$_WASSET)
                            sys$setast(1);

               return OK;
            }
	}
	if ((sys$flush(&((FILEX *)fp->_file)->fx_rab) & 1) == 0)
	    return (FAIL);
	return (OK);
}

/*
**	SIread	- read from a file
**
**	Parameters:
**		fp	- the file pointer
**		buf	- the buffer
**		n	- the number of characters to read
**		count	- the number of characters read
**
**	Returns:
**		ENDFILE	- end of file
**		FAIL	- error
*/
STATUS
SIread(fp, n, count, buf)
FILE	*fp;
i4	n;
i4	*count;
char	*buf;
{
	i4	len;
	i4	left;
	STATUS	status;

        if(fp->_flag & _SI_RACC_OPT)
        {
                FILEX *fx;

                fx = (FILEX *) fp->_file;


                /*
                **      Using Optimized GCN RACC files, read is unbuffered
                **      call to RMS get. Ignore user specified bytes
                **      return full record size.
                */

                if ((rms_get(&fx->fx_rab) & 1) == 0)
                    return (ENDFILE);

                MEcopy(fx->fx_rab.rab$l_rbf,fx->fx_rab.rab$w_rsz, buf);
                *count = fx->fx_rab.rab$w_rsz;
                /*      Increment the RMS relative Record Number        */
                ++(*((i4 *)(fx->fx_rab.rab$l_kbf)));
                return(OK);
        }

	len = n;
	while (len)
	{
		while ((left = fp->_cnt) == 0)
		{
			if ((status = $fill(fp)) != OK)
			{
				/*
				** for RACC files, decrement key buffer which
				** $fill incremented
				*/
				if (fp->_flag & _IORACC)
				{
					FILEX *fx;
					i4 *kbuf;

					fx = ((FILEX *)fp->_file);
					kbuf = (i4 *)(fx->fx_rab.rab$l_kbf);
					--(*kbuf);
				}
				if (status == FAIL)
					return (FAIL);
				*count = n - len;
				if (*count == 0)
					return (ENDFILE);
				return (OK);
			}
		}
		if (len > left)
		{
			MEcopy(fp->_ptr, left, buf);
			len -= left;
			fp->_ptr += left;
			fp->_cnt = 0;
			if (((FILEX *)fp->_file)->fx_fab.fab$l_dev & DEV$M_TRM)
			{
				n -= len;
				return (OK);
			}
			buf += left;
		}
		else
		{
			MEcopy(fp->_ptr, len, buf);
			fp->_ptr += len;
			fp->_cnt -= len;
			break;
		}
	}
	*count = n;
	return (OK);

}

/*
**	SIfseek - seek to specific byte in open RACC file
**
**	Parameters:
**		file		- file pointer
**		offset		- byte offset to seek to
**		position	- file position offset is relative to:
**					beginning, end, or current position.
**
**	Returns:
**		OK		- seek worked
**		FAIL		- offset out of range, wrong file type.
*/

SIfseek(
FILE *fp,
OFFSET_TYPE offset,
i4 position)
{
	FILEX *fx;
	i4 firstbytes;
	i4 recnum;
	i4 totalsize;
	OFFSET_TYPE currofs;
	OFFSET_TYPE newofs;
	i4 *crec;
	char *newpos;

	if ((fp->_flag & _IORACC) == 0)
	{
	    /* rewind is only positioning allowed for text file */
	    if ( offset == 0 && position == SI_P_START )
	    {
		fp->_ptr = fp->_base;
		fp->_cnt = 0;
		fx = (FILEX *) fp->_file;
		sys$rewind(&fx->fx_rab);
		$fill(fp);
		return(OK);
	    }
	    else		
            {
               /* Allow cursor movement within current record */
               if ( (position != SI_P_CURRENT ) ||
                    (offset  > (OFFSET_TYPE) fp->_cnt)   ||
                    ( (offset < 0) && (fp->_ptr + offset < fp->_base) ) )

                  return (SI_BAD_SYNTAX);

               fp->_cnt -= offset;
               fp->_ptr += offset;

               return(OK);
           }
	}

        if(fp->_flag & _SI_RACC_OPT)
        {
                /*
                **      Use GCN optimized RACC access.
                **      Convert Byte number to a record number
                **      Then RMS Find that record number
                */

                fx = (FILEX *) fp->_file;
                currofs = *((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf));
                totalsize = *((i4 *)(fp->_base - 8));
                switch(position)
                {
                        case SI_P_START:
                                if (offset < fx->fx_fab.fab$w_mrs)
                                {
                                        /*
                                       ** optimize for first record
                                        */

                                        (*((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf))) = 1;
                                        return(OK);
                                }
                                else
                                {
                                        /*
                                        ** Calculate record Number
                                        */
                                        recnum = offset/(fx->fx_fab.fab$w_mrs) + 1;
                                        (*((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf))) = recnum;
                                        return(OK);
                                }
                                break;
                        case SI_P_CURRENT:
                                recnum = currofs + offset/(fx->fx_fab.fab$w_mrs ) + 1;
                                (*((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf))) = recnum;
                                return(OK);
                                break;
                        case SI_P_END:
                                break;
                        default:
                                return(SI_BAD_SYNTAX);
                }
                return(OK);
        }


	/*
	** if write is allowed, make sure EVERYTHING is in the file, including
	** the trailing part record.  We want seek backwards to work!  We will
	** read current record back in, so we'll be OK if we're still in the
	** last record.  This also updates total size to reflect partially
	** written records.  We get current offset FIRST because $flush will
	** goof it up, and we might need it.  If we don't SIftell is cheap.
	**
	** currofs is no longer valid after the $flush, so we need a second
	** SIftell to do the 'same record' optimization.
	*/
	currofs = SIftell(fp);
	if (fp->_flag & _IOWRT)
	{
		$flush(fp,TRUE);
		newofs = SIftell(fp);
	}
	else
	{
		newofs = currofs;
	}

	/*
	** turn everything into a seek from the beginning.  totalsize
	** needed to check end.
	*/
	totalsize = *((i4 *)(fp->_base - 8));
	switch(position)
	{
	case SI_P_START:
		break;
	case SI_P_CURRENT:
		offset += currofs;
		break;
	case SI_P_END:
		offset += totalsize;
		break;
	default:
		return(SI_BAD_SYNTAX);
	}
	if (offset < 0 || offset > totalsize)
		return(FAIL);

	fx = ((FILEX *)fp->_file);

        /*
        ** If we're still in the current record, we can just adjust
        ** the offset.
        */
        newpos = fp->_ptr - (newofs - offset);
        if (newpos >= fp->_base && newpos < fp->_ptr + fp->_cnt)
        {
            fp->_ptr = newpos;
            fp->_cnt += (newofs - offset);
            return OK;
        }

	/*
	** compute desired record number, subtract bytes from offset
	*/
	recnum = 1;
	if (offset >= (firstbytes = fx->fx_fab.fab$w_mrs - RACCLEADING))
	{
		offset -= firstbytes;
		recnum += offset/(fx->fx_fab.fab$w_mrs) + 1;
		offset %= fx->fx_fab.fab$w_mrs;
	}
	else
		offset += RACCLEADING;

	/*
	** read proper record into memory - we initialize the pointers
	** even when reading a new record because of a "boundary" case -
	** position after last byte of file (seek prior to append), which
	** happens to be the first first byte of a new record ($fill will fail).
	** Key buffer gets desired record - 1 because we increment before read.
	*/
	crec = (i4 *)(fx->fx_rab.rab$l_kbf);
	fp->_cnt = fx->fx_rab.rab$w_rsz;
	fp->_ptr = fx->fx_rab.rab$l_rbf;
	*crec = recnum - 1;
	$fill(fp);

	/* adjust by offset into record */
	fp->_cnt -= offset;
	fp->_ptr += offset;
	return (OK);
}

/*
**	SIftell - get current file offset in open RACC file.
**
**	Parameters:
**		file		- file pointer
**
**	Returns:
**		position	- current byte offset from beginning of file
**
**		OK		- position returned
**		FAIL		- wrong file type.
*/

OFFSET_TYPE
SIftell(
FILE *fp
)
{
	OFFSET_TYPE offset;
	FILEX *fx;

	if ((fp->_flag & _IORACC) == 0)
		return(SI_BAD_SYNTAX);

        if(fp->_flag & _SI_RACC_OPT)
        {
                /*
                **      For GCN optimized RACC files the return value
                **      From SIFtell is the current RMS Relative
                **      Record Number.                
		**      SIFtell isn't used by GCN, but since I've optimized
                **      SIfseek, it seems a good idea to optimize this.
                */

                fx = (FILEX *) fp->_file;
                offset = *((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf));
                return(offset);
        }
        else
        {

		/*
		** offset = record size times number of complete records
		** read / written (current key - 1), plus bytes into current
		** record minus the lead bytes.  Use pointer difference to
		** count current bytes because fp->_cnt may only measure to
		** end of file instead of end of record.
		*/
		fx = ((FILEX *)fp->_file);
		offset = *((OFFSET_TYPE *)(fx->fx_rab.rab$l_kbf)) - 1;
		offset *= fx->fx_fab.fab$w_mrs;
		offset += (fp->_ptr - fp->_base) - RACCLEADING;
		return (offset);
        }
}

/*
**	Character support routines:
**
*/

/*
**      This is called only by the SIgetc macro.
**
**      12/91 (Mike S) Make this work for RACC files.
*/
_filbuf(f)
register FILE	*f;
{
        if ((f->_flag & _IORACC) == 0)
        {
                /* Not a RACC file */
                if ($fill(f) == OK)
                        return (SIgetc(f));
                return (EOF);
        }
        else
        {
                STATUS status;
                i4 count;
                char *ch;

                /* A RACC file */
                f->_cnt++;              /* Restore this;SIgetc decremented it */
                status = SIread(f, (i4)1, &count, &ch);
                if (status == OK && count == 1)
                        return ch;
                else
                        return EOF;
        }
}

/*
**      This is called only by the SIputc macro.
**
**      12/91 (Mike S)
**      Make this work for RACC files.  Note that this isn't optimized for
**      appending to a RACC file using SIputc; doing that will call SIwrite
**      for every character, due to the fact that the _cnt field of a RACC FILE
**      will be 0 at end of file, even if there's room left in the buffer.
*/
_flsbuf(c, f)
int		c;
register FILE	*f;
{

        if ((f->_flag & _IORACC) == 0)
        {
                if ($flush(f, FALSE))
                        SIputc(c, f);
                else
                        return EOF;
        }
        else
        {
                STATUS status;
                i4 count;

                /* A RACC file */
                f->_cnt++;              /* Restore this;SIputc decremented it */
                status = SIwrite((i4)1, &c, &count, f);
                if (status == OK)
                        return c;
                else
                        return EOF;
        }
}

/*
**	$FLUSH	- type specific flush handler
**
**	NOTES ON RACC:
**		$flush always writes out the record, and positions to NEXT
**		record.  This means that our current pointer may have to be
**		recovered by the caller - see SIfseek.  This is why a "flush"
**		operation on a RACC file is REALLY a "seek"
**
**	Parameters:
**		fp	- the file pointer
**		really  - will force sys$flush to be called if true
**
**	Returns:
**		n	- the size of the buffer
**		0	- end of file
**		-1	- error
**
**	History:
**		02/07/84 (lichtman) -- added flushing of files with variable length records and no attributes
**		07/26/95 (duursma) -- added code to deal with terminal devices,
**		which had been treated as carriage control devices up to now;
**		this made it impossible to output a message to the terminal
**		without a newline (eg. for a password prompt).
**		Fixes bug 68699/69765.
*/
int
$flush(
register FILE	*fp,
int		really)
{
	FILEX		*fx;
	int		n;
	i4		wrsize;
	i4		*cursize;
	bool		is_term = FALSE;
	char		saved_char;
	i4		status;
        i4              ast_status;

	n = fp->_ptr - fp->_base;
	if (n == 0 && !(fp->_flag & _IORACC))
		return(0);

	fx = ((FILEX *) fp->_file);

	/*
	** RACC file - ALWAYS write record.  We may have done a seek
	** to the middle, so size update uses total size.  Record
	** key is incremented, which is superflous if call is from
	** close or SIfseek.
	*/

	if (fp->_flag & _IORACC)
	{
	    fx->fx_rab.rab$w_rsz = fx->fx_fab.fab$w_mrs;
	    fx->fx_rab.rab$l_rbf = fp->_base;
	    fx->fx_rab.rab$l_rop |= RAB$M_UIF;
	    if ((rms_put(&fx->fx_rab) & 1) == 0)
		    return (-1);

	    /* count COMPLETE records */
	    wrsize = *((i4 *)(fx->fx_rab.rab$l_kbf)) - 1;
    
	    /*
	    ** Need to increment to next record, BUT we also need
	    ** to READ next record in case we're in the middle of
	    ** the file, overwriting it.  Set _cnt and _ptr before
	    ** attempted read - a fail is OK (we're at EOF)
	    */
	    fp->_cnt = 0;
	    fp->_ptr = fx->fx_rab.rab$l_rbf;
	    $fill(fp);

	    /* point to current byte count */
	    cursize = (i4 *)(fp->_base - 8);

	    /*
	    ** compute size: complete records times record size plus
	    ** n minus leading bytes.  If this pushes us over the limit,
	    ** modify current size
	    */
	    wrsize *= fx->fx_fab.fab$w_mrs;
	    wrsize += n - RACCLEADING;
	    if (wrsize > *cursize)
		    *cursize = wrsize;

	    fp->_ptr = fp->_base;

	    return(fx->fx_fab.fab$w_mrs);
	}

	switch (fx->fx_fab.fab$b_rfm)
	{
	case FAB$C_FIX:
	    if (fp->_cnt == 0)
	    {
		fx->fx_rab.rab$w_rsz = fx->fx_fab.fab$w_mrs;
		fx->fx_rab.rab$l_rbf = fp->_base;
		if ((rms_put(&fx->fx_rab) & 1) == 0)
		    return (-1);
		fp->_ptr = fp->_base;
		fp->_cnt = fx->fx_fab.fab$w_mrs;
	    }
	    break;
		
	case FAB$C_VAR:
	case FAB$C_VFC:
	case FAB$C_STMLF:
	    if (fx->fx_fab.fab$b_rat & (FAB$M_CR|FAB$M_PRN))
		is_term = FALSE;
	    else if (fx->fx_fab.fab$l_dev & DEV$M_TRM)
		is_term = TRUE;
	    else
		break;
	    {
		register char	*p;

		/*! BEGIN BUG */
		/* the _cnt field should be reset when the _ptr */
		/* is set. This is needed for exception handling */
		/* and general neatness. */

		fp->_cnt = BUFSIZ;
		/*! END BUG */
                if (fx->delay_flush)
                {
#ifdef OS_THREADS_USED
                   if (CS_is_mt())
                   {
                      CSp_semaphore( SEM_EXCL, (CS_SEMAPHORE *) fp->flush_mutex);
                      fx->busy_buf = 1;
                   }
                   else
#endif
                      ast_status = sys$setast(0);
                }

		for (fp->_ptr = fp->_base; n > 0; ) 
		{
		    p = fp->_ptr;
		    while (--n >= 0 && *p++ != '\n');

		    if (n >= 0)
		    {
			if (is_term) {
			    ((FILEX *) fp->_file)->fx_rab.rab$w_rsz = p - fp->_ptr + 1;
			    /* include the LF and also add a CR, must blast next char */
			    saved_char = *p;
			    *p = '\r';
			} else
			    /* CR-controlled device, ignore the LF character */
			    ((FILEX *) fp->_file)->fx_rab.rab$w_rsz = p - fp->_ptr - 1;

			((FILEX *) fp->_file)->fx_rab.rab$l_rbf = fp->_ptr;
			status = rms_put(&((FILEX *) fp->_file)->fx_rab) & 1;
			if (is_term)
			    /* restore char blasted above */
			    *p = saved_char;
			if (status == 0) {
                            goto failure;
			}
			fp->_ptr = p;
		    }
		    else
		    {
			n = p - fp->_ptr;
			break;
		    }
		}
		if (n > 0 && (really || is_term || n == fx->fx_rab.rab$l_ctx))
		{
		    fx->fx_rab.rab$w_rsz = n;
		    fx->fx_rab.rab$l_rbf = fp->_ptr;
		    if ((rms_put(&fx->fx_rab) & 1) == 0)
			    goto failure;
		    n = 0;
		}	
		if (n)
			MEcopy(fp->_ptr, n, fp->_base);
		fp->_cnt = fx->fx_rab.rab$l_ctx - n;
		fp->_ptr = fp->_base + n;

                if (fx->delay_flush)
                {
#ifdef OS_THREADS_USED
                   if (CS_is_mt())
                   {
                      fx->busy_buf = 0;
                      CSv_semaphore( (CS_SEMAPHORE *) fp->flush_mutex);
                   }
                   else
#endif
                      if (ast_status == SS$_WASSET)
                            sys$setast(1);
                }
	    }
	    break;
	}
	if (really || is_term)
	    sys$flush(&fx->fx_rab);
	return (fp->_cnt);	

failure:
   if (fx->delay_flush)
   {
#ifdef OS_THREADS_USED
      if (CS_is_mt())
      {
         fx->busy_buf = 0;
         CSv_semaphore( (CS_SEMAPHORE *) fp->flush_mutex);
      }
      else
#endif
         if (ast_status == SS$_WASSET)
            sys$setast(1);
   }

   return -1;
}

/*
** Name:	$fill() -	type specific fill handler
**
** Parameters:
**	fp	- a file pointer
**
** Returns:
**	OK	- success
**	ENDFILE	- the end of file
**	FAIL	- error
**
** History:
**	4/16/86 (jhw) -- Flush both 'stderr' and 'stdout' on input from 'stdin'.
*/
int
$fill(
register FILE	*fp)
{
	FILEX		*fx;
	int		prompt = 0;
	STATUS		status = 0;
	i4		totsize;
	i4		recnum;

	fx = ((FILEX *) fp->_file);
	fx->fx_rab.rab$l_rop &= ~RAB$M_PMT;
	if (stdin == fp && (fx->fx_fab.fab$l_dev & DEV$M_TRM))
	{
	    FILE	*ofp = stderr;

	    for (;;)
	    { /* for 'stderr' and then 'stdout' */
		if ((((FILEX *)ofp->_file)->fx_fab.fab$l_dev & DEV$M_TRM) != 0)
		{	/* flush if output to terminal */
		    $flush(ofp, FALSE);
		    if (ofp->_ptr > ofp->_base)
		    { /* output buffer not empty */
			if ((fx->fx_rab.rab$l_rop & RAB$M_PMT) == 0)
			{ /* move to input prompt */
			    fx->fx_rab.rab$l_rop |= RAB$M_PMT;
			    fx->fx_rab.rab$l_pbf = ofp->_base;
			    fx->fx_rab.rab$b_psz = ofp->_ptr - ofp->_base;
			}
			else
			{ /* append to input prompt */
			    /* Assumes it's small enough to fit, i.e.,
			    **	rab$b_psz + (ofp->_ptr - ofp->_base) <= 256
			    */
			    MEcopy(ofp->_base, ofp->_ptr - ofp->_base,
				fx->fx_rab.rab$l_pbf+fx->fx_rab.rab$b_psz);
			    fx->fx_rab.rab$b_psz += ofp->_ptr - ofp->_base;
			}
			ofp->_ptr = ofp->_base;
			ofp->_cnt = ((FILEX *)ofp->_file)->fx_rab.rab$l_ctx;
		    }
		}
		if (ofp == stderr)
		    ofp = stdout;	/* do 'stdout' now */
		else
		    break;
	    }	/* end for */
	}

	/*
	** we increment RACC keys AFTER writes, so we increment
	** BEFORE reads - key is always key for current buffer.
	** We are LEAVING this incremented on fail because $flush uses
	** this to increment the key when writing at end of file.
	*/
	if (fp->_flag & _IORACC)
		++(*((i4 *)(fx->fx_rab.rab$l_kbf)));

	if ((rms_get(&fx->fx_rab) & 1) == 0)
	{
		if (fp->_flag & _IORACC)
			return (ENDFILE);
		if (fx->fx_rab.rab$l_sts == RMS$_EOF)
			return (ENDFILE);
		if (fx->fx_rab.rab$l_sts == RMS$_TNS)
		{
			/*! BEGIN BUG */
			fp->_cnt = fx->fx_rab.rab$w_rsz;
			fp->_ptr = fx->fx_rab.rab$l_rbf;
			return (OK);
			/*! END BUG */
		}
		return (FAIL);
	}

	fp->_cnt = fx->fx_rab.rab$w_rsz;
	fp->_ptr = fx->fx_rab.rab$l_rbf;

	if (fp->_flag & _IORACC)
	{
		/*
		** calculate bytes read from record number retrieved and record
		** size.  If > than total size, this is the last record -
		** adjust fp->_cnt to reflect partial data.
		*/
		recnum = *((i4 *)(fx->fx_rab.rab$l_kbf));
		totsize = *((i4 *)(fp->_base - 8));
		recnum *= fp->_cnt;
		if ((recnum -= RACCLEADING) > totsize)
			fp->_cnt -= recnum - totsize;
	}

	if (fx->fx_fab.fab$b_rat & (FAB$M_CR|FAB$M_PRN))
		fp->_ptr[fp->_cnt++] = '\n';
	if ((fx->fx_fab.fab$l_dev & DEV$M_TRM))
		fp->_ptr[fp->_cnt++] = (fx->fx_rab.rab$l_stv & 0xff) == '\r' ? '\n' : fx->fx_rab.rab$l_stv;
	return (OK);
}

/*
**	SIterminal -- return whether file is a terminal
*/
bool
SIterminal(fp)
FILE	*fp;
{
	if (!fp || !fp->_file)
		return(FALSE);
	if (((FILEX *) fp->_file)->fx_fab.fab$l_dev & DEV$M_TRM)
		return(TRUE);
	else
		return(FALSE);
}
