/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <er.h>
#include    <cs.h>  /* needed for CS_SEMAPHORE */
#include    <cv.h>
#include    <ci.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <me.h>
#include    <nm.h>
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <sl.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcu.h>
#include    <gcatrace.h>
#include    <gcc.h>
#include    <gccpci.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcxdebug.h>
#include    <gcm.h>

#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <mh.h>

/**
**
**  Name: gccutil.c - Contains miscellaneous utility functions.
**
**  Description:
**        
**      The following functions are defined in this module: 
**
**          gcc_add_ccb() - create a CCB and add it to the table array.
**          gcc_del_ccb() - delete a CCB and make its slot available.
**          gcc_get_obj() - allocate an object of a specified type
**          gcc_rel_obj() - release an object of a specified type
**	    gcc_alloc() - Allocate a block of memory of a specified size
**	    gcc_str_alloc() - Allocate and copy a string.
**	    gcc_free() - Free a block of memory
**	    gcc_er_open() - Open the error log.
**	    gcc_er_close() - Close the error log.
**	    gcc_er_init() - Initialize the error log message header.
**	    gcc_er_log() - Write an error message to the error log.
**	    gcc_conv_vec() - Convert a vector of atomic datatypes to/from NET
**	    gcc_ctbl_init() - Initialize conversion tables.
**	    
**  History:
**      10-Feb-88 (jbowers)
**          Initial module creation
**      06-Feb-89 (jbowers)
**          Updated for ule_format parm list changes.
**      03-Mar-89 (ham)
**          Added gcc_ctbl_init and C literal translation.
**      09-Mar-89 (jbowers)
**          Modified gcc_er_init to support selective level-based error logging.
**	13-Mar-89 (jrb)
**	    Changed ule_format call to pass gcc_parm_cnt thru so that number
**	    of parameters will be available to ERlookup for consistency checks.
**	21-may-89 (jrb)
**	    Updated for another ule_format interface change.
**      30-Mar-89 (jbowers)
**	    gcc_alloc(): added logging of status returned by MEreqmem in the
**	    event of allocation failure.
**      30-Mar-89 (jbowers)
**	    gcc_get_obj(): added logging of allocation failure.
**      09-May-89 (cmorris)
**          gcc_er_log(): don't dereference IIGCc_global unless it's been set!
**	12-jun-89 (jorge)
**	    added include descrip.h
**	21-jun-89 (pasker)
**	    Added include gcatrace.h and added tracing to GCC_ALLOC(),
**	    GCC_FREE(), GCC_GET_OBJ() and GCC_REL_OBJ()
**	05-jul-89 (seiwald)
**	    Bigger, ASCII sized INXL_L_TBL.
**	06-jul-89 (pasker)
**	    included CS.H, since SCF now requires it.
**	16-Oct-89 (seiwald)
**	     Ifdef'ed GCF62 call to ule_format.  -DGCF62 to get 62 behavior.
**      14-Nov-89 (cmorris)
**           Cleaned up comments.
**	07-Dec-89 (seiwald)
**	     GCC_MDE reorganised: mde_hdr no longer a substructure;
**	     some unused pieces removed; pci_offset, l_user_data,
**	     l_ad_buffer, and other variables replaced with p1, p2, 
**	     papp, and pend: pointers to the beginning and end of PDU 
**	     data, beginning of application data in the PDU, and end
**	     of PDU buffer.
**	09-Dec-89 (seiwald)
**	     Allow for variously sized MDE's: maintain a list of queues
**	     of MDE's of the 7 sizes.
**	11-Dec-89 (seiwald)
**	     MEfill released MDE before, not after, requeuing it.
**	09-Jan-90 (seiwald)
**	     Don't MEfill the MDE on allocation.  Just clear the flags word.
**	12-Jan-90 (seiwald)
**	     Removed references to MDE flags.  With proper MDE ownership,
**	     flags are no longer needed.
**      16-Jan-90 (cmorris)
**           Tracing tidyup.
**	25-Jan-90 (seiwald)
**	     Removed MDE chain from CCB ccb_pce.
**      27-Jan-90 (cmorris)
**           Real-time tracing tidyup.
**	30-Jan-90 (seiwald)
**	    Broken if statement in gcc_er_log() lead IIGCc_global to be
**	    dereferenced after it was tested for NULL.  Added an "else".
**      05-Feb-90 (cmorris)
**          Use various logging masks and the new GCC_LGLVL_STRIP_MACRO macro
**          (partially an NSE porting change)
**	05-Feb-90 (seiwald)
**	    Don't call TRset_file( TR_F_CLOSE ) in gcc_er_close - gcc_er_log
**	    uses ER, not TR.
**	05-Feb-90 (seiwald)
**	    Move the LOG_LVL code up from gcc_er_init into
**	    gcc_er_open.  This makes logging of GCA_REGISTER failures
**	    possible, since gcc_er_init isn't called until after
**	    GCA_REGISTER succeeds.
**	12-Feb-90 (seiwald)
**	    Moved het support into gccpdc.c.
**      01-Mar-90 (cmorris)
**          Fetch installation id from GCC global.
**	19-Jun-90 (seiwald)
**	     Cured the CCB table of tables mania - active CCB's now live
**	     on a single array.
**	7-Jan-91 (seiwald)
**	    Made sizes of 1K TPDU's mimic <6.4 GCC's: always at most 1024 
**	    bytes of user data.  <6.4 GCC's couldn't handle GCA_TO_DESCR's
**	    larger than 1024 bytes: they would get split during het conversion 
**	    and the second fragment would get lost.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	31-May-91 (cmorris)
**	    AL send queue was used so removed it.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    Added include of gc.h
**	2-Sep-92 (seiwald)
**	    Replaced calls to ule_format with calls to new gcu_erlog().
**	28-Sep-92 (brucek)
**	    ANSI C fixes.
**	02-Dec-92 (brucek)
**	    Added MOattach's and MOdetach's for CCBs.
**	11-Jan-93 (edg)
**	    Stop looking for symbol II_GCCxx_LOG -- the equivalent is looked
**	    for in gca.  Use gcu_get_tracesym() to get other tracing symbols.
**	10-Feb-93 (seiwald)
**	    gcc_er_init is now void.
**	11-Feb-93 (seiwald)
**	    Use QU routines rather than the roll-your-own gca_qxxx ones.
**	17-Feb-93 (seiwald)
**	    No more GCC_ID_GCA_PLIST.
**	17-Feb-93 (seiwald)
**	    gcc_find_ccb() is no more.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	24-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42
**	    13-jul-94 (kirke)
**	        Added initialization of mde_q and split in gcc_add_ccb().
**      19-apr-95 (chech02)
**          After gcc_get_obj() call, don't need to call to MEfill().
**	20-Nov-95 (gordy)
**	    Added prototypes.  Added argument to gcc_er_init() to handle
**	    combined initialization routines.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	19-Jun-96 (gordy)
**	    Initialize new CCB layer request queues.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declaractions.  Generalized
**	    the gcu_erlog() parameters to simplify header file dependencies.
**	 3-Jun-97 (gordy)
**	    Added routines for manipulating the initial connection user data.
**	 6-Jun-97 (rajus01)
**	    Added queue initialization for GCS.
**	18-Aug-97 (gordy)
**	    Added gcc_encrypt_mode() for encryption mode translation.
**	10-Oct-97 (rajus01)
**	    Added gcc_encode(), gcc_decode() routines for password encryption.
**	22-Oct-97 (rajus01)
**	    Added gcc_add_mde(), gcc_del_mde(). gcc_get_obj(), gcc_del_obj()
**	    are made static. 
**	 2-Oct-98 (gordy)
**	    Moved MIB class definitons to gcm.h.
**	10-may-1999 (popri01)
**	    Add mh header file, especially for MHrand() prototype, which 
**	    returns an f8.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      26-Jun-2001 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level.
**	12-May-03 (gordy)
**	    Changed doc_stacks from QUEUE to PTR.
**	 6-Jun-03 (gordy)
**	    CCB is now attached as MIB object only after successfull listen.
**	16-Jan-03 (gordy)
**	    Allow MDE's upto 32K.
**	16-Jun-04 (gordy)
**	    Added optional mask to combine with encryption key.
**	    CI encryption key is eight bytes in length (CIsetkey()
**	    only references first eight bytes of key buffer).
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/

/* 
** Password encryption 
*/

