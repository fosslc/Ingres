/*
** Copyright (c) 2004, 2005 Ingres Corporation
*/

/*
** Name: iffgca.c
**
** Description:
**      Module provides communications functionality.
**
** History:
**      SIR 111718
**      23-Feb-2004 (fanra01)
**          Created based on iibrowse.sc.
**      18-Mar-2004 (fanra01)
**          Made gca and gcm functions static and renamed to resolve
**          conflicts.
**          Add instance to the requested instance data.
**          Enclosed strings in the required ERx macro.
**      08-Apr-2004 (fanra01)
**          Ensure that memory allocated for storing values is freed.
**          Update trace messages to force message on errors.
**          Add GCM functions for reserved credential requests.
**          Differentiate between addresses to registry name servers and
**          instance name servers.
**          Removed protocol information from instance information as it
**          cannot be retrieved using reserved credentials.
**      08-Jul-2004 (noifr01)
**          updated the row count when building the GCM request, to gcm_count
**          (i.e. the smallest multiple of gcm_count) rather than 0 (no limit)
**      20-Sep-2004 (hweho01)
**          Changed the connection string, so it can connect the slave
**          name server directly.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
**      01-Jun-2005 (fanra01)
**          SIR 114614
**          Add optional system path to instance information.
**      01-Aug-2005 (fanra01)
**          Bug 114967
**          Allow multiple initializations of the GCA layer and trace the fact
**          initialization has occurred multiple times.
**          Allow multiple terminations from the GCA layer and trace the fact
**          multiple terminations have occurred.
**	1-Dec-2010 (kschendel)
**	    Compiler warning fixes.
*/
# include <compat.h>
# include <gl.h>
# include <me.h>
# include <qu.h>
# include <si.h>
# include <st.h>
# include <tr.h>

# include <er.h>
# include <sp.h>
# include <mo.h>
# include <sl.h>
# include <iicommon.h>
# include <gca.h>
# include <gcn.h>
# include <gcm.h>
# include <gcu.h>

# include "iff.h"
# include "iffint.h"
# include "iffgca.h"

static char tgtspec[] = { ERx("@%s::/iinmsvr") };
static char tgtnmsrv[]= { ERx("@%s,tcp_ip,%s") };

static i4   msg_max = 0;

/*
** Standard non-dbms server classes
** List excludes INGRES and named database servers.
*/
static  char*   srvclass[] =
{
    ERx("IINMSVR"),
    ERx("NMSVR"),
    ERx("IUSVR"),
    ERx("STAR"),
    ERx("COMSVR"),
    ERx("JDBC"),
    ERx("DASVR"),
    ERx("ICESVR"),
    ERx("RMCMD"),
    NULL
};

/*
** Name Server GCM information for registered installations.
*/
static	GCM_INFO	inst_info[] =
{
    { GCN_MIB_SERVER_CLASS, {EOS} },
    { GCN_MIB_SERVER_ADDRESS, {EOS} },
    { GCN_MIB_SERVER_OBJECT, {EOS} }
};

/*
** Name Server GCM information for registered servers.
*/
static	GCM_INFO	svr_info[] = 
{
    { GCN_MIB_SERVER_CLASS      , {EOS} },
    { GCN_MIB_SERVER_ADDRESS    , {EOS} },
    { GCN_MIB_SERVER_OBJECT     , {EOS} }
};

/*
** Communications Server GCM information for protocols.
*/
static	GCM_INFO	net_info[] =
{
    { GCC_MIB_PROTOCOL          , {EOS} },
    { GCC_MIB_PROTO_PORT        , {EOS} }
};

/*
** Version and instance information for instance
*/
static  GCM_INFO     gv_info[] = 
{
    { ERx("exp.clf.gv.version")      , {EOS} },
    { ERx("exp.clf.gv.env")          , {EOS} },
    { ERx("exp.clf.gv.majorvers")    , {EOS} },
    { ERx("exp.clf.gv.minorvers")    , {EOS} },
    { ERx("exp.clf.gv.genlevel")     , {EOS} },
    { ERx("exp.clf.gv.bytetype")     , {EOS} },
    { ERx("exp.clf.gv.hw")           , {EOS} },
    { ERx("exp.clf.gv.os")           , {EOS} },
    { ERx("exp.clf.gv.patchlvl")     , {EOS} },
    { ERx("exp.clf.gv.instance")     , {EOS} },
    { ERx("exp.clf.gv.tcpport")      , {EOS} },
    { ERx("exp.clf.gv.sql92_conf")   , {EOS} },
    { ERx("exp.clf.gv.language")     , {EOS} },
    { ERx("exp.clf.gv.charset")      , {EOS} },
    { ERx("exp.clf.gv.system")       , {EOS} }, 
    { ERx("exp.clf.gv.bldlevel")     , {EOS} }
};

static GCM_INFO     cnf_info[] = 
{
        
    { ERx("exp.clf.gv.cnf_index")    , {EOS} },
    { ERx("exp.clf.gv.cnf_name")     , {EOS} },
    { ERx("exp.clf.gv.cnf_value")    , {EOS} }

};

