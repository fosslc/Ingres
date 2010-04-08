/*
** Copyright (C) 1987, 1992, 2008 Ingres Corporation  
**
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <tr.h>

# include <exhdef.h>
# include <csinternal.h>
# include "cslocal.h"

# include "csmgmt.h"

/**
** Name:	csmoscb.c	- MO interface to thread SCBs
**
** Description:
**	 MO interfaces for VMS CS SCBs.
**
** Functions:
**
**	CS_scb_attach	- make an SCB known to MO/IMA
**	CS_detach_scb	- make an SCB unknown to MO/IMA
**	CS_scb_index	- MO index functions for known CS SCB's
**	CS_scb_self_get	- MO get function for cs_self field
**	CS_state_get	- MO get function for cs_state field
**	CS_memory_get	- MO get function for cs_memory field
**	CS_thread_type_get	- MO get function for cs_thread_type field
**	CS_scb_mask_get	- MO get function for cs_ mask fields.
**
** History:
**	26-Oct-1992 (daveb)
**	    Add checks/logging in attach/detach routines.  Was getting
**	    segv's after re-attaching an SCB that had it's links zeroed
**	    out.
**	2-Nov-1992 (daveb)
**	    make CS_scb_classes GLOBALCONSTDEF for VMS sharability.
**	9-Dec-1992 (daveb)
**	    Make some things unsigned.  Also change cl.clf to .clf.
**	13-Jan-1993 (daveb)
**	    Rename exp.clf.cs to exp.clf.vms.cs in all MO objects.
**	18-Jan-1993 (daveb)
**	    in CS_memory_get, initialize str to buf; discovered by mikem.
**      16-jul-93 (ed)
**	    added gl.h
**	17-May-1994 (daveb) 59454
**	    User proper ptr method instead of int method for scb_owner
**	18-jan-1996 (duursma)
**	    Cross-Integration of Unix change 412675: use the right output
**	    functions for SCB's and SID's; also removed some unused objects
**	    ({as,rw}_q_{prev,next})
**      09-sep-1997 (kinte01)
**          Cross-Integration of Unix Change 429702. pass the thread ID 
**          instead of the address of the SCB.
**      06-feb-1998 (kinte01)
**          Cross-integration of Unix change. 
**      07-aug-97 (inkdo01)
**          Removed preceding canor01 change. Tampering with the index
**          class makes keyed retrieval strategies stop working for SCB 
**          classes.
**	19-may-1998 (kinte01)
**	   Cross-integrate Unix change 434548
**         13-Feb-98 (fanra01)
**           Modified to use the SID as the key.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**      19-may-1999 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	06-aug-1999 (devjo01)
**	    Use CS_SMUTEX_MASK to determine if blocking mutex is shared.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**      09-may-2003 (horda03) Bug 104254
**          Removed reference to scb->cs_sm_root.
**	15-jul-2003 (devjo01)
**	    Use MOsidout() instead of MOptrout() in CS_scb_self_get().
**	04-nov-2005 (abbjo03)
**	    Split CS_SCB bio and dio counts into reads and writes.
**	14-oct-2008 (joea)
**	    Integrate 16-nov-1998 change from unix version.  Updated
**	    CS_EVENT_WAIT mask interpretations.
**/

typedef struct
{
    i4	bit;
    char *str;
} MASK_BITS;

/* ---------------------------------------------------------------- */

MO_INDEX_METHOD CS_scb_index;

MO_GET_METHOD CS_scb_self_get;
MO_GET_METHOD CS_scb_state_get;
MO_GET_METHOD CS_scb_memory_get;
MO_GET_METHOD CS_scb_thread_type_get;
MO_GET_METHOD CS_scb_mask_get;

static char index_name[] = "exp.clf.vms.cs.scb_index";

