/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <gca.h>
#include    <erclf.h>
#include    <ulf.h>
#include    <cm.h>
#include    <tr.h>
#include    <er.h>
#include    <me.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <si.h>
#include    <st.h>
#include    <pm.h>

/*
PROGRAM = iimonitor

NEEDLIBS = GCFLIB COMPATLIB 
**
**  History:
**	xx-xx-xx (fred)
**	    created
**	07-21-89 (greg)
**	    hack -- set local variable AID to dummy value to avoid call to
**		    mon_release with garbage
**	    Accept yyyy as server name as well as II_DBMS_xx_yyyy.
**	10-aug-89 (jrb)
**	    Added code to make sure we only send local errors to iimon_error.
**	26-dec-89 (greg)
**	    use CL_ERR_DESC for TRset_file/TRrequest, not i4
**	    Fix calls to PCexit
**      09-jan-1991 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	13-aug-1991 (stevet)
**	    Changed read-in character set support to use II_CHARSET.
**	12-oct-1991 (seg)
**	    Add support for some OS2-specific port problems.
**	13-jul-1992 (rog)
**	    Use TRformat and SIprintf instead of TRdisplay to show
**	    iimonitor output so that long lines are not cut at 132 bytes.
**	29-oct-92 (andre)
**	    rather than relying on knowing the range of generic error numbers,
**	    use protocol level to determine where to find the local error number
**	2-Jul-1993 (daveb)
**	    give main a return type, include <nm.h>, <pc.h>, and remove some
**	    unused variables, all to shut up GNU CC warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-93 (walt)
**	    Use TR_OUTPUT, TR_L_OUTPUT, TR_INPUT, TR_L_INPUT in the TRset_file
**	    calls rather than add to the (growing list of) ifdefs.
**	26-jul-93 (tyler)
**	    Added missing PMinit() call.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	28-Jul-1993 (daveb) 58935
**	    Get meaningful error on UNIX for failure to connect.
**	    Greg broke this on 07-21-89, cause for his retirement from
**	    active coding, or so it is alleged.
**      21-Mar-1995 (tutto01) BUG 67113
**          Check for SERVER_CONTROL privilege before execution.
**	13-Jun-95 (emmag & tutto01)
**	    Added NT specific port resolution.
**	30-Jun-1995 (hanch04) BUG 69514
**	    Change GChostname to PMhost
**      24-Nov-2000 (hanal04) Bug 103285 INGSRV 1322
**          "stop server" leads to E_CLFE07. Catch this error and treat 
**          it the same as a E_GC0001 error.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Jun-2004 (schka24)
**	    Safe environment variable handling.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	27-Apr-2006 (gordy)
**	    Added support for GCF servers.  Handle GCA_ERROR messages.
[@history_template@]...
*/

VOID	iimon_release();
VOID	iimon_error();

# define    _DUMMY_AID_VALUE	-9999

i4	    lang_code;

