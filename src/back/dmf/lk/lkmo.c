/*
** Copyright (c) 1992, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <ulf.h>
#include    <lkdef.h>
#include    <lgkdef.h>

/*
** Name: LKMO.C		- LK Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for LK. Managed objects appear in the MIB, and allow
**	monitoring, analysis, and control of LK through the MO interface.
**
** History:
**	Fall, 1992 (bryanp)
**	    Written for the new locking system.
**	5-Dec-1992 (daveb)
**	    Make a bunch of things unsigned output; some were coming out
**	    with minus signs.
**	11-Mar-1993 (daveb)
**	    Add lkb_rsb_id_id object, so you can join the tables.
**	30-Mar-1993 (daveb)
**	    llb_timeout needs to be unsigned, so -1 comes out as big number
**	    instead of 00000000-1.
**	1-Apr-1993 (daveb)
**	    llb_sid should be MOuintget too.
**	26-apr-1993 (bryanp)
**	    Updates due to 6.5 cluster support.
**	24-may-1993 (bryanp)
**	    More cluster-related updates: new statistics, new bitflag values.
**	    Various changes to support RSB's in the new rbk_table.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions and proper prototypes for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>, <cv.h>
**      10-Mar-1994 (daveb) 60514
**          Use right output function for SCBs and SIDs, MOptrget or MOpvget.
**      16-Mar-1994 (daveb)
**          Add missing index object references for
**  	    "exp.dmf.lk.llb_status",
**  	    "exp.dmf.lk.llb_related_llb_id_id",
**          "exp.dmf.lk.llb_wait_id_id",
**  	    "exp.dmf.lk.lkb_grant_mode",
**  	    "exp.dmf.lk.lkb_request_mode",
**  	    "exp.dmf.lk.lkb_state",
**  	    "exp.dmf.lk.lkb_attribute",
**  	    "exp.dmf.lk.rsb_grant_mode",
**  	    "exp.dmf.lk.rsb_convert_mode",
**  	    "exp.dmf.lk.rsb_name"
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex field address
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**	19-Dec-1995 (jenjo02)
**	     Removed semaphore protection of various queues, letting
**	     dirty reads prevail. The queues being protected are
**	     not subject to deletion, and the checks surrounding
**	     the veracity of the objects should suffice.
**	03-jan-1996 (duursma) bug #71334
**	    Deleted references to old lkd_stat.unsent_gdlck_search.
**	    Added references to new lkd_stat.walk_wait_for.
**	12-Jan-1996 (jenjo02)
**	     lkd_stat.next_id renamed to lkd_next_llbname.
**      22-nov-1996 (dilma04)
**          Row Locking Project:
**          Add support for LK_PH_PAGE and LK_VAL_LOCK.
**	25-Feb-1997 (jenjo02)
**	    Replaced llb_timeout (unused) with llb_ew_stamp.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	08-Sep-1998 (jenjo02)
**	    Added new lkd_stat, max locks allocated per transaction.
**	16-Nov-1998 (jenjo02)
**	  o Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	08-Feb-1999 (jenjo02)
**	    Extract waiting LKB from llb_lkb_wait instead of llb_wait.
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	08-Oct-1999 (jenjo02)
**	    LK_ID changed from structure to u_i4, which is what it was
**	    anyway. Affects rsb_id, lkb_id.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	28-Feb-2002 (jenjo02)
**	    LLB_WAITING is now contrived. Added LKmo_llb_status_num_get
**	    get method to decide setting of LLB_WAITING based on
**	    llb_waiters.
**	07-Jul-2003 (devjo01)
**	    Use MOsidget to extract session ID for an LLB.
**      08-Jan-2004 (devjo01)
**	    Hide type of cx_lock_id for Linux port.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	13-Mar-2006 (jenjo02)
**	    Deleted llb_ew_stamp.
**	04-Oct-2007 (jonj)
**	    CX_EXTRACT_LOCK_ID parms in LK_lkb_cx_lock_id_get() reversed,
**	    clobbering lkb->lkb_cx_req.cx_lockid and causing 
**	    LK_NOTHELD errors during lock conversion.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: lkd_tstat[0] now holds cumulative stats.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
*/

