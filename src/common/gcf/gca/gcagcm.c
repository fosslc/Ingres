/*
** Copyright (c) 1992, 2009 Ingres Corporation.
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>
#include    <si.h>
#include    <sl.h>
#include    <cs.h>
#include    <gc.h>
#include    <me.h>
#include    <mu.h>
#include    <qu.h>
#include    <sp.h>
#include    <st.h>
#include    <mo.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcm.h>
#include    <gcmint.h>


/**
**
**  Name: gcagcm.c
**
**  Description:
**      Contains the following functions:
**
**          gcm_response(): generate GCM response message
**          gcm_reg_trap(): register trap callback
**          gcm_unreg_trap(): register trap callback
**          gcm_monitor(): GCM monitor function (invoked by MO)
**          gcm_deliver(): send outstanding GCM trap indication messages
**          gcm_fs_completion(): completion exit for internal GCA_FASTSELECTs
**
**          gcm_put_str(): put string into message buffer
**          gcm_put_int(): put integer into message buffer
**          gcm_put_long(): put long integer into message buffer
**          gcm_get_str(): get string pointer from message buffer
**          gcm_get_int(): get integer from message buffer
**          gcm_get_long(): get long integer from message buffer
**
**          gcm_add_mon(): add monitor descriptor to table
**          gcm_find_mon(): find monitor descriptor in table
**          gcm_del_mon(): remove monitor descriptor from table
**          gcm_add_trap(): add trap descriptor to table
**          gcm_find_trap(): find trap descriptor in table
**          gcm_del_trap(): remove trap descriptor from table
**          gcm_add_work(): add msg work area to queue
**          gcm_del_work(): remove msg work area from queue
**          gcm_find_work(): find first msg work area on queue
**          gcm_set_delivery(): establish trap delivery thread
**          gcm_get_delivery(): retrieve trap delivery thread service parms
**
**  History:
**      28-Jun-92 (brucek)
**          Created.
**      11-Sep-92 (brucek)
**          Moved declarations of non-static functions to gcmint.h;
**	    convert to use of QU* routines for queue management.
**      16-Sep-92 (brucek)
**          Use ANSI C function prototypes;
**          convert to use real MO;
**          reject incomplete input messages;
**          support row_count of zero on GET and GETNEXT;
**	    set error_index to -1 if error detected in header.
**      24-Sep-92 (brucek)
**          Filter out extraneous traps as per trap_reason;
**	    fix usage of regexp arg to MOset_monitor().
**      30-Sep-92 (brucek)
**          Quit returning rows of response data after error.
**      07-Oct-92 (brucek)
**          Reject GCM_SET_TRAP messages if server hasn't registered
**	    for trap support.
**      14-Oct-92 (brucek)
**          Indicate various protocol errors with specific status codes.
**      15-Oct-92 (brucek)
**          Second try at supporting row_count of zero on GET and GETNEXT.
**	15-Oct-92 (seiwald)
**	    Ifndef'ed GCF64.
**	01-Dec-92 (brucek)
**	    Move element_count out of GCM_MSG_HDR so it's always right
**	    before the array of GCM_VAR_BINDINGs.
**	02-Dec-92 (brucek)
**	    Remove check of partner's protocol level: there's no need
**	    to disallow connections from newer-level clients since we
**	    guarantee that the contents of a given message never change.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	11-Jan-93 (brucek)
**	    Converted gcm_reg_trap into two GCA service routines callable
**	    via IIGCa_call interface: gcm_reg_trap and gcm_unreg_trap;
**	    Fill mon_table and trap_table with zeros.
**      23-Mar-93 (smc)
**          Fixed up forward declarations of static functions to not be
**          declared extern.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-Aug-93 (brucek)
**	    Added ignore-case argument on call to STbcompare() for
**	    GCM_UNSET_TRAP.
**      11-aug-93 (ed)
**          added missing includes
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	07-sep-93 (johnst)
**	    Added "lint truncation warning ... " comment for safe truncation
**	    case where value is used as structure offset.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	21-Sep-93 (brucek)
**	    Cast mon_table pointers to PTR for MEcopy() calls.
**	17-Jan-94 (brucek)
**	    Rework object descriptor usage to ensure that no gca_alloc()
**	    is done for >64K bytes; removed ifndefs GCF64.
**	14-Mar-94 (seiwald)
**	    Altered gcm_response() interface so as not to take a SVC_PARMS.
**	16-May-94 (seiwald)
**	    gcm_reg_trap() and gcm_unreg_trap() now take SVC_PARMS.
**	17-May-1994 (daveb) 59127
**	    Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**      12-jun-1995 (chech02)
**          Added semaphores to protect critical sections (MCT).
**	22-aug-1995 (canor01)
**	    replace CL_misc_sem with GC_misc_sem to protect 
**	    initialization of globals (MCT).
**	 3-Nov-95 (gordy)
**	    Removed MCT semaphores.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	 3-Apr-96 (gordy)
**	   Cast parameters in IIGCa_call().
**	 3-Sep-96 (gordy)
**	    Added GCA control blocks.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	29-Dec-97 (gordy)
**	    Fix building of GCA message header.
**	27-Jan-98 (gordy)
**	    Added trusted & user_name parameters to gcm_response()
**	    for checking user privileges.
**	21-May-98 (gordy)
**	    Pass in length of buffers to gcm_response().
**	 1-Jun-98 (gordy)
**	    Make sure buffer large enough for FASTSELECT.
**	15-Sep-98 (gordy)
**	    Added GCA flags to permit server to control GCM access.
**	22-Oct-98 (gordy)
**	    Added special handling for Name Server internal user ID:
**	    not trusted, no WRITE access, requires READ_ANY permission,
**	    may only access small set of Name Server MO objects.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	01-Apr-99 (kinte01)
**          Added include of si.h so definition of FILE will be included
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Apr-04 (gordy)
**	    Enhanced access handling for generic and unprivileged users.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**/


/*
**  local defines, typedefs and structures
*/


#define	MO_MAX_CLASSID		256
#define	MO_MAX_INSTANCE		256
#define	MO_MAX_VALUE		1024