#define         CRYPT_DELIM     0xFF
#define		CSET_BASE	0x40
#define		CSET_RANGE	0x80

/*
** Local data
*/

static char	*user_unknown = "<unknown>";


/*
** Forward references
*/

static PTR         gcc_get_obj( i4  obj_id, u_char size );
static VOID        gcc_rel_obj( i4  obj_id, PTR obj_ptr );



/*        
**	The following group of functions create, find and release CCB's.  These
**	are used at various times by various routines during the duration of a
**	connection.
**
**	A CCB is created when a connection is begun.  It exists for the
**	duration of the connection and is destroyed when the connection is
**	terminated.  It contains all the status information required by GCC to
**	execute user requests on the connection.  It is uniquely identified by
**	the connection id.
*/

/*{
** Name: gcc_add_ccb()	- Create a CCB and add it to the table array
**
** Description:
**        
**      A CCB is created and initialized.  
**
** Inputs:
**      None
**
** Outputs:
**      None
**
**	Returns:
**	    GCC_CCB * : A pointer to the new CCB.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-Feb-88 (jbowers)
**          Initial function creation.
**  	18-Dec-91 (cmorris)
**  	    Initialize protocol bridge queue.
**	19-Jun-96 (gordy)
**	    Initialize layer request queues.
**	12-May-03 (gordy)
**	    Changed doc_stacks from QUEUE to PTR and removed initialization.
**	 6-Jun-03 (gordy)
**	    Delay MIB attach until GCA/GCC LISTEN completes.
**	21-Jul-09 (gordy)
**	    Initialize username.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
*/

GCC_CCB *
gcc_add_ccb( VOID )
{
    GCC_CCB             *ccb;
    i4			cei;
    STATUS		status;

    /*
    ** Scan the list of known CCBs for an unused one.
    */

    for( cei = 0; cei < IIGCc_global->gcc_max_ccb; cei++ )
	if( IIGCc_global->gcc_tbl_ccb[ cei ] == NULL )
		break;

    /*
    ** If we have filled the list of CCB pointers, we'll need
    ** to reallocate a longer list.
    */

    if( cei == IIGCc_global->gcc_max_ccb )
    {
	    GCC_CCB	**newvec;
	    i4		newmax = cei + 10;

	    /* Allocate and clear a new CCB pointer list. */

	    newvec = (GCC_CCB **)gcc_alloc( newmax * sizeof( GCC_CCB * ) );
	    
	    if( !newvec )
		     return NULL;

	    MEfill( (newmax-cei) * sizeof( GCC_CCB * ), 0, (PTR)&newvec[cei] );

	    /* Copy & free old list if present. */

	    if( cei )
	    {
		MEcopy( (PTR)IIGCc_global->gcc_tbl_ccb, 
			cei * sizeof( GCC_CCB * ), (PTR)newvec );
		gcc_free( (PTR)IIGCc_global->gcc_tbl_ccb );
	    }

	    /* Install new list. */

	    IIGCc_global->gcc_max_ccb = newmax;
	    IIGCc_global->gcc_tbl_ccb = newvec;
    }

    /* Get a new CCB */

    if( !( ccb = (GCC_CCB *)gcc_get_obj( GCC_ID_CCB, 0 ) ) )
	    return NULL;

    IIGCc_global->gcc_tbl_ccb[ cei ] = ccb;
    ccb->ccb_hdr.lcl_cei = cei;
    ccb->ccb_hdr.username = user_unknown;
    QUinit(&ccb->ccb_aae.al_evq.evq);
    QUinit(&ccb->ccb_aae.a_rcv_q);
    QUinit(&ccb->ccb_pce.mde_q[INFLOW]);
    QUinit(&ccb->ccb_pce.mde_q[OUTFLOW]);
    QUinit(&ccb->ccb_pce.pl_evq.evq);
    QUinit(&ccb->ccb_sce.sl_evq.evq);
    QUinit(&ccb->ccb_tce.tl_evq.evq);
    QUinit(&ccb->ccb_tce.t_snd_q);
    QUinit(&ccb->ccb_tce.t_gcs_cbq);
    QUinit(&ccb->ccb_bce.b_snd_q);

    return ccb;
} 


