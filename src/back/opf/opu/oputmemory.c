/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPU_TMEMORY      TRUE
#include    <opxlint.h>

/**
**
**  Name: OPUTMEMORY.C - get temp memory
**
**  Description:
**      This routine will get a temp memory buffer
**
**  History:    
**      3-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
PTR opu_tmemory(
	OPS_STATE *global,
	i4 size);

/*{
** Name: opu_tmemory	- get temporary memory buffer
**
** Description:
**      This routine will get a scratch memory buffer.
**      THE SAME BUFFER WILL BE OBTAINED EACH TIME... so this routine can
**      not be used when two buffers are required.  The purpose of the routine
**      is to be used in a very localized situation (such as an ADF conversion
**      prior to be moved into a key buffer).  This will avoid situations in
**      which large stack buffers of "MAXTUP" size are allocated (and cause
**      page faults because the end of the buffer is referenced in the stack).
**      It will eliminate the need for large stacks to be allocated (especially
**      if 32K strings will be allowed).
**
**      A separate memory stream will be created for this buffer and this
**      stream will be extended every time a larger buffer is required.  
**      The "opu_memory" memory stream is not used since fragmentation
**      and dangling ptrs would result.
**
**      FIXME - the current routine will destroy the stream each time - when
**      ULM implements the "reinitstream" function then use it
**
** Inputs:
**      global                          ptr to global state variable
**      size                            size of buffer required
**
** Outputs:
**	Returns:
**	    PTR to temporary buffer area
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	11-oct-2006 (hayke02)
**	    Send E_OP0002_NOMEMORY to errlog.log. This change fixes bug 116309.
[@history_line@]...
*/
PTR
opu_tmemory(
	OPS_STATE          *global,
	i4                size)
{
    DB_STATUS               ulmstatus;  /* return status from ULM */

    if ( global->ops_mstate.ops_tsize >= size )
	return (global->ops_mstate.ops_tptr);	/* return ptr if it is big enough */

    if ( global->ops_mstate.ops_tstreamid )
    {
	/* deallocate ULM temp buffer memory stream if it is not big enough */
        global->ops_mstate.ops_tptr = NULL;
        global->ops_mstate.ops_tsize = 0;
	opu_deallocate(global, &global->ops_mstate.ops_tstreamid);
    }

    {
	/* allocate a temp buffer memory stream to be used by the optimizer */
	global->ops_mstate.ops_tstreamid = opu_allocate(global); /* save the streamid for
                                        ** destruction of stream */
    }

    {
        /* get a temp buffer of appropriate size from the new stream */

	global->ops_mstate.ops_ulmrcb.ulm_streamid_p = &global->ops_mstate.ops_tstreamid; /* allocate 
					** memory from the temp stream */
        global->ops_mstate.ops_ulmrcb.ulm_psize = size; /* size required */
	if ( (ulmstatus = ulm_palloc( &global->ops_mstate.ops_ulmrcb )) != E_DB_OK )
	{
	    if (global->ops_mstate.ops_ulmrcb.ulm_error.err_code == E_UL0005_NOMEM)
	    {
		opx_lerror(E_OP0002_NOMEMORY, 0, 0, 0, 0, 0);
		opx_error( E_OP0002_NOMEMORY);  /* out of memory */
	    }
#ifdef E_OP0093_ULM_ERROR
	    else
		opx_verror( ulmstatus, E_OP0093_ULM_ERROR, 
		    global->ops_mstate.ops_ulmrcb.ulm_error.err_code); /* check for 
					** other ULM error*/
#endif
	}

	global->ops_mstate.ops_tptr = global->ops_mstate.ops_ulmrcb.ulm_pptr; /* save ptr to temp 
					** buffer for future use */
	global->ops_mstate.ops_tsize = size;	/* save size of this buffer */
    }
    return( global->ops_mstate.ops_tptr );  /* return the allocated memory */
}
