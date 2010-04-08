/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cs.h>
# include	<qu.h>
# include	<lo.h>
# include	"LOerr.h"


/*
**	LOqueue.c
**		contains the routines
**			LOsave()
**			LOreset()
**
**	History
**		05/04/83	--	(gb)	written (whoopie)
**		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**		26-aug-89 (rexl)
**		    Added calls to protect stream allocator.
**              01-sep-93 (smc)
**                  Made temp address holder a PTR to be portable to axp_osf.
*/

typedef	struct
{
	QUEUE		LOQ_q;
	LOCATION	LOQ_loc;
	char		LOQ_buf[MAX_LOC];
}	LOQ;

QUEUE	locq	=	{
				&locq,
				&locq
			};


/*
	LOsave()
		save the location of the current process in a queue.

		returns OK on success.
*/

STATUS
LOsave()
{
	STATUS		ret_val = OK;
	LOQ		*new_locq;
	PTR		temp;


	temp = MEreqmem(0, sizeof(LOQ), FALSE, &ret_val);
	if(ret_val == OK)
	{
		new_locq = (LOQ *)temp;

		LOgt(new_locq->LOQ_buf, &(new_locq->LOQ_loc));

		(void)QUinsert((QUEUE *) new_locq, (QUEUE *) &locq.q_next);
	}

	return(ret_val);
}


/*
	LOreset()
		reset the location of the current process to the value last
			entered in the queue.

		Returns
			OK		--	sucess
			LONOSAVE	--	queue is empty
			other status's if can't change to location saved.
*/

STATUS
LOreset()
{
	STATUS		ret_val = OK;
	STATUS		LOchange();
	QUEUE		*QUremove();
	QUEUE		*top;


	if (top = QUremove(locq.q_next))
		ret_val = LOchange(&(((LOQ *)(top))->LOQ_loc));
	else
		ret_val = LONOSAVE;

	return(ret_val);
}