/*{
** Name: gcc_del_ccb()	- Delete the CCB for a given connection ID.
**
** Description:
**        
**      gcc_del_ccb accepts a connection ID as input.  It uses the ID to  
**      to find the pointer to the CCB for the specified connection ID.  
**	The found CCB is removed from the table and the pointer in the table 
**	NULLed.   The CCB is released. 
**
** Inputs:
**      ccb                        CCB to delete
**
** Outputs:
**      None
**
**	Returns:
**	    OK - CCB found and deleted.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    A pointer entry in the CCB table is NULL'd
**
** History:
**      10-Feb-88 (jbowers)
**          Initial function creation.
**	 4-Dec-97 (rajus01)
**	    Free the memory allocated for remote auth.
**	 6-Jun-03 (gordy)
**	    Check to see if MIB attached before detach.
**	21-Jul-09 (gordy)
**	    Free dynamic resources.
*/

STATUS
gcc_del_ccb( GCC_CCB *ccb )
{
    /* Update GCC MIB */
    if ( ccb->ccb_hdr.flags & CCB_MIB_ATTACH )
    {
	char	cei_buff[16];
	MOulongout(0, (u_i8)ccb->ccb_hdr.lcl_cei, sizeof(cei_buff), cei_buff);
	MOdetach( GCC_MIB_CONNECTION, cei_buff );
    }

    ccb->ccb_hdr.flags |= CCB_FREE;
    if( ccb->ccb_hdr.auth )
    {
	MEfree( ccb->ccb_hdr.auth );
	ccb->ccb_hdr.auth = NULL;
    }

    if ( ccb->ccb_hdr.username  &&  ccb->ccb_hdr.username != user_unknown )  
    	gcc_free( (PTR)ccb->ccb_hdr.username );

    if ( ccb->ccb_hdr.password )  gcc_free( (PTR)ccb->ccb_hdr.password );
    if ( ccb->ccb_hdr.target )  gcc_free( (PTR)ccb->ccb_hdr.target );
    if ( ccb->ccb_hdr.emech )  gcc_free( (PTR)ccb->ccb_hdr.emech );

    IIGCc_global->gcc_tbl_ccb[ ccb->ccb_hdr.lcl_cei ] = NULL;
    gcc_rel_obj( GCC_ID_CCB, (PTR)ccb );

    return( OK );
}


/*{
** Name: gcc_get_obj	- Allocate an object of a specified type
**
** Description:
**      gcc_get_obj centralizes the allocation of data objects by gcc routines. 
**      It is aware of a specific repertoire of objects, which currently
**	comprises the following:
**
**	MDE
**	CCB
**      GCA parameter lists
**	CCB tables
**	
**	The requested object is allocated from a free pool if available.
**	Otherwise a new one is obtained.  The combination of gcc_get_obj and
**	gcc_rel_obj allow the allocation strategy to be defined and
**	subsequently modified for optimization in a single place, with the
**	mechanism hidden from the users of the objects.
**
** Inputs:
**      obj_id                          Identifier of the requested object
**
** Outputs:
**      generic_status                  Environment-independent status
**      system_status                   System-specific status
**
**	Returns:
**	    object			Pointer to the allocated object
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate memory
**
** History:
**      11-Feb-88 (jbowers)
**          Initial function implementation.
**      30-Mar-89 (jbowers)
**	    Added logging of allocation failure.
**      15-Nov-89 (cmorris)
**           Added GCX tracing to gcc_get_obj:- counts of allocated and
**           queued objects are output.
**	7-Jan-91 (seiwald)
**	    Made sizes of 1K TPDU's mimic <6.4 GCC's: always at most 
**	    1024 bytes of user data.
**  	16-Mar-94 (cmorris)
**  	    Use STlength to get length of string being handed to gcc_er_log:
**  	    previously used sizeof may give wrong answer for odd length (and
**  	    other) strings on some architectures.
**	22-Oct-97 (rajus01)
**	    Set pxtend to pend.
**	16-Jan-03 (gordy)
**	    Allow MDE's upto 32K.
*/
static PTR
gcc_get_obj( i4  obj_id, u_char size )
{
    STATUS              status;
    PTR                 obj_ptr;
    char		*obj_type;

    switch (obj_id)
    {
    case GCC_ID_CCB:
    {
	/*
	** Search the queue of free CCB's.  If one is available, dequeue it.  If
	** not, allocate a new one.
	*/

	if ((obj_ptr  = (PTR)QUremove(IIGCc_global->gcc_ccb_q.q_next)) == NULL)
	{
	    obj_ptr = gcc_alloc(sizeof(GCC_CCB));
	    if (obj_ptr == NULL)
	    {
		obj_type = "CCB";
		break;
	    }
	    IIGCc_global->gcc_hw_ccb += 1;
	}
	MEfill(sizeof(GCC_CCB), (u_char) 0, obj_ptr);
	IIGCc_global->gcc_ct_ccb += 1;
	GCX_TRACER_2_MACRO("gcc_get_obj>ccb>", obj_ptr,
	    "CCBs allocated = %d, queued = %d",
            IIGCc_global->gcc_ct_ccb,
            (IIGCc_global->gcc_hw_ccb - IIGCc_global->gcc_ct_ccb));
# ifdef GCXDEBUG
        if( IIGCc_global->gcc_trace_level >= 2 )
            TRdisplay("gcc_get_obj: %d CCB(s) now allocated, %d queued\n",
                      IIGCc_global->gcc_ct_ccb,
                  (i4)(IIGCc_global->gcc_hw_ccb - IIGCc_global->gcc_ct_ccb));
#endif /* GCXDEBUG */
	break;
    } /* end case CCB */
    case GCC_ID_MDE:
    {
	GCC_MDE		*mde;
	i4		bufsiz;
	i4		queue;
	i4		pad1k;

	/*
	** NB: pre 6.4 set tpdu_size to 1K on TL CR and 0(!) on TL CC.
	** Both meant the 1K size, but see the next comment.
	*/
	size = size < TPDU_128_SZ || size > TPDU_64K_SZ ? TPDU_1K_SZ : size;
	queue = size - TPDU_128_SZ;

	/*
	** Compatibility pre 6.4: even though they negotiated a 1K TPDU 
	** size, they actually had 1K in user data alone!  The TPDU was
	** 1K plus the size of the TL -> AL PCI.  Pad1k is the difference
	** between a properly sized MDE and a jumbo pre 6.4 1K MDE.
	*/

	bufsiz = GCC_OFF_TPDU + ( 1 << size );
	pad1k = ( size == TPDU_1K_SZ ) ? GCC_OFF_APP - GCC_OFF_TPDU : 0;

	/*
	** Search the queue of free MDE's.  If one is available, dequeue it.  If
	** not, allocate a new one.
	*/

	if ((obj_ptr = (PTR)QUremove(IIGCc_global->gcc_mde_q[queue].q_next)) == NULL)
	{
	    obj_ptr = gcc_alloc( sizeof(GCC_MDE) + bufsiz + pad1k );
	    if (obj_ptr == NULL)
	    {
		obj_type = "MDE";
		break;
	    }
	    IIGCc_global->gcc_hw_mde += 1;
	}

	mde = (GCC_MDE *)obj_ptr;

	/*
	** To size 1K MDE's large enough to accomodate a jumbo pre 6.4 1K
	** MDE, we add pad1k bytes between the start of the TPDU and the
	** start of user data, by adding pad1k bytes to the user data 
	** pointers and to the end pointer.  This allows us to accept
	** oversized incoming 1K MDE's while never causing us to generate
	** an oversized outbound one.
	*/

	mde->ptpdu = (char *)(mde + 1) + GCC_OFF_TPDU;
	mde->papp = (char *)(mde + 1) + GCC_OFF_APP + pad1k;
	mde->pdata = (char *)(mde + 1) + GCC_OFF_DATA + pad1k;
	mde->pend = (char *)(mde + 1) + bufsiz + pad1k;
	mde->p1 = mde->p2 = mde->papp;
	mde->pxtend = mde->pend;
	mde->queue = queue;
	mde->mde_generic_status[0] = OK;
	IIGCc_global->gcc_ct_mde += 1;

	GCX_TRACER_2_MACRO("gcc_get_obj>mde>", obj_ptr,
  	        "MDEs allocated = %d, queued = %d",
		IIGCc_global->gcc_hw_mde,
		(IIGCc_global->gcc_hw_mde - IIGCc_global->gcc_ct_mde));
# ifdef GCXDEBUG
        if( IIGCc_global->gcc_trace_level >= 2 )
            TRdisplay("gcc_get_obj: %d MDE(s) now allocated, %d queued\n",
                      IIGCc_global->gcc_ct_mde,
                 (i4)(IIGCc_global->gcc_hw_mde - IIGCc_global->gcc_ct_mde));
#endif /*  GCXDEBUG */
	break;
    } /* end case MDE */
    default:
	{
	    obj_ptr = NULL;
	    obj_type = "UNKNOWN";
	    break;
	}
    } /* end switch */
    if (obj_ptr == NULL)
    {
	GCC_ERLIST		erlist;

	status = E_GC2008_OUT_OF_MEMORY;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = STlength(obj_type);
	erlist.gcc_erparm[0].value = (PTR)obj_type;
	gcc_er_log(&status,
		   (CL_ERR_DESC *)NULL,
		   (GCC_MDE *)NULL,
		   &erlist);
    }
    return(obj_ptr);
} /* end gcc_get_obj */

