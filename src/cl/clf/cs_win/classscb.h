/*
**  Copyright (c) 1995, 2000 Ingres Corporation.
*/

/*
**  Name: classscb.h
**
**  Description:
**	Contains the SCB-specific MO routine definitions.
**
**  History:
**      12-Dec-95 (fanra01)
**          Extracted data for NT DLL build.  Static index_name is now defined
**          multiply in the data file hence undef of index_name.
**	14-apr-1997 (canor01)
**	    Ch. "exp.clf.nt.cs.scb_index" to pass back the 'thread id'
**	    (cs_self) rather than addr of SCB, so it matches thread id's
**	    from MOs such as lock and logging objects when using native
**	    threads.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Added exp.clf.unix.cs.scb_thread_id and CS_thread_id_get
**	    method to return OS thread id.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio, cs_dio from CS_SCB. Added methods to
**	    compute these values, also one for cs_lio.
*/
static char	moscb_index_name[] = "exp.clf.nt.cs.scb_index";
# undef         index_name
# define        index_name     moscb_index_name

GLOBALDEF MO_CLASS_DEF CS_scb_classes[] =
{
  { 0, index_name,
	0, MO_READ, index_name,
	0, CS_scb_self_get, MOnoset,
	0, CS_scb_index },

  /* special methods for special formatting */

  { 0, "exp.clf.nt.cs.scb_self",
	0, MO_READ, index_name,
	0, CS_scb_self_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_state",
	0, MO_READ, index_name,
	0, CS_scb_state_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_state_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_state), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_state), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_memory",
	MO_SIZEOF_MEMBER(CS_SCB, cs_memory), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_memory), CS_scb_sync_obj_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_thread_type",
	0, MO_READ, index_name,
	0, CS_scb_thread_type_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_thread_type_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_thread_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_thread_type), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_mask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_mask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), MOuintget, MOnoset,
	0, CS_scb_index },
/*
  { 0, "exp.clf.nt.cs.scb_asmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_asmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), MOuintget, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_asunmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_asunmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), MOuintget, MOnoset,
	0, CS_scb_index }, */

  { 0, "exp.clf.nt.cs.scb_thread_id",
	MO_SIZEOF_MEMBER(CS_SCB, cs_thread_id), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_thread_id), CS_thread_id_get, MOnoset,
	0, CS_scb_index },

  /* standard string methods */

  { 0, "exp.clf.nt.cs.scb_username",
	MO_SIZEOF_MEMBER(CS_SCB, cs_username), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_username), MOstrget, MOnoset,
	0, CS_scb_index },

  /* standard integer methods */

  { 0, "exp.clf.nt.cs.scb_length",
	MO_SIZEOF_MEMBER(CS_SCB, cs_length), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_length), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_type), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_owner",
	MO_SIZEOF_MEMBER(CS_SCB, cs_owner), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_owner), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_client_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_client_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_client_type), MOintget, MOnoset,
	0, CS_scb_index },

/*  { 0, "exp.clf.nt.cs.scb_stk_area",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_area), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_area), MOpvget, MOnoset,
	0, CS_scb_index },*/

  { 0, "exp.clf.nt.cs.scb_stk_size",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_size ), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_size), MOintget, MOnoset,
	0, CS_scb_index },
/*
  { 0, "exp.clf.nt.cs.scb_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_priority),
	MO_READ|MO_SERVER_WRITE, index_name,
	CL_OFFSETOF(CS_SCB, cs_priority), MOintget, MOintset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_base_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_base_priority), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_base_priority), MOintget, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_timeout",
	MO_SIZEOF_MEMBER(CS_SCB, cs_timeout), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_timeout), MOintget, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_sem_count",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sem_count), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sem_count), MOintget, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_sm_root",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sm_root), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sm_root), MOpvget, MOnoset,
	0, CS_scb_index }, */
/*
  { 0, "exp.clf.nt.cs.scb_sm_next",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sm_next), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sm_next), MOpvget, MOnoset,
	0, CS_scb_index }, */

  { 0, "exp.clf.nt.cs.scb_mode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_mode), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_nmode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_nmode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_nmode), MOintget, MOnoset,
	0, CS_scb_index },
/*
  { 0, "exp.clf.nt.cs.scb_uic",
	MO_SIZEOF_MEMBER(CS_SCB, cs_uic), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_uic), MOintget, MOnoset,
	0, CS_scb_index }, */

  { 0, "exp.clf.nt.cs.scb_cputime",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cputime), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cputime), CS_scb_cpu_get, MOnoset,
	0, CS_scb_index }, 

  { 0, "exp.clf.nt.cs.scb_bio",
	0, MO_READ, index_name,
	0, CS_scb_bio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_dio",
	0, MO_READ, index_name,
	0, CS_scb_dio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_lio",
	0, MO_READ, index_name,
	0, CS_scb_lio_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_locks",
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

  { 0, "exp.clf.nt.cs.scb_connect",
	MO_SIZEOF_MEMBER(CS_SCB, cs_connect), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_connect), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_svcb",
	MO_SIZEOF_MEMBER(CS_SCB, cs_svcb), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_svcb), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.nt.cs.scb_ppid",
	0, MO_READ, index_name,
	0, MOzeroget, MOnoset,
	(PTR)0, CS_scb_index }, 

  { 0, "exp.clf.nt.cs.scb_inkernel",
	MO_SIZEOF_MEMBER(CS_SCB, cs_inkernel), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_inkernel), CS_scb_inkernel_get, MOnoset,
	0, CS_scb_index },
/*
  { 0, "exp.clf.nt.cs.scb_async",
	MO_SIZEOF_MEMBER(CS_SCB, cs_async), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_async), MOintget, MOnoset,
	0, CS_scb_index }, */

  { 0, "exp.clf.nt.cs.scb_pid",
	MO_SIZEOF_MEMBER(CS_SCB, cs_pid), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_pid), MOintget, MOnoset,
	0, CS_scb_index },
/*
  { 0, "exp.clf.nt.cs.scb_ef_mask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_ef_mask), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_ef_mask), MOuintget, MOnoset,
	0, CS_scb_index }, */

  { 0, "exp.clf.nt.cs.scb_cnd",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cnd), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cnd), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0 }
};
