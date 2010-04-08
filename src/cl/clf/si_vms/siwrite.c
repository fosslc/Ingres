/*
** Copyright (C) 1983, 2005 Ingres Corporation
**
*/

#include        <compat.h>
#include        <gl.h>
#include	<me.h>
#include        <si.h>
#include	<devdef.h>
#include	"silocal.h"

FUNC_EXTERN i4 rms_put(RABDEF *);
FUNC_EXTERN int $flush(FILE *, int);


/*
**  SIwrite	- write data to a stream
**
**  Parameters:
**	n	- size of the item to be written
**	buf	- address of the item to be written
**	count   - useless variable kept for historical purposes
**	fp	- file to which the item is to be written
**
**  Returns:
**	FAIL	- error
**
**  History:
**	27-jul-95 (duursma)
**	    Now that terminal devices no longer have the FAB$M_CR bit set,
**	    must do an additional check for DEV$M_TRM being 0 before deciding
**	    to handle the case of variable length record text files.
**	27-jul-2005 (abbjo03)
**	    Add special case for processing SI_BIN files with logical records
**	    greater than RMS maximums.
*/
STATUS
SIwrite(
i4	n,
char	*buf,
i4	*count,
FILE	*fp)
{
    register i4	len;
    register i4	left = fp->_cnt;
    FILEX	*fx = ((FILEX *)fp->_file);

    if (fp->_flag & _SI_RACC_OPT)
    {
	/*
	** Using Optimized GCN file, no buffering involved.  Do a direct
	** RMS write to the file. Ignore user specified number of bytes,
	** we are writing a whole record.
	*/
	fx->fx_rab.rab$w_rsz = fx->fx_fab.fab$w_mrs;
	fx->fx_rab.rab$l_rbf = buf;
	fx->fx_rab.rab$l_rop |= RAB$M_UIF;
	if ((rms_put(&fx->fx_rab) & 1) == 0)
	    return FAIL;

	/* increment the RMS Relative Record Number */
	++(*((i4 *)(fx->fx_rab.rab$l_kbf)));
	*count = fx->fx_fab.fab$w_mrs;
	fx->fx_fab.fab$l_xab = NULL;
	return OK;
    }

    /*
    ** Special case:  SI_BIN (i.e., fixed-length binary) files with logical
    ** records exceeding VMS maximum record size.
    */
    if (fp->_type == SI_BIN && fx->fx_fab.fab$b_rfm == FAB$C_VAR
	&& fx->fx_fab.fab$w_mrs == MAX_RMS_SEQ_RECLEN
	&& n > MAX_RMS_SEQ_RECLEN)
    {
	if (n > fx->rec_length)
	{
	    *count = 0;
	    return FAIL;
	}
	left = n;
	while (left)
	{
	    fx->fx_rab.rab$w_rsz = (left > MAX_RMS_SEQ_RECLEN
				    ? MAX_RMS_SEQ_RECLEN : left);
	    fx->fx_rab.rab$l_rbf = buf;
	    if ((rms_put(&fx->fx_rab) & 1) == 0)
	    {
		*count = 0;
		return FAIL;
	    }
	    left -= fx->fx_rab.rab$w_rsz;
	    buf += fx->fx_rab.rab$w_rsz;
	}
	*count = n;
	return OK;
    }

    /*
    ** Handle binary file fixed/variable length records here.
    ** Partly a performance fix but also fixes the problem that
    ** VAR files were being written as multiple physical records
    ** per logical-record.
    */
    if ( (fp->_flag & _IORACC) == 0 && fx->fx_fab.fab$b_rfm == FAB$C_FIX
	  || ( fx->fx_fab.fab$b_rfm == FAB$C_VAR &&
	    /* terminal no longer has M_CR set so must check M_TRM */
	    (fx->fx_fab.fab$l_dev & DEV$M_TRM) == 0 &&
	    (fx->fx_fab.fab$b_rat & (FAB$M_CR|FAB$M_PRN)) == 0 ))
    {
	*count = n;

	if (fx->fx_fab.fab$b_rfm == FAB$C_FIX)
	{
	    /*
	    **  Buffer up a whole records before writing.  If
	    **  buffer is empty and SIwrite is for the whole record
	    **  then write the record from the callers buffer.
	    */
	    if (left != fx->fx_fab.fab$w_mrs || n != fx->fx_fab.fab$w_mrs)
	    {
		while (n >= left)
		{
		    MEcopy(buf, left, fp->_ptr);
		    fx->fx_rab.rab$w_rsz = fx->fx_fab.fab$w_mrs;
		    fx->fx_rab.rab$l_rbf = fp->_ptr;
		    if ((rms_put(&fx->fx_rab) & 1) == 0)
		    {
			*count -= n;
			return FAIL;
		    }
		    n -= left;
		    buf += left;
		    left = fx->fx_fab.fab$w_mrs;
		    fp->_ptr = fp->_base;
		    fp->_cnt = left;
		}
		if (n < left)
		{
		    MEcopy(buf, n, fp->_ptr);
		    fp->_ptr += n;
		    fp->_cnt -= n;
		}
		return OK;
	    }
	    else
	    {
		fx->fx_rab.rab$l_rbf = buf;
		fx->fx_rab.rab$w_rsz = fx->fx_fab.fab$w_mrs;
		if ((rms_put(&fx->fx_rab) & 1) == 0)
		{
		    *count -= n;
		    return FAIL;
		}
		return OK;
	    }
	}
	else if (left == fx->fx_rab.rab$l_ctx)
	{
	    /* The record is the unit presented to SIwrite(). */
	    fx->fx_rab.rab$w_rsz = n;
	    fx->fx_rab.rab$l_rbf = buf;
	    if ((rms_put(&fx->fx_rab) & 1) == 0)
	    {
		*count = 0;
		return FAIL;
	    }
	    return OK;
	}
    }

    /*
    ** Variable length records with carriage return attribute or
    ** non-carriage return attributes but using character I/O and
    ** RACC file end up here.
    */
    if (fp->_flag & _IORACC)
	left = ((FILEX *)fp->_file)->fx_fab.fab$w_mrs - (fp->_ptr - fp->_base);
    len = n;
    while (len)
    {
	if (len >= left)
	{
	    MEcopy(buf, left, fp->_ptr);
	    len -= left;
	    fp->_ptr += left;
	    fp->_cnt = 0;
	    buf += left;
	    if ((left = $flush(fp, FALSE)) <= 0)
	    {
		*count = 0;
		return FAIL;
	    }
	}
	else
	{
	    MEcopy(buf, len, fp->_ptr);
	    fp->_ptr += len;
	    if (fp->_cnt > len)
		fp->_cnt -= len;
	    else
		fp->_cnt = 0;
	    break;
	}
    }

    *count = n;
    return OK;
}

/*
**
**  Name: SIstd_write() - Write to standard stream
**
**  Description:
**      This function takes a null-terminated string and writes it to the
**      appropriate standard stream (stdin, stdout, stderr).
**
**  Inputs:
**      std_stream      - SI_STD_IN, SI_STD_OUT, or SI_STD_ERR
**      *str            - Pointer to a null-terminated string
**
**  Outputs:
**      none
**      Returns:
**          return from WriteFile()
**      Exceptions:
**          none
**
*  Side Effects:
**      none
**
**  History:
**      28-jul-2000 (somsa01)
**          Created.
*/

STATUS
SIstd_write(i4 std_stream, char *str)
{
    FILE        *stream;

    switch (std_stream)
    {
        case SI_STD_IN:
            stream = stdin;
            break;

        case SI_STD_OUT:
            stream = stdout;
            break;

        case SI_STD_ERR:
            stream = stderr;
            break;
    }

    SIfprintf(stream, "%s", str);
    SIflush(stream);

    return(TRUE);
}

