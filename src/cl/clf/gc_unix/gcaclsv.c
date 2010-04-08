/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <me.h>
#include    <gl.h>
#include    <qu.h>
#include    <me.h>
#include    <gc.h>

#include    <bsi.h>

#include    "gcacli.h"
#include    "gcarw.h"

/**
**
**  Name: GC.C - GCA private CL routines
**
**  Description:
**
**          GCsave - Save system-dependent data structures
**          GCrestore - Restore system-dependent data structures
**
** History:
**	01-Jan-90 (seiwald)
**	    Removed setaside buffer.
**      24-May-90 (seiwald)
**          Built upon new BS interface defined by bsi.h.
**	    New BS interface include save/restore entry points.
**	06-Jun-90 (seiwald)
**	    Made GCalloc() return the pointer to allocated memory.
**	18-Jun-90 (seiwald)
**	    GCsave & GCrestore lng_user_data is a i4.
**	30-Jun-90 (seiwald)
**	    Private and Lprivate renamed to bcb and lbcb, as private
**	    is a reserved word somewhere.
**	2-Jan-91 (seiwald)
**	    Moved the bulk of the save/restore operation into GCA mainline.
**	6-Apr-91 (seiwald)
**	    Be a little more assiduous in clearing svc_parms->status on
**	    function entry.
**	18-Jun-91 (seiwald)
**	    Removed sanity check in GCsave which ensured that GC_send_sm 
**	    and GC_recv_sm were idle.  The idea was that if data were
**	    buffered before a process switch the line protocol would be
**	    corrupted.  In practice the only data ever buffered were
**	    chop marks, left around from an endretrieve.  This harmless 
**	    purging cruft made GCsave considered its state insane.  
**	    Ignorance is now once again bliss.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name: GC_SAVE_DATA - structure for passing GC association between processes
**
** Description:
**	Connection information can be passed between parent/child 
**	processes by GCsave and GCrestore.  This structure describes 
**	the GC level information which must be passed.
**
**	Note that this structure incorporates the BCB wholly.  BS
**	is responsible for maintaining its own compatibility across 
**	versions.
**
**	GC_SAVE_LEVEL_MAJOR must be changed if incompatible changes are
**	made to this structure; this will cause GCrestore to reject
**	the connection.
**
**	GC_SAVE_LEVEL_MINOR can be changed when minor, compatible,
**	changes are made to this structure.
**
** History:
**	31-Dec-90 (seiwald)
**	    Created.
*/

# define	GC_SAVE_LEVEL_MAJOR	0x0604
# define	GC_SAVE_LEVEL_MINOR	0x0001

typedef struct _GC_SAVE_DATA
{
    short		save_level_major;
    short		save_level_minor;
    u_i4		id;
    char		bcb[ BS_MAX_BCBSIZE ];
} GC_SAVE_DATA;

GLOBALREF       BS_DRIVER       *GCdriver;

/*
** Restrictions:
**	Association must be idle before save:
**		Sender/receiver state machines idle.
**		Outstanding send/receive requests will be cancelled
**			by GCrestore().
*/


/*{
** Name: GCsave	- Save data for process switch
**
** Description:
**	The caller passes a block of user data and an ACB to be saved, and a 
**	"save name" identifying this group of saved data for subsequent 
**	recovery.  The ACB, the LCB and the user data are saved in a system-
**	dependent way, and are tagged with the save name in a way that will 
**	allow GCrestore to recover the data when passed the same save name.
**	GCsave must save any system-dependent substructures associated with 
**	the LCB, if required.
**
** Inputs:
**	    user_data		A poiner to a block of user data to be saved.
**	    lng_user_data	Length of the user data block.
**	    gc_cb		Our GCA_GCB
**	    save_name		A pointer to unique identifier for this save
***				operation
**
** Outputs:
**	    save_name		The name of a save area to be used by GCrestore.
**
**	Returns:
**	    STATUS		Success or failure indication
**		OK		Normal completion
**		GC_SAVE_FAIL	A system-dependent failure occurred
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-June-87 (jorge [J. Noa])
**          Initial function creation
**	8-jun-93 (ed)
**	    added VOID for prototypes
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
*/