/*
** Name: gca_request
**
** Description:
**	Issues GCA_REQUEST for target server and returns
**	association ID.  Uses username and password in
**	global variables if available.
**
** Input:
**	target		Target server.
**
** Output:
**	assoc_id	Association ID.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
gca_request( char *target, char* usr, char* pwd, i4 *assoc_id )
{
    GCA_RQ_PARMS	rq_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rq_parms ), 0, (PTR)&rq_parms );
    rq_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    rq_parms.gca_partner_name = target;

    if ( *usr  &&  *pwd )
    {
	rq_parms.gca_modifiers |= GCA_RQ_REMPW;
	rq_parms.gca_rem_username = usr;
	rq_parms.gca_rem_password = pwd;
    }

    call_status = IIGCa_call( GCA_REQUEST, (GCA_PARMLIST *)&rq_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    *assoc_id = rq_parms.gca_assoc_id;

    if ( call_status == OK  &&  rq_parms.gca_status != OK )
	status = rq_parms.gca_status, call_status = FAIL;

    return( status );
}

/*
** Name: iff_gca_fastselect
**
** Description:
**	Issues GCA_FASTSELECT for target server, sending
**	GCM message provided and returning GCM response.
**	Uses username and password in global variables if 
**	available.
**
** Input:
**	target		Target server.
**	msg_type	GCM message type.
**	msg_len		Length of GCM message.
**	msg_buff	GCM message.
**	buff_max	Length of buffer for response.
**
** Output:
**	msg_type	GCM response message type.
**	msg_len		Length of GCM response.
**	msg_buff	GCM response.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
iff_gca_fastselect( char *target, char* usr, char* pwd, i4 *msg_type, 
		i4 *msg_len, char *msg_buff, i4 buff_max )
{
    GCA_FS_PARMS	fs_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( fs_parms ), 0, (PTR)&fs_parms );
    fs_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    fs_parms.gca_modifiers = GCA_RQ_GCM;
    fs_parms.gca_partner_name = target;
    fs_parms.gca_buffer = msg_buff;
    fs_parms.gca_b_length = buff_max;
    fs_parms.gca_message_type = *msg_type;
    fs_parms.gca_msg_length = *msg_len;

    if ( *usr  &&  *pwd )
    {
	fs_parms.gca_modifiers |= GCA_RQ_REMPW;
	fs_parms.gca_rem_username = usr;
	fs_parms.gca_rem_password = pwd;
    }

    call_status = IIGCa_call( GCA_FASTSELECT, (GCA_PARMLIST *)&fs_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  fs_parms.gca_status != OK )
	status = fs_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
    {
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
        ERx("GCA_FASTSELECT failed: 0x%x\n"), status );
	call_status = status;
    }
    else
    {
	*msg_type = fs_parms.gca_message_type;
	*msg_len = fs_parms.gca_msg_length;
    }

    return( call_status );
}
/*
** Name: gca_receive
**
** Description:
**	Issues a GCA_RECEIVE request and returns GCA message.
**
** Input;
**	assoc_id	Association ID.
**	msg_len		Length of message buffer.
**	msg_buff	Message buffer.
**
** Output:
**	msg_type	GCA message type.
**	msg_len		Length of GCA message.
**	msg_buff	GCA Message.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
gca_receive( i4 assoc_id, i4 *msg_type, i4 *msg_len, char *msg_buff )
{
    GCA_RV_PARMS	rv_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( rv_parms ), 0, (PTR)&rv_parms );
    rv_parms.gca_assoc_id = assoc_id;
    rv_parms.gca_b_length = *msg_len;
    rv_parms.gca_buffer = msg_buff;

    call_status = IIGCa_call( GCA_RECEIVE, (GCA_PARMLIST *)&rv_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  rv_parms.gca_status != OK )
	status = rv_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("GCA_RECEIVE failed: 0x%x\n"),
        status );
    else  if ( ! rv_parms.gca_end_of_data )
    {
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("GCA_RECEIVE buffer overflow\n") );
	call_status = FAIL;
    }
    else
    {
	*msg_type = rv_parms.gca_message_type;
	*msg_len = rv_parms.gca_d_length;
    }

    return( call_status );
}


/*
** Name: gca_send
**
** Description:
**	Issues GCA_SEND request to send GCA message.
**
** Input:
**	assoc_id	Association ID.
**	msg_type	GCA message type.
**	msg_len		Length of GCA message.
**	msg_buff	GCA message.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
gca_send( i4 assoc_id, i4 msg_type, i4 msg_len, char *msg_buff )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_association_id = assoc_id;
    sd_parms.gca_message_type = msg_type;
    sd_parms.gca_msg_length = msg_len;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
	status = sd_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("GCA_SEND failed: 0x%x\n"),
        status );

    return( call_status );
}


/*
** Name: gca_release
**
** Description:
**	Issues GCA_SEND request to send GCA_RELEASE message.
**
** Input:
**	assoc_id	Association ID.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
gca_release( i4 assoc_id, char* msg_buff )
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status;
    STATUS		call_status;
    char		*ptr = msg_buff;

    ptr += gcu_put_int( ptr, 0 );

    MEfill( sizeof( sd_parms ), 0, (PTR)&sd_parms );
    sd_parms.gca_assoc_id = assoc_id;
    sd_parms.gca_message_type = GCA_RELEASE;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_msg_length = ptr - msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  sd_parms.gca_status != OK )
	status = sd_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
        ERx("GCA_SEND(GCA_RELEASE) failed: 0x%x\n"), status );

    return( call_status );
}


/*
** Name: gca_disassoc
**
** Description:
**	Issues GCA_DISASSOC request.
**
** Input:
**	assoc_id	Association ID.
**
** Ouput:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static STATUS
gca_disassoc( i4 assoc_id )
{
    GCA_DA_PARMS	da_parms;
    STATUS		status;
    STATUS		call_status;

    MEfill( sizeof( da_parms ), 0, (PTR)&da_parms );
    da_parms.gca_association_id = assoc_id;

    call_status = IIGCa_call( GCA_DISASSOC, (GCA_PARMLIST *)&da_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  da_parms.gca_status != OK )
	status = da_parms.gca_status, call_status = FAIL;

    if ( call_status != OK )
	iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("GCA_DISASSOC failed: 0x%x\n"),
        status );

    return( call_status );
}

/*
** Name: iff_gcm_load
**
** Description:
**	Builds a GCM request to retrieve the GCM information
**	represented by a GCM_INFO array and issues a FASTSELECT
**	request for the target server.  The GCM response data
**	is added to the GCM data queue.  The GCM data queue
**	may be cleared by calling gcm_unload().
**
**	Returns FAIL if there is additional data to be retrieved
**	and places the instance to be used for the next request
**	in beg_inst (which should be initialized to a zero length
**	string for the first call).
**
** Input:
**	target		Target server.
**	gcm_count	Number of entries in gcm_info array.
**	gcm_info	GCM info array.
**	beg_inst	Beginning instance, init to zero length string.
**	msg_len		Length of msg_buff.
**	msg_buff	Buffer for GCM messages.
**	gcm_data	GCM data queue.
**
** Output:
**	beg_inst	Instance for next call if return FAIL.
**	gcm_data	Entries added.
**
** Returns:
**	STATUS		OK, FAIL or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
**      Builds a target address from the II_CONNECT structure.
**   16-Apr-2004 (fanra01)
**      Add test for a null addr to specify connection to registry name
**      server.
*/