typedef struct _GCM_OBJ_DESCR GCM_OBJ_DESCR;
typedef struct _GCM_MON_DESCR GCM_MON_DESCR;
typedef struct _GCM_TRAP_DESCR GCM_TRAP_DESCR;
typedef struct _GCM_MSG_WORK GCM_MSG_WORK;


    struct _GCM_OBJ_DESCR
    {
        i4      l_obj_class;
        char    *obj_class;
        i4      l_obj_inst;
        char    *obj_inst;
        i4      l_obj_value;
        char    *obj_value;
        i4      obj_perms;
    };

    struct _GCM_MON_DESCR
    {
        i4      mon_handle;
        i4      trap_reason;
        i4      trap_handle;
        i4      l_trap_address;
        char    *trap_address;
        i4      element_count;
        GCM_OBJ_DESCR *obj_descr;
    };

    struct _GCM_TRAP_DESCR
    {
        i4      trap_handle;
        VOID    (*trap_callback)();
    };

    struct _GCM_MSG_WORK
    {
        QUEUE    	q;
	bool		started;
        GCM_MON_DESCR   *mon_descr;
        GCA_FS_PARMS    parms;
        STATUS          status;
        i4         async_id;
        i4         async_flag;
        char            *data_area;
        char            buffer[sizeof(GCA_MSG_HDR) + GCA_FS_MAX_DATA];
        char            target[ 1 ];	/* Variable length */
    };


/*
**  local forward references.
*/

static VOID		gcm_check_init( VOID );
static STATUS		gcm_monitor( char *classid, char *twinid, 
				     char *instance, char *value, 
				     PTR mon_data, i4  message );

static VOID		gcm_fs_completion( i4 async_id );
static STATUS		gcm_add_mon( GCM_MON_DESCR *mon_descr );
static STATUS		gcm_del_mon( GCM_MON_DESCR *mon_descr );     
static GCM_MON_DESCR	*gcm_find_mon( i4  mon_handle );     
static STATUS		gcm_add_trap( GCM_TRAP_DESCR *trap_descr );     
static STATUS		gcm_del_trap( GCM_TRAP_DESCR *trap_descr );     
static GCM_TRAP_DESCR	*gcm_find_trap( i4  trap_handle );     
static STATUS		gcm_add_work( GCM_MSG_WORK *work_area );     
static STATUS		gcm_del_work( GCM_MSG_WORK *work_area );     
static GCM_MSG_WORK	*gcm_find_work( VOID );     
static GCA_SVC_PARMS	*gcm_get_delivery( VOID );     


/*
** Local variables.
*/

static	bool		gcm_initialized = FALSE;
static	MU_SEMAPHORE	gcm_semaphore;

/*
** Name: gcm_check_init
**
** Description:
**	Initialize GCM semaphore first time through.
**
** Input:
**	None
**
** Output:
**	None
**
** Returns:
**	void
*/

static VOID
gcm_check_init( VOID )
{
    if ( ! gcm_initialized )
    {
	gcm_initialized =TRUE;
	MUi_semaphore( &gcm_semaphore );
	MUn_semaphore( &gcm_semaphore, "GCM" );
    }

    return;
}



/*
** Name: gcm_response - generate GCM response message
**
** Description:
**	This routine generates a message in response to the GCM
**	query passed in.
**
** Inputs:
**	gcb		GCA control block.
**	acb		Association control block.
**	trusted		Is client trusted?
**	user_name	User ID of client.
**	in_msg_hdr	GCA message header.
**	in_data		GCA message.
**	in_data_length	Length of message.
**	out_msg_hdr	GCA message header area.
**	out_data	GCA message buffer.
**	out_data_length	Length of message buffer.
**
** Returns:
**	STATUS:		
**	    E_GC0000_OK			- all is well;
**	    E_GC0013_ASSFL_MEM 		- unable to allocate memory;
**	    E_GC0034_GCM_PROTERR	- generic protocol error;
**	    E_GC0035_GCM_INVLVL		- invalid protocol level;
**	    E_GC0036_GCM_NOEOD		- end_of_data not specified;
**	    E_GC0037_GCM_INVTYPE	- invalid message type;
**	    E_GC0038_GCM_INVCOUNT	- invalid element or row count;
**	    E_GC0039_GCM_NOTRAP		- no trap support available;
**	    E_GC003A_GCM_INVMON		- invalid monitor handle or description;
**	    E_GC003B_GCM_INVTRAP	- invalid trap handle;
**	    MO-derived status code	- various;
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	08-Jul-92 (brucek)
**	    Created.
**	17-Jan-96 (gordy)
**	    Since any error status is sent to client, do not return error
**	    to caller of this routine.  To the caller, we have successfully
**	    satisfied the request by returning the error to the client.
**	 3-Sep-96 (gordy)
**	    Added GCA control blocks.
**	29-Dec-97 (gordy)
**	    Fix building of GCA message header.
**	27-Jan-98 (gordy)
**	    Added trusted & user_name parameters for checking user privileges.
**	21-May-98 (gordy)
**	    Pass in length of buffers.
**	15-Sep-98 (gordy)
**	    Added GCA flags to permit server to control GCM access.
**	22-Oct-98 (gordy)
**	    Added special handling for Name Server internal user ID:
**	    not trusted, no WRITE access, requires READ_ANY permission,
**	    may only access small set of Name Server MO objects.
**	 6-Apr-04 (gordy)
**	    Enhanced generic and unprivileged access.  Generic access
**	    only allowed for READ operations with same restrictions as
**	    unprivileged access.  Server must grant GCM_READ_ANY or
**	    GCM_WRITE_ANY to allow unprivileged access.  Object access
**	    is then further restricted by MO_ANY_READ and MO_ANY_WRITE
**	    object attributes (rather than enforcing known objects).  
**	    Permission bits from unprivileged users are ignored since 
**	    they can't be trusted.  
**	21-Jul-09 (gordy)
**	    Dynamically allocate trap address.
*/

