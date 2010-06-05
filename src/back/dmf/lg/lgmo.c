/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <ulf.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/*
** Name: LGMO.C		- LG Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for LG. Managed objects appear in the MIB, and allow
**	monitoring, analysis, and control of LG through the MO interface.
**
** History:
**	Fall, 1992 (bryanp)
**	    Written for the new logging system.
**	10-November-1992 (rmuth)
**	    Remove lgd_n_servers.
**	13-nov-1992 (bryanp)
**	    Display log addresses in "triple" format
**	19-nov-1992 (bryanp)
**	    Added database name & owner, and display of tran ID in hexascii.
**	17-dec-1992 (bryanp)
**	    ldb_index must be static.
**	18-jan-1993 (bryanp)
**	    Adding MO objects for XA_RECOVER support.
**	15-mar-1993 (rogerk)
**	    Updated lgd_status and ldb_status values.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I:
**	    Renamed stucture members of LG_LA to new names. This means
**		replacing lga_high with la_sequence, and lga_low with la_offset.
**	    Substantial additions to bring MO support up to date, including
**		most importantly the addition of the "LFB" table.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Added new lfb_archive_window fields and removed old lfb_j_first,
**	    lfb_j_last, lfb_d_first and lfb_d_last fields.  Also changed
**	    ldb_[j/d]_[first/last]_cp fields to ldb_[j/d]_[first/last]_la.
**	23-aug-1993 (bryanp)
**	    Add wait reason LXB_LOGSPACE_WAIT.
**      23-sep-1993 (iyer)
**          Change the current version IICXformat_xa_xid to format the
**          extended XA XID. This would imply that the function would
**          accept a pointer to the extended structure DB_XA_EXTD_DIS_TRAN_ID
**          instead of DB_XA_DIS_TRAN_ID and format the additional members
**          branch_flag and branch_seqnum.
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_stall_limit, lgd_dmu_cnt, lxb_dmu_cnt fields.
**	31-jan-1994 (mikem)
**	    bug #57045
**	    Changed MO interface to lg parameters to allow updating of the
**	    lgd_gcmt_threshold and lgd_gcmt_numticks fields in the LGD.  This
**	    allows runtime tuning of the fast commit algorithm.  Also fixed
**	    some fields which had too long exported names.
**      10-Mar-1994 (daveb) 60514
**          Use right output function for SCBs and SIDs, MOptrget or MOpvget.
**      13-Apr-1994 (daveb) 62240
**          Add % logful object.
**	23-may-1994 (bryanp) B62546
**	    Correct lpb_status formatting -- it was using the pr_type values.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named in calls to LGMUTEX functions.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    New mutexes to augment lone lgd_mutex.
**      08-Dec-1997 (hanch04)
**          Added new fields, lfb_hdr_lgh_begin_blk, lfb_hdr_lgh_end_blk,
**	    lfb_hdr_lgh_cp_blk, lfb_hdr_lgh_last_lsn,
**	    lfb_hdr_lgh_last_lsn_lsn_high, lfb_hdr_lgh_last_lsn_lsn_low,
**	    la_block.
**	    Remove lfb_last_lsn, lfb_last_lsn_high, lfb_last_lsn_low
**          for support of logs > 2 gig
**	17-Dec-1997 (jenjo02)
**	    Number of logwriter threads is now carried in lgd_lwlxb.lwq_count
**	    instead of lgd_n_logwriters.
**	26-Jan-1998 (jenjo02)
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb, removed lpb_gcmt_asleep.
**	24-Aug-1998 (jenjo02)
**	    lxb_tran_id now embedded in LXH structure within LXB.
**	05-Oct-1998 (thoda04)
**	    Process XID data bytes in groups of four, high to low, bytes for all platforms.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
** 	24-Aug-1999 (jenjo02)
**	    Removed mutexing of lgd_lpbb, lgd_lxbb, lgd_ldbb, none
**	    of which were really needed.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED/HANDLE log transactions
**      02-Mar-2000 (fanra01)
**          Bug 100700
**          Add the lpb_gcmt_sid and lpb_gcmt_asleep for backward compatibilty.
**          The lpb_gcmt_sid returns the same value as lpb_gcmt_lxb.
**          lpb_gcmt_asleep will always return 0.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-oct-2002 (jenjo02)
**          LGmo_lxb_is_xa_dis_tran_id - only format if LXB_SHARED_HANDLE,
**          All xa transactions should be shared, (set session with xa_xid)
**	07-Jul-2003 (devjo01)
**	    Use MOsidget instead of MOptrget when accessing session IDs.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      05-may-2005 (stial01)
**          LGmo_ldb_status_get new flag for LDB_CSP_RECOVER
**      13-jun-2005 (stial01)
**          LGmo_ldb_status_get removed flag for LDB_CSP_RECOVER
**	15-Mar-2006 (jenjo02)
**	    lgd_stat.wait cleanup.
**	06-Mar-2008 (smeke01) b120058 b119880
**	    Correct typo in lgd_stat.wait cleanup.
**      03-dec-2008 (stial01)
**          Fix size of format buffer used for database name.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** The Index methods are subroutines which know how to advance through the
** virtual tables from one row to the next. Currently, we do not use an LFB
** index method, since the only LFB we export is the local LFB. If we ever
** change this to export multiple LFBs (during cluster recovery), we'll have
** to add an LFB index method.
*/
static STATUS	LGmo_lpb_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	LGmo_lxb_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	LGmo_ldb_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

/*
** Many columns are simple primitive types and are returned just as integers.
** Other columns have special formatting requirements; these columns need "get"
** methods to perform the column-specific formatting:
*/
static	MO_GET_METHOD LGmo_lgd_status_get;
static	MO_GET_METHOD	LGmo_lfb_status_get;
static	MO_GET_METHOD	LGmo_lfb_lgh_status_get;
static	MO_GET_METHOD	LGmo_lfb_lgh_tran_get;
static	MO_GET_METHOD	LGmo_lsn_get;
static	MO_GET_METHOD	LGmo_lfb_active_log_get;
static	MO_GET_METHOD	LGmo_lfb_lgh_percentage_logfull_get;
static	MO_GET_METHOD	LGmo_lxb_wait_reason_get;
static	MO_GET_METHOD	LGmo_lxb_status_get;
static	MO_GET_METHOD	LGmo_lpb_status_get;
static	MO_GET_METHOD	LGmo_ldb_status_get;
static	MO_GET_METHOD	LGmo_lgla_get;
static	MO_GET_METHOD	LGmo_lxb_tran_get;
static	MO_GET_METHOD	LGmo_lxb_dbname_get;
static	MO_GET_METHOD	LGmo_lxb_dbowner_get;
static	MO_GET_METHOD	LGmo_lxb_dbid_get;
static	MO_GET_METHOD	LGmo_lxb_prid_get;
static	MO_GET_METHOD	LGmo_lxb_dis_tran_id_hexdump;
static  VOID		IICXformat_xa_xid(DB_XA_EXTD_DIS_TRAN_ID 
                                                  *xa_extd_xid_p,
					    char  *text_buffer);
