/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <clconfig.h>
# include <cs.h>
# include <mo.h>
# include <tr.h>

# include <csinternal.h>
# include "cslocal.h"

# include "csmgmt.h"

/**
** Name:	csmoscb.c	- MO interface to thread SCBs
**
** Description:
**	 MO interfaces for UNIX CS SCBs.
**
** Functions:
**
**	CS_scb_attach	- make an SCB known to MO/IMA
**	CS_detach_scb	- make an SCB unknown to MO/IMA
**	CS_scb_index	- MO index functions for known CS SCB's
**	CS_scb_self_get	- MO get function for cs_self field
**	CS_state_get	- MO get function for cs_state field
**	CS_memory_get	- MO get function for cs_memory field
**	CS_thread_type_get	- MO get function for cs_thread_type field
**	CS_scb_mask_get	- MO get function for cs_ mask fields.
**	CS_thread_id_get- MO get function for cs_thread_id field
**	CS_scb_bio_get  - MO get function for sum(cs_bior+cs_biow)
**	CS_scb_dio_get  - MO get function for sum(cs_dior+cs_diow)
**	CS_scb_lio_get  - MO get function for sum(cs_lior+cs_liow)
**	CS_scb_cpu_get  - MO get function for session cpu usage
**
** History:
**	26-Oct-1992 (daveb)
**	    Add checks/logging in attach/detach routines.  Was getting
**	    segv's after re-attaching an SCB that had it's links zeroed
**	    out.
**	1-Dec-1992 (daveb)
**	    Use unsigned long out for SCB instance values, because they
**	    might be negative as pointer values.
**	13-Jan-1993 (daveb)
**	    Rename MO objects to include a .unix component.
**	18-jan-1993 (mikem)
**	    Initialized str to point to buf in the CS_scb_memory_get() routine.
**	    It was not initialized in the CS_MUTEX case.  
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      01-sep-93 (smc)
**          Changed MOulongout to MOptrout to be portable to axp_osf.
**	14-oct-93 (swm)
**	    Bug #56445
**	    Assign sem or cnd pointers to new cs_sync_obj (synchronisation
**	    object) rather than overloading cs_memory which is a i4;
**	    pointers do not fit into i4s on all machines!
**	    Also, now that semaphores are exclusive, remove unnecessary
**	    code that negates the sem address.
**	    Changed CS_scb_memory_get() to get cs_sync_obj field rather
**	    than cs_memory when cs_state field is CS_CNDWAIT or CS_MUTEX.
**	    Also, since semaphores are exclusive eliminate the test for a
**	    negated semaphore address.
**	16-oct-93 (swm)
**	    Bug #56445
**	    Changed name of CS_scb_memory_get() to CS_scb_sync_obj_get()
**	    to reflect the nature of information actually returned.
**	02-nov-93 (swm)
**	    Bug #56447
**	    A CS_SID is now guaranteed to be able to hold a PTR but can
**	    be bigger than a i4 (eg. axp_osf), so use MOptrout instead
**	    of MOlongout in CS_scb_self_get().
**	20-apr-94 (mikem)
**	    Added include of tr.h (module calls TRdisplay()).
**      10-Mar-1994 (daveb) 60514
**          Use right output function for SCBs and SIDs, MOptrget or MOpvget.
**  	    Also remove some unused objects ({as,rw}_q_{prev,next}),
**  	    and use MOuintget instead of MOintget for _mask fields.
**      23-Mar-1994 (daveb)
**          Make priority writable by SERVER admin, for experimental
**  	    purposes.
**	03-jun-1996 (canor01)
**	    Splay tree functions are not reentrant, so lock the global
**	    trees before calling them.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**	    use csmt version of this file.
**	14-apr-1997 (canor01)
**	    Ch. "exp.clf.unix.cs.scb_index" to pass back the 'thread id' 
**	    (cs_self) rather than addr of SCB, so it matches thread id's 
**	    from MOs such as lock and logging objects when using native 
**	    threads.
**	7-aug-97 (inkdo01)
**	    Removed preceding canor01 change. Tampering with the index
**	    class makes keyed retrieval strategies stop working for SCB classes.
**	16-Mar-2001 (thaju02)
**	    Removed reference to scb->cs_sm_root. (B104254)
**	13-Feb-98 (fanra01)
**	    Modified methods for index, attach and detach to use SID as
**	    instance for searching but still return the scb.
**	25-Sep-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats.
**	16-Nov-1998 (jenjo02)
**	    Added cs_dior, cs_diow, cs_bior, cs_biow, cs_lior, cs_liow stats,
**	    updated intepretation of CS_EVENT_WAIT state.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	17-Nov-1999 (jenjo02)
**	    Added exp.clf.unix.cs.scb_thread_id and CS_thread_id_get
**	    method to return OS thread id.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio, cs_dio from CS_SCB. Added methods to
**	    compute these values, also one for cs_lio.
**      13-Dec-2002 (devjo01) Bug 103257 (redux)
**          Increased buffer size in CS_scb_self_get.
**	07-Jul-2003 (devjo01)
**	    Use MOsidout() instead of MOptrout() in CS_scb_self_get().
**	10-Apr-2008 (smeke01) b120115
**	    Use CS_scb_self_get instead of MOpvget for scb_index.
**	09-Feb-2009 (smeke01) b119586 
**	    Added get function CS_scb_cpu_get.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

typedef struct
{
    i4	bit;
    char *str;
} MASK_BITS;



MO_INDEX_METHOD CS_scb_index;

MO_GET_METHOD CS_scb_self_get;
MO_GET_METHOD CS_scb_state_get;
MO_GET_METHOD CS_scb_sync_obj_get;
MO_GET_METHOD CS_scb_thread_type_get;
MO_GET_METHOD CS_scb_mask_get;
MO_GET_METHOD CS_thread_id_get;
MO_GET_METHOD CS_scb_bio_get;
MO_GET_METHOD CS_scb_dio_get;
MO_GET_METHOD CS_scb_lio_get;
MO_GET_METHOD CS_scb_cpu_get;

static char index_name[] = "exp.clf.unix.cs.scb_index";

GLOBALDEF MO_CLASS_DEF CS_scb_classes[] =
{
  { 0, index_name,
	0, MO_READ, index_name,
	0, CS_scb_self_get, MOnoset,
	0, CS_scb_index },

  /* special methods for special formatting */

  { 0, "exp.clf.unix.cs.scb_self",
	0, MO_READ, index_name,
	0, CS_scb_self_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_state",
	0, MO_READ, index_name,
	0, CS_scb_state_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_bio",
	0, MO_READ, index_name,
	0, CS_scb_bio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_dio",
	0, MO_READ, index_name,
	0, CS_scb_dio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_lio",
	0, MO_READ, index_name,
	0, CS_scb_lio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_state_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_state), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_state), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_memory",
	MO_SIZEOF_MEMBER(CS_SCB, cs_memory), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_memory), CS_scb_sync_obj_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_thread_type",
	0, MO_READ, index_name,
	0, CS_scb_thread_type_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_thread_type_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_thread_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_thread_type), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_mask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_mask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_asmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_asmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_asunmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_asunmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_thread_id",
	MO_SIZEOF_MEMBER(CS_SCB, cs_thread_id), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_thread_id), CS_thread_id_get, MOnoset,
	0, CS_scb_index },

  /* standard string methods */

  { 0, "exp.clf.unix.cs.scb_username",
	MO_SIZEOF_MEMBER(CS_SCB, cs_username), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_username), MOstrget, MOnoset,
	0, CS_scb_index },

  /* standard integer methods */

  { 0, "exp.clf.unix.cs.scb_length",
	MO_SIZEOF_MEMBER(CS_SCB, cs_length), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_length), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_type), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_owner",
	MO_SIZEOF_MEMBER(CS_SCB, cs_owner), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_owner), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_client_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_client_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_client_type), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_stk_area",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_area), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_area), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_stk_size",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_size ), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_size), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_priority),
	MO_READ|MO_SERVER_WRITE, index_name,
	CL_OFFSETOF(CS_SCB, cs_priority), MOintget, MOintset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_base_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_base_priority), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_base_priority), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_timeout",
	MO_SIZEOF_MEMBER(CS_SCB, cs_timeout), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_timeout), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_sem_count",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sem_count), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sem_count), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_sm_next",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sm_next), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sm_next), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_mode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_mode), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_nmode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_nmode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_nmode), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_uic",
	MO_SIZEOF_MEMBER(CS_SCB, cs_uic), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_uic), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_cputime",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cputime), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cputime), CS_scb_cpu_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_dior",
	MO_SIZEOF_MEMBER(CS_SCB, cs_dior), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_dior), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_diow",
	MO_SIZEOF_MEMBER(CS_SCB, cs_diow), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_diow), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_bior",
	MO_SIZEOF_MEMBER(CS_SCB, cs_bior), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_bior), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_biow",
	MO_SIZEOF_MEMBER(CS_SCB, cs_biow), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_biow), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_lior",
	MO_SIZEOF_MEMBER(CS_SCB, cs_lior), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_lior), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_liow",
	MO_SIZEOF_MEMBER(CS_SCB, cs_liow), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_liow), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_locks",
	MO_SIZEOF_MEMBER(CS_SCB, cs_locks), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_locks), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_lkevent",
	MO_SIZEOF_MEMBER(CS_SCB, cs_lkevent), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_lkevent), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_logs",
	MO_SIZEOF_MEMBER(CS_SCB, cs_logs), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_logs), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_lgevent",
	MO_SIZEOF_MEMBER(CS_SCB, cs_lgevent), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_lgevent), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_connect",
	MO_SIZEOF_MEMBER(CS_SCB, cs_connect), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_connect), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_svcb",
	MO_SIZEOF_MEMBER(CS_SCB, cs_svcb), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_svcb), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_ppid",
	MO_SIZEOF_MEMBER(CS_SCB, cs_ppid), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_ppid), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_inkernel",
	MO_SIZEOF_MEMBER(CS_SCB, cs_inkernel), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_inkernel), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_async",
	MO_SIZEOF_MEMBER(CS_SCB, cs_async), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_async), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_pid",
	MO_SIZEOF_MEMBER(CS_SCB, cs_pid), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_pid), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_ef_mask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_ef_mask), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_ef_mask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_cnd",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cnd), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cnd), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0 }
};