STATUS
gcm_response
( 
    GCA_CB	*gcb, 
    GCA_ACB	*acb, 
    bool	trusted,
    char	*user_name,
    GCA_MSG_HDR	*in_msg_hdr, 
    char	*in_data,
    i4		in_data_length,
    GCA_MSG_HDR	*out_msg_hdr, 
    char	*out_data, 
    i4		out_data_length
)
{

    bool		generic_user = FALSE;
    i4			in_perms;
    i4			in_row_count;
    i4			in_element_count;
    i4			in_trap_reason;
    i4			in_trap_handle;
    i4			in_mon_handle;
    i4			in_l_trap_address;
    char		*in_trap_address;
    bool		trap_hdr_present = FALSE;
    char		*out_data_end = out_data + out_data_length;
    i4			out_element_count = 0;
    i4			out_error_status = OK;
    i4			out_error_index = -1;
    i4			l_obj_array;
    GCM_OBJ_DESCR	*obj_array;
    GCM_OBJ_DESCR	*obj_ptr;
    char		class_buffer[MO_MAX_CLASSID + 1];
    char		instance_buffer[MO_MAX_INSTANCE + 1];
    char		value_buffer[MO_MAX_VALUE + 1];
    GCM_MON_DESCR	*mon_descr;
    GCM_TRAP_DESCR	*trap_descr;
    STATUS 		(*old_monitor)();     
    STATUS		mo_status;
    i4			mo_operation;
    char		*mo_regexp;
    char		*q;
    char		*r;
    i4			i;

    gcm_check_init();

    /*
    ** Requests made with the GCF reserved
    ** internal user ID are restricted.
    */ 
    if ( ! STcompare( GCN_NOUSER, user_name ) )
    {
	generic_user = TRUE;
	trusted = FALSE;
    }

    /*
    ** Clients with MONITOR privilege are trusted.
    */
    if ( ! trusted  &&  ! generic_user  &&
	 gca_chk_priv(user_name, GCA_PRIV_MONITOR) == OK )
    {
	trusted = TRUE;
    }

    out_msg_hdr->msg_type = GCM_RESPONSE;
    out_msg_hdr->data_offset = sizeof (GCA_MSG_HDR);
    out_msg_hdr->data_length = 0;
    out_msg_hdr->buffer_length = out_msg_hdr->data_offset;
    out_msg_hdr->flags.flow_indicator = GCA_NORMAL;
    out_msg_hdr->flags.end_of_data = TRUE;
    out_msg_hdr->flags.end_of_group = TRUE;

    /* position into response buffer */

    r = out_data;
    r += sizeof(i4);			/* bump past error_status */
    r += sizeof(i4);			/* bump past error_index */
    r += sizeof(i4);			/* bump past future[0] */
    r += sizeof(i4);			/* bump past future[1] */
    r += sizeof(i4);			/* bump past client_perms */
    r += sizeof(i4);			/* bump past row_count */
    r += sizeof(i4);			/* bump past element_count */

    /* validate input message header */
    if( in_msg_hdr->flags.end_of_data == FALSE )
    {
  	out_error_status = E_GC0036_GCM_NOEOD;
  	goto response_hdr;
    }

    switch ( in_msg_hdr->msg_type )
    {
	case GCM_GET:
	case GCM_GETNEXT:
	    /*
	    ** Requirements for READ:
	    **    o  READ can't be disabled.
	    **	  o  Client must be trusted or ANY may READ.
	    */
	    if ( gcb->gca_flags & GCA_GCM_READ_NONE  ||
	         ! (trusted  ||  gcb->gca_flags & GCA_GCM_READ_ANY) )
	    {
		out_error_status = E_GC003F_PM_NOPERM;
		goto response_hdr;
	    }
	    break;

	case GCM_SET_TRAP:
	case GCM_UNSET_TRAP:
	case GCM_TRAP_IND:
	    trap_hdr_present = TRUE;

	    /*
	    ** Requirements for TRAP:
	    **    o  Must be trusted.
	    */
	    if ( ! trusted )
	    {
		out_error_status = E_GC003F_PM_NOPERM;
		goto response_hdr;
	    }
	    break;

	case GCM_SET:
	    /*
	    ** Requirements for WRITE:
	    **    o  WRITE can't be disabled.
	    **    o  Can't be GCF reserved internal user ID (generic).
	    **	  o  Client must be trusted or ANY may WRITE.
	    */
	    if ( gcb->gca_flags & GCA_GCM_WRITE_NONE  ||  generic_user  ||
	         ! (trusted  ||  gcb->gca_flags & GCA_GCM_WRITE_ANY) )
	    {
		out_error_status = E_GC003F_PM_NOPERM;
		goto response_hdr;
	    }
	    break;

	case GCM_RESPONSE:
	    out_error_status = E_GC0034_GCM_PROTERR;
	    goto response_hdr;

	default:
	    out_error_status = E_GC0037_GCM_INVTYPE;
	    goto response_hdr;
    }

    /* 
    ** For SET_TRAP only, copy message data to persistent storage 
    */
    if ( in_msg_hdr->msg_type  == GCM_SET_TRAP )
    {
	mon_descr = (GCM_MON_DESCR *) gca_alloc( sizeof(GCM_MON_DESCR)
		+ in_data_length );

	if( !mon_descr )
	{
	    out_error_status = E_GC0013_ASSFL_MEM;
	    goto response_hdr;
	}

	MEcopy( in_data, in_data_length, 
		(PTR) mon_descr + sizeof(GCM_MON_DESCR) );

	in_data = (char *)mon_descr + sizeof(GCM_MON_DESCR);
    }


    /* read in query header */

    q = in_data;
    q += sizeof(i4);			/* bump past error_status */
    q += sizeof(i4);			/* bump past error_index */
    q += sizeof(i4);			/* bump past future[0] */
    q += sizeof(i4);			/* bump past future[1] */
    q += gcm_get_int(q, &in_perms);   	/* get client_perms */
    q += gcm_get_int(q, &in_row_count);	/* get row_count */

    /*
    ** Make sure only the permission bits are set.
    ** Ignore permission bits from untrusted clients
    ** (requires MO_ANY_READ|MO_ANY_WRITE attributes).
    */
    in_perms = trusted ? in_perms & MO_PERMS_MASK : 0;

    if( trap_hdr_present )
    {
        q += gcm_get_int(q, &in_trap_reason);
        q += gcm_get_int(q, &in_trap_handle);
        q += gcm_get_int(q, &in_mon_handle);
        q += gcm_get_str(q, &in_trap_address, &in_l_trap_address);
    }

    q += gcm_get_int(q, &in_element_count);	/* get element_count */


    /* validate input element count */

    if( in_element_count < 1 )
    {
	if( in_element_count < 0 )
            out_error_status = E_GC0038_GCM_INVCOUNT;
        goto response_hdr;
    }


    /* allocate object descriptor array */

    l_obj_array = in_element_count * sizeof(GCM_OBJ_DESCR);

    if( l_obj_array > acb->sz_to_descr )
    {
	if( acb->to_descr )
	    gca_free(acb->to_descr);

	if( !(acb->to_descr = gca_alloc(l_obj_array)) )
	{
	    acb->sz_to_descr = 0;
	    out_error_status = E_GC0013_ASSFL_MEM;
	    goto response_hdr;
	}

	acb->sz_to_descr = l_obj_array;
    }

    obj_array = (GCM_OBJ_DESCR *)acb->to_descr;


    /* initialize object descriptor array */

    for (i = 0, obj_ptr = obj_array; i < in_element_count; i++, obj_ptr++)
    {
	char	*c;
	i4	len;

	q += gcm_get_str(q, &c, &len);		/* get input classid */
	obj_ptr->l_obj_class = len;
	obj_ptr->obj_class = c;

	q += gcm_get_str(q, &c, &len);		/* get input instance */
	obj_ptr->l_obj_inst = len;
	obj_ptr->obj_inst = c;

	q += gcm_get_str(q, &c, &len);		/* get input value */
	obj_ptr->l_obj_value = len;
	obj_ptr->obj_value = c;

	q += gcm_get_int(q, &obj_ptr->obj_perms);	/* get input perms */
    }


    /* initialize per specific message type */

    switch ( in_msg_hdr->msg_type )
    {
	case GCM_GET:
	case GCM_GETNEXT:
	    /*
	    **  Validate row count.
	    **  If row count of zero specified, set to -1 to cause
	    **  semi-permanent loop.
	    */

	    if( in_row_count < 0 )
	    {
	        out_error_status = E_GC0038_GCM_INVCOUNT;
	        goto response_hdr;
	    }

	    if ( !in_row_count )  in_row_count = -1;
	    break;

	case GCM_SET:
	    /*
	    **  Force row count to 1.
	    */

	    in_row_count = 1;
	    break;

	case GCM_SET_TRAP:
	    /*
	    **  Validate that trap support is enabled.
	    **  Initialize monitor descriptor.
	    **  Reset ACB pointer to object array (it must persist).
	    **  Add monitor descriptor to table.
	    **  Set "error index" to monitor handle (if error_status is 0
	    **  then error_index is used to pass back the monitor handle).
	    **  Force row count to 1.
	    */

	    if ( ! (gcb->gca_flags & GCA_RG_TRAP) )
	    {
		out_error_status = E_GC0039_GCM_NOTRAP;
		goto response_hdr;
	    }

	    mon_descr->mon_handle = 0;
	    mon_descr->trap_reason = in_trap_reason;
	    mon_descr->trap_handle = in_trap_handle;

	    mon_descr->trap_address = (char *)gca_alloc(in_l_trap_address + 1);

	    if ( ! mon_descr->trap_address )
	    {
		gca_free( (PTR)mon_descr );
		out_error_status = E_GC0013_ASSFL_MEM;
		goto response_hdr;
	    }

	    mon_descr->l_trap_address = in_l_trap_address;
	    MEcopy( in_trap_address, 
	    	    in_l_trap_address, mon_descr->trap_address );
	    mon_descr->trap_address[ in_l_trap_address ] = '\0';
	    mon_descr->element_count = in_element_count;
	    mon_descr->obj_descr = obj_array;
	    acb->to_descr = NULL;

	    if( gcm_add_mon(mon_descr) != OK )
	    {
		gca_free( (PTR)mon_descr->trap_address );
		gca_free( (PTR)mon_descr );
	        out_error_status = E_GC0013_ASSFL_MEM;
	        goto response_hdr;
	    }

	    out_error_index = mon_descr->mon_handle;
	    in_row_count = 1;
	    break;

	case GCM_UNSET_TRAP:
	    /*
	    **  Find monitor descriptor in table.
	    **  Validate trap reason, handle, and address.
	    **  Use object array from monitor descriptor rather than message.
	    **  Force row count to 1.
	    */

	    if( !(mon_descr = gcm_find_mon(in_mon_handle) ) )
	    {
	        out_error_status = E_GC003A_GCM_INVMON;
	        goto response_hdr;
	    }

	    if( mon_descr->trap_reason != in_trap_reason ||
	        mon_descr->trap_handle != in_trap_handle ||
	        mon_descr->l_trap_address != in_l_trap_address ||
	        STncmp(mon_descr->trap_address, in_trap_address,
		    in_l_trap_address ) != 0 )
	    {
	        out_error_status = E_GC003A_GCM_INVMON;
		mon_descr = NULL;
	        goto response_hdr;
	    }

	    obj_array = mon_descr->obj_descr;
	    in_row_count = 1;
	    break;

	case GCM_TRAP_IND:
	    /*
	    **  Find trap descriptor in table.
	    **  Invoke trap callback.
	    **  No further processing.
	    */

	    if( trap_descr = gcm_find_trap(in_trap_handle) ) 
		(*trap_descr->trap_callback)(-1 /*XXX in_buffer*/);
	    else
	        out_error_status = E_GC003B_GCM_INVTRAP;

	    goto response_hdr;
    }

    /* process each variable in object array */
    mo_operation = in_msg_hdr->msg_type;

    while ( in_row_count-- )
    {
	for (i = 0, obj_ptr = obj_array; i < in_element_count; i++, obj_ptr++)
	{
	    MEcopy(obj_ptr->obj_class, obj_ptr->l_obj_class, class_buffer);
	    class_buffer[obj_ptr->l_obj_class] = '\0';
	    MEcopy(obj_ptr->obj_inst, obj_ptr->l_obj_inst, instance_buffer);
	    instance_buffer[obj_ptr->l_obj_inst] = '\0';
	    MEcopy(obj_ptr->obj_value, obj_ptr->l_obj_value, value_buffer);
	    value_buffer[obj_ptr->l_obj_value] = '\0';

	    switch (mo_operation)
	    {
		case GCM_GET:
		    obj_ptr->l_obj_value = MO_MAX_VALUE;
		    mo_status = MOget( in_perms,
				class_buffer,
				instance_buffer,
				&obj_ptr->l_obj_value,
				value_buffer,
				&obj_ptr->obj_perms );
		    break;
			
		case GCM_GETNEXT:
		    obj_ptr->l_obj_class = MO_MAX_CLASSID;
		    obj_ptr->l_obj_inst = MO_MAX_INSTANCE;
		    obj_ptr->l_obj_value = MO_MAX_VALUE;
		    mo_status = MOgetnext( in_perms,
				obj_ptr->l_obj_class,
				obj_ptr->l_obj_inst,
				class_buffer,
				instance_buffer,
				&obj_ptr->l_obj_value,
				value_buffer,
				&obj_ptr->obj_perms );
		    break;

		case GCM_SET:
		    mo_status = MOset( in_perms,
				class_buffer,
				instance_buffer,
				value_buffer );
		    obj_ptr->obj_perms = 0;
		    break;

		case GCM_SET_TRAP:
		    mo_regexp = instance_buffer[0] ? instance_buffer : NULL;
		    mo_status = MOset_monitor(
				class_buffer,
				(PTR) (SCALARP)mon_descr->mon_handle,
				mo_regexp,
				gcm_monitor,
				&old_monitor );
		    obj_ptr->obj_perms = 0;
		    obj_ptr->obj_value[0] = '\0';
		    break;

		case GCM_UNSET_TRAP:
		    mo_regexp = instance_buffer[0] ? instance_buffer : NULL;
		    mo_status = MOset_monitor(
				class_buffer,
				(PTR) (SCALARP)mon_descr->mon_handle,
				mo_regexp,
				NULL,
				&old_monitor );
		    obj_ptr->obj_perms = 0;
		    obj_ptr->obj_value[0] = '\0';
		    break;
	    }


	    /* update object descriptor string lengths */

            obj_ptr->l_obj_class = STlength(class_buffer);
            obj_ptr->l_obj_inst = STlength(instance_buffer);
            obj_ptr->l_obj_value = STlength(value_buffer);


	    /* test for buffer overflow */

	    if ( (r + sizeof (i4) + obj_ptr->l_obj_class +
		      sizeof (i4) + obj_ptr->l_obj_inst +
		      sizeof (i4) + obj_ptr->l_obj_value +
		      sizeof (i4)) > out_data_end )
		goto response_hdr;

	    obj_ptr->obj_class = r + sizeof(i4);
	    r += gcm_put_str(r, class_buffer);
	    obj_ptr->obj_inst = r + sizeof(i4);
	    r += gcm_put_str(r, instance_buffer);
	    obj_ptr->obj_value = r + sizeof(i4);
	    r += gcm_put_str(r, value_buffer);
	    r += gcm_put_int(r, obj_ptr->obj_perms);

	    if( mo_status != OK && out_error_index < 0 )
	    {
		out_error_status = mo_status;
		out_error_index = out_element_count;
		in_row_count = 0;
	    }

	    out_element_count++;
	}

	mo_operation = GCM_GETNEXT;
    }

    /*
    ** Set fields in header of response message 
    */

response_hdr:

    out_msg_hdr->data_length = r - out_data;	/* set output message length */
    out_msg_hdr->buffer_length = out_msg_hdr->data_offset +
				 out_msg_hdr->data_length;

    r = out_data;				/* position back to start */
    r += gcm_put_long(r, out_error_status);	/* set error_status */
    r += gcm_put_int(r, out_error_index);	/* set error_index */
    r += gcm_put_int(r, 0);			/* clear future[0] */
    r += gcm_put_int(r, 0);			/* clear future[1] */
    r += gcm_put_int(r, 0);			/* clear client_perms */
    r += gcm_put_int(r, 0);			/* clear row_count */
    r += gcm_put_int(r, out_element_count);	/* set element_count */

    if( in_msg_hdr->msg_type == GCM_UNSET_TRAP && mon_descr )
    {
	gcm_del_mon(mon_descr);
        gca_free( (PTR)mon_descr->obj_descr );
	gca_free( (PTR)mon_descr->trap_address );
        gca_free( (PTR)mon_descr );
    }

    return( OK );
}


