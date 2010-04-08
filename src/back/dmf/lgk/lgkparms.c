/*
** Copyright (c) 1993, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <si.h>
#include    <tm.h>
#include    <cs.h>
#include    <er.h>
#include    <tr.h>
#include    <pm.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <lgkdef.h>
#include    <lgkparms.h>

/**
**
**  Name: LGKPARMS.C - get rcp system parameters.
**
**  Description:
**	This module contains routines to get recovery system parameters.
**	
**	These routines were in dmfrcp.c, then lgkm.c, now here.
**   
**
**  History:
**	26-apr-1993 (andys/bryanp)
**	    Created as part of the 6.5 Cluster Support project.
**	    get_lgk_config, lgk_get_counts, lgk_calculate_size extracted 
**	    from lgkm.c and moved here. 
**	    Change get_lgk_config to lgk_get_config.
**	    Don't doubly multiply blocksize by 1024. Pass wordy parameter
**	    through to lgk_get_config so that iishowres -d is more verbose.
**	    Removed the extra blank lines in the wordy output.
**	    This code uses both SI and TR to display output, which is gross.
**	    PMget calls don't return a sys_err, so don't try to pass a sys_err
**		to ule_format if the PMget call fails.
**	24-may-1993 (bryanp)
**	    Improve error message logging for invalid parameter values.
**	    Clear sys_err so that garbage errors aren't report.
**	    Add support for 2K log file page size.
**	    Add support for separate RSB and LKB tables in the locking system.
**	21-June-1993 (rmuth)
**	    PM requires that we set the default nodename if we want to
**	    pick up using '$'.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>, <cv.h>, <si.h>
**	26-Jul-1993 (jnash)
**	    Add support for locking the LGK data segment via 
**	    ii.$.rcp.log.lgk_memory_lock .
**	20-Sep-1993 (jnash)
**	    Make rconf->cp_p an f8, allowing CP logfile percentage to include 
**	    fractional component.
**	28-mar-1994 (jnash)
**	    Replace cp_percent with cp_interval.
**	23-may-1994 (jnash)
**	    Change "pm not found" logfull from 95% to  90% of log file and 
**	    force abort limit from 80% to 75%.  These changes reduce 
** 	    the liklihood of hitting "absolute logfull".
**	08-May-1998 (jenjo02)
**	    Partitioned log files. Added nparts to RCONF into which number
**	    of log partitions is returned.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-Mar-2000 (jenjo02)
**	    Get number of log file partitions from (new) PM variable
**  	    "ii.$.rcp.log.log_file_parts".
**	    Adjust buffer size computation to include LBBs.
**	    RSBs now have embedded LKBs, so adjust size needed for LKBs
**	    appropriately.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-apr-2001 (devjo01)
**	    s103715 generic cluster.  Added CX mem requirements to
**	    'lgk_calculate_size' calculations.
**      23-Oct-2002 (hanch04)
**          Changed memory size variables to use a SIZE_TYPE to allow
**          memory > 2 gig
**	3-Dec-2003 (schka24)
**	    Deprecate cp_interval in favor of cp_interval_mb.
**	    Update a few of the builtin defaults to something a little
**	    closer to the typical CBF default.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	04-aug-2005 (abbjo03)
**	    Correct the upper range for the E_DMA806 error for cp_interval_mb.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*
**  Forward function references.
*/


