/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <tm.h>
#include    <dmftm.h>

/**
**
**  Name: DMFTMMO.C - MO interface to os provided statistics.
**
**  Description:
**	This module provides the MO interface to getting statistics provided
**	by the TMperfstat() function.  For information about how to register
**	a table to get this information see "tmregstr.sql".
**
**	external functions:
**          template_proc() - a short comment about template_proc
**
**	internal functions:
** 	    TMmo_stat_index()      - Routine called to get perf status info.
** 	    TMmo_collect()   	   - Make performance stats available through 
**				     updating a MO "control" object.
**
**
**  History:
**      26-jul-93 (mikem)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**/


/*
** Definition of all global variables owned by this file.
*/



/*
**  Definition of static variables and forward static functions.
*/
static TM_PERFSTAT	Tmstat;

static STATUS TMmo_collect( 
    i4  offset, 
    i4  lsbuf, 
    char *sbuf, 
    i4  size, 
    PTR object);

static STATUS TMmo_stat_index(
    i4      msg,
    PTR     cdata,
    i4      linstance,
    char    *instance,
    PTR     *instdata);

static MO_CLASS_DEF TMmo_tmperf_classes[] = 
{
    {0, "exp.dmf.perf.tmperf_collect", 
	0, MO_READ|MO_SERVER_WRITE, 0, 
	0, 
	MOzeroget, TMmo_collect, (PTR)0, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_utime.TM_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_utime.TM_secs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_utime.TM_secs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_utime.TM_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_utime.TM_msecs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_utime.TM_msecs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_stime.TM_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_stime.TM_secs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_stime.TM_secs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_stime.TM_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_stime.TM_msecs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_stime.TM_msecs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_cpu.TM_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_cpu.TM_secs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_cpu.TM_secs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_cpu.TM_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_cpu.TM_msecs), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_cpu.TM_msecs),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_maxrss", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_maxrss), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_maxrss),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_idrss", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_idrss), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_idrss),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_minflt", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_minflt), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_minflt),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_majflt", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_majflt), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_majflt),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_nswap", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nswap), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nswap),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_reads", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_reads), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_reads),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_writes", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_writes), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_writes),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_dio", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_dio), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_dio),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_msgsnd", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgsnd), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgsnd),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_msgrcv", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgrcv), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgrcv),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_msgtotal", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgtotal), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgtotal),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_nsignals", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nsignals), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nsignals),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_nvcsw", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nvcsw), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nvcsw),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0, "exp.dmf.perf.tmperf_nivcsw", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nivcsw), MO_READ, 0,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nivcsw),
	MOintget, MOnoset, (PTR)&Tmstat, MOcdata_index
    },
    {0}
};



/*{
** Name: TMmo_stat_index()	- Routine called to get perf status info.
**
** Description:
**	Index routine for the MO interface to operating system statistics.
**	The gateway interface to this information is a table with one row and
**	an i4 column for each statistic.  This routine initializes the data
**	on the first call and then ends the search after one row is returned.
**
** Inputs:
**	msg				indicates first or next operation.
**					    MO_GET - first call.
**					    MO_NEXT - subsequent calls.
**	cdata				unused - required by MO interface.
**      linstance			unused - required by MO interface.
**	instance			unused - required by MO interface.
**
** Outputs:
**	instdata			Is set to static data area containing
**					performance statistics, by first call
**					to TMmo_stat_index() with the MO_GET
**					msg.
**					
**
**	Returns:
**	    OK 			- success
**	    MO_NO_INSTANCE	- When called for 2nd row, tells MO to stop 
**				  getting rows.
**	    MO_BAD_MSG		- If something other than MO_GET or MO_NEXT 
**				  is passed in msg.
**
** History:
**      26-jul-93 (mikem)
**         Created.
*/
/* ARGSUSED */
static STATUS TMmo_stat_index(
    i4      msg,
    PTR     cdata,
    i4      linstance,
    char    *instance,
    PTR     *instdata)
{
    STATUS	status;
    CL_ERR_DESC	sys_err;

    switch (msg)
    {
	case MO_GET:
	{
	    /* first call */
	    TMperfstat(&Tmstat, &sys_err);

	    *instdata = (PTR) &Tmstat;
	    status = OK;

	    break;
	}

	case MO_GETNEXT:
	{
	    /* one row table - stop after one row */
	    status = MO_NO_INSTANCE;

	    break;
	}

	default:
	    status = MO_BAD_MSG;
	    break;

    }

    return(status);
}

/*{
** Name: TMmo_collect()	- Collect statistics.
**
** Description:
**	Is called once by updating the exp.dmf.perf.tmperf_collect control
**	object.  It initializes the static memory area with a new copy of
**	statistics, subsequent MO calls will read this area.
**
** Inputs:
**	offset				unused - required by MO interface. 
**	lsbuf				unused - required by MO interface. 
**	sbuf				unused - required by MO interface. 
**	size				unused - required by MO interface. 
**	object				unused - required by MO interface. 
**
** Outputs:
**	none.
**
**	Returns:
**	    OK.
** History:
**      26-jul-93 (mikem)
**          Created.
*/
/* ARGSUSED */
static STATUS TMmo_collect( 
    i4  offset, 
    i4  lsbuf, 
    char *sbuf, 
    i4  size, 
    PTR object)
{
    CL_ERR_DESC	sys_err;

    (void) TMperfstat(&Tmstat, &sys_err);

    return(OK);
}

/*{
** Name: TMmo_attach_tmperf()	- Make performance stats available through MO.
**
** Description:
**	"Registers" the performance stat interface with MO.  Must be called
**	before any register 
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    status as is returned from MOclassdef().
**
** History:
**      26-jul-93 (mikem)
**          Created.
*/
STATUS
dmf_tmmo_attach_tmperf(void)
{
    CL_ERR_DESC	sys_err;
    STATUS	status;

    /* initialize the stat structure before anyone can use it */
    (void) TMperfstat(&Tmstat, &sys_err);

    status = MOclassdef(MAXI2, TMmo_tmperf_classes);

    return(status);
}
