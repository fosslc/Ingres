/*
**Copyright (c) 2004 Ingres Corporation
*/

#include   <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <er.h>
#include   <cs.h>
#include    <fdset.h>
#include   <csev.h>
#include   <di.h>
#include   <dislave.h>
#include   "dilocal.h"

/**
**
**  Name: DILOCAL.C - This file contains routines local to DI which are
**		      common across alot of DI modules.
**
**  Description: 
**
**
**
**  History:
**	30-nov-1992 (rmuth)
**          Created.
**      10-mar-1993 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating.  Before causing a slave to take action the master will 
**	    set the slave control block status to DI_INPROGRESS, the slave in 
**	    turn will not change this status until it has completed the 
**	    operation.
**
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-july-1993 (mikem)
**	    Added externs of gen_{P,V}sem() to quiet warnings on su4_us5 build.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h, GLOBALDEFs to 
**	    dilru.c
**	16-Nov-1998 (jenjo02)
**	    Distinguish between Log I/O, Disk I/O, Read or Write,
**	    when suspended waiting for slave.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Forward and/or External typedef/struct references.
*/
FUNC_EXTERN     STATUS  gen_Psem();
FUNC_EXTERN     STATUS  gen_Vsem();

/*
**  Forward and/or External function references.
*/

/*
**  Defines of other constants.
*/


/* typedefs go here */

/*
** Definition of all global variables owned by this file.
*/



/*
**  Definition of static variables and forward static functions.
*/

/*{
** Name: DI_slave_send - Send an event to the slave and suspend until
**			 completion.
**
** Description:
**
** Inputs:
**	slave_number	slave id
**	di_op		A preformatted Slave event control block.
**
** Outputs:
**	big_status	Indicates if an error occured which we cannot
**			recover from.
**	small_status	Indicates an error occured which we can recover
**			from.
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**	  VOID
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**      10-mar-1993 (mikem)
**          Changed the type of the first parameter to DI_send_ev_to_slave() and
**          the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave()
**          could access the slave control block's status.
**          This routine will now initialize the status to DI_INPROGRESS, before
**          making the request and the slave will change the status once the
**          operation is complete.
**
*/
VOID
DI_slave_send(
    i4		slave_number,
    DI_OP     	*di_op,
    STATUS	*big_status,
    STATUS	*small_status,
    STATUS	*intern_status)
{
    STATUS status;

    do
    {
	status = DI_inc_Di_slave_cnt( slave_number, intern_status );
	if ( status != OK )
	{
	    *big_status = status;
	    break;
	}

	status = DI_send_ev_to_slave( di_op, slave_number, intern_status );
	if ( status != OK )
	{
	    *small_status = status;

	}

	status = DI_dec_Di_slave_cnt( slave_number, intern_status );
	if ( status != OK )
	{
	    *big_status = status;
	    break;
	}
    } while (FALSE);

    return;
}

/*{
** Name: DI_inc_Di_slave_cnt - Autoincrements an element in the 
**			       Di_slave_cnt structure;
**
** Description:
**
** Inputs:
**	Number		Index into Di_slave_cnt array
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**
*/
STATUS
DI_inc_Di_slave_cnt(
    i4		number,
    STATUS	*intern_status)
{

    STATUS    status;

    do
    {
        status = gen_Psem(&DI_sc_sem);
        if ( status != OK )
	{
	    *intern_status = DI_LRU_GENPSEM_ERR;
	    break;
	}

	++Di_slave_cnt[ number ];

	status = gen_Vsem(&DI_sc_sem);
	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    *intern_status = DI_LRU_GENVSEM_ERR;
	    break;
	}

    } while (FALSE);

    return( status );
}

/*{
** Name: DI_dec_Di_slave_cnt - Autodecrement an element in the 
**			       Di_slave_cnt structure;
**
** Description:
**
** Inputs:
**	Number		Index into Di_slave_cnt array
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**
*/
STATUS
DI_dec_Di_slave_cnt(
    i4		number,
    STATUS	*intern_status)
{

    STATUS    status;

    do
    {
        status = gen_Psem(&DI_sc_sem);
        if ( status != OK )
	{
	    /*
	    ** This is very bad as we may have corrupted our internal
	    ** DIlru/DIslave structures. But this event can occur
	    ** due to a thread being aborted
	    */
	    --Di_slave_cnt[ number ];
	    *intern_status = DI_LRU_GENPSEM_ERR;
	    break;
	}

	--Di_slave_cnt[ number ];

	status = gen_Vsem(&DI_sc_sem);
	if ( status != OK )
	{
	    Di_fatal_err_occurred = TRUE;
	    *intern_status = DI_LRU_GENVSEM_ERR;
	    break;
	}

    } while (FALSE);

    return( status );
}

/*{
** Name: DI_send_ev_to_slave - Send an event to a sepecfied slave
**
** Description:
**
** Inputs:
**	di_op		Event control block.
**	slave_number	Slave who we want to send event to.
**
** Outputs:
**	intern_status	Internal error indicating reason for failure.
**
**    Returns:
**        OK
**	  A range of DIlru errors .
**
**    Exceptions:
**        none
**
** Side Effects:
**
** History:
**    30-nov-1992 (rmuth)
** 	    Created.
**      10-mar-1993 (mikem)
**          bug #47624
**          Bug 47624 resulted in CSsuspend()'s from the DI system being woken
**          up early.  Mainline DI would then procede while the slave would
**          actually be processing the requested asynchronous action.  Various
**          bad things could happen after this depending on timing: (mainline
**          DI would change the slave control block before slave read it,
**          mainline DI would call DIlru_release() and ignore it failing which
**          would cause the control block to never be freed eventually leading
**          to the server hanging when it ran out of slave control blocks, ...
**
**          Fixes were made to scf to hopefully eliminate the unwanted
**          CSresume()'s.  In addition defensive code has been added to DI
**          to catch cases of returning from CSresume while the slave is
**          operating.  Before causing a slave to take action the master will
**          set the slave control block status to DI_INPROGRESS, the slave in
**          turn will not change this status until it has completed the
**          operation.
**
**	    Changed first argument to be a DI_OP pointer so that we could set
**	    disl->status in this one place rather than in all the callers.
**	16-Nov-1998 (jenjo02)
**	    Distinguish between Log I/O, Disk I/O, Read or Write,
**	    when suspended waiting for slave.
**
*/
STATUS
DI_send_ev_to_slave(
    DI_OP     	*di_op,
    i4		slave_number,
    STATUS	*intern_status )
{
    STATUS	status;

    di_op->di_evcb->status = DI_INPROGRESS;
    status = CScause_event(di_op->di_csevcb, slave_number);
    if ( status != OK )
    {
	*intern_status = DI_LRU_CSCAUSE_EV_ERR;
    }
    else
    {
	i4	suspend_mask;

	if (di_op->di_evcb->file_op == DI_SL_WRITE)
	    suspend_mask = CS_IOW_MASK;
	else 
	    suspend_mask = CS_IOR_MASK;

	if (di_op->di_evcb->io_open_flags & DI_O_LOG_FILE_MASK)
	    suspend_mask |= CS_LIO_MASK;
	else
	    suspend_mask |= CS_DIO_MASK;

	status = CSsuspend( suspend_mask, 0, (PTR)di_op->di_csevcb );
	if ( status != OK )
        {
	    *intern_status = DI_LRU_CSSUSPEND_ERR;
	}
    }

    return(status);
}
