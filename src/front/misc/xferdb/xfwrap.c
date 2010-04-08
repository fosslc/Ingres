
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<cm.h>
# include	<st.h>
# include	<er.h>
# include       <si.h>
# include       <lo.h>
# include	<fe.h>
# include	<ug.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<xf.h>
# include	"erxf.h"

/**
** Name:	xfwrap.c - word wrap routines for views, integrities and permits
**
** Description:
**	This module is the general interface to files.  There are two
**	flavors of the interface: direct writes to the underlying SI
**	routines, and buffered text where the text is held until
**	a "flush" command causes it to be formatted and written.
**	The type of file is determined by flags to xfopen.
**
**	This interfaces uses handles (TXT_HANDLE *), much like SI.  
**	However, it is a filter above SI.  Many TXT_HANDLEs may access
**	the same underlying file.  The xfopen on an SI file
**	causes the SI file to be created.  xfreopen is a way of opening
**	a handle with different characteristics (ie: buffering).
**
**	The text buffering is done with a standoff algorithm to expand buffer
**	size as necessary.  Buffers are reused, so overhead is low.  
**	When the buffered text is flushed, it is formatted into lines of
**	a maximum preconfigured length.
**
**	This file defines:
**
**	xfopen		initialize text file handle, open underlying file.
**	xfreopen	initialize new text file handle on top of old.
**	xfduplex	initialize new text file handle on top of two olds.
**
**	xfwrite		write to text file.
**	xfflush		flush text to real "disk" file, free the handle.
**	xfclose		flush text, free handle, close underlying file.
**	xffilename	return the name of a file, given the handle
**
** History:
**	26-aug-1987 (rdesmond)
**		written.
**	3-may-1990 (billc)
**		rewritten, with new interface.
**      26-sep-91 (jillb/jroy--DGC) (from 6.3)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	10/16/92 (dkh) - Fixed bug 47356.  Routine t_getline() never blindly
**			 assumed that the result buffer after calling
**			 fmt_multi is always null terminated.  This is not
**			 true since we are formatting into a DB_LTXT_TYPE.
**			 As a result, garbage data can be picked up and
**			 used.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-Oct-2001 (hanch04)
**          Added functions xfread and xfdelete
**	1-jul-2002 (gupsh01)
**	    Added support for opening binary files by xfopen.
**	21-Jul-2003 (hanje04)
**	    In xfclose check that FILE pointer is valid before closing
**	    and NULL afterwards. This is to stop the same file being closed
**	    twice on Linux which causes a SEGV
**  30-Nov-2004 (komve01)
**      Undo change# 473469 for Bug# 113498.
**/

/* # define's */

# define XFT_BUFSIZE	2048

/* GLOBALREF's */
/*
** We don't officially know anything about these here, but this 
** seems like a good place to define them.
*/
GLOBALREF       TXT_HANDLE     *Xf_in ;
GLOBALREF       TXT_HANDLE     *Xf_out ;
GLOBALREF       TXT_HANDLE     *Xf_both ;
GLOBALREF       TXT_HANDLE     *Xf_reload ;
GLOBALREF       TXT_HANDLE     *Xf_unload ;

/* extern's */

GLOBALREF	LOCATION	Xf_dir;
FUNC_EXTERN ADF_CB	*FEadfcb();

/* static's */

static void	t_initbuf(char *text, i4  len);
static void	t_getline(char **outbuf, bool *more);
static void	writeout(TXT_HANDLE *tdp, char *buf);

static TXT_HANDLE	*inithandle(i4 flags);

static TXT_HANDLE 	*Freehandles = NULL;


/*{
** Name:	xfopen - open an output file.
**
** Description:
**	Opens a text file, initializes, and returns
**	a handle.  The handle is a pointer to the struct that tracks the
**	status of the text buffer and the underlying file.
**
** Inputs:
**	name	- name of file to open.
**	flags	- binary flags specifying special behavior.
**
** Outputs:
**	none.
**
**	Returns:
**		*TXT_HANDLE - a 'handle' on success.  On failure, aborts.  
**
** History:
**      3-may-1990 (billc) - first written.
**	1-Jul-2002 (gupsh01)
**	    Added support for writing to the binary files. 
*/