static STATUS	LKmo_llb_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	LKmo_lkb_index(
		    i4	    msg,
		    PTR	    cdata,
		    i4	    linstance,
		    char    *instance,
		    PTR	    *instdata);

static STATUS	LKmo_rsb_index(
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
static MO_GET_METHOD LKmo_lkd_status_get;
static MO_GET_METHOD LKmo_llb_status_get;
static MO_GET_METHOD LKmo_llb_status_num_get;
static MO_GET_METHOD LKmo_llb_related_llb_id_get;
static MO_GET_METHOD LKmo_llb_wait_id_get;
static MO_GET_METHOD LKmo_lkb_state_get;
static MO_GET_METHOD LKmo_lkb_attribute_get;
static MO_GET_METHOD LKmo_lkb_grant_mode_get;
static MO_GET_METHOD LKmo_lkb_request_mode_get;
static MO_GET_METHOD LKmo_rsb_grant_mode_get;
static MO_GET_METHOD LKmo_rsb_convert_mode_get;
static MO_GET_METHOD LKmo_rsb_name_get;


static MO_CLASS_DEF LKmo_lkd_classes[] =
{
    {0, "exp.dmf.lk.lkd_status",
	0, MO_READ, 0,
	0, LKmo_lkd_status_get, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_status_num",
	MO_SIZEOF_MEMBER(LKD, lkd_status), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_status),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_csp_id",
	MO_SIZEOF_MEMBER(LKD, lkd_csp_id), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_csp_id),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_rsh_size",
	MO_SIZEOF_MEMBER(LKD, lkd_rsh_size), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_rsh_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_lkh_size",
	MO_SIZEOF_MEMBER(LKD, lkd_lkh_size), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_lkh_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_max_lkb",
	MO_SIZEOF_MEMBER(LKD, lkd_max_lkb), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_max_lkb),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_llb_inuse",
	MO_SIZEOF_MEMBER(LKD, lkd_llb_inuse), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_llb_inuse),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_rsb_inuse",
	MO_SIZEOF_MEMBER(LKD, lkd_rsb_inuse), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_rsb_inuse),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_lkb_inuse",
	MO_SIZEOF_MEMBER(LKD, lkd_lkb_inuse), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_lkb_inuse),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_lbk_count",
	MO_SIZEOF_MEMBER(LKD, lkd_lbk_count), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_lbk_count),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_lbk_size",
	MO_SIZEOF_MEMBER(LKD, lkd_lbk_size), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_lbk_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_sbk_count",
	MO_SIZEOF_MEMBER(LKD, lkd_sbk_count), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_sbk_count),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_sbk_size",
	MO_SIZEOF_MEMBER(LKD, lkd_sbk_size), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_sbk_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_rbk_count",
	MO_SIZEOF_MEMBER(LKD, lkd_rbk_count), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_rbk_count),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_rbk_size",
	MO_SIZEOF_MEMBER(LKD, lkd_rbk_size), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_rbk_size),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_lock_stall",
	MO_SIZEOF_MEMBER(LKD, lkd_lock_stall), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_lock_stall),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_node_fail",
	MO_SIZEOF_MEMBER(LKD, lkd_node_fail), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_node_fail),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_csid",
	MO_SIZEOF_MEMBER(LKD, lkd_csid), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_csid),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_next_llbname",
	MO_SIZEOF_MEMBER(LKD, lkd_next_llbname), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_next_llbname),
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.create_list",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.create_list), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.create_list),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.request_new",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].request_new), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].request_new),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.request_convert",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].request_convert), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].request_convert),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.convert",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].convert), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].convert),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.release",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].release), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].release),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.release_partial",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.release_partial), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.release_partial),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.release_all",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.release_all), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.release_all),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.wait",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].wait), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].wait),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.convert_wait",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].convert_wait), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].convert_wait),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.convert_search",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.convert_search), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.convert_search),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.convert_deadlock",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].convert_deadlock), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].convert_deadlock),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.deadlock_search",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.deadlock_search), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.deadlock_search),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.deadlock",
	MO_SIZEOF_MEMBER(LKD, lkd_tstat[0].deadlock), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_tstat[0].deadlock),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.cancel",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.cancel), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.cancel),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.enq",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.enq), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.enq),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.deq",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.deq), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.deq),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.gdlck_search",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.gdlck_search), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.gdlck_search),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.gdeadlock",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.gdeadlock), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.gdeadlock),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.gdlck_grant",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.gdlck_grant), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.gdlck_grant),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.totl_gdlck_search",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.totl_gdlck_search), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.totl_gdlck_search),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.gdlck_call",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.gdlck_call), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.gdlck_call),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.gdlck_sent",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.gdlck_sent), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.gdlck_sent),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.cnt_gdlck_call",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.cnt_gdlck_call), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.cnt_gdlck_call),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.cnt_gdlck_sent",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.cnt_gdlck_sent), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.cnt_gdlck_sent),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.walk_wait_for",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.walk_wait_for), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.walk_wait_for),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.sent_all",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.sent_all), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.sent_all),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.synch_complete",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.synch_complete), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.synch_complete),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.asynch_complete",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.asynch_complete), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.asynch_complete),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.csp_msgs_rcvd",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.csp_msgs_rcvd), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.csp_msgs_rcvd),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.csp_wakeups_sent",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.csp_wakeups_sent), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.csp_wakeups_sent),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.allocate_cb",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.allocate_cb), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.allocate_cb),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.deallocate_cb",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.deallocate_cb), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.deallocate_cb),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.sbk_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.sbk_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.sbk_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.lbk_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.lbk_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.lbk_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.rbk_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.rbk_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.rbk_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.rsb_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.rsb_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.rsb_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.lkb_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.lkb_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.lkb_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.llb_highwater",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.llb_highwater), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.llb_highwater),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.max_lcl_dlk_srch",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.max_lcl_dlk_srch), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.max_lcl_dlk_srch),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.dlock_locks_examined",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.dlock_locks_examined), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.dlock_locks_examined),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.max_rsrc_chain_len",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.max_rsrc_chain_len), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.max_rsrc_chain_len),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.max_lock_chain_len",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.max_lock_chain_len), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.max_lock_chain_len),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.lk.lkd_stat.max_locks_per_txn",
	MO_SIZEOF_MEMBER(LKD, lkd_stat.max_lkb_per_txn), MO_READ, 0,
	CL_OFFSETOF(LKD, lkd_stat.max_lkb_per_txn),
	MOuintget, MOnoset, (PTR)0, MOcdata_index
    },
    { 0 }
};

