/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include 	<ci.h>
# include 	<cv.h>
# include	<gc.h>
# include	<me.h>
# include 	<mh.h>
# include	<qu.h>
# include	<si.h>
# include	<st.h>
# include	<tm.h>
# include	<tr.h>

# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<gcaint.h>
# include	<gcn.h>
# include	<gcnint.h>
# include	<gcu.h>

/*
** Name: gcnstick.c - routines to create and use tickets
**
** Description:
**    External routines
**	gcn_make_tickets() - create authorization tickets on server
**	gcn_save_rtickets() - cache tickets on client
**	gcn_use_rticket() - get and decrypt a ticket on the client
**	gcn_use_lticket() - validate and dispose of a ticket on server
**	gcn_drop_tickets() - drop local/remote tickets
**
**    Internal routines
**	gcn_new_ticket() - generate an lticket, rticket pair on server
**	gcn_dec_rticket() - decrypt a rticket into an lticket on client
**	gcn_chk_lticket() - validate lticket and upgrade
**	gcn_upgrade_ticket() - upgrade tickets to server version
**	gcn_build_key() - build key for CIencode()
**	gcn_get_instpwd() - get the password for a installation
**	gcn_expire_tickets() - discard old tickets
**
** History:
**	19-Jul-91 (seiwald)
**	    Created.
**	23-Mar-92 (seiwald)
**	    Installation password error message shuffled.
**	23-Mar-92 (seiwald)
**	    Defined GCN_GLOBAL_USER as "*" to help distinguish the owner
**	    of global records from other uses of "*".
**	23-Mar-92 (seiwald)
**	    Moved GCN_RTICKET, GCN_LTICKET to gcn.h.
**	    Headers added.
**	23-Mar-92 (seiwald)
**	    Hand in length of ticket buffers to gcn_make_tickets() and
**	    gcn_save_tickets() so that client code needn't know their
**	    exact size.
**	24-Mar-92 (seiwald)
**	    An rticket's time-to-live is appended before being returned, 
**	    so that its expiration time may be determine when generated,
**	    rather than relying on GCN_EXPIRE_LTICKET on the server being
**	    larger than GCN_EXPIRE_RTICKET on the client.
**	26-Mar-92 (seiwald)
**	    Installation password now just a GCN_LOGIN entry with user "*".
**	1-Apr-92 (seiwald)
**	    Documented.
**	17-apr-92 (seiwald)
**	    Generate raw tickets that are binary (rather than hex) and
**	    exactly 8 bytes long.
**	29-Sep-92 (brucek)
**	    New argument for gcn_nq_add.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      07-Dec-94 (liibi01)
**          Cross integration from 6.4 (Bug 54145). When a connection is made
**          gcn_use_lticket() is called by the server-end GCN. First, old
**          tickets are expired, then the lookup to match the incoming ticket
**          is done. But if the incoming ticket is expired, this sequence could
**          lead to E_GC0143. Instead, the expiry is performed after the lookup
**          has found/deleted the incoming ticket from its queue. This allows
**          an extension of the expiry window for the incoming connection. 
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Added prototypes.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	27-May-97 (thoda04)
**	    Included cv.h for the function prototypes.  WIN16 fixes.
**	 4-Dec-97 (rajus01)
**	    Moved GCN_TICK_MAG, GCN_L_TICKET defines to gcnint.h.
**	    GCN_L_TICK_MAG define no longer exists. Updated
**	    gcn_new_ticket(), gcn_dec_rticket() prototypes.
**	17-Feb-98 (gordy)
**	    Extended GCM configuration capabilities and added additional
**	    parameters for GCM instrumentation.
**	19-Feb-98 (gordy)
**	    Use gcn_nq_scan(), gcn_nq_qdel() for better performance.
**	15-May-98 (gordy)
**	    Don't allow tickets which could act like wildcards.
**	10-Jun-98 (gordy)
**	    Bulk ticket expiration moved to background processing.
**	    Using a ticket must now also check for individual ticket
**	    expiration just in case bulk processing not done recently.
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
**	10-Jul-98 (gordy)
**	    Made the tracing better for invalid tickets.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Jun-04 (gordy)
**	    Raw ticket info is obfuscated via encryption to hide the
**	    underlying random number generator.
**	09-Jul-2004 (wansh01)
**	    Added gcn_drop_tickets() to support admin DROP command
**	15-Jul-04 (gordy)
**	    Enhanced encryption of stored passwords.
**	16-Jul-04 (gordy)
**	    Added new versions of tickets which vary server/client and
**	    client/server so that both are encrypted (differently).
**	28-Sep-04 (gordy)
**	    Fixed retrieval/decoding of installation passwords which
**	    were stored in enhanced format.
**	26-Oct-05 (gordy)
**	    Ensure buffers are large enough to hold passwords,
**	    encrypted passwords and authentication certificates.
**      25-jan-2006 (loera01) SIR 115667
**          In gcn_make_tickets(), reduce the interval required for
**          checking for expired tickets.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/

/*
**  Forward and/or External function references.
*/

