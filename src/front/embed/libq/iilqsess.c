/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = rs4_us5
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<me.h>
# include	<si.h>		 /* needed for iicxfe.h */
# include	<st.h>		 
# include	<bt.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<generr.h>
# include	<gca.h>
# include	<iirowdsc.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iicx.h>
# include	<iicxfe.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<iilibqxa.h>
# include	<erlq.h>
# include	<adh.h>

/**
+*  Name: iilqsess.c	- Manager of LIBQ state for program's database sessions.
**
**  Description:
**	The routines in this file manage the state information for multiple
**	database sessions within a single application.  The state for a
**	single connection is a dynamically allocated II_LBQ_CB structure.   
**	When a program has multiple connections to the database there
**	will be a list of II_LBQ_CBs.  A new II_LBQ_CB state structure is
**	created each time an application makes a connection to a database.
**	The current II_LBQ_CB is always pointed at by the global IIlbqcb
**	pointer.  
**
**	Information on the list of states is kept in the II_GLB_CB, a global
**	static structure pointed at by the global IIglbcb pointer.  This
**	information is in the form a an IISES_STATE structure.  The
**	II_GLB_CB structure is shared across sessions.
**
**	Note that a multiple-session style of CONNECT statement uses a SESSION
**	clause to distinguish it from a single-session style CONNECT.  By the
**	SESSION clause, the user specifies the session identifer.  The
**	session identifier identifies a session's II_LBQ_CB.
**
**	An application may switch database sessions by using the
**	SET_INGRES statement.  Switching sessions involves searching
**	the II_LBQ_CB list for the session identified by the session-id and
**	setting the IIlbqcb pointer to point at the II_LBQ_CB.
**
**	At the time of disconnecting from the database, the II_LBQ_CB state for
**	the current session is freed and the list is relinked. 
**
**	If the program has used a single-session style of CONNECT, an
**	II_LBQ_CB with a zero session identifier will be created.
**
**  Defines:
**	IILQsidSessID(sess_id)		- Saves the user-specified session id 
**	IILQcnConName(con_name)		- Saves user-specified connection name
**	IILQsnSessionNew(sess_id)	- Creates a session state
**	IILQssSwitchSession(lqp)	- Switches session state to be current
**	IILQfsSessionHandle(sess_id)	- Finds libq ptr by session id.
**	IILQscConnNameHandle(con_name)	- Finds libq ptr by connection name.
**	IILQncsNoCurSession()		- Set state to no current session
**	IILQsdSessionDrop(sess_id)	- Drops session state
**	IILQsepSessionPush()		- Push current session id
**	IILQspSessionPop()		- Pop session id
**	IILQscSessionCompare()		- Compare stack sess id with current
**	IILQpsdPutSessionData()		- Store some session data for a client
**	IILQgsdGetSessionData()		- Get session data for a client.
**	IILQcseCopySessionError()	- Copy session's error state.
-*
**  History
**	18-may-1989	- written (bjb)
**	29-mar-1990 (barbara)
**	    Added routines to push and pop session ids.  A stack of session ids
**	    representing nested "looping" statements is maintained to catch
**	    the error of switching sessions mid-loop.
**	28-aug-1990 (Joe)
**	    Added IILQgsd and IILQpsd for support of multiple session by
**	    the UI module.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160).o***	    This means saving error descriptor information before freeing
**	    II_LBQ_CB in SessionDrop.
**	17-aug-91 (leighb) DeskTop Porting Change: added st.h
**	07-oct-1992 (larrym)
**	    Added IILQncsNoCurSession.  Needed for 4GL programs so they
**	    can return an application to an unconnected state and still
**	    access IL code through it's system session.
**	16-nov-1992 (larrym)
**	    Added support for System Generated Sessions.  If the session 
**	    saved in the global control block == IIsqconSysGen, then we'll
**	    find the next available session id and connect to it.  The user
**	    can find out which session they got with an inquire_sql.  
**      18-jan-1993 (larrym)
**          Added <si.h>.  A modification to iicxfe.h required this
**	02-mar-1993 (larrym)
**	    Modified for XPG4 connections; all sessions are "multi" now.
**	    added IILQcnConName(), IILQscConnNameHandle()
**	15-jun-1993 (larrym)
**	    Renamed IILQssSessionSwitch to IILQfsSessionHandle to more 
**	    accurately reflect what it does.  Wrote new function called
**	    IILQssSwitchSession, which now does the switching.  The code was 
**	    moved from IILQssSetSqlio.  IILQssSetSqlio calls this routine now.
**	    Neither of the session handle functions generates an error,
**	    if the session is not found (it just returns null).  This is now 
**	    the responsibility of the caller.   This allows Disconnect to 
**	    generate it's own message if a user tries to disconnect from a
**	    nonexistent session.  Similarly, IILQssSetSqlio will generate
**	    an error if a session doesn't exist (it used to leave this up
**	    to the old IILQssSessionSwitch).  This was the fix for 50756.
**	    See also entries in iiinqset, iiingres, and iisqcrun.
**      25-Apr-1994 (larrym)
**          Fixed bug 61137, connection failure not getting propigated to
**          correct libq control block.  Modified interface to
**          IILQcseCopySessionError to allow specification of source and
**          destination control blocks.
**	03-jan-1997 (mosjo01)
**	    NO_OPTIM rs4_us5 to resolve IIUIdbdata failure to return user
**	    or dba values.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	30-Apr-99 (gordy)
**	    Don't copy the error code to the default session in
**	    IILQsdSessionDrop().  It is the callers responsibility 
**	    to carry-over information.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-oct-2001 (abbjo03)
**	    In IILQsbnSessionByName(), check that ii_lq_con_name is valid
**	    before calling STequal().
**	05-nov-2001 (abbjo03)
**	    Change IILQsnSessionNew() to protect the setting of session IDs in
**	    heavily concurrent situations.
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

void	IILQcseCopySessionError();
void    IILQncsNoCurSession();

/*{
+*  Name: IILQsidSessID	- Save the user-specified session id.
**
**  Description:
**	A call to this routine is generated by the preprocessor when
**	the program issues a CONNECT or a DISCONNECT statement using the
**	SESSION clause.  This routine saves away the user-specified session
**	id so that it can be used by IIdbconnect when allocating an II_LBQ_CB
**	state to represent the database connection.  A special routine is
**	needed to record the session id because the interface to IIdbconnect
**	can't be changed without requiring all existing programs to
**	repreprocess.  The session id is saved in the II_SES_STATE structure
**	in LIBQ's global control block, pointed at by IIglbcb.
**
**	A session id of zero is reserved for the id of a single-connect
**	session.  However, an error is not given in this routine if a user
**	has specified an id of zero (mainly because it will be "lost"
**	in IIdbconnect when the error number is reset). 
**	NOTE:  This has changed.  We now return an error on an attempt to 
**	connect to session 0.  IIdbconnect now detects this and punts.
**
**  Inputs:
**	session_id	- User defined session number
**
**  Outputs:
**	None.
**
**  Returns:
**	Nothing.
**
**  Preprocessor generated calls:
** 	EXEC SQL CONNECT dbname SESSION 5 IDENTIFIED by username
**	IIsqInit(&sqlca);
**	IILQsidSessID(5);
**	IIsqUser("username");
**	IIsqConnect(0, "dbname", (char *)0);
**
**	EXEC SQL DISCONNECT SESSION 5;
**	IIsqInit(&sqlca);
**	IILQsidSessID(5);
**	IIsqDisconnect();
-*
**  Side Effects:
**	The IISES_STATE element in LIBQ's global control block (IIglbcb)
**	is updated.
**
**  History:
**	18-may-1989 - Written. (bjb)
**	21-mar-1991 - Put in desktop porting change. (teresal)
**	02-mar-1993 (larrym)
**	    Added check for attemtp to connect to session 0, which was never
**	    supported and is now illegal.
*/
VOID
IILQsidSessID(i4 session_id)
{
    II_THR_CB	*thr_cb = IILQthThread();

    if ( session_id )
        thr_cb->ii_th_sessid = session_id;
    else
        thr_cb->ii_th_flags |= II_T_SESSZERO;
}