/*{
** Name: gcm_reg_trap - register GCM trap callback function
**
** Revision:
**	This applies to GCA_API_LEVEL_4 or greater.
**
** Description:
**	This is a GCA service entered via the IIGCa_call interface.
**	It registers a callback function to be driven when a GCM trap
**	indication is received.
**
** Inputs:
**	gca_trap_callback
**	    Trap indication callback function.
**
** Outputs:
**	gca_trap_handle
**	    Identifier for newly-registered callback.
**	gca_status
**	    E_GC0000_OK			Normal completion
**	    E_GC0013_ASSFL_MEM		Unable to allocate memory for new trap
**		
**
** History:
**      11-Jan-93 (brucek)
**	    Created.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

VOID
gcm_reg_trap( GCA_SVC_PARMS *svc_parms )
{
    STATUS		status;
    GCM_TRAP_DESCR	*trap_descr;
    GCA_RT_PARMS 	*rt_parms = (GCA_RT_PARMS *)svc_parms->parameter_list;

    /* allocate a new trap descriptor */

    trap_descr = (GCM_TRAP_DESCR *) gca_alloc(sizeof(GCM_TRAP_DESCR));

    if( !trap_descr )
    {
	rt_parms->gca_status = E_GC0013_ASSFL_MEM;
	return;
    }

    /* add new descriptor to trap table */

    trap_descr->trap_callback = rt_parms->gca_trap_callback;
    status = gcm_add_trap(trap_descr);

    if( status != OK )
    {
	rt_parms->gca_status = E_GC0013_ASSFL_MEM;
	return;
    }

    rt_parms->gca_trap_handle = trap_descr->trap_handle;
    rt_parms->gca_status = E_GC0000_OK;
    return;
}



