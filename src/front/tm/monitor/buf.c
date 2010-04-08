/*
** Copyright (c) 2004 Ingres Corporation
**
**  BUFFER MANIPULATION ROUTINES
*/

# include	<compat.h>
# include	<me.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<si.h>
# include	<buf.h>
# include	"maceight.h"
# include	"ermo.h"

/*
**  BUFPUT -- put character onto buffer
**
**	The character 'c' is put onto the buffer 'bp'.	If the buffer
**	need be extended it is.
**
** History:
**
**	8/8/85 (peb) -- Added additional routines to provide a special
**			interface for the macro package. This allows
**			us to support 8-bit characters in the macro package
**			without major surgery. The general idea is that the
**			buffer manager accepts/returns 'nat' values consisting
**			of an eight-bit character (low-order byte) and a
**			flag indicating whether the character is quoted in the
**			high order byte (or bytes). These quote characters
**			are porperly stored by the buffer manager.
**			The routine mac_bufcrunch removes all QUOTED indicators
**			from the returned string.
**	9/18/85 (peb)	Updated m_bufput to put escape sequence in proper order
**			Must be escape then character.
**	19-may-88 (bruceb)
**		Changed ME[c]alloc calls into calls on MEreqmem.
**	27-may-88 (bruceb)
**		Parenthesize arg to sizeof, so compiles on DG.
**	15-nov-91 (leighb) DeskTop Porting Change:
**		Changed 'BUFSIZE' to 'TMBUFSIZE' to avoid conflict with
**		'BUFSIZE' in compiler header.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

static char	*Buf_flat = NULL;


m_bufput(val, buffer)
i4		val;
struct buf	**buffer;
{

	/*
	** The general idea is that if the character is QUOTED, then
	** we store an escape character followed by the eight bit
	** character itself. The ESC_QUOTE character must be an eight
	** bit quantity that is not a legal character in the
	** character set (recommend using one of the four characters
	** reserved for pattern matching.
	*/


	bufput(val & BYTEMASK, buffer);
	if (val & QUOTED)
		bufput(ESC_QUOTE, buffer);
}

i4
m_bufget(buffer)
struct buf	**buffer;
{
	i4	val;

	/*
	** this is the inverse of m_bufput
	*/

	if ((val = bufget(buffer)) == ESC_QUOTE)
		return (bufget(buffer) | QUOTED);

	return (val);
}


void
bufput(c, buffer)
char		c;
struct buf	**buffer;
{
	register struct buf	*b;
	register struct buf	*a;
	register struct buf	**bp;
	FUNC_EXTERN	char			*bufalloc();

	bp = buffer;
	b = *bp;
	if (b == NULL || b->ptr >= &b->buffer[TMBUFSIZE])	 
	{
		/* allocate new buffer segment */
		a = (struct buf *) bufalloc((i4)sizeof(*a));
		a->nextb = b;
		a->ptr = a->buffer;
		*bp = b = a;
	}

	*b->ptr++ = c;
}



/*
**  BUFGET -- get character off of buffer
**
**	The buffer is popped and the character is returned.  If the
**	segment is then empty, it is returned to the free list.
*/

int
bufget(buffer)
struct buf	**buffer;
{
	register struct buf	*b;
	register char		c;
	register struct buf	**bp;

	bp = buffer;
	b = *bp;

	if (b == NULL || b->ptr == b->buffer)
	{
		/* buffer is empty -- return end of file */
		return (0);
	}

	c = *--(b->ptr);

	/* check to see if we have emptied the (non-initial) segment */
	if (b->ptr == b->buffer && b->nextb != NULL)
	{
		/* deallocate segment */
		*bp = b->nextb;
		buffree(b);
	}

	return (c & 0377);
}



/*
**  BUFPURGE -- return an entire buffer to the free list
**
**	The buffer is emptied and returned to the free list.  This
**	routine should be called when the buffer is to no longer
**	be used.
*/

void
bufpurge(buffer)
struct buf	**buffer;
{
	register struct buf	**bp;
	register struct buf	*a;
	register struct buf	*b;

	bp = buffer;
	b = *bp;
	*bp = NULL;

	/* return the segments to the free list */
	while (b != NULL)
	{
		a = b->nextb;
		buffree(b);
		b = a;
	}
}




/*
**  BUFFLUSH -- flush a buffer
**
**	The named buffer is truncated to zero length.  However, the
**	segments of the buffer are not returned to the system.
*/

void
bufflush(buffer)
struct buf	**buffer;
{
	register struct buf	*b;
	register struct buf	**bp;

	bp = buffer;
	b = *bp;
	if (b == NULL)
		return;

	/* return second and subsequent segments to the system */
	bufpurge(&b->nextb);

	/* truncate this buffer to zero length */
	b->ptr = b->buffer;
}



/*
**  BUFCRUNCH -- flatten a series of buffers to a string
**
**	The named buffer is flattenned to a conventional C string,
**	null terminated.  The buffer is deallocated.  The string is
**	allocated "somewhere" off in memory, and a pointer to it
**	is returned.
*/


char *
bufcrunch(buffer)
struct buf	**buffer;
{
	register char	*p;
	char		*bufflatten();

	p = bufflatten(*buffer, (i4) 1);
	*p = '\0';
	*buffer = NULL;
	return (Buf_flat);
}

char *
bufflatten(buf, length)
struct buf	*buf;
i4		length;
{
	register struct buf	*b;
	register char		*p;
	register char		*q;
	FUNC_EXTERN	char			*bufalloc();

	b = buf;

	/* see if we have advanced to beginning of buffer */
	if (b != NULL)
	{
		/* no, keep moving back */
		p = bufflatten(b->nextb, length + (b->ptr - b->buffer));
	}
	else
	{
		/* yes, allocate the string */
		Buf_flat = p = bufalloc(length);
		return (p);
	}

	/* copy buffer into string */
	for (q = b->buffer; q < b->ptr; )
		*p++ = *q++;

	/* deallocate the segment */
	buffree(b);

	/* process next segment */
	return (p);
}



/*
**  BUFALLOC -- allocate clear memory
**
**	This is similar to the system malloc routine except that
**	it has no error return, and memory is guaranteed to be clear
**	when you return.
**
**	It might be nice to rewrite this later to avoid the nasty
**	memory fragmentation that malloc() tends toward.
**
**	The error processing might have to be modified if used anywhere
**	other than INGRES.
*/

char *
bufalloc(size)
i4	size;
{
	char	*p;
	i4	status;
	char	errbuf[ER_MAX_LEN];

	if ((p = (char *)MEreqmem((u_i4)0, (u_i4)size, (bool)FALSE,
		(STATUS *)&status)) == NULL)
	{
		ERreport(status, errbuf);
		/* In macro processor: %s\n */
		ipanic(E_MO0042_1500200, errbuf);
	}

	bufclear(p, size);

	return (p);
}



/*
**  BUFCLEAR -- clear a block of core memory
**
**	Parameters:
**		p -- pointer to area
**		size -- size in bytes
*/

void
bufclear(p, size)
char	*p;
i4	size;
{
	MEfill((u_i2)size, '\0', p);
}



/*
**  BUFFREE -- free memory
*/

void
buffree(ptr)
register struct buf	*ptr;
{
	MEfree(ptr);	/* PC porting change (9/30/89) - remove i4 cast */
}