static STATUS
iff_gcm_load( ING_CONNECT* tgt, char* addr, i4 gcm_count, GCM_INFO *gcm_info,
    char *beg_inst, i4 msg_len, char *msg_buff, i4* fields, QUEUE *gcm_data )
{
    STATUS	status;
    char	*ptr = msg_buff;
    i4		i, msg_type, len;
    i4		data_count = 0;
    i4      tgtlen;
    char    *target;

    tgtlen = STlength( tgt->hostname );
    tgtlen += (addr == NULL) ? STlength( tgtspec ) :
        STlength( tgtnmsrv ) + STlength( addr );
    if ((target = MEreqmem( 0, tgtlen, TRUE, &status)) != NULL)
    {
        if (addr == NULL)
        {
            /*
            ** Build target specification for Registry Name Server:
            ** @::/iinmsvr for local, @host::/iinmsvr for remote.
            */
            STprintf( target, tgtspec, tgt->hostname );
        }
        else
        {
            /*
            ** Build target specification for Name Server:
            ** @::/@addr for local, @host::/@addr for remote.
            ** We don't allow @::/iinmsvr for local as there is the
            ** possibility that the master name server is not the
            ** intended target.
            */
            STprintf( target, tgtnmsrv, tgt->hostname, addr );
        }
    }
    else
    {
        return(status);
    }

    /*
    ** Build GCM request.
    */
    msg_type = GCM_GETNEXT;
    ptr += gcu_put_int( ptr, 0 );		/* error_status */
    ptr += gcu_put_int( ptr, 0 );		/* error_index */
    ptr += gcu_put_int( ptr, 0 );		/* future[2] */
    ptr += gcu_put_int( ptr, 0 );
    ptr += gcu_put_int( ptr, -1 );		/* client_perms: all */
    ptr += gcu_put_int( ptr, gcm_count );	/* row count: multiple of gcm_count */
    ptr += gcu_put_int( ptr, gcm_count );

    for( i = 0; i < gcm_count; i++ )
    {
	ptr += gcu_put_str( ptr, gcm_info[i].classid );
	ptr += gcu_put_str( ptr, beg_inst );
	ptr += gcu_put_str( ptr, ERx("") );
	ptr += gcu_put_int( ptr, 0 );
    }

    len = ptr - msg_buff;

    /*
    ** Connect to target, send GCM request and
    ** receive GCM response.
    */
    status = iff_gca_fastselect( target, tgt->user, tgt->password, &msg_type, &len, msg_buff, msg_len );

    if ( status == OK )
    {
	char	classid[ GCN_OBJ_MAX_LEN ]; 
	char	instance[ GCN_OBJ_MAX_LEN ];
	char	cur_inst[ GCN_OBJ_MAX_LEN ];
	i4	len, err_index, junk, elements;

	/*
	** Extract GCM info.
	*/
	ptr = msg_buff;
	ptr += gcu_get_int( ptr, &status );
	ptr += gcu_get_int( ptr, &err_index );
	ptr += gcu_get_int( ptr, &junk );		/* skip future[2] */
	ptr += gcu_get_int( ptr, &junk );
	ptr += gcu_get_int( ptr, &junk );		/* skip client_perms */
	ptr += gcu_get_int( ptr, &junk );		/* skip row_count */
	ptr += gcu_get_int( ptr, &elements );

	if ( status )  
	{
	    elements = max( 0, err_index );
	    /*
	    ** Ignore possible end-of-elements status.
	    */
	    if ( status == MO_NO_NEXT )
            status = OK;
	    else
        {
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("GCM error: 0x%x\n"),
                status );
            if (status == MO_NO_CLASSID)
            {
                *fields = elements;
                status = OK;
                gcm_count = elements;
            }
        }
	}

	for(;;)
	{
	    for( i = 0; i < gcm_count; i++ )
	    {
		if ( ! elements-- )
		{
		    /*
		    ** Completed processing of current GCM response.
		    ** Return FAIL to indicate need for additional
		    ** request beginning with last valid instance
		    ** (make sure we actually loaded something from
		    ** the current message to avoid an infinite loop).
		    */
		    if ( data_count )  status = FAIL;
		    break;
		}
		else
		{
		    /*
		    ** Extract classid, instance, and value.
		    */
		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, classid );
		    classid[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, instance );
		    instance[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &len );
		    MEcopy( ptr, len, gcm_info[i].value );
		    gcm_info[i].value[ len ] = EOS;
		    ptr += len;

		    ptr += gcu_get_int( ptr, &junk );	/* skip perms */
		}

		/*
		** A mis-match in classid indicates the end of
		** the entries we requested.
		*/
		if ( STcompare( classid, gcm_info[i].classid ) )
		    break;

		/*
		** Instance value should be same for each 
		** classid in group.  Save for first classid,
		** check all others.
		*/
		if ( ! i )
		    STcopy( instance, cur_inst );	/* Save new instance */
		else  if ( STcompare( instance, cur_inst ) )
		{
		    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Found invalid GCM instance: '%s' != '%s'\n"),
			    instance, cur_inst );
		    break;
		}
	    }

	    if ( i < gcm_count )
		break;		/* Completed processing of GCM response info */
	    else
	    {
		GCM_DATA	*data;

		/*
		** Save instance in case we need to issue
		** another GCM FASTSELECT request for more
		** info.
		*/
		STcopy( cur_inst, beg_inst );

		/*
		** Add entry to GCM data queue.
		*/
		data = (GCM_DATA *)MEreqmem( 0, sizeof( GCM_DATA ) + 
						sizeof( char * ) * 
						(gcm_count - 1), 
					     FALSE, NULL );
		if ( ! data )
		{
		    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Memory allocation failed\n") );
		    break;
		}

		data_count++;
		QUinsert( &data->q, gcm_data->q_prev );

		for( i = 0; i < gcm_count; i++ )
		{
		    data->data[i] = STalloc( gcm_info[i].value );

		    if ( ! data->data[i] )
		    {
			i4 j;

			iff_trace( IFF_TRC_ERR,
                IFF_TRC_ERR, ERx("Memory allocation failed\n") );
			QUremove( &data->q );
			for( j = 0; j < i; j++ )  MEfree( (PTR)data->data[j] );
			MEfree( (PTR)data );
			break;
		    }
		}
	    }
	}
    }
    MEfree( target );
    return( status );
}

