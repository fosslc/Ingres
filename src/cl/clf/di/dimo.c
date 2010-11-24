/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

#include    <compat.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <sp.h>
#include    <mo.h>
#include    <tm.h>
#include    <tr.h>
#include    <fdset.h>
#include    <csev.h>
#include    "dimo.h"
#include    <dislave.h>
#include    "dilocal.h"

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
**      26-jul-93 (mikem)
**	    Created.
**	17-aug-1993 (tomm)
**	    Must include fdset.h since protypes being included reference
**	    fdset.
**      10-sep-93 (smc)
**          Removed initialization of 1st member of the DImo_tmperf_classes 
**          array, as this was dead code (and not portable to 64 bit machines).
**	20-sep-93 (mikem)
**	    Removed initialization of offset field of MO class in 
**	    DImo_init_slv_mem() as it was not being used.  
**	01-may-1995 (amo ICL)
**	    Added ANB's asyncio amendments
**	03-jun-1996 (canor01)
**	    Remove ME_stream_sem semaphore
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs to dilocal.h
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**/


/*
**  Forward and/or External typedef/struct references.
*/

/*}
** Name: DI_SLAVE_MOINFO - Performance stats of the slaves.
**
** Description:
**	
**	A place to store performance stats of slaves in the server.  Space is
**	allocated at di initialize time for each slave.  A MO control object
**	is used to force the server to update the stats of all slaves by calling
**	each slave with the DI_COLLECT_STATS operation.  Then it is expected
**	that the MO interface will be used to retrieve the slave statistics
**	stored in this structure (one row per slave).
**	
** History:
**      26-jul-93 (mikem)
**          Created.
*/
typedef struct
{
    i4         	dislmo_numslaves; 	/* number of slaves */
    CSSL_CB 		*dislmo_slv_cntrl_blk; 	/* event system control block */
    TM_PERFSTAT		*dislmo_diresource;	/* dynamically allocated 
						** resource slot per slave, 
						** indexed by slave #.
						*/
} DI_SLAVE_MOINFO;



/*
**  Definition of forward static functions.
*/
static STATUS DImo_stat_index(
    i4      msg,
    PTR     cdata,
    i4      linstance,
    char    *instance,
    PTR     *instdata);

static STATUS DImo_collect(
    i4  offset,
    i4  lsbuf,
    char *sbuf,
    i4  size,
    PTR object);

static i4 DImo_array_index_get(
    i4  offset, 
    i4  size, 
    PTR object, 
    i4  lsbuf, 
    char *sbuf);

/*
** Globals
*/

/*
**  Definition of static variables.
*/

static DI_SLAVE_MOINFO Di_slave_moinfo;         /* slave performance stats */