/*{
+*  Name: IILQcnConName	- Save the user-specified session id.
**
**  Desription:
**	A call to this routine is generated by the preprocessor when
**	the program issues a CONNECT or a DISCONNECT statement using the
**	AS connection_name clause.  This routine saves away the user-specified 
**	connection_name  so that it can be used by IIdbconnect when allocating 
**	an II_LBQ_CB state to represent the database connection.  A special 
**	routine is needed to record the connection name because the interface 
**      to IIdbconnect can't be changed without requiring all existing 
**	programs to repreprocess.  The connection name is saved in the 
**      II_SES_STATE structure in LIBQ's global control block, pointed at by 
**	IIglbcb.
**
**  Inputs:
**	con_name	- user specified connection name.
**
**  Outputs:
**	none.
**
**  Returns:
**	nothing.
**
**  Preprocessor generated calls:
** 	EXEC SQL CONNECT dbname AS 'conname' IDENTIFIED by username
**	IIsqInit(&sqlca);
**	IILQcnConName("conname");
**	IIsqUser("username");
**	IIsqConnect(0, "dbname", (char *)0);
**
**	EXEC SQL DISCONNECT 'conname';
**	IIsqInit(&sqlca);
**	IILQcnConName("conname");
**	IIsqDisconnect();
-*
**  Side effects:
**	The IISES_STATE element in LIBQ's global control block (IIglbcb)
**
**  History:
**	02-mar-1993 (larrym)
**	    written.
**
*/

VOID
IILQcnConName( char *con_name )
{
    II_THR_CB *thr_cb = IILQthThread();

    STlcopy( con_name, thr_cb->ii_th_cname, LQ_MAXCONNAME );
    thr_cb->ii_th_flags |= II_T_CONNAME;

    return;
}

/*{
+*  Name: IILQfsSessionHandle	- Find libq session CB by session id.
**
**  Description:
**	When a program issues a SET_INGRES statement to switch between
**	database sessions, this routine is called to search the list
**	of II_LBQ_CB structures which represent the state of the individual
**	database sessions.  The list of states is accessed via
**	the IISES_STATE element in the global LIBQ control block
**	pointed at by IIglbcb.  If a state is found whose identifier
**	matches the input session_id, a pointer to that state is returned.
**	If the state is not found, null is returned.
**
**	This function should not generate any error messages.  It is desinged
**	to be used by other functions that want to know if a given session
**	exists (and if so, presumably they want to switch to it).  Any
**	desired error generation will be handled by the caller.  More 
**	specifically, this function was written to allow DISCONNECT sess_id
**	to only generate one error (bad disconnect) when it tries to first
**	switch to then disconnect from a user specified session.
**
**  Inputs
**	session_id	- User defined session identifier
**
**  Outputs:
**	None
**
**  Returns:
**	A pointer to the switched session state (II_LBQ_CB) or null.
**
**  Side effects:
**	None.
-*
**  History:
**	18-may-1989 - Written (bjb)
**	02-dec-1992 (larrym)
**	    Added XA support.  An XA app has it's own concept of what a 
**	    session_id is, and so when it wants to switch to sessions 
**	    we need to map it's xa_session_id to an ingres_session_id,
**	    then procede with the switch.
**	02-mar-1993 (larrym)
**	    if user tries to switch to session 0, return error.
**	03-mar-1993 (larrym)
**	    modified XA support - XA now uses IILQscConnNameHandle.
**	28-may-1993 (larrym)
**	    changed name to IILQfsSessionHandle from IILQssSessionSwitch,
**	    'cause it really doesn't switch at all.  The function that
**	     switches is IILQssSwitchSession.
*/

