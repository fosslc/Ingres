/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: LGKPARMS.H - External data structures for calls to LGK
**
** Description:
**	This file contains the RCONF structure definition. In earlier releases
**	of Ingres, the RCONF structure described the layout of a binary file
**	named rcpconfig.dat. This file was written by rcpconfig when given one
**	of the -init, -force_init, or -config options, and contained parameters
**	governing the Ingres logging, locking, and recovery systems.
**
**	With the introduction of the PM facility in Ingres 6.5, the
**	rcpconfig.dat file is no longer used. Instead, Ingres software calls PM
**	to get the logging, locking, and recovery configuration information.
**	However, it is still useful to collect together all the parameter values
**	into a single structure, and that structure is still called the RCONF,
**	because that meant we had to change less code.
**
**	So the RCONF is now used to call lgk_get_config() to get the config
**	parameters.
**
** History:
**	26-apr-1993 (andys/bryanp)
**	    Created from LGKDEF.H.
**	24-may-1993 (bryanp)
**	    Added new resource block configuration parameter.
**	20-sep-1993 (jnash)
**	    Modify cp_p to be an f8 to allow cps to be expressed with fractional
**	    components.
**	08-May-1998 (jenjo02)
**	    Added nparts, number of log partitions,
**	    logkb, size of log file in KB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-apr-2001 (devjo01)
**	    Change params to lgk_calculate_size, add lgk_get_counts.
**      23-Oct-2002 (hanch04)
**          Changed lgk_calculate_size to use a SIZE_TYPE to allow
**	    memory > 2 gig
*/

#ifndef INCLUDED_LGKDEF_H
/* To pick up LGLK_INFO definition */
#include <lgkdef.h>
#endif 

typedef struct _RCONF RCONF;

/*}
** Name: RCONF - Recovery process configuration block.
**
** Description:
**      This structure contains the recovery process configuration block.
**
** History:
**      30-Sep-1986 (ac)
**          Created for Jupiter.
**      28-Sep-1989 (ac)
**          Added dual logging support.
**	20-jun-1990 (bryanp)
**	    Removed active_log from the RCONF.
**	26-apr-1993 (andys/bryanp)
**	    Moved from lgkdef.h to lgkparms.h.
**	    Change name of get_lgk_config to lgk_get_config.
**	24-may-1993 (bryanp)
**	    Added new resource block configuration parameter.
**	21-june-1993 (jnash)
**	    Add lgk_mem_lock.
**	20-sep-1993 (jnash)
**	    Modify cp_p to be an f8 to allow cps to be expressed with 
**	    fractional components.
**	08-May-1998 (jenjo02)
**	    Added nparts, number of log partitions,
**	    logkb, size of log file in KB.
**	3-Dec-2003 (schka24)
**	    Added CP interval in Mb.
**	21-Mar-2006 (jenjo02)
**	    Add optimwrites for optimized log writes.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add rbblocks, rfblocks
**	    for buffered log reads.
*/
struct _RCONF
{
    i4	bcnt;	    /* Number of log buffers in the memory. */
    i4	blks;	    /* Maximum small blocks dynamic memory for logging
			    ** system. */
    i4	ldbs;	    /* Maximum large blocks dynamic memory for 
			    ** logging system. */
    i4	bsize;	    /* The current block size of the log file. */
    i4	cpcnt;	    /* The maximum cp interval for invoking the
			    ** archiver. */
    i4	lf_p;	    /* The log-full-limit in percentage. */
    f8	cp_p;	    /* The percentage of log file for each 
			    ** consistency point. (Deprecated) */
    i4	cp_mb;	    /* The consistency point interval in MiB */
    i4	abort_p;    /* The force-abort-limit in percentage. */
    i4	lkh;	    /* Size of the lock table. */
    i4	rsh;	    /* Size of the resource table. */
    i4	lkblk;	    /* Maximum small blocks dynamic memory for locking
			    ** system. */
    i4	resources;  /* Maximum resource block dynamic memory for logging
			    ** system. */
    i4	lkllb;	    /* Maximum large blocks dynamic memory for locking
			    ** system. */
    i4	maxlkb;	    /* Maximum number of logical locks allowed for each
			    ** lock list. */
    i4	init;	    /* Flag to indicate the first time using the log 
			    ** file. */
    i4	dual_logging; /* Flag to indicate if the dual logging is enabled. */
    i4	lgk_mem_lock; /* Flag to indicate lgk memory locked. */
    i4	nparts;	    /* Number of log file partitions */
    i4	logkb;	    /* Size of log file in KB */
    i4  optimwrites;  /* Flag if logging is to optimized writes */
    i4	rbblocks;   /* readbackward_blocks */
    i4	rfblocks;   /* readforward_blocks */
};

/*
** Function prototypes for LGK functions
*/
FUNC_EXTERN STATUS  lgk_get_config(
					RCONF		*rconf, 
					CL_ERR_DESC	*sys_err,
					i4 wordy);
FUNC_EXTERN STATUS  lgk_calculate_size(
					i4 wordy,
					LGLK_INFO   *lglk_counts,
					SIZE_TYPE   *totsize);
FUNC_EXTERN STATUS lgk_get_counts( LGLK_INFO   *lglk_counts, i4  wordy );

