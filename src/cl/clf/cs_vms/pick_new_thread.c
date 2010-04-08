# include	<compat.h>
# include	<bt.h>
# include	<ints.h>
# include	<cs.h>

FUNC_EXTERN CS_SCB * CS_xchng_thread();

/*
**	pick_new_thread	-	given a pointer to the SCB of the current thread, the 
**						ready mask from Cs_srv_block, determine the highest 
**						ready queue bit and call CS_xchng_thread to
**						get a pointer to the SCB of the new thread to be 
**						started.
**	19-jul-2000 (kinte01)
**	   Add missing external function references
*/

CS_SCB *
pick_new_thread(CS_SCB * curr_scb, int ready_mask)
{
	int	rdymsk, ffs;

	rdymsk = ready_mask;
	ffs = 15 - BTnext(-1, &rdymsk, 16);

	return CS_xchng_thread(curr_scb, ffs);
}
