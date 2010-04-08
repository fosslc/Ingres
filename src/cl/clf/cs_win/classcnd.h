/*
**      12-Dec-95 (fanra01)
**          Extracted data for NT DLL build.  Static index_name is now defined
**          multiply in the data file hence undef of index_name.
**	22-jan-2004 (somsa01)
**	    Deleted cnd_waiters, no longer used. Also enabled cnd_next,
**	    cnd_prev, cnd_waiter.
*/
MO_INDEX_METHOD CS_cnd_index;
static char     mocnd_index_name[] = "exp.clf.nt.cs.cnd_index";
# undef         index_name
# define        index_name      mocnd_index_name

GLOBALDEF MO_CLASS_DEF CS_cnd_classes[] =
{
	{0, index_name, 0, MO_READ, index_name, 0,
	MOuivget, MOnoset, NULL, CS_cnd_index},

	{0, "exp.clf.nt.cs.cnd_next",
		MO_SIZEOF_MEMBER(CS_CONDITION, cnd_next),
		MO_READ, index_name,
		CL_OFFSETOF(CS_CONDITION, cnd_next),
	MOptrget, MOnoset, NULL, CS_cnd_index},

	{0, "exp.clf.nt.cs.cnd_prev",
		MO_SIZEOF_MEMBER(CS_CONDITION, cnd_prev),
		MO_READ, index_name,
		CL_OFFSETOF(CS_CONDITION, cnd_prev),
	MOptrget, MOnoset, NULL, CS_cnd_index},

	{0, "exp.clf.nt.cs.cnd_waiter",
		MO_SIZEOF_MEMBER(CS_CONDITION, cnd_waiter),
		MO_READ, index_name,
		CL_OFFSETOF(CS_CONDITION, cnd_waiter),
	MOptrget, MOnoset, NULL, CS_cnd_index},

	{0, "exp.clf.nt.cs.cnd_name",
		MO_SIZEOF_MEMBER(CS_CONDITION, cnd_name),
		MO_READ, index_name,
		CL_OFFSETOF(CS_CONDITION, cnd_name),
	MOstrpget, MOnoset, NULL, CS_cnd_index},

	{0}
};