static char llb_index[] = "exp.dmf.lk.llb_id.id_id";

static MO_CLASS_DEF LKmo_llb_classes[] =
{
    {0, "exp.dmf.lk.llb_id.id_id",
	MO_SIZEOF_MEMBER(LLB, llb_id.id_id), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_id.id_id),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_lkb_count",
	MO_SIZEOF_MEMBER(LLB, llb_lkb_count), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_lkb_count),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_status",
	0, MO_READ, llb_index,
	0, LKmo_llb_status_get, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_status_num",
	0, MO_READ, llb_index,
	0, LKmo_llb_status_num_get, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_llkb_count",
	MO_SIZEOF_MEMBER(LLB, llb_llkb_count), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_llkb_count),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_max_lkb",
	MO_SIZEOF_MEMBER(LLB, llb_max_lkb), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_max_lkb),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_related_llb_id_id",
	0, MO_READ, llb_index,
	0, LKmo_llb_related_llb_id_get, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_related_llb",
	MO_SIZEOF_MEMBER(LLB, llb_related_llb), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_related_llb),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_related_count",
	MO_SIZEOF_MEMBER(LLB, llb_related_count), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_related_count),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_name0",
	MO_SIZEOF_MEMBER(LLB, llb_name[0]), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_name[0]),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_name1",
	MO_SIZEOF_MEMBER(LLB, llb_name[1]), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_name[1]),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_wait_id_id",
	0, MO_READ, llb_index,
	0, LKmo_llb_wait_id_get, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_search_count",
	MO_SIZEOF_MEMBER(LLB, llb_search_count), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_search_count),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_pid",
	MO_SIZEOF_MEMBER(LLB, llb_cpid.pid), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_cpid.pid),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_sid",
	MO_SIZEOF_MEMBER(LLB, llb_cpid.sid), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_cpid.sid),
	MOsidget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_connect_count",
	MO_SIZEOF_MEMBER(LLB, llb_connect_count), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_connect_count),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_event.type_high",
	MO_SIZEOF_MEMBER(LLB, llb_event.type_high), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_event.type_high),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_event.type_low",
	MO_SIZEOF_MEMBER(LLB, llb_event.type_low), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_event.type_low),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_event.value",
	MO_SIZEOF_MEMBER(LLB, llb_event.value), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_event.value),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_evflags",
	MO_SIZEOF_MEMBER(LLB, llb_evflags), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_evflags),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_stamp",
	MO_SIZEOF_MEMBER(LLB, llb_stamp), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_stamp),
	MOuintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    {0, "exp.dmf.lk.llb_tick",
	MO_SIZEOF_MEMBER(LLB, llb_tick), MO_READ, llb_index,
	CL_OFFSETOF(LLB, llb_tick),
	MOintget, MOnoset, (PTR)0, LKmo_llb_index
    },
    { 0 }
};

