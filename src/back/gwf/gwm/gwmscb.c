/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <erglf.h>
# include <sp.h>
# include <mo.h>
# include <cs.h>
# include <st.h>
# include <tr.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <adf.h>
# include <scf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <ulf.h>
# include <ulm.h>
# include <gwf.h>
# include <gwfint.h>
# include <gca.h>
# include <gcm.h>

# include "gwmint.h"

/**
** Name:	gwmscb.c	- GWM session level stuff
**
** Description:
**	This file defines things that operate on the session
**	level for GWM.
**
**	At present, the only thing here is
**	maintenance of the per-session vnode domain. This is
**	list of vnodes the user is interested in seeing in GWM
**	queries.  The vnodes in the list must be reachable by
**	GCA.   There must be an authorization entry for the
**	"ingres" user, as that will be used for remote connections.
**
**	Memory is obtained with the FIXME allocator.
**
**	GM_scb_init	- initialize GWM session stuff.
**	GM_rm_scb	- remove GWM session stuff.
**	GM_domain	- get[next] element in user domain.
*
** Internal:
**	GM_ad_domain	- add vnode to domain for a session
**	GM_rm_domain	- remove vnode from domain for a session.
**	GM_gt_scb	- locate GM_SCB for this session.
**
** History:
**	6-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    add includes of <gca.h> and <gcm.h> for the things in
**	    the connection block in gwmplace.h.  Handle no-session
**	    differently from no domain; hand right args to get
**	    current session.
**	13-Nov-1992 (daveb)
**	    fix numerous semaphore protocol errors.
**	2-Dec-1992 (daveb)
**	    Add GM_domain() to get user domain items; make most
**	    everything else internal.  Add GM_DEFAULT_DOM_STR when
**	    scb is created.
**	3-Dec-1992 (daveb)
**	    Get default domain from GM_globals.
**	3-Feb-1993 (daveb)
**	    improve documentation.
**	10-Mar-1993 (daveb)
**	    Turn on perms for session objects ONLY when session guise
**	    is claimed.  If asked for remotely (when no session is valid),
**	    then we sometimes get back garbage session id's, which cause
**	    SEGVs and the like.  So don't allow them to ask.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Remove GM_gt_gw_sess() and replace with calls to gws_gt_gw_sess().
**	20-Aug-1993 (daveb)
**	    Reset back to default domain, not empty one.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in GM_gt_scb().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**/

/* forwards */

static MO_SET_METHOD GM_adddomset;
static MO_SET_METHOD GM_deldomset;
static MO_SET_METHOD GM_resdomset;

static MO_INDEX_METHOD GM_domain_index;

static void GM_nuke_domain( GM_SCB *scb );
static DB_STATUS GM_ad_domain( char *vnode );
static DB_STATUS GM_rm_domain( char *vnode );

static GM_SCB *GM_gt_scb(void);



/* MIB for domains */

static char domain_index[] = "exp.gwf.gwm.session.dom.index";

static MO_CLASS_DEF GM_scb_classes[] =
{
    /* control objects - work on current session only, so only allow
       to be read by things with session permission.  This turns
       them off completely for remote operations, since GCM never
       claims the "session" guises.  */

    { 0, "exp.gwf.gwm.session.control.add_vnode",
	  0, MO_SES_READ|MO_SES_WRITE, 0,
	  0, MOblankget, GM_adddomset,
	  0, MOcdata_index },

    { 0, "exp.gwf.gwm.session.control.del_vnode",
	  0, MO_SES_READ|MO_SES_WRITE, 0,
	  0, MOblankget, GM_deldomset,
	  0, MOcdata_index },

    { 0, "exp.gwf.gwm.session.control.reset_domain",
	  0, MO_SES_READ|MO_SES_WRITE, 0,
	  0, MOblankget, GM_resdomset,
	  0, MOcdata_index },

    /* query objects -- only column is index of nodes for this session */

    { MO_INDEX_CLASSID|MO_CDATA_INDEX, domain_index,
	  MO_SIZEOF_MEMBER(SPBLK, key), MO_SES_READ, 0,
	  CL_OFFSETOF(SPBLK, key), MOstrpget, MOnoset,
	  0, GM_domain_index },

    { 0 }
} ;


/*{
** Name:	GM_scd_startup	- define SCB MIB clases.
**
** Description:
**	Call this once at facility startup to define all the SCB
**	related classes.
**
** Re-entrancy:
**	yes, but shouldn't be called more than once.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	MOclassdef result.
**
** History:
**	17-Oct-1992 (daveb)
**	    created.
*/

