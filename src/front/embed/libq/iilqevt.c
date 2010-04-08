/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<generr.h>
# include	<gca.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>
# include	<adh.h>
# include	<fe.h>


/* {
** Name: iilqevt.c - Process events for access by applications.
**
** Description:
**	This module contains the routines to read events arriving
**	from the DBMS, to save a description of those events in an
**	internal list, and to return information about the latest
**	event to the user application via the INQUIRE_SQL statement.
**
**	At the present time only synchronous event processing is
**	supported, i.e. the user application must poll via the
**	GET DBEVENT statement to get an event or the user application can 
**	code an event handler or an FRS event activation block.
**	Example statements andthe generated code are shown below:
**
**	EXEC SQL GET DBEVENT
**	generates:
**		IILQesEvStat(0, 0);
**
**	EXEC SQL GET DBEVENT WITH WAIT = 4;
**	generates:
**		IILQesEvStat(0, 4);
**
** Defines:
**	IILQesEvStat	- Get and consume the event. 
**	IILQeaEvAdd 	- Read event and print and/or add to list.
**	IILQedEvDisplay - Display event information to stdout.
**	IILQefEvFree	- Free the first consumed event on the list (if any).
**
** History:
**	03-nov-1989	(barbara)
**	    Written for Phoenix/Alerters.
**	07-may-1990	(barbara)
**	    Don't add/process events if disconnecting.
**	25-may-1990	(barbara)
**	    Cleaned up formating of displayed events.
**	15-may-1990	(barbara)
**	    When processing GET EVENT call IIqry_start for some basic
**	    error checking, such as are we connected?
**	18-apr-1991	(teresal)
**	    Made major modifications to eliminate GET EVENT ability
**	    to retrieve event information - this functionality has been
**	    replaced with INQUIRE_SQL.  GET EVENT now just polls for and 
**	    consumes the event (if found). Added concept of "consuming" an event
**	    which means the event has been polled for and has been put on the
**	    top of the list.  An consumed event will remain on the
**	    top of the list so the user can inquire on it until the next event 
**	    is polled for.
**	20-jul-1991	(teresal)
**	    Change EVENT to DBEVENT.
**	02-01-1992 (kathryn)
**	    Terminal Monitor now registers a dbevent handler.  Only display 
**	    event information for II_G_TM flag when an event handler has not 
**	    been registered.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-jan-2009 (stial01)
**          Fix define EV_MSGMAX to reflect dependency on DB_MAXNAME.
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/


/*{
** Name: IILQesEvStat - Get event information/clean up afterwards. 
**
** Description:
**	This routine is called when an application issues the GET
**	DBEVENT statement or when a user defined event handler is called.  
**	See the module header for an example of the GET DBEVENT statement 
**	and the generated code. 
**
**	This routine checks the II_EVENT list and returns if it is non-empty.
**	Otherwise, it attempts to read an event from GCA, telling 
**	LIBQGCA to block on this read according to the value of the WAIT 
**	clause in the GET DBEVENT statement.
**
**	An event cannot be read from GCA if the application is currently
**	executing a query, e.g. in a SELECT loop or a dbproc message
**	handler or a DBEVENTHANDLER.  Therefore, if the wait parameter is
**	non-zero, an error is raised (really a warning) and no read is
**	attempted.
**
**	Getting an event is not a query to the dbms so IIqry_start
**	is called only for some error checking such as "are we
**	connected?".  IILQesEvStat does not set the INQRY
**	flag because in theory, nothing can interrupt the GET DBEVENT
**	statement, not even the DBEVENTHANDLER.
**
**	Since the GET DBEVENT statement is not a query and does not read
**	a response block, IIqry_end is not called.  Instead, this routine
**	performs one task of IIqry_end and that is to set the ii_lq_dml
**	back to QUEL.  It is assumed that outer scopes (eg SELECT loops,
**	handler entry points) will save and restore their version of the
**	dml and the SQLCA pointer.
**
**	Note that the II_EVENT list may be empty.
**
**	1) When called with the 'escall' flag set to 0 it is being
**	   called from a user application.
**
**	2) When called with the 'escall' flag set to 1 it does not 
**         reset the dml and does not call query start - this is because
**	   the function is being called internally by LIBQ, i.e., before 
**	   the user defined event handler is called.
**
** Input:
**	escall		Flag to indicate who called this routine:
**			-1: function is being called by LIBQ to consume 
**			    the event before a user defined event
**			    handler is called.
**			 0: function is being called by a GET DBEVENT 
**			    statement in the application.
**			 1: function is called by the WHENEVER DBEVENT code
**			    generated in the user's application.
**	eswait		Number of seconds that LIBQGCA must block on
**			reading event.  A value of -1 means to wait forever.
** Outputs:
**	None.
**	Returns:
**	    VOID
**
**	Errors:
**	    E_LQ00C7_EVWAIT - The WAIT option is not valid when GET DBEVENT
**			      is nested within another query.
**	    E_LQ002A_NODB   - Not connected when getting event.
**
** Side Effects:
**	-  If an event is read from GCA, a new event will be inserted
**	   into LIBQ's list of II_EVENTS
**	-  If an error happens in reading event (e.g. an error in
**	   IIcgc_event_read or a user error in specifying the WAIT clause),
**	   the II_EV_GETERR flag is set which will cause the event to not
**	   be added to the list.  Note that timing out when reading an event is 
**	   NOT an error.
**
** History:
**	1-nov-89 (barbara)
**	    Written for Phoenix/Alerters
**	15-jun-1990 (barbara)
**	    When fixing bug 21832, which involved more error checking, I
**	    decided to make this routine call IIqry_start instead of
**	    duplicating more code in here.  IIqry_start will return
**	    after error checking if the type is GCA_EVENT.  
**	15-mar-1991 (kathryn)
**	    Issue error if "get event with wait" is issued from any user 
**	    defined handler.
**	19-apr-1991 (teresal)
**	    Major modification to have this routine only be
**	    called once and to consume the event.
*/
VOID
IILQesEvStat(escall, eswait)
i4	escall;
i4	eswait;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		savedml;	

    /* 
    ** First cleanup event queue.   Remove event (if any)
    ** from II_EVENT list if it has already been consumed.
    */
    if (IIlbqcb->ii_lq_event.ii_ev_list != (II_EVENT *)0)
	IILQefEvFree( IIlbqcb );

    /* Only do a query init if being called because of a "GET DBEVENT" */
    if (escall == 0)	
    {
        if (IIqry_start(GCA_EVENT, 0, 0, ERx("get dbevent")) != OK)
	    return;
    }	
    /* Make sure dml is set if called from WHENEVER handling */
    else if (escall == 1)
    {
	savedml = IIlbqcb->ii_lq_dml;
        IIlbqcb->ii_lq_dml = DB_SQL;
    }
    /*
    ** Only try to read an event if LIBQ's list is empty.
    */
    if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0)
    {
	/*
	** If nested in a SELECT/RETRIEVE loop or a DBPROC execution loop
	** or if called from a handler within LIBQ, don't attempt to
	** read event data from GCA; give error if user requested WAIT.
	*/
	if (   IIlbqcb->ii_lq_flags & (II_L_RETRIEVE|II_L_DBPROC)
	    || IIlbqcb->ii_lq_curqry & II_Q_INQRY
	    || ((thr_cb->ii_hd_flags & 
		(II_HD_MSHDLR|II_HD_EVHDLR|II_HD_ERHDLR))
               && (IILQscSessionCompare( thr_cb ) == TRUE))
           )
	{
	    if (eswait != 0)
	    {
		IIlocerr(GE_LOGICAL_ERROR, E_LQ00C7_EVWAIT, II_ERR,0,(char *)0);
		IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_GETERR;
	    }
	}
	else
	{
            IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;

	    if (IIcgc_event_read( IIlbqcb->ii_lq_gca, eswait ) == FAIL)
	        IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_GETERR;
	}
    }
    /* 
    ** Completion of processing GET DBEVENT.  Mark event as consumed
    ** from II_EVENT list if there has been no error in getting the event. 
    */
    if (IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_GETERR)
	IIlbqcb->ii_lq_event.ii_ev_flags &= ~II_EV_GETERR;
    else if (IIlbqcb->ii_lq_event.ii_ev_list != (II_EVENT *)0)
	IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_CONSUMED; 

    /* If more events still on list, set sqlcode */
    if (escall == 0 && IISQ_SQLCA_MACRO(IIlbqcb))
	IIsqEvSetCode( IIlbqcb );
    /* Reset DML if control is returning to user */
    if (escall == 0)
        IIlbqcb->ii_lq_dml = DB_QUEL;
    else if (escall == 1)
	IIlbqcb->ii_lq_dml = savedml;
}


