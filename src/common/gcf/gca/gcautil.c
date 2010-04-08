/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <gc.h>
#include    <me.h>
#include    <mu.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcd.h>
#include    <gcaint.h>
#include    <gcaimsg.h>
#include    <gcocomp.h> 
#include    <gcatrace.h>
#include    <gcu.h>

/**
**
**  Name: gcautil.C - Contains miscellaneous utility functions.
**
**  Description:
**        
**      The following functions are contained in gcautil.c:
**
**          gca_alloc()		GCA memory allocation function
**	    gca_str_alloc()	GCA string allocation function.
**          gca_free()		GCA memory deallocation function
**	    gca_is_eog()	Is message end-of-group
**	    gca_descr_id()	Get descriptor ID.
**	    gca_to_len()	Calculate length of GCA_TO_DESCR message.
**	    gca_to_fmt()	Format GCA_TO_DESCR message.
**	    gca_resolved()	Unpack GCN_RESOLVED message.
**
**  History:    $Log-for RCS$
**      28-Apr-87 (jbowers)
**          Initial module implementation
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**      23-Jan-89 (cmorris)
**          Tracing tidyup
**	08-Mar-90 (seiwald)
**	    Since (*IIGCa_static->usr_alloc)() doesn't (necessarily)
**	    zero memory, we no longer ask MEreqmem to do so, either.
**	06-Jun-90 (seiwald)
**	    Made gca_alloc() return the pointer to allocated memory.
**	17-aug-91 (leighb) DeskTop Porting Changes:
**	    include pc.h and gcaipc.h for PMFE (IBM/PC).
**	    use a separate allocation tag for each spawn level under MS-DOS.
**	27-Mar-92 (GordonW)
**	    Remove external references to MEreqmem/MEfree. This are in me.h
**	    Change the gca_free log error to output status from MEfree
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**      12-jun-1995 (chech02)
**          Added semaphores to protect critical sections (MCT).
**	 3-Nov-95 (gordy)
**	    Removed MCT ifdefs.  Should not have to protect CL calls.  The
**	    CL routines should take care of any such non-sense.  Combined
**	    routines from gcamisc.c here.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	 1-Feb-96 (gordy)
**	    Support compressed VARCHARs.
**	 3-Sep-96 (gordy)
**	    Restructured global data structures.
**	20-Dec-96 (gordy)
**	    Added GCA_QC_ID as end-of-group message since always 
**	    followed by GCA_TDESCR or GCA_RESPONSE.
**	31-Jan-97 (gordy)
**	    Support formatted descriptors in gca_to_len(), gca_to_fmt().
**	26-Feb-97 (gordy)
**	    Extracted utility functions to gcu.
**	29-May-97 (gordy)
**	    New RESOLVED format at PROTOCOL_LEVEL_63.
**	17-Oct-97 (rajus01)
**	    Extract the new attribute info such as remote authentication,
**	    encryption mode, encryption mechanism.
**	12-Jan-98 (gordy)
**	    Host name added for local connections to support direct
**	    GCA network connections.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	14-May-98 (gordy)
**	    Added remote authentication mechanism in GCN2_D_RESOLVED.
**	19-May-98 (gordy)
**	    Allocate buffer to hold resolve data.  Return error if no 
**	    local partners.
**	04-18-2000 (wansh02)
**	    bug 101148/star 8749484
**	    Made change to gca_to_fmt() to only copy 16 bytes of column descriptor
**	    (without the gca_attname) to col_arr. In the case of INCLUDE_NAMES is
**	    not set in the informix gateway, column name is not set in the
**	    descriptor and therefore causes the access violation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    Removed PTR from GCA_COL_ATT and GCA_ELEMENT.  Use the 64 bit PTR
**	    code for all platforms (most generalized).
**	22-Mar-02 (gordy)
**	    Removed references to PTR types in GCA messages and eliminated
**	    nasty ifdef which resulted from not fixing this problem correctly
**	    when the first 64 bit PTR types were supported.  Replaced
**	    gca_getlong() with routine to extract tuple descriptor ID.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	16-Jan-03 (gordy)
**	    Fixed problem with GCA_TO_DESCR arr_stat value and IBM GCC.
**	17-Jul-09 (gordy)
**	    Added gca_str_alloc().
**/


