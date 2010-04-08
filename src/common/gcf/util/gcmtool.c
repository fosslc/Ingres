/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <cm.h>
#include <er.h>
#include <gc.h>
#include <id.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <sl.h>
#include <iicommon.h>
#include <gca.h>
#include <gcm.h>

FUNC_EXTERN	i4	gcm_put_str();
FUNC_EXTERN	i4	gcm_put_int();
FUNC_EXTERN	i4	gcm_put_long();
FUNC_EXTERN	i4	gcm_get_str();
FUNC_EXTERN	i4	gcm_get_int();
FUNC_EXTERN	i4	gcm_get_long();

/**
**
**  Name: gcmtool.c 
**
**  Description:
**	This is a simple front end which sends GCM messages directly to
**	a designated target.
**
**
**  History:
**	    
**      15-Jun-92 (brucek)
**	    Created.
**	11-Nov-1992 (daveb)
**	    decode error status a bit better.  Give help on prompts.
**	    default target server to be the current name server /iinmsvr
**      01-Dec-92 (brucek)
**	    Move element_count out of GCM_MSG_HDR and directly in front
**	    of array of GCM_VAR_BINDINGs.
**      14-jul-93 (ed)
**          unnested dbms.h
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	5-Oct-93 (seiwald)
**	    Defunct gca_cb_decompose, gca_decompose, gca_lsncnfrm now gone.
**	05-Jan-94 (brucek)
**	    Add support for multi-segment response messages (up to 4K
**	    total max length, with end_of_data flag OFF on all but last).
**	16-Nov-95 (gordy)
**	    Bumped protocol level.
**	11-Apr-96 (gordy)
**	    Cast parameters in IIGCa_call().
**	 1-Aug-96 (gordy)
**	    Added option to send GCA_RELEASE message to end GCM exchange.
**	21-May-97 (gordy)
**	    Clear parms before use.
**	27-May-97 (gordy)
**	    Increase size of buffers to allow for GCA message headers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

/*
**      mkming hints
NEEDLIBS =      GCFLIB COMPATLIB 

OWNER =         INGUSER

PROGRAM =       gcmtool
*/

static		STATUS	gcm_request();
static		STATUS	gcm_disassoc();
static		STATUS	gcm_release();
static		i4	gcm_message();
static		VOID	gcm_complete();

static		char	target[255];
static		char	response[400];
static		i4	message_type = GCM_GETNEXT;
static		char	work_buff[4120];
static		PTR	save_data_area;

static		char	msg_buff[4120];
static		PTR	msg_ptr;
static		i4	msg_len;

static		char	obj_class[16][255];
static		char	obj_instance[16][255];
static		char	obj_value[16][255];
static		i4	obj_perms[16];
static		i4	element_count;
static		i4	row_count = 1;

static		i4	trap_reason;
static		i4	trap_handle;
static		i4	mon_handle;
static		char	trap_address[256];

static		i4	err_fatal = 0;

static STATUS gcm_fastselect();
static i4 get_async();