static STATUS
LKmo_llb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *lbk_table;
    LLB		    *llb;

    CVal(instance, &ix);

    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lkd->lkd_lbk_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[ix]);
	    if (llb->llb_type != LLB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)llb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lkd->lkd_lbk_count)
	    {
		llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[ix]);
		if (llb->llb_type == LLB_TYPE)
		{
		    *instdata = (PTR)llb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lkd->lkd_lbk_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

static MO_GET_METHOD LK_lkb_rsb_get;
static MO_GET_METHOD LK_lkb_llb_get;
static MO_GET_METHOD LK_lkb_cx_lock_id_get;

static char lkb_index[] = "exp.dmf.lk.lkb_id.id_id";

static MO_CLASS_DEF LKmo_lkb_classes[] =
{
    {0, "exp.dmf.lk.lkb_id.id_id",
	MO_SIZEOF_MEMBER(LKB, lkb_id), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_id),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_count",
	MO_SIZEOF_MEMBER(LKB, lkb_count), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_count),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_grant_mode",
	0, MO_READ, lkb_index,
	0, LKmo_lkb_grant_mode_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_grant_mode_num",
	MO_SIZEOF_MEMBER(LKB, lkb_grant_mode), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_grant_mode),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_request_mode",
	0, MO_READ, lkb_index,
	0, LKmo_lkb_request_mode_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_request_mode_num",
	MO_SIZEOF_MEMBER(LKB, lkb_request_mode), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_request_mode),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_state",
	0, MO_READ, lkb_index,
	0, LKmo_lkb_state_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_state_num",
	MO_SIZEOF_MEMBER(LKB, lkb_state), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_state),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_attribute",
	0, MO_READ, lkb_index,
	0, LKmo_lkb_attribute_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_attribute_num",
	MO_SIZEOF_MEMBER(LKB, lkb_attribute), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_attribute),
	MOintget, MOnoset, (PTR)0, LKmo_lkb_index
    },
    {0, "exp.dmf.lk.lkb_rsb_id_id",
	0, MO_READ, lkb_index,
	0,
	LK_lkb_rsb_get, MOnoset, (PTR)0, LKmo_lkb_index
    }, 
    {0, "exp.dmf.lk.lkb_llb_id_id",
	0, MO_READ, lkb_index,
	0,
	LK_lkb_llb_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
   {0, "exp.dmf.lk.lkb_cx_req.cx_lock_id",
	MO_SIZEOF_MEMBER(LKB, lkb_cx_req.cx_lock_id), MO_READ, lkb_index,
	CL_OFFSETOF(LKB, lkb_cx_req.cx_lock_id),
	LK_lkb_cx_lock_id_get, MOnoset, (PTR)0, LKmo_lkb_index
    },
    { 0 }
};
static char *lock_key_string =
",DATABASE,TABLE,PAGE,EXTEND,SV_PAGE,CREATE,NAME,CONFIG,\
DB_TBL_ID,SV_DATABASE,SV_TABLE,EVENT,CONTROL,JOURNAL,OPEN_DB,CKP_DB,\
CKP_CLUSTER,BM_LOCK,BM_DATABASE,BM_TABLE,SYS_CONTROL,EV_CONNECT,AUDIT,ROW";