/*{
** Name: gca_alloc()	- GCA memory allocation function
**
** Description:
**        
**      gca_alloc is used by all GCA routines for memory allocation.  If an 
**      allocation function has been specified by the user through the  
**      GCA_INITIATE service, the user's function is used.  Otherwise, the  
**      standard CL function MEreqmem is used.  The parm list is identical to 
**	MEalloc.
**
** Inputs:
**      number          Number of objects
**      size            Size of each object
**
** Outputs:
**      block_ptr	Pointer to allocated memory block
**
** Returns:
**	STATUS
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-May-87 (jbowers)
**          Initial function implementation
**      02-Oct-87 (jbowers)
**          Changed default memory allocation routine from MEalloc to MEreqmem
**	16-Jul-1989 (jorge)
**  	    Removed confusing commented referenece to MEalloc.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	30-Aug-1995 (sweeney)
**	    restructured an ugly #ifdef.
**	 3-Sep-96 (gordy)
**	    Restructured global data structures.
*/

PTR
gca_alloc( u_i4 size )
{
    STATUS	status;
    PTR		ptr;

    if ( IIGCa_global.gca_alloc )
	status = (*IIGCa_global.gca_alloc)( 1, size, &ptr );
    else
	ptr = MEreqmem( 0, (u_i4) size, TRUE, &status );

    GCX_TRACER_1_MACRO( "gca_alloc<", ptr, "return code = %x", (i4) status );

    return ptr;
}


/*
** Name: gca_str_alloc
**
** Description:
**	GCA version to STalloc() which uses the GCA memory allocator.
**	Allocated strings should be freed with gca_free().
**
** Input:
**	str	String to be saved.
**
** Output:
**	None.
**
** Returns:
**	char *	Saved string.
**
** History:
**	17-Jul-09 (gordy)
**	    Created.
*/

char *
gca_str_alloc( char *str )
{
    i4		length = str ? STlength( str ) + 1 : 0;
    char	*buff = length ? gca_alloc( length ) : NULL;

    if ( buff )  MEcopy( (PTR)str, length, (PTR)buff );
    return( buff );
}


/*{
** Name: gca_free()	- GCA memory deallocation function
**
** Description:
**        
**      gca_free is used by all GCA routines for memory allocation.  If a
**      deallocation function has been specified by the user through the  
**      GCA_INITIATE service, the user's function is used.  Otherwise, the  
**      standard CL function ME alloc is used.  The parm list is identical to 
**	MEalloc.
**
** Inputs:
**      block_ptr	Pointer to allocated memory block
**
** Outputs:
**      None			
**
** Returns:
**	STATUS
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-May-87 (jbowers)
**          Initial function implementation
**	 3-Sep-96 (gordy)
**	    Restructured global data structures.
*/

VOID
gca_free( PTR ptr )
{
    STATUS status;

    if ( IIGCa_global.gca_free )
	status = (*IIGCa_global.gca_free)( ptr );
    else
	status = MEfree( ptr );

    GCX_TRACER_1_MACRO( "gca_free<", ptr, "status code = %x", (i4) status);

    return;
}

/*
** Name: gca_is_eog - is this message the last in a group?
**
** Description:
**	Because the GCA protocol requires that certain messages always
**	be followed by other messages, GCA and GCC can optimize I/O by
**	not actually sending the messages until the last message of a
**	group passes through.  Ideally, the caller of GCA_SEND would
**	provide such information, but instead we detect the last message
**	of a group by looking at the message type.  
**
** History:
**	20-Dec-96 (gordy)
**	    Added GCA_QC_ID since always followed by GCA_TDESCR 
**	    or GCA_RESPONSE.
**	30-Jun-99 (rajus01)
**	    gca_is_eog() now takes protocol version as parameter instead
**	    of acb.
*/

bool
gca_is_eog( i4  message_type, u_i4 gca_prot )
{
    /* Only these don't end a group. */

    return
	    !( message_type == GCA_RETPROC && 
		gca_prot >= GCA_PROTOCOL_LEVEL_60 ) &&
	    message_type != GCA_TUPLES &&
	    message_type != GCA_TO_DESCR &&
	    message_type != GCA_CDATA &&
	    message_type != GCA_TDESCR &&
	    message_type != GCA_QC_ID;
}



