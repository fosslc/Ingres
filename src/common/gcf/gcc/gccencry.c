/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cv.h>
#include    <cm.h>
#include    <si.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <me.h>
#include    <mu.h>
#include    <mh.h>
#include    <nm.h>		 
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcs.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcatrace.h>
#include    <gcxdebug.h>
#include    <gcaint.h>

/*
** Name: gccencry.c
**
** Description:
**	GCS interface routines for supporting encryption in GCC.
**
** History:
**      06-Jun-97 (rajus01)
**          Created.
**	20-Aug-97 (gordy)
**	    Moved init/term to general Comm Server processing.
**	    Replaced some dynamic memory objects with static
**	    values.  Simplified the GCS mechanism selection 
**	    algorithm (this still needs work).  Made the MDE
**	    user data area processing more robust.
**	17-Oct-97 (rajus01)
**	    gcc_gcs_mechanisms() now takes mechanism name as input
**	    instead of mechanism ID.
**	22-Oct-97 (rajus01)
**	    Added gcc_emech_ovhd().
**	20-Mar-98 (gordy)
**	    Select mechanism with highest encryption level.
**	    Simplified gcc_emech_ovhd().
**	31-Mar-98 (gordy)
**	    Added mechanism name parameter in gcc_ind_encrypt() for 
**	    default preference.  Allow '*' as wildcard in mechanism name.
**	15-May-98 (gordy)
**	    Use GCS function to lookup mechanisms by name, supports "default".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-Jun-2001 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level
*/

/*
** Local functions
*/
static	i4	gcc_emech_ovhd( GCS_MECH mech_id );
static	i4	gcc_emech_level( GCS_MECH mech_id );


/*
** Name: gcc_gcs_mechanisms
**
** Description:
**	Determine GCS mechanisms which may be used for encryption.
**
**	If no mechanism name is provided (or value is '*'), this
**	routine returns a list (possibly empty) of available 
**	mechanisms supporting encryption.  
**
**	When a mechanism name is provided, the list returned will 
**	contain at most a single entry if the mechanism matching 
**	the input name exists (mechanism may not be available or
**	or may not support encryption).  An empty list will be 
**	returned otherwise.
**
**	It is the callers responsibility to provide a mechanism ID
**	array of sufficient size to hold all possible GCS mechanism
**	IDs.
**
** Input:
**	name		Requested mechanism name, may be NULL.
**
** Output:
**	mech_ids	Array of selected mechanism IDs.
**
** Returns:
**	u_char		Number of mechanism IDs returned.
**
** History:
**	12-Aug-97 (gordy)
**	    Created.
**	17-Oct-97 (rajus01)
**	    gcc_gcs_mechanisms() now takes mechanism name as input
**	    instead of mechanism ID.
**	31-Mar-98 (gordy)
**	    Allow mechanism name of '*' as wildcard.
**	15-May-98 (gordy)
**	    Use GCS function to lookup mechanisms by name, supports "default".
*/

u_char
gcc_gcs_mechanisms( char *name, u_char *mech_ids )
{
    GCS_INFO_PARM	info;
    GCS_MECH		mech_id;
    STATUS 		status;
    u_char		count = 0;
    i4			i;

    if ( name  &&  name[ 0 ]  &&  STcompare( name, GCC_ANY_MECH ) )
    {
	if ( (mech_id = gcs_mech_id( name )) != GCS_NO_MECH )
	    mech_ids[ count++ ] = mech_id;
	else
	{
# ifdef GCXDEBUG
	    if ( IIGCc_global->gcc_trace_level >= 1 )
		TRdisplay( "gcc_gcs: unknown mechanism '%s'\n", name );
# endif /* GCXDEBUG */
	}
    }
    else
    {
	status = IIgcs_call( GCS_OP_INFO, GCS_NO_MECH, (PTR)&info );
	if ( status != OK )  info.mech_count = 0;

	for( i = 0; i < info.mech_count; i++ )
	    if ( 
		 info.mech_info[i]->mech_status == GCS_AVAIL  &&
		 info.mech_info[i]->mech_caps & GCS_CAP_ENCRYPT
	       )
		mech_ids[ count++ ] = info.mech_info[i]->mech_id;
    }

    return( count );
} /* gcc_gcs_mechanisms */