TXT_HANDLE *
xfopen(char	*name, i4  flags)
{
    register TXT_HANDLE *tdp;
    i4  i;
    i4                  ftype = SI_TXT;
    i4                  ftype_bin = SI_BIN;
    i4             flen = 0;

    tdp = inithandle(flags | TH_BASEFILE);

# ifdef hp9_mpe
    if (flags & TH_SCRIPT)
        ftype = SI_CMD;
    flen = 256;
# endif

    /* We write to the same directory that the COPY INTO will use. */
    LOcopy(&Xf_dir, tdp->f_locbuf, &(tdp->f_loc));
    LOfstfile(name, &(tdp->f_loc));

    if (flags & TH_READONLY)
    {
	if (SIfopen(&(tdp->f_loc), ERx("r"), ftype, flen, &(tdp->f_fd)) 
	    != OK)
	{
	    IIUGerr(E_XF0002_Cannot_create, UG_ERR_ERROR, 1, xffilename(tdp));
	    return (NULL);
	}
    }
    else
    {
 	  if (flags & TH_BINWRITE)
	  {
	    if (SIfopen(&(tdp->f_loc), ERx("w"), ftype_bin, flen, &(tdp->f_fd)) 
	      != OK)
		{
	      IIUGerr(E_XF0002_Cannot_create, UG_ERR_ERROR, 1, xffilename(tdp));
	      return (NULL);
		}
	  }
	  else 
	  {
	    if (SIfopen(&(tdp->f_loc), ERx("w"), ftype, flen, &(tdp->f_fd)) 
	      != OK)
		{
	      IIUGerr(E_XF0002_Cannot_create, UG_ERR_ERROR, 1, xffilename(tdp));
	      return (NULL);
		}
	  }
    }
    return (tdp);
}
/*{
** Name:	xfreopen - duplicate an open file handle.
**
** Description:
**	Initializes a new handle that writes to the same underlying
**	physical file as the given old handle.
**
** Inputs:
**	oldtdp	- an existing handle.
**	flags	- binary flags specifying special behavior.
**
** Outputs:
**	none.
**
**	Returns:
**		TXT_HANDLE - a 'handle' on success.  On failure, aborts.  
**
** History:
**      3-may-1990 (billc) - first written.
*/

TXT_HANDLE *
xfreopen(TXT_HANDLE *oldtdp, i4  flags)
{
    i4  i;
    TXT_HANDLE	*tdp;

    /* This should be more disastrous. */
    if ( !(oldtdp->t_flags & TH_BASEFILE))
	return NULL;

    tdp = inithandle(flags);

    tdp->t_next = oldtdp;

    return (tdp);
}

/*{
** Name:	xfduplex - open a file that writes to two underlying files.
**
** Description:
**	Opens a text file, initializes, and returns
**	a handle.  The handle is a pointer to the struct that tracks the
**	status of the text buffer.  Writing to this handle causes writes
**	to both of the underlying files.
**
** Inputs:
**	tdp1	- an existing handle.
**	tdp2	- an existing handle.
**	flags	- binary flags specifying special behavior.
**
** Outputs:
**	none.
**
**	Returns:
**		TXT_HANDLE - a 'handle' on success.  On failure, aborts.  
**
** History:
**      3-may-1990 (billc) - first written.
*/

TXT_HANDLE *
xfduplex(TXT_HANDLE *tdp1, TXT_HANDLE *tdp2, i4  flags)
{
    TXT_HANDLE	*tdp;

    /* This should be more disastrous. */
    if ( !(tdp1->t_flags & TH_BASEFILE) || !(tdp2->t_flags & TH_BASEFILE))
	return NULL;

    tdp = inithandle(flags | TH_DUPLEX);

    tdp1->t_next = tdp2;
    tdp->t_next = tdp1;

    return (tdp);
}
/*{
** Name:	inithandle 
**
** Description:
**	Opens a text handle, initializes and returns it.
**	The handle is a pointer to the struct that tracks the
**	status of the text buffer and the underlying file.
*/