II_LBQ_CB *
IILQfsSessionHandle(i4 session_id)
{
    II_LBQ_CB	*lqp = NULL;
    char	ebuf[20];


    if ( ! session_id )
    {
	IIlocerr( GE_LOGICAL_ERROR, E_LQ00BC_SESSZERO, II_ERR, 0, NULL );
	return( NULL );
    }

    if (IIXA_CHECK_IF_XA_MACRO)
    {
	/* it would be nice to do some tracing here */
	if (IIXA_CHECK_SET_INGRES_STATE(IICX_XA_T_ING_INTERNAL))
	{
	    /* check_set_ingres_state returns the error error CHECK */
	    return NULL;
	}
    }

    if ( MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) == OK )
    {
	for( lqp = IIglbcb->ii_gl_sstate.ss_first; lqp; lqp = lqp->ii_lq_next )
	    if ( lqp->ii_lq_sid == session_id )  break;
    
	MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
    }

    return( lqp );
}

/*{
+*  Name: IILQsnSessionNew	- Create an II_LBQ_CB state in a list.
**
**  Desription:
**	This routine is called from IIdbconnect.  It searches the
**	list of session states to verify that the passed-in session id
**	is unique.  (If not an error is issued and the routine returns.)  
**	Or the user may have asked us to generate a session number for them.
**	A new session state is then allocated and placed at the head of
**	the list of session states.  The session list is kept track of in
**	an IISES_STATE element in LIBQ's global state structure, pointed
**	at by IIglbcb.  After the state has been created, the IISES_STATE
**	element is updated to reflect the new state at the top of the list.
**
**  Inputs:
**	*session_id	- pointer to User defined session identifier
**
**  Outputs:
**	None
**
**  Returns:
**	TRUE	- II_LBQ_CB state was successfully created.
**	FALSE	- II_LBQ_CB state was not created.
**
**  Notes:
**	A word about how we generate a session id.  We assume that for the 
**	most part, a user's session IDs will be < 64.  So we have a bit
**	array that is 64 bits wide, and we go though all the IIlbqcb's, 
**	setting bit n for session id n (we start off with session 0 set since
**	session 0 is no longer a valid session id).  When we're
**	done, we BTnot the bit array, and then look for the first set bit.
**	That is the next available session id.  E.g, if we're connected to
**	sessions 1, 2, and 4, the bit array looks like:
**		bit: ...0 0 0 1 0 1 1 1
**		ses: ...7 6 5 4 3 2 1 0
**	After we BTnot, it looks like
**		bit:	1 1 1 0 1 0 0 0
**		ses:    7 6 5 4 3 2 1 0
**	And a BTnext will return 3, as the first bit set.
**	It get's a little more complicated dealing with the possibility that
**	A user might have 65 sessions going and wants another one.  What we
**	do then is to shift everything over 64 and do it again.  That is,
**	on the first round, we only set the bit array for session numbers in 
**	the range of 0 - 63 (with 0 being a special case).  If BTnext returns
**	a -1, then no sessions were available in that range, so we add 64 to
**	our shifter, and loop again, looking for sessions in the range of 64
**	to 127.  If BTnext then returns a positive number, we add the value
**	of the shifter to it (+64) and that is our new session.  This can
**	continue all the way up to MAXI4.
**
**  Side effects:
**	The IISES_STATE element in LIBQ's global control block (IIglbcb)
**	is updated.
-*
**  History:
**	18-may-1989 - Written (bjb)
**	16-nov-1992 (larrym)
**	    Added support for System Generated Sessions.  If the session 
**	    saved in the global control block == 0, then we'll
**	    find the next available session id and connect to it.  The user
**	    can find out which session they got with an inquire_sql.  
**	    Changed the interface - session_id is now a pointer.   If user
**	    requested a system generated session we set the result in 
**	    *session_id.
**	14-mar-1993 (larrym)
**	    if the session id is 0 we also want to generate a session id.
**	    (for the new connection name stuff).  Using -98 will generate
**	    both a session_id and a connection name.
**	05-nov-2001 (abbjo03)
**	    Protect the setting of session IDs in heavily concurrent situations
**	    by holding the semaphore throughout the function.
*/
II_LBQ_CB *
IILQsnSessionNew(
i4	*session_id)
{
    register II_LBQ_CB	*lqp;	/* Pointer to an II_LBQ_CB list element */
    char 	ebuf[20];
    char	ses_vector[IISESVECTORSIZE]; /* bit array for ses id's */
    i4		ses_shift;		     /* ses_id range */
    i4		gen_ses;		     /* generated session */
    i4		i;		/* temp counter */

    if (MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem) != OK)
	return NULL;

    if (*session_id == 0 || *session_id == IIsqconSysGen)
    {
        /* System Gen'd session */
	ses_shift = 0;
	gen_ses = 0;
	/* initialize session vector (i.e., clear all the bits) */
	for (i=0; i < IISESVECTORSIZE; i++) ses_vector[i] = (char)0;
	/* 
	** Always set session 0: assume multi session semantics.
	** This also saves us if IIglbcb->ii_gl_sstate.ss_first == NULL,
	** as BTnot will set the vector to 1...110, and BTnext will tell
	** us that session 1 is the first available.
	*/
	BTset(0, ses_vector);

	/* loop till we find one or shift to the max */
	while( gen_ses <= 0  &&  ses_shift < (MAXI4 - IISESSHIFT) )
	{
	    for (lqp = IIglbcb->ii_gl_sstate.ss_first; lqp;
		    lqp = lqp->ii_lq_next)
		if (lqp->ii_lq_sid >= ses_shift &&
			lqp->ii_lq_sid < ses_shift + IISESSHIFT)
		    BTset(lqp->ii_lq_sid-ses_shift, ses_vector);

	    BTnot(64, ses_vector);

	    gen_ses = BTnext((i4)-1, ses_vector, IISESSHIFT);
	    /* 
	    ** if gen_ses is -1, that means all bits are off, so no need to  
	    ** clear it for next round.
	    */
	    if( gen_ses >= 0 )
	    {
		/* found an available session.  Add the shift amount to it */
		gen_ses += ses_shift;
	    }
	    else 
	    {
	       /* didn't find one.  Shift again */
	       ses_shift += IISESSHIFT;
	    }
	}
	if (gen_ses <= 0) 
	{
	    /* 
	    ** we looked all the way up to MAXI4 and couldn't find an open
	    ** session.  If we're here, something is terribly wrong.
	    */
	    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem);
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ000F_SESLIM, II_ERR, 0, (char *) 0);
	    return( NULL );
	}
	else
	{
	    *session_id = gen_ses ;
	}
    }
    else
    {
	/* user specified session.  See if it already exists. */
	if (IIXA_CHECK_IF_XA_MACRO)
	{
	    /* it would be nice to do some tracing here */
	    if (IIXA_CHECK_SET_INGRES_STATE(IICX_XA_T_ING_INTERNAL))
	    {
		MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem);
		/* check_set_ingres_state returns the error error CHECK */
		return NULL;
	    }
	}
	for (lqp = IIglbcb->ii_gl_sstate.ss_first; lqp; lqp = lqp->ii_lq_next)
	    if (lqp->ii_lq_sid == *session_id)
	    {
		MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem);
		/* session already exists */
		CVna(*session_id, ebuf);
		IIlocerr(GE_LOGICAL_ERROR, E_LQ00BA_SESSDUP, II_ERR, 1, ebuf);
		return NULL;
	    }
    }

    if ((lqp = (II_LBQ_CB *)MEreqmem((u_i4)0, (u_i4)sizeof(II_LBQ_CB),
	TRUE, (STATUS *)NULL)) == NULL)
    {
	MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem);
	CVna( *session_id, ebuf );
	IIlocerr(GE_NO_RESOURCE, E_LQ00BB_SESSALLOC, II_ERR, 1, ebuf); 
	return( NULL );
    }

    lqp->ii_lq_sid = *session_id;

    if (!IIglbcb->ii_gl_sstate.ss_tot)
	IIcgc_init(IIhdl_gca, &IIglbcb->ii_gl_sstate.ss_gchdrlen);

    lqp->ii_lq_next = IIglbcb->ii_gl_sstate.ss_first;
    IIglbcb->ii_gl_sstate.ss_first = lqp;
    IIglbcb->ii_gl_sstate.ss_tot++;

    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );

    return( lqp );
}

