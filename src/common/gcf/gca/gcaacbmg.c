/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <me.h>
#include    <qu.h>
#include    <cs.h>
#include    <mu.h>
#include    <gca.h>
#include    <gc.h>
#include    <gcaint.h>
#include    <gcm.h>

#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>

/**
**
**  Name: gcaacbmgt.c - Association Control Block (ACB) management functions
**
**  Description:
**        
**      gcaacbmgt.c contains a group of functions which create, find and  
**      destroy ACB's.  These are used at various times by various routines 
**      during the duration of an association. 
**        
**      An ACB is created when an association is begun.  It exists for the  
**      duration of the association and is destroyed when the association  
**      is terminated.  It contains all the status information required by 
**      GCA to execute user requests on the association.  It is uniquely  
**      identified by the association id. 
**        
**	Pointers to all existing ACB's are maintained on a single array.
**	The position in this array corresponds to the ACB's association id.
**	When a new ACB is needed, it is allocated and a pointer to it is 
**	placed into the array.  This array is extending (by reallocating
**	and copying) in increments of about 10.  When an ACB is freed
**	by a call to gca_del_acb(), its assoc_id is set to -1 to indicate
**	that gca_add_acb() can recover it the next time an ACB is needed.
**
**	This mechanism allows for fairly speedy lookup by simply indexing
**	into the array and validating the ACB, and for fast deletion by
**	simply setting the assoc_id to -1.
**
**      The following functions are defined in this module: 
**
**          gca_add_acb() - create an ACB and add it to the table array.
**          gca_find_acb() - find an ACB given the association ID.
**          gca_next_acb() - return the ID of the next assocn in  the list.
**          gca_del_acb() - delete an ACB and make its slot available.
**	    gca_rs_acb() - restore ACB tables after GCA_RESTORE
**	    gca_free_acbs() - free storage for unused ACBs.
**	    gca_append_aux() - append data to aux data buffer.
**	    gca_aux_element() - append element (adding header) to aux data.
**	    gca_del_aux() - delete leading aux data element.
**
**
**  History:    $Log-for RCS$
**      22-Apr-87 (jbowers)
**          Initial module creation
**      10-Aug-87 (jbowers)
**          Added the function gca_next_acb()
**	16-May-89 (GordonW)
**	    check return from gca_alloc call.
**	20-Sep-89 (seiwald)
**	    Removed insane system for managing ACB pointers.
**	    They now live on a single list.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	23-Jan-90 (seiwald)
**	    Moved freeing of send/receive buffers in from gca_disassoc.
**	25-Jan-90 (seiwald)
**	    New gca_free_acbs function to free storage on terminate.
**	16-Feb-90 (seiwald)
**	    Zombie checking removed from mainline to VMS CL.  Removed
**	    references to ACB flags controlling zombie checking.
**	16-Feb-90 (seiwald)
**	    Protect GCA_ACB queue in gca_find_acb, gca_next_acb with 
**	    semaphore calls; remove hocus read_ctl access control.
**	22-Feb-90 (seiwald)
**	    Fixed gca_next_acb(), botched by last change.
**	05-Apr-90 (seiwald)
**	    Reset non-shared parts of the ACB on GCA_RESTORE.
**	09-Apr-90 (seiwald)
**	    Reset non-shared parts of the ACB on GCA_RESTORE, take 2.
**	06-Jun-90 (seiwald)
**	    Made gca_alloc() return the pointer to allocated memory.
**	20-Jun-90 (seiwald)
**	    Buffers handed to GCsend() and GCreceive() now have
**	    GCA_CL_HDR_SZ bytes before them for use by the CL.
**	28-jun-91 (kirke)
**	    Allocate correct association array size in gca_rs_acb().
**	06-Sep-92 (brucek)
**	    Add MO calls for gca_{add,del}_acb.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	10-Mar-93 (edg)
**	    Track ACBs being used in new IIGCa_static variable acbs_in_use.
**	    This is so the info is more readily available via fastselect.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**      12-jun-1995 (chech02) 
**          Added semaphores to protect critical sections (MCT). 
**	27-jun-1995 (emmag)
**	    #ifdef MCT'ed the previous change.
**	 3-Nov-95 (gordy)
**	    Removed MCT ifdefs from generic semaphores.  Remove Ingres 
**	    server specific code and generalize for all servers.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	21-Dec-95 (gordy)
**	    Free the send/recv doc resources in acb.
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
**	18-Feb-97 (gordy)
**	    Initialize the concatenation flag as default.
**	 2-May-97 (gordy)
**	    Aux data now variable length.  Add buffer management for
**	    aux data to ACB.  
**	 8-Jul-97 (gordy)
**	    Need to update max length when allocating new aux data buffer.
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitions to gcm.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-aug-2003 (abbjo03)
**	    On VMS, gca_del_acb may be called from an AST and it calls
**	    MUp_semaphore, which then calls CSp_semaphore and causes trouble
**	    because the latter does not expect to be in an AST. To avoid this,
**	    implement a free list of ACBs, to be reused when gca_add_acb gets
**	    called (after the AST).	
**	19-sep-2003 (abbjo03)
**	    Additional change on deleting and reusing ACBs so single-threaded
**	    servers (e.g., gcn) will not be affected.
**	15-Oct-03 (gordy)
**	    Removed VMS kludge from prior submissions which was due to
**	    the server calling FASTSELECT which is not a RESUME operation
**	    and having AST conflicts as a result.  RESUME of FASTSELECT
**	    is now supported at GCA_API_LEVEL_6, so kludge is not needed.
**	17-Jul-09 (gordy)
**	    Support long user names.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**/