/*{
** Name: gcm_unreg_trap - unregister GCM trap callback function
**
** Revision:
**	This applies to GCA_API_LEVEL_4 or greater.
**
** Description:
**	This is a GCA service entered via the IIGCa_call interface.
**	It removes a previously registered trap callback function.
**
** Inputs:
**	gca_trap_handle
**	    Identifier for callback to unregister.
**
** Outputs:
**	gca_status
**	    E_GC0000_OK			Normal completion
**	    E_GC003B_GCM_INVTRAP	Invalid trap identifier specified
**		
**
** History:
**      11-Jan-93 (brucek)
**	    Created.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

VOID
gcm_unreg_trap( GCA_SVC_PARMS *svc_parms )
{
    STATUS		status;
    GCM_TRAP_DESCR	*trap_descr;
    GCA_UT_PARMS 	*ut_parms = (GCA_UT_PARMS *)svc_parms->parameter_list;


    /* find trap descriptor associated with input handle */

    trap_descr = gcm_find_trap(ut_parms->gca_trap_handle);

    if( !trap_descr )
    {
	ut_parms->gca_status = E_GC003B_GCM_INVTRAP;
	return;
    }

    /* remove trap descriptor from trap table */

    status = gcm_del_trap(trap_descr);

    if( status != OK )
    {
	ut_parms->gca_status = E_GC003B_GCM_INVTRAP;
	return;
    }

    gca_free( (PTR)trap_descr );
    ut_parms->gca_status = E_GC0000_OK;
    return;
}



/*
** Name: gcm_monitor() - GCM monitor function 
**
** Description:
**	This is the monitor function for GCM traps.  It is invoked
**	by MO either explicitly via MOtell_monitor or implicitly
**	within the get or set method for a given object.
**
**	This routine generates a trap indication message, and 
**	allocates and initializes a message work area which
**	contains the GCA parameter list and associated parameters 
**	necessary to allow the trap indication to be delivered.
**
**	It also calls gcm_deliver() to ensure that delivery will
**	occur in the case where there is no currently outstanding
**	asynchronous request to cause gcm_deliver() to be driven.
**
** Returns:
**	STATUS.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
**	 3-Apr-96 (gordy)
**	   Cast parameters in IIGCa_call().
**	21-Jul-09 (gordy)
**	    Work buffer is dynamic length to handle long target strings.
*/

