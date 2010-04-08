/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <mh.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <sr.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dm0pbmcb.h>

/**
** Name: DMTSCOST.C - Functions used to cost a SORT.
**
** Description:
**
**      This file contains the functions necessary to estimate 
**      the cost of a sort.  This file defines:
**    
**      dmt_sort_cost()   -  Routine to perform the sort
**                           cost operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	10-dec-90 (rogerk)
**	    Added support for DMF_SCANFACTOR server parameter.
**	 8-jul-1992 (walt)
**	    Prototyping DMF.
**	26-oct-1992 (rogerk)
**	    Added include of LG.H as its now needed to include dm0pbmcb.
**      10-nov-1992 (rogerk)
**	    Added include of DMP.H as it is now needed by DMSE.H
**	14-dec-92 (jrb)
**	    dmse_cost no longer returns a status.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-1993 (bryanp)
**	    In a star server, this routine may get called, but there is no
**		local buffer manager. In this case, use the svcb_c_buffers
**		field as the dmt_cache_size to return. Yeah, it's garbage, but
**		apparently star thinks it's useful information...
**      09-sep-1993 (smc)
**          Moved include of cs.h ahead of lg.h to define CS_SID.
**      06-mar-1996 (stial01)
**          Return dmt_io_blocking, dmt_cache_size for each pool.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: dmt_sort_cost - Estimates the cost of a sort.
**
**  INTERNAL DMF call format:      status = dmt_sort_cost(&dmt_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMT_SORT_COST,&dmt_cb);
**
** Description:
**    The dmt_sort_cost function determines the cost of a sort run.
**    The estimated cpu cost is the number of comparisions needed, and the
**    estimated io cost is the number of logical disk transfers.
**
** Inputs:
**      dmt_cb 
**          .type                           Must be set to DMT_CB.
**          .length                         Must be at least sizeof(DMT_CB).
**          .dmt_flags_mask                 Must be zero.
**          .dmt_s_estimated_records        Estimate of the number of records
**                                          to be sorted.
**          .dmt_s_width                    Width in bytes of input and 
**                                          output records.
**
** Output:
**      dmt_cb 
**          .dmt_s_io_cost                  Estimated I/O cost.
**          .dmt_s_cpu_cost                 Estimated CPU cost.
**          .dmt_io_blocking		    Readahead blocking factor.
**	    .dmt_cache_size		    Size of Local Cache.
**						We return the size of the single
**						page portion of the local cache,
**						not including any group pages.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM001A_BAD_FLAG
**
**
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmt_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmt_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	18-oct-1988 (rogerk)
**	    Added dmt_cache_size field.
**	10-dec-90 (rogerk)
**	    Added support for DMF_SCANFACTOR server parameter.
**	    Use svcb_scanfactor value for the blocking factor rather than
**	    the number of pages in a group buffer.  The scanfactor value
**	    defaults to the group count but can be overridden by a server
**	    startup parameter.
**	14-dec-92 (jrb)
**	    dmse_cost no longer returns a status.
**	23-aug-1993 (bryanp)
**	    In a star server, this routine may get called, but there is no
**		local buffer manager. In this case, use the svcb_c_buffers
**		field as the dmt_cache_size to return. Yeah, it's garbage, but
**		apparently star thinks it's useful information...
**      06-mar-1996 (stial01)
**          Return dmt_io_blocking, dmt_cache_size for each pool.
**      25-Sep-2008 (hanal04) Bug 120942
**          Float overflows in OPF can lead to NaN values for the
**          dmt_s_estimated_records. Return an error if this happens to
**          prevent MH_BADARG being signalled in MHln() from dmse_cost().
**          If we generate a signal we'll bring down the DBMS.
*/

DB_STATUS
dmt_sort_cost(
DMT_CB    *dmt_cb)
{
    DM_SVCB	  *svcb = dmf_svcb;
    DB_STATUS	  s;
    DMP_LBMCB     *lbm = svcb->svcb_lbmcb_ptr;
    DM0P_BMCB     *bm;
    i4            cache_ix;
    i4            local_error;

    for (cache_ix = 0; cache_ix < DM_MAX_CACHE; cache_ix ++)
    {
	dmt_cb->dmt_io_blocking[cache_ix] = svcb->svcb_scanfactor[cache_ix];

	if (lbm &&
		(bm = lbm->lbm_bmcb[cache_ix])) /* assign bm */
	{
	    dmt_cb->dmt_cache_size[cache_ix] = bm->bm_sbufcnt;
	}
	else
	    dmt_cb->dmt_cache_size[cache_ix] = svcb->svcb_c_buffers[cache_ix];
    }

    if(MHisnan(dmt_cb->dmt_s_estimated_records))
    {
        /* Caller has passed NaN which will bring the DBMS down */
        uleFormat(NULL, E_DM9717_SORT_COST_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                   (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmt_cb->error, 0, E_DM004A_INTERNAL_ERROR);
        return(E_DB_ERROR);
    }

    dmse_cost(dmt_cb->dmt_s_estimated_records, dmt_cb->dmt_s_width,
	&dmt_cb->dmt_s_io_cost, &dmt_cb->dmt_s_cpu_cost);

    return (E_DB_OK);
}
