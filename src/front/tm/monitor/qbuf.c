# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	<buf.h>

GLOBALREF bool	Qb_memory_full;		
static struct qbuf	Qb[QBUFS]	ZERO_FILL;

/*	stat field values	*/

# define	QB_OPEN		01
# define	QB_READ		02
# define	QB_WRIT		04


/*
** Copyright (c) 2004 Ingres Corporation
**
**	QUERY BUFFER manipulation routines
**
**	History:
**
**	14-mar-90 (teresal)
**		Modified query buffer so it is dynamically allocated.
**		This allows a user's input query to be arbitraily large. 
**		The query buffer will expand as needed - the only 
**		limitation is available memory.  Bug 9489
**	10-nov-90 (kathryn) - Change q_putc() q_getc().
**		Only decrement qb_count if it is > 0.
**	17-mar-92 (seg) Must use ZERO_FILL to init structure array to zero.
**	17-dec-92 (rogerl)
**		(q_putc) Delete unused array of buffers.
**	07-feb-93 (rogerl) handle potentially large buffers with qrcpbuf/MErqlng
**	24-feb-93 (rogerl)
**		(q_putc) QUEL wants to initialize a second qbuf structure at startup
**		in case there is to be macro processing
**      15-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchin's changes (from 6.4) for axp_osf
**              10/15/93 (kchin)
**              Cast 2nd argument to PTR when calling putprintf(), this
**              is due to the change of datatype of variable parameters
**              in putprintf() routine itself.  The change is made in
**              q_open() and q_putc().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
*/


struct qbuf	*
q_open(char mode)
{
	register struct qbuf	*qb;
	STATUS			stat;

	switch (mode)
	{
	    case 'r':
			mode = QB_READ;
			break;
	    case 'w':
			mode = QB_WRIT;
			break;
	    default:
			return (NULL);
	}

	for (qb = &Qb[0]; qb < &Qb[QBUFS]; qb++)
	    if (!(qb->qb_stat & QB_OPEN))
	    {
		    /* Allocate the Query buffer */
		    qb->qb_bufsiz 	= QBUFSIZE;
		    if ((qb->qb_pbuf = (char *)MEreqmem((u_i4)0, 
			    (u_i4)QBUFSIZE, (bool)FALSE, &stat)) == NULL)
		    {
			    /* Memory allocation failed */
			    putprintf(ERget(E_MO0003_Unable_alloc_init), 
							(PTR)qb->qb_bufsiz);
			    return(NULL);
		    }

		    qb->qb_stat		= mode | QB_OPEN;
		    qb->qb_char		= qb->qb_pbuf;
		    qb->qb_count	= qb->qb_bufsiz;
		    qb->qb_end		= 0;
		    Qb_memory_full	= FALSE;

		    return (qb);
	    }

	return (NULL);
}


q_close(qb)
register struct qbuf	*qb;
{
	if (qb->qb_stat & QB_OPEN)
		qb->qb_stat = 0;

	/* Free Query Buffer Memory */
	MEfree(qb->qb_pbuf);
}


i4
q_getc(qb)
register struct qbuf	*qb;
{
	if ((qb->qb_stat & QB_OPEN) && (qb->qb_stat & QB_READ)) {
		if (qb->qb_count > 0)
		{
			qb->qb_count--;
			return (*qb->qb_char++ & 0377);
		}
	}

	return (0);
}

q_putc(qb, c)
register struct qbuf	*qb;
register char		c;
{
	i4	ret_val = -1;

	if ((qb->qb_stat & QB_OPEN) && (qb->qb_stat & QB_WRIT))
	{
		ret_val = 0;

		if (qb->qb_count == 0)
		{
			/*
			** Allocate more memory for the Query Buffer.
			** Double the buffer size. But first make sure we
			** haven't already tried to get memory and failed.
			*/
			char	*oldqbuf = qb->qb_pbuf;
			i4	newbufsiz = qb->qb_bufsiz * 2;
			STATUS	stat;

			if (Qb_memory_full) 
			    return( 0 );

			newbufsiz = qb->qb_bufsiz * 2;
			if ((qb->qb_pbuf = (char *)MEreqmem((u_i4)0, 
				(SIZE_TYPE)newbufsiz, (bool)FALSE, 
			    &stat)) == NULL)
			{
			    /* Memory allocation failed */
			    putprintf(ERget(E_MO0004_Unable_alloc_mem), 
							    (PTR)newbufsiz);
			    qb->qb_pbuf = oldqbuf; /* Reset ptr */
			    qb->qb_char--;	       /* Point to last char */
			    *qb->qb_char++ = '\n'; /* Set '\n' for editor */
			    Qb_memory_full = TRUE;
			    return(-1);
			}
			qrcpbuf( oldqbuf, qb->qb_pbuf, qb->qb_bufsiz );
			MEfree(oldqbuf);

			qb->qb_count	= newbufsiz - qb->qb_bufsiz;
			qb->qb_char	= qb->qb_pbuf + qb->qb_bufsiz;
			qb->qb_bufsiz 	= newbufsiz;
		}
		/* Add character to query buffer */
		qb->qb_count--; 
		*qb->qb_char++ = c;
		qb->qb_end++;
	}

	return (ret_val);
}

struct qbuf *
q_ropen(struct qbuf *qb, char mode)
{
	struct	qbuf	*ret_val = 0;


	if (qb->qb_stat & QB_OPEN)
	{
		switch (mode)
		{
		    case 'r':
			qb->qb_stat	= QB_READ | QB_OPEN;
			qb->qb_count	= qb->qb_end;
			qb->qb_char	= qb->qb_pbuf;
			break;

		    case 'w':
			qb->qb_stat	= QB_WRIT | QB_OPEN;
			qb->qb_count	= qb->qb_bufsiz;
			qb->qb_end	= 0;
			qb->qb_char	= qb->qb_pbuf;
			Qb_memory_full	= FALSE;
			break;

		    case 'a':
			qb->qb_stat	= QB_WRIT | QB_OPEN;
			qb->qb_count	= qb->qb_bufsiz - qb->qb_end;
			qb->qb_char	= qb->qb_pbuf + qb->qb_end;
			break;
		}

		ret_val		= qb;
	}

	return (ret_val);
}