/*
** Name: gcc_rqst_encrypt
**
** Description:
**	Build CR TPDU's user data. The layout of user data buffer
**	pointed by mde->p2 is shown below.
**
**	----------------------------------------------------------
**	| ID | total_len | len1 | value1 | len2 | value2 | .......|
**	----------------------------------------------------------
**
**	ID		TL_PRTCT_PARMVAL_ID
**	total_len	Combined length of following data objects.
**	len{n}		Length of GCS token for {n}th mechanism.
**	value{n}	Token created by GCS for {n}th mechanism.
**
**	The number of data objects matches the number of mechanisms
**	in the TCR_TPDU protection parameter.  A length of 0 (and 
**	no associated token value) indicates GCS returned an error 
**	when initializing the related mechanism.
**
** Input:
**	mde		Message Data Element
**	ccb		Connection Control Block
**	mech_count	Number of GCS mechanisms.
**	mech_ids	Array of GCS mechanism IDs.
**
** Output:
**	mde->p2		Buffer carrying user data.
**	ccb->t_gcs_cbq  Queue List containig Mechanism ID and control block.
**
** Returns:
**	None.
**
** History:
**      06-Jun-97 (rajus01)
**          created.
**	20-Aug-97 (gordy)
**	    Replaced some dynamic memory objects with static
**	    values.  Made the MDE user data area processing 
**	    more robust.
**	 6-Jun-03 (gordy)
**	    Target address now in ccb->ccb_hdr.trg_addr.
*/

VOID
gcc_rqst_encrypt
( 
    GCC_MDE	*mde, 
    GCC_CCB	*ccb, 
    u_char	mech_count, 
    u_char	*mech_ids 
)
{
    GCS_EINIT_PARM	einit_parm;
    GCS_CBQ 		*cq;
    char		*total_len;
    char		*data_objects;
    char		*obj_len;
    STATUS		gcs_status;
    i4			i;

    /*
    ** Build header.  Since the total length is not
    ** yet known, save relevant pointers so it can
    ** be updated later.
    */
    mde->p2 += GCaddn1( TL_PRTCT_PARMVAL_ID, mde->p2 );
    total_len = mde->p2;
    mde->p2 += GCaddn2( 0, mde->p2 );
    data_objects = mde->p2;

    for( i = 0; i < mech_count; i++ )
    {
	if ( ! (cq = ( GCS_CBQ * )gcc_alloc( sizeof( *cq ) )) )
	{
	    STATUS 		generic_status;

	    /*
	    ** We set the initial object length to 0,
	    ** so log the failure and continue with
	    ** the next mechanism.
	    */
	    generic_status = E_GC2004_ALLOCN_FAIL;
	    gcc_er_log( &generic_status, NULL, mde, NULL );
	    continue;
	}

	/*
	** Object length not yet known, so save location.
	*/
	obj_len = mde->p2; 
	mde->p2	+= GCaddn2( 0, mde->p2 );

	einit_parm.initiator 	= TRUE;
	einit_parm.peer	 	= ccb->ccb_hdr.trg_addr.node_id;
	einit_parm.buffer	= (PTR)mde->p2;
	einit_parm.length	= (i4)(mde->pend - mde->p2);
	einit_parm.escb	 	= NULL; 

        gcs_status = IIgcs_call(GCS_OP_E_INIT, mech_ids[i], (PTR)&einit_parm);

	if ( gcs_status != OK )
	{
	    STATUS 		generic_status;
	    GCC_ERLIST		erlist;

	    /*
	    ** We set the initial object length to 0,
	    ** so log the failure and continue with
	    ** the next mechanism.
	    */
	    gcc_er_log( &gcs_status, NULL, mde, NULL );
	    generic_status = E_GC281B_TL_INIT_ENCRYPT;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( mech_ids[i] );
	    erlist.gcc_erparm[0].value = (PTR)&mech_ids[i];
	    gcc_er_log( &generic_status, NULL, mde, &erlist );
	    gcc_free( (PTR)cq );
	    continue;
	}

	cq->mech_id = mech_ids[ i ];
	cq->cb 	= einit_parm.escb;
	QUinit( &cq->cbq );
	QUinsert( &cq->cbq, &ccb->ccb_tce.t_gcs_cbq );

	/*
	** Update the object length.
	*/
	GCaddn2( einit_parm.length, obj_len );
	mde->p2 += einit_parm.length;
    }

    /*
    ** Update total length.
    */
    GCaddn2( mde->p2 - data_objects, total_len );

    return;
} /* gcc_rqst_encrypt */


