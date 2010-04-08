/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <bt.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <mo.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>

#include    <lg.h>
#include    <lk.h>

#include    <adf.h>
#include    <ulf.h>

#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>

#include    <dm.h>

#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmxe.h>
#include    <dm1b.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0llctx.h>
#include    <dm0pbmcb.h>

/*
** Name: DM0PMO.C		- Buffer Manager Managed Objects
**
** Description:
**	This file contains the routines and data structures which define
**	the managed objects for the DMF buffer manager.
**
** History:
**	18-oct-1993 (rmuth)
**	    Created, used ICL's code for dm0p_lbmcb_classes.
**	25-oct-1995 (stoli02/thaju02)
**	    Added buffer manager single and group synchronous writes.
**      06-mar-1996 (stial01)
**          Variable Page Size project: new classes for new BM objects.
**          Added DM0Pmo_bm_index, DM0Pmo_lbm_index
**	25-mar-96 (nick)
**	    Added FC wait statistic.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed bmcb_tblpsize, bmcb_tblpindex, lbm_tblpriority.
**	01-Apr-1997 (jenjo02)
**	    Added new stat for group waits.
**	03-Apr-1998 (jenjo02)
**	    New stats for WriteBehind threads.
**	01-Sep-1998 (jenjo02)
**	  o Moved BUF_MUTEX waits out of bmcb_mwait, lbm_mwait, into DM0P_BSTAT where
**	    mutex waits by cache can be accounted for.
**	24-Aug-1999 (jenjo02)
**	    DM0P_BSTAT expanded to included most stats by buffer type, 
**	    bs_?[BMCB_PTYPES] now contains the sum of all types.
**	10-Jan-2000 (jenjo02)
**	    Added stats by page type for groups.
**	    Added get methods for fixed, free, modified buffer/group
**	    counts.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-Jan-2003 (jenjo02)
**	    bs_stat name changes for Cache Flush Agents.
**      16-Apr-2008 (hanal04) Bug 120273
**          exp.dmf.dm0p.bm_reclaim and exp.dmf.dm0p.bm_replace
**          Are BM values, not lbm values. Call DM0Pmo_bm_index()
**          not DM0Pmo_lbm_index().
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add new objects for CR stats.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
**	19-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add cr_log_readio for physical log reads.
*/

static STATUS DM0Pmo_bm_index(
	i4     msg,
	PTR     cdata,
	i4     linstance,
	char    *instance,
	PTR     *instdata);

static STATUS DM0Pmo_lbm_index(
	i4     msg,
	PTR     cdata,
	i4     linstance,
	char    *instance,
	PTR     *instdata);

static MO_GET_METHOD DM0Pmo_fcount;
static MO_GET_METHOD DM0Pmo_mcount;
static MO_GET_METHOD DM0Pmo_lcount;
static MO_GET_METHOD DM0Pmo_gfcount;
static MO_GET_METHOD DM0Pmo_gmcount;
static MO_GET_METHOD DM0Pmo_glcount;