MASK_BITS mask_bits[] =
{
    { CS_TIMEOUT_MASK,		"TIMEOUT" },
    { CS_INTERRUPT_MASK,	"INTERRUPT" },
    { CS_BAD_STACK_MASK,	"BAD_STACK" },
    { CS_DEAD_MASK,		"DEAD" },
    { CS_IRPENDING_MASK,	"IRPENDING" },
    { CS_NOXACT_MASK,		"NOXACT" },
    { CS_MNTR_MASK,		"MNTR" },
    { CS_FATAL_MASK,		"FATAL" },
    { CS_MID_QUANTUM_MASK,	"MID_QUANTUM" },
    { CS_EDONE_MASK,		"EDONE_MASK" },
    { CS_IRCV_MASK,		"IRCV" },
    { CS_TO_MASK,		"TIMEOUT" },
    { 0, 0 }
};


/*{
** Name:	CS_scb_attach	- make an SCB known to MO/IMA
**
** Description:
**	Links the specified SCB into the known thread tree.  Logs
**	error to server log if it's already present (it shouldn't).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**	(OS-threads: yes, locks global tree mutex)
**
** Inputs:
**	scb		the thread to link in.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.	
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	13-Feb-98 (fanra01)
**	    Modified to use the SID as the key.
*/
void
CS_scb_attach( CS_SCB *scb )
{
    SPBLK node;
    SPBLK *sp;

# ifdef OS_THREADS_USED
    CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
    node.key = (PTR) scb->cs_self;
    sp = SPlookup( &node, Cs_srv_block.cs_scb_ptree );
    if( sp != NULL )
    {
	TRdisplay("CS_scb_attach: attempt to attach existing SCB %p!!!\n",
		  scb );
    }
    else
    {
        scb->cs_spblk.uplink = NULL;
	scb->cs_spblk.leftlink = NULL;
	scb->cs_spblk.rightlink = NULL;
	scb->cs_spblk.key = (PTR) scb->cs_self;
	SPinstall( &scb->cs_spblk, Cs_srv_block.cs_scb_ptree );
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
}



/*{
** Name:	CS_detach_scb	- make an SCB unknown to MO/IMA
**
** Description:
**	Unlinks the specified SCB from the known thread tree.  Best
**	called just before de-allocating the block.  Logs a message
**	to the server log with TRdisplay if the block isn't there
**	(it should be).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**
** Inputs:
**	scb		the thread to zap.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.	
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	13-Feb-98 (fanra01)
**	    Modified to use the SID as the key.
*/

void
CS_detach_scb( CS_SCB *scb )
{
    SPBLK node;
    SPBLK *sp;

# ifdef OS_THREADS_USED
    CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
    node.key = (PTR) scb->cs_self;
    sp = SPlookup( &node, Cs_srv_block.cs_scb_ptree );
    if( sp == NULL )
    {
	TRdisplay("CS_detach_scb: attempt to detach unknown SCB %p\n",
		  scb );
    }
    else
    {
	SPdelete( &scb->cs_spblk, Cs_srv_block.cs_scb_ptree );
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
}

/* ---------------------------------------------------------------- */

/*{
** Name:	CS_scb_index	- MO index functions for known CS SCB's
**
** Description:
**	Standard MO index function to get at the nodes in the known
**	scb tree.
**
** Re-entrancy:
**	NO; this may be a problem in real MP environments.  FIXME.
**
** Inputs:
**	msg		the index op to perform
**	cdata		the class data, ignored.
**	linstance	the length of the instance buffer.
**	instance	the input instance
**
** Outputs:
**	instance	on GETNEXT, updated to be the one found.
**	idata		updated to point to the SCB in question.
**
** Returns:
**	standard index function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	1-Dec-1992 (daveb)
**	    Use MOulongout, 'cause it might be "negative" as a
**  	    pointer.  
**	13-Feb-98 (fanra01)
**	    Modified to use SID as instance but returns the scb as the
**	    instance data to be passed to methods.  Incurs a lookup performance
**	    penalty.
*/

STATUS
CS_scb_index(i4 msg,
	     PTR cdata,
	     i4  linstance,
	     char *instance, 
	     PTR *instdata )
{
    STATUS stat = OK;
    PTR ptr;

# ifdef OS_THREADS_USED
    CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
    switch( msg )
    {
    case MO_GET:
	if( OK == (stat = CS_get_block( instance,
				       Cs_srv_block.cs_scb_ptree,
				       &ptr ) ) )
	    *instdata = (PTR) CS_find_scb((CS_SID) ptr);
	break;

    case MO_GETNEXT:
	if( OK == ( stat = CS_nxt_block( instance,
					Cs_srv_block.cs_scb_ptree,
					&ptr ) ) )
	{
	    *instdata = (PTR) CS_find_scb((CS_SID) ptr);
	    stat = MOptrout( MO_INSTANCE_TRUNCATED,
			      ptr,
			      linstance,
			      instance );
	}
	break;

    default:
	stat = MO_BAD_MSG;
	break;
    }
# ifdef OS_THREADS_USED
    CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
# endif /* OS_THREADS_USED */
    return( stat );
}

/* ---------------------------------------------------------------- */


/*{
** Name:	CS_scb_self_get	- MO get function for cs_self field
**
** Description:
**	A nice interpretation of the cs_self fields, special
**	casing CS_DONT_CARE and CS_ADDER_ID.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
**	02-nov-93 (swm)
**	    Bug #56447
**	    A CS_SID is now guaranteed to be able to hold a PTR but can
**	    be bigger than a i4 (eg. axp_osf), so use MOptrout instead
**	    of MOlongout.
**	27-Sep-2001 (inifa01)
**	    Buffer to small to hold resultant string on 64 bit platform.
**	    The maximum number of decimal digits required to display the value of
**	    an integer is defined as ( [<number of bits used to represent the number> / 3] + 1).
**	    Increased buffer size to 22.
**      13-Dec-2002 (devjo01) Bug 103257 (redux)
**          Increasing size to 22 is insufficient, since this does not
**          allow for a '\0' terminator.  Bumped 'buf' size to 24.
*/

STATUS
CS_scb_self_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 24 ];
    char *str = buf;
    CS_SCB *scb = (CS_SCB *)object;
    
    if( scb->cs_self == CS_DONT_CARE )
	str = "CS_DONT_CARE";
    else if( scb->cs_self == CS_ADDER_ID )
	str = "CS_ADDER_ID";
    else
	(void) MOsidout( 0, (PTR)scb->cs_self, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	CS_state_get	- MO get function for cs_state field
**
** Description:
**	A nice interpretation of the cs_state fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with nice state string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented. 
*/
STATUS
CS_scb_state_get(i4 offset,
		 i4  objsize,
		 PTR object,
		 i4  luserbuf,
		 char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;
    char *str;

    switch (scb->cs_state)
    {
    case CS_FREE:
	str = "CS_FREE";
	break;
    case CS_COMPUTABLE:
	str = "CS_COMPUTABLE";
	break;
    case CS_STACK_WAIT:
	str = "CS_STACK_WAIT";
	break;
    case CS_UWAIT:
	str = "CS_UWAIT";
	break;
    case CS_EVENT_WAIT:
	str = "CS_EVENT_WAIT";
	break;
    case CS_MUTEX:
	str = "CS_MUTEX";
	break;
    case CS_CNDWAIT:
	str = "CS_CNDWAIT";
	break;
	
    default:
	str = "<invalid>";
	break;
    }

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}



/*{
** Name:	CS_memory_get	- MO get function for cs_memory field
**
** Description:
**	A nice interpretation of the cs_memory field.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented. 
**	18-jan-1993 (mikem)
**	    Initialized str to point to buf in the CS_scb_memory_get() routine.
**	    It was not initialized in the CS_MUTEX case.  
**	14-oct-93 (swm)
**	    Bug #56445
**	    Assign sem or cnd pointers to new cs_sync_obj (synchronisation
**	    object) rather than overloading cs_memory which is a i4;
**	    pointers do not fit into i4s on all machines!
**	    Also, now that semaphores are exclusive, remove unnecessary
**	    code that negates the sem address.
**	    Changed CS_scb_memory_get() to get cs_sync_obj field rather
**	    than cs_memory when cs_state field is CS_CNDWAIT or CS_MUTEX.
**	    Also, since semaphores are exclusive eliminate the test for a
**	    negated semaphore address.
**	16-oct-93 (swm)
**	    Bug #56445
**	    Changed name of CS_scb_memory_get() to CS_scb_sync_obj_get()
**	    to reflect the nature of information actually returned.
**	16-Nov-1998 (jenjo02)
**	    Updated CS_EVENT_WAIT mask interpretations to coincide
**	    with current reality.
*/
STATUS
CS_scb_sync_obj_get(i4 offset,
		  i4  objsize,
		  PTR object,
		  i4  luserbuf,
		  char *userbuf)
{
    STATUS stat;
    char buf[ 80 ];
    char *str = buf;
    
    CS_SCB *scb = (CS_SCB *)object;

    switch (scb->cs_state)
    {
    case CS_EVENT_WAIT:

	str = scb->cs_memory & CS_DIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "DIOR" : "DIOW"
		:
	      scb->cs_memory & CS_BIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "BIOR" : "BIOW"
		:
	      scb->cs_memory & CS_LIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "LIOR" : "LIOW"
		:
	      scb->cs_memory & CS_LOCK_MASK ?
		  "LOCK"
		:
	      scb->cs_memory & CS_LOG_MASK ?
		  "LOG"
		:
	      scb->cs_memory & CS_LKEVENT_MASK ?
		  "LKEvnt"
		:
	      scb->cs_memory & CS_LGEVENT_MASK ?
		  "LGEvnt"
		:
	      scb->cs_memory & CS_TIMEOUT_MASK ?
		  "TIME"
		:
		  "OTHER";
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );
	break;

    case CS_MUTEX:

	buf[0] = 'x';
	buf[1] = ' ';
	(void) MOptrout( 0, scb->cs_sync_obj, sizeof(buf), &buf[2] );
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );
	break;

    case CS_CNDWAIT:
	stat =  MOptrout( MO_VALUE_TRUNCATED, scb->cs_sync_obj,
			   luserbuf, userbuf );

    default:

	stat =  MOulongout( MO_VALUE_TRUNCATED, scb->cs_memory,
			   luserbuf, userbuf );

	break;
    }
    return( stat );
}


