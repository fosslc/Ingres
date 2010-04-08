# include	<compat.h>  
# include	<gl.h>
# include	<qu.h>  
# include	<me.h>  
# include	<er.h>  
# include	<lo.h>  
# include     	"lolocal.h"


/*
	LOqueue.c
		contains the routines
			LOsave()
			LOreset()

		History
			05/04/83	--	(gb)	written (whoopie)
**			09/23/83	--	(dd)  	VMS CL	""  ""
**      16-jul-93 (ed)
**	    added gl.h
**	26-oct-01 (kinte01)
**	    correct compiler warnings
*/

typedef	struct
{
	QUEUE		LOQ_q;
	LOCATION	LOQ_loc;
	char		LOQ_buf[MAX_LOC+1];
}	LOQ;

globaldef QUEUE	locq	=	{
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

	temp = MEreqmem(0,sizeof(LOQ),FALSE,&ret_val);

	if (temp != NULL)
	{
		new_locq = (LOQ *)temp;

		LOgt(new_locq->LOQ_buf, &(new_locq->LOQ_loc));
		QUinsert((QUEUE *) new_locq, &locq);
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
	QUEUE		*QUremove();
	QUEUE		*top;
	if (top = QUremove(locq.q_next))
		ret_val = LOchange(&(((LOQ *)(top))->LOQ_loc));
	else
		ret_val = LONOSAVE;

	return(ret_val);
}
