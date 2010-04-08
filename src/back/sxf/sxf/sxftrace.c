
/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <tm.h>
# include    <tr.h>
# include    <st.h>
# include    <me.h>
# include    <cs.h>
# include    <lk.h>
# include    <lg.h>
# include    <adf.h>
# include    <scf.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
/*
**
**  Name: SXFTRACE.C - SXF trace entry point.
**
**  Description:
**
**	Function prototypes defined in SXFTRACE.H.
**
**      This routine takes a SXF trace request and processes it.
**
**          sxf_trace - The SXF trace entry point.
**
**  History:
**      16-jun-93 (robf)
**          Created for Secure 2.0
**	10-oct-93 (robf)
**          Updated include files to remove dbms.h
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/
FUNC_EXTERN	DB_STATUS	scf_call();
#define	SXF_MAX_MSGLEN		1024
static    	bool          	sxf_tracing_on=FALSE;

/*{
** Name: sxf_trace	- SXF trace entry point.
**
** Description:
**      This routine takes a standard trace control block as input, and
**	sets or clears the given trace point.  The trace points are either
**	specific to a session or server wide.
**
** Inputs:
**      debug_cb                        A standard debug control block.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-93 (robf)
**          Created  for Secure 2.0
**	11-oct-93 (robf)
**          Rework includes per current integration
**	30-mar-94 (robf)
**          Add tracing of shared memory info.
**	09-Sep-2008 (jonj)
**	    Correct mis-typed ule_format output error_code.
*/
DB_STATUS
sxf_trace(
DB_DEBUG_CB  *trace_cb
)
{
	i4	    bitno = trace_cb->db_trace_point;
	DB_STATUS   status=E_DB_OK;
	i4	    *vector  ;
	i4	    err_code;
	SXF_SCB	    *scb;
	i4    	    error;

        switch (bitno)
        {
	case    SXS_TDUMP_SHM:
		/*
		** Dump shared memory
		*/
		sxap_shm_dump();
		SXF_DISPLAY("SXF shared memory info dumped to trace log");
		break;

	case    SXS_TDUMP_STATS:
		/*
		** Dump statistics
		*/
		sxc_printstats();
		SXF_DISPLAY("SXF server statistics dumped to trace log");
		break;

	case    SXS_TDUMP_SESSION:
		/*
		** Dump session info
		*/
		status = sxau_get_scb(&scb, &err_code);
		if (status != E_DB_OK)
		{
		    _VOID_ ule_format(err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		    _VOID_ ule_format(E_SX001E_TRACE_FLAG_ERR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		    break;
		}
		SXF_DISPLAY("SXF Session info dumped to trace log");
		break;


	case	0:
	case    SXS_TAUD_LOG:
	case    SXS_TAUD_PHYS:
	case    SXS_TAUD_STATE:
	case    SXS_TAUD_SHM:
	case    SXS_TAUD_PLOCK:

	    status = sxau_get_scb(&scb, &err_code);
	    if (status != E_DB_OK)
	    {
		_VOID_ ule_format(err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		_VOID_ ule_format(E_SX001E_TRACE_FLAG_ERR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		break;
	    }
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		if (ult_set_macro(&scb->sxf_trace,
		    (trace_cb->db_trace_point),
		    trace_cb->db_vals[0],
		    trace_cb->db_vals[1]) != E_DB_OK)
		{
		    _VOID_ ule_format(E_SX001E_TRACE_FLAG_ERR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		}
	    }
	    else if (trace_cb->db_trswitch == DB_TR_OFF)
	    {
		if (ult_clear_macro(&scb->sxf_trace,
		    (trace_cb->db_trace_point)) != E_DB_OK)
		{
		    _VOID_ ule_format(E_SX001E_TRACE_FLAG_ERR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		}
	    }
	    break;
        default:
	     _VOID_ ule_format(E_SX001E_TRACE_FLAG_ERR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		
	}
	sxf_tracing_on=TRUE;
	return (status);    
}

/*
**
**  Name: sxf_scc_trace - Send a trace to the user via SCF
**
**
**  History:
**      16-jun-93 (robf)
**          Created for Secure 2.0, cloned  from DMF tracing
**/
VOID
sxf_scc_trace( char *msg_buffer)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_SXF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)STlength(msg_buffer);
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d while attempting to send info from SXF\n",
	    scf_cb.scf_error.err_code);
    }
}

/*{
** Name: sxf_scctalk	- send output line to front end
**
** Description:
**      Send a formatted output line to the front end. A NL character
**	is added to the end of the message. Buffer to which a pointer
**	is received should have on extra byte for `\n'.
**
** Inputs:
**	arg				place holder
**	msg_len				length of message
**      msg_buffer                      ptr to message
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	16-jun-93 (robf)
**		Created
*/
i4
sxf_scctalk(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer)
{
    char	 *p;

    if (msg_length == 0)
	return;

    /* Truncate message if necessary */
    if (msg_length > SXF_MAX_MSGLEN)
	msg_length = SXF_MAX_MSGLEN;

    /* If last char is null, replace with NL, else add NL */
    if (*(p = (msg_buffer + msg_length - 1)) == '\0')
	*p = '\n';
    else
    {
	*(++p) = '\n';
	msg_length++;
    }
    /*
    **	Send output to the user
    */
    sxf_scc_trace(msg_buffer);
}

/*{
** Name: sxf_display	- like TRdisplay but output goes to user's terminal
**
** Description:
**      send a formatted output line to the front end
**
** Inputs:
**	format				format string
**	params				variable number of parameters
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	If output requires more than 131 chars, part of it will be truncated.
**
** History:
**	24-sep-92 (robf)
**		Created
*/
VOID
sxf_display(char *format, PTR  p1, PTR  p2, PTR p3, PTR p4, PTR p5)
{
    char		buffer[SXF_MAX_MSGLEN + 1]; /* last char for `\n' */

    MEfill (sizeof(buffer), 0, buffer);
    /* TRformat removes `\n' chars, so to ensure that gwf_scctrace()
    ** outputs a logical line (which it is supposed to do), we allocate
    ** a buffer with one extra char for NL and will hide it from TRformat
    ** by specifying length of 1 byte less. The NL char will be inserted
    ** at the end of the message by gwf_scctrace().
    */
    _VOID_ TRformat(sxf_scctalk, 0, buffer, sizeof(buffer) - 1, format, 
	p1, p2, p3, p4, p5 );
}

/*{
** Name: sxf_chk_sess_trace	- Checks if a session trace flag is set.
**
** Description:
**      Checks to see if a session trace flag is set.
**
** Inputs:
**	None
**
** Outputs:
**	Returns:
**	    bool
**
** History:
**	5-jul-93 (robf)
**		Created
*/
bool  
sxf_chk_sess_trace(i4 flag)
{
	DB_STATUS status;
	SXF_SCB   *scb;
	i4   err_code;
	i4	  val1,val2;
	/*
	**	Only check if there is tracing
	**	This saves the call to SCF if no tracing is needed
	*/
	if(sxf_tracing_on==FALSE)
		return FALSE;
	/*
	**	Get session id
	*/
        status = sxau_get_scb(&scb, &err_code);
	if( status!=E_DB_OK)
	{
		return FALSE;
	}
	/*
	**	Check if trace flag is set
	*/
    	if(ult_check_macro(&scb->sxf_trace, flag, &val1, &val2))
		return TRUE;
	else
		return FALSE;
}