/*{
** Name: gcc_rel_obj	- Release an object of a specified type
**
** Description:
**      gcc_rel_obj returns an object previously obtained by gcc_get_obj.
**
** Inputs:
**      obj_id                          Identifier of the requested object
**	obj_ptr				Pointer to object being released
**
** Outputs:
**	None
**	
**	Returns:
**	    E_DB_OK
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    Enqueues a released object on a free object queue, or frees the
**	    memory allocated to the object.
**
** History:
**      11-Feb-88 (jbowers)
**          Initial function implementation.
**      14-Nov-89 (cmorris)
**           Added GCX tracing to gcc_rel_obj:- memory is filled with "-1"s
**           to help track memory references to freed memory; counts of
**           allocated and queued objects are output.
**      04-Dec-89 (cmorris)
**           Don't fill released objects unless tracing level is >= 2.
**      05-Dec-89 (cmorris)
**           Check for non-null parameter list before trying to free it.
**
*/
static VOID
gcc_rel_obj( i4  obj_id, PTR obj_ptr )
{
    switch (obj_id)
    {
    case GCC_ID_CCB:
    {
# ifdef GCXDEBUG
        if (IIGCc_global->gcc_trace_level >= 2)
            MEfill(sizeof(GCC_CCB), (u_char) -1, obj_ptr);
# endif /* GCXDEBUG */
	/*
	** Add the CCB to the queue of free CCB's.
	*/
	QUinsert( (QUEUE *)obj_ptr, &IIGCc_global->gcc_ccb_q );
	((GCC_CCB *)obj_ptr)->ccb_hdr.flags |= CCB_FREE;
	IIGCc_global->gcc_ct_ccb -= 1;
	
	GCX_TRACER_2_MACRO("gcc_rel_obj>ccb>", obj_ptr,
	    "CCBs allocated = %d, CCBs queued = %d",
            IIGCc_global->gcc_ct_ccb,
            (IIGCc_global->gcc_hw_ccb - IIGCc_global->gcc_ct_ccb));
# ifdef GCXDEBUG
        if( IIGCc_global->gcc_trace_level >= 2 )
            TRdisplay("gcc_rel_obj: %d CCB(s) now allocated, %d queued\n",
                      IIGCc_global->gcc_ct_ccb,
                 (i4)(IIGCc_global->gcc_hw_ccb - IIGCc_global->gcc_ct_ccb));
#endif /* GCXDEBUG */
	break;
    } /* end case CCB */
    case GCC_ID_MDE:
    {
	i4 queue = ((GCC_MDE *)obj_ptr)->queue;

# ifdef GCXDEBUG
        if (IIGCc_global->gcc_trace_level >= 2)
            MEfill(sizeof(GCC_MDE), (u_char) -1, obj_ptr);
# endif /* GCXDEBUG */

	/*
	** Add the MDE to the queue of free MDE's.
	*/

	QUinsert((QUEUE *)obj_ptr, &IIGCc_global->gcc_mde_q[ queue ]);
	IIGCc_global->gcc_ct_mde -= 1;
	GCX_TRACER_2_MACRO("gcc_rel_obj>mde>", obj_ptr,
	    "MDEs allocated = %d, MDEs queued = %d",
		IIGCc_global->gcc_hw_mde,
                (IIGCc_global->gcc_hw_mde - IIGCc_global->gcc_ct_mde));
# ifdef GCXDEBUG
        if( IIGCc_global->gcc_trace_level >= 2 )
            TRdisplay("gcc_rel_obj: %d MDE(s) now allocated, %d queued\n",
                      IIGCc_global->gcc_ct_mde,
                (i4)(IIGCc_global->gcc_hw_mde - IIGCc_global->gcc_ct_mde));
#endif /*  GCXDEBUG */
	break;
    } /* end case MDE */
    default:
	{
	    break;
	}
    } /* end switch */
    return;
} /* end gcc_rel_obj */
/*
*/