/*
**  Name: lgk_get_config	- get the Logging/Locking configuration parms.
**
** Description:
**	This routine calls upon PM to get the Logging/Locking configuration
**	parms for the local node. It converts these values to numbers, scales
**	some of them to appropriate units, and stores them into the caller's
**	RCONF.
**
**	If asked to, this routine also echos the parameter values to the
**	process's trace log.
**
** Inputs:
**	wordy			- should we echo the parameters?
**
** Outputs:
**	rconf			- filled in with the LG/LK parameter values
**	sys_err			- system specific error code.
**
** Returns:
**	STATUS
**
**  History:
**	29-apr-1993 (andys/keving/bryanp)
**	    Created from dmr_get_lglk_parms.
**	    Added functions to get logging and locking parameters from
**		PM and calculate size required for lgk shared memory.
**	    Change name to lgk_get_config.
**	    Corrected argument to PMload. Should be err_func, not sys_err.
**	    Following LRC "review", change db_limit to database_limit, and
**		change force_abort to force_abort_limit.
**	    PMget calls don't return a sys_err, so don't try to pass a sys_err
**		to ule_format if the PMget call fails.
**	24-may-1993 (bryanp)
**	    Improve error message logging for invalid parameter values.
**	    Add support for 2K log file page size.
**	    Add support for separate RSB and LKB tables in the locking system.
**	21-June-1993 (rmuth)
**	    PM requires that we set the default nodename if we want to
**	    pick up using '$'. Currentlt we do this by pciking up from
**	    the GC nodename.
**	26-Jul-1993 (jnash)
**	    Add support for locking the LGK data segment via 
**	    ii.$.rcp.log.lgk_memory_lock.
**	20-Sep-1993 (jnash)
**	    Make rconf->cp_p an f8, allowing CP logfile percentage to include 
**	    fractional component.
**	16-dec-1993 (tyler)
**	    Replaced GChostname() with PMhost() call.
**	28-mar-1994 (jnash)
**	    Replace cp_percent with cp_interval.
**	23-may-1994 (jnash)
**	    Change "pm not found" logfull from 95% to  90% of log file and 
**	    force abort limit from 80% to 75%.  These changes reduce 
** 	    the liklihood of hitting "absolute logfull".
**	08-May-1998 (jenjo02)
**	    Partitioned log files. Added nparts to RCONF into which number
**	    of log partitions is returned.
**	21-Mar-2000 (jenjo02)
**	    Get number of log file partitions from (new) PM variable
**  	    "ii.$.rcp.log.log_file_parts".
**	16-May-2003 (jenjo02)
**	    Fudge LKB allocation to be a minimum of 1/2 again the
**	    number of RSBs.
**	04-aug-2005 (abbjo03)
**	    Correct the upper range for the E_DMA806 error for cp_interval_mb.
**	21-Mar-2006 (jenjo02)
**	    Add ii.$.rcp.log.optimize_writes for optimized log
**	    writes.
**      26-Apr-2007 (hanal04) SIR 118199
**          Add timestamp to wordy header to help support.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add ii.$.rcp.log.readbackward_blocks,
**	    ii.$.rcp.log.readforward_blocks for buffered log reads.
*/
STATUS
lgk_get_config(RCONF *rconf, CL_ERR_DESC *sys_err, i4  wordy)
{
    STATUS	status;
    i4	err_code;
    char	*pm_str;
    char	*parm_name;

    CL_CLEAR_ERR(sys_err);

    /* Get the configuration data. */

    PMinit();
    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)NULL);
    if (status)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    /*
    ** Setup the PM default nodename, non-pseudo servers will already have
    ** made these calls in SCF startup. 
    */
    PMsetDefault( 1, PMhost() );
 
    if (wordy)
        TRdisplay("%@ Retrieving configuration data for the logging system.\n");

    parm_name = "ii.$.rcp.log.buffer_count";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->bcnt);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->bcnt = 20;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    if (rconf->bcnt < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->bcnt);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
	
    parm_name = "ii.$.rcp.log.tx_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->blks);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->blks = 250;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->blks < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->blks);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
	
    parm_name = "ii.$.rcp.log.database_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->ldbs);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->ldbs = 32;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->ldbs < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->ldbs);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    parm_name = "ii.$.rcp.log.archiver_interval";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->cpcnt);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->cpcnt = 1;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    /* It's hard to max-limit archiver interval without knowing how big a CP
    ** is yet, but there's very little reason to allow giant numbers.
    */
    if (rconf->cpcnt < 1 || rconf->cpcnt > 20)
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->cpcnt,
		    0, 1, 
		    0, 90);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    parm_name = "ii.$.rcp.log.block_size";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->bsize);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->bsize = 4;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->bsize == 2  || rconf->bsize == 4  ||
	rconf->bsize == 8  || rconf->bsize == 16 ||
	rconf->bsize == 32 || rconf->bsize == 64)
	/* OK */;
    else
    {
	uleFormat(NULL, E_DMA807_LGK_BLKSIZE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->bsize);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    rconf->bsize *= 1024;

    parm_name = "ii.$.rcp.log.full_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->lf_p);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->lf_p = 90;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->lf_p >= 100 || rconf->lf_p <= 0)
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->lf_p,
		    0, 1, 
		    0, 99);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    parm_name = "ii.$.rcp.log.cp_interval";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVaf(pm_str, '.', &rconf->cp_p);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->cp_p = 5;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->cp_p >= 100 || rconf->cp_p <= 0)
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, (i4)rconf->cp_p,
		    0, 1,
		    0, 99);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    parm_name = "ii.$.rcp.log.force_abort_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->abort_p);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->abort_p = 75;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->abort_p >= 100 || rconf->abort_p <= 0)
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->abort_p,
		    0, 1,
		    0, 99);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    /* Get the number of log file partitions */
    parm_name = "ii.$.rcp.log.log_file_parts";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->nparts);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->nparts = 1;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->nparts > LG_MAX_FILE || rconf->nparts <= 0)
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->nparts,
		    0, 1,
		    0, LG_MAX_FILE);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    /* Get the log file size in KB */
    parm_name = "ii.$.rcp.file.kbytes";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->logkb);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
    {
	parm_name = "ii.$.rcp.file.size";
	status = PMget(parm_name, &pm_str);
	if (status == OK)
	{
	    status = CVal(pm_str, &rconf->logkb);
	    if (status)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, 
			(i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, parm_name,
			0, pm_str);
		return (E_DMA805_LGK_CONFIG_ERROR);
	    }
	    rconf->logkb = (rconf->logkb + 1023) / 1024;
	}
    }
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    parm_name = "ii.$.rcp.log.cp_interval_mb";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->cp_mb);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	/* No cp_interval_mb, use the old cp_interval instead (defaulted to
	** 5% above).  We'll believe the rcp.file.kbytes parameter, although
	** the only 100% reliable log file size is buried in the log file
	** header.  We can't get at that yet.
	*/
	if (rconf->logkb > 0)
	{
	    rconf->cp_mb = ((i4) ((f8) rconf->logkb * rconf->cp_p / 100.0) + 512) / 1024;
	    if (rconf->cp_mb == 0) rconf->cp_mb = 1;
	}
	else
	{
	    rconf->cp_mb = 5;			/* Fallback default */
	}
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    /* Upper limit is 1/3 of log */
    if (rconf->cp_mb <= 0 ||
	(rconf->logkb > 0 && rconf->cp_mb >= rconf->logkb/2048))
    {
	uleFormat(NULL, E_DMA806_LGK_PARM_RANGE_ERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 5,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->cp_mb,
		    0, 1,
		    0, rconf->logkb/2048);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    /* These only allowed with single-partition log files */
    rconf->optimwrites = FALSE;
    rconf->rbblocks = 1;
    rconf->rfblocks = 1;

    if ( rconf->nparts == 1 )
    {
	parm_name = "ii.$.rcp.log.optimize_writes";
	status = PMget(parm_name, &pm_str);
	if (status == OK)
	{
	    if (STcasecmp(pm_str, "ON") == 0)
	    {
		rconf->optimwrites = TRUE;
	    }
	    else if (STcasecmp(pm_str, "OFF" ) != 0)
	    {
		uleFormat(NULL, E_DMA809_LGK_NO_ON_OFF_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, parm_name,
			0, pm_str);
		return (E_DMA805_LGK_CONFIG_ERROR);
	    }
	}
	else if (status != PM_NOT_FOUND)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L,
		(i4 *)NULL, &err_code, 0);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}

	parm_name = "ii.$.rcp.log.readbackward_blocks";
	status = PMget(parm_name, &pm_str);
	if (status == OK)
	{
	    status = CVal(pm_str, &rconf->rbblocks);
	    if (status)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, 
			(i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, parm_name,
			0, pm_str);
		return (E_DMA805_LGK_CONFIG_ERROR);
	    }
	}
	else if (status != PM_NOT_FOUND)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L,
		(i4 *)NULL, &err_code, 0);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
	if ( rconf->rbblocks <= 0 )
	    rconf->rbblocks = 1;

	parm_name = "ii.$.rcp.log.readforward_blocks";
	status = PMget(parm_name, &pm_str);
	if (status == OK)
	{
	    status = CVal(pm_str, &rconf->rfblocks);
	    if (status)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, 
			(i4 *)NULL, &err_code, 0);
		uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
			0, parm_name,
			0, pm_str);
		return (E_DMA805_LGK_CONFIG_ERROR);
	    }
	}
	else if (status != PM_NOT_FOUND)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L,
		(i4 *)NULL, &err_code, 0);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
	if ( rconf->rfblocks <= 0 )
	    rconf->rfblocks = 1;
    }



    /**********************************************************************/
    /* Locking stuff:                                                     */
    /**********************************************************************/
    parm_name = "ii.$.rcp.lock.hash_size";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->lkh);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->lkh = 1009;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->lkh < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->lkh);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    parm_name = "ii.$.rcp.lock.resource_hash";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->rsh);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->rsh = rconf->lkh;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->rsh < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->rsh);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    parm_name = "ii.$.rcp.lock.lock_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->lkblk);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status ==PM_NOT_FOUND)
	rconf->lkblk = 8000;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->lkblk < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->lkblk);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    parm_name = "ii.$.rcp.lock.resource_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->resources);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->resources = rconf->lkblk;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->resources < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->resources);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    /* 
    ** To accomodate LKBs embedded in RSBs, there must be at
    ** least one LKB for each RSB. Those LKBs are never freed
    ** or made available to other RBSs (they are always "in use")
    ** so to manage multiple locks on a resource, silently up the
    ** LKB count to at least 1/2 again the number of RSBs.
    */
    if ( rconf->lkblk < rconf->resources + (rconf->resources >> 1) )
	rconf->lkblk = rconf->resources + (rconf->resources >> 1);
 
    parm_name = "ii.$.rcp.lock.list_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->lkllb);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->lkllb = 250;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->lkllb < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->lkllb);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    parm_name = "ii.$.rcp.lock.per_tx_limit";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	status = CVal(pm_str, &rconf->maxlkb);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, 0L, 
		    (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA808_LGK_NONNUM_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status == PM_NOT_FOUND)
	rconf->maxlkb = ((rconf->lkblk + rconf->resources) / rconf->lkllb) * 10;
    else
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, 
		(i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
    if (rconf->maxlkb < 0)
    {
	uleFormat(NULL, E_DMA804_LGK_NEGATIVE_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
		    0, parm_name,
		    0, pm_str,
		    0, rconf->maxlkb);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }

    rconf->lgk_mem_lock = FALSE;

    parm_name = "ii.$.rcp.log.lgk_memory_lock";
    status = PMget(parm_name, &pm_str);
    if (status == OK)
    {
	if (STcasecmp(pm_str, "ON") == 0)
	{
	    rconf->lgk_mem_lock = TRUE;
	}
	else if (STcasecmp(pm_str, "OFF" ) != 0)
	{
	    uleFormat(NULL, E_DMA809_LGK_NO_ON_OFF_PARM, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, parm_name,
		    0, pm_str);
	    return (E_DMA805_LGK_CONFIG_ERROR);
	}
    }
    else if (status != PM_NOT_FOUND)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L,
	    (i4 *)NULL, &err_code, 0);
	return (E_DMA805_LGK_CONFIG_ERROR);
    }
 
    rconf->dual_logging = 1;

    if (wordy)
    {
	TRdisplay("\nDisplay the configuration data entered.\n\n");

	TRdisplay("The log file size: %dKB\n",
		    rconf->logkb);
	TRdisplay("The number of log file partitions: %d\n",
		    rconf->nparts);
	TRdisplay("The number of log buffers in the memory : %d\n",
		    rconf->bcnt);
	TRdisplay(
	    "The maximum number of transactions in the logging system : %d\n",
		    rconf->blks);
	TRdisplay("The maximum number of databases in the logging system: %d\n",
		    rconf->ldbs);
	TRdisplay(
	    "The max consistency point interval for invoking archiver : %d\n",
		    rconf->cpcnt);
	TRdisplay("The block size of the log file : %d Kbytes\n",
		    rconf->bsize/1024);
	TRdisplay("The log-full-limit in percentage : %d\n", rconf->lf_p);
	TRdisplay("The consistency point size : %d Mbytes of log\n",
		    rconf->cp_mb);
	TRdisplay("The force-abort-limit in percentage : %d\n", rconf->abort_p);
	if ( rconf->optimwrites )
	    TRdisplay("Optimized log writes are enabled.\n");
	TRdisplay("The number of readbackward blocks : %d\n", rconf->rbblocks);
	TRdisplay("The number of readforward blocks : %d\n", rconf->rfblocks);

	TRdisplay("The size of the lock hash table : %d\n", rconf->lkh);
	TRdisplay("The size of the resource hash table : %d\n", rconf->rsh);
	TRdisplay("The maximum number of locks in the locking system : %d\n",
		    rconf->lkblk);
	TRdisplay("The maximum number of resources in the locking system: %d\n",
		    rconf->resources);
	TRdisplay(
	    "The maximum number of lock lists in the locking system : %d\n",
		    rconf->lkllb);
	TRdisplay("The maximum number of locks allowed per transaction : %d\n",
		    rconf->maxlkb);
	if (rconf->lgk_mem_lock)
	    TRdisplay("The shared LGK data segment is LOCKED.\n");
     }

    return (OK);
}