static STATUS
gcm_monitor( char *classid, char *twinid, char *instance, 
	     char *value, PTR mon_data, i4  message )
{
    GCM_MON_DESCR	*mon_descr;
    GCM_OBJ_DESCR	*obj_ptr;
    GCM_MSG_WORK	*work_ptr;
    GCA_FO_PARMS 	fo_parms;
    i4  		mon_handle;
    char		*object_id;
    i4 		error_status;
    STATUS		call_status;
    char		*q;
    i4			i;


    /* find monitor descriptor */

    mon_handle = (i4)(SCALARP) mon_data;
    if( !(mon_descr = gcm_find_mon(mon_handle) ) )
	return OK;


    /* filter out traps by trap reason */

    if( !( mon_descr->trap_reason & GCM_TRAP_MASK(message) ) )
	return OK;


    /* 
    ** determine object id to report trap on 
    **
    ** If neither classid nor twinid are found in monitor descriptor,
    ** just ignore this trap.  This is an error, but it's hard to say
    ** whose, and anyway there's nothing much we can do at this point.
    */

    obj_ptr = mon_descr->obj_descr;
    object_id = NULL;

    for( i = 0; i < mon_descr->element_count; i++, obj_ptr++)
    {
	if( classid && STcompare(obj_ptr->obj_class, classid) == 0 )
	{
	    object_id = classid;
	    break;
	}
	if( twinid && STcompare(obj_ptr->obj_class, twinid) == 0 )
	{
	    object_id = twinid;
	    break;
	}
    }

    if( !object_id )
	return FAIL;


    /* Allocate message work area */ 

    if ( ! (work_ptr = (GCM_MSG_WORK *)gca_alloc( sizeof(GCM_MSG_WORK) +
			STlength( mon_descr->trap_address ) + 2 )) )
	return FAIL;

    work_ptr->mon_descr = mon_descr;
    work_ptr->started = FALSE;
    work_ptr->async_flag = GCA_ASYNC_FLAG | GCA_ALT_EXIT;

    work_ptr->target[0] = '/';
    work_ptr->target[1] = '@';
    STcopy(mon_descr->trap_address, work_ptr->target + 2);


    /* Invoke GCA_FORMAT */

    fo_parms.gca_buffer = (PTR) work_ptr->buffer;
    fo_parms.gca_b_length = sizeof(work_ptr->buffer);

    call_status = IIGCa_call(   GCA_FORMAT,
                                (GCA_PARMLIST *)&fo_parms,
                                (i4) GCA_SYNC_FLAG,
                                (PTR) 0,
                                (i4) -1,
                                &work_ptr->status);

    if( call_status != OK || work_ptr->status != OK )
    {
	gca_free( (PTR)work_ptr );
	return FAIL;
    }

    work_ptr->data_area = fo_parms.gca_data_area;


    /* Format GCM_TRAP_IND message */
 
    q = work_ptr->data_area;
    q += gcm_put_long(q, 0);		/* bump past error_status */
    q += gcm_put_int(q, 0);		/* bump past error_index */
    q += gcm_put_int(q, 0);		/* bump past future[0] */
    q += gcm_put_int(q, 0);		/* bump past future[1] */
    q += gcm_put_int(q, 0);	   	/* set client_perms */
    q += gcm_put_int(q, 1);		/* set row_count */

    q += gcm_put_int(q, message);			/* set trap_reason */
    q += gcm_put_int(q, mon_descr->trap_handle);	/* set trap_handle */
    q += gcm_put_int(q, mon_handle);			/* set mon_handle */
    q += gcm_put_str(q, mon_descr->trap_address);	/* set trap_address */

    q += gcm_put_int(q, 1);		/* set element_count */
    q += gcm_put_str(q, object_id);	/* set classid or twinid */
    q += gcm_put_str(q, instance);	/* set instance */
    q += gcm_put_str(q, value);		/* set value */
    q += gcm_put_int(q, 0);		/* set perms */


    /* Set up GCA_FASTSELECT parameters */
	
    work_ptr->parms.gca_partner_name = work_ptr->target;
    work_ptr->parms.gca_modifiers = GCA_RQ_GCM;
    work_ptr->parms.gca_buffer = (PTR) work_ptr->buffer;
    work_ptr->parms.gca_b_length = sizeof work_ptr->buffer;
    work_ptr->parms.gca_user_name = NULL;
    work_ptr->parms.gca_password = NULL;
    work_ptr->parms.gca_completion = gcm_fs_completion;

    work_ptr->parms.gca_message_type = GCM_TRAP_IND;
    work_ptr->parms.gca_msg_length = q - work_ptr->data_area;

    gcm_add_work(work_ptr);
    gcm_deliver();

    return( OK );
}


/*
** Name: gcm_deliver() - deliver outstanding trap indications
**
** Description:
**	This routine is called by gca_listen() when a previous
**	asynchronous operation either completes or needs to be
**	resumed.  It is also called by gcm_monitor() when a new 
**	trap indication	message is added to the queue.
**
**	Generally, it will RESUME the current GCA_FASTSELECT, if 
**	any, or else kick off the next GCA_FASTSELECT, if any.
**
**	It may quite possibly do nothing at all, in the case where
**	it is called by	gcm_monitor() while the previous asynchronous
**	operation is still pending.
**
** Returns:
**	Status:
**	    OK.
**
** History:
**	08-Sep-92 (brucek)
**	   Created.
**	 3-Apr-96 (gordy)
**	   Cast parameters in IIGCa_call().
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	21-Jul-09 (gordy)
**	    Free dynamic trap address resources.
*/

STATUS
gcm_deliver( VOID )
{
    STATUS		status;
    STATUS		call_status;
    GCM_MSG_WORK	*work_ptr;
    GCM_MON_DESCR	*mon_descr;


    /* find next outstanding message to move along */

deliver_next_msg:

    if( !(work_ptr = gcm_find_work()) )
	return OK;

    if(	work_ptr->parms.gca_status == E_GCFFFF_IN_PROCESS )
	return OK;


    /* Invoke GCA_FASTSELECT if operation not yet started or incomplete */

    if( work_ptr->started == FALSE ||
	work_ptr->parms.gca_status == E_GCFFFE_INCOMPLETE )
    {
        call_status =  IIGCa_call(GCA_FASTSELECT,
                                 (GCA_PARMLIST *)&work_ptr->parms,
                                 work_ptr->async_flag,
                                 (PTR)(SCALARP)work_ptr->async_id,
                                 (i4) 10000,
                                 &work_ptr->status);

        if( call_status != OK || work_ptr->status != OK )
        {
    	    gcm_del_work(work_ptr);
	    gca_free( (PTR)work_ptr );
	    goto deliver_next_msg;
        }
	
	work_ptr->started = TRUE;
	work_ptr->async_flag |= GCA_RESUME;
	return OK;
    }


    /* if successful completion, check status from response message */

    if( work_ptr->parms.gca_status == E_GC0000_OK )
    {
        char		*q;
        i4		error_status;

        q = work_ptr->data_area;
        q += gcm_get_long(q, &error_status);

        if( error_status )
        {
    	    gcm_del_mon(work_ptr->mon_descr);
            gca_free( (PTR)work_ptr->mon_descr->obj_descr );
	    gca_free( (PTR)work_ptr->mon_descr->trap_address );
            gca_free( (PTR)work_ptr->mon_descr );
        }
    }

    gcm_del_work(work_ptr);
    gca_free( (PTR)work_ptr );
    goto deliver_next_msg;
}