/*{
** Name:	LK_lkb_rsb_get -- MO get method for RSB id for LKB
**
** Description:
**	Finds the RSB id the lock block belongs to.
**
** Re-entrancy:
**	Potentially a problem -- it races up an unprotected queue.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		taken to be a pointer to an LKB
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with RSB id, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	11-Mar-1993 (daveb)
**	    created.
**	20-Jun-2003 (jenjo02)
**	    Sanity check lkb and rsb before dereferencing.
**	    B110495, INGSRV2396.
*/

static STATUS
LK_lkb_rsb_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS cl_stat = MO_NO_INSTANCE;
    LKB		*lkb;
    RSB		*rsb;

    if ( (lkb = (LKB*)object) &&
	(rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb)) )
    {
	cl_stat = MOulongout( MO_VALUE_TRUNCATED,
			     (u_i8)rsb->rsb_id, lsbuf, sbuf );
    }

    return( cl_stat );
}
	      

/*{
** Name:	LK_lkb_llb_get -- MO get method for LLB id for LKB
**
** Description:
**	Finds the LLB id the lock block belongs to.
**
** Re-entrancy:
**	Potentially a problem -- it races up an unprotected queue.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		taken to be a pointer to an LKB
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with LLB id, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	11-Mar-1993 (daveb)
**	    created.
**	20-Jun-2003 (jenjo02)
**	    Sanity check lkb and llb before dereferencing.
**	    B110495, INGSRV2396.
*/

static STATUS
LK_lkb_llb_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS cl_stat = MO_NO_INSTANCE;
    LKB		*lkb;
    LLB		*llb;

    if ( (lkb = (LKB*)object) &&
	(llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb)) )
    {
	cl_stat = MOulongout( MO_VALUE_TRUNCATED,
			     (u_i8)llb->llb_id.id_id, lsbuf, sbuf );
    }

    return( cl_stat );
}


static STATUS
LK_lkb_cx_lock_id_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    char   format_buf[50];
    LKB	*lkb = (LKB *)object;
    LK_UNIQUE	lock_id;

    /* First parm is destination, second, source */
    CX_EXTRACT_LOCK_ID(&lock_id, &lkb->lkb_cx_req.cx_lock_id);
    TRformat(NULL, 0, format_buf, sizeof(format_buf),"%x.%x",
	     lock_id.lk_uhigh, lock_id.lk_ulow);
    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, lsbuf, sbuf ));
}

