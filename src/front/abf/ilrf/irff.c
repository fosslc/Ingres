/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "irstd.h"

/**
** Name:	irff.c - fetch frame information
**
** Description:
**	internal frame information fetch routine
*/

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

extern QUEUE M_list, F_list;

/*{
** Name:	fid_fetch - fetch frame
**
** Description:
**	used only within ILRF - fetches a frame node by unique id.
**
** Inputs:
**	id	FID portion of id.
**	irblk	client IRBLK
**
** Outputs:
**	return:
**		frame node, NULL for fail.
**
** History:
**	8/86 (rlm)	written
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

IR_F_CTL *
fid_fetch(id, irblk)
i4	id;
IRBLK	*irblk;
{
	i4 cid;	/* client id */
	IR_F_CTL *ptr;

	if (irblk->irtype == IORI_IMAGE)
		cid = irblk->irbid;
	else
		cid = NULLIRBID;

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug,ERx("search for frame %d, client %d\n"),id,cid);
	dump_lists();
# endif

	/* look for the frame in memory */
	for (ptr = (IR_F_CTL *)M_list.q_next; ptr != (IR_F_CTL *)&M_list;
		ptr = (IR_F_CTL *)ptr->fi_queue.q_next)
	{
		if (ptr->fid.id == id && ptr->irbid == cid)
			return (ptr);
	}

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug,ERx("not on memory list\n"));
# endif

	/* look for the frame in the temporary file list */
	for (ptr = (IR_F_CTL *)F_list.q_next; ptr != (IR_F_CTL *)&F_list;
		ptr = (IR_F_CTL *)ptr->fi_queue.q_next)
	{
		if (ptr->fid.id == id && ptr->irbid == cid)
			return (ptr);
	}

# ifdef ILRFDTRACE
	SIfprintf(Fp_debug,ERx("not on temp file list\n"));
# endif

	/* not found in either of the lists */
	return (NULL);
}