static MO_CLASS_DEF DImo_tmctl_classes[] = 
{
    {0, "exp.clf.di.dimo_collect", 
	0, MO_READ|MO_WRITE, 0, 
	0, 
	MOzeroget, DImo_collect, (PTR)0, MOcdata_index
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
** Name: DImo_init_slv_mem()	- Initialize space for di slave MO support.
**
** Description:
**	The server maintains a buffer to store performance stats of all slaves.
**	This buffer is dynamically allocated at system startup and is used by
**	all subsequent MO slave statistic requests.  
**
**	DImo_collect() will fill in this memory with the current set of slave
**	statistics.
**
**	This routine must only be called subsequent to slave startup, as it
**	makes a call to DImo_collect() to initialize the slave statistics.
**
**	This call also register's the MO interface to the di statistics.
**
** Inputs:
**	num_slaves		number of DI slaves for this server.
**	di_slave		event system control block to control slaves.
**
** Outputs:
**	none.
**
**	Returns:
**	    OK,FAIL
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	20-sep-93 (mikem)
**	   Removed initialization of offset field of MO class as it was not
**	   being used.  Slave number is calculated by using PTR arithmetic
**	   between pointer to member of array and direct access to 
**	   "&Di_slave_moinfo.dislmo_diresource[0]" in the DImo_array_index_get()
**	   function, rather than storing the value of base address in the
**	   MO data structure.
**	01-jun-95 (canor01)
**	   add semaphores for memory call
**	   
*/
STATUS
DImo_init_slv_mem(
    i4		num_slaves, 
    CSSL_CB	*di_slave)
{
    STATUS	status = OK;
    i4		num_slots;

    if (num_slaves <= 0)
    {
	/* 0 slaves, allocate a single slot and just use servers stats rather
	** than making slave calls
	*/
	num_slots = 1;
    }
    else
    {
	num_slots = num_slaves;
    }

    Di_slave_moinfo.dislmo_numslaves = num_slaves;
    Di_slave_moinfo.dislmo_slv_cntrl_blk = di_slave;
    Di_slave_moinfo.dislmo_diresource = 
	(TM_PERFSTAT *) MEreqmem((u_i4) 0, 
	    (u_i4) (sizeof(TM_PERFSTAT) * num_slots), TRUE, (STATUS *) NULL);

    if (!Di_slave_moinfo.dislmo_diresource)
    {
	status = FAIL;
    }
    else
    {
	/* initialize stats in case there is a retrieve of stats before the
	** control object is called to collect the stats.
	*/
	status = DImo_collect((i4) 0, (i4) 0, (char *) 0, (i4) 0, (PTR) 0);

	if (!status)
	    status = MOclassdef(MAXI2, DImo_tmperf_classes);
	if (!status)
	    status = MOclassdef(MAXI2, DImo_tmctl_classes);
    }

    return(status);
}
/*{
** Name: DImo_collect()	- collect all slave information into static buffer.
**
** Description:
**	Routine talks to each slave and collects it's performance information.
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
**      01-apr-93 (login)
**          Created.
*/
/* ARGSUSED */
static STATUS 
DImo_collect(
    i4  offset,
    i4  lsbuf,
    char *sbuf,
    i4  size,
    PTR object)
{
    i4		slv_no;
    CSEV_CB	*evcb;
    DI_SLAVE_CB	*disl;
    CL_SYS_ERR  sys_err;
    STATUS	status = OK;
    STATUS      intern_status = OK; 
    STATUS	send_status = OK;
    DI_OP       di_op;


    if (Di_slave_moinfo.dislmo_numslaves > 0 && !Di_async_io)
    {
	CSreserve_event(Di_slave_moinfo.dislmo_slv_cntrl_blk, &evcb);

	/* need to manufacture a di_op for the slave call */
	di_op.di_flags = 0;
	di_op.di_csevcb = evcb;
	di_op.di_evcb = disl = (DI_SLAVE_CB *)evcb->data;

	disl->status = OK;

	/* there are slaves to talk to, loop collecting stats from each slave */
	for (slv_no = 0; slv_no < Di_slave_moinfo.dislmo_numslaves; slv_no++)
	{
	    disl->file_op = DI_COLLECT_STATS;
	    disl->dest_slave_no = slv_no;

	    DI_slave_send(disl->dest_slave_no, &di_op, &status, 
					&send_status, &intern_status);

	    if (status != OK)
	    {
		break;
	    }
	    else if (send_status != OK)
	    {
		status = send_status;
		break;
	    }
	    else if (intern_status != OK)
	    {
		status = intern_status;
		break;
	    }

	    MEcopy((PTR)disl->buf, sizeof(TM_PERFSTAT), 
		   (PTR)&Di_slave_moinfo.dislmo_diresource[slv_no]);
	}

	CSfree_event(evcb);
    }
    else
    {
	status = TMperfstat(&Di_slave_moinfo.dislmo_diresource[0], &sys_err);
    }

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
**      26-jul-93 (mikem)
**         Created.
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
    i4	index_key;
    i4	slave_index;

    CVal(instance, &index_key);

    switch (msg)
    {
	case MO_GET:
	{
	    if (index_key < 1)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    else if ((index_key == 1) && 
		     (Di_slave_moinfo.dislmo_numslaves == 0))
	    {
		/* special case for "zero slave server" */
		slave_index = 0;
	    }
	    else if (index_key <= Di_slave_moinfo.dislmo_numslaves)
	    {
		slave_index = index_key - 1;
	    }
	    else
	    {
		status = MO_NO_INSTANCE;
		break;
	    }

	    /* index_key is the slave number that we are interested in.  The
	    ** array index starts at 0 while the index_key starts at 1.
	    */
	    *instdata = 
		(PTR) &Di_slave_moinfo.dislmo_diresource[slave_index];

	    status = OK;

	    break;
	}

	case MO_GETNEXT:
	{
	    if (++index_key <= 0)
	    {
		status = MO_NO_INSTANCE;
	    }
	    else if (index_key <= Di_slave_moinfo.dislmo_numslaves)
	    {
		*instdata =
		    (PTR) &Di_slave_moinfo.dislmo_diresource[index_key - 1];
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
**      26-jul-93 (mikem)
**          Created.
**	20-sep-93 (mikem)
**	    Fixed comments to reflect use &Di_slave_moinfo.dislmo_diresource[0]
**	    rather than "object" parameter.
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
    STATUS stat = OK;
    i4 ival = 0;

    /* the value to be returned is the array index + 1, gotten by doing
    ** pointer arithmetic on the object passed in and on the base of the
    ** array.
    */

    ival = ((TM_PERFSTAT *) object) - &Di_slave_moinfo.dislmo_diresource[0];

    stat = MOlongout(MO_VALUE_TRUNCATED, ival, lsbuf, sbuf);

    return(stat);
}