static STATUS
LKmo_lkb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *sbk_table;
    LKB		    *lkb;

    CVal(instance, &ix);

    sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lkd->lkd_sbk_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[ix]);
	    if (lkb->lkb_type != LKB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)lkb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lkd->lkd_sbk_count)
	    {
		lkb = (LKB *)LGK_PTR_FROM_OFFSET(sbk_table[ix]);
		if (lkb->lkb_type == LKB_TYPE)
		{
		    *instdata = (PTR)lkb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lkd->lkd_sbk_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

static char rsb_index[] = "exp.dmf.lk.rsb_id.id_id";

static MO_CLASS_DEF LKmo_rsb_classes[] =
{
    {0, "exp.dmf.lk.rsb_id.id_id",
	MO_SIZEOF_MEMBER(RSB, rsb_id), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_id),
	MOintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_grant_mode",
	0, MO_READ, rsb_index,
	0, LKmo_rsb_grant_mode_get, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_grant_mode_num",
	MO_SIZEOF_MEMBER(RSB, rsb_grant_mode), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_grant_mode),
	MOintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_convert_mode",
	0, MO_READ, rsb_index,
	0, LKmo_rsb_convert_mode_get, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_convert_mode_num",
	MO_SIZEOF_MEMBER(RSB, rsb_convert_mode), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_convert_mode),
	MOintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name",
	0, MO_READ, rsb_index,
	0, LKmo_rsb_name_get, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name0",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_type), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_type),
	MOintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name1",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key1), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key1),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name2",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key2), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key2),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name3",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key3), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key3),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name4",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key4), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key4),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name5",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key5), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key5),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_name6",
	MO_SIZEOF_MEMBER(RSB, rsb_name.lk_key6), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_name.lk_key6),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_value0",
	MO_SIZEOF_MEMBER(RSB, rsb_value[0]), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_value[0]),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_value1",
	MO_SIZEOF_MEMBER(RSB, rsb_value[1]), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_value[1]),
	MOuintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    {0, "exp.dmf.lk.rsb_invalid",
	MO_SIZEOF_MEMBER(RSB, rsb_invalid), MO_READ, rsb_index,
	CL_OFFSETOF(RSB, rsb_invalid),
	MOintget, MOnoset, (PTR)0, LKmo_rsb_index
    },
    { 0 }
};

