/*
** Copyright (C) 1995, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <mo.h>
#include    <tm.h>

/**
**
**  Name: DIMO.C - Provide "MO" support for information about slaves.
**
**  Description:
**	Provides "MO" support for performance information about di slaves.
**
**          template_proc() - a short comment about template_proc
**
**
**  History:
**	30-aug-1995 (shero03)
**	    Created from unix.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**/

/*
**  Forward and/or External typedef/struct references.
*/
GLOBALREF TM_PERFSTAT	Di_moinfo;
GLOBALREF i4 		Di_moinited;

/*
**  Definition of forward static functions.
*/
static STATUS DImo_collect(
    i4  offset,
    i4  lsbuf,
    char *sbuf,
    i4  size,
    PTR object);

static STATUS DImo_stat_index(
    i4      msg,
    PTR     cdata,
    i4      linstance,
    char    *instance,
    PTR     *instdata);

static i4 DImo_array_index_get(
    i4  offset, 
    i4  size, 
    PTR object, 
    i4  lsbuf, 
    char *sbuf);


/*
**  Definition of static variables.
*/

static MO_CLASS_DEF DImo_tmctl_classes[] = 
{
    {0, "exp.clf.di.dimo_collect", 
	0, MO_READ|MO_WRITE, 0, 
	0, 
	DImo_collect, DImo_collect, (PTR)0, MOcdata_index
    },
    0
};

/* MO classes for di performance information.  For initialization
** to work correctly the "exp.clf.di.di_slaveno" entry must be the
** first in the array.
*/
static char di_index[] = "exp.clf.di.di_slaveno";
static MO_CLASS_DEF DImo_tmperf_classes[] = 
{
    {0, "exp.clf.di.di_slaveno", 
	sizeof(TM_PERFSTAT), MO_READ, di_index,
	0,
	DImo_array_index_get, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_utime.tm_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_utime.TM_secs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_utime.TM_secs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_utime.tm_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_utime.TM_msecs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_utime.TM_msecs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_stime.tm_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_stime.TM_secs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_stime.TM_secs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_stime.tm_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_stime.TM_msecs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_stime.TM_msecs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_cpu.tm_secs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_cpu.TM_secs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_cpu.TM_secs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_cpu.tm_msecs", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_cpu.TM_msecs), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_cpu.TM_msecs),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_maxrss", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_maxrss), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_maxrss),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_idrss", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_idrss), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_idrss),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_minflt", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_minflt), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_minflt),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_majflt", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_majflt), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_majflt),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_nswap", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nswap), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nswap),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_reads", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_reads), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_reads),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_writes", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_writes), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_writes),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_dio", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_dio), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_dio),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_msgsnd", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgsnd), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgsnd),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_msgrcv", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgrcv), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgrcv),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_msgtotal", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_msgtotal), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_msgtotal),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_nsignals", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nsignals), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nsignals),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_nvcsw", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nvcsw), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nvcsw),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0, "exp.clf.di.dimo_nivcsw", 
	MO_SIZEOF_MEMBER(TM_PERFSTAT, tmperf_nivcsw), MO_READ, di_index,
	CL_OFFSETOF(TM_PERFSTAT, tmperf_nivcsw),
	MOintget, MOnoset, (PTR)0, DImo_stat_index
    },
    {0}
};



/*{
** Name: DImo_init()	- Initialize di slave MO support.
**
** Description:
**	The server maintains a buffer to store performance stats of the server.
**
**	DImo_collect() will fill in this memory with the current set of slave
**	statistics.
**
**	This call also register's the MO interface to the di statistics.
**
** Inputs:
**
** Outputs:
**	none.
**
**	Returns:
**	    OK,FAIL
**
** History:
**      30-aug-1995 (shero03)
**          Created from the unix version.
**	    Note that there are no slave support - only the base server
**	   
*/
STATUS
DImo_init( VOID )
{
    STATUS	status = OK;

    if (Di_moinited)
        return(status);
    else
    	Di_moinited = TRUE;

    /* initialize stats in case there is a retrieve of stats before the
    ** control object is called to collect the stats.
    */
    status = DImo_collect((i4) 0, (i4) 0, (char *) 0, (i4) 0, (PTR) 0);

    if (!status)
        status = MOclassdef(MAXI2, DImo_tmperf_classes);
    if (!status)
        status = MOclassdef(MAXI2, DImo_tmctl_classes);

    return(status);
}
/*{
** Name: DImo_collect()	- collect information into static buffer.
**
** Description:
**	Routine collects the server's performance information.
**
** Inputs:
**	offset				argument is ignored.
**	lsbuf				argument is ignored.
**	sbuf				argument is ignored.
**	size				argument is ignored.
**	object				argument is ignored.
**
** Outputs:
**	none.
**
**	Returns:
**	    E_DB_OK
**
** History:
**      30-aug-1995 (shero03)
**          Created from the unix version.
*/
/* ARGSUSED */
static STATUS 
DImo_collect(
    i4 offset,
    i4 lsbuf,
    char *sbuf,
    i4 size,
    PTR object)
{
    STATUS	status;
    CL_SYS_ERR  sys_err;

    status = TMperfstat(&Di_moinfo, &sys_err);

    return(status);
}

