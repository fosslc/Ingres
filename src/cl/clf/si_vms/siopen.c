/*
** Copyright (c) 1983, 2005 Ingres Corporation
**
*/
#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include	<lo.h>
#include	<me.h>
#include	<pc.h>
#include	<st.h>
#include	<tm.h>
#include	<devdef.h>
#include	<descrip.h>
#include	<rms.h>
#include	<ssdef.h>
#include	<starlet.h>
#include	<si.h>
#include	"silocal.h"
#include        <astjacket.h>

/**
** Name:	siopen.c - open named stream
**
** Purpose:
**	Provides functions to open standard I/O files.
**
** Defines:
**	status = SIfopen(loc, mode, type, length, &file)
**	status = SIopen(loc, mode, &file)  [OBSOLESCENT]
**	
** History:
**	07-jul-2005 (abbjo03)
**	    Created by extracting from si.c.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
*/

FUNC_EXTERN	int	$fill(FILE *);

static void log_error_message(i4 err, char *fname, char mode, char *usermsg);

#ifdef OS_THREADS_USED
GLOBALREF CS_SEMAPHORE SI_pending_mutex;
#endif
GLOBALREF i4    SI_init_pending;
GLOBALREF FILEX SI_pending_list;

/*
**  SIfopen - open a file given its type and record length
**
**  Parameters:
**	location	- location of the file
**	mode		- access mode of the file:
**			  w = write, r = read, a = append, rw = update
**	file_type	- the type of file (SI_TXT, SI_BIN, SI_VAR or SI_RACC)
**	rec_length	- the record length
**	desc		- value to get file pointer
**
**  Returns:
**	OK		- file is open
**	FAIL		- file open failed, desc is not valid
**
**  History:
**	26-jul-95 (duursma)
**	    When opening a terminal device, do not set the DEV$M_CR
**	    in the FAB, which would cause the terminal to act as a
**	    carriage controlled device, making it impossible to do
**	    things like prompt messages.  Fixes bug 68699/69765.
**	8-Sep-1995 (dougb) bug 65193
**	    Ensure that files are not opened exclusively.  Corrects a
**	    problem with sharing of files opened for logging.
**	8-Apr-1996 (mckba01) bug 73743, 71092, 72768i
**	    Remove code that sets the maximum record size
**	    for text files (TXT_FILE). Add check on rec_length
**	    to determine if the user buffer needs to be made larger.
**	3-jun-1996 (angusm)
**	    Fix to 65193 kills performance of other users
**	    of SI files, e.g. COPY TABLE to FILE, ESQL. Redo.
**	21-Aug-2000 (horda03)
**	    The ACP, RCP and II_DBMS log files will be opened shared
**	    read. (102372)
**	7-Jul-2000 (loera01) Bug 102029
**	    If SYS$CREATE fails due to "file locked by another use"
**	    error, go into a five-second wait/retry loop.  Log
**	    severe open errors to the error log.
**	11-Jul-2001 (bolke01)
**	    The CSP log files will be opened shared read. (105951)
**	27-jul-2005 (abbjo03)
**	    Save the file type and logical record length.
**      30-Dec-2009 (horda03) Bug 123091/123092
**          When opening TEXT files for writing, indicate that the
**          SYS$FLUSH calls should be done periodically.
*/
STATUS
SIfopen(
LOCATION	*location,
char		*mode,
i4		file_type,
i4		rec_length,
FILE		**desc)
{
    FILE	*f;
    FILEX	*p;
    i4		fd;
    i4		i;
    int		ast_value;
    char	modec;
    char	*name;
    char	nam_name[256];
    char	null_buf[MAX_LOC];
    NAMDEF	nam;
    char	cbuf[RACCMLEN+1];
    int		GcnOptFlag = 0;
    STATUS	status;
    i4		flags = 0;
    LOINFORMATION	locinfo;

#ifdef OS_THREADS_USED
    if (SI_init_pending_mutex)
    {
       SI_init_pending_mutex = 0;

       CSw_semaphore( &SI_pending_mutex,  CS_SEM_SINGLE, "SI PENDING" );  
    }
#endif

    if (file_type == GCN_RACC_FILE)
    {
	/*
	** Determine if this function is being called by the GCN for
        ** GCN Optimized RACC file access.
	**
	** To minimize impact on customers and to avoid causing them
	** unecessary confusion, if the optimization flag is set and mode
	** is not write (w), we should check that the file being opened for
	** read or update (r, rw, a) is actually an optimized file.  This
	** case will occur if they have not deleted the files in their name
	** server database when installing the Modified GCN.  This will be
	** achieved by reading the first record in the file and checking
	** for the RACCMAGIC characters.
	*/
	GcnOptFlag = 1;
	file_type = SI_RACC;
    }

    /*
    ** convert mode string to single character modec -
    ** "rw" becomes 'u'
    */
    switch (modec = *mode)
    {
    case 'r':
	switch (*(mode + 1))
	{
	case 'w':
	    if (file_type != SI_RACC)
		return SI_BAD_SYNTAX;
	    modec = 'u';
	    break;
	case '\0':
	    break;
	default:
	    return SI_BAD_SYNTAX;
	}
    case 'w':
	break;
    case 'a':
	/*
	** if file doesn't exist for "append", turn it into
	** a write.
	*/
	if (LOinfo(location, &flags, &locinfo) != OK)
	    modec = 'w';
	break;
    default:
	return SI_BAD_SYNTAX;
    }

    LOtos(location, &name);
    ast_value = sys$setast(0);

    /*
    **	Search for a free file descriptor.
    */
    for (;;)
    {
	for (fd = 0, f = &SI_iobptr[0]; fd < _NFILE && (f->_flag & _IOPEN);
		++fd, ++f)
	    ;
	if (fd >= _NFILE)
	{
	    char msg[64];

	    STprintf(msg, "Process has more than %d files open", _NFILE);
	    log_error_message(0,
		(location && location->string && *location->string) ?
		location->string : "<Null>", modec, msg);
	    break;
	}

	f->_ptr = f->_base;
	f->_type = (char)file_type;
	if (f->_file == NULL)
	{
	    /*
	    ** We're now allocating the FILEX dynamically.
	    ** We won't bother deallocating it at file close, since
	    ** the application will likely open another file soon.
	    */
	    f->_file = MEreqmem(0, sizeof(FILEX),  TRUE, &status);
	    if (f->_file == NULL)
	    {
		if (ast_value == SS$_WASSET)
		    sys$setast(1);
		return status;
	    }
#ifdef OS_THREADS_USED
            {
               char mutex_name [CS_SEM_NAME_LEN+1];

               f->flush_mutex = (PTR) MEreqmem(0, sizeof(CS_SEMAPHORE), TRUE, &status);

               if (f->flush_mutex == NULL)
               {
                  if (ast_value == SS$_WASSET)
                    sys$setast(1);
                  return status;
               }
               STprintf( mutex_name, "SI_flush_%d", fd );
               CSw_semaphore( (CS_SEMAPHORE *) f->flush_mutex, CS_SEM_SINGLE, mutex_name );
            }
#endif
	}
	p = (FILEX *)f->_file;

	/* Initialize the RMS fab */

	MEfill(sizeof(*p), '\0', (PTR)p);

	p->rec_length = rec_length;
	p->fx_fab.fab$b_bid = FAB$C_BID;
	p->fx_fab.fab$b_bln = FAB$C_BLN;
	p->fx_fab.fab$b_fns = STlength(p->fx_fab.fab$l_fna = name);

	f->_cnt = 0;

	if (file_type == SI_RACC)
	{
	    /*
	    ** Whatever the user's mode, RACC files must be able to
	    ** support "read" to allow seeking by reading an old
	    ** record back into memory.  Rec_length has to be long
	    ** enough for RACCLEADING bytes in first record.
	    */
	    if (rec_length < RACCLEADING)
		rec_length = RACCLEADING;
	    if (modec == 'r')
		p->fx_fab.fab$b_fac = FAB$M_GET;
	    else
	    {
		p->fx_fab.fab$b_fac = FAB$M_GET | FAB$M_PUT | FAB$M_UPD;
		p->xab_rdt.xab$b_cod = XAB$C_RDT;
		p->xab_rdt.xab$b_bln = XAB$C_RDTLEN;
		p->fx_fab.fab$l_xab = &(p->xab_rdt);
	    }

	    p->fx_fab.fab$b_org = FAB$C_REL;
	    p->fx_fab.fab$b_rfm = FAB$C_FIX;
	    p->fx_fab.fab$w_mrs = rec_length;
	}
	else
	{
	    p->fx_fab.fab$b_org = FAB$C_SEQ;
	    p->fx_fab.fab$l_fop = FAB$M_SQO;
	    if (modec == 'r')
	    {
		p->fx_fab.fab$b_fac = FAB$M_GET;
		p->fx_fab.fab$b_shr = FAB$M_SHRGET | FAB$M_SHRPUT | FAB$M_UPI;
	    }
	    else
	    {
		p->fx_fab.fab$b_fac = FAB$M_PUT;
		p->fx_fab.fab$b_rat = 0;
		if (file_type == SI_TXT)
		{
		    p->fx_fab.fab$b_rfm = FAB$C_VAR;
		    p->fx_fab.fab$b_rat = FAB$M_CR;

                    p->delay_flush = 1;
                    p->fx_fab.fab$b_shr  = FAB$M_SHRGET | FAB$M_UPI;

		    if (rec_length > SI_MAX_TXT_REC || rec_length < 0)
		    {
			if (ast_value == SS$_WASSET)
			    sys$setast(1);
			return FAIL;
		    }
		}
		else if (file_type == SI_BIN)
		{
		    if (rec_length <= MAX_RMS_SEQ_RECLEN)
		    {
			p->fx_fab.fab$b_rfm = FAB$C_FIX;
			p->fx_fab.fab$w_mrs = rec_length;
		    }
		    else
		    {
			p->fx_fab.fab$b_rfm = FAB$C_VAR;
			p->fx_fab.fab$w_mrs = MAX_RMS_SEQ_RECLEN;
		    }
		}
		else if (file_type == SI_VAR)
		    p->fx_fab.fab$b_rfm = FAB$C_VAR;
		else
		    break;
	    }
	}

	/* Initialize the RMS rab */
	p->fx_rab.rab$b_bid = RAB$C_BID;
	p->fx_rab.rab$b_bln = RAB$C_BLN;
	p->fx_rab.rab$l_fab = &p->fx_fab;
	if (modec == 'a' && file_type != SI_RACC)
	    p->fx_rab.rab$l_rop = RAB$M_EOF;

	/* Initialize the RMS nam */
	MEfill(sizeof(nam), '\0', (PTR)&nam);
	nam.nam$b_bid = NAM$C_BID;
	nam.nam$b_bln = NAM$C_BLN;
	nam.nam$l_esa = nam_name;
	nam.nam$b_ess = (unsigned char)sizeof(nam_name);
	p->fx_fab.fab$l_nam = &nam;

	/* See if we can open it */
	if (modec != 'w')
	    sys$open(&p->fx_fab);
	else
	{
	    for (i = 0; i < 10; i++)
	    {
		sys$create(&p->fx_fab);
		if (p->fx_fab.fab$l_dev & DEV$M_TRM)
		{
		    /* it's a terminal...turn off carriage control */
		    /* tediously close the file, trash the M_CR bit, */
		    /* then recreate the file */
		    sys$close(&p->fx_fab);
		    p->fx_fab.fab$b_rat &= ~FAB$M_CR;
		    sys$create(&p->fx_fab);
		    break;
		}
		/*
		** File locked on create?  Wait and try again.
		*/
		if (p->fx_fab.fab$l_sts == RMS$_FLK)
		    PCsleep(1000);
		else
		    break;
	    }
	}
	p->fx_fab.fab$l_nam = NULL;

	if ((p->fx_fab.fab$l_sts & 1) == 0)
	    break;

	/* It's a file: see if we support it */
	for (;;)
	{
	    int	buffer_size;

	    /*
	    ** for RACC, allocate a size and a key buffer
	    ** fab$l_ctx is the allocated address for free -
	    ** f->_base, etc the actual i/o buffer area.  l_ctx,
	    ** or f->_base - 8, contains the size,  f->_base - 4
	    ** the key buffer.  rab$l_ctx unused for RACC files.
	    */
	    if (file_type == SI_RACC)
	    {
		if (p->fx_fab.fab$b_org != FAB$C_REL)
		    break;
		if (p->fx_fab.fab$b_rfm != FAB$C_FIX)
		    break;
		buffer_size = p->fx_fab.fab$w_mrs + 8;
	    }
	    else
	    {
		if (p->fx_fab.fab$b_org != FAB$C_SEQ ||
			(p->fx_fab.fab$b_rfm != FAB$C_VAR &&
			p->fx_fab.fab$b_rfm != FAB$C_FIX &&
			p->fx_fab.fab$b_rfm != FAB$C_VFC &&
			p->fx_fab.fab$b_rfm != FAB$C_STMLF) ||
			(p->fx_fab.fab$b_rat != 0 &&
			(p->fx_fab.fab$b_rat & FAB$M_CR) == 0 &&
			(p->fx_fab.fab$b_rat & FAB$M_PRN) == 0))
		    break;

		buffer_size = BUFSIZ;
		if (rec_length > buffer_size)
		    buffer_size = rec_length;
		if (p->fx_fab.fab$w_mrs > BUFSIZ)
		    buffer_size = p->fx_fab.fab$w_mrs;
		if (p->fx_fab.fab$l_dev & DEV$M_TRM)
		    buffer_size = 256;
		if (p->fx_fab.fab$b_rfm == FAB$C_FIX)
		    buffer_size = p->fx_fab.fab$w_mrs;

		p->fx_rab.rab$l_ctx = buffer_size;
		if (modec != 'r')
		    f->_cnt = buffer_size;
	    }

	    f->_base = MEreqmem(0, buffer_size, FALSE, NULL);
	    if (f->_base == NULL)
		break;
	    p->fx_fab.fab$l_ctx = (unsigned int)f->_base;

	    if (file_type == SI_RACC)
	    {
		p->fx_rab.rab$l_kbf = f->_base + 4;
		if (modec != 'w')
		    *((i4 *)(p->fx_rab.rab$l_kbf)) = 0;
		else
		    *((i4 *)(p->fx_rab.rab$l_kbf)) = 1;
		f->_base += 8;
		p->fx_rab.rab$l_ubf = f->_base;
		p->fx_rab.rab$w_usz = buffer_size - 8;
		p->fx_rab.rab$b_ksz = 4;
		p->fx_rab.rab$b_rac = RAB$C_KEY;
		f->_cnt = buffer_size - 8;
	    }
	    else
	    {
		p->fx_rab.rab$l_ubf = f->_base;
		p->fx_rab.rab$w_usz = buffer_size;
	    }

	    if ((sys$connect(&p->fx_rab) & 1) == 0)
		break;

	    /*
	    ** flags based on user modes
	    */
	    f->_flag = _IOWRT | _IOPEN;
	    if (modec == 'r')
		f->_flag = _IOREAD | _IOPEN;
	    if (modec == 'u')
		f->_flag |= _IOREAD;
	    if (file_type == SI_RACC)
		f->_flag |= _IORACC;

	    *desc = f;
	    f->_ptr = f->_base;
	    if (ast_value == SS$_WASSET)
		sys$setast(1);

	    /*
	    ** For a RACC file obtain the size in bytes from the front
	    ** of the file, or write 0 if just created.  We assume
	    ** buffer size >= RACCLEADING bytes.
	    **
	    ** On append mode we do a seek to the end, and if it's a new
	    ** file we actually flush the zero record to disk so that a
	    ** later open will find the correct magic in the file, even
	    ** without a close.
	    */
	    if (file_type == SI_RACC)
	    {
		if (!GcnOptFlag)
		{
		    if (modec == 'w')
		    {
			STlcopy(RACCMAGIC, f->_ptr, RACCMLEN);
			*((i4 *)(f->_ptr + RACCMLEN)) = 0;
			*((i4 *)(f->_base - 8)) = 0;
		    }
		    else
		    {
			i4 realsize;

			if ($fill(f) != OK)
			    break;
			STlcopy(f->_ptr, cbuf, RACCMLEN);
			if (STcompare(cbuf, RACCMAGIC) != 0)
			    break;
			realsize = *((i4 *)(f->_ptr + 2));
			*((i4 *)(f->_base - 8)) = realsize;
			if ((realsize + RACCLEADING) > p->fx_fab.fab$w_mrs)
			    f->_cnt = p->fx_fab.fab$w_mrs;
			else
			    f->_cnt = realsize + RACCLEADING;
		    }
		    f->_ptr += RACCLEADING;
		    f->_cnt -= RACCLEADING;
		}
		else if (modec != 'w')
		{
		    /*
		    ** Check first record to determine if it is an
		    ** optimized file or not.
		    */
		    i4 realsize;

		    if ($fill(f) != OK)
		    {
			/*
			** Read failed indicating no records in the file.
			** This does not happen with existing RACC files
			** the RACCMAGIC bits are always placed in the
			** first record upon creation of the file.
			**
			** Set flag to indicate use of GCN Optimized RACC
			** files.
			*/
			f->_flag |= _SI_RACC_OPT;
		    }
		    else
		    {
			STlcopy(f->_ptr, cbuf, RACCMLEN);
			if (STcompare(cbuf, RACCMAGIC) != 0)
			{
			    /*
			    ** No RACCMAGIC bits so we are dealing with
			    ** a new GCN optimized RACC file.
			    **
			    ** Set flag to indicate use of GCN Optimized
			    ** RACC files.
			    */
			    f->_flag |= _SI_RACC_OPT;
			}
			else
			{
			    realsize = *((i4 *)(f->_ptr + 2));
			    *((i4 *)(f->_base - 8)) = realsize;
			    if ((realsize + RACCLEADING) > p->fx_fab.fab$w_mrs)
				f->_cnt = p->fx_fab.fab$w_mrs;
			   else
				f->_cnt = realsize + RACCLEADING;
			   f->_ptr += RACCLEADING;
			   f->_cnt -= RACCLEADING;
			}
		    }
		}
		else if (modec == 'w')
		{
		    /*
		    ** Creating a new File.  Set flag to indicate use of
		    ** GCN Optimized RACC files.
		    */
		    f->_flag |= _SI_RACC_OPT;
		}

		if (modec == 'a')
		    SIfseek(f, 0L, SI_P_END);
		if (modec == 'w')
		    SIfseek(f, 0L, SI_P_START);
	    }

	    if (ast_value == SS$_WASSET)
		sys$setast(1);
	    return (OK);
	}
	sys$close(&p->fx_fab);
	break;
    }
    *desc = NULL;
    if (ast_value == SS$_WASSET)
	sys$setast(1);
    return SI_CANT_OPEN;
}