GLOBALCONSTDEF MO_CLASS_DEF CS_scb_classes[] =
{
  { 0, index_name,
	0, MO_READ, index_name,
	0, MOpvget, MOnoset,
	0, CS_scb_index },

  /* special methods for special formatting */

  { 0, "exp.clf.vms.cs.scb_self",
	0, MO_READ, index_name,
	0, CS_scb_self_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_state",
	0, MO_READ, index_name,
	0, CS_scb_state_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_state_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_state), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_state), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_memory",
	MO_SIZEOF_MEMBER(CS_SCB, cs_memory), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_memory), CS_scb_memory_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_thread_type",
	0, MO_READ, index_name,
	0, CS_scb_thread_type_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_thread_type_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_thread_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_thread_type), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_mask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_mask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_mask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_asmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_asmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asmask), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_asunmask",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), CS_scb_mask_get, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_asunmask_num",
	MO_SIZEOF_MEMBER(CS_SCB, cs_asunmask), MO_READ, index_name,
	CL_OFFSETOF( CS_SCB, cs_asunmask), MOuintget, MOnoset,
	0, CS_scb_index },

  /* standard string methods */

  { 0, "exp.clf.vms.cs.scb_username",
	MO_SIZEOF_MEMBER(CS_SCB, cs_username), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_username), MOstrget, MOnoset,
	0, CS_scb_index },

  /* standard integer methods */

  { 0, "exp.clf.vms.cs.scb_length",
	MO_SIZEOF_MEMBER(CS_SCB, cs_length), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_length), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_type), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_owner",
	MO_SIZEOF_MEMBER(CS_SCB, cs_owner), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_owner), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_client_type",
	MO_SIZEOF_MEMBER(CS_SCB, cs_client_type), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_client_type), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_stk_area",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_area), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_area), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_stk_size",
	MO_SIZEOF_MEMBER(CS_SCB, cs_stk_size ), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_stk_size), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_priority), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_priority), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_base_priority",
	MO_SIZEOF_MEMBER(CS_SCB, cs_base_priority), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_base_priority), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_timeout",
	MO_SIZEOF_MEMBER(CS_SCB, cs_timeout), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_timeout), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_sem_count",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sem_count), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sem_count), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_sm_next",
	MO_SIZEOF_MEMBER(CS_SCB, cs_sm_next), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_sm_next), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_mode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_mode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_mode), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_nmode",
	MO_SIZEOF_MEMBER(CS_SCB, cs_nmode), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_nmode), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_uic",
	MO_SIZEOF_MEMBER(CS_SCB, cs_uic), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_uic), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_cputime",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cputime), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cputime), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_dior",
	MO_SIZEOF_MEMBER(CS_SCB, cs_dior), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_dior), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_diow",
	MO_SIZEOF_MEMBER(CS_SCB, cs_diow), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_diow), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_bior",
	MO_SIZEOF_MEMBER(CS_SCB, cs_bior), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_bior), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.unix.cs.scb_biow",
	MO_SIZEOF_MEMBER(CS_SCB, cs_biow), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_biow), MOintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_locks",
	MO_SIZEOF_MEMBER(CS_SCB, cs_locks), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_locks), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_connect",
	MO_SIZEOF_MEMBER(CS_SCB, cs_connect), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_connect), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_svcb",
	MO_SIZEOF_MEMBER(CS_SCB, cs_svcb), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_svcb), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_ppid",
	MO_SIZEOF_MEMBER(CS_SCB, cs_ppid), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_ppid), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_inkernel",
	MO_SIZEOF_MEMBER(CS_SCB, cs_inkernel), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_inkernel), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_async",
	MO_SIZEOF_MEMBER(CS_SCB, cs_async), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_async), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_pid",
	MO_SIZEOF_MEMBER(CS_SCB, cs_pid), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_pid), MOuintget, MOnoset,
	0, CS_scb_index },

  { 0, "exp.clf.vms.cs.scb_cnd",
	MO_SIZEOF_MEMBER(CS_SCB, cs_cnd), MO_READ, index_name,
	CL_OFFSETOF(CS_SCB, cs_cnd), MOpvget, MOnoset,
	0, CS_scb_index },

  { 0 }
};


static MASK_BITS mask_bits[] =
{
    { CS_TIMEOUT_MASK,		"TIMEOUT" },
    { CS_INTERRUPT_MASK,	"INTERRUPT" },
    { CS_BAD_STACK_MASK,	"BAD_STACK" },
    { CS_DEAD_MASK,		"DEAD" },
    { CS_IRPENDING_MASK,	"IRPENDING" },
    { CS_NOXACT_MASK,		"NOXACT" },
    { CS_MNTR_MASK,		"MNTR" },
    { CS_FATAL_MASK,		"FATAL" },
    { CS_MID_QUANTUM_MASK,	"MID_QUANTUM" },
    { CS_EDONE_MASK,		"EDONE_MASK" },
    { CS_IRCV_MASK,		"IRCV" },
    { CS_TO_MASK,		"TIMEOUT" },
    { 0, 0 }
};