/*{
** Name: gcc_alloc()	- GCC memory allocation function
**
** Description:
**        
**      gcc_alloc is used by all GCC routines for memory allocation.  The  
**      standard CL function MEreqmem is used.
**
** Inputs:
**      size                            Size of allocated memory.
**
** Outputs:
**
**	Returns:
**	    PTR				Pointer to allocated memory; NULL if
**					allocation failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-Jun-88 (jbowers)
**          Initial function implementation
**      30-Mar-89 (jbowers)
**          Added logging of status returned by MEreqmem in the event of
**	    allocation failure.
*/
PTR
gcc_alloc( u_i4 size )
{
    u_i2		tag = 0;
    bool		clear = TRUE;
    STATUS		status;
    PTR			ptr;

    ptr = MEreqmem(tag, size, clear, &status);

    if (status != OK)
    {
	gcc_er_log(&status, NULL, NULL, NULL);
	ptr = NULL;
    }

    return (ptr);
} /* end gcc_alloc */

/*{
** Name: gcc_free()	- GCC memory deallocation function
**
** Description:
**        
**      gcc_free is used by all GCC routines for memory allocation.  The  
**      standard CL function MEfree is used.  The parm list is identical to 
**	MEfree.
**
** Inputs:
**      block_ptr			pointer to allocated memory block
**
** Outputs:
**      none			
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-May-87 (jbowers)
**          Initial function implementation
*/
STATUS
gcc_free( PTR block )
{
    return (MEfree(block));
} /* end gcc_free */


/*
** Name: gcc_str_alloc
**
** Description:
**	Allocate memory for a string of a given length, copy the string
**	contents and terminate the string.
**
** Input:
**	str	String to be saved.
**	len	Length of string
**
** Output:
**	None.
**
** Returns:
**	char *	Saved string.
**
** History:
**	21-Jul-09 (gordy)
**	    Created.
*/

char *
gcc_str_alloc( char *str, i4 len )
{
    char *buff = (char *)gcc_alloc( len + 1 );

    if ( buff )
    {
    	MEcopy( (PTR)str, len, (PTR)buff );
	buff[ len ] = EOS;
    }

    return( buff );
}


/*{
** Name: gcc_er_open()	- Open the error log
**
** Description:
**
**      gcc_er_open opens an error log file if one is specified.  It attemts to
**	translate an attribute name of the form II_GCC[ii]_LOG.  If successful,
**	it opens a file whose name is the value of the attribute.
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code specifying error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Oct-88 (jbowers)
**          Initial function implementation
**	11-Jan-92 (edg)
**	    No longer look for and open II_GCCxx_LOG.  gcu_get_tracesym()
**	    will get the other symbols -- Installation codes are no longer
**	    part of these.
**      30-Aug-1995 (sweeney)
**          got rid of some unused buffers left over from NMgtAt days.
*/
STATUS
gcc_er_open( STATUS *generic_status, CL_ERR_DESC *system_status)
{
    STATUS	status;
    char	*tran_place = NULL;
    char	logid[16];
    i4		i;

    status = OK;
    *generic_status = OK;

	
    /*
    ** Get II_GCC_LOGLVL/II_GCC_ERRLVL or PM equivalents 
    ** ii.host.gcc.instance.log_level/error_level.
    */
    gcu_get_tracesym("II_GCC_LOGLVL", "!.log_level", &tran_place);
    if ((tran_place != NULL) && (*tran_place != '\0'))
    {
	CVan(tran_place, &i);
	IIGCc_global->gcc_lglvl = i;
    }
    else
    {
	IIGCc_global->gcc_lglvl = GCC_LGLVL_DEFAULT;
    }    

    gcu_get_tracesym("II_GCC_ERRLVL", "!.error_level", &tran_place);
    if ((tran_place != NULL) && (*tran_place != '\0'))
    {
	CVan(tran_place, &i);
	IIGCc_global->gcc_erlvl = i;
    }
    else
    {
	IIGCc_global->gcc_erlvl = GCC_ERLVL_DEFAULT;
    }    

    return(status);
} /* end gcc_er_open */

/*{
** Name: gcc_er_close()	- Close the error log
**
** Description:
**
**      gcc_er_close closes an error log file if one has been opened.
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code specifying error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Oct-88 (jbowers)
**          Initial function implementation
**	06-Jun-89 (seiwald)
**	    Hand TRset_file() system_status, not &status.
**      04-Oct-89 (cmorris)
**          Correct previous fix:- we were incorrectly passing &system_status.
**
*/
STATUS
gcc_er_close( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    *generic_status = OK;

    return OK;
} /* end gcc_er_close */