VOID
GCsave( svc_parms )
SVC_PARMS	*svc_parms;
{
    GCA_GCB		*gcb = (GCA_GCB *)svc_parms->gc_cb;
    GC_SAVE_DATA	*save_data = (GC_SAVE_DATA *)svc_parms->svc_buffer;

    svc_parms->status = OK;

    /* Get size of buffer */

    if( svc_parms->reqd_amount < sizeof( *save_data ) )
    {
	GCTRACE(0)( "GCsave: buffer too small\n" );
	svc_parms->status = GC_SAVE_FAIL;
	return;
    }

    svc_parms->rcv_data_length = sizeof( *save_data );

    /* Note idleness of send/receive state machines. */

    if( gcb->send.state || gcb->recv.state )
    {
	GCTRACE(1)("GCsave: line not idle (%d %d)\n",
		gcb->send.state, gcb->recv.state );
    }

    /* Allow BS to save state. */

    if( GCdriver->save )
    {
	BS_PARMS	bsp;
	CL_ERR_DESC	sys_err;

	bsp.syserr = &sys_err;
	bsp.bcb = gcb->bcb;

	GCTRACE(1)( "GCsave: calling BSsave\n" );

	(*GCdriver->save)( &bsp );

	if( bsp.status != OK )
	{
	    GCTRACE(0)( "GCsave: BSsave failed %x\n", bsp.status );
	    svc_parms->status = GC_SAVE_FAIL;
	    return;
	}
    }

    /* Plop user data into save struct */

    save_data->save_level_major = GC_SAVE_LEVEL_MAJOR;
    save_data->save_level_minor = GC_SAVE_LEVEL_MINOR;
    save_data->id = gcb->id;
    MEcopy( gcb->bcb, sizeof( gcb->bcb ), save_data->bcb );
}


/*{
** Name: GCrestore	- Restore data following process switch
**
** Description:
**	This routine recovers data previously saved by GCsave();  The invoker
**	passes a "save name" to identify the saved data.  This identifier was
**	used by GCsave to "tag" the saved data for subsequent recovery.
**	User data, if any, the ACB for an association and the related LCB are
**	recovered.  The LCB pointer in the ACB is set to the recovered LCB, and
**	any system-specific substructures related to the LCB are either 
**	recovered or allocated as necessary.
**	Pointers to the user data and the ACB are returned to the invoker.
**
** Inputs:
**	    save_name		A unique identifier for this save operation
**
** Outputs:
**	    lng_user_data	Length of the user data block.
**	    user_data		A poiner to a block of recovered user data.
**	    gc_cb		Our GCA_GCB.
**
**	Returns:
**	    STATUS		    success or failure indication
**		OK		    Normal completion
**		GC_BAD_SAVE_NAME    Save name is not recognized; no previous
**				    GCsave was done with that name.
**		GC_RESTORE_FAIL	    A system-dependent failure occurred, 
**				    preventing successful completion.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-June-87 (jorge [J. Noa])
**          Initial function creation
**	10-Apr-97 (gordy)
**	    ACB removed from service parms, replaced by GC control block.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

VOID
GCrestore( svc_parms )
SVC_PARMS	*svc_parms;
{
    GC_SAVE_DATA	*save_data = (GC_SAVE_DATA *)svc_parms->svc_buffer;
    GCA_GCB		*gcb;

    svc_parms->status = OK;

    /* Check for proper save block. */

    if( save_data->save_level_major != GC_SAVE_LEVEL_MAJOR )
    {
	svc_parms->status = GC_BAD_SAVE_NAME;
	return;
    }

    /* Allocate GCB */

    if( !( gcb = (GCA_GCB *)(*GCalloc)( sizeof( *gcb ) ) ) )
    {
	svc_parms->status = GC_RESTORE_FAIL;
	return;
    }

    MEfill( sizeof( *gcb ), 0, (PTR)gcb );

    /* Copy parts of gcb only. */

    svc_parms->gc_cb = (PTR)gcb;
    gcb->id = save_data->id;
    MEcopy( save_data->bcb, sizeof( gcb->bcb ), gcb->bcb );

    /* Allow BS to restore state. */

    if( GCdriver->restore )
    {
	BS_PARMS	bsp;
	CL_ERR_DESC	sys_err;

	bsp.syserr = &sys_err;
	bsp.bcb = gcb->bcb;

	GCTRACE(1)( "GCrestore: calling BSrestore\n" );

	(*GCdriver->restore)( &bsp );

	if( bsp.status != OK )
	{
	    GCTRACE(0)("GCrestore: BSrestore failed %x\n", bsp.status );
	    svc_parms->status = GC_RESTORE_FAIL;
	}
    }
}