/*
** Name: gcm_unload
**
** Description:
**	Removes entries from a GCM data queue as built by
**	iff_gcm_load(), and frees associated resources.
**
** Input:
**	count		Number of GCM classids for each instance.
**	gcm_data	GCM data queue.
**
** Output:
**	gcm_data	Entries removed.
**
** Returns:
**	VOID.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

static VOID
gcm_unload( i4 count, QUEUE *gcm_data )
{
    GCM_DATA	*data, *next;
    i4		i;

    for(
	 data = (GCM_DATA *)gcm_data->q_next;
	 (QUEUE *)data != gcm_data;
	 data = next
       )
    {
	next = (GCM_DATA *)data->q.q_next;

	for( i = 0; i < count; i++ )  MEfree( (PTR)data->data[i] );
	QUremove( &data->q );
	MEfree( (PTR)data );
    }

    return;
}

/*
** Name: iff_display
**
** Description:
**      Display the instance obtained instance data
**
** Inputs:
**      ins_ctx     instance context structure
**      server      list of database servers
**      proto       list of protocols
**      instance    instance data
**      config      config data
**
** Outputs:
** 
**      None
**
** Returns:
** 
**      None
**
** History:
**      02-Mar-2004 (fanra01)
**          Created.
*/
VOID
iff_display( ING_INS_CTX* ins_ctx, ING_INS_DATA* server, ING_INS_DATA* proto,
    ING_INS_DATA* instance, ING_INS_DATA* config )
{
    GCM_DATA	*data;
    i4		i;
    i4      count;

    if (server != NULL)
    {
        for(
         data = (GCM_DATA *)server->list.q_next;
         (QUEUE *)data != &server->list;
         data = (GCM_DATA *)data->q.q_next
           )
        {
            for( i = 0, count = server->fields; i < count; i++ )
                if ( i )
                {
                    if (IFF_TRC_CALL < ins_ctx->trclvl)
                        TRdisplay( ERx("\t%s"), data->data[i] );
                }
                else
                    iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, ERx("%%8* %s"),
                        data->data[i] );

            if (IFF_TRC_CALL < ins_ctx->trclvl)
                TRdisplay( ERx("\n") );
        }
    }
    if (proto != NULL)
    {
        for(
         data = (GCM_DATA *)proto->list.q_next; 
         (QUEUE *)data != &proto->list;
         data = (GCM_DATA *)data->q.q_next
           )
        {
            for( i = 0, count = proto->fields; i < count; i++ )
                if ( i )
                {
                    if (IFF_TRC_CALL < ins_ctx->trclvl)
                        TRdisplay( ERx("\t%s"), data->data[i] );
                }
                else
                    iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, ERx("%%8* %s"),
                        data->data[i] );

            if (IFF_TRC_CALL < ins_ctx->trclvl)
                TRdisplay( ERx("\n") );
        }
    }
    if (instance != NULL)
    {
        for(
         data = (GCM_DATA *)instance->list.q_next;
         (QUEUE *)data != &instance->list;
         data = (GCM_DATA *)data->q.q_next
           )
        {
            for( i = 0, count = instance->fields; i < count; i++ )
                if ( i )
                {
                    if (IFF_TRC_CALL < ins_ctx->trclvl)
                        TRdisplay( ERx("\t%s"), data->data[i] );
                }
                else
                    iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, ERx("%%8* %s"),
                        data->data[i] );

            if (IFF_TRC_CALL < ins_ctx->trclvl)
                TRdisplay( ERx("\n") );
        }
    }
    if (config != NULL)
    {
        for(
         data = (GCM_DATA *)config->list.q_next;
         (QUEUE *)data != &config->list;
         data = (GCM_DATA *)data->q.q_next
           )
        {
            for( i = 0, count = config->fields; i < count; i++ )
                if ( i )
                {
                    if (IFF_TRC_CALL < ins_ctx->trclvl)
                        TRdisplay( ERx("\t%s"), data->data[i] );
                }
                else
                    iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, ERx("%%8* %s"),
                        data->data[i] );

            if (IFF_TRC_CALL < ins_ctx->trclvl)
                TRdisplay( ERx("\n") );
        }
    }
    return;
}

