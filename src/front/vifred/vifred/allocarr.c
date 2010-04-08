/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**  allocarr.c
**
**  contains:
**	allocArr()
**
**  history:
**	2/1/85 drh - changed to FEalloc in allocArr.
**	1/29/85 (peterk) -- split off from vftofrm.qc
**	10-jul-87 (bab)
**		Changed from _VOID_ allocArr(num, ptr1, ptr2) to
**		PTR allocArr(num).  This means that all calls to
**		this routine needed to be changed from
**		allocArr(number, pointer, pointer) to
**		pointer = pointer = allocArr(number).
**		Also, changed memory allocation to use FEreqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>

/*
** return a pointer to an array of size num.
*/

PTR
allocArr(num)
register i4	num;
{
	PTR	blk;

	if (num <= 0)
	{
		return((PTR)NULL);
	}
	if ((blk = FEreqmem((u_i4)0, (u_i4)(num * sizeof(char *)), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("allocArr"));
	}
	return(blk);
}