/*{
** Name: gca_add_acb()	- Create an ACB and add it to the table array
**
** Description:
**      An ACB is created and initialized.  The table array is searched for the 
**      first ununsed ACB.  If none is found, a new one is allocated.  The
**	table is extended if necessary.
**
** Inputs:
**	None
**
** Outputs:
**      None
**
** Returns:
**	GCA_ACB *	A pointer to the new ACB.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      22-Apr-87 (jbowers)
**          Initial function creation.
**      07-Aug-87 (jbowers)
**          Changed ACB initialization to match change to ACB replacing 
**          scv_parms queues with simple pointers.
**      23-Feb-88 (jbowers)
**          In gca_add_acb, change ACB initialization to MEfill the ACB to
**          0 to ensure complete initialization.
**	04-Oct-89 (seiwald)
**	    Unswitched args to MEcopy().
**	07-Mar-90 (seiwald)
**	    Zero acbvec after allocating.  We can't assume gca_alloc() 
**	    will zero memory.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	 3-Nov-95 (gordy)
**	    Remove Ingres server specific code and generalize for all servers.
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
**	18-Feb-97 (gordy)
**	    Initialize the concatenation flag as default.
**	 2-May-97 (gordy)
**	    Initialize the peer info version.  
**	 6-Jun-03 (gordy)
**	    Move MIB ACB attachment to point where association is exposed
**	    to application (end of LISTEN/REQUEST).
**	22-aug-2003 (abbjo03)
**	    For VMS, reuse any ACB from the free list which was not deleted
**	    by gca_del_acb because of an AST.
**	15-Oct-03 (gordy)
**	    Removed VMS kludge from prior submissions which was due to
**	    the server calling FASTSELECT which is not a RESUME operation
**	    and having AST conflicts as a result.  RESUME of FASTSELECT
**	    is now supported at GCA_API_LEVEL_6, so kludge is not needed.
*/