/*{
** Name: lgk_get_counts()	- get logging and locking system parameters.
**
** Description:
**	This routine is very similar to the lgk_get_config routine, but it
**	formats the values slightly differently and returns them in a different
**	structure. It is used by "iishowres", the utility that describes the
**	usage of the LG/LK shared memory.
**
** Inputs:
**      none.
**
** Outputs:
**      lglk_counts                     ptr to structure to fill in with lg/lk
**					user specified configuration info.
**
**	Returns:
**	    OK	- success.
**	    FAIL- some problem reading the file.
**
** History:
**      18-feb-90 (mikem)
**	    created.
**	21-mar-90 (mikem)
**	    Added extra error handling to deal with an "rcp.par" file with a bad
**	    format.  Now will PCexit(FAIL) if there are not enough lines in the
**	    rcp.par file, or if any of the fields we are interested in can't be
**	    converted from string to integer.
**	26-apr-1993 (bryanp/andys/keving)
**	    Removed usage of rcp.par file. Now we read resource sizes from
**		PM, we use get_lgk_config to return pertinent numbers.
**	    Moved from lgkmemusg.c to lgkm.c.
**	    Change get_lgk_config to lgk_get_config.
**	24-may-1993 (bryanp)
**	    Added lgk_max_resources support to track resource block usage.
*/
STATUS
lgk_get_counts( LGLK_INFO   *lglk_counts, i4  wordy )
{
    STATUS	ret_val = OK;
    RCONF	rconf;
    CL_ERR_DESC	sys_err;
	
    /* Use lgk_config to get all lgk parms */
    ret_val = lgk_get_config(&rconf, &sys_err, wordy);

    if (!ret_val)
    {
        /* Copy parameters into result lgkinfo block */
	lglk_counts->lgk_lock_hash = rconf.lkh; 
	lglk_counts->lgk_res_hash = rconf.rsh; 
	lglk_counts->lgk_log_bufs = rconf.bcnt; 
	lglk_counts->lgk_block_size = rconf.bsize; 
	lglk_counts->lgk_max_xacts = rconf.blks; 
	lglk_counts->lgk_max_dbs = rconf.ldbs; 
	lglk_counts->lgk_max_locks = rconf.lkblk; 
	lglk_counts->lgk_max_resources = rconf.resources;
	lglk_counts->lgk_max_list = rconf.lkllb; 
    }
    return(ret_val);
}