/*
** Name: LKmo_rsb_index		- MO index method for rsb table traversing.
**
** History:
**	24-may-1993 (bryanp)
**	    RSB's are now separate from LKBs, so we walk the rbk_table.
**      17-nov-1994 (medji01)
**          Mutex Granularity Project
**              - Changed LK_xmutex() calls to pass mutex field address
*/
static STATUS
LKmo_rsb_index(
    i4	    msg,
    PTR	    cdata,
    i4	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS	    status = OK;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;
    i4	    ix;
    SIZE_TYPE	    *rbk_table;
    RSB		    *rsb;

    CVal(instance, &ix);

    rbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_rbk_table);

    switch(msg)
    {
	case MO_GET:
	    if (ix < 1 || ix > lkd->lkd_rbk_count)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(rbk_table[ix]);
	    if (rsb->rsb_type != RSB_TYPE)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    *instdata = (PTR)rsb;
	    status = OK;

	    break;

	case MO_GETNEXT:
	    while (++ix <= lkd->lkd_rbk_count)
	    {
		rsb = (RSB *)LGK_PTR_FROM_OFFSET(rbk_table[ix]);
		if (rsb->rsb_type == RSB_TYPE)
		{
		    *instdata = (PTR)rsb;
		    status = MOlongout( MO_INSTANCE_TRUNCATED,
					(i8)ix,
					linstance, instance);
		    break;
		}
	    }
	    if (ix > lkd->lkd_rbk_count)
		status = MO_NO_INSTANCE;
		
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

/*
** Name: LKmo_lkd_status_get	    - MO Get function for the lkd_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lkd_status field.
**
** Inputs:
**	offset			    - offset into the LKD, ignored.
**	objsize			    - size of the lkd_status field, ignored
**	object			    - the LKD address
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
**	    Created.
**	24-may-1993 (bryanp)
**	    More cluster-related updates: new statistics, new bitflag values.
**	19-Nov-2007 (jonj)
**	    Use new LKD_STATUS define for string constants.
*/
static STATUS
LKmo_lkd_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LKD		*lkd = (LKD *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    LKD_STATUS, lkd->lkd_status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_llb_status_get	    - MO Get function for the llb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the llb_status field.
**
** Inputs:
**	offset			    - offset into the LLB, ignored.
**	objsize			    - size of the llb_status field, ignored
**	object			    - the LLB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LLB_STATUS define for string constants.
*/
static STATUS
LKmo_llb_status_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LLB		*llb = (LLB *)object;
    i4		status = llb->llb_status;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    if ( llb->llb_waiters )
	status |= LLB_WAITING;

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    LLB_STATUS, status);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_llb_status_num_get	    - MO Get function for the llb_status field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the llb_status field.
**
** Inputs:
**	offset			    - offset into the LLB, ignored.
**	objsize			    - size of the llb_status field, ignored
**	object			    - the LLB address
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - written with nice status string
**
** Returns:
**	STATUS
**
** History:
**	22-Jan-2002 (jenjo02)
**	    Created to deduce LLB_WAITING
*/
static STATUS
LKmo_llb_status_num_get(
i4 offset,
i4 objsize,
PTR object,
i4 lsbuf,
char *sbuf )
{
    LLB		*llb = (LLB *)object;
    i4		status = llb->llb_status;

    if ( llb->llb_waiters )
	status |= LLB_WAITING;

    return (MOulongout( MO_VALUE_TRUNCATED,
			(u_i8)status, lsbuf, sbuf));
}

/*{
** Name:	LKmo_llb_related_llb_id_get -- MO get method for related list
**
** Description:
**	Finds the id of the list to which this lock list is related.
**
** Re-entrancy:
**	Potentially a problem -- it races up an unprotected queue.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		taken to be a pointer to an LLB
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with related ID, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	26-apr-1993 (bryanp)
**	    created.
*/
static STATUS
LKmo_llb_related_llb_id_get(
    i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    LLB	*llb = (LLB *)object;
    LLB *related_llb;

    if (llb->llb_related_llb)
    {
	related_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
	return (MOulongout( MO_VALUE_TRUNCATED,
			    (u_i8)related_llb->llb_id.id_id, lsbuf, sbuf));
    }
    else
    {
	return (MOulongout( MO_VALUE_TRUNCATED,
			    (u_i8)0, lsbuf, sbuf));
    }
}

/*{
** Name:	LKmo_llb_wait_id_get -- MO get method for wait ID
**
** Description:
**	Finds the id of the lock on which this lock list is waiting.
**
** Re-entrancy:
**	Potentially a problem -- it races up an unprotected queue.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		taken to be a pointer to an LLB
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with lock ID, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	26-apr-1993 (bryanp)
**	    created.
*/
static STATUS
LKmo_llb_wait_id_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    LLB	*llb = (LLB *)object;
    LKB	*wait_lkb;
    LLBQ	*llbq;

    if ( llb->llb_waiters )
    {
	llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_next);
	wait_lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
	return (MOulongout( MO_VALUE_TRUNCATED,
			    (u_i8)wait_lkb->lkb_id, lsbuf, sbuf));
    }
    else
    {
	return (MOulongout( MO_VALUE_TRUNCATED,
			    (u_i8)0, lsbuf, sbuf));
    }
}

/*
** Name: LKmo_lkb_state_get	    - MO Get function for the lkb_state field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lkb_state field.
**
** Inputs:
**	offset			    - offset into the LKB, ignored.
**	objsize			    - size of the lkb_state field, ignored
**	object			    - the LKB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LKB_SSTATE define for string constants.
*/
static STATUS
LKmo_lkb_state_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LKB		*lkb = (LKB *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    LKB_STATE, lkb->lkb_state);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

static MO_GET_METHOD LKmo_lkb_attribute_get;

/*
** Name: LKmo_lkb_attribute_get	- MO Get function for the lkb_attribute field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lkb_attribute field.
**
** Inputs:
**	offset			    - offset into the LKB, ignored.
**	objsize			    - size of the lkb_attribute field, ignored
**	object			    - the LKB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LKB_ATTRIBUTE define for string constants.
*/
static STATUS
LKmo_lkb_attribute_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LKB		*lkb = (LKB *)object;
    char	format_buf [500];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
	    LKB_ATTRIBUTE, lkb->lkb_attribute);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_lkb_grant_mode_get - MO Get function for the lkb_grant_mode field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lkb_grant_mode field.