/*{
+*  Name: IILQsbySessionByName
**
**  Description:
**	Search sessions for matching connection name.  It is 
**	possible to specify a state to be ignored during the
**	search.  This permits looking for duplicates after a 
**	session has been created (which may required if the 
**	connection name can not be generated until after the 
**	session is created).
**
**  Inputs
**	con_name	Connection name 
**	ignore		Session control block to ignore, may be NULL.
**
**  Outputs:
**	None
**
**  Returns:
**	A pointer to the session (II_LBQ_CB) or null.
**
**  Side effects:
**	None.
-*
**  History:
**	31-oct-2001 (abbjo03)
**	    The II_LBQ_CB may not be completely filled when we are called. Check
**	    that ii_lq_con_name is valid before calling STequal().
*/

II_LBQ_CB *
IILQsbnSessionByName( char *con_name, II_LBQ_CB *ignore )
{
    II_LBQ_CB	*lqp = NULL;

    if ( con_name  &&  *con_name  &&
	 MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) == OK )
    {
	for( 
	     lqp = IIglbcb->ii_gl_sstate.ss_first; 
	     lqp; 
	     lqp = lqp->ii_lq_next 
	   )
	    if (lqp != ignore && lqp->ii_lq_con_name &&
			STequal(lqp->ii_lq_con_name, con_name))
		break;

	MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
    }

    return( lqp );
}

/*{
+*  Name: IILQscConnNameHandle	- Get Handle of II_LBQ_CB by conn name
**
**  Description:
**	When a program issues a SET_INGRES statement to switch between
**	database sessions, this routine is called to search the list
**	of II_LBQ_CB structures which represent the state of the individual
**	database sessions.  The list of states is accessed via
**	the IISES_STATE element in the global LIBQ control block
**	pointed at by IIglbcb.  If a state is found whose identifier
**	matches the input con_name, a pointer to that state is returned.
**	If the state is not found, null is returned.
**
**  Inputs
**	con_name	- User defined connection name 
**
**  Outputs:
**	None
**
**  Returns:
**	A pointer to the switched session state (II_LBQ_CB) or null.
**
**  Side effects:
**	None.
-*
**  History:
**	03-mar-1993 (larrym)
**	    written.
*/