/*
** Name: gca_descr_id
**
** Description:
**	Returns the descriptor ID from a GCA_TD_DATA object.
**	The object may be formatted (GCD_TD_DATA) or may be
**	a byte-stream buffer.
**
** Input:
**	frmtd	TRUE if formatted.
**	ptr	Descriptor buffer.
**
** Output:
**	None.
**
** Returns:
**	i4	Descriptor ID.
**
** History:
**	22-Mar-02 (gordy)
**	    Created.
*/

i4
gca_descr_id( bool frmtd, PTR ptr )
{
    i4	id;

    if ( frmtd )
	id = ((GCD_TD_DATA *)ptr)->gca_id_tdscr;
    else
    {
	u_char *descr = (u_char *)ptr + (sizeof(i4) * 2);
	GCA_GETI4_MACRO( descr, &id );
    }

    return( id );
}



/*{
** Name: gca_to_len     - compute length of GCA_TO_DESCR message
**       gca_to_fmt     - format GCA_TO_DESCR message
**
** Description:
**      These two routines compute the length of the GCA_TO_DESCR message
**      and format the message, given the GCA_TD_DATA supplied by the
**      client as sd_parms->gca_descriptor.
**
**      Computing the size and formatting the message are split so that
**      allocating the buffer can be done elsewhere.
**
** Inputs:
**      tuple_desc      Client supplied GCA_TD_DATA describing tuples.
**      tod             Resultant GCA_TO_DESCR message.
**
** History:
**      08-Oct-89 (seiwald)
**          Written.
**	13-sep-93 (johnst)
**	    Integrated from 6.4 codeline the Asiatech (kchin) change for 
**	    axp_osf 64-bit platform. This is the (seiwald) fix to gca_to_len() 
**	    and gca_to_fmt() to format GCA messages for platforms that use
**	    64-bit pointers:
**      	03-Feb-93 (seiwald)
**          	Roll the GCA_OBJECT_DESC element by element to avoid including
**          	the structure pads present on 64 bit pointer machines.
**	 1-Feb-96 (gordy)
**	    Support compressed VARCHARs.
**	31-Jan-97 (gordy)
**	    Support formatted descriptors in gca_to_len(), gca_to_fmt().
**	21-Dec-01 (gordy)
**	    Removed PTR from GCA_COL_ATT and GCA_ELEMENT.
**	22-Mar-02 (gordy)
**	    Change remaining references from NAT to I4.
**	16-Jan-03 (gordy)
**	    GCA_NOTARRAY value changed in 2.5 release.  GCA_TO_DESCR really
**	    shouldn't be dependent on this value, but apparently the IBM
**	    GCC does not like the change.  Send 0 instead (original value).
*/

i4
gca_to_len( bool frmtd, PTR ptr )
{
    i4	count;
    
    if ( frmtd )
	count = ((GCD_TD_DATA *)ptr)->gca_l_col_desc;
    else
    {
	u_char *descr = (u_char *)ptr + (sizeof(i4) * 3);
	GCA_GETI4_MACRO( descr, &count );
    }

    return( GCA_OBJ_SIZE( count ) );
}

