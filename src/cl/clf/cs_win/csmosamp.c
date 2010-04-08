/*
** Copyright (c) 1998, 2000 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>	/* needed by lg.h */
# include <cv.h>
# include <pc.h>	/* needed by lk.h */
# include <sp.h>	/* needed by mo.h for splay trees */
# include <mo.h>	/* Managed Objects */
# include <lk.h>	/* needed by cssampler.h */
# include <csinternal.h>

# include "cssampler.h"

/*
** Name: CSmosamp.c	- Managed Object interface for Sampler
**
** Description:
**	This file contains the routines which manipulate 
**	the managed objects for the CSsampler:
** 
** 	CS_samp_thread_index - MO index method for sampler thread array
**
** History:
**	13-Nov-1998 (wonst02)
** 	    Created.
** 	07-Jan-1999 (wonst02)
** 	    Comments and minor changes.
**	20-jan-1999 (canor01)
**	    Remove include of non-CL header
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	10-jan-2000 (somsa01)
**	    Added include of csinternal.h .
**	11-oct-2000 (somsa01)
**	    Changed MAXTHREADS to MAXSAMPTHREADS.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

GLOBALREF CSSAMPLERBLKPTR CsSamplerBlkPtr;


/*{
** Name: CS_samp_thread_index - MO index method for sampler thread array
**
** Description:
**	Traverse the CS sampler thread array. Supports:
** 
**		MO_GET - Get via a particular index value
**		MO_GETNEXT - Get the next in the array
** 
** Inputs:
** 	msg				MO_GET or MO_GETNEXT
** 	cdata				Class data (passed to the index method)
** 	linstance			Length of instance
** 	instance			Ptr to instance definition
** 	instdata			Ptr to instance data area
**
** Outputs:
** 	instdata			Ptr to instance data
**	
** Returns:
** 
** Exceptions:
** 
** Side Effects:
**
** History:
** 	13-Nov-1998 (wonst02)
** 	    Original.
*/
STATUS	CS_samp_thread_index(
    i4 	    msg,
    PTR	    cdata,
    i4 	    linstance,
    char    *instance,
    PTR	    *instdata)
{
    STATUS	    status = OK;
    i4     	    ix;
    CSSAMPLERBLK    *cssb=CsSamplerBlkPtr; /* Make a local ptr to see in debug */

    if (cssb == NULL)
    	return MO_NO_INSTANCE;

    if (instance && *instance != '\0')
    	CVal(instance, &ix);
    else
	ix = CS_INTRNL_THREAD - 1;	/* Start at the -1 (internal) thread,
					** but backup one for incrementing. */

    switch(msg)
    {
	case MO_GET:
	    if (ix < -1 || ix >= MAXSAMPTHREADS)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    /* Return the requested state of this thread */
	    *instdata = (PTR)&cssb->Thread[ix];
	    status = OK;

	    break;

	case MO_GETNEXT:
	    ++ix;				/* Next thread entry */
	    if (ix < -1 || ix >= MAXSAMPTHREADS)
	    {
		status = MO_NO_INSTANCE;
		break;
	    }
	    /* Return the requested state of this thread */
	    *instdata = (PTR)&cssb->Thread[ix];
	    /* Return the updated instance number */
	    status = MOlongout( MO_INSTANCE_TRUNCATED,
	    		    	(i8)ix,
	    		    	linstance, instance);
	    break;

	default:
	    status = MO_BAD_MSG;
	    break;
    }

    return (status);
}