STATUS
GM_scb_startup(void)
{
    return( MOclassdef( MAXI2, GM_scb_classes ) );
}



/*{
** Name:	GM_rm_scb	- remove GWM scb for current session.
**
** Description:
**	Clear out the SCB for the session.
**
** Re-entrancy:
**	worry about sharing memory_left counters with parent streams.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK		if all went well.
**	E_DB_ERROR	if allocation error occured; it is logged.
**
** History:
**	7-oct-92 (daveb)
**	    created.
*/
DB_STATUS
GM_rm_scb(void)
{
    GM_SCB	*scb;
    DB_STATUS	db_stat = E_DB_OK;

    do				/* one-time through */
    {
	scb = GM_gt_scb();
	if( scb == NULL )
	{
	    GM_error( E_GW82C1_NO_SCB );
	    db_stat = E_DB_ERROR;
	    break;
	}

	if( GM_gt_sem( &scb->gs_sem ) != E_DB_OK )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}
	GM_nuke_domain( scb );
	GM_release_sem( &scb->gs_sem );
	    
	/* close stream and delete all mem in it */
	CSr_semaphore( &scb->gs_sem );

    } while ( FALSE );

    return( db_stat );
}



/*{
** Name:	GM_domain	- locate first/next element in user domain.
**
** Description:
**	Returns either the first or the next item in the user's per-session
**	MIB domain.  If the user SCB doesn't yet exist returns default or
**	NULL.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	this		domain to find successor to, or NULL for first one.
**
** Outputs:
**	none
**
** Returns:
**	item in the domain, or NULL.
**
** History:
**	2-Dec-1992 (daveb)
**	    created.
**	12-Aug-1993 (daveb)
**	    Never return NULL when this == NULL and next is true.
**	    Return default domain there's no scb, or when the
**	    domain tree has been emptied by GM_nuke_domain().
*/

STATUS
GM_domain( i4  next, char *this, char **new )
{
    STATUS	cl_stat = OK;
    SPBLK	lookup;
    SPBLK 	*pb = NULL;
    GM_SCB	*scb = GM_gt_scb();
    i4		got_sem = FALSE;
    char	vbuf[ GM_MIB_PLACE_LEN ];
    char	*answer = NULL;

    do
    {
	if( scb == NULL )
	{
	    answer = ( this != NULL && next ) ?
		NULL : GM_globals.gwm_def_domain;
	    break;
	}
	if( (cl_stat = GM_gt_sem( &scb->gs_sem )) != OK )
	    break;
	got_sem = TRUE;
	
	if( this == NULL )	/* always gets something */
	{
	    if( next )
	    {
		pb = SPfhead( &scb->gs_domain );
		answer = pb ? (char *)pb->key : GM_globals.gwm_def_domain;
	    }
	    else		/* Bad call: (this == NULL && next = 0 )*/
	    {
		GM_breakpoint();
		answer = GM_globals.gwm_def_domain;
	    }
	}
	else
	{
	    lookup.key = this;
	    pb = SPlookup( &lookup, &scb->gs_domain );
	    if( pb == NULL )
	    {
		/* maybe it's a server in a vnode we have */
		GM_just_vnode( this, sizeof(vbuf), vbuf );
		if( vbuf[0] != EOS )
		{
		    lookup.key = vbuf;
		    pb = SPlookup( &lookup, &scb->gs_domain );		    
		}
	    }
	    if( pb != NULL && next )
		pb = SPfnext( pb );

	    answer = (pb != NULL) ? pb->key : NULL;
	}

    } while( FALSE );

    if( got_sem )
	GM_release_sem( &scb->gs_sem );

    *new = answer;

    return( cl_stat );
}