/*
** Name: gcc_ind_encrypt
**
** Description:
**	Read the CR TPDU's user data received from the client.
**	Initialize the GCS mechanisms requested for encryption.
**	Select an encryption mechanism and enable TL encryption.
**
**	Data format errors will result in a TL abort being
**	issued and FAIL being returned.  GCS encryption 
**	initialization failures will be logged but no further 
**	action will be taken (OK will be returned).  It is the 
**	callers responsibility to ensure encryption enabled if 
**	required for the connection.
**	
** Input:
**	mde		Message Data Element
**	ccb		Connection Control Block
**	mech_count	Number of Mechanism IDs in the list.
** 	mech_ids	Mechanism ID list received from client	
**	name		Requested mechanism name, may be NULL.
**
** Output:
**	ccb		SET encrypt_flag in ccb.
**
** Returns:
**	STATUS		OK or FAIL.
**
** Side Effects:
**	gcc_tl_abort() will be called if the user data area is
**	not formatted properly.
**
** History:
**      10-Jun-97 (rajus01)
**          created.
**	20-Aug-97 (gordy)
**	    Replaced some dynamic memory objects with static
**	    values.  Simplified the GCS mechanism selection 
**	    algorithm (this still needs work).  Made the MDE
**	    user data area processing more robust.
**	20-Mar-98 (gordy)
**	    Select mechanism with highest encryption level.
**	31-Mar-98 (gordy)
**	    Added mechanism name parameter for default preference.
**	    Allow '*' as wildcard in mechanism name.
**	15-May-98 (gordy)
**	    Use GCS function to lookup mechanisms by name, supports "default".
*/

STATUS
gcc_ind_encrypt
( 
    GCC_MDE	*mde, 
    GCC_CCB	*ccb,
    u_char	mech_count,
    u_char	*mech_ids,
    char	*name
)
{
    GCS_EINIT_PARM	einit_parm;
    PTR			mech_cbs[ GCS_MECH_MAX ];
    STATUS		gcs_status;
    u_char		parm_id;
    i4			parm_len;
    char		*parm;
    i4			i, obj; 
    i4			mechs = 0;

    while( mde->p1 < mde->p2 )
    {
	mde->p1 += GCgeti1( mde->p1, &parm_id );
	mde->p1 += GCgeti2( mde->p1, &parm_len );
	parm = mde->p1;
	mde->p1 += parm_len;

	/*
	** Skip the parameter if it is not the protection values.
	*/
	if ( parm_id != TL_PRTCT_PARMVAL_ID )  continue;

	/*
	** Read each individual mechanism token.
	*/
	for( obj = 0; parm < mde->p1  &&  obj < mech_count; obj++ )
	{
	    i4  obj_len;

	    /*
	    ** Note that a 0 length indicates a failure 
	    ** initializing the mechanism and we simply 
	    ** skip to the next mechanism.
	    */
	    parm += GCgeti2( parm, &obj_len );
	    if ( ! obj_len ) continue;

	    einit_parm.initiator = FALSE;
	    einit_parm.peer = NULL;
	    einit_parm.escb = NULL;
	    einit_parm.buffer = parm;
	    einit_parm.length = obj_len;

	    gcs_status = IIgcs_call( GCS_OP_E_INIT, mech_ids[ obj ],
				     (PTR)&einit_parm );
	    if ( gcs_status == OK )
	    {
		mech_ids[ mechs ] = mech_ids[ obj ];
		mech_cbs[ mechs++ ] = einit_parm.escb;
	    }
	    else
	    {
		STATUS 		generic_status;
		GCC_ERLIST	erlist;

		gcc_er_log( &gcs_status, NULL, mde, NULL );
		generic_status = E_GC281B_TL_INIT_ENCRYPT;
		erlist.gcc_parm_cnt = 1;
		erlist.gcc_erparm[0].size = sizeof( mech_ids[ obj ] );
		erlist.gcc_erparm[0].value = (PTR)&mech_ids[ obj ];
		gcc_er_log( &generic_status, NULL, mde, &erlist );
	    }

	    parm += obj_len;
	}

	/*
	** Check to see if we overran the mechanisms tokens.
	** We should have also processed the same number of
	** tokens as mechanisms.
	*/
	if ( parm > mde->p1  ||  obj != mech_count )
	{
	    STATUS 	generic_status;
	    GCC_ERLIST	erlist;
	    u_char	tpdu;

	    generic_status = E_GC281A_TL_INV_TPDU_DATA;
	    tpdu = TCR_TPDU;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( tpdu );
	    erlist.gcc_erparm[0].value = (PTR)&tpdu;
	    gcc_tl_abort( mde, ccb, &generic_status, NULL, &erlist );
	    return( FAIL );
	}
    }

    /*
    ** Check to see of we overran the user data area.
    */
    if ( mde->p1 > mde->p2 )
    {
	STATUS 		generic_status;
	GCC_ERLIST	erlist;
	u_char		tpdu;

	generic_status = E_GC281A_TL_INV_TPDU_DATA;
	tpdu = TCR_TPDU;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( tpdu );
	erlist.gcc_erparm[0].value = (PTR)&tpdu;
	gcc_tl_abort( mde, ccb, &generic_status, NULL, &erlist );
	return( FAIL );
    }

    if ( mechs )
    {
	GCS_INFO_PARM	info;
	GCS_MECH	mech_id;
	i4		mech = -1;
	i4		lvl, mech_lvl = 0;

	if ( name  &&  name[ 0 ]  &&  STcompare( name, GCC_ANY_MECH ) )
	{
	    /*
	    ** See if requested mechanism is available.
	    */
	    if ( (mech_id = gcs_mech_id( name )) != GCS_NO_MECH )
	    {
		for( i = 0; i < mechs; i++ )
		    if ( (i4)mech_id == mech_ids[i] )
		    {
			mech = i;	/* Requested encryption mechanism */
			break;
		    }

		if ( mech < 0 )
		{
# ifdef GCXDEBUG
		    if ( IIGCc_global->gcc_trace_level >= 1 )
			TRdisplay( "gcc_gcs: mechanism '%s' unavailable\n", 
				   name );
# endif /* GCXDEBUG */
		}
	    }
	    else
	    {
# ifdef GCXDEBUG
		if ( IIGCc_global->gcc_trace_level >= 1 )
		    TRdisplay( "gcc_gcs: unknown mechanism '%s'\n", name );
# endif /* GCXDEBUG */
	    }
	}

	if ( mech < 0 )
	{
	    /*
	    ** Select mechanism with highest encryption level.
	    ** We arbitrarily take the first mechanism found
	    ** with the highest level.
	    */
	    for( i = mech = 0; i < mechs; i++ )
		if ( (lvl = gcc_emech_level( mech_ids[i] )) > mech_lvl )
		{
		    mech = i;		/* Highest encryption level */
		    mech_lvl = lvl;
		}
	}

	ccb->ccb_tce.mech_id = mech_ids[ mech ];
	ccb->ccb_tce.gcs_cb  = mech_cbs[ mech ];
	ccb->ccb_tce.t_flags |= GCCTL_ENCRYPT;

	/*
	** Shutdown the other mechanisms.
	*/
	for( i = 0; i < mechs; i++ )
	    if ( i != mech )
	    {
		GCS_ETERM_PARM	eterm;

		eterm.escb = mech_cbs[i];
		IIgcs_call( GCS_OP_E_TERM, mech_ids[i], (PTR)&eterm );
	    }
    }

    return( OK );
} /* gcc_ind_encrypt */