/*
** History:
**	2-Jul-1993 (daveb)
**	    Gave main function a return type, removed some unused
**	    variables.
**      7-May-2009 (hanal04) Bug 122033
**          When peer terminates the associated we need a generic message
**          as the peer may not have been the DBMS.
**     25-Oct-2010 (maspa05) Bug 124632
**          Increase size of buffer for output and ensure the size of the
**          string we're formatting with TRformat will fit in the buffer
*/
int
main(argc, argv)
i4	    argc;
char	    **argv;
{
    i4			error;
    i4			status;
    i4			aid = -9999;
    i4			count;
    i4			dbms_died = 0;
    char		buf[2048];
    STATUS              ret_status;
    GCA_DATA_VALUE	*input;
    GCA_PARMLIST	gca_parms;
    DB_TEXT_STRING	*text_string;
    CL_ERR_DESC		cl_err;
    i4		peer_protocol;
    char                *host;
    char                userbuf[GL_MAXNAME];
    char                *user = userbuf;

    MEadvise(ME_INGRES_ALLOC);

    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT,
                             &cl_err);
 
    status = TRset_file(TR_I_OPEN, TR_INPUT, TR_L_INPUT,
                             &cl_err);

    /*
    **  Initialize PM functions.
    */

    (void) PMinit();
    switch(PMload((LOCATION *) NULL, (PM_ERR_FUNC *) NULL))
    {
        case OK:
             /* loaded successfully */
             break;
        case PM_FILE_BAD:
             TRdisplay("IIMONITOR: Syntax error detected.\n");
        default:
             TRdisplay("IIMONITOR: Unable to open data file.\n");
             PCexit(FAIL);
    }

    PMsetDefault(0, "ii");
    host = PMhost();
    PMsetDefault(1, host);
    PMsetDefault(2, ERx("dbms"));
    PMsetDefault(3, ERx("*"));

    /* Set CM character set */
    ret_status = CMset_charset(&cl_err);
    if ( ret_status != OK)
    {
	TRdisplay("\n Error while processing character set attribute file.\n");
	PCexit(FAIL);
    }

    if (argc > 2)
    {
	TRdisplay("Usage: iimonitor <server_name>\n");
	PCexit(FAIL);
    }

    if (ERlangcode(NULL, &lang_code) != OK)
    {
	TRdisplay("IIMONITOR: Could not establish language code\n");
	PCexit(FAIL);
    }
    

    /*
    **  Make sure user has SERVER_CONTROL privilege.
    */
  
    IDname(&user);
    if (gca_chk_priv(user, GCA_PRIV_SERVER_CONTROL) != OK)
    {
       TRdisplay("IIMONITOR: User does not have the SERVER_CONTROL privilege.\n");
       PCexit(FAIL);
    }


    MEfill(sizeof(gca_parms), 0, &gca_parms);
    status = IIGCa_call(GCA_INITIATE,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
			
    if (status)
	PCexit(error);

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    if (argc < 2)
    {
	NMgtAt("II_DBMS_SERVER",
	     &gca_parms.gca_rq_parms.gca_partner_name);

	if ((gca_parms.gca_rq_parms.gca_partner_name  == NULL)  ||
	    (*gca_parms.gca_rq_parms.gca_partner_name == EOS))
	{
	    TRdisplay("Usage: iimonitor <server_name>\n");
	    PCexit(FAIL);
	}

	STlcopy(gca_parms.gca_rq_parms.gca_partner_name, buf, sizeof(buf)-1);
	gca_parms.gca_rq_parms.gca_partner_name = buf;
    }
    else 
    {
	STlcopy(argv[1], buf, sizeof(buf)-1);
	gca_parms.gca_rq_parms.gca_partner_name = buf;
    }

    gca_parms.gca_rq_parms.gca_modifiers = GCA_NO_XLATE | GCA_CS_CMD_SESSN;

    status= IIGCa_call(GCA_REQUEST,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);

# ifndef UNIX

    /* If on a system with complicated port names, try again with a
       likely transformation.  Unfortunately, if this is wrong, you
       won't get any error message that makes sense -- you'll get
       GCFE07 "illegal port name".  Since portnames on UNIX are only
       one way, we turn this off and always get good error messages
       (daveb) */

    if ((argc > 1) && ((status) ||
	((error = gca_parms.gca_rq_parms.gca_status) != E_GC0000_OK)))
    {
	char	*inst_ptr;

	NMgtAt("II_INSTALLATION", &inst_ptr);
#if defined(OS2) || defined(NT_GENERIC)
	if ((inst_ptr == NULL) || (*inst_ptr == NULL))
	    (VOID)STlpolycat(2, sizeof(buf)-1, "ingres\\", argv[1], buf);
	else
	    (VOID)STlpolycat(4, sizeof(buf)-1, "ingres\\", inst_ptr, "\\", argv[1], buf);
#else
	if ((inst_ptr == NULL) || (*inst_ptr == NULL))
	{
	    (VOID)STlpolycat(2, sizeof(buf)-1, "II_DBMS_", argv[1], buf);
	}
	else
	{
	    (VOID)STlpolycat(4, sizeof(buf)-1, "II_DBMS_", inst_ptr, "_", argv[1], buf);
	}
#endif

	gca_parms.gca_rq_parms.gca_partner_name = buf;
        gca_parms.gca_rq_parms.gca_modifiers = GCA_NO_XLATE  | GCA_CS_CMD_SESSN;

	status = IIGCa_call(GCA_REQUEST,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    }

# endif	/* UNIX, really "try again with different port" */

    if (status ||
        ((error = gca_parms.gca_rq_parms.gca_status) != E_GC0000_OK))
    {
        TRdisplay("GCA_REQUEST failed to connect to with error = %x\n", error);
        iimon_error(aid, error);
    }

    aid = gca_parms.gca_rq_parms.gca_assoc_id;
    peer_protocol = gca_parms.gca_rq_parms.gca_peer_protocol;


    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_rv_parms.gca_buffer = buf;
    gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
    gca_parms.gca_rv_parms.gca_association_id = aid;
    gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
    status= IIGCa_call(GCA_RECEIVE,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if ((status) ||
	    ((error = gca_parms.gca_rv_parms.gca_status) != E_GC0000_OK))
    {
	TRdisplay("GCA_RECEIVE returned status %x\n", error);
	iimon_error(aid, error);
    }

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_it_parms.gca_buffer = buf;
    status= IIGCa_call(GCA_INTERPRET,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if (status)
    {
	TRdisplay("GCA_INTERPRET returned status %x\n", error);
	iimon_error(aid, error);
    }

    if (gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
    {
	i4	    err;

	/*
	** if protocol is <= 2, local error is in gca_id_error; otherwise it is
	** in gca_local_error
	*/
	if (peer_protocol <= GCA_PROTOCOL_LEVEL_2)
	{
	    err = ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
		    gca_e_element[0].gca_id_error;
	}
	else
	{
	    err = ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
		    gca_e_element[0].gca_local_error;
	}
	
	iimon_error(aid, err);
    }

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_fo_parms.gca_buffer = buf;
    gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
    status= IIGCa_call(GCA_FORMAT,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if (status)
    {
	TRdisplay("GCA_FORMAT returned status %x\n", error);
	iimon_error(aid, error);
    }

    ((GCA_SESSION_PARMS *) gca_parms.gca_fo_parms.gca_data_area)->gca_l_user_data = 1;
    ((GCA_SESSION_PARMS *) gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_index = GCA_SVTYPE;
    ((GCA_SESSION_PARMS *)
	    gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.gca_type = GCA_TYPE_INT; 
    
    ((GCA_SESSION_PARMS *)
	    gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.gca_l_value  = sizeof(i4); 
    
    *((i4 *) ((GCA_SESSION_PARMS *)
	    gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.gca_value) = GCA_MONITOR; 
    
    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_sd_parms.gca_buffer = buf;
    gca_parms.gca_sd_parms.gca_msg_length =
	sizeof(GCA_SESSION_PARMS) + sizeof(i4) - sizeof(char); 
    gca_parms.gca_sd_parms.gca_message_type = GCA_MD_ASSOC;
    gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
    gca_parms.gca_sd_parms.gca_association_id = aid;
    status= IIGCa_call(GCA_SEND,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if ((status) ||
	    ((error = gca_parms.gca_sd_parms.gca_status) != E_GC0000_OK))
    {
	TRdisplay("GCA_SEND returned status %x\n", error);
	iimon_error(aid, error);
    }

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_rv_parms.gca_buffer = buf;
    gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
    gca_parms.gca_rv_parms.gca_association_id = aid;
    gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
    status= IIGCa_call(GCA_RECEIVE,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if ((status) ||
	    ((error = gca_parms.gca_rv_parms.gca_status) != E_GC0000_OK))
    {
	TRdisplay("GCA_RECEIVE returned status %x\n", error);
	iimon_error(aid, error);
    }

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_it_parms.gca_buffer = buf;
    status= IIGCa_call(GCA_INTERPRET,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if (status)
    {
	TRdisplay("GCA_INTERPRET returned status %x\n", error);
	iimon_error(aid, error);
    }

    if (gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
    {
	i4	    id_err;

	id_err = ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
						gca_e_element[0].gca_id_error;

	/* If this is in the generic error range, send the local error */
	if (id_err > 30000  &&  id_err < 50000)
	{
	    iimon_error(aid, 
		((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
					    gca_e_element[0].gca_local_error);
	}
	else
	{
	    iimon_error(aid, id_err);
	}
    }
    
    while ((status == OK) && (dbms_died == 0))
    {
	MEfill(sizeof(gca_parms), 0, &gca_parms);
	gca_parms.gca_fo_parms.gca_buffer = buf;
	gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
	status= IIGCa_call(GCA_FORMAT,
			    &gca_parms,
			    GCA_SYNC_FLAG,
			    (PTR)0,
			    -1,
			    &error);
	if (status)
	{
	    TRdisplay("GCA_FORMAT returned status %x\n", error);
	    iimon_error(aid, error);
	}

	MEfill(gca_parms.gca_fo_parms.gca_d_length,
		    0,
		    gca_parms.gca_fo_parms.gca_data_area);
	input = (GCA_DATA_VALUE *)
			((char *) gca_parms.gca_fo_parms.gca_data_area
				+ sizeof(i4) + sizeof(i4));
	input->gca_type = DB_QTXT_TYPE;
	input->gca_l_value = gca_parms.gca_fo_parms.gca_d_length;
	status = TRrequest(input->gca_value,
				input->gca_l_value,
				&count,
				&cl_err,
				"IIMONITOR> ");
	if (status)
	    break;
	if (count < 2)
	    continue;
	input->gca_value[count] = 0;
	input->gca_l_value = count;
	if (STcasecmp("quit", input->gca_value ) == 0)
	    break;
	
	MEfill(sizeof(gca_parms), 0, &gca_parms);
	gca_parms.gca_sd_parms.gca_buffer = buf;
	gca_parms.gca_sd_parms.gca_msg_length =
		(2 * sizeof(i4)) + count +
			    sizeof(input->gca_type)
			    + sizeof(input->gca_precision)
			    + sizeof(input->gca_l_value);
	gca_parms.gca_sd_parms.gca_message_type = GCA_QUERY;
	gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
	gca_parms.gca_sd_parms.gca_association_id = aid;
	status= IIGCa_call(GCA_SEND,
			    &gca_parms,
			    GCA_SYNC_FLAG,
			    (PTR)0,
			    -1,
			    &error);
	if ((status) ||
		((error = gca_parms.gca_sd_parms.gca_status) != E_GC0000_OK))
	{
	    if (error == E_GC0001_ASSOC_FAIL)
	    {
		dbms_died = 1;
		break;
	    }
	    TRdisplay("GCA_SEND returned status %x\n", error);
	    iimon_error(aid, error);
	}
	do
	{
	    MEfill(sizeof(gca_parms), 0, &gca_parms);
	    gca_parms.gca_rv_parms.gca_buffer = buf;
	    gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
	    gca_parms.gca_rv_parms.gca_association_id = aid;
	    gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
	    status= IIGCa_call(GCA_RECEIVE,
				&gca_parms,
				GCA_SYNC_FLAG,
				(PTR)0,
				-1,
				&error);
	    if (status || gca_parms.gca_rv_parms.gca_status != E_GC0000_OK)
	    {
		if ((gca_parms.gca_rv_parms.gca_status == E_GC0001_ASSOC_FAIL)
			||
                    (gca_parms.gca_rv_parms.gca_status == E_CLFE07_BS_READ_ERR))
		{
		    dbms_died = 1;
		    break;
		}
		TRdisplay("GCA_RECEIVE returned status %x\n",
				gca_parms.gca_rv_parms.gca_status);
		PCexit(FAIL);
	    }

	    MEfill(sizeof(gca_parms), 0, &gca_parms);
	    gca_parms.gca_it_parms.gca_buffer = buf;
	    status= IIGCa_call(GCA_INTERPRET,
				&gca_parms,
				GCA_SYNC_FLAG,
				(PTR)0,
				-1,
				&error);
	    if (status)
	    {
		TRdisplay("GCA_INTERPRET returned status %x\n", error);
		iimon_error(aid, error);
	    }
	    if (gca_parms.gca_it_parms.gca_message_type == GCA_TDESCR)
	    {
		continue;
	    }
	    else if ((gca_parms.gca_it_parms.gca_message_type == GCA_RELEASE)
			|| (gca_parms.gca_it_parms.gca_status
					    == E_GC0001_ASSOC_FAIL)
			|| (gca_parms.gca_it_parms.gca_status
					    == E_GC0023_ASSOC_RLSED))
	    {
		dbms_died = 1;
		break;
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type == GCA_TUPLES)
	    {
		text_string = (DB_TEXT_STRING *)
			gca_parms.gca_it_parms.gca_data_area;
		TRdisplay("%t\n", text_string->db_t_count,
				text_string->db_t_text);
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type == GCA_TRACE)
	    {
	      /* buf is sized for largest output line which is currently
		 "Last Query" so it's size of query text (ER_MAX_LEN) +
	        20 for "    Last Query:" */

		char	buf[ER_MAX_LEN+20]; 

		TRflush();

		if (gca_parms.gca_it_parms.gca_d_length >= sizeof(buf))
		    gca_parms.gca_it_parms.gca_d_length = sizeof(buf) - 1;

		TRformat(0, 0, buf, sizeof(buf), "%t", 
			 gca_parms.gca_it_parms.gca_d_length,
			 gca_parms.gca_it_parms.gca_data_area);
		
		buf[gca_parms.gca_it_parms.gca_d_length] = EOS;

		STtrmwhite(buf);
		SIprintf("%s\n", buf);
		SIflush(stdout);
	    }
	    else if  (gca_parms.gca_it_parms.gca_message_type == GCA_ERROR)
	    {
		char	buf[ER_MAX_LEN];
		GCA_E_ELEMENT   *ele;
		GCA_ER_DATA   *er;

		er = (GCA_ER_DATA *)gca_parms.gca_it_parms.gca_data_area;
		if ( er->gca_l_e_element )
			ele = (GCA_E_ELEMENT *)er->gca_e_element; 
		TRflush();

		if (ele->gca_error_parm[0].gca_l_value >= sizeof(buf))
		    ele->gca_error_parm[0].gca_l_value = sizeof(buf) - 1;
		
		if ( ele->gca_l_error_parm )    
			TRformat(0, 0, buf, sizeof(buf), "%t", 
			 ele->gca_error_parm[0].gca_l_value,
			 ele->gca_error_parm[0].gca_value );
			  
		buf[ele->gca_error_parm[0].gca_l_value] = EOS;

		STtrmwhite(buf);
		SIprintf("%s\n", buf);
		SIflush(stdout);
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type != GCA_RESPONSE)
	    {
		TRdisplay("Unexpected message type %x (%<%d.) -- ignored\n",
				gca_parms.gca_it_parms.gca_message_type);
	    }
	}
	while ((gca_parms.gca_it_parms.gca_message_type != GCA_RESPONSE)
			&& (dbms_died == 0));
    }
    

    if (!dbms_died)
    {
	iimon_release(aid);
    }
    else
    {
	TRdisplay("Server process terminated association\n");
    }

    if ((status) && (status != 0x012103))
	TRdisplay("Reason for termination: %x%<(%d.)\n", status);

    TRset_file(TR_I_CLOSE, 0, 0, &cl_err);
    TRset_file(TR_T_CLOSE, 0, 0, &cl_err);

    PCexit(OK);
    /* NOTREACHED */
    return( FAIL );
}

/*
** History:
**	2-Jul-1993 (daveb)
**	    removed unused variable 'error'
*/
VOID
iimon_error(aid, errcod)
i4  aid;
i4  errcod;
{
    i4			err_msg_len;
    CL_ERR_DESC		cl_err_code;
    char		errbuf[ER_MAX_LEN];
    static i4		been_here_before = FALSE;

    if (been_here_before == TRUE)
    {
	TRdisplay("IIMONITOR: Cannot handle a second error %d\n", errcod);
    }
    else
    {
	been_here_before = TRUE;

	if (ERslookup(
		errcod,
		NULL,
		0,
		(char *) NULL,
		errbuf,
		ER_MAX_LEN,
		lang_code,
		&err_msg_len,
		&cl_err_code,
		0,
		NULL)
	    == OK)
	{
		TRdisplay("IIMONITOR: %s\n", errbuf);
	}
	else
	{
	    TRdisplay(
	"IIMONITOR: Could not find error message for %d using ERslookup().\n", 
								    errcod);
	}

	iimon_release(aid);
    }

    TRset_file(TR_I_CLOSE, 0, 0, &cl_err_code);
    TRset_file(TR_T_CLOSE, 0, 0, &cl_err_code);

    PCexit(FAIL);
}

VOID
iimon_release(aid)
i4		aid;
{
    i4			error;
    i4			status;
    char		buf[2048];
    GCA_PARMLIST	gca_parms;
    GCA_DA_PARMS	da_parms;


    if (aid == _DUMMY_AID_VALUE)
	return;

    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_fo_parms.gca_buffer = buf;
    gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
    status= IIGCa_call(GCA_FORMAT,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if (status)
    {
	TRdisplay("GCA_FORMAT (for release) returned status %x\n", error);
	iimon_error(aid, error);
    }

    ((GCA_ER_DATA *) gca_parms.gca_fo_parms.gca_data_area)->gca_l_e_element = 0;
    
    MEfill(sizeof(gca_parms), 0, &gca_parms);
    gca_parms.gca_sd_parms.gca_buffer = buf;
    gca_parms.gca_sd_parms.gca_msg_length = 
	sizeof( ((GCA_ER_DATA *)0)->gca_l_e_element );
    gca_parms.gca_sd_parms.gca_message_type = GCA_RELEASE;
    gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
    gca_parms.gca_sd_parms.gca_association_id = aid;
    status= IIGCa_call(GCA_SEND,
			&gca_parms,
			GCA_SYNC_FLAG,
			(PTR)0,
			-1,
			&error);
    if (status)
    {
	TRdisplay("GCA_SEND (for release) returned status %x\n", error);
	iimon_error(aid, error);
    }

    MEfill(sizeof(da_parms), 0, &da_parms);
    da_parms.gca_association_id = aid;
    da_parms.gca_status = E_GC0000_OK;
    status = IIGCa_call(GCA_DISASSOC,
			    (GCA_PARMLIST *)&da_parms,
			    GCA_SYNC_FLAG,
			    (PTR)0,
			    -1,
			    &error);

    if (status)
    {
	TRdisplay("GCA_DISASSOC returned status %x\n", error);
	iimon_error(aid, error);
    }
}