/*{
** Name:	GM_scbinit	- init GWM scb for current session.
**
** Description:
**	Allocate and save the GM_SCB for this session.
**
** Re-entrancy:
**	worry about sharing memory_left counters with parent streams.
**
** Inputs:
**	gw_session	the session of interest.
**	scbp		pointer to SCB to fill in.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK		if all went well.
**	E_DB_ERROR	if allocation error occured; it is logged.
**
** History:
**	7-oct-92 (daveb)
**	    created.
**	20-Aug-1993 (daveb)
**	    bigger buffer for sem name to handle long places (on VMS).
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
static GM_SCB *
GM_gt_scb( void )
{
    GW_SESSION	*gw_sess;
    GM_SCB	*scb = NULL;
    char	sem_name[ CS_SEM_NAME_LEN + GM_MIB_PLACE_LEN ];

    do
    {
	if( (gw_sess = gws_gt_gw_sess()) == NULL )
	    break;

	if( (scb = (GM_SCB *)gw_sess->gws_exit_scb[ GW_IMA ]) != NULL )
	    break;

	/* need to create a new one */

	scb = (GM_SCB *)GM_ualloc( sizeof(*scb),
				  &gw_sess->gws_ulm_rcb,
				  &Gwf_facility->gwf_tcb_lock );
	if( scb == NULL )
	    break;

	/* Got it, now set it all up. */

	gw_sess->gws_exit_scb[ GW_IMA ] = (PTR)scb;
	GM_i_sem( &scb->gs_sem );
	CSn_semaphore( &scb->gs_sem,
		      STprintf( sem_name, "GM sess %p", scb ));
	SPinit( &scb->gs_domain, STcompare );

	/* FIXME -- should be SERVER */
	if( GM_ad_domain( GM_globals.gwm_def_domain ) != E_DB_OK )
	{
	    /* "GWM: Internal error:  error starting domain for
	       session %0x, scb %1x" */

	    GM_2error( (GWX_RCB *)0, E_GW82C2_NO_DOMAIN_START, 
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( gw_sess ), (PTR)gw_sess,
		      sizeof(scb), (PTR)scb );
	}

    } while (FALSE );

    return( scb );
}

/*{
** Name:	GM_nuke_domain	- reset entire domain to nothing.
**
** Description:
**	Removes all the vnodes in the domain.
**
** Re-entrancy:
**	must have the sem on the SCB set on entry.
**
** Inputs:
**	scb		the GWM session being worked on.
**
** Outputs:
**	scb->gs_domain	is cleared of nodes.
**
** Returns:
**	none.	
**
** History:
**	17-Oct-1992 (daveb)
**	    created.
*/
static void
GM_nuke_domain( GM_SCB *scb )
{
    SPBLK 	*sp;

    while( (sp = SPfhead( &scb->gs_domain )) != NULL )
    {
	SPdelete( sp, &scb->gs_domain );
	GM_free( (PTR)sp );
    }
}



/*{
** Name:	GM_ad_domain	- add thing to the domain
**
** Description:
**	Adds a non-duplicate domain entry; a duplicate is a silent no-op.
**
** Re-entrancy:
**	yes, assuming the semaphore on the domain tree is properly acquired.
**
** Inputs:
**	thing		the place string in question.
**
** Outputs:
**	none.
**
** Returns:
**	E_DB_OK		if the operation succeeded,
**	E_DB_ERROR	if badness happened; details logged.
**
** History:
**	6-oct-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    don't error if no scb, create it.
**	2-Dec-1992 (daveb)
**	    allocate space for it here, not from place key.
**	3-Dec-1992 (daveb)
**	    make server disappearance work.
*/
static DB_STATUS
GM_ad_domain( char *thing )
{
    DB_STATUS	db_stat = E_DB_OK;
    GM_SCB	*scb;
    SPBLK	lookup;
    SPBLK	*sp;
    SPBLK	*nsp;
    bool	gotsem = FALSE;
    char	vbuf[ GM_MIB_PLACE_LEN ];
    char	sbuf[ GM_MIB_PLACE_LEN ];

    do				/* one time loop */
    {
	scb = GM_gt_scb();
	if( scb == NULL )
	    break;

	/* Map magic constants into real places immediately */

# if 0
	if( STequal( thing, GM_DOM_VNODE_STR ) )
	    thing = GM_my_vnode();
	else if( STequal( thing, GM_DOM_SERVER_STR ) )
	    thing = GM_my_server();
# endif

	if( GM_gt_sem( &scb->gs_sem ) != OK )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}
	gotsem = TRUE;
	lookup.key = (PTR)thing;
        sp = SPlookup( &lookup, &scb->gs_domain );
	if( sp == NULL )
	{
	    sp = (SPBLK *)GM_alloc( sizeof(*sp) + STlength(thing) + 1 );
	    if( sp == NULL )
	    {
		db_stat = E_DB_ERROR;
		break;
	    }
	    sp->key = (PTR)((char *)sp + sizeof(*sp));
	    STcopy( thing, (char *)sp->key );
	    (void) SPinstall( sp, &scb->gs_domain );
	}

	/* if this is a vnode, delete any server entries for it
	   that follow in the tree. */

	GM_just_vnode( thing, sizeof(vbuf), vbuf );
	if( vbuf[0] != EOS )
	    break;

	while( (nsp = SPfnext(sp)) != NULL )
	{
	    GM_just_vnode( nsp->key, sizeof( sbuf ), sbuf );
	    if( STcompare( thing, sbuf ) )
		break;
		
	    SPdelete( nsp, &scb->gs_domain );
	    GM_free( (PTR)nsp );
	}

    } while( FALSE );
    
    if( gotsem )
	(void) GM_release_sem( &scb->gs_sem );

    return( db_stat );
}

    
/*}
** Name:	GM_rm_domain	- remove domain entry for a session
**
** Description:
**	Removes domain entry; non-existant thing is a silent no-op.
**
** Re-entrancy:
**	yes, assuming the semaphore on the domain tree is properly acquired.
**
** Inputs:
**	sess_id		session id in question, not necessarily this one.
**	thing		the thing to remove.
**
** Outputs:
**	none
**
** Returns:
**	E_DB_OK		if the operation succeeded,
**	E_DB_ERROR	if badness happened; details logged.
**
** History:
**	7-oct-92 (daveb)
**	    created.
*/