static	MO_GET_METHOD	LGmo_lxb_is_prepared;
static	MO_GET_METHOD	LGmo_lxb_is_xa_dis_tran_id;

static MO_CLASS_DEF LGmo_lgd_classes[] =
{
    {0, "exp.dmf.lg.lgd_status",
	0, MO_READ, 0,
	0, LGmo_lgd_status_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_status_num",
	MO_SIZEOF_MEMBER(LGD, lgd_status), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_status), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpb_inuse",
	MO_SIZEOF_MEMBER(LGD, lgd_lpb_inuse), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpb_inuse), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lxb_inuse",
	MO_SIZEOF_MEMBER(LGD, lgd_lxb_inuse), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lxb_inuse), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_ldb_inuse",
	MO_SIZEOF_MEMBER(LGD, lgd_ldb_inuse), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_ldb_inuse), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpd_inuse",
	MO_SIZEOF_MEMBER(LGD, lgd_lpd_inuse), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpd_inuse), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_protect_count",
	MO_SIZEOF_MEMBER(LGD, lgd_protect_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_protect_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_n_logwriters",
	MO_SIZEOF_MEMBER(LGD, lgd_lwlxb.lwq_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lwlxb.lwq_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_no_bcp",
	MO_SIZEOF_MEMBER(LGD, lgd_no_bcp), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_no_bcp), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpbb_count",
	MO_SIZEOF_MEMBER(LGD, lgd_lpbb_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpbb_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpbb_size",
	MO_SIZEOF_MEMBER(LGD, lgd_lpbb_size), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpbb_size), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lxbb_count",
	MO_SIZEOF_MEMBER(LGD, lgd_lxbb_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lxbb_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lxbb_size",
	MO_SIZEOF_MEMBER(LGD, lgd_lxbb_size), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lxbb_size), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpdb_count",
	MO_SIZEOF_MEMBER(LGD, lgd_lpdb_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpdb_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lpdb_size",
	MO_SIZEOF_MEMBER(LGD, lgd_lpdb_size), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lpdb_size), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lfbb_count",
	MO_SIZEOF_MEMBER(LGD, lgd_lfbb_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lfbb_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_lfbb_size",
	MO_SIZEOF_MEMBER(LGD, lgd_lfbb_size), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_lfbb_size), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_ldbb_count",
	MO_SIZEOF_MEMBER(LGD, lgd_ldbb_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_ldbb_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_ldbb_size",
	MO_SIZEOF_MEMBER(LGD, lgd_ldbb_size), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_ldbb_size), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_cnodeid",
	MO_SIZEOF_MEMBER(LGD, lgd_cnodeid), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_cnodeid), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_csp_pid",
	MO_SIZEOF_MEMBER(LGD, lgd_csp_pid), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_csp_pid), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.add",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.add), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.add), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.remove",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.remove), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.remove), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.begin",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.begin), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.begin), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.end",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.end), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.end), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.write",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.write), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.write), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.split",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.split), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.split), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.force",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.force), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.force), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.readio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.readio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.readio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.writeio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.writeio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.writeio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.wait",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.wait[LG_WAIT_ALL]), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.wait[LG_WAIT_ALL]), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.group_force",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.group_force), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.group_force), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.group_count",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.group_count), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.group_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.inconsist_db",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.inconsist_db), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.inconsist_db), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.pgyback_check",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.pgyback_check), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.pgyback_check), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.pgyback_write",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.pgyback_write), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.pgyback_write), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.kbytes",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.kbytes), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.kbytes), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.free_wait",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.wait[LG_WAIT_FREEBUF]), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.wait[LG_WAIT_FREEBUF]), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.stall_wait",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.wait[LG_WAIT_LOGFULL]), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.wait[LG_WAIT_LOGFULL]), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.bcp_stall_wait",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.wait[LG_WAIT_BCP]), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.wait[LG_WAIT_BCP]), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.log_readio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.log_readio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.log_readio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.dual_readio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.dual_readio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.dual_readio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.log_writeio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.log_writeio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.log_writeio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_stat.dual_writeio",
	MO_SIZEOF_MEMBER(LGD, lgd_stat.dual_writeio), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_stat.dual_writeio), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_cpstall",
	MO_SIZEOF_MEMBER(LGD, lgd_cpstall), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_cpstall), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_check_stall",
	MO_SIZEOF_MEMBER(LGD, lgd_check_stall), MO_READ, 0,
	CL_OFFSETOF(LGD, lgd_check_stall), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_gcmt_threshold",
	MO_SIZEOF_MEMBER(LGD, lgd_gcmt_threshold), MO_READ | MO_WRITE, 0,
	CL_OFFSETOF(LGD, lgd_gcmt_threshold),
	MOintget, MOintset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lgd_gcmt_numticks",
	MO_SIZEOF_MEMBER(LGD, lgd_gcmt_numticks), MO_READ | MO_WRITE, 0,
	CL_OFFSETOF(LGD, lgd_gcmt_numticks),
	MOintget, MOintset, (PTR)0, MOcdata_index
    },
    {0}
};