/*
** Name: gcc_rsp_encrypt
**
** Description:
**	Build CC TPDU's user data. The layout of user data buffer
**	pointed by mde->p2 is shown below.
**
**	--------------------
**	| ID | len | value |
**	--------------------
**
**	ID		TL_PRTCT_PARMVAL_ID
**	len		Length of GCS token.
**	value		Token created by GCS.
**
**	A length of 0 (and no associated token value) indicates 
**	GCS returned an error when initializing the mechanism.
**
** Input:
**      mde             Message Data Element
**      ccb             Connection Control Block
**
** Output:
**	None.
**
** Returns:
**      None
**
** Side Effects:
**	If GCS fails when creating a confirmation token,
**	encryption will be disabled.
**
** History:
**      06-Jun-97 (rajus01)
**          created.
**	20-Aug-97 (gordy)
**	    Made the MDE user data area processing more robust.
*/

VOID
gcc_rsp_encrypt( GCC_MDE *mde, GCC_CCB *ccb )
{
    GCS_ECONF_PARM	ecnf_parm;
    STATUS		gcs_status;
    char		*total_len;

    /*
    ** Build header.  Since the length is not
    ** yet known, save relevant pointers so it
    ** can be updated later.
    */
    mde->p2 += GCaddn1( TL_PRTCT_PARMVAL_ID, mde->p2 );
    total_len = mde->p2;
    mde->p2 += GCaddn2( 0, mde->p2 );

    ecnf_parm.initiator	= FALSE;
    ecnf_parm.escb	= ccb->ccb_tce.gcs_cb;
    ecnf_parm.buffer   	= (PTR)mde->p2;
    ecnf_parm.length	= (i4)(mde->pend - mde->p2);

    gcs_status = IIgcs_call( GCS_OP_E_CONFIRM, 
			     ccb->ccb_tce.mech_id, (PTR)&ecnf_parm );

    if ( gcs_status == OK )
    {
	GCaddn2( ecnf_parm.length, total_len );
	mde->p2 += ecnf_parm.length;
    	ccb->ccb_hdr.emech_ovhd = gcc_emech_ovhd( ccb->ccb_tce.mech_id );
    }	
    else
    {
	GCS_ETERM_PARM	eterm_parm;
	STATUS		generic_status;
	GCC_ERLIST	erlist;

	/*
	** We really don't expect this to happen
	** since we successfully initiated the
	** mechanism in gcc_ind_encrypt().  The
	** initial object length was set to 0, so 
	** all we need to do is log the failure 
	** and disable encryption.
	*/
	gcc_er_log( &gcs_status, NULL, mde, NULL );
	generic_status = E_GC281B_TL_INIT_ENCRYPT;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( ccb->ccb_tce.mech_id );
	erlist.gcc_erparm[0].value = (PTR)&ccb->ccb_tce.mech_id;
	gcc_er_log( &generic_status, NULL, mde,  &erlist );

	ccb->ccb_tce.t_flags &= ~GCCTL_ENCRYPT;
	eterm_parm.escb = ccb->ccb_tce.gcs_cb;
	IIgcs_call( GCS_OP_E_TERM, ccb->ccb_tce.mech_id, (PTR)&eterm_parm );
    }

    return;
} /* gcc_rsp_encrypt */