II_LBQ_CB *
IILQscConnNameHandle( char *con_name )
{
    II_LBQ_CB	*lqp = NULL;

    if (!(IIXA_CHECK_IF_XA_MACRO))		/* normal session switch */
	lqp = IILQsbnSessionByName( con_name, NULL );
    else
    {
	/* special XA session switch */

	/* ((extra parens needed 'cause it's a macro)) */
	if (IIXA_CHECK_SET_INGRES_STATE((
		IICX_XA_T_ING_INTERNAL |
		IICX_XA_T_ING_ACTIVE_PEND |
		IICX_XA_T_ING_ACTIVE)) != E_DB_OK)
	{
	    /* check_set_ingres_state returns the error error CHECK */
	}
	else
	{
	    /* for now, xa internally identifies sessions by session id */
	    i4      session_id;
	    /* Get the INGRES session ID from the supplied XA_session_id */
	    if (IIXA_GET_AND_SET_XA_SID(con_name, &session_id) != E_DB_OK)
	    {
		/* error returned by caller */
	    }
	    else
	    {
		i4	old_ingres_state;

		IIXA_SET_INGRES_STATE( 
		    IICX_XA_T_ING_INTERNAL, 
		    &old_ingres_state);
	        lqp = IILQfsSessionHandle(session_id);
		/* need to pop INGRES_STATE */
		IIXA_SET_INGRES_STATE( old_ingres_state, &old_ingres_state);
	    }
	}
    }

    return( lqp );
}

/*{
+*  Name: IILQssSwitchSession	- Make another session the current one.
**
**  Description:
**	This is the routine that actually makes a different session your
**	current one.  It does this by setting the current session pointer
**	(a.k.a, IIlbqcb) to point to the session control block you pass it.
**	The session control block is of type II_LBQ_CB.   This functionality
**	was previously in iiinqset.c as part of the SET_SQL statement.
**
**  Inputs
**	thr_cb		Thread-local-storage control block.
**      IIlbqcb	        Session control block to be made current.
**
**  Outputs:
**      None
**
**  Returns:
**      Nothing.
**
**  Side effects:
**      None.
-*
**  History:
**      28-may-1993 (larrym)
**          written.
*/
STATUS
IILQssSwitchSession( II_THR_CB *thr_cb, II_LBQ_CB *IIlbqcb )
{
    STATUS	status = OK;

    /*
    ** Validate requested session.  If NULL or 
    ** default session, set no current session 
    ** (switch to default).  If same as current, 
    ** do nothing.
    */
    if ( ! IIlbqcb  ||  IIlbqcb == &thr_cb->ii_th_defsess )
	IILQncsNoCurSession();
    else  if ( thr_cb->ii_th_session != IIlbqcb )
    {
	if ( IIlbqcb->ii_lq_flags & II_L_CURRENT  ||
	     MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) != OK )
	{
	    /*
	    ** Cannot switch to session which is already current.
	    */
	    IIlocerr( GE_LOGICAL_ERROR, E_LQ00C9_SESS_ACTIVE, II_ERR, 0, NULL );
	    status = FAIL;
	}
	else
	{
	    /*
	    ** Release current session and activate requested session.
	    */
	    thr_cb->ii_th_session->ii_lq_flags &= ~II_L_CURRENT;
	    thr_cb->ii_th_session = IIlbqcb;
	    IIlbqcb->ii_lq_flags |= II_L_CURRENT;

	    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );

	    /*
	    ** Set the tracing state for the session selected.
	    */
	    IILQgstGcaSetTrace( IIlbqcb, IITRC_SWITCH );
	    IILQsstSvrSetTrace( IIlbqcb, IITRC_SWITCH );
	    IIdbg_toggle( IIlbqcb, IITRC_SWITCH );
	}
    }

    return( status );
}

/*{
+*  Name: IILQncsNoCurSession	- set state to be no current session.
**
**  Description:
**	This function set's the session state to "no current connection",
**	similar to the state achieved by a DISCONNECT statement.  It is needed
**	when a user's 4GL application has just disconnected from a session, 
**	and then did something that requires our tools to switch to an
**	internal (negative) session.  The tools needs to restore the application
**	to its disconnected state.
**
**  Inputs
**	None.
**
**  Outputs:
**	None
**
**  Returns:
**	Void.
**
**  Side effects:
**	Modifies IIlbqcb
-*
**  History:
**	07-oct-1992 (larrym)
**		written.
*/

VOID
IILQncsNoCurSession()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb;

    /*
    ** Release current session.
    */
    thr_cb->ii_th_session->ii_lq_flags &= ~II_L_CURRENT;

    /* 
    ** Point at default session and 
    ** initialize fields for safety 
    */
    IIlbqcb = thr_cb->ii_th_session = &thr_cb->ii_th_defsess;
    IIlbqcb->ii_lq_curqry = II_Q_DEFAULT;
    IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;
    IIlbqcb->ii_lq_flags = 0;

    return;
}


