/*
** Copyright 1993, 2008 Ingres Corporation
*/

#include    <compat.h>

#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
#include    <efndef.h>
#include    <iledef.h>
#include    <iosbdef.h>
#include    <jpidef.h>
#include    <ssdef.h>
#include    <syidef.h>
#include    <starlet.h>

static STATUS vms_perfstat();

/**
**
**  Name: TMPRFSTAT.C - return per-process performance statistics.
**
**  Description:
**
**	Returns per-process performance statistics.  Implements TMperfstat()
**	function.  Currently contains implementations for getrusage() machines
**	(BSD - mostly) and XXX machines (SVR4 - mostly).
**
**	External routines:
**
**	    TMperfstat()  - Return performance statistics available from the OS.
**
**	Internal routines:
**	
**	    default_perfstat()   - default (null) implementation of TMperfstat()
**	    vms_perfstat() - VMS implementation of TMperfstat()
**
**  History:
**      26-jul-93 (mikem)
**	    Created.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	    Remove non-VMS code.
**	13-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Replace uses of
**	    CL_SYS_ERR by CL_ERR_DESC.
**/


/*
**  Definition of static variables and forward static functions.
*/

static STATUS default_perfstat(
    TM_PERFSTAT *stat,
    CL_ERR_DESC *sys_err);

static TM_PERFSTAT default_tmperf = {
    {-1, -1}, 	/* stat->tmperf_utime    */
    {-1, -1}, 	/* stat->tmperf_stime    */
    {-1, -1}, 	/* stat->tmperf_cpu      */
    -1, 	/* stat->tmperf_maxrss   */
    -1,		/* stat->tmperf_idrss    */
    -1,		/* stat->tmperf_minflt   */
    -1,		/* stat->tmperf_majflt	 */
    -1,		/* stat->tmperf_nswap	 */
    -1,		/* stat->tmperf_reads	 */
    -1,		/* stat->tmperf_writes	 */
    -1,		/* stat->tmperf_dio   	 */
    -1,		/* stat->tmperf_msgsnd	 */
    -1,		/* stat->tmperf_msgrcv	 */
    -1,		/* stat->tmperf_msgtotal */
    -1,		/* stat->tmperf_nsignals */
    -1,		/* stat->tmperf_nvcsw	 */
    -1		/* stat->tmperf_nivcsw	 */
};


/*{
** Name: TMperfstat()   - Return performance statistics available from the OS.
**
** Description:
**      Returns available performance statistics available from the OS for
**      the current process.  As best as is possible, per platform, TMperfstat()
**      provides a snapshot of all performance statistics available and
**      returns the statistics by filling in the TM_PERFSTAT structure passed
**      in by the caller.  Implimentations of this function will attempt to
**      collect the data required with the minimum possible affect on the
**      state being measured (ie. will try to make a single system call vs.
**      multiple system calls where possible).
**
**      The members of the TM_PERFSTAT structure are public and are defined in
**      <tm.h>;  see <tm.h> for description of current members of TM_PERFSTAT
**      structure.
**
**      Some statistics may not be available on some operating systems, if
**      a statistic is not available then TMperfstat() will initialize the
**      field to the value "-1".  In the case of SYSTIME structure members
**      one must test both members of the structure for -1.
**
**      It is expected that over time new CL proposals will add members to
**      the TM_PERFSTAT structure as new OS performance statistics become
**      available.   Clients which access the structure members by name,
**      and do not make structure order assumptions will be upward compatible
**      with future structure definition changes.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**      sys_err
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jul-93 (mikem)
**         Created.
*/
STATUS 
TMperfstat(
    TM_PERFSTAT *stat,
    CL_ERR_DESC *sys_err)
{
    STATUS	status;

    CL_CLEAR_ERR(sys_err);
    if ((status = vms_perfstat(stat, sys_err)) != OK)
	(void) default_perfstat(stat, sys_err);

    return(status);
}