/*{
** Name:	CS_thread_type_get	- MO get function for cs_thread_type field
**
** Description:
**	A nice interpretation of the cs_thread_type.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "internal", "user",
**			or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
*/

STATUS
CS_scb_thread_type_get(i4 offset,
		       i4  objsize,
		       PTR object,
		       i4  luserbuf,
		       char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;
    char buf[ 80 ];
    char *str = buf;

    if( scb->cs_thread_type == CS_INTRNL_THREAD )
	str = "internal";
    else if( scb->cs_thread_type == CS_USER_THREAD )
	str = "user";
    else
	(void) MOlongout( 0, scb->cs_thread_type, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	CS_scb_mask_get	- MO get function for cs_ mask fields.
**
** Description:
**	A nice interpretation of mask fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
*/

STATUS
CS_scb_mask_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 80 ];
    i4 mask = *(i4 *)((char *)object + offset);
    MASK_BITS *mp;
    char *p = buf;
    char *q;

    for( mp = mask_bits; mp->str != NULL; mp++ )
    {
	if( mask & mp->bit )
	{
	    if( p != buf )
		*p++ = ',';
	    for( q = mp->str; *p++ = *q++ ; )
		continue;
	}
    }
    *p = EOS;
    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ));
}
/*{
** Name:	CS_thread_id_get    - MO get function for cs_thread_id field
**
** Description:
**	Returns actual OS thread id when OS_THREADS_USED,
**	zero otherwise.
**
**	Written as a get function to provide for cs_thread_id's 
**	that may be structures.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_thread_id field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		Decimal value of cs_thread_id.
**
** Returns:
**	standard get function returns.
**
** History:
**	17-Nov-1999 (jenjo02)
**	    Added.
*/