static MO_CLASS_DEF LGmo_lfb_classes[] =
{
    {0, "exp.dmf.lg.lfb_status",
	0, MO_READ, 0,
	0, LGmo_lfb_status_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_status_num",
	MO_SIZEOF_MEMBER(LFB, lfb_status), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_status), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_version",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_version), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_version),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_checksum",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_checksum), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_checksum),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_size",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_size), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_count",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_count), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_count),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_status",
	0, MO_READ, 0,
	0, LGmo_lfb_lgh_status_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_status_num",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_status), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_status),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_l_logfull",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_l_logfull), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_l_logfull),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_l_abort",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_l_abort), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_l_abort),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_l_cp",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_l_cp), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_l_cp),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_cpcnt",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_cpcnt), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_cpcnt),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_tran_id",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_tran_id), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_tran_id),
	LGmo_lfb_lgh_tran_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_tran_high",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_tran_id.db_high_tran), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_tran_id.db_high_tran),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_tran_low",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_tran_id.db_low_tran), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_tran_id.db_low_tran),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_begin",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_begin), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_begin), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_begin_seq",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_begin.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_begin.la_sequence),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_begin_blk",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_begin.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_begin.la_block),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_begin_off",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_begin.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_begin.la_offset),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_end",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_end), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_end), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_end_seq",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_end.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_end.la_sequence),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_end_blk",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_end.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_end.la_block),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_end_off",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_end.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_end.la_offset),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_cp",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_cp), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_cp), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_cp_seq",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_cp.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_cp.la_sequence),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_cp_blk",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_cp.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_cp.la_block),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_cp_off",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_cp.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_cp.la_offset),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_last_lsn",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_last_lsn), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_last_lsn), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_last_lsn_lsn_high",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_last_lsn.lsn_high), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_last_lsn.lsn_high),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_last_lsn_lsn_low",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_last_lsn.lsn_low), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_last_lsn.lsn_low),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_active_logs",
	MO_SIZEOF_MEMBER(LFB, lfb_header.lgh_active_logs), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_header.lgh_active_logs),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_hdr_lgh_percentage_logfull",
	 0, MO_READ, 0,
	 0 ,
	 LGmo_lfb_lgh_percentage_logfull_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lga",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lga), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lga), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lga.la_sequence",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lga.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lga.la_sequence), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lga.la_block",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lga.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lga.la_block), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lga.la_offset",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lga.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lga.la_offset), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lsn",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lsn), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lsn), 
	LGmo_lsn_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lsn_high",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lsn.lsn_high), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lsn.lsn_high), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_forced_lsn_low",
	MO_SIZEOF_MEMBER(LFB, lfb_forced_lsn.lsn_low), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_forced_lsn.lsn_low), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_start",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_start), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_start), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_start.la_sequence",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_start.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_start.la_sequence), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_start.la_block",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_start.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_start.la_block), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_start.la_offset",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_start.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_start.la_offset), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_end",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_end), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_end), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_end.la_sequence",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_end.la_sequence), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_end.la_sequence), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_end.la_block",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_end.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_end.la_block), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_end.la_offset",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_end.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_end.la_offset), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_prevcp",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_prevcp), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_prevcp), 
	LGmo_lgla_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_prevcp.la_sequence",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_prevcp.la_sequence),MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_prevcp.la_sequence), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_prevcp.la_block",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_prevcp.la_block), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_prevcp.la_block), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_archive_prevcp.la_offset",
	MO_SIZEOF_MEMBER(LFB, lfb_archive_window_prevcp.la_offset), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_archive_window_prevcp.la_offset), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_active_log",
	0, MO_READ, 0,
	0, LGmo_lfb_active_log_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_active_log_num",
	MO_SIZEOF_MEMBER(LFB, lfb_active_log), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_active_log), 
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_buf_cnt",
	MO_SIZEOF_MEMBER(LFB, lfb_buf_cnt), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_buf_cnt), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_reserved_space",
	MO_SIZEOF_MEMBER(LFB, lfb_reserved_space), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_reserved_space),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_channel_blk",
	MO_SIZEOF_MEMBER(LFB, lfb_channel_blk), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_channel_blk),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_dual_channel_blk",
	MO_SIZEOF_MEMBER(LFB, lfb_dual_channel_blk), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_dual_channel_blk),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_split",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.split), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.split),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_write",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.write), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.write),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_force",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.force), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.force),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_wait",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.wait), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.wait),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_end",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.end), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.end),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_writeio",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.writeio), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.writeio),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_kbytes",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.kbytes), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.kbytes),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_log_readio",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.log_readio), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.log_readio),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_dual_readio",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.dual_readio), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.dual_readio),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_log_writeio",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.log_writeio), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.log_writeio),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lg.lfb_stat_dual_writeio",
	MO_SIZEOF_MEMBER(LFB, lfb_stat.dual_writeio), MO_READ, 0,
	CL_OFFSETOF(LFB, lfb_stat.dual_writeio),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0}
};

static char lpb_index[] = "exp.dmf.lg.lpb_id.id_id";
static MO_CLASS_DEF LGmo_lpb_classes[] =
{
    {0, "exp.dmf.lg.lpb_id.id_instance",
	MO_SIZEOF_MEMBER(LPB, lpb_id.id_instance), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_id.id_instance),
	MOuintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_id.id_id",
	MO_SIZEOF_MEMBER(LPB, lpb_id.id_id), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_id.id_id),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_status",
	0, MO_READ, lpb_index,
	0, LGmo_lpb_status_get, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_status_num",
	MO_SIZEOF_MEMBER(LPB, lpb_status), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_status),
	MOuintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_lpd_count",
	MO_SIZEOF_MEMBER(LPB, lpb_lpd_count), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_lpd_count),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_pid",
	MO_SIZEOF_MEMBER(LPB, lpb_pid), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_pid),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_cond",
	MO_SIZEOF_MEMBER(LPB, lpb_cond), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_cond),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_bufmgr_id",
	MO_SIZEOF_MEMBER(LPB, lpb_bufmgr_id), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_bufmgr_id),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_force_abort_sid",
	MO_SIZEOF_MEMBER(LPB, lpb_force_abort_cpid.sid), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_force_abort_cpid.sid),
	MOsidget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_gcmt_sid",
	MO_SIZEOF_MEMBER(LPB, lpb_gcmt_lxb), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_gcmt_lxb),
	MOsidget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_gcmt_lxb",
	MO_SIZEOF_MEMBER(LPB, lpb_gcmt_lxb), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_gcmt_lxb),
	MOptrget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_gcmt_asleep",
	sizeof(i4), MO_READ, lpb_index,
	0,
	MOzeroget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.readio",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.readio), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.readio),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.write",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.write), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.write),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.force",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.force), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.force),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.wait",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.wait), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.wait),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.begin",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.begin), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.begin),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0, "exp.dmf.lg.lpb_stat.end",
	MO_SIZEOF_MEMBER(LPB, lpb_stat.end), MO_READ, lpb_index,
	CL_OFFSETOF(LPB, lpb_stat.end),
	MOintget, MOnoset, (PTR)0, LGmo_lpb_index
    },
    {0}
};

