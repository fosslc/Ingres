/*
** Copyright (c) 1985, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ME.h>
#include    "MEerr.h"

/*
**  MELIFO.C -- general buffer allocation routines
**
**	allow buffers with LIFO de-allocation
**		MEneed()
**		MEinitbuf()
**
**	These routines allow the deallocation of a need() type buffer,
**	and also using the same buffer for various SERIALIZED purposes
**	by marking the end of one, beginning of the next.
**		MEfbuf()
**		MEmarkbuf()
**		MEseterr()
**
**	The sequence of operations is
**		MEinitbuf	-- initialize buffer
**		MEmarkbuf	-- mark place to free to
**		MEneed		-- allocate space in buffer
**		MEfbuf		-- free to mark
**
** History:
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_PROTOTYPED" and prototyped function
**		headers.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**      16-jul-93 (ed)
**	    added gl.h
**	11-aug-93 (ed)
**	    unconditional prototypes
*/


/*
	used to insure all buffers and all values within
		buffers are aligned on 4 byte boundaries
		(first needed on 3b5).
*/

# define	ALIGN_VAL	(sizeof (ALIGN_RESTRICT) -1)


/*
**  MENEED
**	Allocate 'nbytes' from 'buf' and set 'buf_ptr' to point
**		to the beginning of this area.
**	On buffer overflow err_func called, buf_ptr set to NULL
**		and error status returned.
**	MEneed() guarantees an even adress on return.
**
**	Parameters:
**		buf	-- buffer
**		nbytes	-- number of bytes desired
**		buf_ptr	-- ptr into buffer
**
**	Returns:
**		pointer to allocated area
**		on  buffer overflow returns 0.
**
*/

STATUS
MEneed(
	register LIFOBUF	*buf,
	register i4		nbytes,
	register char		**buf_ptr)
{
	STATUS	ret_val = OK;


	nbytes = (nbytes + ALIGN_VAL) & ~(ALIGN_VAL);

	if (nbytes > buf->lb_nleft)
	{
/*! BEGIN ALIGN adjust parameters before error fn call in case of non-local return (tim) */
		*buf_ptr	 = NULL;
		ret_val		 = ME_BF_OUT;
		(*buf->lb_err_func)(buf->lb_err_num, 0);
/*! END ALIGN */
	}
	else
	{
		*buf_ptr	 = buf->lb_xfree;
		buf->lb_xfree	+= nbytes;
		buf->lb_nleft	-= nbytes;
	}

	return(ret_val);
}


/*
**  MEINITBUF -- initialize a buffer
**
**	Must be called before the first MEneed() call on the buffer.
**
**	Parameters:
**		bf	 -- buffer
**		size	 -- size of buffer area
**		err_num	 -- error code for overflow
**		err_func -- function to call with err_code on error
**
**	Returns:
**		OK		-- success
**		ME_BF_ALIGN	-- buffer not aligned
**
*/

STATUS
MEinitbuf(
	register LIFOBUF	*buf,
	i4			size,
	i4			err_num,
	i4			(*err_func)())
{
	STATUS	ret_val = OK;


	if ((i4)buf & ALIGN_VAL)
	{
		ret_val		 = ME_BF_ALIGN;
	}
	else
	{
		buf->lb_nleft	 = size - sizeof *buf;
		buf->lb_xfree	 = buf->lb_buffer;
		buf->lb_err_num	 = err_num;
		buf->lb_err_func = err_func;
	}

	return(ret_val);
}


/*
**  MEFBUF -- frees part of a buffer
**
**	Parameters:
**		buf	-- buffer
**		bytes	-- a previous return from MEmarkbuf().
**
**	Returns:
**		OK		-- success
**		ME_BF_PARAM	-- bytes arg bad
**
*/

STATUS
MEfbuf(
	register LIFOBUF	*buf,
	i4			bytes)
{
	STATUS			ret_val = OK;
	register i4		i;


	if (bytes & ALIGN_VAL)
	{
		ret_val = ME_BF_FALIGN;
	}
	else if ((i = bytes - buf->lb_nleft) < 0)
	{
		ret_val = ME_BF_PARAM;
	}
	else
	{
		buf->lb_xfree -= i;
		buf->lb_nleft += i;
	}

	return (ret_val);
}


/*
**  MEMARKBUF -- Mark a place in the buffer to deallocate to
**
**	Parameters:
**		buf -- buffer
**
**	Returns:
**		int >= 0 marking place in buffer (should be used in calling
**			MEfbuf()
**
**	Side Effects:
**		none
*/

i4
MEmarkbuf(
	LIFOBUF		*buf)
{
	return(buf->lb_nleft);
}

/*
**  MESETERR -- change the error info for a buffer
**
**	Parameters:
**		bf	 -- buffer
**		errnum	 -- new overflow error code
**		err_func -- new error handler
**
**	Returns:
**		none
**
*/

VOID
MEseterr(
	LIFOBUF			*buf,
	i4			errnum,
	i4			(*err_func)())
{
	buf->lb_err_num	 = errnum;
	buf->lb_err_func = err_func;
}