main(argc,argv)
int argc;
char *argv[];
{
    STATUS		status;
    STATUS		call_status;
    GCA_IN_PARMS	in_parms;
    GCA_FO_PARMS	fo_parms;
    GCA_TE_PARMS	te_parms;
    CL_ERR_DESC		err_code;

    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &err_code);

    STcopy("/iinmsvr", target);
    while( argc-- > 1 )
    {
	++argv;
	if( STequal( *argv, "-xf") )
	    err_fatal++;
	else
	    STcopy( *argv, target);
    }

    /* Issue initialization call to ME */

    MEadvise(ME_INGRES_ALLOC);

    /*
    ** GCA initiate
    */

    in_parms.gca_normal_completion_exit = gcm_complete;
    in_parms.gca_expedited_completion_exit = gcm_complete;

    in_parms.gca_alloc_mgr = NULL;
    in_parms.gca_dealloc_mgr = NULL;
    in_parms.gca_p_semaphore = NULL;
    in_parms.gca_v_semaphore = NULL;

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
    in_parms.gca_api_version = GCA_API_LEVEL_4;
    in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

    call_status = IIGCa_call(   GCA_INITIATE,
                                (GCA_PARMLIST *)&in_parms,
                                (i4) GCA_SYNC_FLAG,
                                (PTR) 0,
                                (i4) -1,
                                &status);

    if (call_status != OK)
        return call_status;

    /*
    ** Invoke GCA_FORMAT
    */

    fo_parms.gca_buffer = work_buff;
    fo_parms.gca_b_length = sizeof work_buff;

    call_status = IIGCa_call(   GCA_FORMAT,
                                (GCA_PARMLIST *)&fo_parms,
                                (i4) GCA_SYNC_FLAG,
                                (PTR) 0,
                                (i4) -1,
                                &status);

    if (call_status != OK)
	return call_status;

    save_data_area = fo_parms.gca_data_area;
 
    SIprintf("\nGCMTool 0.1\n");

    for (;;)
    {
	bool	fast;

	/* ask for input */

	SIprintf("\nTarget name (eg: /iinmsvr) [%s]\n", target);
	SIgetrec(response, sizeof response, stdin);
        STtrmwhite(response);
	if( response[0] )
	    STcopy( response, target );

	SIprintf("Fastselect? ['Y'] ");
	SIgetrec(response, sizeof response, stdin);
	if( response[0] == 'n' || response[0] == 'N' )
	    fast = FALSE;
	else
	    fast = TRUE;

	/*
	** Invoke first asyncronous service 
	*/

	if( fast == TRUE )
	    gcm_fastselect(0);
	else
	    gcm_request(0);

	/*
	** Loop until all done with exchange
	*/

	GCexec();

	/* All done yet? */

	SIprintf("All done? [N] ");
	SIgetrec(response, sizeof response, stdin);
	if( response[0] == 'y' || response[0] == 'Y' )
	{
	    SIprintf("\nGCMTool Exiting...\n");
	    break;
	}
    }

    /*
    ** Invoke GCA_TERMINATE
    */

    call_status = IIGCa_call(   GCA_TERMINATE,
                                (GCA_PARMLIST *)&te_parms,
                                (i4) GCA_SYNC_FLAG,
                                (PTR) 0,
                                (i4) -1,
                                &status);

    if (call_status != OK)
	return call_status;

    PCexit( OK );
}

static	i4		parm_index = 0;
static	i4		parm_req[16];
static	GCA_PARMLIST	parms[16];