/*
** Managed objects for the buffer manager:
**     dm0p_lbm_common_classes:  
**       - Buffer manager statistics for this server 
**       - One record is returned.
*/
static MO_CLASS_DEF dm0p_lbm_common_classes[]=
{
     {0, "exp.dmf.dm0p.lbmc_lockreclaim", 
	 MO_SIZEOF_MEMBER(DMP_LBMCB,lbm_lockreclaim),
         MO_READ, 0, CL_OFFSETOF(DMP_LBMCB,lbm_lockreclaim),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.lbmc_fcflush", MO_SIZEOF_MEMBER(DMP_LBMCB,lbm_fcflush),
         MO_READ, 0, CL_OFFSETOF(DMP_LBMCB,lbm_fcflush),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.lbmc_bmcwait", MO_SIZEOF_MEMBER(DMP_LBMCB,lbm_bmcwait),
         MO_READ, 0, CL_OFFSETOF(DMP_LBMCB,lbm_bmcwait),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     { 0 }
 };

/*
** Managed objects for the buffer manager:
**     dm0p_lbm_classes:  
**       - Buffer cache statistics for each page size for this server.
**       - One record is returned for each page size.
*/
static MO_CLASS_DEF dm0p_lbm_classes[]=
{
     {0, "exp.dmf.dm0p.lbm_pgsize", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_pgsize),
	 MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_pgsize),
	 MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_fix", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_fix[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_fix[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_hit", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_hit[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_hit[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_check", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_check[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_check[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_refresh", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_refresh[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_refresh[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_reads", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_reads[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_reads[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_unfix", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_unfix[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_unfix[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_dirty", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_dirty[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_dirty[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_force", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_force[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_force[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_writes", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_writes[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_writes[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_iowait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_iowait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_iowait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_mwait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_mwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_mwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_reclaim", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_reclaim[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_reclaim[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_replace", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_replace[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_replace[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_greads", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_greads[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_greads[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_gwrites", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_gwrites[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_gwrites[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_syncwr", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_syncwr[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_syncwr[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_gsyncwr", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_gsyncwr),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_gsyncwr),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_fwait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_fwait),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_fwait),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_fcwait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_fcwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_fcwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_gwait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_gwait),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_gwait),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_active", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_cfa_active),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_cfa_active),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_hwm", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_cfa_hwm),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_cfa_hwm),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_flush", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_cache_flushes),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_cache_flushes),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_threads", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_cfa_cloned),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_cfa_cloned),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_flushed", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_wb_flushed),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_wb_flushed),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_wb_gflushed", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_wb_gflushed),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_wb_gflushed),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     /* Added for MVCC: */
     {0, "exp.dmf.dm0p.lbm_pwait", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_pwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_pwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_requests", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_crreq),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_crreq),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_hits", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_crhit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_crhit),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_allocated", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_craloc),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_craloc),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_materialized", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_crmat),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_crmat),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_unable", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_nocr),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_nocr),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_tossed", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_crtoss),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_crtoss),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_root_tossed", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_roottoss),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_roottoss),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_log_reads", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_lread),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_lread),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_log_readio", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_lreadio),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_lreadio),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_log_undos", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_lundo),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_lundo),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_journal_reads", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_jread),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_jread),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_journal_undos", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_jundo),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_jundo),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     {0, "exp.dmf.dm0p.lbm_cr_journal_hits", MO_SIZEOF_MEMBER(DM0P_BSTAT,bs_jhit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BSTAT,bs_jhit),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_lbm_index
     },
     /* End of MVCC additions */
     { 0 }
 };
/*
** Managed objects for the buffer manager:
**
** dm0p_bm_common_classes:   
**       - Buffer manager configuration 
**       - Buffer manager statistics that reflect the activity of
**         all servers connected to the buffer manager.
**       - One record is returned.
*/
static MO_CLASS_DEF dm0p_bm_common_classes[]=
{
     {0, "exp.dmf.dm0p.bmc_dbcsize", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_dbcsize),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_dbcsize),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_tblcsize", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_tblcsize),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_tblcsize),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_srv_count", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_srv_count),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_srv_count),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_cpcount", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_cpcount),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_cpcount),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_cpindex", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_cpindex),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_cpindex),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_cpcheck", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_cpcheck),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_cpcheck),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_lockreclaim", 
	 MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_lockreclaim),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_lockreclaim),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_fcflush", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_fcflush),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_fcflush),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_bmcwait", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_bmcwait),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_bmcwait),
         MOuintget, MOnoset, (PTR)-1, MOcdata_index
     },
     {0, "exp.dmf.dm0p.bmc_status", MO_SIZEOF_MEMBER(DM0P_BM_COMMON,bmcb_status),
         MO_READ, 0, CL_OFFSETOF(DM0P_BM_COMMON,bmcb_status),
         MOintget, MOnoset, (PTR)-1, MOcdata_index
     },
     { 0 }
 };