/*{
** Name: gcc_er_init()	- Initialize the server title for the error log
**
** Description:
**
**	gcc_er_init receives and saves the value of a string containing an
**	identifier to be inserted in each status record placed in the error
**	log.  This will be generically referred to as the "Server ID."
**	It will subsequently appear in all log records.
**
**	The attributes II_GCCii_ERRLVL and II_GCCii_LOGLVL are translated to
**	determine the level of status messages to be logged out to the global
**	log and the comm server's private log.  These values are saved in
**	GCC_GLOBAL.
**
** Inputs:
**	server_name	String containing name to be used
**	server_id	String containing identifier to be used
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-Oct-88 (jbowers)
**          Initial function implementation
**      09-Mar-89 (jbowers)
**          Modified gcc_er_init to support selective level-based error logging:
**	    translage II_GCCii_ERRLVL and II_GCCii_LOGLVL to determine the level
**	    of status messages to be logged out to the global log and the comm
**	    server's private log.
**	12-Mar-90 (seiwald)
**	    Call GChostname rathern than LGcnode_info for the node name.
**	    This trims about 100k off the executable. 
**	22-Aug-90 (seiwald)
**	    Pass STlength of node rather than sizeof, so we don't pass
**	    nulls.
**	20-Nov-95 (gordy)
**	    Added server_name parameter since now used by protocol bridge.
*/
VOID
gcc_er_init( char *server_name, char *server_id )
{
    char node[50];

    GChostname( node, sizeof( node ) );

    gcu_erinit( node, server_name, server_id );
}


/*{
** Name: gcc_er_log()	- GCC error logging function
**
** Description:
**
**      gcc_er_log is used by all GCC routines to log error messages.  The
**	errror identifier passed as an argument is used to fetch the message and
**	log it.
**
**	The level of message to be logged out is determined by the values of the
**	global variables gcc_erlvl and gcc_lglvl for the public and private logs
**	respectively.  Every message has a level value assigned to it.  0 is the
**	highest level.  for a given level specification, all messages with
**	levels numerically less than or equal to the specified level are logged.
**	Thus, for example, if gcc_lglvl is 5, all messages with assigned levels
**	from 0 to 5 will be put to the private log.  If gcc_erlvl is 3, messages
**	of levels 0 to 3 will be logged to the public log.
**
** Inputs:
**      generic_status	    Mainline status code specifying error.
**	system_status	    System_specific status code.
**	mde		    MDE of the message related to the error.
**	plist		    pointer to error parameter list.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-May-87 (jbowers)
**          Initial function implementation
**      09-Mar-89 (jbowers)
**          Modified gcc_er_log to support selective level-based error logging:
**	    compare the level specified in the input status code to the logging
**	    levels in gcc_erlvl and gcc_lglvl to determine the messages to be
**	    logged out to the global log and the comm server's private log.
**	13-Mar-89 (jrb)
**	    Changed ule_format call to pass gcc_parm_cnt thru so that number
**	    of parameters will be available to ERlookup for consistency checks.
*/

VOID
gcc_er_log( STATUS *generic_status, CL_ERR_DESC *system_status, 
	    GCC_MDE *mde, GCC_ERLIST *plist )
{
    ER_ARGUMENT	eargs[12];
    i4		level;
    i4		i = 0;

    /* Strip off the message level code */

    level = ( *generic_status & (i4) GCLVL_MASK ) / GCLVL_BIT;
    *generic_status = GCC_LGLVL_STRIP_MACRO(*generic_status);

    /* Don't log if the user has gone to great lengths to shut off logging. */

    if( IIGCc_global &&
	level > IIGCc_global->gcc_erlvl &&
	level > IIGCc_global->gcc_lglvl )
	    return;

    /* Copy accursed GCC_ERLIST into ER_ARGUMENT's */

    if( plist )
    {
	for( i = 0; i < plist->gcc_parm_cnt; i++ )
	{
	    eargs[i].er_value = (PTR)plist->gcc_erparm[i].value;
	    eargs[i].er_size = plist->gcc_erparm[i].size;
	}
    }

    /* 
    ** Now log the thing. 
    */
    gcu_erlog( (i4)(mde && mde->ccb ? mde->ccb->ccb_hdr.lcl_cei : -1),
	       (i4)1, *generic_status, system_status, i, (PTR)eargs );

    return;
}


/*
** Name: gcc_set_conn_info
**
** Description:
**	Build the connection user data area in the MDE.  
**	This routine is included for backward compatibility.
**	Connection user data used to be built by the AL but
**	is now done by the SL.  This routine performs the
**	same actions as was originally done by the AL.
**
**	Since this is the initial PDU, we must perform 
**	translation to transfer syntax in case this is 
**	a heterogeneous association.  This is actually 
**	a layer violation, as this should be being done 
**	by PL based on an ASN.1 type specification.  
**	However, as a practical matter, it is done here.
**
** Input:
**	ccb		Connection control block.
**	mde		Message data element.
**
** Output:
**
** Returns:
**	VOID
**
** History:
**	 3-Jun-97 (gordy)
**	    Extracted from gccal.c
**	20-Mar-98 (gordy)
**	    Provide empty security label if none available.
**	21-Jul-09 (gordy)
**	    Password dynamically allocated.  Remove security label stuff.
*/

VOID
gcc_set_conn_info( GCC_CCB *ccb, GCC_MDE *mde )
{
    char	*tmp_src_vec;
    i4		str_len;
    i2		tmpi2;

    /*
    ** GCA association flags and protocol level.
    */
    tmpi2 = ccb->ccb_hdr.gca_flags;
    mde->p2 += GCaddi2( &tmpi2, mde->p2 );
    tmpi2 = ccb->ccb_hdr.gca_protocol;
    mde->p2 += GCaddi2( &tmpi2, mde->p2 );

    /*
    ** Username and password.
    */
    str_len = STlength( ccb->ccb_hdr.username ) + 1;
    GCnadds( ccb->ccb_hdr.username, str_len, mde->p2 );
    mde->p2 += GCC_L_UNAME;

    if ( ccb->ccb_hdr.password )
    {
	str_len = STlength( ccb->ccb_hdr.password ) + 1;
	GCnadds( ccb->ccb_hdr.password, str_len, mde->p2 );
    }
    else
    {
	tmp_src_vec = "";
	str_len = STlength( tmp_src_vec ) + 1;
	GCnadds( tmp_src_vec, str_len, mde->p2 );
    }

    mde->p2 += GCC_L_PWD;

    /*
    ** Account name (deprecated).
    */
    tmp_src_vec = "";
    str_len = STlength( tmp_src_vec ) + 1;
    GCnadds( tmp_src_vec, str_len, mde->p2 );
    mde->p2 += GCC_L_ACCTNAME;

    /* 
    ** Security label, plus sec label type. 
    */
    tmp_src_vec = "";
    str_len = STlength( tmp_src_vec ) + 1;
    GCnadds( tmp_src_vec, str_len, mde->p2 );
    mde->p2 += GCC_L_SECLABEL;

    tmpi2 = 0;
    mde->p2 += GCaddi2( &tmpi2, mde->p2 );

    /*
    ** Access point identifier (deprecated).
    */
    tmp_src_vec = "";
    str_len = STlength( tmp_src_vec ) + 1;
    GCnadds( tmp_src_vec, str_len, mde->p2 );
    mde->p2 += GCC_L_API;

    return;
}