/*{
** Name: vms_perfstat()   - VMS implementation of TMperfstat()
**
** Description:
**      Implementation of TMperfstat() on VMS.
**
**	The current implementation supports the following structure members:
**		tmperf_cpu
**		tmperf_maxrss
**		tmperf_idrss
**		tmperf_majflt
**		tmperf_dio
**		tmperf_msgtotal
**	all other members are set to -1.
**
**	It is believed that this implementation should run on both vms vax 
**	and vms alpha.
**
**      See TMperfstat() for function details.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**      sys_err
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jul-93 (mikem)
**         Created.
*/
static STATUS
vms_perfstat(
    TM_PERFSTAT *stat,
    CL_ERR_DESC *sys_err)
{
    IOSB	iosb;
    ILE3	syi_itmlst[2];
    int		system_status;
    STATUS	status = OK;
    static int	bytes_per_page = -1;
    ILE3        jpiget[] = { 
        /* JPI$_CPUTIM - proc's accumulated CPU time in 10-millisecond ticks */
        {4, JPI$_CPUTIM, (PTR)&stat->tmperf_cpu.TM_msecs, 0},

	/* JPI$_VIRTPEAK - the peak virtual address size of the process */
        {4, JPI$_VIRTPEAK, (PTR)&stat->tmperf_maxrss, 0},

	/* JPI$_PPGCNT - number of pages the process has in the working set */
        {4, JPI$_PPGCNT, (PTR)&stat->tmperf_idrss, 0},

	/* JPI$_PAGEFLTS - the number of page faults incurred by the process */
        {4, JPI$_PAGEFLTS, (PTR)&stat->tmperf_majflt, 0},

	/* JPI$_DIRIO - the number of direct I/O operations of the process */
        {4, JPI$_DIRIO, (PTR)&stat->tmperf_dio, 0},

	/* JPI$_BUFIO - the number of buffered I/O operations of the process */
        {4, JPI$_BUFIO, (PTR)&stat->tmperf_msgtotal, 0},

	/* end of list */
	{0, 0, 0, 0} };

    CL_CLEAR_ERR(sys_err);
    /* set everything we won't be using to -1 */
    default_perfstat(stat, sys_err);

    if (bytes_per_page == -1)
    {
#ifdef  alpha
	/* determine number of bytes per page - only need to do once, use
	** system call so that on it works on both vax vms and alpha vms.
	** This code has not been tested, it is meant as hint to the alpha
	** team.
	*/

	syi_itmlst[0].syi$w_buflen = 4;
	syi_itmlst[0].syi$w_itemcode = SYI$_PAGE_SIZE;
	syi_itmlst[0].syi$a_bufadr = &bytes_per_page;
	syi_itmlst[0].syi$a_reslen = 0;

	syi_itmlst[1].syi$w_buflen = 0;
	syi_itmlst[1].syi$w_itmcod = 0;
	syi_itmlst[1].syi$gt_bufadr = 0;
	syi_itmlst[1].syi$gw_reslen = 0;

	system_status = sys$getsyiw(EFN$C_ENF, 0, 0, syi_itmlst, &iosb, 0, 0);
	if (system_status & 1)
	    system_status = iosb.iosb$w_status;
	if ((system_status & 1) == 0)
	{
	    return(FAIL);
	}
#else
	bytes_per_page = 512;
#endif  /* alpha */
    }

    /* get the system statistics */

    system_status = sys$getjpiw(EFN$C_ENF, 0, 0, &jpiget, &iosb, 0, 0);
    if (system_status & 1)
	system_status = iosb.iosb$w_status;
    if (!(system_status & 1))
    {
	status = FAIL;
    }
    else
    {
	/* adjust units of system stats */

	/* vms gives 10's of ms. we want just ms. */
	stat->tmperf_cpu.TM_msecs *= 10;

	/* vms give pages we want bytes */
	stat->tmperf_maxrss *= bytes_per_page;
	stat->tmperf_idrss *= bytes_per_page;
    }

    return(status);
}


/*{
** Name: default_perfstat()   - default (null) implementation of TMperfstat()
**
** Description:
**	If no system support exists to gather performance statistics on the
**	machine then the following implementation is used.  This implementation
**	simply initializes all fields of the structure to -1.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**
**      sys_err				system specific error.
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jul-93 (mikem)
**         Created.
*/
/* ARGSUSED */
static STATUS
default_perfstat(
    TM_PERFSTAT *stat,
    CL_ERR_DESC *sys_err)
{
    /* assign -1 to all fields */

    *stat = default_tmperf;

    return(TM_NOSUPPORT);
}
