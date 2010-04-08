/*
**  Copyright (c) 1995, 2003 Ingres Corporation
*/

/*
** History:
**      02-Jun-1995 (jenjo02)
**          Removed references to cs_cnt_sem, cs_long_cnt, cs_ms_long_et,
**          cs_list, all of which are obsolete and due to be scrunched
**          out of the structure.
**	30-sep-1996 (canor01)
**	    Updated references for cs.sem_stats for changes in this
**	    structure.
**	26-feb-1997 (canor01)
**	    Made cs_mo_sems and index_name globals and moved them
**	    to csdata.c to avoid linker warnings.
**	24-jul-2000 (somsa01 for jenjo02)
**	    ifdef'd out all the code with USE_MO_WITH_SEMS. It's been
**	    a long time since MO has been used with semaphores, so this
**	    modules really isn't needed any more.
**	13-oct-2003 (somsa01)
**	    Use MOsidget to retreive session ID values.
*/

#ifdef USE_MO_WITH_SEMS

GLOBALDEF char CSsem_index_name[] = "exp.clf.nt.cs.sem_index";

GLOBALDEF MO_CLASS_DEF CS_sem_classes[] =
{
  { 0, "exp.clf.nt.cs.do_mo_sems", sizeof(IIcs_mo_sems),
	MO_READ|MO_SERVER_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&IIcs_mo_sems, MOcdata_index },

    /* this is really attached, and uses the default index method */

  { MO_INDEX_CLASSID|MO_CDATA_INDEX, CSsem_index_name,
	0, MO_READ, 0,
	0, MOstrget, MOnoset, 0, MOname_index },

   /* these have local methods for formatting */

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_addr", 
	0, MO_READ, CSsem_index_name,
	0, MOpvget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_type", 
	0, MO_READ, CSsem_index_name,
	0, CS_sem_type_get, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_scribble_check", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_scribble_check),
	MO_READ, CSsem_index_name, 0,
	CS_sem_scribble_check_get, MOnoset, 0, MOidata_index },
	
   /* These are all builtin methods */

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_value",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_value), MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_value), MOintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_count", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_count), MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_count), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_owner", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_owner), MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_owner), MOsidget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_pid), MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_name", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_name),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_name), CS_sem_name_get, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_init_pid", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_pid),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_pid), MOuintget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_sid",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sid), MO_READ, index_class,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sid), MOsidget, MOnoset,
	0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_stats.smsx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smsx_count),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smsx_count),
	MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_stats.smxx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smxx_count),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smxx_count),
	MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_stats.smx_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_smx_count),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_smx_count),
	MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_stats.sms_count",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_sms_count),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_sms_count),
	MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_stats.sms_hwm",
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_smstatistics.cs_sms_hwm),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_smstatistics.cs_sms_hwm),
	MOuintget, MOnoset, 0, MOidata_index },

  { MO_CDATA_INDEX, "exp.clf.nt.cs.sem_init_addr", 
	MO_SIZEOF_MEMBER(CS_SEMAPHORE, cs_sem_init_addr),
	MO_READ, CSsem_index_name,
	CL_OFFSETOF(CS_SEMAPHORE, cs_sem_init_addr), MOptrget, MOnoset,
	0, MOidata_index },

	{0}
};

#endif /* USE_MO_WITH_SEMS */