VOID
gca_to_fmt( bool frmtd, PTR ptr, GCA_OBJECT_DESC *tod )
{
    GCD_TD_DATA *fmt_descr = (GCD_TD_DATA *)ptr;
    char	*tod_ptr = (char *)tod;
    char	*col_ptr;
    i4		flags, count;
    i4		i, tmpi4;
    i2		tmpi2;

    /*
    ** Read tuple descriptor header
    */
    if ( frmtd )
    {
	flags = fmt_descr->gca_result_modifier;
	count = fmt_descr->gca_l_col_desc;
	col_ptr = (char *)fmt_descr->gca_col_desc;
    }
    else
    {
	char *descr = (char *)ptr;
	descr += GCA_GETI4_MACRO( descr, &tmpi4 );  /* gca_tsize */
	descr += GCA_GETI4_MACRO( descr, &flags );  /* gca_result_modifier */
	descr += GCA_GETI4_MACRO( descr, &tmpi4 );  /* gca_id_tdscr */
	descr += GCA_GETI4_MACRO( descr, &count );  /* gca_l_col_desc */
	col_ptr = descr;			    /* gca_col_desc */
    }

    /*
    ** Write object descriptor header
    */
    MEfill( GCA_OBJ_NAME_LEN, EOS, (PTR)tod_ptr );
    tod_ptr += GCA_OBJ_NAME_LEN;			/* data_object_name */
    tmpi4 = GCA_IGNPRCSN | GCA_IGNLENGTH | (flags & GCA_COMP_MASK);
    tod_ptr += GCA_PUTI4_MACRO( &tmpi4, tod_ptr );	/* flags */
    tod_ptr += GCA_PUTI4_MACRO( &count, tod_ptr );	/* el_count */

    for( i = 0; i < count; i++ )
    {
	i2	type, prec;
	i4	length;

	/*
	** Read column description
	*/
	if ( frmtd )
	{
	    GCD_COL_ATT	*col_fmt = &((GCD_COL_ATT *)col_ptr)[i];

	    type = col_fmt->gca_attdbv.db_datatype;
	    prec = col_fmt->gca_attdbv.db_precision;
	    length = col_fmt->gca_attdbv.db_length;
	}
	else
	{
	    col_ptr += GCA_GETI4_MACRO( col_ptr, &tmpi4 );   /* db_data */
	    col_ptr += GCA_GETI4_MACRO( col_ptr, &length );  /* db_length */
	    col_ptr += GCA_GETI2_MACRO( col_ptr, &type );    /* db_datatype */
	    col_ptr += GCA_GETI2_MACRO( col_ptr, &prec );    /* db_prec */
	    col_ptr += GCA_GETI4_MACRO( col_ptr, &tmpi4 );   /* gca_l_attname */
	    col_ptr += tmpi4;				     /* gca_attname */
	}

	/*
	** Write object description
	*/
	tod_ptr += GCA_PUTI2_MACRO( &type, tod_ptr );		/* type */
	tod_ptr += GCA_PUTI2_MACRO( &prec, tod_ptr );		/* prec */
	tod_ptr += GCA_PUTI4_MACRO( &length, tod_ptr );		/* length */
	tmpi4 = 0;
	tod_ptr += GCA_PUTI4_MACRO( &tmpi4, tod_ptr );		/* od_ptr */
	tmpi4 = 0;
	tod_ptr += GCA_PUTI4_MACRO( &tmpi4, tod_ptr );		/* arr_stat */
    }

    return;
}


/*
** Name: gca_resolved() - unpack GCN_RESOLVED message in GCA_RQNS
**
** Description:
**	Unpack the response to a GCN_NS_RESOLVE request in GCA_RQNS.
**	The following response messages are supported:
**
**	GCN_RESOLVED	Original response.  Remote address count added
**			at GCA_PROTOCOL_LEVEL_40.
**	GCN2_RESOLVED	Response at GCA_PROTOCOL_LEVEL_63.
**
**	If a message length is provided, the message is saved in a
**	dynamically allocated buffer (rqcb->buffer).  If the length
**	is zero, the message is used in-place and must not be modified
**	while the Name Server info (rqcb->ns) is being used.
**
** Input:
**	length		Length of message, may be 0.
**	buffer		Message.
**	rqcb		Request control block.
**	protocol	Negotiated protocol level.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or E_GC0013_ASSFL_MEM.
**
** History:
**	26-Dec-91 (seiwald)
**	    Extracted from gca_request().
**	29-May-97 (gordy)
**	    New RESOLVED format at PROTOCOL_LEVEL_63.
**	 4-Dec-97 (rajus01)
**	    Length of remote auth and remote auth is passed in the
**	    resolved message.
**	12-Jan-98 (gordy)
**	    Host name added for local connections to support direct
**	    GCA network connections.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	14-May-98 (gordy)
**	    Added remote authentication mechanism.
**	19-May-98 (gordy)
**	    Allocate buffer to hold data (static buffers removed).
**	    Return error if no local partners.
*/