static TXT_HANDLE *
inithandle(i4 flags)
{
    TXT_HANDLE	*tdp;

    /* look for an unused descriptor. */
    if (Freehandles != NULL)
    {
	tdp = Freehandles;
	Freehandles = Freehandles->t_next;
	tdp->t_next = NULL;
	tdp->t_i = 0;
    }
    else
    {
	if ((tdp = (TXT_HANDLE *) XF_REQMEM(sizeof(TXT_HANDLE), TRUE)) == NULL)
	{
	    IIUGerr(E_XF0051_Out_of_text_descs, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}
    }

    if (flags & TH_IS_BUFFERED)
    {
	/* Does this handle have a buffer we can re-use? */
	if (tdp->t_size == 0)
	{
	    /* Buffered/formatted output, so allocate a buffer. */

	    tdp->t_size = XFT_BUFSIZE;

	    /* This gets freed only if we allocate a larger buffer later */
	    if ((tdp->t_buf = (char *) XF_REQMEM(tdp->t_size, FALSE)) == NULL)
	    {
		IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
		/* NOTREACHED */
	    }
	}
	tdp->t_buf[0] = EOS;
    }
    else
    {
	tdp->t_buf = NULL;
    }
    tdp->t_flags = flags;

    return (tdp);
}

/*{
** Name:   xfreplace - Search and replace strings in buffered text
**
** Description:
**
**      Does a case sensitive/insensitive search for one or more
**      occurances of a target string. If the replacement string
**      is bigger than the target, ensures result buffer will be
**      big enough.
**
** Inputs:
**	tdp	         - an existing handle.
**      doOnce           - flag only single replacement required
**      ignCase          - flag for case sensitivity
**      target           - substring to look for
**      replace          - desired replacement  
**
** Outputs:
**	none.
**
** Returns:
**      Replacements made    
**
** History:
**      13-jun-2008 (coomi01) Bug 110708
**         First written.
*/
i4 
xfreplace(TXT_HANDLE *tdp, i4 doOnce, i4 ignCase, char *target, char *replace)
{
    i4  result = 0;
    i4    tlen;
    i4	  rlen;
    PTR    ptr;
    i4   count;
    i4   index;
    i4 reqSize;

    if (! (tdp->t_flags & TH_IS_BUFFERED) )
    {
	/* 
	** For unbuffered writes no effect 
	*/
	return 0;
    }

    tlen = STlength(target);
    rlen = STlength(replace);

    /* 
    ** Count target(s)
    */
    count = 0;
    index = 0;
    while (index < (tdp->t_i - tlen))
    {
	/* 
	** Search for target
	*/
	ptr = STstrindex(&tdp->t_buf[index], target, tdp->t_i-index, ignCase);

	if (NULL == ptr)
	{
	    /* 
	    ** Nothing more to do
	    */
	    break;
	}
	else
	{
	    count++;
	    index = (ptr-tdp->t_buf) + tlen;

	    /* 
	    ** Further search required ?
	    */
	    if ( doOnce )
		break;
	}
    }

    /*
    ** If nothing found return
    */
    if ( 0 == count )
	return 0;

    /* 
    ** Will this expand current content ?
    */
    if (tlen < rlen)
    {
	/* 
	** Calc min buffer size
	*/
	reqSize = tdp->t_i + count*(rlen - tlen);

	if ( reqSize > tdp->t_size )
	{
	    /*
	    ** NOT Enough headroom left in buffer to expand
	    */
	    char	*newbuf;
	    i4	newsize = tdp->t_size;

	    /* 
	    ** Not enough room in buffer - must do 'standoff'.  Allocate a new 
	    ** buffer at least twice as big as the old, making sure that it's
	    ** big enough to hold the text. 
	    */
	    while ( newsize <= reqSize )
		newsize *= 2;
	    
	    if ((newbuf = (char *) XF_REQMEM(newsize, FALSE) ) == NULL)
	    {
		IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
		/* NOTREACHED */
	    }

	    /* copy old buffer into the new buffer. */
	    STlcopy(tdp->t_buf, newbuf, tdp->t_i);
	    
	    /* free the old buffer. */
	    MEfree((PTR) tdp->t_buf);
	    
	    /*
	    ** Save the new details
	    */
	    tdp->t_buf  = newbuf;
	    tdp->t_size = newsize;
	}
    }

    /*
    **  Now repeat the search and do the replacement
    */
    index = 0;
    while (count)
    {
	/* 
	** Search for target
	*/
	ptr = STstrindex(&tdp->t_buf[index], target, tdp->t_i-index, ignCase);

	if (NULL != ptr)
	{
	    /* 
	    ** Fix our work position
	    */
	    index = ptr - tdp->t_buf;

	    /* 
	    ** First, shift the tail down to right location
	    ** - Use MEmemmove to prevent overlap corruption
	    */
	    MEmemmove(ptr+rlen, ptr+tlen, tdp->t_i - index);

	    /*
	    ** Now copyin new text portion
	    */
	    MEmemmove(ptr, replace, rlen);

	    /*
	    ** Adjust length
	    */
	    tdp->t_i += (rlen - tlen);

	    /*
	    ** Next search starts further down
	    */
	    index += rlen; 

	    /*
	    ** Bump the result count
	    */
	    result++;
	}

	/*
	** Once again ?
	*/
	count--;
    }

    /*
    ** Completed
    */
    return result;
}

/*{
** Name:	xfwrite - open a text-buffer output pseudo-file.
**
** Description:
**	Write.  If unbuffered, write directly.  Otherwise,
**	this routine adds text to the internal buffer.  If the buffer is too
**	small, we'll allocate a new one with double the size.
**
** Inputs:
**	tdp		the 'handle' of the pseudo-text file.
**	text		the text to add.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**      3-may-1990 (billc) - first written.
**	28-nov-1990 (marian)
**		Fix problem with allocating more buffer space.  Change
**		while loop to use newsize instead of tdp->t_size.  This
**		was causing an infinite loop.
**	30-Jan-2008 (kibro01) b119825
**	    Allocate more space if the used space is equal to leave room for
**	    the null needed for t_getline.
*/

void
xfwrite(TXT_HANDLE *tdp, char *text)
{
    i4	tlen;

    if ( !(tdp->t_flags & TH_IS_BUFFERED))
    {
	/* Unbuffered write. */
	writeout(tdp, text);
	return;
    }

    tlen = STlength(text);

    /* is the buffer big enough? */
    if ((tdp->t_size - tdp->t_i) <= tlen)
    {
	char	*newbuf;
	i4	newsize = tdp->t_size;

	/* 
	** Not enough room in buffer - must do 'standoff'.  Allocate a new 
	** buffer at least twice as big as the old, making sure that it's
	** big enough to hold the text. 
	*/
	while ((newsize - tdp->t_i) <= tlen)
	    newsize *= 2;

	if ((newbuf = (char *) XF_REQMEM(newsize, FALSE) ) == NULL)
	{
	    IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}
	/* copy old buffer into the new buffer. */
	STlcopy(tdp->t_buf, newbuf, tdp->t_i);

	/* free the old buffer. */
	MEfree((PTR) tdp->t_buf);

	tdp->t_buf = newbuf;
	tdp->t_size = newsize;
    }

    STcopy(text, &(tdp->t_buf[tdp->t_i]));
    tdp->t_i += tlen;
}

/*
** writeout - low level write.  This is where we deal with duplex file handles
**		and other complications.
*/

static void
writeout(TXT_HANDLE *tdp, char *buf)
{
    auto i4     count;
    STATUS	stat = FAIL;
    i4		oflags = tdp->t_flags;

    for ( ; tdp != NULL; tdp = tdp->t_next)
    {
	/* We can only do true writes to BASEFILEs. */
	if ( !(tdp->t_flags & TH_BASEFILE))
	    continue;

	stat = SIwrite((i4)STlength(buf), buf, &count, tdp->f_fd);
	if (stat != OK)
	    break;

	/* If our original handle wasn't duplexed, we're through. */
	if ( !(oflags & TH_DUPLEX))
	    break;
    }

    if (stat != OK)
    {
	IIUGerr(E_XF0003_Cannot_write_to_file, UG_ERR_FATAL,
				1, xffilename(tdp));
	/* NOTREACHED */
    }
}
/*{
** Name:	xfread - read a text-buffer input pseudo-file.
**
** Description:
**	Read. 
**
** Inputs:
**	tdp		the 'handle' of the pseudo-text file.
**	text		the text to add.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**      19-Oct-2001 (hanch04)
**          Created.
*/

void
xfread(TXT_HANDLE *tdp, i4 numofbytes, char *buf)
{
    STATUS	stat = FAIL;
    i4	count;

    stat = SIread(tdp->f_fd, numofbytes, &count, buf);

    if (stat != OK)
    {
	IIUGerr(E_XF0003_Cannot_write_to_file, UG_ERR_FATAL,
	    1, xffilename(tdp));
	 /* NOTREACHED */
    }
}

/*{
** Name:	xfflush - flush a text-buffer output pseudo-file.
**
** Description:
**	This routine does the flush work of writing the text to
**	the underlying file and resetting the internal buffers.
**
** Inputs:
**	tpd		pointer to the text-buffer control block.
**
** Outputs:
**	none.
**
**	Returns:
**		STATUS - OK or FAIL.
**
** History:
**       1-Apr-2003 (hanal04) Bug 109584 INGSRV 2102
**           Remove ugly hack which added a space to the end of the line.
**           F_CU in f_colfmt() as been rewritten to pad with NULLs.
**           Trailing spaces are no longer stripped so we do not
**           need to hack one into the output. (See also bug 110026).
**       16-Nov-2004 (komve01) Bug 113498
**           Upon detecting a discontinued statement, do not write a newline
**			 character into the file. Fixed for copy.in to work correctly for views.
**       30-Nov-2004 (komve01)
**           Undo change# 473469 for Bug# 113498.
*/

void
xfflush(TXT_HANDLE *tdp)
{
    auto char	*outbuf;
    auto bool	more = TRUE;
    i4		cnt = 0;

    if (tdp->t_i == 0)
    {
	/* Nothing to flush.  Unbuffered file always has t_i == 0. */
	return;
    }

    /* break the buffer into chunks of size < LINELEN */
    t_initbuf(tdp->t_buf, tdp->t_i);

    while (more)
    {
	t_getline(&outbuf, &more);
	if (more)
	{
	    if (cnt++ > 0)
		writeout(tdp, ERx("\n"));
	    writeout(tdp, outbuf);
	}
    } 
    tdp->t_i = 0;
    tdp->t_buf[0] = EOS;

    writeout(tdp, GO_STMT);
}

/*{
** Name:	xfclose - free a handle and, if necessary, 
**			close underlying file.
**
** Description:
**
** Inputs:
**	tdp		the 'handle' of the pseudo-text file.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**      3-may-1990 (billc) - first written.
*/

void
xfclose(tdp)
TXT_HANDLE 	*tdp;
{
    xfflush(tdp);

    /* If this is file was xfopened instead of reopened, close it. */
    if (tdp->t_flags & TH_BASEFILE && tdp->f_fd)
    {
	SIclose(tdp->f_fd);
	tdp->f_fd=NULL;

	/* set permission to exexute main scripts for unloaddb */
	if (tdp->t_flags & TH_SCRIPT)
	    PEworld(ERx("+r+w+x"), &(tdp->f_loc));
    }

    tdp->t_next = Freehandles;
    Freehandles = tdp;
}
/*{
** Name:	xfdelete - delete an output file.
**
** Description:
**	delete a text file
**
** Inputs:
**	name	- name of file to delete.
**
** Outputs:
**	none.
**
**	Returns:
**
** History:
**      19-Oct-2001 (hanch04)
**          Created.
*/

void
xfdelete(TXT_HANDLE *tdp)
{
    LOdelete(&(tdp->f_loc));
}

/*{
** Name:        xffilename - get full path for a file
**
** Description:
**      Given a COPYDB/UNLOADDB file type, return the full path string.
**
** Inputs:
**      type    Type of the script for which to get name.
**
** Outputs:
**      none.
**
**      Returns:
**              ptr to string containing full path.
**              WARNING: the return value won't persist, so use immeidately.
**
**      Exceptions:
**              none.
**
** Side Effects:
**
** History:
**      15-jul-87 (rdesmond) written.
*/

char *
xffilename(TXT_HANDLE *tdp)
{
    auto char   *filestring;

    if (tdp->t_flags & TH_BASEFILE)
	LOtos(&(tdp->f_loc), &filestring);
    else if (tdp->t_next != NULL)
	filestring = xffilename(tdp->t_next);
    return (filestring);
}


/*{
** Name:	t_initbuf - initialize word wrap module with input string.
**
** Description:
**	This procedure must be called prior to calls to t_getline.  
**
** Inputs:
**	text		input string to format.
**	len		length of string to format.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**	26-aug-1987 (rdesmond)
**		written.
**	18-oct-1988 (marian)
**		Use DB_CNTSIZE instead of sizeof(DB_TEXT_STRING) - 1.
**	23-apr-1989 (marian)
**		Changed the format "cf0.%d" to "cu0.%d".  This was done
**		because the FMT routine was not working for reports or
**		forms.  The new format is an internal format that is not
**		documented for users.
**      3-may-1990 (billc)
**		made static, built simpler interface cover.
*/

static	DB_DATA_VALUE	Dbdvin = { NULL, 0, DB_CHR_TYPE, 0 };
static	DB_DATA_VALUE	Dbdvout = 
		{ NULL, XF_LINELEN + DB_CNTSIZE, DB_LTXT_TYPE, 0 };
static	FMT		*Ofmt = NULL;
static	PTR		Wksp = NULL;
static	i4		Wklen = 0;
static	ADF_CB		*Adfcb = NULL;

static void
t_initbuf(char *text, i4  len)
{
    char	fmtstr[60];
    auto i4     fmtsize;
    PTR		blk;
    auto i4	output_len;
    
    if (Adfcb == NULL)
	Adfcb = FEadfcb();

    /* only need to allocate format area once */
    if (Dbdvout.db_data == NULL)
    {
	STprintf(fmtstr, ERx("cu0.%d"), Dbdvout.db_length - DB_CNTSIZE);
	if (fmt_areasize(Adfcb, fmtstr, &fmtsize) != OK)
	{
	    IIUGerr(E_XF0053_Cannot_init_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}

	if ((blk = XF_REQMEM(fmtsize, TRUE)) == NULL) 
	{
	    IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}

        if (fmt_setfmt(Adfcb, fmtstr, blk, &Ofmt, &output_len) != OK)
	{
	    IIUGerr(E_XF0053_Cannot_init_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}

	/* set DB_DATA_VALUE for formatted output */
	if ((Dbdvout.db_data = XF_REQMEM(Dbdvout.db_length + 1, TRUE)) == NULL)
	{
	    IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}
    }

    /*
    **	Now set up the DB_DATA_VALUE for the input message string
    **	to use in the formatting routines.
    */

    Dbdvin.db_length = len;
    Dbdvin.db_data = (PTR) text;

    /* determine amount of workspace needed for output formatting */
    fmt_workspace(Adfcb, Ofmt, &Dbdvin, &output_len);

    /* If format workspace not big enough, allocate another one. */
    if (output_len > Wklen)
    {
	if (Wksp != NULL)
	    MEfree(Wksp);
	if ((Wksp = XF_REQMEM(output_len, TRUE)) == NULL)
	{
	    IIUGerr(E_XF0050_Cannot_alloc_buf, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
	}
	Wklen = output_len;
    }

    /* Initialize format with input string. */
    IIfmt_init(Adfcb, Ofmt, &Dbdvin, Wksp);
}

/*{
** Name:	t_getline - get next formatted line from original string.
**
** Description:
**	This procedure is called successively after t_initbuf to retrieve
**	the next line from the original input string which is equal in
**	to the output buffer size specified in t_initbuf.
**
** Inputs:
**	none.
**
** Outputs:
**	outbuf		ptr to the static output buffer.
**	more		TRUE - there is valid output in the buffer.
**			FALSE - there is no more in the output buffer.
**
**	Returns:
**		none.
**
**	Exceptions:
**		none.
**
** Side Effects:
**
** History:
**	26-aug-1987 (rdesmond)
**		written.
**	27-jul-89 (marian)
**		Added FALSE to fmt_multi call to correspond with the changes
**		to fmt.
**      3-may-1990 (billc)
**		made static, built simpler interface cover.
**      3/21/91 (elein) (b35574)
**		Add FALSE parameter to call to
**              fmt_multi. TRUE is used internally for boxed
**              messages only.  They need to have control
**              characters suppressed.
**       1-Apr-2003 (hanal04) Bug 109584 INGSRV 2102
**              F_CU format used by copydb/unloaddb has been modified
**              to pad with NULLs. We no longer need to strip spaces
**              from t_segment, though we must ensure we have null
**              terminated the string.
*/

static char		t_segment[XF_LINELEN + 1];

static void
t_getline(char **outbuf, bool *more)
{
    DB_TEXT_STRING	*textstr;

    if (fmt_multi(Adfcb, Ofmt, &Dbdvin, Wksp, &Dbdvout, 
					more, FALSE, FALSE) != OK)
    {
	IIUGerr(E_XF0054_Cannot_get_line, UG_ERR_FATAL, 0);
	/* NOTREACHED */
    }

    /* map DB_DATA_VALUE.db_data into DB_TEXT_STRING */
    textstr = (DB_TEXT_STRING *)Dbdvout.db_data;

    /*
    **  bug 47356: Can't arbitrarily just trim the whitespace off the end of
    **  a buffer for a long text since long text is not a null terminated
    **  string.  We need to use the count information to determine how
    **  many bytes of real data there is and only operate on those bytes.
    **
    **  Also, can't use the buffer pointed to by textstr->db_t_text since
    **  we can't be sure that there is room for the null byte to terminate
    **  the string.
    */
    STlcopy((char *) textstr->db_t_text, t_segment, (i4) textstr->db_t_count);

    /* Ensure we are NULL terminated */
    t_segment[XF_LINELEN] = '\0';

    *outbuf = t_segment;
}