/*{
** Name:	CS_scb_attach	- make an SCB known to MO/IMA
**
** Description:
**	Links the specified SCB into the known thread tree.  Logs
**	error to server log if it's already present (it shouldn't).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**
** Inputs:
**	scb		the thread to link in.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.	
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
*/
void
CS_scb_attach( CS_SCB *scb )
{
    SPBLK node;
    SPBLK *sp;

    node.key = (PTR)scb->cs_self;
    sp = SPlookup( &node, Cs_srv_block.cs_scb_ptree );
    if( sp != NULL )
    {
	TRdisplay("CS_scb_attach: attempt to attach existing SCB %x!!!\n",
		  scb );
    }
    else
    {
        scb->cs_spblk.uplink = NULL;
	scb->cs_spblk.leftlink = NULL;
	scb->cs_spblk.rightlink = NULL;
	scb->cs_spblk.key = (PTR)scb->cs_self;
	SPinstall( &scb->cs_spblk, Cs_srv_block.cs_scb_ptree );
    }
}



/*{
** Name:	CS_detach_scb	- make an SCB unknown to MO/IMA
**
** Description:
**	Unlinks the specified SCB from the known thread tree.  Best
**	called just before de-allocating the block.  Logs a message
**	to the server log with TRdisplay if the block isn't there
**	(it should be).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**
** Inputs:
**	scb		the thread to zap.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.	
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	19-may-1998 (kinte01)
**	   Cross-integrate Unix change 434548
**    	   13-Feb-98 (fanra01)
**           Modified to use the SID as the key.
*/

void
CS_detach_scb( CS_SCB *scb )
{
    SPBLK node;
    SPBLK *sp;

    node.key = (PTR)scb->cs_self;
    sp = SPlookup( &node, Cs_srv_block.cs_scb_ptree );
    if( sp == NULL )
    {
	TRdisplay("CS_detach_scb: attempt to detach unknown SCB %x\n",
		  scb );
    }
    else
    {
	SPdelete( &scb->cs_spblk, Cs_srv_block.cs_scb_ptree );
    }
}

/* ---------------------------------------------------------------- */

/*{
** Name:	CS_scb_index	- MO index functions for known CS SCB's
**
** Description:
**	Standard MO index function to get at the nodes in the known
**	scb tree.
**
** Re-entrancy:
**	NO; this may be a problem in real MP environments.  FIXME.
**
** Inputs:
**	msg		the index op to perform
**	cdata		the class data, ignored.
**	linstance	the length of the instance buffer.
**	instance	the input instance
**
** Outputs:
**	instance	on GETNEXT, updated to be the one found.
**	idata		updated to point to the SCB in question.
**
** Returns:
**	standard index function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	19-may-1998 (kinte01)
**	   Cross-integrate Unix change 434548
**         13-Feb-98 (fanra01)
**           Modified to use SID as instance but returns the scb as the
**           instance data to be passed to methods.  Incurs a lookup 
**	     performance
*/

STATUS
CS_scb_index(i4 msg,
	     PTR cdata,
	     i4 linstance,
	     char *instance, 
	     PTR *instdata )
{
    STATUS stat = OK;
    PTR ptr;

    switch( msg )
    {
    case MO_GET:
	if( OK == (stat = CS_get_block( instance,
				       Cs_srv_block.cs_scb_ptree,
				       &ptr ) ) )
	    *instdata = (PTR) CS_find_scb((CS_SID) ptr);
	break;

    case MO_GETNEXT:
	if( OK == ( stat = CS_nxt_block( instance,
					Cs_srv_block.cs_scb_ptree,
					&ptr ) ) )
	{
            *instdata = (PTR) CS_find_scb((CS_SID) ptr);
            stat = MOptrout( MO_INSTANCE_TRUNCATED,
                              ptr,
			      linstance,
			      instance );
	}
	break;

    default:
	stat = MO_BAD_MSG;
	break;
    }
    return( stat );
}

/* ---------------------------------------------------------------- */


/*{
** Name:	CS_scb_self_get	- MO get function for cs_self field
**
** Description:
**	A nice interpretation of the cs_self fields, special
**	casing CS_DONT_CARE and CS_ADDER_ID.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
*/