/*
** Name: gcc_cnf_encrypt
**
** Description:
**	Read the CC TPDU's user data received from the partner.
**	Confirm the GCS mechanism (if) selected for encryption 
**	and free resources of any other mechanisms initialized 
**	but not selected.
**
**	Data format errors and GCS encryption confirm failures 
**	will result in a TL abort being issued and FAIL being 
**	returned.  OK will be returned if encryption is confirmed 
**	or our partner failed to select an encryption mechanism.  
**	It is the callers responsibility to ensure encryption 
**	enabled if required for the connection.
**	
** Input:
**	mde		Message Data Element
**	ccb		Connection Control Block
**
** Output:
**      ccb       	Store the control block in ccb for
**			encode and decode operations.
**
** Returns:
**	STATUS		OK or FAIL.
**
** Side Effects:
**	The global encryption flag will be reset if encryption
**	is not confirmed.  gcc_tl_abort() will be called if the 
**	user data area is not formatted properly or encryption
**	confirmation fails.
**
** History:
**      06-Jun-97 (rajus01)
**          created.
**	20-Aug-97 (gordy)
**	    Made the MDE user data area processing more robust.
*/

STATUS
gcc_cnf_encrypt( GCC_MDE *mde, GCC_CCB *ccb )
{
    STATUS		status = OK;
    GCS_CBQ		*cbque;
    u_char		parm_id;
    i4			parm_len;
    char		*parm;
    bool		found = FALSE;

    while( mde->p1 < mde->p2 )
    {
	mde->p1 += GCgeti1( mde->p1, &parm_id );
	mde->p1 += GCgeti2( mde->p1, &parm_len );
	parm = mde->p1;
	mde->p1 += parm_len;

	/*
	** Skip the parameter if it is not the protection value.
	*/
	if ( parm_id != TL_PRTCT_PARMVAL_ID )  continue;

	/*
	** Find the GCS encryption control block for the 
	** mechanism selected by our partner.
	*/
	for( 
	     cbque  = (GCS_CBQ *)ccb->ccb_tce.t_gcs_cbq.q_next;
	     cbque != (GCS_CBQ *)&ccb->ccb_tce.t_gcs_cbq;
	     cbque  = (GCS_CBQ *)cbque->cbq.q_next 
	   )
	    if ( cbque->mech_id == ccb->ccb_tce.mech_id )
	    {
		GCS_ECONF_PARM	ecnf_parm;
		STATUS		gcs_status;

		ecnf_parm.initiator = TRUE;
		ecnf_parm.escb = cbque->cb;
		ecnf_parm.buffer = parm; 
		ecnf_parm.length = parm_len;

		gcs_status = IIgcs_call( GCS_OP_E_CONFIRM,
					 cbque->mech_id, (PTR)&ecnf_parm );

		if ( gcs_status != OK )
		{
		    gcc_er_log( &gcs_status, NULL, mde, NULL );
		    break;
		}

		/*
		** Encryption has been confirmed.  Save the
		** control block and dump the queue entry
		** so it won't be processed below.
		*/
		ccb->ccb_tce.t_flags |= GCCTL_ENCRYPT;
		ccb->ccb_tce.gcs_cb = cbque->cb;	
		ccb->ccb_hdr.emech_ovhd = gcc_emech_ovhd(ccb->ccb_tce.mech_id);
		QUremove( (QUEUE *)cbque );
		gcc_free( (PTR)cbque );
		break;
	    }

	/*
	** If we didn't find the mechanism (we should have!)
	** or the GCS confirmation failed, we are in deep
	** doo-doo.  Our partner has begun encryption, but
	** we are unable to do so.  We can't even exchange
	** messages at this point.  All we can do is abort
	** the connection and let things fail along the way.
	*/
	if ( ! (ccb->ccb_tce.t_flags & GCCTL_ENCRYPT) )
	{
	    STATUS 	generic_status;
	    GCC_ERLIST	erlist;

	    generic_status = E_GC281B_TL_INIT_ENCRYPT;
	    erlist.gcc_parm_cnt = 1;
	    erlist.gcc_erparm[0].size = sizeof( cbque->mech_id );
	    erlist.gcc_erparm[0].value = (PTR)&cbque->mech_id;
	    gcc_tl_abort( mde, ccb, &generic_status, NULL, &erlist );
	    status = FAIL;
	}
    }

    /*
    ** GCS control blocks remaining on the queue
    ** are no longer needed.  Free their resources.
    ** The control block for the active mechanism
    ** (if there is one) was removed above.
    */
    for( 
	 cbque  = (GCS_CBQ *)ccb->ccb_tce.t_gcs_cbq.q_next;
	 cbque != (GCS_CBQ *)&ccb->ccb_tce.t_gcs_cbq;
	 cbque  = (GCS_CBQ *)ccb->ccb_tce.t_gcs_cbq.q_next
       )
    {
	GCS_ETERM_PARM	eterm_parm;

	eterm_parm.escb = cbque->cb;
	IIgcs_call( GCS_OP_E_TERM, cbque->mech_id, (PTR)&eterm_parm );
	QUremove( (QUEUE *)cbque );
	gcc_free( (PTR)cbque );
    }

    /*
    ** Check to see of we overran the user data area.
    */
    if ( mde->p1 > mde->p2 )
    {
	STATUS 		generic_status;
	GCC_ERLIST	erlist;
	u_char		tpdu;

	generic_status = E_GC281A_TL_INV_TPDU_DATA;
	tpdu = TCC_TPDU;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( tpdu );
	erlist.gcc_erparm[0].value = (PTR)&tpdu;
	gcc_tl_abort( mde, ccb, &generic_status, NULL, &erlist );
	status = FAIL;
    }

    return( status );
} /* gcc_cnf_encrypt */