static STATUS	gcn_new_ticket( char *, char *, char *, i4 );
static STATUS	gcn_dec_rticket( char *, char *, u_i1 *, i4 * );
static STATUS	gcn_chk_lticket( u_i1 *, i4, char * );
static i4	gcn_upgrade_ticket( u_i1 *, i4, u_i1 * );
static VOID	gcn_build_key( char *, char * );
static STATUS	gcn_get_instpwd( char *, i4, char ** );
static i4	gcn_expire_tickets( GCN_QUE_NAME * );

/* 
** CRYPT_SIZE is missing from <ci.h> 
*/

# ifndef CRYPT_SIZE
# define CRYPT_SIZE 8
# endif

# define	RVEC_MAX	5	/* May have expired tickets */
# define	LVEC_MAX	1	/* Only expect 1 matching ticket */


/*
** Name: gcn_make_tickets() - create authorization tickets on server
**
** Description:
**	Given a user name and client installation, generates and
**	returns a small number of tickets which can each be used once
**	as a password for this installation.  To be used, these tickets
**	must first be decrypted by a Name Server in an installation
**	knowing the installation password.
**
**	This routine returns the generated rtickets to the caller;
**	each ticket has appended its time-to-live in seconds, which gets
**	translated into an expiration time by gcn_save_rtickets().
**
**	This routine stores the generated ltickets on the LTICKET
**	queue; each lticket has an expiration time appended.
**
**	The ltickets are stored as:
**
**		queue:	"lticket"
**		uid:	user
**		obj:	*
**		val:	lticket,expiration
**
** Inputs:
**	user		make tickets for this user
**	inst		encode with this (client's) installation's password
**	rtickvec	put text of tickets here
**	l_rtickvec	make this many tickets
**	l_rticket	each ticket at most this length
**
** Returns:
**	STATUS
**
** History:
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough to hold password.
**	21-Jul-09 (gordy)
**	    Declare standard sized buffer for password.  Use dynamic
**	    storage if length exceeds default size.  Free resources
**	    if allocated in gcn_get_instpwd().
*/

STATUS
gcn_make_tickets
(
    char	*user, 
    char	*inst,
    char	*rtickvec,
    i4		l_rtickvec,
    i4		l_rticket,
    i4		prot
)
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tup;
    char		expiration[ 32 ];
    char		timetolive[ 32 ];
    char		pbuff[ 128 ];
    char		*passwd = pbuff;
    i4			size = sizeof( pbuff );
    STATUS		status = OK;

    for(;;)
    {
	/* Must pass in big enough buffer */
	if ( l_rticket < GCN_L_TICKET )
	{
	    status = E_GC0144_GCN_INPW_BROKEN;
	    break;
	}

	/* Look up installation passwd */
	if ( (status = gcn_get_instpwd( inst, size, &passwd )) != OK )
	    break;

	/* Open ticket queue. */
	nq = gcn_nq_find( GCN_LTICKET );
	if ( ! nq  ||  gcn_nq_lock( nq, TRUE ) != OK )
	{
	    status = E_GC0134_GCN_INT_NQ_ERROR;
	    break;
	}

	/* 
	** Create the expiration and time-to-live 
	*/
	CVla( IIGCn_static.ticket_exp_time, timetolive );
	CVla( IIGCn_static.now.TM_secs + (IIGCn_static.ticket_exp_time + 
					  IIGCn_static.ticket_exp_interval),
	      expiration );

	/* 
	** Create tickets for the user and
	** Save them on the tickets queue. 
	*/
	for( ; l_rtickvec--; rtickvec += l_rticket )
	{
	    char 	*rticket = rtickvec;
	    char	lticket[ GCN_L_TICKET ];

	    /* Create a ticket */
	    status = gcn_new_ticket( passwd, lticket, rticket, prot );
	    if ( status != OK )  break;

	    /* Put expiration time on local ticket */
	    STcat( lticket, "," );
	    STcat( lticket, expiration );

	    /* Put time-to-live on remote ticket */
	    STcat( rticket, "," );
	    STcat( rticket, timetolive );

	    /* Add to tickets queue. */
	    tup.uid = user;
	    tup.obj = GCN_GLOBAL_USER;
	    tup.val = lticket;
	    (VOID)gcn_nq_add( nq, &tup );
	    IIGCn_static.ltckt_created++;
	}

	/* Close ticket queue. */
	gcn_nq_unlock( nq );
	break;
    }

    if ( passwd != pbuff )  MEfree( (PTR)passwd );
    return( status );
}