/*{
** Name: IILQeaEvAdd - Read an event and print and/or add to LIBQ's event list
**
** Description:
**	This routine calls LIBQGCA to read the values and data associated
**	with an EVENT that has been received from the DBMS.  It saves the
**	information in a list element and attaches that event to LIBQ's
**	list of events.  It is called via a handler from LIBQGCA when a
**	GCA_EVENT message has been detected.  To prevent itself from
**	being called recursively on a GCA_EVENT message which spans block
**	boundaries, this routine sets a LIBQGCA flag IICGC_PROCEVENT_MASK
**	before reading data and resets this flag when through.
**
**	Ignoring block boundaries, an event message is layed out with
**	the following format:
**
**	    typedef struct {
**		GCA_NAME	event_name;
**		GCA_NAME	event_owner;
**		GCA_NAME	event_db;
**		GCA_DATA_VALUE	event_time;	  A date value
**		i4		event_data_count  No. of data values following
**	    [
**		GGA_DATA_VALUE  event_data[1];
**	    ]
**
**	If the terminal monitor is running or if the application has
**	requested (via SET_INGRES or II_EMBED_SET) to print event
**	information, the information will be printed to the screen
**	in a format similar to TRACE messages.  If not running as the
**	TM, IILQeaEvAdd allocates an IIEVENT structure and reads the
**	event name, owner name, database name, timestamp information
**	into II_EVENT.  If data is present, space for this is allocated,
**	the data is copied and the ii_etext field in the II_EVENT is set
**	to point at the data.
**
**	Note that it is currrently an error if: 
**	    1.  there is more than one GCA_DATA_VALUE for event data;
**	    2.  the gca_type field for data is anything other than DB_VCH_TYPE.
**
**	This restriction will be relaxed in future releases.
**
**	The II_EVENT is inserted at the end of the II_EVENT list.
**	The event queue is a FIFO list so when the user asks for an
**	event, he gets the first event in the list.  Once the event is polled for,
**	the event is considered to be consumed; an event is polled 
**	for as follows:
**	    1.  After being accessed by the application with the GET EVENT
**		statement;
**	    2.  During WHENEVER DBEVENT CALL SQLPRINT error handling;
**	    3.  A user defined event handler is called.
**	    4.  At disconnect time.
**	Once an event has been consumed it will continue to be on the 
**	event list until we poll for the next event at which time the 
**	previous event will be freed.
**
**      This routine calls the user DBEVENTHANDLER, if any, after adding the 
**      event to the queue.   Note that it does not call the handler
**	if the EVENT is being read in response to a GET DBEVENT statement.
**
** Inputs:
**	thr_cb		Thread-local-storage control block.
** Outputs:
**	None
**	Errors:
**	    E_LQ00C5_EVALLOC - Unable to allocate memory for event handling.
**	    E_LQ00C6_EVPROTOCOL - Read protocol failure in event handling.
** Side Effects:
**	None
**
** History:
**	1-nov-89 (barbara)
**	    Written for Phoenix/Alerters	
**	07-may-1990 (barbara)
**	    Don't add/process events if disconnecting.
**	15-mar-1991 (kathryn)
**	    Call user defined event handler if one was set.
**	01-apr-1991 (kathryn)
**	    Push and Pop session ids around user handlers to check that user
**	    has not switched sessions in handler without switching back.
**	22-apr-1991 (teresal)
**	    Changed event handler to always consume the event.
**	02-01-1992 (kathryn)
**	    Terminal Monitor now reigsters a database event handler. 
**	    Only do special handling for II_G_TM flag when a dbevent handler
**	    has not been registered. Note that II_G_TM flag is used by report 
**	    writer.
*/
VOID
IILQeaEvAdd( II_THR_CB *thr_cb )
{
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4			(*event_hdl)() = IIglbcb->ii_gl_hdlrs.ii_evt_hdl;
    II_EVENT		event, *evt, *eptr;
    char		*evcharattrs[3];
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    i4			name_len;
    DB_DATA_VALUE	db_num;
    DB_DATA_VALUE	db_char;
    DB_DATA_VALUE	db_date;
    i4			type, prec;
    i4			i, ev_data_count;
    i2			count;
    i4		length;
    i4		gl_flags = IIglbcb->ii_gl_flags;

    /* Don't process events if program is disconnecting. */ 
    if (IIlbqcb->ii_lq_curqry & II_Q_DISCON)
	return;
    /*
    ** For applications we dynamically allocate memory for event information
    ** so that it may be saved on a list.  For II_G_TM flag, use stack if a 
    ** handler is not registered.
    */
    if ( event_hdl  ||  ! (gl_flags & II_G_TM) )
    {
	if ((evt = (II_EVENT *)MEreqmem((u_i4)0, (u_i4)sizeof(II_EVENT),
			TRUE, (STATUS *)0)) == (II_EVENT *)0)
	{
	    IIlocerr( GE_NO_RESOURCE, E_LQ00C5_EVALLOC, II_ERR, 0, (char *)0 );
	    return;
	}
    }
    else
    {
	evt = &event;
    }
    evt->iie_next = (II_EVENT *)0;
    evt->iie_text = (char *)0;
    /* 
    ** Inform LIBQGCA that event is being processed.  Want to avoid this
    ** routine being called recursively if event spans more than one block.
    */
    cgc->cgc_m_state |= IICGC_PROCEVENT_MASK; 
    /* 
    ** Set up for getting character event attributes (name, owner, db).
    ** These are in the form of a GCA_NAME -- a length followed by name. 
    */
    II_DB_SCAL_MACRO(db_num, DB_INT_TYPE, sizeof(i4), &name_len);
    II_DB_SCAL_MACRO(db_char, DB_CHR_TYPE, 0, 0);
    evcharattrs[0] = evt->iie_name;
    evcharattrs[1] = evt->iie_owner;
    evcharattrs[2] = evt->iie_db;

    for (i = 0; i < 3; i++)
    {
	if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num)
		    != sizeof(name_len))
	{
	    goto proto_err;
	}
	db_char.db_length = min(name_len, DB_MAXNAME);
	db_char.db_data = evcharattrs[i];
	if (name_len > 0)			/* Get as much as will fit */
	{
	    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_char)
		    != db_char.db_length)
	    {
		goto proto_err;
	    }
	evcharattrs[i][db_char.db_length] = '\0';
	}
	if (name_len > DB_MAXNAME)		/* Skip the rest if any */
	{
	    db_char.db_length = name_len - DB_MAXNAME;
	    db_char.db_data = (PTR)0;
	    _VOID_ IIcgc_get_value( cgc, GCA_EVENT, IICGC_VVAL, &db_char );
	}
    }

    /* Get date attribute.  This is in the form of a GCA_DATA_VALUE  */

    /* Read type */
    db_num.db_data = (PTR)&type;
    if (   IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(type)
	|| type != DB_DTE_TYPE
       )
    {
	goto proto_err;
    }

    /* Read precision */
    db_num.db_data = (PTR)&prec;
    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(prec))
	goto proto_err;

    /* Read length */
    db_num.db_length = sizeof(i4);
    db_num.db_data = (PTR)&length;
    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(i4))
	goto proto_err;

    /* Read date data */
    II_DB_SCAL_MACRO(db_date, DB_DTE_TYPE, length, 0);
    db_date.db_data = evt->iie_time.db_date;
    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_date)
		!= db_date.db_length)
    {
	goto proto_err;
    }

    /* See if there is any event text */
    II_DB_SCAL_MACRO(db_num, DB_INT_TYPE, sizeof(i4), &ev_data_count);
    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(i4))	
	goto proto_err;

    if (ev_data_count == 1)
    {
	/* Read GCA_DATA_VALUE containing text */
	db_num.db_data = (PTR)&type;
	if (   IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num)
		!= sizeof(i4)
	     || type != DB_VCH_TYPE
	   )
	{
	    goto proto_err;
	}
	db_num.db_data = (PTR)&prec;
	if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(i4))
	    goto proto_err;
	db_num.db_data = (PTR)&length;
	db_num.db_length = sizeof(i4);
	if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num)
		!= sizeof(i4))
	{
	    goto proto_err;
	}
	/* Read off two byte text count and check that it's a legal value */
	db_num.db_data = (PTR)&count;
	db_num.db_length = sizeof(i2);
	if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_num) != sizeof(i2))
	    goto proto_err;
	if (count > length -2)
	    goto proto_err;

	/* Allocate for text -- add one for null terminator */
	if ((evt->iie_text = (char *)MEreqmem((u_i4)0, (u_i4)(count +1),
			TRUE, (STATUS *)0)) == (char *)0)
	{
	    IIlocerr( GE_NO_RESOURCE, E_LQ00C5_EVALLOC, II_ERR, 0, (char *)0 );
	    return;
	}
	db_char.db_data = evt->iie_text;
	db_char.db_length = count;
	if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_char) != count)
	{
	    goto proto_err;
	}
	evt->iie_text[count] = '\0';

	/* Read off any extra bytes beyond count */
	if (length = length -(count +2))
	{
	    db_char.db_data = (char *)0;
	    db_char.db_length = length;
	    if (IIcgc_get_value(cgc, GCA_EVENT, IICGC_VVAL, &db_char) != length)
	    {
	        goto proto_err;
	    }
	}
    }
    else if (ev_data_count != 0)	/* More than one piece of text is err */
    {
	goto proto_err;
    }

    /* Print out information on event */
    if ( (gl_flags & II_G_TM  &&  ! event_hdl)  ||
	 IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_DISP )
	IILQedEvDisplay( IIlbqcb, evt );

    /*
    ** If II_G_TM flag and no dbevent handler, don't save event onto LIBQ's 
    ** event list, but do free any space that may have been allocated for 
    ** text data.
    */ 
    if ( gl_flags & II_G_TM  &&  ! event_hdl )
    {
        if (evt->iie_text != (char *)0)
	    MEfree(evt->iie_text);
    }
    else /* save event as last element on LIBQ's event list */
    {
	if ((eptr = IIlbqcb->ii_lq_event.ii_ev_list) == (II_EVENT *)0)
	{
	    IIlbqcb->ii_lq_event.ii_ev_list = evt;
	}
	else
	{
	    while (1)
	    {
		if (eptr->iie_next == (II_EVENT *)0)
		{
		    eptr->iie_next = evt;
		    break;
		}
		eptr = eptr->iie_next; 
	    }
	}
    }

    cgc->cgc_m_state &= ~IICGC_PROCEVENT_MASK; 	/* Event processing finished */

    /* Call DBEVENTHANDLER, if one was set */

    if ( event_hdl  &&  ! (thr_cb->ii_hd_flags & II_HD_EVHDLR) )
    {
	IILQsepSessionPush( thr_cb );
	thr_cb->ii_hd_flags |= II_HD_EVHDLR;
	/* Consume event before calling handler */
	IILQesEvStat(-1, 0);
	_VOID_ (*event_hdl)();
	thr_cb->ii_hd_flags &= ~II_HD_EVHDLR;

	/*
	** Issue error if user has switched sessions in handler and not
	** switched back.
	*/
	_VOID_ IILQscSessionCompare( thr_cb );  /* issues the error */
	IILQspSessionPop( thr_cb );
    }
    if ( ! (gl_flags & II_G_TM) ) 
	IIsqEvSetCode( IIlbqcb );	/* Set SQLCA */

    return;