/*
** Name: gcc_get_conn_info
**
** Description:
**	Extracts connection user data from the MDE as
**	built by gcc_set_conn_info().
**
** Input:
**	ccb		Connection control block.
**	mde		Message data element.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 3-Jun-97 (gordy)
**	    Created.
**	31-Mar-06 (gordy)
**	    Sanity check lengths read from IO buffers.
**	21-Jul-09 (gordy)
**	    Username and password are now dynamically allocated.
**	    Remove security label stuff.
*/

VOID
gcc_get_conn_info( GCC_CCB *ccb, GCC_MDE *mde )
{
    char	user[ GCC_L_UNAME + 1 ];
    char	pwd[ GCC_L_PWD + 1 ];
    i2		tmpi2;

    if ( (mde->p1 + 2) > mde->p2 )  return;
    mde->p1 += GCgeti2( mde->p1, &tmpi2 );
    ccb->ccb_hdr.gca_flags = tmpi2;

    if ( (mde->p1 + 2) > mde->p2 )  return;
    mde->p1 += GCgeti2( mde->p1, &tmpi2 );
    ccb->ccb_hdr.gca_protocol = tmpi2;

    if ( (mde->p1 + GCC_L_UNAME) > mde->p2 )  return;
    mde->p1 += GCngets( mde->p1, (i4)GCC_L_UNAME, user );
    user[ GCC_L_UNAME ] = EOS;
    ccb->ccb_hdr.username = gcc_str_alloc( user, STlength( user ) );
    ccb->ccb_hdr.flags |= CCB_USERNAME;

    if ( (mde->p1 + GCC_L_PWD) > mde->p2 )  return;
    mde->p1 += GCngets( mde->p1, (i4)GCC_L_PWD, pwd );
    pwd[ GCC_L_PWD ] = EOS;
    if ( *pwd )  ccb->ccb_hdr.password = gcc_str_alloc( pwd, STlength( pwd ) );

    if ( (mde->p1 + GCC_L_ACCTNAME) > mde->p2 )  return;
    mde->p1 += GCC_L_ACCTNAME;

    if ( (mde->p1 + GCC_L_SECLABEL) > mde->p2 )  return;
    mde->p1 += GCC_L_SECLABEL;

    if ( (mde->p1 + 2) > mde->p2 )  return;
    mde->p1 += GCgeti2( mde->p1, &tmpi2 );

    if ( (mde->p1 + GCC_L_API) > mde->p2 )  return;
    mde->p1 += GCC_L_API;

    return;
}


/*
** Name: gcc_encrypt_mode
**
** Description:
**	Translate string encryption mode to numeric encryption mode.
**
** Input:
**	str		String encryption mode.
**
** Output:
**	mode		Numeric encryption mode.
**
** Returns:
**	STATUS		OK or FAIL (invalid string value).
**
** History:
**	18-Aug-97 (gordy)
**	    Created.
*/

STATUS
gcc_encrypt_mode( char *str, i4  *mode )
{
    if ( ! str  ||  ! *str )
	return( FAIL );
    else  if ( ! STbcompare( str, 0, GCC_ENC_OFF_STR, 0, TRUE ) )
	*mode = GCC_ENCRYPT_OFF;
    else  if ( ! STbcompare( str, 0, GCC_ENC_OPT_STR, 3, TRUE ) )
	*mode = GCC_ENCRYPT_OPT;
    else  if ( ! STbcompare( str, 0, GCC_ENC_ON_STR, 0, TRUE ) )
	*mode = GCC_ENCRYPT_ON;
    else  if ( ! STbcompare( str, 0, GCC_ENC_REQ_STR, 3, TRUE ) )
	*mode = GCC_ENCRYPT_REQ;
    else
	return( FAIL );

    return( OK );
}

/*
** Name: gcc_encode
**
** Description:
**	Encrypts the string using key provided.
**
**	Output buffer should be at least GCC_ENCODE_LEN( len )
**	bytes in length.
**
**	An optional key mask is combined with the generated key
**	if provided (must be eight bytes in length).
**
** Input:
**	str 		String to be encrypted.
**	key		Key for encryption.
**	mask		Key mask, may be NULL.
**
** Output:
**	buff		Encrypted value (binary).
**
** Returns:
**	i4 		Length of encrypted value, 0 if error.
**
** History:
**	07-Oct-97 (rajus01)
**	    Extracted from gcs_encode().
**	16-Jun-04 (gordy)
**	    Added optional mask to combine with encryption key.
**	    CI encryption key is eight bytes in length.
**	15-Jul-04 (gordy)
**	    Changed parameter type to better reflect actual usage.
**	31-Mar-06 (gordy)
**	    Defined external max encoding size.
**	21-Jul-09 (gordy)
**	    Remove restrictions on password length.
**	07-Oct-09 (hweho01) SIR 122536
**	    Avoid memory overlay, increase the size of kbuff to    
**	    hold the EOS.
*/