/*
** Name: gcn_save_rtickets() - cache tickets on client
**
** Description:
**	Stashes a bunch of tickets that the client has collected for
**	a particular use until the client calls gcn_use_rticket() to
**	fetch and use one.
**
**	Each ticket has its time-to-live in seconds appended to it; we 
**	replace this with its actual expiration time.
**
**	The rtickets are stored as:
**
**		queue:	"rticket"
**		uid:	user
**		obj:	installation
**		val:	rticket,expiration
**
** Inputs:
**	user		save tickets for this user
**	inst		save for this installation
**	rtickvec	get text of tickets here
**	l_rtickvec	save this many tickets
**	l_rticket	each ticket at most this length
**
** Returns:
**	STATUS
*/

STATUS
gcn_save_rtickets( char *user, char *inst, 
		   char *rtickvec, i4  l_rtickvec, i4  l_ticket )
{
	GCN_QUE_NAME	*nq;
	GCN_TUP		tup;

	/* Open ticket queue. */

	nq = gcn_nq_find( GCN_RTICKET );
	if( !nq || gcn_nq_lock( nq, TRUE ) != OK )
	    return E_GC0134_GCN_INT_NQ_ERROR;

	/* Save tickets on the tickets queue. */

	for( ; l_rtickvec--; rtickvec += l_ticket )
	{
	    char	ticket[ GCN_L_TICKET ];
	    char	expiration[ 32 ];
	    char	*pv[2];
	    i4	timetolive;		

	    /* Separate time-to-live of ticket from ticket. */

	    (void)gcu_words( rtickvec, ticket, pv, ',', 2 );
	    CVal( pv[1], &timetolive );

	    /* Put expiration time on the ticket. */

	    CVla( IIGCn_static.now.TM_secs + timetolive, expiration );

	    STcat( ticket, "," );
	    STcat( ticket, expiration );

	    /* Add to tickets queue. */

	    tup.uid = user;
	    tup.obj = inst;
	    tup.val = ticket;
	    (VOID)gcn_nq_add( nq, &tup );
	    IIGCn_static.rtckt_created++;
	}

	/* Close ticket queue. */

	gcn_nq_unlock( nq );

	return OK;
}


/*
** Name: gcn_use_rticket() - get and decrypt a ticket on the client
**
** Description:
**	Turns a string from a ticket, as held for the user's use in the
**	client Name Server, into an lticket, suitable for handing to the
**	server Name Server as authentication for the user.
**
** Inputs:
**	user	ticket for this user...
**	inst	...trying to connect for this installation...
**	passwd	...using this installation password
**
** Outputs:
**	lticket	ticket suitable for using as a password
**
** Returns:
**	OK	ticket found
**	FAIL	no tickets cached for user/installation
**		any other error
**
** History:
**	19-Feb-98 (gordy)
**	    Use gcn_nq_scan(), gcn_nq_qdel() for better performance.
**	10-Jun-98 (gordy)
**	    Bulk ticket expiration moved to background processing.
**	    Using a ticket must now also check for individual ticket
**	    expiration just in case bulk processing not done recently.
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
*/