/*{
** Name: lgk_calculate_size()	- Calculate memory required for lg/lk
**
** Description:
**	Given the user specified lg/lk configuration parameter calculate the
**	total memory needed for the lg/lk shared memory segment.
**
** Inputs:
**      wordy				if non-zero then print out "wordy" 
**					    description of memory usage.
**	totsize				ptr to SIZE_TYPE for total size
**
** Outputs:
**	none.
**
**	Returns:
**	    OK
**	    FAIL			if memory required exceeds
**					    MAX_SIZE_TYPE
**
** History:
**	18-feb-90 (mikem)
**          created.
**	29-apr-1993 (bryanp)
**	    Lots of changes for the new PM-ized portable LG/LK system.
**	    Prototyped code for 6.5.
**	24-may-1993 (bryanp)
**	    Added support for separate resource and lock block configurations.
**	21-Mar-2000 (jenjo02)
**	    Adjust buffer size computation to include LBBs.
**	    RSBs now have embedded LKBs, so adjust size needed for LKBs
**	    appropriately.
**	18-apr-2001 (devjo01)
**	    s103715 generic cluster.  Change params passed to
**	    'lgk_calculate_size'.
**	16-May-2003 (jenjo02)
**	    Corrected LKB size calculation to include only those
**	    in excess of RSBs.
**	17-aug-2004 (thaju02)
**	    If total overflows, cap it at MAXI4 for now.
**	29-Dec-2008 (jonj)
**	    Remove MAXI4 cap, overflow only occurs now when mem needed
**	    overflows MAX_SIZE_TYPE, in which case we return FAIL.
**	    Correct undercalculation of log buffer memory.
*/
STATUS
lgk_calculate_size(
i4		wordy,
LGLK_INFO*	plgkcounts,
SIZE_TYPE	*totsize)
{
    SIZE_TYPE	total = 0;
    SIZE_TYPE	subtotal = 0;
    SIZE_TYPE	oldtotal;
    SIZE_TYPE	size;
    bool	overflowed = FALSE;

    /* lock lists */

    LK_size(LGK_OBJ0_LK_LLB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_list ) 
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_list);
    subtotal = (size * plgkcounts->lgk_max_list);

    LK_size(LGK_OBJ1_LK_LLB_TAB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_list )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_list);
    subtotal += (size * plgkcounts->lgk_max_list);

    if (wordy)
    {
	SIprintf("Memory allocation for %d lock lists = %lu bytes\n",
		    plgkcounts->lgk_max_list, subtotal);
    }

    /* locks */


    /* 
    ** Each RSB has an embedded LKB, so we need space
    ** only for those LKBs in excess of RSBs.
    */
    LK_size(LGK_OBJ2_LK_BLK, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < 
	    (plgkcounts->lgk_max_locks - plgkcounts->lgk_max_resources) )
	overflowed = TRUE;
    total += (size * (plgkcounts->lgk_max_locks - 
		    plgkcounts->lgk_max_resources));
    subtotal = (size * (plgkcounts->lgk_max_locks -  
		      plgkcounts->lgk_max_resources));
    /* ... but we need the full complement of LKB pointers */
    LK_size(LGK_OBJ3_LK_BLK_TAB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_locks )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_locks);
    subtotal += (size * plgkcounts->lgk_max_locks);

    if (wordy)
    {
	SIprintf("Memory allocation for %d locks = %lu bytes\n", 
		    plgkcounts->lgk_max_locks, subtotal);
    }

    /* resources */

    LK_size(LGK_OBJB_LK_RSB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_resources )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_resources);
    subtotal = (size * plgkcounts->lgk_max_resources);
    LK_size(LGK_OBJC_LK_RSB_TAB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_resources )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_resources);
    subtotal += (size * plgkcounts->lgk_max_resources);

    if (wordy)
    {
	SIprintf("Memory allocation for %d resource blocks = %lu bytes\n", 
		    plgkcounts->lgk_max_resources, subtotal);
    }

    /* hash tables */

    LK_size(LGK_OBJ4_LK_LKH, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_lock_hash )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_lock_hash);
    subtotal = (size * plgkcounts->lgk_lock_hash);
    LK_size(LGK_OBJ5_LK_RSH, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_res_hash )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_res_hash);
    subtotal += (size * plgkcounts->lgk_res_hash);

    if (wordy)
    {
	SIprintf("Memory allocation for hash tables (%d lock hash table,",
			plgkcounts->lgk_lock_hash);
	SIprintf("%d resource hash table) = %lu bytes.\n", 
			plgkcounts->lgk_res_hash, subtotal);
    }

    /* log buffers */

    LG_size(LGK_OBJD_LG_LBB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_log_bufs )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_log_bufs);
    subtotal = (size * plgkcounts->lgk_log_bufs);
    /* (Add another buffer-size for space lost in alignment) */
    if ( ((MAX_SIZE_TYPE - total)/plgkcounts->lgk_block_size) < 
	    plgkcounts->lgk_log_bufs+1 )
	overflowed = TRUE;
    total += ((plgkcounts->lgk_log_bufs+1) * plgkcounts->lgk_block_size);
    subtotal += ((plgkcounts->lgk_log_bufs+1) * plgkcounts->lgk_block_size);

    if (wordy)
    {
	SIprintf(
	    "Memory allocation for %d (%d-sized) log buffers = %lu bytes\n", 
		    plgkcounts->lgk_log_bufs, 
		    plgkcounts->lgk_block_size,
		    subtotal);
    }

    /* transactions */

    LG_size(LGK_OBJ7_LG_LXB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_xacts )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_xacts);
    subtotal = (size * plgkcounts->lgk_max_xacts);
    LG_size(LGK_OBJ8_LG_LXB_TAB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_xacts )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_xacts);
    subtotal += (size * plgkcounts->lgk_max_xacts);

    if (wordy)
    {
	SIprintf("Memory allocation for %d transactions = %lu bytes\n", 
		    plgkcounts->lgk_max_xacts, subtotal);
    }

    /* databases */

    LG_size(LGK_OBJ9_LG_LDB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_dbs )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_dbs);
    subtotal = (size * plgkcounts->lgk_max_dbs);
    LG_size(LGK_OBJA_LG_LDB_TAB, &size);
    if ( ((MAX_SIZE_TYPE - total)/size) < plgkcounts->lgk_max_dbs )
	overflowed = TRUE;
    total += (size * plgkcounts->lgk_max_dbs);
    subtotal += (size * plgkcounts->lgk_max_dbs);

    if (wordy)
    {
	SIprintf("Memory allocation for %d databases = %lu bytes\n", 
		    plgkcounts->lgk_max_dbs, subtotal);
    }

    /* internal memory */

    /* Misc LG structures */
    LG_size(LGK_OBJE_LG_MISC, &size);
    if ( (MAX_SIZE_TYPE - total) < size )
	overflowed = TRUE;
    total += (size);
    subtotal = (size);

    /* Misc LK structures */
    LK_size(LGK_OBJF_LK_MISC, &size);
    if ( (MAX_SIZE_TYPE - total) < size )
	overflowed = TRUE;
    total += (size);
    subtotal += (size);

    /* fudge factor for various administrative control blocks */

    if ( (MAX_SIZE_TYPE - total) < 8192 )
	overflowed = TRUE;
    total += 8192;
    subtotal += 8192;

    /* round total to next nearest ME_MPAGESIZE boundary */
    if ( MAX_SIZE_TYPE < (total + ME_MPAGESIZE) )
	overflowed = TRUE;
    oldtotal = total;
    total = (((total + ME_MPAGESIZE - 1) / ME_MPAGESIZE) * ME_MPAGESIZE);
    subtotal += (total - oldtotal);

    if (wordy)
    {
	SIprintf("Memory allocation for internal data structures = %lu bytes\n",
		    subtotal);
	if ( !overflowed )
	    SIprintf("Total allocation for logging/locking resources = %lu bytes\n",
			total);
    }

    *totsize = total;
    
    /* check for overflow */

    if ( overflowed && !wordy )
    {
        SIprintf("Total LG/LK allocation exceeds max of %lu bytes\n"
	         "Adjust logging/locking configuration values and try again\n",
		     MAX_SIZE_TYPE);
        return(FAIL);
    }

    return(OK);
}