proto_err:
    IIlocerr(GE_COMM_ERROR, E_LQ00C6_EVPROTOCOL, II_ERR, 0, (char *)0);
    return;
}

/*{
** Name - IILQedEvDisplay - Display event information to stdout.
**
** Description:
**	This routine is called to display all the information associated
**	with an event to the standard output.  It is called at the
**	following times:
**	1) after a GCA_EVENT message event has been read from GCA
**	   and either of the following is true:
**	   - the TM is running or
**	   - the program has set DBEVENTDISPLAY through II_EMBED_SET or
**		SET_INGRES.
**	2) if a 3GL program has issued the following statement:
**	    EXEC SQL WHENEVER DBEVENT CALL SQLPRINT;
**	   In this case, we are called by IIsqPrint.
**
** Inputs:
**	IIlbqcb		Current session control block.
**	event		Pointer to an II_EVENT structure containing event info.
**			(Should never be null)
**
** Outputs:
**	None.
**	Returns:
**	VOID.
**
** Side Effects:
**	The event information is displayed to stdout.
**
** History:
**	21-nov-1989	(barbara)
**	    Written for Phoenix/Alerters.
**	25-may-1990	(barbara)
**	    Cleaned up formatting display of events.
**	09-dec-1992 	(teresal)
**	    Modified ADF control block reference to be session specific
**	    instead of global (part of decimal changes).
*/

