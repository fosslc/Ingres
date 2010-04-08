/*
**  Copyright (c) 1995, 2000 Ingres Corporation.
*/

/*
**  Name: classint.h
**
**  Description:
**	This file contains the MO definitions of the CS internal classes.
**
**  History:
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	23-jan-2004 (somsa01)
**	    Re-defined MO methods for num_active, and added hwm_active.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
*/

GLOBALDEF MO_CLASS_DEF CS_int_classes[] =
{
# if 0
  { 0, "exp.clf.nt.cs.debug", sizeof(CSdebug),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&CSdebug, MOcdata_index },
# endif

  { 0, "exp.clf.nt.cs.lastquant", sizeof(Cs_lastquant),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&Cs_lastquant, MOcdata_index },

  { 0, "exp.clf.nt.cs.dio_resumes", sizeof(dio_resumes),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&dio_resumes, MOcdata_index },

  { 0, "exp.clf.nt.cs.num_processors", sizeof(Cs_numprocessors),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_numprocessors, MOcdata_index },

  { 0, "exp.clf.nt.cs.max_sem_loops", sizeof(Cs_max_sem_loops),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_max_sem_loops, MOcdata_index },

  { 0, "exp.clf.nt.cs.desched_usec_sleep", sizeof(Cs_desched_usec_sleep),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_desched_usec_sleep, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.scballoc",
	sizeof( Cs_srv_block.cs_scballoc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scballoc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.scbdealloc",
	sizeof( Cs_srv_block.cs_scbdealloc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scbdealloc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.elog",
	sizeof( Cs_srv_block.cs_elog ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_elog ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.process",
	sizeof( Cs_srv_block.cs_process ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_process ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.attn",
	sizeof( Cs_srv_block.cs_attn ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_attn ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.startup",
	sizeof( Cs_srv_block.cs_startup ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_startup ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.shutdown",
	sizeof( Cs_srv_block.cs_shutdown ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_shutdown ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.format",
	sizeof( Cs_srv_block.cs_format ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_format ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.saddr",
	sizeof( Cs_srv_block.cs_saddr ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_saddr ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.reject",
	sizeof( Cs_srv_block.cs_reject ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_reject ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.disconnect",
	sizeof( Cs_srv_block.cs_disconnect ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_disconnect ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.read",
	sizeof( Cs_srv_block.cs_read ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_read ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.write",
	sizeof( Cs_srv_block.cs_write ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_write ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.max_sessions",
	sizeof( Cs_srv_block.cs_max_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.num_sessions",
	sizeof( Cs_srv_block.cs_num_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_num_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.user_sessions",
	sizeof( Cs_srv_block.cs_user_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_user_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.max_active",
	sizeof( Cs_srv_block.cs_max_active ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_active ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

# if 0
  { 0, "exp.clf.nt.cs.srv_block.multiple",
	sizeof( Cs_srv_block.cs_multiple ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_multiple ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
# endif

  { 0, "exp.clf.nt.cs.srv_block.cursors",
	sizeof( Cs_srv_block.cs_cursors ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cursors ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.num_active",
	0, MO_READ, 0,
	0, CSmo_num_active_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.hwm_active",
	0, MO_READ, 0,
	0, CSmo_hwm_active_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.stksize",
	sizeof( Cs_srv_block.cs_stksize ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stksize ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.stk_count",
	sizeof( Cs_srv_block.cs_stk_count ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.ready_mask",
	sizeof( Cs_srv_block.cs_ready_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_ready_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.known_list",
	sizeof( Cs_srv_block.cs_known_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_known_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wt_list",
	sizeof( Cs_srv_block.cs_wt_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_wt_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.to_list",
	sizeof( Cs_srv_block.cs_to_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_to_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.current",
	sizeof( Cs_srv_block.cs_current ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_current ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.next_id",
	sizeof( Cs_srv_block.cs_next_id ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_next_id ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.error_code",
	sizeof( Cs_srv_block.cs_error_code ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_error_code ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.state",
	sizeof( Cs_srv_block.cs_state ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_state ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.mask",
	sizeof( Cs_srv_block.cs_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.aquantum",
	sizeof( Cs_srv_block.cs_aquantum ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.aq_length",
	sizeof( Cs_srv_block.cs_aq_length ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.bquantum0",
	sizeof( Cs_srv_block.cs_bquantum[0] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[0] ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.bquantum1",
	sizeof( Cs_srv_block.cs_bquantum[1] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[1] ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.toq_cnt",
	sizeof( Cs_srv_block.cs_toq_cnt ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_toq_cnt ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.q_per_sec",
	sizeof( Cs_srv_block.cs_q_per_sec ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_q_per_sec ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.quantums",
	sizeof( Cs_srv_block.cs_quantums ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_quantums ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.idle_time",
	sizeof( Cs_srv_block.cs_idle_time ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_idle_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.cpu",
	sizeof( Cs_srv_block.cs_cpu ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cpu ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.svcb",
	sizeof( Cs_srv_block.cs_svcb ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_svcb ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.stk_list",
	sizeof( Cs_srv_block.cs_stk_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.gca_name",
	sizeof( Cs_srv_block.cs_name ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_name ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.pid",
	sizeof( Cs_srv_block.cs_pid ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_pid ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.event_mask",
	sizeof( Cs_srv_block.cs_event_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_event_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smssx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smssx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smssx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smsxx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smsxx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsxx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smsx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smsx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smss_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smss_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smss_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smmsx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmsx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmsx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smmxx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmxx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmxx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.smmx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.sem_stats.cs_smms_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smms_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smms_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bio_time",
	0, MO_READ, 0,
	0, CSmo_bio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bio_waits",
	0, MO_READ, 0,
	0, CSmo_bio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bio_done",
	0, MO_READ, 0,
	0, CSmo_bio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.bior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.biow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.biow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.biow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dio_time",
	0, MO_READ, 0,
	0, CSmo_dio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dio_waits",
	0, MO_READ, 0,
	0, CSmo_dio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dio_done",
	0, MO_READ, 0,
	0, CSmo_dio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.dior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.diow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.diow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.diow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lio_time",
	0, MO_READ, 0,
	0, CSmo_lio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lio_waits",
	0, MO_READ, 0,
	0, CSmo_lio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lio_done",
	0, MO_READ, 0,
	0, CSmo_lio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.liow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.liow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.liow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lg_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lg_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lg_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lg_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lge_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lge_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lge_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lk_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lk_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lk_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lk_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lke_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lke_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.lke_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.tm_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.tm_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.tm_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.nt.cs.srv_block.wait_stats.tm_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

# if 0
  { 0, "exp.clf.nt.cs.srv_block.optcnt",
	sizeof( Cs_srv_block.cs_optcnt ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_optcnt ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
# endif

# if 0
  /* Need CS_options_idx and MOdouble_get methods */

  { 0, "exp.clf.nt.cs.srv_block.options.index",
	sizeof( Cs_srv_block.cs_option.cso_index ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_index ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.nt.cs.srv_block.options.value",
	sizeof( Cs_srv_block.cs_option.cso_value ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_value ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.nt.cs.srv_block.options.facility",
	sizeof( Cs_srv_block.cs_option.cso_facility ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_facility ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.nt.cs.srv_block.options.strvalue",
	sizeof( Cs_srv_block.cs_option.cso_strvalue ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_strvalue ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.nt.cs.srv_block.options.float",
	sizeof( Cs_srv_block.cs_option.cso_float ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_float ),
	MOdouble_get, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },
# endif
  { 0 }
};