static i4
gcm_message( start )
PTR	start;
{
    i4		i;
    char	*q = start;

    SIprintf("GCM message type? (GETNEXT %x) [%x] ",
	     GCM_GETNEXT, message_type);
    SIgetrec(response, sizeof response, stdin);
    STtrmwhite(response);
    if( response[0] )
	CVahxl(response, &message_type);

    for( i = 0; i < 16; i++ )
    {
        SIprintf("Classsid (null OK for getnext) [%s]\n",
		 obj_class[i]);
        SIgetrec(response, sizeof response, stdin);
        STtrmwhite(response);
        if( response[0] )
        {
	    STcopy("", obj_class[i]);
            if( response[0] != '\\' )
	        STcopy(response, obj_class[i]);
        }

        SIprintf("Instance (null OK for getnext) [%s]\n",
		 obj_instance[i]);
        SIgetrec(response, sizeof response, stdin);
        STtrmwhite(response);
        if( response[0] )
        {
	    STcopy("", obj_instance[i]);
            if( response[0] != '\\' )
	        STcopy(response, obj_instance[i]);
        }

        SIprintf("Value (set only) [%s]\n", obj_value[i]);
        SIgetrec(response, sizeof response, stdin);
        STtrmwhite(response);
        if( response[0] )
        {
	    STcopy("", obj_value[i]);
            if( response[0] != '\\' )
	        STcopy(response, obj_value[i]);
        }

	SIprintf("More variables in request? ['N'] ");
	SIgetrec(response, sizeof response, stdin);
	if( response[0] == 'y' || response[0] == 'Y' )
	    continue;
	else
	    break;
    }

    element_count = ++i;

    if( message_type == GCM_GET ||
	message_type == GCM_GETNEXT )
    {
	SIprintf("How many rows do you want back? [%d] ", row_count);
	SIgetrec(response, sizeof response, stdin);
	STtrmwhite(response);
	if( response[0] )
	    CVan(response, &row_count);
    }

    if( message_type == GCM_SET_TRAP ||
	message_type == GCM_UNSET_TRAP ||
	message_type == GCM_TRAP_IND )
    {
	SIprintf("What's the reason for this trap? [%d] ", trap_reason);
	SIgetrec(response, sizeof response, stdin);
	STtrmwhite(response);
	if( response[0] )
	    CVan(response, &trap_reason);

	SIprintf("What's the handle on this trap? [%d] ", trap_handle);
	SIgetrec(response, sizeof response, stdin);
	STtrmwhite(response);
	if( response[0] )
	    CVan(response, &trap_handle);

	SIprintf("What's the handle on this mon? [%d] ", mon_handle);
	SIgetrec(response, sizeof response, stdin);
	STtrmwhite(response);
	if( response[0] )
	    CVan(response, &mon_handle);

	SIprintf("Where you want this trap delivered? [%s] ", trap_address);
	SIgetrec(response, sizeof response, stdin);
	STtrmwhite(response);
	if( response[0] )
	    STcopy(response, trap_address);
    }

    q += sizeof(i4);			/* bump past error_status */
    q += sizeof(i4);			/* bump past error_index */
    q += sizeof(i4);			/* bump past future[0] */
    q += sizeof(i4);			/* bump past future[1] */
    q += gcm_put_int(q, -1);	   	/* set client_perms */
    q += gcm_put_int(q, row_count);	/* set row_count */

    if( message_type == GCM_SET_TRAP ||
	message_type == GCM_UNSET_TRAP ||
	message_type == GCM_TRAP_IND )
    {
	q += gcm_put_int(q, trap_reason);	/* set trap_reason */
	q += gcm_put_int(q, trap_handle);   	/* set trap_handle */
	q += gcm_put_int(q, mon_handle);	/* set mon_handle */
	q += gcm_put_str(q, trap_address);   	/* set trap_address */
    }

    q += gcm_put_int(q, element_count);	/* set element_count */

    for( i = 0; i < element_count; i++ )
    {
        q += gcm_put_str(q, obj_class[i]);	/* set input classid */
        q += gcm_put_str(q, obj_instance[i]);	/* set input instance */
        q += gcm_put_str(q, obj_value[i]);	/* set value */
        q += gcm_put_int(q, 0);			/* set perms */
    }

    return q - start;
}

static i4
gcm_display( start )
PTR	start;
{
    i4 		error_status;
    i4  		error_index;
    char		*r = start;
    i4			i;

    r += gcm_get_long(r, &error_status);	/* get error_status */
    r += gcm_get_int(r, &error_index);		/* get error_index */

    r += sizeof(i4);			/* bump past future[0] */
    r += sizeof(i4);			/* bump past future[1] */
    r += sizeof(i4);	   		/* bump past client_perms */
    r += sizeof(i4);			/* bump past row_count */

    r += gcm_get_int(r, &element_count);	/* get element_count */

    STcopy("", obj_class[0]);
    STcopy("", obj_instance[0]);
    STcopy("", obj_value[0]);

    if( error_status )
    {
	SIprintf("Error Status received: %x\n%s\n",
		 error_status, ERget( error_status ) );
	SIprintf("Error Index: %d\n", error_index);
    }

    for( i = 0; i < element_count; i++ )
    {
	char	*c;
        i4	l;

	r += gcm_get_str(r, &c, &l);		/* get classid */
	MEcopy(c, l, obj_class[0]);
	obj_class[0][l] = '\0';
	r += gcm_get_str(r, &c, &l);		/* get instance */
	MEcopy(c, l, obj_instance[0]);
	obj_instance[0][l] = '\0';
	r += gcm_get_str(r, &c, &l);		/* get value */
	MEcopy(c, l, obj_value[0]);
	obj_value[0][l] = '\0';
	r += gcm_get_int(r, &obj_perms[0]);	/* get perms */

	SIprintf("%s |%s |%s |%x\n",
		 obj_class[0], obj_instance[0],
		 obj_value[0], obj_perms[0] );
    }

    if( error_status && err_fatal )
	PCexit( FAIL );
	
    return r - start;
}