VOID
IILQedEvDisplay( II_LBQ_CB *IIlbqcb, II_EVENT *event )
{
    DB_DATA_VALUE	db_dte;
    DB_EMBEDDED_DATA	ev_dbuf;
    char		date_buf[ADH_EXTLEN_DTE +1];
/* 
** In EV_MSGMAX - allow DB_MAXNAME*3 + DATELEN + 70 chars of text
** 		  + newlines plus titles
*/
#define	EV_MSGMAX	(250 + (3*DB_MAXNAME))
    char		ev_msgbuf[EV_MSGMAX];

    /* Convert DATE */
    II_DB_SCAL_MACRO(db_dte, DB_DTE_TYPE, DB_DTE_LEN, &event->iie_time);
    db_dte.db_prec = 0;	  /* Should later fix II_DB_SCAL_MACRO to set prec */
    ev_dbuf.ed_type = DB_CHR_TYPE;
    ev_dbuf.ed_length = ADH_EXTLEN_DTE;
    ev_dbuf.ed_data = date_buf;
    ev_dbuf.ed_null = (i2 *)0;
    adh_dbcvtev( IIlbqcb->ii_lq_adf, &db_dte, &ev_dbuf );

    /* Put contents of event into a buffer, truncating text */
    if (event == (II_EVENT *)0)
	return;
    if (event->iie_text != (char *)0)
    {
	STprintf( ev_msgbuf,
		  "\nEVENT NAME: %s\nOWNER:      %s\nDATABASE:   %s\
\nDATE:       %s\nTEXT:       %.70s\n",
		  event->iie_name, event->iie_owner, event->iie_db,
		  date_buf, event->iie_text);
    }
    else
    {
	STprintf( ev_msgbuf,
		  "\nEVENT NAME: %s\nOWNER:      %s\nDATABASE:   %s\
\nDATE:       %s\n",
		  event->iie_name, event->iie_owner, event->iie_db, date_buf );
    }

    IImsgutil(ev_msgbuf);
}