/*{
** Name: DImo_stat_index()	- Routine called to get perf status info.
**
** Description:
**	Index routine for the MO interface to operating system statistics for
**	the slaves.
**	The gateway interface to this information is a table with one row for
**	each slave and an i4 column for each statistic.
**
** Inputs:
**      msg                             indicates first or next operation.
**                                          MO_GET - first call.
**                                          MO_NEXT - subsequent calls.
**      cdata                           unused - required by MO interface.
**      linstance                       length of instance data.
**      instance                        pointer to slave num to give info about.
**
** Outputs:
**      instdata                        Is set to static data area containing
**                                      performance statistics associated with
**					the slave number indicated by 
**					instance.
**
**	Returns:
**	    E_DB_OK
**
**
** History:
**	30-aug-1995 (shero03)
**	    Created.
**	    This routine is needed to keep the MO objects similar to UNIX
**	    (In NT there are 0 slaves)
**	
*/
/* ARGSUSED */
static STATUS 
DImo_stat_index(
    i4      msg,
    PTR     cdata,
    i4      linstance,
    char    *instance,
    PTR     *instdata)
{
    STATUS	status;
    i4     	index_key;

    CVal(instance, &index_key);

    switch (msg)
    {
	case MO_GET:
	{
	    if (index_key == 1)
	    {
	        *instdata = (PTR) &Di_moinfo;
		status = OK;
	    }
	    else
	    {
		status = MO_NO_INSTANCE;
	    }

	    break;
	}

	case MO_GETNEXT:
	{
	    if (++index_key <= 0)
	    {
		status = MO_NO_INSTANCE;
	    }
	    else if (index_key == 1)
	    {
  	        *instdata =  (PTR) &Di_moinfo;
		status = MOlongout(MO_INSTANCE_TRUNCATED, (i8)index_key,
					linstance, instance);
	    }
	    else
	    {
		status = MO_NO_NEXT;
	    }

	    break;
	}

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return(status);
}
/*{
** Name: DImo_array_index_get()	- compute array index given object location.
**
** Description:
**	Convert the address passed as the object location into an array
**	index + 1.  The base address of this array must be globally accessible
**	and in this case is "&Di_slave_moinfo.dislmo_diresource[0]".  A
**	generic "get offset of" function needs the based address of the array
**	to be passed in as a parameter, but the current MO interface does not 
**	have anywhere to store (and have passed into this routine) the base 
**	address of the array (ie. a PTR).
**
**	The output, userbuf, has a maximum length of luserbuf that will
**      not be exceeded.  If the output is bigger than the buffer, it
**      will be chopped off and MO_VALUE_TRUNCATED returned.
**
** Inputs:
**	offset				unused.
**	size				unused.
**	object				pointer to a member of the 
**					Di_slave_moinfo.dislmo_diresource array.
**
**      lsbuf				the length of the user buffer.
**
** Outputs:
**      sbuf				the buffer to use for output.
**
**	Returns:
**       OK
**              if the operation succeseded
**       MO_VALUE_TRUNCATED
**              the output buffer was too small, and the string was truncated.
** History:
**	30-aug-1995 (shero03)
**	    Created.
**	    This routine is needed to keep the MO objects similar to UNIX
**	    (In NT there are 0 slaves)
*/
/* ARGSUSED */
static i4
DImo_array_index_get(
    i4  offset, 
    i4  size, 
    PTR object, 
    i4  lsbuf, 
    char *sbuf)
{
    STATUS stat;
    i4 ival;

    /* the value to be returned is the array index + 1, gotten by doing
    ** pointer arithmetic on the object passed in and on the base of the
    ** array.
    */

    ival = 1;

    stat = MOlongout(MO_VALUE_TRUNCATED, ival, lsbuf, sbuf);

    return(stat);
}