/*
** Name: gcm_fs_completion() - GCA_FASTSELECT completion exit
**
** Description:
**	This is the completion exit for the GCA_FASTSELECT calls
**	issued by gcm_monitor() and gcm_deliver().  
**
**	It drives the completion exit for the trap delivery thread
**	with status INCOMPLETE, to allow the caller of GCA_LISTEN
**	to RESUME that thread in the proper context.
**
** Returns:
**	None.
**
** History:
**	08-Sep-92 (brucek)
**	   Created.
*/

static VOID
gcm_fs_completion( i4 async_id )
{
    GCA_SVC_PARMS	*svc_parms;

    svc_parms = gcm_get_delivery();
    gca_resume( (PTR)svc_parms );
    return;
}


/*
** Name: gcm_put_str() - place variable length string into buffer.
**
** Description:
**	Places string s into buffer p in length/value format.
**	Handles NULL s pointer as an empty string.
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_put_str( char *p, char *s )
{
	i4	len;

	if( !s || !*s )
	{
		len = 0;
		I4ASSIGN_MACRO( len, *p );
	}
	else
	{
		len = STlength( s );
		I4ASSIGN_MACRO( len, *p );
		MEcopy( s, len, p + sizeof(i4) );
	}

	return sizeof(i4) + len;
}

/*
** Name: gcm_put_int() - place integer into buffer.
**
** Description:
**	Places int i into buffer p.
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_put_int( char *p, i4  i )
{
	i4	i4val = i;

	I4ASSIGN_MACRO( i4val, *p );

	return sizeof(i4);
}


/*
** Name: gcm_put_long() - place long integer into buffer.
**
** Description:
**	Places long i into buffer p.
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_put_long( char *p, i4 i )
{
	i4	i4val = i;

	I4ASSIGN_MACRO( i4val, *p );

	return sizeof(i4);
}

/*
** Name: gcm_get_str() - get var string from buffer.
**
** Description:
**	Sets char ** ss to string in buffer p and l to length of string. 
**	If the string is 0 length, points ss at the constant "".
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_get_str( char *p, char **ss, i4  *l )
{
	i4	len;

	I4ASSIGN_MACRO( *p, len );

	if( len )
	    *ss = p + sizeof(i4);
	else
	    *ss = "";

	*l = len;

	return sizeof(i4) + len;
}

/*
** Name: gcm_get_int() - get integer from buffer.
**
** Description:
**	Gets integer i from buffer p.
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_get_int( char *p, i4  *i )
{
	i4	i4val;

	I4ASSIGN_MACRO( *p, i4val );

	*i = i4val;

	return sizeof(i4);
}

/*
** Name: gcm_get_long() - get long integer from buffer.
**
** Description:
**	Gets long integer i from buffer p.
**
** Returns:
**	Amount of buffer used.
**
** History:
**	08-Jul-92 (brucek)
**	   Created.
*/

i4
gcm_get_long( char *p, i4 *i )
{
	i4	i4val;

	I4ASSIGN_MACRO( *p, i4val );

	*i = i4val;

	return sizeof(i4);
}


/*
**  GCM static storage
*/

static	i4		l_mon_table = 0;
static	GCM_MON_DESCR	**mon_table = NULL;
static	i4		l_trap_table = 0;
static	GCM_TRAP_DESCR	**trap_table = NULL;
static	i4		work_unique_id = 0;
static	QUEUE		work_queue = {NULL, NULL};
static	GCA_SVC_PARMS	*deliver_parms = NULL;

/*
** Name: gcm_add_mon() - add mon_descr to mon_table
**
** Description:
**	This routine scans the monitor descriptor table to find the
**	next available empty slot, which it fills with the input mon_descr.
**
**	If the table is full, a new table is allocated which is twice the
**	size of the old table, and all entries are copied over.
**
** Returns:
**	OK, if mon_descr added to table.
**	FAIL, if no available slot and table could not be re-sized.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_add_mon( GCM_MON_DESCR *mon_descr )
{
    STATUS		status;
    i4			i;
    i4			new_l_mon_table;
    GCM_MON_DESCR	**new_mon_table;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    /* scan monitor table for empty slot */

    for( i = 0; i < l_mon_table; i++ )
    {
	if( mon_table[i] == NULL )
	{
	    mon_descr->mon_handle = i;
	    mon_table[i] = mon_descr;
	    status = OK;
	    goto mon_exit;
	}
    }


    /* no empty slot found -- grow monitor table */

    new_l_mon_table = l_mon_table ? (l_mon_table * 2) : 256;
    new_mon_table = (GCM_MON_DESCR **) gca_alloc(new_l_mon_table * sizeof(PTR));

    if( !new_mon_table )
    {
	status = FAIL;
	goto mon_exit;
    }

    MEfill( new_l_mon_table * sizeof(PTR), 0, new_mon_table );

    if( l_mon_table )
    {
	MEcopy((PTR)mon_table, l_mon_table * sizeof(PTR), (PTR)new_mon_table);
	gca_free( (PTR)mon_table );
    }

    mon_descr->mon_handle = l_mon_table;
    new_mon_table[l_mon_table] = mon_descr;

    mon_table = new_mon_table;
    l_mon_table = new_l_mon_table;
    status = OK;


mon_exit:

    /*  release semaphore */

    MUv_semaphore (&gcm_semaphore);

    return status;
}