STATUS
gca_resolved( i4  length, char *buffer, GCA_RQCB *rqcb, i4 protocol )
{
    GCA_RQNS	*ns = &rqcb->ns;
    char	*ptr, *buf;
    i4		i, count, type;
    STATUS	status = OK;

    if ( ! length )
	buf = buffer;
    else
    {
	if ( ! (rqcb->rslv_buff = (char *)gca_alloc( length )) )
	    return( E_GC0013_ASSFL_MEM );

	MEcopy( buffer, length, rqcb->rslv_buff );
	buf = rqcb->rslv_buff;
    }

    if ( protocol < GCA_PROTOCOL_LEVEL_63 )
    {
	/* 
	** Count in GCN_RESOLVED introduced at level 4. 
	*/
	ns->rmt_count = 1;
	if ( protocol >= GCA_PROTOCOL_LEVEL_40 )
	    buf += gcu_get_int( buf, &ns->rmt_count);

	/*
	** Remote addresses.
	*/
	for( i = 0; i < ns->rmt_count; i++ )
	{
	    buf += gcu_get_str( buf, &ns->node[i] );
	    buf += gcu_get_str( buf, &ns->protocol[i] );
	    buf += gcu_get_str( buf, &ns->port[i] );
	}

	if ( ! ns->node[0][0] )  ns->rmt_count = 0;

	/* 
	** Remote connection info. 
	*/
	ns->lcl_user = "";
	buf += gcu_get_str( buf, &ns->rmt_user );
	buf += gcu_get_str( buf, &ns->rmt_pwd );
	buf += gcu_get_str( buf, &ns->rmt_db );

	/* 
	** Get local address type, count, & values. 
	*/
	buf += gcu_get_str( buf, &ptr );	/* Server class - unused */
	buf += gcu_get_int( buf, &ns->lcl_count );

	for( i = 0; i < ns->lcl_count; i++ )
	{
	    buf += gcu_get_str( buf, &ns->lcl_addr[i] );
	    ns->lcl_l_auth[i] = 0;
	}
    }
    else
    {
	/*
	** Local user ID.
	*/
	buf += gcu_get_str( buf, &ns->lcl_user );

	/*
	** Local connection address.
	*/
	buf += gcu_get_int( buf, &ns->lcl_count );

	for( i = 0; i < ns->lcl_count; i++ )
	{
	    buf += gcu_get_str( buf, &ns->lcl_host[i] );
	    buf += gcu_get_str( buf, &ns->lcl_addr[i] );
	    buf += gcu_get_int( buf, &ns->lcl_l_auth[i] );
	    ns->lcl_auth[i] = buf;
	    buf += ns->lcl_l_auth[i];
	}

	/*
	** Remote connection address.
	*/
	buf += gcu_get_int( buf, &ns->rmt_count);

	for( i = 0; i < ns->rmt_count; i++ )
	{
	    buf += gcu_get_str( buf, &ns->node[i] );
	    buf += gcu_get_str( buf, &ns->protocol[i] );
	    buf += gcu_get_str( buf, &ns->port[i] );
	}

	/*
	** Remote connection data
	*/
	ns->rmt_db = ns->rmt_user = ns->rmt_pwd = "";
	ns->rmt_auth = ns->rmt_mech = "";
	ns->rmt_emech = ns->rmt_emode = "";
	ns->rmt_auth_len = 0;
	buf += gcu_get_int( buf, &count );

	for( i = 0; i < count; i++ )
	{
	    buf += gcu_get_int( buf, &type );

	    switch( type )
	    {
		case GCN_RMT_DB :
		    buf += gcu_get_str( buf, &ns->rmt_db );
		    break;

		case GCN_RMT_USR :
		    buf += gcu_get_str( buf, &ns->rmt_user );
		    break;

		case GCN_RMT_PWD :
		    buf += gcu_get_str( buf, &ns->rmt_pwd );
		    break;

		case GCN_RMT_AUTH :
		    buf += gcu_get_int( buf, &ns->rmt_auth_len );
		    ns->rmt_auth = buf;
		    buf += ns->rmt_auth_len;
		    break;

		case GCN_RMT_EMECH :
		    buf += gcu_get_str( buf, &ns->rmt_emech );
		    break;

		case GCN_RMT_EMODE :
		    buf += gcu_get_str( buf, &ns->rmt_emode );
		    break;

		case GCN_RMT_MECH :
		    buf += gcu_get_str( buf, &ns->rmt_mech );
		    break;

		default :
		    GCA_DEBUG_MACRO(3)
			("unknown remote data type: %d\n", type );
		    buf += gcu_get_int( buf, &type );	/* get length */
		    buf += type;			/* skip value */
		    break;
	    }
	}
    }

    if ( ! ns->lcl_count )  status = E_GC0021_NO_PARTNER;

    return( status );
}