/*
** Name: iff_load_install
**
** Description:
**	Connects to installation registry master Name Server
**	on target host.  If target host name is zero length
**	string, connection is made to master Name Server on
**	local host.
**
**	Uses GCN_NS_OPER request to obtain installation info
**	from master Name Server and adds INST_DATA entries
**	to the install queue.  The install queue may be
**	cleared by calling ibr_unload_install().
**
**	No entries are added to the queue if an error code
**	is returned.
**
** Input:
**	host		Target host name, zero length string for local.
**	install		Queue to receive installation information.
**
** Output:
**	install		Queue entries.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   25-Feb-2004 (fanra01)
**      Modified version of the original ibr_load_install.
**      Modified to use the provided message area instead of a global one.
**      Replaced SIprintf with trace calls.
*/

STATUS
iff_load_install( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt,
    ING_INS_DATA* instance )
{
    STATUS	status;
    char	*ptr = ins_ctx->msg_buff;
    char    *target;
    i4		assoc_id, msg_type;
    i4		msg_len = msg_max;
    i4      tgtlen;
    char*   usr;
    char*   pwd;

    tgtlen = STlength( tgt->hostname ) + STlength( tgtspec );
    if ((target = MEreqmem( 0, tgtlen, TRUE, &status)) != NULL)
    {
        usr = tgt->user;
        pwd = tgt->password;
        /*
        ** If the hostname is set and the user has not been specified
        ** use reserved user name otherwise use effective credentials.
        */
        if (*tgt->hostname)
        {
            if (*usr == '\0')
            {
                STcopy( IFF_RESERVED_USR, usr );
            }
            if (*pwd == '\0')
            {
                STcopy( IFF_RESERVED_PWD, pwd );
            }
        }
        else
        {
            *usr = '\0';
            *pwd = '\0';
        }
        STprintf( target, tgtspec, tgt->hostname );
    }
    else
    {
        return( status );
    }
    /*
    ** Connect to master Name Server.
    */
    if ( (status = gca_request( target, usr, pwd, &assoc_id )) == OK )
    {
	/*
	** Build and send GCN_NS_OPER request.
	*/
	ptr += gcu_put_int( ptr, GCN_DEF_FLAG );
	ptr += gcu_put_int( ptr, GCN_RET );
	ptr += gcu_put_str( ptr, ERx("") );
	ptr += gcu_put_int( ptr, 1 );
	ptr += gcu_put_str( ptr, ERx("NMSVR") );
	ptr += gcu_put_str( ptr, ERx("") );
	ptr += gcu_put_str( ptr, ERx("") );
	ptr += gcu_put_str( ptr, ERx("") );

	status = gca_send( assoc_id, GCN_NS_OPER, ptr - ins_ctx->msg_buff,
       ins_ctx->msg_buff );

	/*
	** Receive response.
	*/
	if ( status == OK )
	    status = gca_receive( assoc_id, &msg_type, &msg_len, ins_ctx->msg_buff );

	if ( status == OK )
	    switch( msg_type )
	    {
		case GCA_ERROR :
		{
		    i4	ele_cnt, prm_cnt, val;

		    ptr = ins_ctx->msg_buff;
		    ptr += gcu_get_int( ptr, &ele_cnt );

		    while( ele_cnt-- )
		    {
			ptr += gcu_get_int( ptr, &status );
			ptr += gcu_get_int( ptr, &val ); /* skip id_server */
			ptr += gcu_get_int( ptr, &val ); /* skip server_type */
			ptr += gcu_get_int( ptr, &val ); /* skip severity */
			ptr += gcu_get_int( ptr, &val ); /* skip local_error */
			ptr += gcu_get_int( ptr, &prm_cnt );

			while( prm_cnt-- )
			{
			    ptr += gcu_get_int( ptr, &val );
			    ptr += gcu_get_int( ptr, &val );
			    ptr += gcu_get_int( ptr, &val );
			    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR, ERx("0x%x: %.*s\n"), 
                    status, val, ptr);
			    ptr += val;
			}
		    }
		}
		break;
		
		case GCN_RESULT :
		{
		    INST_DATA	*data;
		    char	*val;
		    i4		i, count;

		    ptr = ins_ctx->msg_buff;
		    ptr += gcu_get_int( ptr, &count );	/* skip gcn_op */
		    ptr += gcu_get_int( ptr, &count );

            instance->fields = count;
		    for( i = 0; i < count; i++ )
		    {
			/*
			** Add entry to install queue.
			*/
			data = (INST_DATA *)MEreqmem( 0, sizeof( *data ), 
						      FALSE, &status );
			if ( ! data )
			{
			    iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("Memory allocation failed %x\n"),
                    status );
			    break;
			}

			QUinsert( &data->q, instance->list.q_prev );

			/*
			** Installation ID is is in gcn_obj
			** and Name Server listen address is
			** in gcn_val.
			*/
			ptr += gcu_get_str( ptr, &val );	/* skip type */
			ptr += gcu_get_str( ptr, &val );	/* skip uid */
			ptr += gcu_get_str( ptr, &val );	
			STcopy( val, data->id );
			ptr += gcu_get_str( ptr, &val );
			STcopy( val, data->addr );
		    }
		}
		break;

		default :
            iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                ERx("Invalid GCA message type: %d\n"), 
                msg_type );
		    break;
	    }

	gca_release( assoc_id, ins_ctx->msg_buff );
    }

    gca_disassoc( assoc_id );

    MEfree( target );
    return( status );
}