STATUS
gcn_use_rticket( char *user, char *inst, char *passwd, u_i1 *lticket, i4 *len )
{
    GCN_QUE_NAME	*nq;
    STATUS		status;
    GCN_TUP		tupmask;
    GCN_TUP		*tupvec[ RVEC_MAX ];
    PTR			qvec[ RVEC_MAX ];
    PTR			scan_cb = NULL;
    char		rticket[ GCN_L_TICKET ];
    char		*p, stamp[ 32 ];
    i4			i, count;
    bool		found = FALSE;

    /* Open ticket queue. */
    if ( ! (nq = gcn_nq_find( GCN_RTICKET )) || gcn_nq_lock( nq, TRUE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    /* Find a ticket for the user and the target */
    CVla( IIGCn_static.now.TM_secs, stamp );
    tupmask.uid = user;
    tupmask.obj = inst;
    tupmask.val = "";

    do
    {
	count = gcn_nq_hscan( nq, &tupmask, &scan_cb, RVEC_MAX, tupvec, qvec );

	/*
	** Search for unexpired ticket.
	*/
	for( i = 0; i < count; i++ )
	{
	    p = STchr( tupvec[ i ]->val, ',' );
	    if ( p  &&  STcompare( p + 1, stamp ) > 0 )  break;	/* ! expired */
	}

	/*
	** Delete expired tickets preceding selected ticket.
	*/
	if ( i )  IIGCn_static.rtckt_expired += gcn_nq_qdel( nq, i, qvec );

	/*
	** If we found a ticket, save info and consume.
	*/
	if ( i < count )
	{
	    found = TRUE;
	    STcopy( tupvec[ i ]->val, rticket );
	    IIGCn_static.rtckt_used += gcn_nq_qdel( nq, 1, &qvec[ i ] );
	}

    } while( ! found  &&  count == RVEC_MAX );

    /* Close ticket queue. */
    gcn_nq_unlock( nq );

    /* No ticket?  FAIL means exactly that. */
    if ( ! found )  
    {
	IIGCn_static.rtckt_cache_miss++;
	return( FAIL );
    }

    /* Strip ticket of its timestamp. */
    p = STchr( rticket, ',' );
    if ( *p ) *p = 0;

    /* Now decrypt it. */
    return( gcn_dec_rticket( passwd, rticket, lticket, len ) );
}


/*
** Name: gcn_use_lticket() - validate and dispose of a ticket on server
**
** Description:
**	Validates that a ticket returned from the client Name Server is
**	a decrypted copy of a ticket generated by this Name Server, by
**	checking for the presence of the ticket for that user on the 
**	LTICKETS queue.
**
**	Since the magic characters of the lticket more or less indicate
**	a properly decrypted ticket, a client should never offer an
**	invalid ticket.  If we are asked to use a ticket not found on
**	the LTICKETS queue, it can be because:
**
**	a) A Name Server imposter is deliberately sending us a
**	   bogus ticket.
**
**	b) The client Name Server failed to purge old tickets, 
**	   possibly because the clock being set back.
**
**	c) The server Name Server cleared its LTICKETS queue, possibly
**	   because the clock was set forward or because the Name Server
**	   files were clobbered.
**
**	In any of these cases, we return the E_GC0143_GCN_INPW_BADTICKET 
**	error number.
**
**	If the ticket is found, it is deleted from the queue and
**	we return OK.
**
** Inputs:
**	user	user being validated
**	raw	ticket validating the user
**	length	length of ticket
**
** Returns:
**	STATUS
**
** History:
**	15-May-98 (gordy)
**	    Don't allow tickets which could act like wildcards.
**	10-Jun-98 (gordy)
**	    Bulk ticket expiration moved to background processing.
**	    Using a ticket must now also check for individual ticket
**	    expiration just in case bulk processing not done recently.
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
**	10-Jul-98 (gordy)
**	    Made the tracing better for invalid tickets.
**	16-Jul-04 (gordy)
**	    Raw ticket is now taken as input.  Moved validation to
**	    gcn_chk_lticket().
*/

STATUS
gcn_use_lticket( char *user, u_i1 *raw, i4 length )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		tupmask;
    GCN_TUP		*tupvec[ LVEC_MAX ];
    PTR			qvec[ LVEC_MAX ];
    PTR			scan_cb = NULL;
    char		tickpat[ GCN_L_TICKET ];
    char		*p, stamp[ 32 ];
    i4			i, count;
    STATUS		status;
    bool		found = FALSE;

    /*
    ** Validate raw ticket and transform to LTICKET.
    */
    if ( (status = gcn_chk_lticket( raw, length, tickpat )) != OK )
	return( status );

    /* 
    ** Find and delete the ticket. 
    */
    if ( ! (nq = gcn_nq_find( GCN_LTICKET )) || gcn_nq_lock( nq, TRUE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    CVla( IIGCn_static.now.TM_secs, stamp );
    STcat( tickpat, "," );
    tupmask.uid = user;
    tupmask.obj = GCN_GLOBAL_USER;
    tupmask.val = tickpat;

    do
    {
	count = gcn_nq_hscan( nq, &tupmask, &scan_cb, LVEC_MAX, tupvec, qvec );

	/*
	** Search for unexpired ticket.
	*/
	for( i = 0; i < count; i++ )
	{
	    p = STchr( tupvec[ i ]->val, ',' );
	    if ( p  &&  STcompare( p + 1, stamp ) > 0 )  break;	/* ! expired */
	}

	/*
	** Delete expired tickets preceding selected ticket.
	*/
	if ( i )  IIGCn_static.ltckt_expired += gcn_nq_qdel( nq, i, qvec );

	/*
	** If we found a ticket, save info and consume.
	*/
	if ( i < count )
	{
	    found = TRUE;
	    IIGCn_static.ltckt_used += gcn_nq_qdel( nq, 1, &qvec[ i ] );
	}

    } while( ! found  &&  count == LVEC_MAX );

    gcn_nq_unlock( nq );

    /* 
    ** Did we use a ticket?
    */
    if ( ! found )  
    {
	if ( IIGCn_static.trace >= 1 )
	{
	    TRdisplay( "Could not find ticket: %s\n", tickpat );

	    if ( IIGCn_static.trace < 4 )
	    {
		i4 trace = IIGCn_static.trace;

		/*
		** Bump up the tracing level temporarily
		** so we can see what happened during the
		** search for the ticket.
		*/
		IIGCn_static.trace = 4;
		scan_cb = NULL;
		count = gcn_nq_hscan( nq, &tupmask, 
				      &scan_cb, LVEC_MAX, tupvec, qvec );
		IIGCn_static.trace = trace;
	    }
	}

	IIGCn_static.ltckt_cache_miss++;
	return( E_GC0143_GCN_INPW_BADTICKET );
    }

    return( OK );
}


/*
** Name: gcn_expire
**
** Description:
**	Check for expired tickets in LTICKET or RTICKET queue.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if ticket expiration performed.
**
** History:
**	10-Jun-98 (gordy)
**	    Created.
*/

bool
gcn_expire( GCN_QUE_NAME *nq )
{
    bool	lticket = FALSE;
    i4		expired;

    /*
    ** Make sure we were called with a ticket queue.
    */
    if ( ! STcasecmp( nq->gcn_type, GCN_LTICKET ) )
	lticket = TRUE;
    else  if ( STcasecmp( nq->gcn_type, GCN_RTICKET ) )
	return( FALSE );

    /* 
    ** Check queue interval to see if expiration checking is needed.
    */
    if ( TMcmp( &IIGCn_static.now, &nq->gcn_next_cleanup ) < 0 )
	    return( FALSE );
	
    STRUCT_ASSIGN_MACRO( IIGCn_static.now, nq->gcn_next_cleanup );
    nq->gcn_next_cleanup.TM_secs += IIGCn_static.ticket_exp_interval;

    if ( gcn_nq_lock( nq, TRUE ) != OK )  return( FALSE );
    expired = gcn_expire_tickets( nq );
    gcn_nq_unlock( nq );

    if ( lticket )
	IIGCn_static.ltckt_expired += expired;
    else  
	IIGCn_static.rtckt_expired += expired;

    return( TRUE );
}


/* Routines internal to this module. */


/*
** Name: gcn_new_ticket() - generate an lticket, rticket pair on server
**
** Description:
**	Creates a single pair of tickets, one encrypted and suitable for
**	sending back to the client, the other plain and suitable for
**	saving until matching against the client's returned, decrypted 
**	ticket.
**
** Inputs
**	passwd		Installation password
**
** Outputs
**	lticket		Server ticket
**	rticket		Client ticket
**
** Returns:
**	STATUS
**
** History:
** 	4-Dec-97 (rajus01)
**	   Added 'prot' parameter to gcn_new_ticket().
**	   Changed the 'raw' ticket structure to handle
**	   installation passwords as remote authentication.
**	   GCN_TICK_MAG is followed by GCN_NEW_TICK_MAG for 
**	   GCA_PROTOCOL_LEVEL_63 and above servers's tickets.
**	23-Jun-04 (gordy)
**	    Added addition encryption of raw ticket info to
**	    hide the underlying random number generator.
**	16-Jul-04 (gordy)
**	    Extended ticket versions.  Upgrade server ticket (LTICKET).
**	28-Sep-04 (gordy)
**	    Standardized ticket tracing.
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough to hold password.
**	21-Jul-09 (gordy)
**	    Use a more appropriate size for encryption key buffer.
*/

static STATUS
gcn_new_ticket( char *passwd, char *lticket, char *rticket, i4 prot )
{
    CI_KS 	ksch;
    char 	key[ CRYPT_SIZE + 1 ];
    u_i1 	ticket0[ GCN_L_TICKET ];
    i4		len;

    /*
    ** The following must be CRYPT_SIZE (8) bytes
    ** long to act as buffers for encryption.
    */
    u_i2	rndm[ 4 ], obfs[ 4 ];

    /* 
    ** Raw ticket must be multiple of CRYPT_SIZE 
    ** to avoid padding. This version is 8 bytes. 
    */
    struct 
    {
	u_i1	magic0;
	u_i1	magic1;
	u_i2	salt1;
	u_i2	salt2;
	u_i2	salt3;
    } raw;

    /*
    ** A ticket is built from a base of random
    ** values which are then obfuscated to hide
    ** the random number generation.
    */
    rndm[ 0 ] = (u_i2)( MHrand() * MAXI2 ) ^ IIGCn_static.now.TM_msecs;
    rndm[ 1 ] = (u_i2)( MHrand() * MAXI2 ) ^ IIGCn_static.now.TM_msecs;
    rndm[ 2 ] = (u_i2)( MHrand() * MAXI2 ) ^ IIGCn_static.now.TM_msecs;
    rndm[ 3 ] = (u_i2)( MHrand() * MAXI2 ) ^ IIGCn_static.now.TM_msecs;

    CIsetkey( (PTR)rndm, ksch );
    CIencode( (PTR)rndm, (i4)sizeof(rndm), ksch, (PTR)obfs );

    /* 
    ** Build the raw ticket 
    */
    raw.magic0 = (u_i1)GCN_TICK_MAG;
    raw.magic1 = (prot >= GCA_PROTOCOL_LEVEL_65) ? (u_i1)GCN_V2_TICK_MAG : 
		 (prot >= GCA_PROTOCOL_LEVEL_63) ? (u_i1)GCN_V1_TICK_MAG :
						   (u_i1)GCN_V0_TICK_MAG; 
    raw.salt1 = obfs[ 0 ] ^ obfs[ 1 ];
    raw.salt2 = obfs[ 1 ] ^ obfs[ 2 ];
    raw.salt3 = obfs[ 2 ] ^ obfs[ 3 ];

    /*
    ** Encrypt raw ticket with installation password
    ** and convert to text as RTICKET.
    */
    gcn_build_key( passwd, key );
    CIsetkey( key, ksch );
    CIencode( (PTR)&raw, (i4)sizeof( raw ), ksch, (PTR)ticket0 );
    CItotext( ticket0, (i4)sizeof( raw ), (PTR)rticket );

    /*
    ** Upgrade raw ticket to final version
    ** and convert to text as LTICKET.
    */
    len = gcn_upgrade_ticket( (u_i1 *)&raw, sizeof( raw ), ticket0 );
    CItotext( ticket0, len, lticket );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_new_ticket: lticket '%s', rticket '%s'\n", 
		   lticket, rticket);

    if ( IIGCn_static.trace >= 5 )
    {
	TRdisplay( "    ticket : " );
	gcx_tdump( &raw, sizeof( raw ) );
	TRdisplay( "    upgrade: " );
	gcx_tdump( ticket0, len );
    }

    return( OK );
}


/*
** Name: gcn_dec_rticket() - do client side decryption of a ticket
**
** Description:
**	Decrypts a client's rticket, turning it into an lticket, suitable
**	for sending to the server and eventual use by gcn_use_lticket().
**
** Inputs:
**	passwd		password with which to decrypt rticket
**	rtickets	encrypted, client ticket
**
** Outputs:
**	lticket		descrypted, server ticket
**	len		length of the ticket.
** History:
**	4-Dec-97 (rajus01)
**	  gcn_dec_rticket() outputs length of the ticket also.
**	  Ticket is not converted into text format. gcn_use_auth()
**	  handles makes the decision.
**	16-Jul-04 (gordy)
**	    Extended ticket versions.  Upgrade ticket for transport
**	    client to server.
**	28-Sep-04 (gordy)
**	    Standardized ticket tracing.
**	26-Oct-05 (gordy)
**	    Ensure buffer is large enough to hold password.
**	21-Jul-09 (gordy)
**	    Use a more appropriate size for encryption key buffer.
*/

static STATUS
gcn_dec_rticket( char *passwd, char *rticket, u_i1 *lticket, i4 *len )
{
    CI_KS 	ksch;
    char 	key[ CRYPT_SIZE + 1 ];
    u_i1	ticket[ GCN_L_TICKET ];

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_use_rticket: rticket '%s'\n", rticket );

    /*
    ** Convert to binary and decrypt with password.
    */
    gcn_build_key( passwd, key );
    CIsetkey( key, ksch );
    CItobin( (PTR)rticket, len, ticket );
    CIdecode( (PTR)ticket, *len, ksch, (PTR)lticket );

    if ( IIGCn_static.trace >= 5 )
    {
	TRdisplay( "    ticket : " );
	gcx_tdump( lticket, *len );
    }

    /* 
    ** If magic chars are garbled we have a bad passwd.
    ** Tickets may be old or new. Check for both.
    */
    if ( lticket[0] != GCN_TICK_MAG  ||
	 (lticket[1] < GCN_V0_TICK_MAG  &&  lticket[1] > GCN_V3_TICK_MAG) ) 
	return( E_GC0142_GCN_INPW_INVALID );

    /*
    ** Upgrade ticket in preparation for 
    ** sending back to the server GCN.
    */
    *len = gcn_upgrade_ticket( lticket, *len, lticket );

    if ( IIGCn_static.trace >= 5 )
    {
	TRdisplay("    upgrade: " );
	gcx_tdump( lticket, *len );
    }

    return( OK );
}


/*
** Name: gcn_chk_lticket
**
** Description:
**	Validates a raw ticket and transforms into LTICKET format.
**
** Input:
**	raw	Ticket.
**	length	Length of ticket
**
** Output:
**	lticket	Transformed ticket.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	16-Jul-04 (gordy)
**	    Created.
**	28-Sep-04 (gordy)
**	    Standardized ticket tracing.
*/

static STATUS
gcn_chk_lticket( u_i1 *raw, i4 length, char *lticket )
{
    u_i1	ticket[ GCN_L_TICKET ];
    i4		ticket_len;
    STATUS	status = OK;

    /*
    ** Upgrade ticket, in case it was not done by client
    ** and convert to text as LTICKET.
    */
    ticket_len = gcn_upgrade_ticket( raw, length, ticket );
    CItotext( ticket, ticket_len, lticket );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "gcn_use_lticket: lticket '%s'\n", lticket );

    if ( IIGCn_static.trace >= 5 )
    {
	TRdisplay( "    ticket : " );
	gcx_tdump( raw, length );
	TRdisplay( "    upgrade: " );
	gcx_tdump( ticket, ticket_len );
    }

    if ( raw[0] != GCN_TICK_MAG  ||
	 (raw[1] < GCN_V0_TICK_MAG  &&  raw[1] > GCN_V3_TICK_MAG) ) 
	status = E_GC0143_GCN_INPW_BADTICKET;

    return( status );
}


/*
** Name: gcn_upgrade_ticket
**
** Description:
**	Performs conversions on tickets which have different
**	transport formats in server/client and client/server
**	communications.
**
**	This routine should be called by the client to upgrade
**	a ticket before passing back to the server.  It should
**	be called by the server to upgrade the ticket being
**	saved locally for later matching with client tickets.
**	It should also be called by the server when a client
**	ticket is received in case client failed to upgrade
**	the ticket.
**
** Input:
**	itb	Client ticket.
**	len	Length of client ticket.
**
** Output:
**	otb	Server ticket (may be same as itb).
**
** Returns:
**	i4	Length of server ticket.
**
** History:
**	16-Jul-04 (gordy)
**	    Created.
*/

static i4
gcn_upgrade_ticket( u_i1 *itb, i4 len, u_i1 *otb )
{
    CI_KS 	ksch;
    u_i1	ticket[ GCN_L_TICKET ];

    if ( itb[0] == GCN_TICK_MAG )
	switch( itb[1] )
	{
	case GCN_V2_TICK_MAG :
	    /*
	    ** A V2 ticket is upgrade to V3 by
	    ** encrypting the ticket with itself
	    ** and building a new magic ID.
	    */
	    if ( len == CRYPT_SIZE )
	    {
		ticket[0] = GCN_TICK_MAG;
		ticket[1] = GCN_V3_TICK_MAG;
		CIsetkey( (PTR)itb, ksch );
		CIencode( (PTR)itb, len, ksch, (PTR)(ticket + 2) );
		len += 2;
		itb = ticket;	/* Ensure copy to otb below */
	    }
	    break;
	}

    if ( otb != itb )  MEcopy( (PTR)itb, (u_i2)len, (PTR)otb );
    return( len );
}


/*
** Name: gcn_build_key
**
** Description:
**	CI encryption routines require keys which are CRYPT_SIZE in length.
**	This routine pads/truncates a string to build an encryption key.
**
**	Output buffer must be at least CRYPT_SIZE + 1 bytes in length.
**
** Inputs:
**	inbuf		Input string
**
** Outputs:
**	outbuf		Encryption key
**
** Returns:
**	VOID
**
** History:
**	21-Jul-09 (gordy)
**	    Converted to build just encryption keys.
*/

static VOID
gcn_build_key( char *inbuf, char *outbuf )
{
    i4 len;

    for( len = 0; len < CRYPT_SIZE; len++ )
	*outbuf++ = *inbuf ? *inbuf++ : ' ';

    *outbuf = EOS;
    return;
}


/*
** Name: gcn_get_instpwd() - get the password for a installation
**
** Description:
**	Scans the GCN_LOGIN queue for the installation password for
**	a given installation.
**
**	The passed in password buffer will be used if the indicated
**	size is sufficient.  Otherwise a suitable buffer will be
**	dynamically allocated.  If resulting buffer is different
**	than input, result buffer should be freed with MEfree().
**
** Inputs:
**	inst		Installation name
**	size		Size of password buffer.
**	passwd		Password buffer.
**
** Outputs:
**	passwd		Installation password.
**
** Returns:
**	STATUS
**
** History:
**	16-Jun-98 (gordy)
**	    Use hashed access scan.
**	15-Jul-04 (gordy)
**	    Enhanced encryption of stored passwords.
**	28-Sep-04 (gordy)
**	    Installation passwords stored in enhanced format were
**	    not being retrieved properly.
**	21-Jul-09 (gordy)
**	    Added size parameter.  Changed output buffer to pointer
**	    reference so that a larger buffer can be allocated if
**	    needed.
*/

static STATUS
gcn_get_instpwd( char *inst, i4 size, char **passwd )
{
    GCN_QUE_NAME	*nq;
    GCN_TUP		*tup, tupmask;
    STATUS		status;
    PTR			scan_cb = NULL;
    i4			ret;
    char		tmpbuf[ 128 ];
    char 		*tmp, *pv[3];
    i4			len, count;
    i4			version = 0;

    /* Open login/password queue. */
    if ( ! (nq = gcn_nq_find(GCN_LOGIN))  ||  gcn_nq_lock( nq, FALSE ) != OK )
	return( E_GC0134_GCN_INT_NQ_ERROR );

    /* Get password for the installation. */
    STpolycat( 2, GCN_GLOBAL_USER, ",", tmpbuf );
    tupmask.uid = GCN_GLOBAL_USER;
    tupmask.obj = inst;
    tupmask.val = tmpbuf;
    ret = gcn_nq_hscan( nq, &tupmask, &scan_cb, 1, &tup, NULL );

    gcn_nq_unlock( nq );
    if ( !ret )  return( E_GC0141_GCN_INPW_NOSUPP );

    /* Copy and return.  */
    len = STlength( tup->val ) + 1;
    tmp = (len <= sizeof( tmpbuf ))
    	  ? tmpbuf : (char *)MEreqmem( 0, len, FALSE, NULL );
    if ( ! tmp )  return( E_GC0121_GCN_NOMEM );

    count = gcu_words( tup->val, tmp, pv, ',', 3 );
    if ( count > 2  &&  pv[2][0] == 'V'  &&  
	 pv[2][1] >= '0'  &&  pv[2][1] <= '9'  &&  pv[2][2] == EOS )
	version = (pv[2][1] - '0');

    /*
    ** Need an output buffer as large as the encypted password.
    */
    len = STlength( pv[1] );
    if ( ! *passwd  ||  len >= size )
	*passwd = (char *)MEreqmem( 0, len + 1, FALSE, NULL );

    if ( ! *passwd )  
    	status = E_GC0121_GCN_NOMEM;
    else
    {
	/* Passwords are stored encrypted - decrypt it now. */
	gcn_login( GCN_VLP_LOGIN, version, FALSE, pv[0], pv[1], *passwd );
    	status = OK;
    }

    if ( tmp != tmpbuf )  MEfree( (PTR)tmp );
    return( status );
}


/*
** Name: gcn_expire_tickets() - discard old tickets
**
** Description:
**	Since tickets are issued by the server in small clumps and 
**	cached on the client, rather than issued and used one at a time,
**	it is possible for a large stockpile of unused tickets to
**	accumulate.  To prevent this, tickets are timestamped with an
**	expiration time and this routine is called periodically to
**	route out expired tickets.
**
** Inputs:
**	nq	name queue of tickets to expire
**
** History:
**	19-Feb-98 (gordy)
**	    Use gcn_nq_scan(), gcn_nq_qdel() for better performance.
**	10-Jun-98 (gordy)
**	    Bulk ticket expiration moved to background processing.
**	    Using a ticket must now also check for individual ticket
**	    expiration just in case bulk processing not done recently.
*/

static i4
gcn_expire_tickets( GCN_QUE_NAME *nq )
{
    PTR		scan_cb = NULL;
    PTR		qvec[ GCN_SVR_MAX ];
    GCN_TUP	*tupvec[ GCN_SVR_MAX ];
    GCN_TUP	tupmask;
    char	stamp[ 32 ];
    i4		i, found, count;
    i4	expired = 0;

    CVla( IIGCn_static.now.TM_secs, stamp );
    tupmask.uid = tupmask.obj = tupmask.val = "";

    /* Scan all tickets */
    do 
    {
	/* Load up the next few tickets */
	found = gcn_nq_scan(nq, &tupmask, &scan_cb, GCN_SVR_MAX, tupvec, qvec);

	/* Loop through tickets */
	for( i = 0, count = 0; i < found; i++ )
	{
	    GCN_TUP	*tup = tupvec[ i ];
	    char	*p;
		
	    /* Expired? */
	    p = STchr( tup->val, ',' );
	    if ( p  &&  STcompare( p + 1, stamp ) > 0 )
		continue;

	    /* Delete this ticket */
	    qvec[ count++ ] = qvec[ i ];
	}

	if ( count )  expired += gcn_nq_qdel( nq, count, qvec );

	/* Continue as long as we get a full load. */
    } while( found == GCN_SVR_MAX );

    return( expired );
}


/*
** Name: gcn_drop_tkts
**
** Description:
**	handle DROP TICKETS command 
**
** Input:
**	char  	LTICKET/RTICKET 
**
** Output:
**	None.
**
** Returns:
**	count  # of tickets dropped.
**
** History:
**	 14-June-2004 (wansh01) 
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

i4 
gcn_drop_tickets( char * ticket )
{
    GCN_QUE_NAME  *nq;
    GCN_TUP	tupmask;
    i4          count;

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "  GCN drop_tickets : entry %s\n", ticket );

    if ( ! (nq = gcn_nq_find( ticket )) || gcn_nq_lock( nq, TRUE ) != OK )
    {
	if ( IIGCn_static.trace >= 4 )
	    TRdisplay ( "gcn_drop_tkts error " );
	return( 0 );
    }

    tupmask.uid = tupmask.obj = tupmask.val = "";
    count = gcn_nq_del( nq, &tupmask );
    gcn_nq_unlock( nq );

    return( count ); 
} 