/*
** Name: gcm_del_mon() - remove mon_descr from mon_table
**
** Description:
**	This routine removes a monitor descriptor from the monitor table.
**
** Returns:
**	OK, if successful.
**	FAIL, if invalid monitor descriptor.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_del_mon( GCM_MON_DESCR *mon_descr )
{
    if( mon_descr->mon_handle > l_mon_table )
	return FAIL;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    mon_table[mon_descr->mon_handle] = NULL;

    MUv_semaphore (&gcm_semaphore);

    return OK;
}


/*
** Name: gcm_find_mon() - find mon_descr in mon_table
**
** Description:
**	This routine finds the monitor descriptor corresponding to
**	a given monitor handle.
**
** Returns:
**	Pointer to monitor descriptor, or NULL, if invalid handle.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static GCM_MON_DESCR *
gcm_find_mon( i4  mon_handle )
{
    GCM_MON_DESCR	*mon_descr;

    if( mon_table == NULL || mon_handle > l_mon_table )
	return NULL;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    mon_descr = mon_table[mon_handle];

    MUv_semaphore (&gcm_semaphore);

    return mon_descr;
}


/*
** Name: gcm_add_trap() - add trap_descr to trap_table
**
** Description:
**	This routine scans the trap descriptor table to find the
**	next available empty slot, which it fills with the input trap_descr.
**
**	If the table is full, a new table is allocated which is twice the
**	size of the old table, and all entries are copied over.
**
** Returns:
**	OK, if trap_descr added to table.
**	FAIL, if no available slot and table could not be re-sized.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_add_trap( GCM_TRAP_DESCR *trap_descr )
{
    STATUS		status;
    i4			i;
    i4			new_l_trap_table;
    GCM_TRAP_DESCR	**new_trap_table;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);


    /* scan trap table for empty slot */

    for( i = 0; i < l_trap_table; i++ )
    {
	if( trap_table[i] == NULL )
	{
	    trap_descr->trap_handle = i;
	    trap_table[i] = trap_descr;
	    status = OK;
	    goto trap_exit;
	}
    }


    /* no empty slot found -- grow trap table */

    new_l_trap_table = l_trap_table ? (l_trap_table * 2) : 16;
    new_trap_table = (GCM_TRAP_DESCR **) 
	gca_alloc(new_l_trap_table * sizeof(PTR));

    if( !new_trap_table )
    {
	status = FAIL;
	goto trap_exit;
    }

    MEfill( new_l_trap_table * sizeof(PTR), 0, new_trap_table );

    if( l_trap_table )
    {
	MEcopy((PTR)trap_table, l_trap_table * sizeof(PTR), 
			(PTR)new_trap_table);
	gca_free( (PTR)trap_table );
    }

    trap_descr->trap_handle = l_trap_table;
    new_trap_table[l_trap_table] = trap_descr;

    trap_table = new_trap_table;
    l_trap_table = new_l_trap_table;
    status = OK;


trap_exit:

    /*  release semaphore */

    MUv_semaphore (&gcm_semaphore);

    return status;
}


/*
** Name: gcm_del_trap() - remove trap_descr from trap_table
**
** Description:
**	This routine removes the trap descriptor from the trap table.
**
** Returns:
**	OK, if successful.
**	FAIL, if invalid trap descriptor.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_del_trap( GCM_TRAP_DESCR *trap_descr )
{

    if( trap_descr->trap_handle > l_trap_table )
	return FAIL;

    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    trap_table[trap_descr->trap_handle] = NULL;

    MUv_semaphore (&gcm_semaphore);

    return OK;
}


/*
** Name: gcm_find_trap() - find trap_descr in trap_table
**
** Description:
**	This routine finds the trap descriptor corresponding to
**	a given trap handle.
**
** Returns:
**	Pointer to trap descriptor, or NULL, if invalid handle.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static GCM_TRAP_DESCR *
gcm_find_trap( i4  trap_handle )
{
    GCM_TRAP_DESCR	*trap_descr;

    if( trap_table == NULL || trap_handle > l_trap_table )
	return NULL;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    trap_descr = trap_table[trap_handle];

    MUv_semaphore (&gcm_semaphore);

    return trap_descr;
}


/*
** Name: gcm_add_work() - add msg work area to queue
**
** Description:
**	This routine adds a new message work area to the queue.
**
** Returns:
**	STATUS:
**	   OK if successful.
**	   FAIL if unsuccessful.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_add_work( GCM_MSG_WORK *work_area )
{

    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    QUinit( &work_area->q );
    QUinsert( &work_area->q, work_queue.q_prev );

    work_area->async_id = ++work_unique_id & 0xffff;

    MUv_semaphore (&gcm_semaphore);

    return OK;
}


/*
** Name: gcm_del_work() - remove msg work area from queue
**
** Description:
**	This routine removes a message work area from the queue.
**
** Returns:
**	STATUS:
**	   OK if successful.
**	   FAIL if unsuccessful.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static STATUS
gcm_del_work( GCM_MSG_WORK *work_area )
{
    STATUS	status = FAIL;
    QUEUE	*w;


    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    for( w = work_queue.q_next; w != &work_queue; w = w->q_next)
	if( w == &work_area->q )
	{
	    QUremove( &work_area->q );
	    status = OK;
	}

    MUv_semaphore (&gcm_semaphore);

    return status;
}


/*
** Name: gcm_find_work() - find next msg work area on queue
**
** Description:
**	This routine searches the message work area queue for
**	outstanding trap indication messages.
**
** Returns:
**	Pointer to next message work area.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
*/

static GCM_MSG_WORK *
gcm_find_work( VOID )
{
    GCM_MSG_WORK	*w = NULL;

    /* use semaphore to protect static storage from concurrent use */

    MUp_semaphore( &gcm_semaphore);

    if( work_queue.q_next != &work_queue )
	w = (GCM_MSG_WORK *)work_queue.q_next;

    MUv_semaphore (&gcm_semaphore);

    return w;
}


/*
** Name: gcm_set_delivery() - establish trap delivery thread
**
** Description:
**	This routine saves off the trap delivery service parms.
**
** Returns:
**	STATUS:
**	   OK if no trap delivery thread previously established.
**	   FAIL if trap delivery thread already established.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

STATUS
gcm_set_delivery( GCA_SVC_PARMS *svc_parms )
{
    STATUS		status = FAIL;

    /* 
    ** use semaphore to protect static storage from concurrent use 
    */
    MUp_semaphore( &gcm_semaphore );

    if ( ! deliver_parms )
    {
	status = OK;
	QUinit( &work_queue );
	deliver_parms = svc_parms;
    }

    MUv_semaphore( &gcm_semaphore );

    return( status );
}


/*
** Name: gcm_get_delivery() - retrieve trap delivery service parms
**
** Description:
**	This routine returns the established trap delivery service parms.
**
** Returns:
**	GCA_SVC_PARMS for trap delivery thread, if already established.
**
** History:
**	28-Jul-92 (brucek)
**	   Created.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

static GCA_SVC_PARMS *
gcm_get_delivery( VOID )
{
    GCA_SVC_PARMS	*svc_parms;

    /* 
    ** use semaphore to protect static storage from concurrent use 
    */
    MUp_semaphore( &gcm_semaphore );
    svc_parms = deliver_parms;
    MUv_semaphore( &gcm_semaphore );

    return( svc_parms );
}