/*
** Name: iff_load_instances
**
** Description:
**	Connects to installation registry master Name Server
**	on target host.  If target host name is zero length
**	string, connection is made to master Name Server on
**	local host.
**
**	Uses FASTSELECT request to obtain installation info
**	from master Name Server and adds INST_DATA entries
**	to the install queue.  The install queue may be
**	cleared by calling ibr_unload_install().
**
**	No entries are added to the queue if an error code
**	is returned.
**
** Input:
**	host		Target host name, zero length string for local.
**	install		Queue to receive installation information.
**
** Output:
**	install		Queue entries.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**      24-Apr-2004 (fanra01)
**	        Created based on iibrowse.
*/

STATUS
iff_load_instances( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt,
    ING_INS_DATA *instance )
{
    STATUS	    status = OK;
    char	    beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    QUEUE       inst;
    GCM_DATA*   data;
    
    /*
    ** Issue GCM FASTSELECT requests to Name Server
    ** until all data retrieved or error occurs.
    */
    QUinit( &inst );
    instance->fields = ARRAY_SIZE( inst_info );
    do
    {
        status = iff_gcm_load( tgt, NULL, instance->fields, inst_info,
            beg_inst, msg_max, ins_ctx->msg_buff, &instance->fields, &inst );
    } while( status == FAIL );

    if ( status != OK )
    {
        gcm_unload( instance->fields, &inst );
    }

    for( 
	 data = (GCM_DATA *)inst.q_next;
	 (QUEUE *)data != &inst;
	 data = (GCM_DATA *)data->q.q_next
       )
    {
	INST_DATA *id;

	/*
	** Installations are represented by their
	** registry Name Server registrations.  
	** Ignore other server registrations.
	*/
        if ( STcasecmp( data->data[0], "NMSVR" ) )  continue;

	/*
	** Add entry to install queue.
	*/
	id = (INST_DATA *)MEreqmem( 0, sizeof( *id ), FALSE, NULL );

	if ( ! id )
	{
	    SIprintf( "\nMemory allocation failed\n" );
	    break;
	}

	QUinsert( &id->q, instance->list.q_prev );
/*
	STcopy( data->data[ 1 ], id->addr );
*/
	STcopy( data->data[ 2 ], id->id );
        /* build the address for connection string  */
        STprintf( id->addr, ERx( "%s::/iinmsvr" ), id->id ); 
    }

    gcm_unload( ARRAY_SIZE( inst_info ), &inst );
    return( status );
}

/*
** Name: iff_unload_install
**
** Description:
**	Removes entries from the installation queue as built by
**	ibr_load_install(), and frees associated resources.
**
** Input:
**	install		Installation queue.
**
** Output:
**	install		Entries removed.
**
** Returns:
**	VOID
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   08-Mar-2004 (fanra01)
**      Based on original in iibrowse.
*/

VOID
iff_unload_install( ING_INS_DATA *install )
{
    INST_DATA	*data, *next;

    for( 
	 data = (INST_DATA *)install->list.q_next;
	 (QUEUE *)data != &install->list;
	 data = next
       )
    {
	next = (INST_DATA *)data->q.q_next;
	QUremove( &data->q );
	MEfree( (PTR)data );
    }

    return;
}