/*
** Managed objects for the buffer manager:
**     dm0p_bm_classes:  
**       - Buffer cache configuration for each page size.
**       - Buffer cache statistics that reflect the activity of all
**         servers connected to the buffer cache.
**       - One record is returned for each page size.
*/
static MO_CLASS_DEF dm0p_bm_classes[]=
{
     {0, "exp.dmf.dm0p.bm_pgsize", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_pgsize),
	 MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_pgsize),
	 MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_hshcnt", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_hshcnt),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_hshcnt),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_bufcnt", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_bufcnt),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_bufcnt),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_sbufcnt", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_sbufcnt),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_sbufcnt),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gcnt", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_gcnt),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_gcnt),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gpages", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_gpages),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_gpages),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_flimit", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_flimit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_flimit),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_mlimit", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_mlimit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_mlimit),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_wbstart", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_wbstart),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_wbstart),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_wbend", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_wbend),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_wbend),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_lcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_lcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_glcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_glcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_fcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_fcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gfcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_gfcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_mcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_mcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gmcount", 0,
         MO_READ, 0, 0,
         DM0Pmo_gmcount, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_fix", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_fix[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_fix[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_hit", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_hit[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_hit[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_check", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_check[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_check[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_refresh", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_refresh[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_refresh[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_reads", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_reads[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_reads[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_unfix", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_unfix[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_unfix[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_dirty", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_dirty[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_dirty[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_force", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_force[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_force[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_writes", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_writes[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_writes[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_iowait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_iowait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_iowait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_mwait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_mwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_mwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_reclaim", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_reclaim[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_reclaim[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_replace", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_replace[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_replace[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_greads", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_greads[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_greads[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gwrites", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_gwrites[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_gwrites[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_syncwr", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_syncwr[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_syncwr[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gsyncwr", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_gsyncwr),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_gsyncwr),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_fwait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_fwait),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_fwait),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_fcwait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_fcwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_fcwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_gwait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_gwait),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_gwait),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_status", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_status),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_status),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.bm_clock", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_clock),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_clock),
         MOintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_active", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_cfa_active),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_cfa_active),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_hwm", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_cfa_hwm),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_cfa_hwm),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_flush", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_cache_flushes),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_cache_flushes),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_threads", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_cfa_cloned),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_cfa_cloned),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_flushed", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_wb_flushed),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_wb_flushed),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.wb_gflushed", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_wb_gflushed),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_wb_gflushed),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     /* Added for MVCC: */
     {0, "exp.dmf.dm0p.bm_pwait", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_pwait[BMCB_PTYPES]),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_pwait[BMCB_PTYPES]),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_requests", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_crreq),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_crreq),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_hits", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_crhit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_crhit),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_allocated", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_craloc),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_craloc),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_materialized", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_crmat),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_crmat),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_unable", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_nocr),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_nocr),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_tossed", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_crtoss),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_crtoss),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_root_tossed", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_roottoss),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_roottoss),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_log_reads", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_lread),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_lread),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_log_readio", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_lreadio),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_lreadio),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_log_undos", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_lundo),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_lundo),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_journal_reads", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_jread),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_jread),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_journal_undos", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_jundo),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_jundo),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     {0, "exp.dmf.dm0p.cr_journal_hits", MO_SIZEOF_MEMBER(DM0P_BMCB,bm_stats.bs_jhit),
         MO_READ, 0, CL_OFFSETOF(DM0P_BMCB,bm_stats.bs_jhit),
         MOuintget, MOnoset, (PTR)-1, DM0Pmo_bm_index
     },
     /* End of MVCC additions */
     { 0 }
 };

/*{
** Name: dm0p_mo_init - Initialise the Buffer Manager Managed opjects.
**
** Description:
**
** Inputs:
*	None
**
** Outputs:
**      None	
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-oct-1993 (rmuth)
**	    Created
**      06-mar-1996 (stial01)
**          Variable Page Size project: new classes for new BM objects.
*/
STATUS
dm0p_mo_init( 
    DMP_LBMCB *lbmcb,
    DM0P_BM_COMMON  *bm_common )
{
    STATUS	status;
    i4		i;
    i4          entries;

    entries = ( sizeof( dm0p_lbm_common_classes ) / sizeof( dm0p_lbm_common_classes[0]));
    for ( i = 0; i < entries; i++)
    {
	dm0p_lbm_common_classes[i].cdata = (PTR)lbmcb;
    }

    entries = ( sizeof( dm0p_bm_common_classes ) / sizeof( dm0p_bm_common_classes[0]));
    for ( i = 0; i < entries; i++)
    {
	dm0p_bm_common_classes[i].cdata = (PTR)bm_common;
    }

    status = MOclassdef(MAXI2, dm0p_lbm_common_classes );

    if ( status == OK )
        status = MOclassdef(MAXI2, dm0p_bm_common_classes );

    if ( status == OK)
	status = MOclassdef(MAXI2, dm0p_lbm_classes );

    if ( status == OK)
	status = MOclassdef(MAXI2, dm0p_bm_classes );

    return( status );
}