**
** Inputs:
**	offset			    - offset into the LKB, ignored.
**	objsize			    - size of the lkb_grant_mode field, ignored
**	object			    - the LKB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LOCK_MODE define for string constants.
*/
static STATUS
LKmo_lkb_grant_mode_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LKB		*lkb = (LKB *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    LOCK_MODE, lkb->lkb_grant_mode);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_lkb_request_mode_get - MO Get ftn for the lkb_request_mode field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the lkb_request_mode field.
**
** Inputs:
**	offset			    - offset into the LKB, ignored.
**	objsize			    - size of the lkb_request_mode field.
**	object			    - the LKB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LOCK_MODE define for string constants.
*/
static STATUS
LKmo_lkb_request_mode_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    LKB		*lkb = (LKB *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    LOCK_MODE, lkb->lkb_request_mode);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_rsb_grant_mode_get - MO Get function for the rsb_grant_mode field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the rsb_grant_mode field.
**
** Inputs:
**	offset			    - offset into the RSB, ignored.
**	objsize			    - size of the rsb_grant_mode field, ignored
**	object			    - the RSB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LOCK_MODE define for string constants.
*/
static STATUS
LKmo_rsb_grant_mode_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    RSB		*rsb = (RSB *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    LOCK_MODE, rsb->rsb_grant_mode);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_rsb_convert_mode_get - MO Get function for rsb_convert_mode field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the rsb_convert_mode field.
**
** Inputs:
**	offset			    - offset into the RSB, ignored.
**	objsize			    - size of the rsb_convert_mode field.
**	object			    - the RSB address
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
**	    Created.
**	19-Nov-2007 (jonj)
**	    Use new LOCK_MODE define for string constants.
*/
static STATUS
LKmo_rsb_convert_mode_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    RSB		*rsb = (RSB *)object;
    char	format_buf [50];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
	    LOCK_MODE, rsb->rsb_convert_mode);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

/*
** Name: LKmo_rsb_name_get - MO Get function for the rsb_name field
**
** Description:
**	This routine is called by MO to provide a "user-friendly"
**	interpretation of the rsb_name field.
**
** Inputs:
**	offset			    - offset into the RSB, ignored.
**	objsize			    - size of the rsb_name field, ignored
**	object			    - the RSB address
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
**	    Created.
**      22-nov-1996 (dilma04)
**          Row Locking Project:
**          Add support for LK_PH_PAGE and LK_VAL_LOCK.
**	26-Jan-2004 (jenjo02)
**	    Remove DB_TAB_PARTITION_MASK bit from lk_key3
**	    for PAGE/ROW locks
**	    unless temp table (lk_key2 < 0).
**	15-Jan-2010 (jonj)
**	    Replace guts with call to common lock key formatter,
**	    LKkey_to_string().
*/
static STATUS
LKmo_rsb_name_get(
i4  offset,
i4  objsize,
PTR object,
i4  luserbuf,
char *userbuf )
{
    RSB		*rsb = (RSB *)object;
    char	format_buf [150];

    MEfill(sizeof(format_buf), (u_char)0, format_buf);

    LKkey_to_string(&rsb->rsb_name, format_buf);

    return (MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}

STATUS
LKmo_attach_lk(void)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    STATUS	status;
    i4		i;

    for (i = 0;
	 i < (sizeof(LKmo_lkd_classes) / sizeof(LKmo_lkd_classes[0]));
	 i++)
    {
	LKmo_lkd_classes[i].cdata = (PTR)lkd;
    }

    status = MOclassdef(MAXI2, LKmo_lkd_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LKmo_llb_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LKmo_lkb_classes);

    if (status == OK)
	status = MOclassdef(MAXI2, LKmo_rsb_classes);

    return (status);
}