/*
** Name: iff_unload_values
**
** Description:
**      Frees a list of values built from GCM messages.
**
** Inputs:
**      values      List of items to be freed including data values.
**
** Outputs:
**      None
**      
** Returns:
**      None
**      
** History:
**      08-Mar-2004 (fanra01)
**          Created
**      08-Apr-2004 (fanra01)
**          Free memory allocated for value.
*/
VOID
iff_unload_values( ING_INS_DATA* value )
{
    GCM_DATA*   data;
    GCM_DATA*   next;
    i4          i;
    
    for( 
	 data = (GCM_DATA *)value->list.q_next;
	 (QUEUE *)data != &value->list;
	 data = next
       )
    {
        next = (GCM_DATA *)data->q.q_next;
        for( i = 0; i < value->fields; i+=1 )
        {
            MEfree( (PTR)data->data[i] );
        }
        QUremove( &data->q );
        MEfree( (PTR)data );
    }
    return;
}

/*
** Name: iff_load_server
**
** Description:
**	Connects to Name Server listening at a known address on
**	target host.  If target host name is zero length string, 
**	connection is made to Name Server on local host.
**
**	Uses GCM FASTSELECT request to obtain registered server
**	info (svr_info array) from Name Server and adds GCM_DATA 
**	entries to the server queue.  The server queue may be 
**	cleared by calling gcm_unload().
**
**	No entries are added to the queue if an error code
**	is returned.
**
**
** Input:
**	host		Target host name, zero length string for local.
**	addr		Name Server listen address.
**	server		Server info queue.
**
** Output:
**	server		Entries added.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Oct-98 (gordy)
**	    Created.
**   26-Feb-2004 (fanra01)
**      Modified to count and return the dbms server classes.
**   22-Apr-2004 (fanra01)
**      Removed protocol information gathered from comms server.
*/

STATUS
iff_dbms_server( ING_INS_CTX* ins_ctx, ING_CONNECT* tgt, char* addr,
    ING_INS_DATA* server, i4* dbms )
{
    STATUS	    status = OK;
    char	    beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    GCM_DATA*   data;
    GCM_DATA*   discard;
    char**      sclass;
    i4          count;
    i4          dcount;
    QUEUE       svrlst;
    
    /*
    ** Issue GCM FASTSELECT requests to Name Server
    ** until all data retrieved or error occurs.
    */
    server->fields = ARRAY_SIZE( svr_info );
    do
    {
        status = iff_gcm_load( tgt, addr, server->fields, svr_info,
            beg_inst, msg_max, ins_ctx->msg_buff, &server->fields,
            &server->list );
    } while( status == FAIL );

    if ( status != OK )
    {
        gcm_unload( server->fields, &server->list );
    }
    else
    {
        /*
        ** test the server classes
        */
        QUinit( &svrlst );
        for (dcount=0, data = (GCM_DATA *)server->list.q_next;
             (QUEUE *)data != &server->list;
            )
        {
            for (count=0, sclass=srvclass; *sclass != NULL;
                sclass+=1)
            {
                if (STcompare( *sclass, data->data[0] ) == 0)
                {
                    iff_trace( IFF_TRC_CALL, ins_ctx->trclvl, ERx("%%8* %s\t%s\n"),
                        data->data[0], data->data[1]);
                    /*
                    ** A standard server class has been found
                    ** remove it from the list and queue it
                    ** onto a discard list
                    */
                    count+=1;
                    discard = (GCM_DATA *)QUremove( (QUEUE *)data );
                    data = (GCM_DATA *)data->q.q_next;
                    if (discard != NULL)
                    {
                        QUinsert( &discard->q, svrlst.q_prev );
                    }
                    break;
                }
            }
            if (count == 0)
            {
                /*
                ** Found a relevant server class
                ** bump the count
                */
                data = (GCM_DATA *)data->q.q_next;
                dcount+=1;    
            }
        }
        if (svrlst.q_prev != NULL)
        {
            gcm_unload( ARRAY_SIZE( svr_info ), &svrlst );
        }
        if (dbms != NULL)
        {
            *dbms = dcount;
        }
    }
    return( status );
}

/*
** Name: iff_load_instance
**
** Description:
**      Requests the version information from the specfied instance.
**      
** Inputs:
**      ins_ctx     context information
**      tgt         target host address information
**      addr        instance address
**
** Outputs:
**      instance    list of version values for instance.
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**      
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
STATUS
iff_load_instance( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt, char *addr,
    ING_INS_DATA* instance )
{
    STATUS	    status = OK;
    char	    beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    
    /*
    ** Issue GCM FASTSELECT requests to Name Server
    ** until all data retrieved or error occurs.
    */
    instance->fields = ARRAY_SIZE( gv_info );
    do
    {
        status = iff_gcm_load( tgt, addr, instance->fields, gv_info,
            beg_inst, msg_max, ins_ctx->msg_buff, &instance->fields,
            &instance->list );
    } while( status == FAIL );

    if ( status != OK )
    {
        gcm_unload( instance->fields, &instance->list );
    }
    return(status);
}