static DB_STATUS
GM_rm_domain( char *thing )
{
    GM_SCB	*scb;
    DB_STATUS	db_stat = E_DB_OK;
    SPBLK	lookup;
    SPBLK	*sp;
    bool	gotsem = FALSE;

    do
    {
	/* Map magic constants into real places immediately */

# if 0
	if( STequal( thing, GM_DOM_VNODE_STR ) )
	    thing = GM_my_vnode();
	else if( STequal( thing, GM_DOM_SERVER_STR ) )
	    thing = GM_my_server();
# endif

	scb = GM_gt_scb();
	if( scb == NULL )
	{
	    GM_error( E_GW82C1_NO_SCB );
	    db_stat = E_DB_ERROR;
	    break;
	}

	if( GM_gt_sem( &scb->gs_sem ) != OK )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}
	gotsem = TRUE;
	lookup.key = (PTR)thing;
	sp = SPlookup( &lookup, &scb->gs_domain );
	if( sp != NULL )
	{
	    SPdelete( sp, &scb->gs_domain );
	    GM_free( (PTR)sp );
	}

    } while( FALSE );
    
    if( gotsem )
	(void) GM_release_sem( &scb->gs_sem );

    return( db_stat );
}



/*{
**  Name:	GM_adddomset - control object to add thing to user domain.
**
**  Description:
**
**	Add the thing to the CURRENT session's domain.
**
**	There is, at present, no way for this to work from
**	another session.  Doing so would require scf_call to
**	take a session_id as an input param for the SCU_INFORMATION
**	operation.
**
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		the length of the user buffer, ignored.
**	 userbuf
**		the user string to convert.
**	 objsize
**		the size of the output buffer.
**	 object
**		the session id.
**  Outputs:
**	 object
**		gets a copy of the input string.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer wasn't big enough.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	11-Nov-1992 (daveb)
**	    handle no session different from no domain..
*/

static STATUS 
GM_adddomset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS cl_stat = MO_NO_INSTANCE;
    
    if( gws_gt_gw_sess() != NULL )
	cl_stat = GM_ad_domain( sbuf ) == E_DB_OK ? OK : MO_NO_INSTANCE;
    return( cl_stat );
}



/*{
**  Name:	GM_deldomset - control object to delete thing to user domain.
**
**  Description:
**
**	Delete the thing from the CURRENT session's domain.
**
**	There is, at present, no way for this to work from
**	another session.  Doing so would require scf_call to
**	take a session_id as an input param for the SCU_INFORMATION
**	operation.
**
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		the length of the user buffer, ignored.
**	 userbuf
**		the user string to convert.
**	 objsize
**		the size of the output buffer.
**	 object
**		ignored
**  Outputs:
**	 object
**		gets a copy of the input string.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer wasn't big enough.
**  History:
**	23-sep-92 (daveb)
**	    documented
** History:
**	11-Nov-1992 (daveb)
**	    handle no session different from no domain..
*/
static STATUS 
GM_deldomset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS	cl_stat = MO_NO_INSTANCE;

    if( gws_gt_gw_sess() != NULL )
	cl_stat = GM_rm_domain( sbuf ) == E_DB_OK ? OK : MO_NO_INSTANCE;

    return( cl_stat );
}