/*{
+*  Name: IILQsdSessionDrop	- Free the current session state
**
**  Description:
**	This routine is called from IIexit immediately after the program
**	has disconnected a session.  It frees the II_LBQ_CB and all its
**	associated structures for the current database session and relinks
**	the list of II_LBQ_CBs. 
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	session_id	- The id of the II_LBQ_CB session to be dropped.
**
**  Outputs:
**	None
**
**  Returns:
**	Nothing
**
**  Side effects:
-*
**  History:
**	18-may-1989 - Written (bjb)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160).
**	    Before freeing the current IIlbqcb, save error state in
**	    default session CB.  This allows errors during connection 
**	    or disconnection to be accessed via INQUIRE_INGRES.  
**	01-24-1991 (kathryn) 
**	    Call IIqid_free() to free memory of ii_lq_qryids. 
**	06-jan-1993 (teresal)
**	    Free the ADF Control Block for this session.
**	02-mar-1993 (larrym)
**	    Free the connection_target.
**	30-Apr-99 (gordy)
**	    Don't copy the error code to the default session.  It is
**	    the callers responsibility to carry-over information.
**	26-Oct-2004 (gupsh01)
**	    Added code to release the memory allocated for unicode
**	    collation. 
*/

VOID
IILQsdSessionDrop( II_THR_CB *thr_cb, i4  session_id )
{
    II_LBQ_CB	*lqp, *xlqp;
    DB_STATUS	stat;

    if ( MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem ) != OK )
        return;

    if ( (lqp = IIglbcb->ii_gl_sstate.ss_first) )
    {
	/* Special case first element */
	if ( lqp->ii_lq_sid == session_id )
	{
	    IIglbcb->ii_gl_sstate.ss_first = lqp->ii_lq_next;
	    lqp->ii_lq_next = NULL;
	}
	else
	{
	    xlqp = lqp;
	    for (lqp = lqp->ii_lq_next; lqp; 
		 lqp = lqp->ii_lq_next, xlqp = xlqp->ii_lq_next)
	    {
		if (lqp->ii_lq_sid == session_id)
		{
		    xlqp->ii_lq_next = lqp->ii_lq_next;
		    lqp->ii_lq_next = NULL;
		    break;
		}
	    }
	}

	if ( lqp  &&  ! --IIglbcb->ii_gl_sstate.ss_tot )
	{
	    IIdbg_toggle( lqp, IITRC_RESET );
	    IILQgstGcaSetTrace( lqp, IITRC_RESET );
	    IILQsstSvrSetTrace( lqp, IITRC_RESET );
	    IIcgc_term();
	}
    }

    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );

    if ( lqp )
    {
	IIcsAllFree( lqp );
	IIqid_free( lqp );

	if ( lqp->ii_lq_gca )  
	    IIcgc_free_cgc( lqp->ii_lq_gca );
	if (lqp->ii_lq_dbg != NULL)
	    MEfree( (PTR)lqp->ii_lq_dbg );
	if (lqp->ii_lq_retdesc.ii_rd_desc.rd_gca != NULL)
	    MEfree( (PTR)lqp->ii_lq_retdesc.ii_rd_desc.rd_gca );
	if (lqp->ii_lq_retdesc.ii_rd_desc.rd_names != NULL)
	    MEfree( (PTR)lqp->ii_lq_retdesc.ii_rd_desc.rd_names );
	if (lqp->ii_lq_parm.ii_sp_buf != NULL)
	    MEfree( (PTR)lqp->ii_lq_parm.ii_sp_buf );
	if (lqp->ii_lq_cdata != NULL)
	    MEfree( (PTR)lqp->ii_lq_cdata);
	if (lqp->ii_lq_error.ii_er_msg != (char *)0)
	    MEfree( (PTR)lqp->ii_lq_error.ii_er_msg );
	if (lqp->ii_lq_error.ii_er_submsg != (char *)0)
	    MEfree( (PTR)lqp->ii_lq_error.ii_er_submsg );
	if (lqp->ii_lq_error.ii_er_smsg != (char *)0)
	    MEfree( (PTR)lqp->ii_lq_error.ii_er_smsg );
	if (lqp->ii_lq_adf != NULL)
	{
	    if (lqp->ii_lq_ucode_init)
	    {
	      if (lqp->ii_lq_ucode_ctbl)
	      { 
		MEfree ((PTR)lqp->ii_lq_ucode_ctbl);
		lqp->ii_lq_ucode_ctbl = NULL;
	      }	

	      if (lqp->ii_lq_ucode_cvtbl)
	      { 
		MEfree ((PTR)lqp->ii_lq_ucode_ctbl);
		lqp->ii_lq_ucode_ctbl = NULL;
	      }	
	      lqp->ii_lq_ucode_init = 0; 
	      stat = adg_clean_unimap();
	    }
	    MEfree( (PTR)lqp->ii_lq_adf );
	}
	if (lqp->ii_lq_con_target != NULL)
	    MEfree( (PTR)lqp->ii_lq_con_target );
	if ( lqp->ii_lq_con_name )
	    MEfree( (PTR)lqp->ii_lq_con_name );

	MEfree( (PTR)lqp );
    }

    return;
}




/* 
** Name: IILQucolinit - Initializes the collation table for adf_cb in
**			II_LBQ_CB for a session. 
**
** Description:
**
**	This function initializes the unicode collation table for the 
**	ADF control block in II_LBQ_CB. This should only be called if
**	unicode coercion is need in the front end.
**
** Input:
**
**	II_LBQ_CB	Libq control block which holds the initiated 
**			values
** Output:
**	IIlbqcb->ii_lq_ucode_ctbl	The fixed sized unicode coercion table 
**					is initiated.
**      IIlbqcb->ii_lq_ucode_cvtbl	The variable sized vtable 
**					is initiated.
** Returns:
**
**      E_DB_ERROR			if aduucolinit fails.
**      E_DB_OK				if successful.
**
** History:
**	16-Jun-2005 (gupsh01)
**	    Moved from adhembed and renamed.
*/
STATUS 
IILQucolinit(i4 setnfc)
{
    STATUS stat = OK;
    II_THR_CB *thr_cb = IILQthThread();
    II_LBQ_CB *IIlbqcb = thr_cb->ii_th_session;


    stat = adh_ucolinit(IIlbqcb, setnfc);

    return stat;
}