static STATUS	LGmo_lpb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LGD		    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *lpbb_table;
    LPB		    *lpb;

    CVal(instance, &ix);

    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lgd->lgd_lpbb_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[ix]);
	    if (lpb->lpb_type != LPB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)lpb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lgd->lgd_lpbb_count)
	    {
		lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[ix]);
		if (lpb->lpb_type == LPB_TYPE)
		{
		    *instdata = (PTR)lpb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lgd->lgd_lpbb_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

static char lxb_index[] = "exp.dmf.lg.lxb_id.id_id";
static MO_CLASS_DEF LGmo_lxb_classes[] =
{
    {0, "exp.dmf.lg.lxb_id.id_instance",
	MO_SIZEOF_MEMBER(LXB, lxb_id.id_instance), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_id.id_instance),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_id.id_id",
	MO_SIZEOF_MEMBER(LXB, lxb_id.id_id), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_id.id_id),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_status",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_status_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_status_num",
	MO_SIZEOF_MEMBER(LXB, lxb_status), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_status),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_db_name",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_dbname_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_db_owner",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_dbowner_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_db_id_id",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_dbid_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_pr_id_id",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_prid_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_wait_reason",
	0, MO_READ, lxb_index,
	0, LGmo_lxb_wait_reason_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_wait_reason_num",
	MO_SIZEOF_MEMBER(LXB, lxb_wait_reason), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_wait_reason),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_sequence",
	MO_SIZEOF_MEMBER(LXB, lxb_sequence), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_sequence),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_first_lga",
	MO_SIZEOF_MEMBER(LXB, lxb_first_lga), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_first_lga),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_first_lga.la_sequence",
	MO_SIZEOF_MEMBER(LXB, lxb_first_lga.la_sequence), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_first_lga.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_first_lga.la_block",
	MO_SIZEOF_MEMBER(LXB, lxb_first_lga.la_block), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_first_lga.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_first_lga.la_offset",
	MO_SIZEOF_MEMBER(LXB, lxb_first_lga.la_offset), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_first_lga.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lga",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lga), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lga),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lga.la_sequence",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lga.la_sequence), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lga.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lga.la_block",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lga.la_block), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lga.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lga.la_offset",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lga.la_offset), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lga.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lsn",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lsn), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lsn), 
	LGmo_lsn_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lsn_high",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lsn.lsn_high), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lsn.lsn_high), 
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_last_lsn_low",
	MO_SIZEOF_MEMBER(LXB, lxb_last_lsn.lsn_low), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_last_lsn.lsn_low),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_cp_lga",
	MO_SIZEOF_MEMBER(LXB, lxb_cp_lga), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cp_lga),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_cp_lga.la_sequence",
	MO_SIZEOF_MEMBER(LXB, lxb_cp_lga.la_sequence), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cp_lga.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_cp_lga.la_block",
	MO_SIZEOF_MEMBER(LXB, lxb_cp_lga.la_block), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cp_lga.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_cp_lga.la_offset",
	MO_SIZEOF_MEMBER(LXB, lxb_cp_lga.la_offset), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cp_lga.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_tran_id",
	MO_SIZEOF_MEMBER(LXB, lxb_lxh.lxh_tran_id), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_lxh.lxh_tran_id),
	LGmo_lxb_tran_get, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_tran_id.db_high_tran",
	MO_SIZEOF_MEMBER(LXB, lxb_lxh.lxh_tran_id.db_high_tran), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_lxh.lxh_tran_id.db_high_tran),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_tran_id.db_low_tran",
	MO_SIZEOF_MEMBER(LXB, lxb_lxh.lxh_tran_id.db_low_tran), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_lxh.lxh_tran_id.db_low_tran),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_dis_tran_id_hexdump",
	MO_SIZEOF_MEMBER(LXB, lxb_dis_tran_id), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_dis_tran_id),
	LGmo_lxb_dis_tran_id_hexdump, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_pid",
	MO_SIZEOF_MEMBER(LXB, lxb_cpid.pid), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cpid.pid),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_sid",
	MO_SIZEOF_MEMBER(LXB, lxb_cpid.sid), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_cpid.sid),
	MOsidget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_reserved_space",
	MO_SIZEOF_MEMBER(LXB, lxb_reserved_space), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_reserved_space),
	MOuintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_user_name",
	MO_SIZEOF_MEMBER(LXB, lxb_user_name), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_user_name[0]),
	MOstrget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_is_prepared",
	MO_SIZEOF_MEMBER(LXB, lxb_status), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_status),
	LGmo_lxb_is_prepared, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_is_xa_dis_tran_id",
	MO_SIZEOF_MEMBER(LXB, lxb_dis_tran_id), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_dis_tran_id),
	LGmo_lxb_is_xa_dis_tran_id, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_stat.split",
	MO_SIZEOF_MEMBER(LXB, lxb_stat.split), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_stat.split),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_stat.write",
	MO_SIZEOF_MEMBER(LXB, lxb_stat.write), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_stat.write),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_stat.force",
	MO_SIZEOF_MEMBER(LXB, lxb_stat.force), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_stat.force),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0, "exp.dmf.lg.lxb_stat.wait",
	MO_SIZEOF_MEMBER(LXB, lxb_stat.wait), MO_READ, lxb_index,
	CL_OFFSETOF(LXB, lxb_stat.wait),
	MOintget, MOnoset, (PTR)0, LGmo_lxb_index
    },
    {0}
};