GCA_ACB *
gca_add_acb( VOID )
{
    STATUS		status;
    GCA_ACB             *acb = NULL;
    i4                  assoc_id;

    /* 
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    /*
    ** Scan the list of known ACBs for an unused one, indicated by
    ** having an assoc_id < 0, or an empty slot.  Empty slots are
    ** created by gca_rs_acb on GCA_RESTORE.
    */
    for( assoc_id = 0; assoc_id < IIGCa_global.gca_acb_max; assoc_id++ )
	if ( IIGCa_global.gca_acb_array[ assoc_id ] == NULL  ||
	     IIGCa_global.gca_acb_array[ assoc_id ]->assoc_id < 0 )
	    break;

    /*
    ** If we didn't find an unused ACB, we'll need to allocate another.
    ** Further, if we have filled the list of ACB pointers, we'll need 
    ** to reallocate a longer list.
    */
    if ( assoc_id >= IIGCa_global.gca_acb_max )
    {
	GCA_ACB	**newarr;
	i4	newmax;

	/*
	** Allocate and clear a new ACB pointer list.
	** Clients tend to have only a few associations
	** while servers can have many.  We initially
	** allocate room for only a small number (for
	** clients) but extend the array by a large
	** number (for servers).
	*/
	newmax = IIGCa_global.gca_acb_max +
		 (IIGCa_global.gca_acb_max ? 100 : 10);
	newarr = (GCA_ACB **)gca_alloc( newmax * sizeof(GCA_ACB *) );
	if ( ! newarr )  goto error;

	/* 
	** Copy & free old list if present. 
	*/
	if ( IIGCa_global.gca_acb_max )
	{
	    MEcopy( (PTR)IIGCa_global.gca_acb_array, 
		    IIGCa_global.gca_acb_max * sizeof(GCA_ACB *), 
		    (PTR)newarr );
	    gca_free( (PTR)IIGCa_global.gca_acb_array );
	}

	/* 
	** Install new list. 
	*/
	IIGCa_global.gca_acb_max = newmax;
	IIGCa_global.gca_acb_array = newarr;
    }

    /* 
    ** Need a new ACB? 
    */
    if ( IIGCa_global.gca_acb_array[ assoc_id ] == NULL )
    {
	IIGCa_global.gca_acb_array[ assoc_id ] = (GCA_ACB *)
						 gca_alloc( sizeof(GCA_ACB) );
	if ( ! IIGCa_global.gca_acb_array[ assoc_id ] )  goto error;
    }

    /* 
    ** Initialize ACB.
    */
    IIGCa_global.gca_acb_active++;
    acb = IIGCa_global.gca_acb_array[ assoc_id ];
    MEfill( sizeof(GCA_ACB), (u_char)0, (PTR)acb );
    acb->assoc_id = assoc_id;
    acb->flags.concatenate = TRUE;

    /*
    ** The type of peer info structure sent is based on
    ** our peer's internal gca version.  When we are the
    ** initiator we don't know our peer's version and we
    ** want to send the current peer info structure.  By
    ** setting the peer info version here we ensure the
    ** correct structure will be sent.
    */
    acb->gca_peer_info.gca_version = GCA_IPC_PROTOCOL;

error:

    /* 
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );

    return( acb );
}


/*{
** Name: gca_find_acb()	- Find the ACB for a given association ID.
**
** Description:
**      gca_find_acb accepts an association ID as input.  It uses the ID to  
**      index into the array of ACBs.
**
** Inputs:
**      assoc_id        Association ID
**
** Outputs:
**      None
**
** Returns:
**	GCA_ACB *	pointer to the ACB, or NULL if invalid ID.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      23-Apr-87 (jbowers)
**          Initial function implementation
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
*/

GCA_ACB *
gca_find_acb( i4  assoc_id )
{
    GCA_ACB             *acb = NULL;

    /*
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    /*
    ** Validate assoc_id, get the ACB, and make sure it is active.
    */
    if ( assoc_id >= 0  &&  assoc_id < IIGCa_global.gca_acb_max )
    {
	acb = IIGCa_global.gca_acb_array[ assoc_id ];
	if ( acb  &&  acb->assoc_id < 0 )  acb = NULL;
    }

    /*
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );

    return( acb );
}


/*{
** Name: gca_next_acb()	- Find the next higher association ID.
**
** Description:
**      gca_next_acb accepts an association ID as input.  Its function is 
**	to return the ID of the association next in the list, i.e., with the
**	next higher association id.  If there is none, a negative value is 
**	returned.
**
** Inputs:
**      assoc_id        Association ID
**
** Outputs:
**      None
**
** Returns:
**	i4		Next Association ID, or -1 if none found.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      10-Aug-87 (jbowers)
**          Initial function implementation
**      05-Nov-87 (jbowers)
**          Fix for loop problem when GCA_Q_ATBL has >1 table.
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
*/

i4
gca_next_acb( i4  assoc_id )
{
    GCA_ACB	**acbp, **acbe;

    /*
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    /*
    ** Scan forward until the end of the list, looking for an active ACB.
    ** Return the assoc_id, or -1 if we hit the end of the list.
    */
    acbp = IIGCa_global.gca_acb_array + (assoc_id + 1);
    acbe = IIGCa_global.gca_acb_array + IIGCa_global.gca_acb_max;

    for( assoc_id = -1; acbp < acbe; acbp++ )
	if ( *acbp  &&  (*acbp)->assoc_id >= 0 )
	{
	    assoc_id = (*acbp)->assoc_id;
	    break;
	}

    /*
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );

    return( assoc_id );
}


/*{
** Name: gca_del_acb()	- Delete the ACB for a given association ID.
**
** Description:
**      gca_del_acb accepts an association ID as input.  It uses the ID to  
**      index into the array of ACBs.  The found ACB is marked unused
**	by setting its assoc_id to -1.
**
** Inputs:
**      assoc_id        Association ID
**
** Outputs:
**      None
**
** Returns:
**	E_DB_OK		ACB found and deleted.
**	E_DB_ERROR	No ACB found for the specified association ID.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      23-Apr-87 (jbowers)
**          Initial function implementation
**	09-Oct-89 (seiwald)
**	    Free memory allocated by GCA_SEND for GCA_TO_DESCR.
**	28-May-91 (seiwald)
**	    Moved in code to find the ACB, since calling gca_find_acb()
**	    results in p()ing the semaphore twice.
**	21-Dec-95 (gordy)
**	    Free the send/recv doc resources in acb.
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
**	 2-May-97 (gordy)
**	    Free aux data buffer.
**	 6-Jun-03 (gordy)
**	    Free additional MIB info data.
**	22-aug-2003 (abbjo03)
**	    On VMS, if called during an AST, do not call MUp_semaphore. Instead,
**	    place the ACB in a free list to be reused after the AST completes.
**	15-Oct-03 (gordy)
**	    Removed VMS kludge from prior submissions which was due to
**	    the server calling FASTSELECT which is not a RESUME operation
**	    and having AST conflicts as a result.  RESUME of FASTSELECT
**	    is now supported at GCA_API_LEVEL_6, so kludge is not needed.
**	9-Sep-2005 (schka24)
**	    Strings that are allocated with STalloc must be freed with
**	    MEfree, not with the gca deallocator.  These are user_id,
**	    client_id, and partner_id.
**	17-Jul-09 (gordy)
**	    Peer user ID's dynamically allocated.
*/

STATUS
gca_del_acb( i4  assoc_id )
{
    GCA_ACB             *acb;
    i4			i;

    /*
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    /* 
    ** Find the acb. 
    */
    if ( assoc_id < 0  ||  assoc_id >= IIGCa_global.gca_acb_max  ||
         ! (acb = IIGCa_global.gca_acb_array[assoc_id]) || acb->assoc_id < 0 )
    {
	MUv_semaphore( &IIGCa_global.gca_acb_semaphore );
	return( E_GC0005_INV_ASSOC_ID );
    }

    /*
    ** Free resources associated with acb.
    */
    if ( acb->flags.mo_attached )
    {
	char assoc_buff[16];
	MOulongout( 0, (u_i8)assoc_id, sizeof(assoc_buff), assoc_buff );
	MOdetach( GCA_MIB_ASSOCIATION, assoc_buff );
    }

    IIGCa_global.gca_acb_active--;
    acb->assoc_id = -1;

    if ( acb->to_descr )  gca_free( (PTR)acb->to_descr );
    if ( acb->buffers )  gca_free( (PTR)acb->buffers );
    if ( acb->gca_aux_data )  gca_free( (PTR)acb->gca_aux_data );
    if ( acb->user_id )  gca_free( (PTR)acb->user_id );
    if ( acb->client_id )  gca_free( (PTR)acb->client_id );
    if ( acb->partner_id )  gca_free( (PTR) acb->partner_id );

    /*
    ** Peer user ID resources can be freed by setting them NULL.
    */
    gca_save_peer_user( &acb->gca_peer_info, NULL );
    gca_save_peer_user( &acb->gca_my_info, NULL );

    if ( acb->recv[ GCA_NORMAL ].doc )  
	gca_free( acb->recv[ GCA_NORMAL ].doc );

    if ( acb->recv[ GCA_EXPEDITED ].doc ) 
	gca_free( acb->recv[ GCA_EXPEDITED ].doc );

    if ( acb->send[ GCA_NORMAL ].doc )  
	gca_free( acb->send[ GCA_NORMAL ].doc );

    if ( acb->send[ GCA_EXPEDITED ].doc ) 
	gca_free( acb->send[ GCA_EXPEDITED ].doc );

    /* 
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );
    return( OK );
}


/*{
** Name: gca_rs_acb()	- Restore acb table for GCA_RESTORE
**
** Description:
**	Takes an assoc_id and builds an otherwise zeroed association array
**	pointing to an ACB with that id.  This supports GCA_RESTORE, which 
**	recovers an association and expects it to keep the same assoc_id.
**
** Inputs:
**      assoc_id	Association ID
**
** Outputs:
**      None
**
** Returns:
**	GCA_ACB *	ACB or NULL if allocation failure
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Sep-89 (seiwald)
**          Written.
**	31-Dec-90 (seiwald)
**	    Made to take an assoc_id and return an ACB, to support
**	    new the GCA_RESTORE which must recreate the ACB.
**	28-jun-91 (kirke)
**	    Allocate correct size ACB pointer table.
**	 3-Nov-95 (gordy)
**	    Removed MCT ifdefs from generic semaphores.
**	 3-Sep-96 (gordy)
**	    Restructured the global GCA data.
**	18-Feb-97 (gordy)
**	    Initialize the concatenation flag as default.
*/

GCA_ACB *
gca_rs_acb( i4  assoc_id )
{
    GCA_ACB	**newarr;
    GCA_ACB	*acb = NULL;

    /*
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    /* 
    ** Allocate the new ACB pointer table. 
    */
    if ( ! (newarr = (GCA_ACB **)
	    gca_alloc( (assoc_id + 1) * sizeof(GCA_ACB *) )) )
	goto error;

    /*
    ** Initialize ACB table.
    */
    IIGCa_global.gca_acb_max = assoc_id + 1;
    IIGCa_global.gca_acb_active = 0;
    IIGCa_global.gca_acb_array = newarr;

    /* 
    ** Allocate and initialize the ACB 
    */
    if ( ! (acb = (GCA_ACB *)gca_alloc( sizeof(GCA_ACB) )) )
	goto error;

    IIGCa_global.gca_acb_active++;
    IIGCa_global.gca_acb_array[ assoc_id ] = acb;
    acb->assoc_id = assoc_id;
    acb->flags.concatenate = TRUE;

  error:

    /*
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );

    return( acb );
}


/*{
** Name: gca_free_acbs() - Free cached acb's
**
** Description:
**	Removes storage for ACB's.  This routine should only
**	be called when all ACB's have been deleted.
**
** History:
**      25-Jan-90 (seiwald)
**          Written.
**	 3-Nov-95 (gordy)
**	    Removed MCT ifdefs from generic semaphores.
**	 3-Sep-96 (gordy)
**	    Added GCA Control Blocks.
*/

VOID
gca_free_acbs( VOID )
{
    GCA_ACB	*acb;
    i4		i;

    /*
    ** If we are operating in a multi-thread (server) environment, we must
    ** protect the ACB table array by obtaining the semaphore permitting
    ** exclusive access, to avoid interference from another thread.
    */
    MUp_semaphore( &IIGCa_global.gca_acb_semaphore );

    if ( IIGCa_global.gca_acb_array )
    {
	for( i = 0; i < IIGCa_global.gca_acb_max; i++ )
	    if ( IIGCa_global.gca_acb_array[ i ] )
		gca_free( (PTR)IIGCa_global.gca_acb_array[ i ] );

	gca_free( (PTR)IIGCa_global.gca_acb_array );
    }

    IIGCa_global.gca_acb_max = 0;
    IIGCa_global.gca_acb_active = 0;
    IIGCa_global.gca_acb_array = NULL;

    /*
    ** Signal that the queue update is complete
    ** by releasing the semaphore.
    */
    MUv_semaphore( &IIGCa_global.gca_acb_semaphore );

    return;
}


/*
** Name: gca_append_aux
**
** Description:
**	Append aux data to the aux data buffer.  The buffer 
**	will be expanded as needed.
**
** Input:
**	acb		Association Control Block
**	length		Length of aux data to be appended.
**	aux		Aux data.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-May-97 (gordy)
**	    Created.
**	 8-Jul-97 (gordy)
**	    Need to update max length when allocating new aux data buffer.
*/

STATUS
gca_append_aux( GCA_ACB *acb, i4  length, PTR aux )
{
    char *buff;

    if ( acb->gca_aux_len + length > acb->gca_aux_max )
    {
	if ( ! (buff = (char *)gca_alloc( acb->gca_aux_len + length )) )
	    return( E_GC0013_ASSFL_MEM );

	if ( acb->gca_aux_data )  
	{
	    if ( acb->gca_aux_len ) 
		MEcopy( (PTR)acb->gca_aux_data, 
			acb->gca_aux_len, (PTR)buff );

	    gca_free( acb->gca_aux_data );
	}

	acb->gca_aux_data = buff;
	acb->gca_aux_max = acb->gca_aux_len + length;
    }

    MEcopy( aux, length, (PTR)(acb->gca_aux_data + acb->gca_aux_len) );
    acb->gca_aux_len += length;

    return( OK );
}


/*
** Name: gca_aux_element
**
** Description:
**	Append an aux data element to the aux data buffer.
**	A GCA_AUX_DATA header is pre-pended to the element.
**
** Input:
**	acb		Association Control Block.
**	type		Aux data lement type.
**	length		Length of aux data element.
**	aux		Aux data element.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-May-97 (gordy)
**	    Created.
*/

STATUS
gca_aux_element( GCA_ACB *acb, i4  type, i4  length, PTR aux )
{
    GCA_AUX_DATA	hdr;
    STATUS		status;

    hdr.len_aux_data = sizeof( hdr ) + length;
    hdr.type_aux_data = type;

    if ( (status = gca_append_aux( acb, sizeof( hdr ), (PTR)&hdr )) == OK )
	status = gca_append_aux( acb, length, aux );

    return( status );
}


/*
** Name: gca_del_aux
**
** Description:
**	Remove the first (possibly only) aux data element.
**
** Input:
**	acb		Association Control Block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 7-May-97 (gordy)
**	    Created.
*/

VOID
gca_del_aux( GCA_ACB *acb )
{
    GCA_AUX_DATA	*hdr =(GCA_AUX_DATA *) acb->gca_aux_data;
    char		*ptr;

    if ( ! hdr  ||  acb->gca_aux_len < sizeof( GCA_AUX_DATA ) )
	acb->gca_aux_len = 0;
    else
    {
	ptr = acb->gca_aux_data + hdr->len_aux_data;
	acb->gca_aux_len -= hdr->len_aux_data;

	if ( acb->gca_aux_len < sizeof( GCA_AUX_DATA ) )
	    acb->gca_aux_len = 0;
	else
	    MEcopy( (PTR)ptr, acb->gca_aux_len, (PTR)acb->gca_aux_data );
    }

    return;
}


/*
** Name: gca_save_peer_user
**
** Description:
**	Manages the memory storage for peer user ID's.
**	A NULL input releases resources.
**
** Input:
**	peer	Peer info.
**	user	User ID.
**
** Output:
**	peer
**	    gca_user_id		Saved user ID.
**
** Returns:
**	void.
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

VOID
gca_save_peer_user( GCA_PEER_INFO *peer, char *user )
{
    if ( peer->gca_user_id )  gca_free( (PTR)peer->gca_user_id );
    peer->gca_user_id = user ? gca_str_alloc( user ) : NULL;
    return;
}