/*{
** Name:	GM_resdomset	- MO set function to reset user domain.
**
** Description:
**	Clears out any existing user domain.  Returns NO_INSTANCE
**	if not running in the context of a session.
**
** Re-entrancy:
**	yes.
**
**  Inputs:
**	 offset
**		ignored.
**	 luserbuf
**		the length of the user buffer, ignored.
**	 userbuf
**		the user string to convert.
**	 objsize
**		the size of the output buffer.
**	 object
**		ignored
**  Outputs:
**	 object
**		gets a copy of the input string.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		if the output buffer wasn't big enough.
** History:
**	11-Nov-1992 (daveb)
**	    documented, handle no session different from no domain..
**	3-Feb-1993 (daveb)
**	    improve documentation.
**	20-Aug-1993 (daveb)
**	    Reset back to default domain, not empty one.
*/

static STATUS 
GM_resdomset( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    GM_SCB	*scb;
    STATUS	cl_stat = MO_NO_INSTANCE;
    GW_SESSION	*gw_sess;

    do				/* one-time through */
    {
	/* not in a session.. */
	if( (gw_sess = gws_gt_gw_sess()) == NULL )
	    break;
	
	/* no domain present */
	scb = GM_gt_scb();
	if( scb == NULL )
	{
	    cl_stat = OK;
	    break;
	}

	if( (cl_stat = GM_gt_sem( &scb->gs_sem )) != OK )
	    break;

	GM_nuke_domain( scb );
	GM_release_sem( &scb->gs_sem );
	if( GM_ad_domain( GM_globals.gwm_def_domain ) != E_DB_OK )
	{
	    /* "GWM: Internal error:  error starting domain for
	       session %0x, scb %1x" */

	    GM_2error( (GWX_RCB *)0, E_GW82C2_NO_DOMAIN_START, 
		      GM_ERR_INTERNAL|GM_ERR_LOG,
		      sizeof( gw_sess ), (PTR)gw_sess,
		      sizeof(scb), (PTR)scb );
	}

    } while ( FALSE );

    return( cl_stat );
}

/*{
** Name:	GM_domain_index	- user domain index
**
** Description:
**	MO index method for the user domain.
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT.
**	cdata		the current domain string.
**	linstance	length of instance buffer.
**	instance	the instance string
**
** Outputs:
**	instance	filled in with new domain string
**	instdata	stuffed with saved instance data, a GM_PLACE_BLK
**
** Returns:
**	OK
**	MO_BAD_MSG
**	MO_NO_INSTANCE
**	MO_NO_NEXT
**	MO_INSTANCE_TRUNCATED
**	MO_VALUE_TRUNCATED
**
** History:
**	3-Feb-1993 (daveb)
**	    documented.
**	6-Mar-2006 (kschendel)
**	    Fix bug 112081, cl_stat was reset after initialization
**	    so proper error code wasn't returned.
*/

static STATUS 
GM_domain_index(i4 msg,
		PTR cdata,
		i4 linstance,
		char *instance, 
		PTR *instdata )
{
    STATUS	cl_stat;
    SPBLK	lookup;
    SPBLK	*sp;
    GM_SCB	*scb;
    bool	gotsem = FALSE;

    do
    {
	scb = GM_gt_scb();
	if( scb == NULL )
	    break;
    
	if( cl_stat = GM_gt_sem( &scb->gs_sem ) != OK )
	    break;
	gotsem = TRUE;
	lookup.key = instance;
	cl_stat = MO_NO_INSTANCE;
	sp = SPlookup( &lookup, &scb->gs_domain );
	
	switch( msg )
	{
	case MO_GET:
	    
	    if( sp != NULL )
		cl_stat = OK;
	    break;
	    
	case MO_GETNEXT:
	    
	    if( sp != NULL )
		sp = SPfnext( sp );
	    else if( *instance == EOS  )
		sp = SPfhead( &scb->gs_domain );
	    else
		break;
	    
	    if( sp == NULL )
		cl_stat = MO_NO_NEXT;
	    else
		cl_stat = MOstrout( MO_INSTANCE_TRUNCATED,
				   sp->key, linstance, instance );
	    break;
	    
	default:
	    
	    cl_stat = MO_BAD_MSG;
	    break;
	}
	
	*instdata = (PTR)sp;
    } while( FALSE );

    if( gotsem )
	(void) GM_release_sem( &scb->gs_sem );

    return( cl_stat );
}