static STATUS	LGmo_lxb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LGD		    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *lxbb_table;
    LXB		    *lxb;

    CVal(instance, &ix);

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lgd->lgd_lxbb_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[ix]);
	    if (lxb->lxb_type != LXB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)lxb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lgd->lgd_lxbb_count)
	    {
		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[ix]);
		if (lxb->lxb_type == LXB_TYPE)
		{
		    *instdata = (PTR)lxb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lgd->lgd_lxbb_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

static char ldb_index[] = "exp.dmf.lg.ldb_id.id_id";
static MO_CLASS_DEF LGmo_ldb_classes[] =
{
    {0, "exp.dmf.lg.ldb_id.id_instance",
	MO_SIZEOF_MEMBER(LDB, ldb_id.id_instance), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_id.id_instance),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_id.id_id",
	MO_SIZEOF_MEMBER(LDB, ldb_id.id_id), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_id.id_id),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_status",
	0, MO_READ, ldb_index,
	0, LGmo_ldb_status_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_status_num",
	MO_SIZEOF_MEMBER(LDB, ldb_status), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_status),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_lpd_count",
	MO_SIZEOF_MEMBER(LDB, ldb_lpd_count), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_lpd_count),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_lxb_count",
	MO_SIZEOF_MEMBER(LDB, ldb_lxb_count), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_lxb_count),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_lxbo_count",
	MO_SIZEOF_MEMBER(LDB, ldb_lxbo_count), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_lxbo_count),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_database_id",
	MO_SIZEOF_MEMBER(LDB, ldb_database_id), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_database_id),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_l_buffer",
	MO_SIZEOF_MEMBER(LDB, ldb_l_buffer), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_l_buffer),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_buffer",
	MO_SIZEOF_MEMBER(LDB, ldb_buffer), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_buffer[0]),
	MOstrget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_db_name",
	DB_DB_MAXNAME, MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_buffer[0]),
	MOstrget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_db_owner",  /* owner is after dbname */
	DB_OWN_MAXNAME, MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_buffer[DB_DB_MAXNAME]),
	MOstrget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_first_la",
	MO_SIZEOF_MEMBER(LDB, ldb_j_first_la), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_first_la),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_first_la.la_sequence",
	MO_SIZEOF_MEMBER(LDB, ldb_j_first_la.la_sequence), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_first_la.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_first_la.la_block",
	MO_SIZEOF_MEMBER(LDB, ldb_j_first_la.la_block), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_first_la.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_first_la.la_offset",
	MO_SIZEOF_MEMBER(LDB, ldb_j_first_la.la_offset), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_first_la.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_last_la",
	MO_SIZEOF_MEMBER(LDB, ldb_j_last_la), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_last_la),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_last_la.la_sequence",
	MO_SIZEOF_MEMBER(LDB, ldb_j_last_la.la_sequence), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_last_la.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_last_la.la_block",
	MO_SIZEOF_MEMBER(LDB, ldb_j_last_la.la_block), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_last_la.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_j_last_la.la_offset",
	MO_SIZEOF_MEMBER(LDB, ldb_j_last_la.la_offset), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_j_last_la.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_first_la",
	MO_SIZEOF_MEMBER(LDB, ldb_d_first_la), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_first_la),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_first_la.la_sequence",
	MO_SIZEOF_MEMBER(LDB, ldb_d_first_la.la_sequence), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_first_la.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_first_la.la_block",
	MO_SIZEOF_MEMBER(LDB, ldb_d_first_la.la_block), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_first_la.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_first_la.la_offset",
	MO_SIZEOF_MEMBER(LDB, ldb_d_first_la.la_offset), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_first_la.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_last_la",
	MO_SIZEOF_MEMBER(LDB, ldb_d_last_la), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_last_la),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_last_la.la_sequence",
	MO_SIZEOF_MEMBER(LDB, ldb_d_last_la.la_sequence), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_last_la.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_last_la.la_block",
	MO_SIZEOF_MEMBER(LDB, ldb_d_last_la.la_block), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_last_la.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_d_last_la.la_offset",
	MO_SIZEOF_MEMBER(LDB, ldb_d_last_la.la_offset), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_d_last_la.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sbackup",
	MO_SIZEOF_MEMBER(LDB, ldb_sbackup), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sbackup),
	LGmo_lgla_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sbackup.la_sequence",
	MO_SIZEOF_MEMBER(LDB, ldb_sbackup.la_sequence), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sbackup.la_sequence),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sbackup.la_block",
	MO_SIZEOF_MEMBER(LDB, ldb_sbackup.la_block), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sbackup.la_block),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sbackup.la_offset",
	MO_SIZEOF_MEMBER(LDB, ldb_sbackup.la_offset), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sbackup.la_offset),
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sback_lsn",
	MO_SIZEOF_MEMBER(LDB, ldb_sback_lsn), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sback_lsn), 
	LGmo_lsn_get, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sback_lsn_high",
	MO_SIZEOF_MEMBER(LDB, ldb_sback_lsn.lsn_high), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sback_lsn.lsn_high), 
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_sback_lsn_low",
	MO_SIZEOF_MEMBER(LDB, ldb_sback_lsn.lsn_low), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_sback_lsn.lsn_low), 
	MOuintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.read",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.read), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.read),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.write",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.write), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.write),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.begin",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.begin), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.begin),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.end",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.end), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.end),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.force",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.force), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.force),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0, "exp.dmf.lg.ldb_stat.wait",
	MO_SIZEOF_MEMBER(LDB, ldb_stat.wait), MO_READ, ldb_index,
	CL_OFFSETOF(LDB, ldb_stat.wait),
	MOintget, MOnoset, (PTR)0, LGmo_ldb_index
    },
    {0}
};

