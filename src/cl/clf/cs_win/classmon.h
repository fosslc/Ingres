GLOBALDEF MO_CLASS_DEF CS_mon_classes[] =
{
	{0, "exp.clf.nt.cs.mon.breakpoint", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_breakpoint_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.stopcond", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_stopcond_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.stopserver", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_stopserver_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.crashserver", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_crashserver_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.shutserver", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_shutserver_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.rm_session", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_rm_session_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.suspend_session", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_suspend_session_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.resume_session", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_resume_session_set,
	0, MOcdata_index},

	{0, "exp.clf.nt.cs.mon.debug", 0,
		MO_READ | MO_SERVER_WRITE, 0, 0,
		MOzeroget, CS_debug_set,
	0, MOcdata_index},

	{0}
};