/*
**  SIopen - open a text file [OBSOLESCENT]
**
**  Parameters:
**	location	- location of the file
**	mode		- access mode of the file ("write", "read", "append")
**	desc		- value to get file pointer
**
**  Returns:
**	OK		- file is open
**	FAIL		- file open failed, desc is not valid
**
**  History:
**	02/07/84 (lichtman)	- written
*/
STATUS
SIopen(
LOCATION	*location,
char		*mode,
FILE		**desc)
{
    return SIfopen(location, mode, SI_TXT, 0, desc);
}

/*
** Name: log_error_message - Write VMS error message to the error log
**
** Parameters:
**	i4 	err	The VMS error number
**	char	*fname 	The file that failed to open
**	char	mode	Flag indicating whether SYS$OPEN or SYS$CREATE was used
**
** Returns:
**	nothing
**
** History:
**	07/14/2000 (loera01) Bug 102029
*/
static void
log_error_message(
i4	err,
char	*fname,
char	mode,
char	*usermsg)
{
    CL_ERR_DESC		error;
    unsigned short	msglen = 0;
    char		buf[256];
    char		*msg;
    struct dsc$descriptor_s bufadr;
    TM_STAMP		time;
    char		stamp[TM_SIZE_STAMP];

    if (usermsg && *usermsg)
    {
	MEcopy(usermsg, STlength(usermsg), buf);
	msglen = STlength(usermsg);
    }
    else
    {
	msglen = 0;
	bufadr.dsc$w_length = sizeof( buf ) - 1;
	bufadr.dsc$b_dtype = DSC$K_DTYPE_T;
	bufadr.dsc$b_class = DSC$K_CLASS_S;
	bufadr.dsc$a_pointer = buf;
	if (sys$getmsg(err, &msglen, &bufadr, 0xF, 0) != SS$_NORMAL)
	    return;
    }
    buf[msglen] = '\0';
    TMget_stamp(&time);
    TMstamp_str(&time, stamp);
    msg = (char *)MEreqmem(0, TM_SIZE_STAMP + msglen + 25 + STlength(fname),
	FALSE, NULL);
    STprintf(msg, "%s %s error on file %s: %s", stamp,
	(mode == 'w' ? "Create" : "Open"), fname, buf);
    ERlog(msg, (STlength(msg)), &error);
    MEfree(buf);
}