static STATUS	LGmo_ldb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LGD		    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *ldbb_table;
    LDB		    *ldb;

    CVal(instance, &ix);

    ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lgd->lgd_ldbb_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[ix]);
	    if (ldb->ldb_type != LDB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)ldb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lgd->lgd_ldbb_count)
	    {
		ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[ix]);
		if (ldb->ldb_type == LDB_TYPE)
		{
		    *instdata = (PTR)ldb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lgd->lgd_ldbb_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

/*
** Name: LGmo_lgd_status_get	    - MO Get function for the lgd_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lgd_status field.
**
** Inputs:
**	offset			    - offset into the LGD, ignored.
**	objsize			    - size of the lgd_status field, ignored
**	object			    - the LGD address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	30-oct-1992 (bryanp)
**	    Created
**	15-mar-1993 (rogerk)
**	    Updated new lgd_status values - got rid of EBACKUP, FBACKUP,
**	    and online purge states..
**	02-May-2003 (jenjo02)
**	    Corrected lgd_status bit interpretations.
*/
static STATUS
LGmo_lgd_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LGD		*lgd = (LGD *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "ONLINE,CPNEEDED,LOGFULL,FORCE_ABORT,RECOVER,\
ARCHIVE,ACP_SHUTDOWN,IMM_SHUTDOWN,START_ARCHIVER,PURGEDB,\
OPEN_DB,CLOSE_DB,START_SHUTDOWN,BCPDONE,CPFLUSH,ECP,ECPDONE,\
CPWAKEUP,BCPSTALL,CKP_SBACKUP,?,?,MAN_ABORT,MAN_COMMIT,RCP_RECOVER,\
JSWITCH,JSWITCHDONE,DUAL_LOGGING,DISABLE_DUAL_LOGGING,EMER_SHUTDOWN,\
?,CLUSTER", lgd->lgd_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lpb_status_get	    - MO Get function for the lpb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lpb_status field.
**
** Inputs:
**	offset			    - offset into the LPB, ignored.
**	objsize			    - size of the lpb_status field, ignored
**	object			    - the LPB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	30-oct-1992 (bryanp)
**	    Created
**	23-may-1994 (bryanp) B62546
**	    Correct the lpb_status formatting -- the %v format string was
**		erroneously using the pr_type display values, rather than the
**		lpb_status display values.
*/
static STATUS
LGmo_lpb_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LPB		*lpb = (LPB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "ACTIVE,MASTER,ARCHIVER,FAST_COMMIT,RUNAWAY,NON_FAST_COMMIT,\
CKPDB,VOID,SHARED_BUFMGR,IDLE,DEAD,DYING,FOREIGN_RUNDOWN,CP_AGENT",
	    lpb->lpb_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_ldb_status_get	    - MO Get function for the ldb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the ldb_status field.
**
** Inputs:
**	offset			    - offset into the LDB, ignored.
**	objsize			    - size of the ldb_status field, ignored
**	object			    - the LDB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	30-oct-1992 (bryanp)
**	    Created
**	15-mar-1993 (rogerk)
**	    Fixed with new ldb status values.  Removed EBACKUP, ONLINE_PURGE
**	    states.
*/
static STATUS
LGmo_ldb_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LDB		*ldb = (LDB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "ACTIVE,JOURNAL,INVALID,NOTDB,PURGE,OPENDB_PEND,CLOSEDB_PEND,\
RECOVER,FAST_COMMIT,PRETEND_CONSISTENT,CLOSEDB_DONE,OPN_WAIT,STALL,BACKUP,\
CKPDB_PEND,,FBACKUP,CKPERROR",
	    ldb->ldb_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_status_get	    - MO Get function for the lxb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lxb_status field.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_status field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	30-oct-1992 (bryanp)
**	    Created
**	15-Dec-1999 (jenjo02)
**	    Updated with latest status bit interpretations.
*/
static STATUS
LGmo_lxb_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "INACTIVE,ACTIVE,CKPDB_PEND,PROTECT,JOURNAL,READONLY,NOABORT,\
RECOVER,FORCE_ABORT,SESSION_ABORT,SERVER_ABORT,PASS_ABORT,DBINCONSISTENT,WAIT,\
DISTRIBUTED,WILLING_COMMIT,RE-ASSOC,RESUME,MAN_ABORT,MAN_COMMIT,LOGWRITER,\
SAVEABORT,NORESERVE,ORPHAN,WAIT_LBB,SHARED,HANDLE,ET",
	    lxb->lxb_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lgla_get	    - MO Get function for log address fields
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of any log address field. The log address is returned
**	in "triple" format; that is, in the format
**
**	    <sequence,block,offset>
**
**	where sequence is the log address sequence counter, block is the log
**	page block number, and offset is the byte offset within the page.
**
** Inputs:
**	offset			    - offset into the block of the log address
**	objsize			    - size of the log address field, ignored
**	object			    - the block address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice log address string
**
** Returns:
**	STATUS
**
** History:
**	13-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lgla_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_LA	*lga = (LG_LA *)((char *)object + offset);
    char	format_buf [80];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "<%d,%d,%d>",
		lga->la_sequence, lga->la_block, lga->la_offset);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_tran_get	    - MO Get function for lxb_tran_id
**
** Description:
**	This routine is called by MO to provide a string representation of the
**	(local) transaction ID field. The transaction ID is returned as a
**	16 byte hexadecimal string.
**
** Inputs:
**	offset			    - offset into the block of the tranid
**	objsize			    - size of the tran ID field, ignored
**	object			    - the block address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with tran ID hex string
**
** Returns:
**	STATUS
**
** History:
**	19-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lxb_tran_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    DB_TRAN_ID	*tran = (DB_TRAN_ID *)((char *)object + offset);
    char	format_buf [80];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%x%x",
		tran->db_high_tran, tran->db_low_tran);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_dbname_get	    - MO Get function for the lxb_db_name field
**
** Description:
**	This routine is called by MO to provide a database name column's value
**	for the indicated LXB.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_db_name field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with database name string
**
** Returns:
**	STATUS
**
** History:
**	19-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lxb_dbname_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    LPD		*lpd;
    LDB		*ldb;
    char	format_buf [DB_DB_MAXNAME + 1];

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

    MEcopy(&ldb->ldb_buffer[0], DB_DB_MAXNAME, format_buf);
    format_buf[DB_DB_MAXNAME] = 0;

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_dbowner_get	    - MO Get function for the lxb_db_owner field
**
** Description:
**	This routine is called by MO to provide a database owner column's value
**	for the indicated LXB.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_db_owner field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with database owner string
**
** Returns:
**	STATUS
**
** History:
**	19-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lxb_dbowner_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    LPD		*lpd;
    LDB		*ldb;
    char	format_buf [DB_OWN_MAXNAME + 1];

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

    /* owner is after db name */
    MEcopy(&ldb->ldb_buffer[DB_DB_MAXNAME], DB_OWN_MAXNAME, format_buf);
    format_buf[DB_OWN_MAXNAME] = 0;

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_dbid_get	    - MO Get function for the lxb_db_id_id field
**
** Description:
**	This routine is called by MO to provide a database id column's value
**	for the indicated LXB.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_db_id_id field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with database id string
**
** Returns:
**	STATUS
**
** History:
**	19-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lxb_dbid_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    LPD		*lpd;
    LDB		*ldb;
    char	format_buf [40]; /* for CVLA long to ascii */

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

    CVla((i4)ldb->ldb_id.id_id, format_buf);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_prid_get	    - MO Get function for the lxb_pr_id_id field
**
** Description:
**	This routine is called by MO to provide the process id column's value
**	for the indicated LXB.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_pr_id_id field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with process id string
**
** Returns:
**	STATUS
**
** History:
**	19-nov-1992 (bryanp)
**	    Created
*/
static STATUS
LGmo_lxb_prid_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    LPD		*lpd;
    LPB		*lpb;
    char	format_buf [40]; /* for CVLA long to ascii */

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    CVla((i4)lpb->lpb_id.id_id, format_buf);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_dis_tran_id_hexdump    - MO Get function for XA_RECOVER
**
** Description:
**	XA_RECOVER support wants to be able to fetch distributed transaction
**	ids in a "raw" format. The agreed upon format is:
**
**	    XXXXXXXX:XXXXXXXX:XXXXXXXX: ... :XXXXXXXX:XA
**
**	That is, each 4 byte word is formatted into 8 hex bytes, separated by
**	colons, and the two character string "XA" is appended at the end.
**
**	Since XA_RECOVER support only requires XA distributed transaction ID's,
**	those are the only type of distributed tran ID's which we need
**	hexdumped. For completeness' sake, we'll hexdump Ingres dis id's too.
**
**      23-Sep-1993 (iyer)
**      The above format has since changed. The new format would have two
**      additional fields appended to the end. The new format would be
**
**	    XXXXXXXX:XXXXXXXX:XXXXXXXX: ... :XXXXXXXX:XA:XXXX:XXXX:EX
**
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_dis_tran_id field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with process id string
**
** Returns:
**	STATUS
**
** History:
**	18-jan-1993 (bryanp)
**	    Created
**      23-Sep-1993 (iyer)
**          Call the IICXformat_xa_xid function with a pointer to 
**          DB_XA_EXTD_DIS_TRAN_ID. The function now accepts a pointer
**          to this struct and formats the additional members of the
**          extended structure.
*/
static STATUS
LGmo_lxb_dis_tran_id_hexdump(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB			    *lxb = (LXB *)object;
    char		    format_buf [500];
    DB_XA_EXTD_DIS_TRAN_ID  *xa_extd_tran_id;
    DB_INGRES_DIS_TRAN_ID   *ingres_tran_id;
    i4			    *raw_i4_ptr;
    char		    *out_ptr;
    i4			    i;

    if (lxb->lxb_dis_tran_id.db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE)
    {
	xa_extd_tran_id = &lxb->lxb_dis_tran_id.db_dis_tran_id.
	                  db_xa_extd_dis_tran_id;
	IICXformat_xa_xid(xa_extd_tran_id, format_buf);
    }
    else
    {
	ingres_tran_id = &lxb->lxb_dis_tran_id.db_dis_tran_id.
							db_ingres_dis_tran_id;
	raw_i4_ptr = (i4 *)ingres_tran_id;
	out_ptr = format_buf;

	for (i = 0; i < sizeof(*ingres_tran_id); i += 4)
	{
	    CVlx(*raw_i4_ptr, out_ptr);
	    raw_i4_ptr++;
	    out_ptr += 8;
	    *out_ptr = ':';
	    out_ptr++;
	}
	STcopy("INGRES", out_ptr);
    }

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_is_prepared	    - MO Get function for XA_RECOVER
**
** Description:
**	XA_RECOVER support wants to be able to select only those transaction
**	branches which are currently in WILLING_COMMIT state. This function
**	makes that easy by implementing a column which has a 'Y' if this LXB
**	is a prepared willing_commit transaction, and 'N' otherwise.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_status field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with process id string
**
** Returns:
**	STATUS
**
** History:
**	18-jan-1993 (bryanp)
**	    Created
**	15-Dec-1999 (jenjo02)
**	    If shared txn, return state of it, not handle.
*/
static STATUS
LGmo_lxb_is_prepared(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB			    *lxb = (LXB *)object;
    char		    format_buf [5];

    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

    format_buf[0] = ((lxb->lxb_status & LXB_WILLING_COMMIT) != 0) ? 'Y' : 'N';
    format_buf[1] = '\0';

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_is_xa_dis_tran_id	    - MO Get function for XA_RECOVER
**
** Description:
**	XA_RECOVER support wants to be able to select only those transaction
**	branches which are branches of XA distributed transactions. This
**	function makes that easy by implementing a column which has a 'Y' if
**	this LXB's distributed transaction ID field is of XA type, and 'N'
**	otherwise.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_status field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with process id string
**
** Returns:
**	STATUS
**
** History:
**	18-jan-1993 (bryanp)
**	    Created
**	15-Oct-2003 (jenjo02)
**	    Return "Y" if shared LXB that has no handles.
*/
static STATUS
LGmo_lxb_is_xa_dis_tran_id(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB			    *lxb = (LXB *)object;
    char		    format_buf [5];

    if ( lxb->lxb_dis_tran_id.db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE
	&& (lxb->lxb_status & LXB_SHARED_HANDLE ||
	    lxb->lxb_handle_count == 0) )
	format_buf[0] = 'Y';
    else
	format_buf[0] = 'N';

    format_buf[1] = '\0';

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lfb_status_get	    - MO Get function for the lfb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lfb_status field.
**
** Inputs:
**	offset			    - offset into the LFB, ignored.
**	objsize			    - size of the lfb_status field, ignored
**	object			    - the LFB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
*/
static STATUS
LGmo_lfb_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LFB		*lfb = (LFB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "USE_DIIO,PRIMARY_LOG_OPENED,DUAL_LOG_OPENED,DUAL_LOGGING,\
DISABLE_DUAL_LOGGING",
	    lfb->lfb_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lfb_lgh_status_get - MO Get function for the lfb_lgh_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lfb_header.lgh_status field.
**
** Inputs:
**	offset			    - offset into the LFB, ignored.
**	objsize			    - size of the lfb_lgh_status field, ignored
**	object			    - the LFB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
*/
static STATUS
LGmo_lfb_lgh_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LFB		*lfb = (LFB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    ",VALID,RECOVER,EOF_OK,BAD,OK",
	    lfb->lfb_header.lgh_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lfb_lgh_percentage_logfull_get - MO Get function for the
**      lfb_lgh_percentage_logfull field
**
** Description:
** This routine is called by MO to calculate the percentage logfull value
**
** Inputs:
** offset       - offset into the LFB, ignored.
** objsize       - size of the lfb_lgh_status field, ignored
** object       - the LFB address
** luserbuf      - length of the output buffer
**
** Outputs:
** userbuf       - written with nice status string
**
** Returns:
** STATUS
**
** History:
** 02-nov-1993 (rogt)
**     Created - cribbed
*/
static STATUS
LGmo_lfb_lgh_percentage_logfull_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LFB  *lfb = (LFB *)object;
    i4 percent ;

    percent = LG_used_logspace( lfb, TRUE ) * 100  / lfb->lfb_header.lgh_count ;

    return (MOlongout( MO_VALUE_TRUNCATED, percent, luserbuf, userbuf ));
}


/*
** Name: LGmo_lfb_lgh_tran_get	    - MO Get function for lfb_lgh_tran_id
**
** Description:
**	This routine is called by MO to provide a string representation of the
**	(local) transaction ID field. The transaction ID is returned as a
**	16 byte hexadecimal string.
**
** Inputs:
**	offset			    - offset into the block of the tranid
**	objsize			    - size of the tran ID field, ignored
**	object			    - the block address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with tran ID hex string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
*/
static STATUS
LGmo_lfb_lgh_tran_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    DB_TRAN_ID	*tran = (DB_TRAN_ID *)((char *)object + offset);
    char	format_buf [80];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%x%x",
		tran->db_high_tran, tran->db_low_tran);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lsn_get	    - MO Get function for log sequence number fields
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of any log sequence number field. The log sequence
**	number is returned in simple "<%d,%d>" format.
**
** Inputs:
**	offset			    - offset into the block of the log seq no
**	objsize			    - size of the log seq no field, ignored
**	object			    - the block address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice log seq no string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
*/
static STATUS
LGmo_lsn_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LG_LSN	*lsn = (LG_LSN *)((char *)object + offset);
    char	format_buf [80];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "<%d,%d>",
		lsn->lsn_high, lsn->lsn_low);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lfb_active_log_get - MO Get function for the lfb_active_log field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lfb_active_log field.
**
** Inputs:
**	offset			    - offset into the LFB, ignored.
**	objsize			    - size of the lfb_active_log field, ignored
**	object			    - the LFB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
*/
static STATUS
LGmo_lfb_active_log_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LFB		*lfb = (LFB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    "II_LOG_FILE,II_DUAL_LOG",
	    lfb->lfb_active_log);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LGmo_lxb_wait_reason_get - MO Get function for lxb_wait_reason field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lxb_wait_reason field.
**
** Inputs:
**	offset			    - offset into the LXB, ignored.
**	objsize			    - size of the lxb_wait_reason field, ignored
**	object			    - the LXB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	26-apr-1993 (bryanp)
**	    Created
**	23-aug-1993 (bryanp)
**	    Add wait reason LXB_LOGSPACE_WAIT.
*/
static STATUS
LGmo_lxb_wait_reason_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LXB		*lxb = (LXB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    ",FORCE,SPLIT,HDR_IO,CKPDB,OPENDB,BCPSTALL,LOGFULL,\
FREEBUF,LASTBUF,FORCED_IO,EVENT,WRITEIO,MINI",
	    lxb->lxb_wait_reason);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** History:
**	26-apr-1993 (bryanp)
**	    Updates for 6.5 cluster support. Currently, we provide only the
**	    local LFB's information, and so we don't have an index method;
**	    instead we set the cdata in each LFB's class to point to the local
**	    lfb. If we ever enhance this code to support a multi-row LFB table
**	    (for instrumenting cluster recovery), then we'll need to remove
**	    this artifical setting of the LFB's cdata and use an index method
**	    instead.
*/
STATUS
LGmo_attach_lg(void)
{
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    STATUS	status;
    i4		i;

    for (i = 0;
	 i < (sizeof(LGmo_lgd_classes) / sizeof(LGmo_lgd_classes[0]));
	 i++)
    {
	LGmo_lgd_classes[i].cdata = (PTR)lgd;
    }

    for (i = 0;
	 i < (sizeof(LGmo_lfb_classes) / sizeof(LGmo_lfb_classes[0]));
	 i++)
    {
	LGmo_lfb_classes[i].cdata = (PTR)&(lgd->lgd_local_lfb);
    }

    status = MOclassdef(MAXI2, LGmo_lgd_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LGmo_lfb_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LGmo_lpb_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LGmo_ldb_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LGmo_lxb_classes);

    return (status);
}

/*
**   Name: IICXformat_xa_xid()
**
**   Description: 
**       Formats the XA XID into the text buffer.
**       
**   WARNING: 
**       Must match IIformat_xa_id() in libqxa/iicxxa.sc
**
**   Inputs:
**       xa_extd_xid_p  - pointer to the XA XID.
**       text_buffer    - pointer to a text buffer that will contain the text
**                        form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**      02-Oct-1992 - First written (mani)
**	18-jan-1993 (bryanp)
**	    Cut and pasted from libqxa into this file. Renamed static but
**	    otherwise unchanged. Probably this routine belongs in cuflib or
**	    some similar place to avoid the duplication of code, but ....
**      22-sep-1993 (iyer)
**          The function now accepts a pointer to the extended XA structure.
**          The format string now has the branch_seqnum and branch_flag added
**          at the end.
**      05-oct-1998 (thoda04)
**          Process XID data bytes in groups of four, high order to low, 
**          bytes for all platforms.
**      11-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          long is not an i4 on 64-bit platforms. Make the necessary
**          changes to ensure XA xid values are hadled correctly when
**          a long is 8 bytes.
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
*/
static VOID
IICXformat_xa_xid(DB_XA_EXTD_DIS_TRAN_ID    *xa_extd_xid_p,
              char       *text_buffer)
{
  char     *cp = text_buffer;
  i4       data_lword_count = 0;
  i4       data_byte_count  = 0;
  u_char   *tp;                  /* pointer to xid data */
  u_i4 unum;                /* unsigned i4 work field */
  i4       i;

  CVlx(xa_extd_xid_p->db_xa_dis_tran_id.formatID,cp);
  STcat(cp, ERx(":"));
  cp += STlength(cp);
 
  CVla((i4)(xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length),cp);
  STcat(cp, ERx(":"));
  cp += STlength(cp);

  CVla((i4)(xa_extd_xid_p->db_xa_dis_tran_id.bqual_length),cp);
  cp += STlength(cp);

  data_byte_count = (i4)(xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length + 
                          xa_extd_xid_p->db_xa_dis_tran_id.bqual_length);

  data_lword_count = (data_byte_count + sizeof(u_i4) - 1) / sizeof(u_i4);
  tp = (u_char*)(xa_extd_xid_p->db_xa_dis_tran_id.data);
                             /* tp -> B0B1B2B3 xid binary data */

  for (i = 0; i < data_lword_count; i++)
  {
     STcat(cp, ERx(":"));
     cp++;
     unum = (u_i4)(((i4)(tp[0])<<24) |   /* watch out for byte order */
                        ((i4)(tp[1])<<16) | 
                        ((i4)(tp[2])<<8)  | 
                         (i4)(tp[3]));
     CVlx(unum, cp);     /* build string "B0B1B2B3" in byte order for all platforms*/
     cp += STlength(cp);
     tp += sizeof(u_i4);
  }
  STcat(cp, ERx(":XA"));

/* Appends branch_seqnum and branch_flag at the end */

  STcat(cp, ERx(":"));  
  cp += STlength (cp);

  CVna(xa_extd_xid_p->branch_seqnum,cp);
  STcat(cp, ERx(":"));  
  cp += STlength(cp);

  CVna(xa_extd_xid_p->branch_flag,cp);
  STcat(cp, ERx(":EX"));  
  

} /* IICXformat_xa_xid */