/*{
** Name - IILQefEvFree - Remove event from LIBQ's list and free memory.
**
** Description:
**	This routine is called after the GET DBEVENT statement, after
**	WHENEVER DBEVENT CALL SQLPRINT error handling, and upon disconnection.
**	It frees the first event (if any) on LIBQ's list (only if it has been
**	consumed first) and frees any memory associated with text data.  
**
** Inputs:
**	IIlbqcb		Current session control block.
** Outputs:
**	None.
**
** History:
**	14-dec-1989	(barbara)
**	    Written for Phoenix/Alerters.
**	19-apr-1991	(teresal)
**	    Modified to only free event if it has been consumed.
*/
VOID
IILQefEvFree( II_LBQ_CB *IIlbqcb )
{
    II_EVENT	*eptr;

    if (((eptr = IIlbqcb->ii_lq_event.ii_ev_list) != (II_EVENT *)0) &&
	(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED)) 
    {
	IIlbqcb->ii_lq_event.ii_ev_list
		= eptr->iie_next;
    	if (eptr->iie_text != (char *)0)
	    MEfree( (PTR)eptr->iie_text );
	eptr->iie_next = (II_EVENT *)0;
	MEfree( (PTR)eptr );
	/* Reset consumed flag */
	IIlbqcb->ii_lq_event.ii_ev_flags &= ~II_EV_CONSUMED;
    }
}