/*
** Name: gcc_emech_ovhd
**
** Description:
**	Get the encryption overhead for the given mechanism ID.
**
** Input:
**      mech_id         Encryption Mechanism ID
**
** Output:
**	None.
**
** Returns:
**	i4		Overhead in bytes.
**
** History:
**      22-Oct-97 (rajus01)
**          created.
**	20-Mar-98 (gordy)
**	    Call GCS for info on specific mechanism rather than all mechs.
*/

static i4
gcc_emech_ovhd( GCS_MECH mech_id )
{
    GCS_INFO_PARM	info;
    STATUS 		status;
    i4			ovhd = 0;

    status = IIgcs_call( GCS_OP_INFO, mech_id, (PTR)&info );
    if ( status == OK )  ovhd = info.mech_info[0]->mech_ovhd;

    return( ovhd );
}


/*
** Name: gcc_emech_level
**
** Description:
**	Get the encryption level for the given mechanism ID.
**
** Input:
**      mech_id         Encryption Mechanism ID
**
** Output:
**	None.
**
** Returns:
**	i4		Overhead in bytes.
**
** History:
**	15-May-98 (gordy)
**	    Created.
*/

static i4
gcc_emech_level( GCS_MECH mech_id )
{
    GCS_INFO_PARM	info;
    STATUS 		status;
    i4			level = 0;

    status = IIgcs_call( GCS_OP_INFO, mech_id, (PTR)&info );
    if ( status == OK )  level = info.mech_info[0]->mech_enc_lvl;

    return( level );
}