/*
** Name: iff_load_config
**
** Description:
**      Requests all public configuration parameter values.
** 
** Inputs:
**      ins_ctx     context information
**      tgt         target host address information
**      addr        instance address
**
** Outputs:
**      config      list of configuration values.
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
STATUS
iff_load_config( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt, char *addr,
    ING_INS_DATA* config )
{
    STATUS	    status = OK;
    char	    beg_inst[ GCN_VAL_MAX_LEN ] = { '\0' };
    
    /*
    ** Issue GCM FASTSELECT requests to Name Server
    ** until all data retrieved or error occurs.
    */
    config->fields = ARRAY_SIZE( cnf_info );
    do
    {
        status = iff_gcm_load( tgt, addr, config->fields, cnf_info,
            beg_inst, msg_max, ins_ctx->msg_buff, &config->fields,
            &config->list );
    } while( status == FAIL );

    if ( status != OK )
    {
        gcm_unload( config->fields, &config->list );
    }
    return(status);
}

/*
** Name: iff_msg_max
**
** Description:
**      Returns the calulated maximum length of message.
**
** Inputs:
**      None.
**      
** Outputs:
**      None.
**      
** Returns:
**      msgmax      value for the maximum length of message.
**                  There is no error return.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
i4
iff_msg_max()
{
    return(msg_max);
}

/*
** Name: iff_gca_init
**
** Description:
**      Initialize the communications interface.
**      
** Inputs:
**      None.
**      
** Outputs:
**      msg_max     initialized to the maximum length of message.
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
STATUS
iff_gca_init()
{
    STATUS status = OK;
    STATUS call_status = OK;
    GCA_IN_PARMS	in_parms;
    
    /*
    ** Initialize communications.
    */
    MEfill( sizeof(in_parms), (u_char)0, (PTR)&in_parms );

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
    in_parms.gca_api_version = GCA_API_LEVEL_5;
    in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

    call_status = IIGCa_call( GCA_INITIATE, (GCA_PARMLIST *)&in_parms,
          GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  in_parms.gca_status != OK )
    {
        iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
            ERx("GCA_INITIATE error: call=0x%08x gca=0x%08x 0x%08x\n"),
            call_status, in_parms.gca_status, status);
        status = in_parms.gca_status, call_status = FAIL;
    }
    else
    {
        msg_max = GCA_FS_MAX_DATA + in_parms.gca_header_length;
        switch(status)
        {
            case E_GC0006_DUP_INIT:
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("GCA_INITIATE duplicate request: 0x%x\n"), status );
                status = OK;
                break;
            case OK:
                break;
            default:
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("GCA_INITIATE error: call=0x%08x gca=0x%08x 0x%08x\n"),
                    call_status, in_parms.gca_status, status);
            break;
        }
    }
    return(status);
}

/*
** Name: iff_gca_term
**
** Description:
**      Terminates the communications interface.
**      
** Inputs:
**      None.
**      
** Outputs:
**      None.
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
STATUS
iff_gca_term()
{
    STATUS status = OK;
    STATUS call_status = OK;
    GCA_IN_PARMS	te_parms;
    
    call_status = IIGCa_call( GCA_TERMINATE, (GCA_PARMLIST *)&te_parms,
          GCA_SYNC_FLAG, NULL, -1, &status );

    if ( call_status == OK  &&  te_parms.gca_status != OK )
    {
        status = te_parms.gca_status, call_status = FAIL;
    }
    else
    {
        switch(status)
        {
            case E_GC0007_NO_PREV_INIT:
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("GCA_TERMINATE early: 0x%x\n"), status );
                status = OK;
                break;
            case OK:
                break;
            default:
                iff_trace( IFF_TRC_ERR, IFF_TRC_ERR,
                    ERx("GCA_TERMINATE error: call=0x%08x gca=0x%08x 0x%08x\n"),
                    call_status, te_parms.gca_status, status);
            break;
        }
    }
    return(status);
}

/*
** Name: iff_gca_ping
**
** Description:
**      Takes the target details and attempts a connection.
**
** Inputs:
**      tgt         target host address information
**
** Outputs:
**      active      flag specifies whether the target has an active instance.
**      
** Returns:
**      status      OK      command completed successfully.
**                  FAIL    an error occurred processing the command.
**
** History:
**      08-Mar-2004 (fanra01)
**          Created.
*/
STATUS
iff_gca_ping( ING_CONNECT* tgt, char* msg_buff, BOOL* active )
{
    STATUS  status = OK;
    i4      tgtlen;
    char *  target;
    i4      assoc_id;
    char*   usr;
    char*   pwd;
    
    if (tgt != NULL)
    {
        *active = TRUE;
        tgtlen = STlength( tgt->hostname ) + STlength( tgtspec );
        if ((target = MEreqmem( 0, tgtlen, TRUE, &status)) != NULL)
        {
            usr = tgt->user;
            pwd = tgt->password;
            /*
            ** If the hostname is set and the user has not been specified
            ** use reserved user name otherwise use effective credentials.
            */
            if (*tgt->hostname)
            {
                if (*usr == '\0')
                {
                    STcopy( IFF_RESERVED_USR, usr );
                }
                if (*pwd == '\0')
                {
                    STcopy( IFF_RESERVED_PWD, pwd );
                }
            }
            else
            {
                *usr = '\0';
                *pwd = '\0';
            }
            STprintf( target, tgtspec, tgt->hostname );
            if ( (status = gca_request( target, usr, pwd, &assoc_id )) != OK )
            {
                *active = FALSE;
            }
            else
            {
                gca_release( assoc_id, msg_buff );
            }
            gca_disassoc( assoc_id );
            MEfree( target );
        }
    }
    return( status );
}