/*
** Name:	IILQsepSessionPush - Push a session id on a stack.
**
** Description:
**	When a loop-structured statement (e.g. a select loop or a
**	database procedure execution loop) is initiated, this routine
**	pushes the current session id on to a stack.  Later, when the 
**	loop is iterating, a comparison is made between the current 
**	session id and the session id on the stack.  If the comparison 
**	fails, an error is issued and the loop exits.
** 
**	Note that the first element on the stack is static, ie. not
**	allocated and not freed.
**
** Inputs:
**	thr_cb		Thread-local-storage control block.
** Outputs:
**	Nothing.
** Returns:
**	Void.
**
** History
**	29-mar-1990	(barbara)
**	    Written to fix bug 9159.
*/
VOID
IILQsepSessionPush( II_THR_CB *thr_cb )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILOOP_ID	*loop_elm, *lpp;

    if ( thr_cb->ii_th_loop.lp_sid == 0 )       /* First loop? */
    {
	thr_cb->ii_th_loop.lp_sid = IIlbqcb->ii_lq_sid;
    }
    else                                        /* Must be nested */
    {
	if ((loop_elm = (IILOOP_ID *)MEreqmem((u_i4)0,
			(u_i4)sizeof(IILOOP_ID), TRUE, (STATUS *)0))
			== (IILOOP_ID *)0)
	{
	    IIlocerr(GE_NO_RESOURCE, E_LQ00C1_LOOP_ALLOC, II_ERR, 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    IIqry_end(); 
	    return;    
	}
	loop_elm->lp_next = (IILOOP_ID *)0;
	loop_elm->lp_sid = IIlbqcb->ii_lq_sid;
	for (lpp = &thr_cb->ii_th_loop; lpp->lp_next; lpp = lpp->lp_next);
	lpp->lp_next = loop_elm;
    }
}

/*
** Name:	IILQspSessionPop - Pop a session id from a stack.
**
** Description:
**	The session id pushed upon entering a loop-structured statement
**	(eg. a select loop or a database procdure execution loop) is
**	popped upon exit from the loop.
**
**	Note that the first element on the stack is static, ie. not
**	allocated and not freed.
**
** Inputs:
**	thr_cb		Thread-local-storage control block.
** Outputs:
**	None.
** Returns:
**	VOID.
**
** History:
**	29-mar-1990	(barbara)
**	    Written to fix bug 9159.
*/

VOID
IILQspSessionPop( II_THR_CB *thr_cb )
{
    IILOOP_ID   *lpp, *sv_lpp;

    for( lpp = sv_lpp = &thr_cb->ii_th_loop; lpp->lp_next; )
    {
    	sv_lpp = lpp;
    	lpp = lpp->lp_next;
    }
    if (lpp != sv_lpp)
    {
	lpp->lp_next = (IILOOP_ID *)0;
	MEfree((PTR)lpp);
	sv_lpp->lp_next = (IILOOP_ID *)0;
    }
    else
    {
    	lpp->lp_sid = 0;
    }
}

/*
** Name:	IILQscSessionCompare	- Compare stacked sid with current sid
**
** Description:
**	Compare the session id on the top of LIBQ's global stack of session
**	ids with the current session.  This comparison is made in each
**	iteration of a loop-structured statement (e.g., SELECT loops,
**	database procedure execution loops).  If the current session id
**	doesn't match the id saved at the start of the loop, then the
**	application must have switched sessions mid-loop, or returned
**	into the middle of a loop without switching back to the correct
**	session.  In either case the program is attempting to read 
**	another session's data, and an error is issued.  If this routine
**	returns FALSE, the caller should break out of the loop and flush
**	the data.
**
** Inputs:
**	thr_cb		Thread-local-storage control block.
** Outputs:
**	None.
** Returns:
**	TRUE	- Current sess id matches id of most recently started loop.
**	FALSE	- Current sess id does not match.
**
** History:
**	03-apr-1990	(barbara)
**	    Written to fix bug 9159.
*/
i4
IILQscSessionCompare( II_THR_CB *thr_cb )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILOOP_ID   *lpp;

    for (lpp = &thr_cb->ii_th_loop; lpp->lp_next; lpp = lpp->lp_next);

    if (IIlbqcb->ii_lq_sid != lpp->lp_sid)
    {
	IIlocerr( GE_LOGICAL_ERROR, E_LQ00C2_SESS_LOOP, II_ERR, 2,
		    &lpp->lp_sid, &IIlbqcb->ii_lq_sid );
	return FALSE;
    }
    return TRUE;
}