static STATUS
gcm_fastselect(id)
i4	id;
{
	GCA_FS_PARMS	*fs_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async(id);

	if (id)
	    resume = GCA_RESUME;

	parm_req[async_id] = GCA_FASTSELECT;
	fs_parms = &parms[async_id].gca_fs_parms;
	if ( ! id )  MEfill( sizeof( *fs_parms ), 0, (PTR)fs_parms );
	fs_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
	fs_parms->gca_partner_name = target;
	fs_parms->gca_modifiers = GCA_RQ_GCM;
	fs_parms->gca_buffer = work_buff;
	fs_parms->gca_b_length = sizeof work_buff;

	if (!resume)
	{
	    fs_parms->gca_msg_length = gcm_message( save_data_area );
	    fs_parms->gca_message_type = message_type;
	}

        call_status = IIGCa_call(GCA_FASTSELECT,
                                 (GCA_PARMLIST *)fs_parms,
				 GCA_ASYNC_FLAG | resume,
                                 (PTR)(SCALARP) async_id,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	return OK;
}

static STATUS
gcm_request(id)
i4	id;
{
	GCA_RQ_PARMS	*rq_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async(id);

	if (id)
	    resume = GCA_RESUME;

	parm_req[async_id] = GCA_REQUEST;
	rq_parms = &parms[async_id].gca_rq_parms;
	if ( ! id )  MEfill( sizeof( *rq_parms ), 0, (PTR)rq_parms );
	rq_parms->gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
	rq_parms->gca_partner_name = target;
	rq_parms->gca_modifiers = GCA_RQ_GCM;

        call_status = IIGCa_call(GCA_REQUEST,
                                 (GCA_PARMLIST *)rq_parms,
				 GCA_ASYNC_FLAG | resume,
                                 (PTR)(SCALARP) async_id,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	return OK;
}

static STATUS
gcm_send(id, assoc_id)
i4	id;
i4	assoc_id;
{
	GCA_SD_PARMS	*sd_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async(id);

	if (id)
	    resume = GCA_RESUME;

	parm_req[async_id] = GCA_SEND;
	sd_parms = &parms[async_id].gca_sd_parms;
	if ( ! id )  MEfill( sizeof( *sd_parms ), 0, (PTR)sd_parms );
	sd_parms->gca_association_id = assoc_id;
	sd_parms->gca_buffer = work_buff;
	sd_parms->gca_end_of_data = TRUE;

	if (!resume)
	{
	    sd_parms->gca_msg_length = gcm_message( save_data_area );
	    sd_parms->gca_message_type = message_type;
	}

        call_status = IIGCa_call(GCA_SEND,
                                 (GCA_PARMLIST *)sd_parms,
				 GCA_ASYNC_FLAG | resume,
                                 (PTR)(SCALARP) async_id,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	msg_ptr = (PTR)&msg_buff;
	msg_len = 0;
	return OK;
}

static STATUS
gcm_receive(id, assoc_id)
i4	id;
i4	assoc_id;
{
	GCA_RV_PARMS	*rv_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async(id);

	if (id)
	    resume = GCA_RESUME;

	parm_req[async_id] = GCA_RECEIVE;
	rv_parms = &parms[async_id].gca_rv_parms;
	if ( ! id )  MEfill( sizeof( *rv_parms ), 0, (PTR)rv_parms );
	rv_parms->gca_association_id = assoc_id;
	rv_parms->gca_buffer = work_buff;
	rv_parms->gca_b_length = sizeof work_buff;
	rv_parms->gca_descriptor = NULL;

        call_status = IIGCa_call(GCA_RECEIVE,
                                 (GCA_PARMLIST *)rv_parms,
				 GCA_ASYNC_FLAG | resume,
                                 (PTR)(SCALARP) async_id,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	return OK;
}

static STATUS
gcm_interpret(assoc_id)
i4	assoc_id;
{
	GCA_IT_PARMS	it_parms;
	STATUS		status;
	STATUS		call_status;
	i4		resume = 0;

	it_parms.gca_buffer = work_buff;

        call_status = IIGCa_call(GCA_INTERPRET,
                                 (GCA_PARMLIST *)&it_parms,
				 GCA_SYNC_FLAG,
                                 (PTR) 0,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	SIprintf("Message type received from INTERPRET was %x\n", 
		it_parms.gca_message_type);
	SIprintf("Message length received from INTERPRET was %d\n", 
		it_parms.gca_d_length);
	SIprintf("End_of_data received? [%s]\n", 
		it_parms.gca_end_of_data ? "Yes" : "No" );

	MEcopy( save_data_area, it_parms.gca_d_length, msg_ptr );

	msg_len += it_parms.gca_d_length;
	msg_ptr += it_parms.gca_d_length;

	if( !it_parms.gca_end_of_data )
	{
	    gcm_receive(0, assoc_id);
	    return call_status;
	}

	SIprintf("Total length of data received was %d bytes\n", 
		gcm_display( &msg_buff ) );

	SIprintf("Any more messages to exchange on this connection? [Y] ");
	SIgetrec(response, sizeof response, stdin);
	if( response[0] != 'n' && response[0] != 'N' )
	    gcm_send(0, assoc_id);
	else
	{
	    SIprintf("Send GCA_RELEASE message? [Y] ");
	    SIgetrec(response, sizeof response, stdin);
	    if( response[0] != 'n' && response[0] != 'N' )
		gcm_release(0, assoc_id);
	    else
		gcm_disassoc(0, assoc_id);
	}

	return call_status;
}

static STATUS
gcm_release(id, assoc_id)
i4	id;
i4	assoc_id;
{
	GCA_SD_PARMS	*sd_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async( id );
	if ( id )  resume = GCA_RESUME;

	parm_req[ async_id ] = GCA_SEND;
	sd_parms = &parms[ async_id ].gca_sd_parms;
	if ( ! id )  MEfill( sizeof( *sd_parms ), 0, (PTR)sd_parms );
	sd_parms->gca_association_id = assoc_id;
	sd_parms->gca_buffer = work_buff;
	sd_parms->gca_end_of_data = TRUE;

	if ( ! resume )
	{
	    GCA_ER_DATA *release = (GCA_ER_DATA *)save_data_area;
	    release->gca_l_e_element = 0;
	    sd_parms->gca_msg_length = sizeof( release->gca_l_e_element );
	    sd_parms->gca_message_type = GCA_RELEASE;
	}

        call_status = IIGCa_call( GCA_SEND,
                                  (GCA_PARMLIST *)sd_parms,
				  GCA_ASYNC_FLAG | resume,
                                  (PTR)(SCALARP) async_id,
                                  (i4)-1,
                                  &status );

	return( call_status );
}

static STATUS
gcm_disassoc(id, assoc_id)
i4	id;
i4	assoc_id;
{
	GCA_DA_PARMS	*da_parms;
	STATUS		status;
	STATUS		call_status;
	i4		async_id;
	i4		resume = 0;

	async_id = get_async(id);

	if (id)
	    resume = GCA_RESUME;

	parm_req[async_id] = GCA_DISASSOC;
	da_parms = &parms[async_id].gca_da_parms;
	if ( ! id )  MEfill( sizeof( *da_parms ), 0, (PTR)da_parms );
	da_parms->gca_association_id = assoc_id;

        call_status = IIGCa_call(GCA_DISASSOC,
                                 (GCA_PARMLIST *)da_parms,
				 GCA_ASYNC_FLAG | resume,
                                 (PTR)(SCALARP) async_id,
                                 (i4) -1,
                                 &status);

   	if (call_status != OK)
	    return call_status;

	return OK;
}


static VOID
gcm_complete(async_id)
i4		async_id;
{
	GCA_FS_PARMS	*fs_parms;
	GCA_RQ_PARMS	*rq_parms;
	GCA_SD_PARMS	*sd_parms;
	GCA_RV_PARMS	*rv_parms;
	GCA_DA_PARMS	*da_parms;
	STATUS		status;
	STATUS		call_status;

	switch (parm_req[async_id])
	{
	case GCA_FASTSELECT:

	    fs_parms = &parms[async_id].gca_fs_parms;

	    if (fs_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
	        gcm_fastselect(async_id);
	        return;
	    }

	    if (fs_parms->gca_status != OK)
	    {
		if (fs_parms->gca_status != E_GC0032_NO_PEER)
		{
		    SIprintf("Fastselect failed; status = %x\n%s\n", 
			     fs_parms->gca_status,
			     ERget( fs_parms->gca_status ));
	    	    GCshut();
		}
		return;
	    }

	    SIprintf("Message type received from FASTSELECT was %x\n",
		fs_parms->gca_message_type);
	    SIprintf("Message length received from FASTSELECT was %d\n", 
		fs_parms->gca_msg_length);
	    SIprintf("Length of data actually present was %d bytes\n", 
		gcm_display( save_data_area ) );

	    GCshut();
	    break;

	case GCA_REQUEST:

	    rq_parms = &parms[async_id].gca_rq_parms;

	    if (rq_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
	        gcm_request(async_id);
	        return;
	    }

	    if (rq_parms->gca_status != OK)
	    {
		if (rq_parms->gca_status != E_GC0032_NO_PEER)
		{
		    SIprintf("Connect failed; status = %x\n%s\n", 
			     rq_parms->gca_status,
			     ERget(rq_parms->gca_status) );
		    GCshut();
		}
		return;
	    }

	    gcm_send(0, rq_parms->gca_assoc_id);
	    break;

	case GCA_SEND:

	    sd_parms = &parms[async_id].gca_sd_parms;

	    if (sd_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
	        gcm_send(async_id, sd_parms->gca_association_id);
	        return;
	    }

	    if (sd_parms->gca_status != OK)
		SIprintf("Send failed; status = %x\n%s\n",
				sd_parms->gca_status,
				ERget( sd_parms->gca_status ) );

	    if ( sd_parms->gca_message_type == GCA_RELEASE )
		gcm_disassoc(0, sd_parms->gca_association_id);
	    else
		gcm_receive(0, sd_parms->gca_association_id);
	    break;

	case GCA_RECEIVE:

	    rv_parms = &parms[async_id].gca_rv_parms;

	    if (rv_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
	        gcm_receive(async_id, rv_parms->gca_association_id);
	        return;
	    }

	    if (rv_parms->gca_status != OK)
		SIprintf("receive failed; status = %x\n%s\n",
				rv_parms->gca_status,
				ERget( rv_parms->gca_status) );

	    gcm_interpret(rv_parms->gca_association_id);
	    break;

	case GCA_DISASSOC:

	    da_parms = &parms[async_id].gca_da_parms;

	    if (da_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
	        gcm_disassoc(async_id, da_parms->gca_association_id);
	        return;
	    }

	    if (da_parms->gca_status != OK)
		SIprintf("Disassoc failed; status = %x\n%s\n",
			 da_parms->gca_status,
			 ERget( da_parms->gca_status ) );

	    GCshut();
	    break;

	default:
	    break;
	}

	parm_req[async_id] = 0;
	return;
}


static i4
get_async(id)
i4 id;
{
        if (id)
            return id;

        if (++parm_index > 15)
            parm_index = 1;

        return parm_index;
}


static void
uncalled( void )
{
    MO_dumpmem( 0, 0 );
}
    
