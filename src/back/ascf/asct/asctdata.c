/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: asctdata.c
**
** Description:
**
**      Provides the global data for the asct facility.
**
** History:
**      01-Mar-1999 (fanra01)
**          Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPICELIBDATA
**	    in the Jamfile.	
*/

/*
**
LIBRARY = IMPICELIBDATA
**
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <er.h>

# include <mo.h>
# include <iicommon.h>
# include <ddfcom.h>

# include <asct.h>

GLOBALDEF   char*   asctlogfile = NULL;
GLOBALDEF   ASCTFR  asctentry ZERO_FILL;
GLOBALDEF   ASCTFL  asctfile ZERO_FILL;
GLOBALDEF   PASCTBF asctbuf = NULL;
GLOBALDEF   i4      asctlogmsgsize = 0;

GLOBALDEF   i4    asctlogsize   = 0;
GLOBALDEF   i4    asctthreshold = 0;
GLOBALDEF   i4    asctlogtimeout= 0;
GLOBALDEF   i4    ascttracemask = 0;
GLOBALDEF   i4    asctsavemask  = 0;

MO_SET_METHOD asct_start_set;
MO_SET_METHOD asct_suspend_set;
MO_SET_METHOD asct_flush_set;
MO_SET_METHOD asct_logfile_set;
MO_SET_METHOD asct_logsize_set;
MO_SET_METHOD asct_logmsgsize_set;
MO_SET_METHOD asct_threshold_set;

MO_GET_METHOD asctfl_state_get;
MO_GET_METHOD asct_tracemask_get;

GLOBALDEF MO_CLASS_DEF asct_class[] =
{

    { 0, "exp.ascf.asct.thread", MO_SIZEOF_MEMBER(ASCTFL, self),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, self),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.entry", MO_SIZEOF_MEMBER(ASCTFR, freebuf),
      MO_READ, 0, CL_OFFSETOF(ASCTFR, freebuf),
      MOintget, MOnoset, (PTR)&asctentry, MOcdata_index },

    { 0, "exp.ascf.asct.state", MO_SIZEOF_MEMBER(ASCTFL, state),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, state),
      asctfl_state_get, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.start", MO_SIZEOF_MEMBER(ASCTFL, start),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, start),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.end", MO_SIZEOF_MEMBER(ASCTFL, end),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, end),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.maxflush", MO_SIZEOF_MEMBER(ASCTFL, maxflush),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, maxflush),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.trigger", MO_SIZEOF_MEMBER(ASCTFL, trigger),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, trigger),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.startp", MO_SIZEOF_MEMBER(ASCTFL, startp),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, startp),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.endp", MO_SIZEOF_MEMBER(ASCTFL, endp),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, endp),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.threshold", MO_SIZEOF_MEMBER(ASCTFL, threshold),
      MO_READ, 0, CL_OFFSETOF(ASCTFL, threshold),
      MOintget, MOnoset, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.logfile", sizeof(asctfile),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOstrpget, asct_logfile_set, (PTR)&asctlogfile, MOcdata_index },

    { 0, "exp.ascf.asct.tracemask", sizeof(ascttracemask),
      MO_READ|MO_SERVER_WRITE, 0,
      0, asct_tracemask_get, MOintset, (PTR)&ascttracemask, MOcdata_index },

    { 0, "exp.ascf.asct.trcstart", sizeof(asctfile),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOzeroget, asct_start_set, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.trcsuspend", sizeof(asctfile),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOzeroget, asct_suspend_set, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.trcflush", sizeof(asctfile),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOzeroget, asct_flush_set, (PTR)&asctfile, MOcdata_index },

    { 0, "exp.ascf.asct.trclogsize", sizeof(asctlogsize),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOintget, asct_logsize_set, (PTR)&asctlogsize, MOcdata_index },

    { 0, "exp.ascf.asct.trcmsgsize", sizeof(asctlogmsgsize),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOintget, asct_logmsgsize_set, (PTR)&asctlogmsgsize, MOcdata_index },

    { 0, "exp.ascf.asct.trcthreshold", sizeof(asctthreshold),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOintget, asct_threshold_set, (PTR)&asctthreshold, MOcdata_index },

    { 0, "exp.ascf.asct.trctimeout", sizeof(asctlogtimeout),
      MO_READ|MO_SERVER_WRITE, 0,
      0, MOintget, MOintset, (PTR)&asctlogtimeout, MOcdata_index },

    { 0 }
};