STATUS
CS_scb_self_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 24 ];
    char *str = buf;
    CS_SCB *scb = (CS_SCB *)object;
    
    if( scb->cs_self == CS_DONT_CARE )
	str = "CS_DONT_CARE";
    else if( scb->cs_self == CS_ADDER_ID )
	str = "CS_ADDER_ID";
    else
	(void) MOsidout( 0, (PTR)scb->cs_self, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	CS_state_get	- MO get function for cs_state field
**
** Description:
**	A nice interpretation of the cs_state fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with nice state string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented. 
*/
STATUS
CS_scb_state_get(i4 offset,
		 i4 objsize,
		 PTR object,
		 i4 luserbuf,
		 char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;
    char *str;

    switch (scb->cs_state)
    {
    case CS_FREE:
	str = "CS_FREE";
	break;
    case CS_COMPUTABLE:
	str = "CS_COMPUTABLE";
	break;
    case CS_STACK_WAIT:
	str = "CS_STACK_WAIT";
	break;
    case CS_UWAIT:
	str = "CS_UWAIT";
	break;
    case CS_EVENT_WAIT:
	str = "CS_EVENT_WAIT";
	break;
    case CS_MUTEX:
	str = "CS_MUTEX";
	break;
    case CS_CNDWAIT:
	str = "CS_CNDWAIT";
	break;
	
    default:
	str = "<invalid>";
	break;
    }

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}



/*{
** Name:	CS_memory_get	- MO get function for cs_memory field
**
** Description:
**	A nice interpretation of the cs_memory field.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented. 
**	18-Jan-1993 (daveb)
**	    initialize str to buf; discovered by mikem.
*/
STATUS
CS_scb_memory_get(i4 offset,
		  i4 objsize,
		  PTR object,
		  i4 luserbuf,
		  char *userbuf)
{
    STATUS stat;
    char buf[ 80 ];
    char *str = buf;
    
    CS_SCB *scb = (CS_SCB *)object;

    switch (scb->cs_state)
    {
    case CS_EVENT_WAIT:

	str = scb->cs_memory & CS_DIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "DIOR" : "DIOW"
		:
	      scb->cs_memory & CS_BIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "BIOR" : "BIOW"
		:
	      scb->cs_memory & CS_LIO_MASK ?
		  scb->cs_memory & CS_IOR_MASK ?
		  "LIOR" : "LIOW"
		:
	      scb->cs_memory & CS_LOCK_MASK ?
		  "LOCK"
		:
	      scb->cs_memory & CS_LOG_MASK ?
		  "LOG"
		:
	      scb->cs_memory & CS_LKEVENT_MASK ?
		  "LKEvnt"
		:
	      scb->cs_memory & CS_LGEVENT_MASK ?
		  "LGEvnt"
		:
	      scb->cs_memory & CS_TIMEOUT_MASK ?
		  "TIME"
		:
		  "OTHER";
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );
	break;

    case CS_COMPUTABLE:
	if ( !(scb->cs_mask & CS_MUTEX_MASK) )
	{
	    stat = MOulongout( MO_VALUE_TRUNCATED, scb->cs_memory,
			       luserbuf, userbuf );
	    break;
	}
	/* Drop into next case */
 
    case CS_MUTEX:
	buf[0] = scb->cs_mask & CS_SMUTEX_MASK ? 's' : 'x';
	buf[1] = ' ';
	(void) MOulongout( 0, scb->cs_memory, sizeof(buf)-2, &buf[2] );
	stat = MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf );
	break;

    default:

	stat =  MOulongout( MO_VALUE_TRUNCATED, scb->cs_memory,
			   luserbuf, userbuf );

	break;
    }
    return( stat );
}


/*{
** Name:	CS_thread_type_get	- MO get function for cs_thread_type field
**
** Description:
**	A nice interpretation of the cs_thread_type.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "internal", "user",
**			or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
*/

STATUS
CS_scb_thread_type_get(i4 offset,
		       i4 objsize,
		       PTR object,
		       i4 luserbuf,
		       char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;
    char buf[ 80 ];
    char *str = buf;

    if( scb->cs_thread_type == CS_INTRNL_THREAD )
	str = "internal";
    else if( scb->cs_thread_type == CS_USER_THREAD )
	str = "user";
    else
	(void) MOlongout( 0, scb->cs_thread_type, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	CS_scb_mask_get	- MO get function for cs_ mask fields.
**
** Description:
**	A nice interpretation of mask fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
*/

STATUS
CS_scb_mask_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 80 ];
    i4 mask = *(i4 *)((char *)object + offset);
    MASK_BITS *mp;
    char *p = buf;
    char *q;

    for( mp = mask_bits; mp->str != NULL; mp++ )
    {
	if( mask & mp->bit )
	{
	    if( p != buf )
		*p++ = ',';
	    for( q = mp->str; *p++ = *q++ ; )
		continue;
	}
    }
    *p = EOS;
    return( MOstrout( MO_VALUE_TRUNCATED, buf, luserbuf, userbuf ));
}

/* ---------------------------------------------------------------- */

