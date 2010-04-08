
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

# ifdef ILRFDTRACE

/**
** Name:	irdump.c - debug tracing routines
**
** Description:
**	Routines used for debug tracing.  Also defines the FILE pointer
**	used for debug tracing,	 referenced to all this stuff is normally
**	ifdef'ed out.
*/

GLOBALDEF FILE *Fp_debug = NULL;

extern QUEUE M_list, F_list;

dump_lists()
{
	IR_F_CTL *ptr;

	for (ptr = (IR_F_CTL *)M_list.q_next; ptr != (IR_F_CTL *)&M_list;
			ptr = (IR_F_CTL *)ptr->fi_queue.q_next)
		SIfprintf(Fp_debug,ERx("\tframe %d, client %d, tag %d, cur%d\n"),
			ptr->fid.id, ptr->irbid, ptr->d.m.tag, ptr->is_current);

	for (ptr = (IR_F_CTL *)F_list.q_next; ptr != (IR_F_CTL *)&F_list;
			ptr = (IR_F_CTL *)ptr->fi_queue.q_next)
		SIfprintf(Fp_debug,ERx("\tframe %d, client %d, pos %d\n"),
					ptr->fid.id, ptr->irbid, ptr->d.f.pos);
}

sess_trace(irblk, call)
IRBLK	*irblk;
char	*call;
{
	SIfprintf(Fp_debug,
		ERx("ENTRY %s: magic %d, client %d, type %d, current frame X%lx\n"),
		call, irblk->magic, irblk->irbid, irblk->irtype,
		irblk->curframe);
}

# else
	/* prevents an empty object file */
	IIAM_bogus()
	{
	}
# endif