/*
** Name:	IILQpsdPutSessionData	- Store client data with a session.
**
** Description:
**	Associate some client data with the currently executing
**	DBMS session and the given client.
**	
**	Repeat calls by the same client will cause the stored data
**	for the current session to get changed.  The data previously
**	stored for the session will be freed.
**
**	Currently only a single client is supported.  This means the
**	first client to call IILQpsdPutSessionData for a session will be
**	the only client that has data stored for it.
**
** Inputs:
**	client_id		This is the id of the client.  This is
**				assumed to be the class code of the
**			        client (i.e. the number for the two letter
**			        directory code - e.g. _UI_CLASS).
**				The client_id is used to identify the client
**				in subsequent IILQgsdGetSessionData calls.
**				It is up to the various LIBQ clients to make
**				sure the names are unique across the clients.
**
**	client_data		This is a PTR value that will be associated
**				with the current session, and will be returned
**				by calls to IILQgsdGetSessionData when the
**				current session is executing and the given
**				client_id is passed.  The PTR value
**				must be NULL, or it must point to memory
**				that can be freed by MEfree.  After this call
**				the memory pointed to be client_data is
**				assumed to be owned by LIBQ, and LIBQ will
**				free the memory when the current session
**				is closed.
**
** Outputs:
**	Returns:
**		A status value.  OK means that client_data is now
**		associated with the current session.  Calls to 
**		IILQgsdGetSessionData will now return client_data whenever
**		the current session is executing.
**		a failure code means that no data could be stored for the
**		current session.  In this case, subsequent calls to
**		IILQgsdGetSessionData when this session is executing
**		will return NULL.
**
** Side Effects:
**	The data pointed to by client_data is now the responsibility of
**	LIBQ.  It will free this data when the current session is
**	closed.  Callers should not maintain copies of the PTR value
**	passed to client_data.
**
** History
**	28-aug-1990 (Joe)
**	    First Written
*/
STATUS
IILQpsdPutSessionData(client_id, client_data)
i4	client_id;
PTR	client_data;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( ! IIlbqcb  ||  ! (IIlbqcb->ii_lq_flags & II_L_CONNECT) )
	return( FAIL );

    if (IIlbqcb->ii_lq_cdata != NULL && IIlbqcb->ii_lq_cid != client_id)
    {
	return FAIL;
    }
    if (IIlbqcb->ii_lq_cdata != NULL)
    {
	MEfree(IIlbqcb->ii_lq_cdata);
    }
    IIlbqcb->ii_lq_cdata = client_data;
    IIlbqcb->ii_lq_cid = client_id;
    return OK;
}

/*
** Name:	IILQgsdGetSessionData	- Store client data with a session.
**
** Description:
**	Get the client data associated with the currently executing
**	session for a particular client.  This will return the PTR
**	value given on the last call to IILQpsdPutSessionData for
**	this session and this client.  Remember, this can be NULL in
**	some cases.
**
** Inputs:
**	client_id		This is the id of the client to get the
**				data for.
**
** Outputs:
**	Returns;                The PTR value associated with the current
**				session for the given client.  This can
**				be NULL if that is the PTR value given
**				by the last IILQpsdPutSessionData call
**				for this session and client, or if there
**				was an error on that call.
**				The memory pointed to by this PTR value
**				is only valid for the life of the DBMS
**				session it is associated with.  Callers
**				should not hold copies of this pointer
**				value for long periods of time unless
**				they know the session will still be
**				active.
**
** History
**	28-aug-1990 (Joe)
**	    First Written
*/
PTR
IILQgsdGetSessionData( i4 client_id)
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( ! IIlbqcb  ||  ! (IIlbqcb->ii_lq_flags & II_L_CONNECT)  ||
	 IIlbqcb->ii_lq_cid != client_id )
	return( NULL );

    return( IIlbqcb->ii_lq_cdata );
}

/*
** Name:	IILQcseCopySessionError	- Save error information 
**
** Description:
**	Copy error-descriptor fields from current II_LBQ_CB structure
**	to a static structure.  Before an II_LBQ_CB is freed (at
**	disconnection time or if an attempted connection fails) the
**	error information must be saved so that applications can
**	access it through INQUIRE_INGRES.  The information is saved
**	in a static II_LBQ_CB to which IIlbqcb is always set when the
**	application is not currently in a valid session.
**
**	Currently the following fields are copied:
**	    ii_er_num		-- Error number
**	    ii_er_other		-- Local error number
**	    ii_er_estat		-- Error severity (not yet used)
**	    ii_er_sbuflen	-- Length of message buffer
**	    ii_er_smsg		-- Error mesage
**
**	Other members of the error descriptor, such as user messages (from
**	database procedures) are not saved.  There should be nothing useful
**	in these fields at connect and disconnect time.
**
** Inputs:
**	None.
** Outputs:
**	None.
** Returns:
**	Nothing.
**
** History:
**	03-dec-1990	(barbara)
**	    Written as part of fix to bug 9160.
**      25-Apr-1994 (larrym)
**          Fixed bug 61137, connection failure not getting propigated to
**          correct libq control block.  Modified interface to
**          IILQcseCopySessionError to allow specification of source and
**          destination control blocks.
*/

void
IILQcseCopySessionError( II_ERR_DESC *sedesc, II_ERR_DESC *dedesc)
{
    i4	len;

    dedesc->ii_er_num = sedesc->ii_er_num;
    dedesc->ii_er_other = sedesc->ii_er_other;
    dedesc->ii_er_estat = sedesc->ii_er_estat;
    if (sedesc->ii_er_smsg && sedesc->ii_er_smsg[0] != '\0')
    {
	len = STlength(sedesc->ii_er_smsg);
	if (dedesc->ii_er_sbuflen < len)
	{
	    if (dedesc->ii_er_sbuflen != 0)
		MEfree(dedesc->ii_er_smsg);
	    if ((dedesc->ii_er_smsg = (char *)MEreqmem((u_i4)0, (u_i4)(len+1),
		TRUE, (STATUS *)NULL)) == NULL)
	    {
		dedesc->ii_er_smsg = (char *)0;
		dedesc->ii_er_smsg = 0;
	    }
	    dedesc->ii_er_sbuflen = len;
	}
	if (dedesc->ii_er_smsg)
	    STlcopy( sedesc->ii_er_smsg, dedesc->ii_er_smsg, len );
    }
}