/*{
** Name: DM0Pmo_bm_index - MO index method for BM traversing 
**
** Description:
**
**
** History:
**      06-mar-1996 (stial01)
**          Variable Page Size project: Created for new BM objects.
*/
static STATUS	DM0Pmo_bm_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    DMP_LBMCB       *lbm = dmf_svcb->svcb_lbmcb_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    DM0P_BMCB       *bm;

    CVal(instance, &ix);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > DM_MAX_CACHE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    bm = lbm->lbm_bmcb[ix - 1];
	    if (!bm || !(bm->bm_status & BM_ALLOCATED))
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)bm;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= DM_MAX_CACHE)
	    {
		bm = lbm->lbm_bmcb[ix - 1];
		if (bm && (bm->bm_status & BM_ALLOCATED))
		{
		    *instdata = (PTR)bm;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > DM_MAX_CACHE)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}


/*{
** Name: DM0Pmo_lbm_index - MO index method for LBM traversing 
**
** Description:
**
**
** History:
**      06-mar-1996 (stial01)
**          Variable Page Size project: Created for new BM objects.
*/
static STATUS	DM0Pmo_lbm_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    DMP_LBMCB       *lbm = dmf_svcb->svcb_lbmcb_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    DM0P_BMCB       *bm;

    CVal(instance, &ix);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > DM_MAX_CACHE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    bm = lbm->lbm_bmcb[ix - 1];
	    if (!bm || !(bm->bm_status & BM_ALLOCATED))
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)&lbm->lbm_stats[ix - 1];
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= DM_MAX_CACHE)
	    {
		bm = lbm->lbm_bmcb[ix - 1];
		if (bm && (bm->bm_status & BM_ALLOCATED))
		{
		    *instdata = (PTR)&lbm->lbm_stats[ix - 1];
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > DM_MAX_CACHE)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

/*{
** Name: DM0Pmo_fcount - MO get method for free page count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_fcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    count = 0;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_QUEUE(bm->bm_fpq, count);

    return (MOlongout( MO_VALUE_TRUNCATED,
			count, luserbuf, userbuf));
}

/*{
** Name: DM0Pmo_mcount - MO get method for modified page count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_mcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    count = 0;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_QUEUE(bm->bm_mpq, count);

    return (MOlongout( MO_VALUE_TRUNCATED,
			count, luserbuf, userbuf));
}

/*{
** Name: DM0Pmo_lcount - MO get method for fixed page count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_lcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    lcount = 0;
    i4		    fcount, mcount;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_ALL_QUEUES(fcount, mcount, lcount);

    return (MOlongout( MO_VALUE_TRUNCATED,
			lcount, luserbuf, userbuf));
}

/*{
** Name: DM0Pmo_gfcount - MO get method for free group count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_gfcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    count = 0;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_QUEUE(bm->bm_gfq, count);

    return (MOlongout( MO_VALUE_TRUNCATED,
			count, luserbuf, userbuf));
}

/*{
** Name: DM0Pmo_gmcount - MO get method for modified group count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_gmcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    count = 0;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_QUEUE(bm->bm_gmq, count);

    return (MOlongout( MO_VALUE_TRUNCATED,
			count, luserbuf, userbuf));
}

/*{
** Name: DM0Pmo_glcount - MO get method for fixed group count
**
** Description:
**
**
** History:
**	10-Jan-2000 (jenjo02)
**	    Written.
*/
static STATUS	DM0Pmo_glcount(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DM0P_BMCB       *bm = (DM0P_BMCB*)object;
    i4		    lcount = 0;
    i4		    fcount, mcount;

    if ( bm && bm->bm_status & BM_ALLOCATED )
	DM0P_COUNT_ALL_GQUEUES(fcount, mcount, lcount);

    return (MOlongout( MO_VALUE_TRUNCATED,
			lcount, luserbuf, userbuf));
}