i4
gcc_encode( char *str, char *key, u_i1 *mask, u_i1 *buff )
{
    CI_KS	ksch;
    char	kbuff[ GCC_KEY_LEN + 1 ];
    char	*ptr;
    i4		len, size = GCC_ENCODE_LEN( STlength( str ) ) + 1;
    bool	done;

    /*
    ** Generate a GCC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.  
    */
    for( ptr = key, len = 0; len < GCC_KEY_LEN; len++ )
    {
	if ( ! *ptr )  ptr = key;
	kbuff[ len ] = *ptr++ ^ (mask ? mask[len] : 0);
    }

    kbuff[ len ] = EOS;
    CIsetkey( kbuff, ksch );

    /*
    ** The string to be encrypted must be a multiple of
    ** GCC_CRYPT_SIZE in length for the block encryption
    ** algorithm used by CIencode().  To provide some
    ** randomness in the output, the first character
    ** or each GCC_CRYPT_SIZE block is assigned a random
    ** value.  The remainder of the block is filled
    ** from the string to be encrypted.  If the final
    ** block is not filled, random values are used as
    ** padding.  A fixed delimiter separates the 
    ** original string and the padding (the delimiter 
    ** must always be present).
    */
    for( done = FALSE, len = 0; ! done  &&  (len + GCC_CRYPT_SIZE) < size; )
    {
	/*
	** First character in each encryption block is random.
	*/
	buff[ len++ ] = (i4) ( CSET_RANGE * MHrand() ) + CSET_BASE;

	/*
	** Load string into remainder of encryption block.
	*/
        while( *str  &&  len % GCC_CRYPT_SIZE )  buff[ len++ ] = *str++;

	/*
	** If encryption block not filled, fill with random padding.
	*/
	if ( len % GCC_CRYPT_SIZE )
	{
	    /*
	    ** Padding begins with fixed delimiter.
	    */
	    buff[ len++ ] = CRYPT_DELIM;
	    done = TRUE;	/* Only done when delimiter appended */

	    /*
	    ** Fill encryption block with random padding.
	    */
	    while( len % GCC_CRYPT_SIZE )
		buff[ len++ ] = (i4)( CSET_RANGE * MHrand() ) + CSET_BASE;
	}
    }

    /*
    ** Encrypt
    */
    buff[ len ] = EOS;
    CIencode( buff, len, ksch, (PTR)buff );

    return( len );
}


/*
** Name: gcc_decode
**
** Description:
**	Decrypts the string encrypted by gcc_encode.
**
**	Decoded output will be smaller than encoded input.  Output
**	buffer should be at least as large as input to ensure
**	sufficient space.  Output buffer may be same as input.
**
**	An optional key mask is combined with the generated key
**	if provided (must be eight bytes in length).
**
** Input:
**	e_str 		Encrypted string.
**	e_len		Length of encrypted string.
**	key		key for decryption.
**	mask		Key mask, may be NULL.
**
** Output:
**	buff		Decrypted value.
**
** Returns:
**	VOID.
**
** History:
**	07-Oct-97 (rajus01)
**	    Extracted from gcs_decode().
**	17-Oct-97 (rajus01)
**	    Define obuff as u_char as there was problem in
**	    removing (0xFF), the CRYPT_DELIM. 
**	16-Jun-04 (gordy)
**	    Added optional mask to combine with encryption key.
**	    CI encryption key is eight bytes in length.
**	15-Jul-04 (gordy)
**	    Changed parameter type to better reflect actual usage.
**	31-Mar-06 (gordy)
**	    Defined external max encoding size.
**	21-Jul-09 (gordy)
**	    Remove restrictions on password length.  Decode
**	    directly into output buffer.
**	07-Oct-09 (hweho01) SIR 122536
**	    Setup kbuff to be consistent with the one in gcc_encode().    
*/

VOID
gcc_decode( u_i1 *e_str, i4 e_len, char *key, u_i1 *mask, char *dbuff )
{
    CI_KS	ksch;
    char	kbuff[ GCC_KEY_LEN + 1 ];
    char	*ptr;
    i4		len;

    /*
    ** Generate a GCC_KEY_LEN size encryption 
    ** key from key provided.  The input key 
    ** will be duplicated or truncated as needed.
    */
    for( ptr = key, len = 0; len < GCC_KEY_LEN; len++ )
    {
	if ( ! *ptr ) ptr = key;
	kbuff[ len ] = *ptr++ ^ (mask ? mask[len] : 0);
    }

    kbuff[ len ] = EOS;
    CIsetkey( kbuff, ksch);
    CIdecode( (PTR)e_str, (i4)e_len, ksch, (PTR)dbuff );
    
    /*
    ** Remove padding and delimiter 
    ** from last encryption block.
    */
    while( e_len  &&  (u_char)dbuff[ --e_len ] != CRYPT_DELIM );
    dbuff[ e_len ] = EOS;

    /*
    ** Extract original string skipping first
    ** character in each encryption block.
    */
    for( len = 0, ptr = dbuff; dbuff[ len ]; len++ )
	if ( len % GCC_CRYPT_SIZE )  *ptr++ = dbuff[ len ];

    *ptr = EOS;
    return;
}

/*
** Name: gcc_add_mde
**
** Description:
**	MDE is created and initialized.
**
** Input:
**	ccb		connection control block.
**	size		number of bytes.
**
** Output:
** 	None.
**
** Returns:
**	GCC_MDE *	ptr to MDE. 
**
** History:
**	 22-Oct-97 (rajus01)
**	    Created.
*/
GCC_MDE *
gcc_add_mde( GCC_CCB *ccb, u_char size )
{
    GCC_MDE *mde;

    mde = ( GCC_MDE * )gcc_get_obj( GCC_ID_MDE, size );

    /* back up the end of PDU buffer to number of bytes in overhead
    ** for the negotiated encryption mechanism.
    */
    if( mde  && ccb &&  ccb->ccb_hdr.emech_ovhd  )
	mde->pend  -= ccb->ccb_hdr.emech_ovhd;

    return mde;
}

/*
** Name: gcc_del_mde
**
** Description:
**	Frees the given mde. It invokes gcc_rel_obj() to free it.
**
** Input:
**	mde		mde to free.
**
** Output:
**	None.
**
** Returns:
**	OK.
**
** History:
**	 22-Oct-97 (rajus01)
**	    Created.
*/
STATUS
gcc_del_mde( GCC_MDE *mde )
{
    gcc_rel_obj( GCC_ID_MDE, (PTR)mde );
    return OK;
}
