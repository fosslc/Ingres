/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_GES_NEXT - get space for evaluating an expression.
**		This space is used over and over again for each expression.
**		The space initially is a 2K block, but if additional space is
**		needed, additional 2K blocks are allocated and linked through
**		a pointer in the previous block.
**
**		r_ges_set initializes the very first block and is done once
**			by the report writer.
**
**		r_ges_reset resets the pointers and must be done before
**			evaluating an expression.
**
**		r_ges_next actually gets space for a given size request.
**			The last byte is set to zero for strings.
**
**	Parameters:
**		size	size of space to get must be no greater than 2044.
**			For strings this must be one more than the length
**			of the string for the zero byte.
**
**	Returns:
**		pointer to space allocated.
**
**	Side effects:
**		Increments Next_space by size to point to available space
**		after that allocated. If the size requested exceeds that
**		of the available space left, another block of space is
**		allocated and linked to the current block through its
**		next-pointer.
**
**	Error messages:
**		SYSERR: size too large.
**
**	Trace Flags:
**		none.
**
**	History:
**		2/14/84 (gac)	written.
**
**	1-may-91 (marc_b) (mainline integration-elein)
**
**		Fixed Alignment problem. Formerly it only allocated enough
**		space for a word. Now it allocates based upon ALIGN_RESTRICT.
**		This change was already implemented in the ingre63p line.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-aug-2002 (abbjo03)
**	    Increase BUFSIZE to 32760 to reflect increase in DB_MAXSTRING.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# define	BUFSIZE		32760

typedef struct ges_block
{
	char		 ges_b_space[BUFSIZE];	/* actual block of space */
	struct ges_block *ges_b_next;		/* pointer to next block */
} GBLOCK;

static GBLOCK	*First_block = NULL;	/* Points to first block of space */
static GBLOCK	*Curr_block = NULL;	/* Points to current block of space */
static char	*Next_space = NULL;	/* Points to next available space
				   in current block */

VOID
r_ges_set()	/* Initial allocation of space (done once in RW) */
{
	First_block = (GBLOCK *) MEreqmem(0,(u_i4) sizeof(GBLOCK),TRUE,(STATUS *) NULL);
	First_block->ges_b_next = NULL;
}


VOID
r_ges_reset()	/* Done every time before evaluating an expression */
{
	Curr_block = First_block;
	Next_space = First_block->ges_b_space;
}



i4	*
r_ges_next(
i2	size)
{
	char	*space;
	GBLOCK	*next_block;

	if (Next_space+size > Curr_block->ges_b_space+BUFSIZE)
	{
		if (size > BUFSIZE)
		{
			r_syserr(E_RW0027_size_too_large);
		}

		if (Curr_block->ges_b_next == NULL)
		{
			next_block = (GBLOCK *) MEreqmem(0,sizeof(GBLOCK),TRUE,(STATUS *) NULL);
			next_block->ges_b_next = NULL;
			Curr_block->ges_b_next = next_block;
		}

		Curr_block = Curr_block->ges_b_next;
		Next_space = Curr_block->ges_b_space;
	}

	space = Next_space;

 	/* Make sure the start of next space begins on double word boundary
 	   thereby avoiding any alignment problems 			*/
 	Next_space=
 		(char *) ME_ALIGN_MACRO(Next_space+size,sizeof(ALIGN_RESTRICT));

	*(space+size-1) = '\0'; /* zero byte for strings */
	return((i4 *)space);
}