STATUS
CS_thread_id_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 20 ];
    char *str = buf;
    CS_SCB *scb = (CS_SCB *)object;

    /*
    ** If CS_THREAD_ID is a structure, this will need to be
    ** modified to fit!
    */
    (void) MOlongout( 0, *(i8*)&scb->cs_thread_id, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}

/*{
** Name:	CS_scb_bio_get 	-- MO get methods for sum of BIO
**				     read/writes.
**
** Description:
**	Returns the sum of BIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_bio_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_bior + scb->cs_biow),
		luserbuf, userbuf));
}

/*{
** Name:	CS_scb_dio_get 	-- MO get methods for sum of DIO
**				     read/writes.
**
** Description:
**	Returns the sum of DIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_dio_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_dior + scb->cs_diow),
		luserbuf, userbuf));
}

/*{
** Name:	CS_scb_lio_get 	-- MO get methods for sum of LIO
**				     read/writes.
**
** Description:
**	Returns the sum of LIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_lio_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_lior + scb->cs_liow),
		luserbuf, userbuf));
}

/*{
** Name:	CS_scb_cpu_get	- MO get function for session_cpu field.
**
** Description:
**	Obtain the cpu time this thread has used from the OS.
**	On some platforms this is not possible, so for these 
**	we return zero.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		The total amount of cpu time in milliseconds
**			used by this thread.
**
** Returns:
**	Standard get function returns. Note that the aim is NOT to 
**	return an error if at all possible. This is because returning an 
**	error in IMA for even just one value in one row causes no data 
**	to be returned for ANY rows in the query, with no error message 
**	to the front end client. A reasonable alternative is to return 
**	zero as the value. 
**
** History:
**	09-Feb-2009 (smeke01) b119586 
**	    Created.  
*/
STATUS
CS_scb_cpu_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
    i4		totalcpu = 0;
    CS_SCB	*pSCB = (CS_SCB *) object;
    CS_THREAD_STATS_CB thread_stats_cb;

#if defined (OS_THREADS_USED) && (defined(LNX) || defined(sparc_sol))
    thread_stats_cb.cs_thread_id = pSCB->cs_thread_id; 
    thread_stats_cb.cs_os_pid = pSCB->cs_os_pid;
    thread_stats_cb.cs_os_tid = pSCB->cs_os_tid;
    thread_stats_cb.cs_stats_flag = CS_THREAD_STATS_CPU; 
    if (CSMT_get_thread_stats(&thread_stats_cb) == OK)
	totalcpu = thread_stats_cb.cs_usercpu + thread_stats_cb.cs_systemcpu;
#else
    totalcpu = pSCB->cs_cputime;
#endif
    return (MOlongout(MO_VALUE_TRUNCATED, totalcpu, luserbuf, userbuf));
}
