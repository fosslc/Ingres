/*
** Copyright (c) 1987, 2010 Ingres Corporation
** All rights reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <clconfig.h>
#include    <systypes.h>
# if !defined (NT_GENERIC)
#include    <sys/ipc.h>
#include    <sys/msg.h>
#include    <syslog.h>
# endif
#ifdef xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */
#ifdef xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif /* xCL_007_FILE_H_EXISTS */
#include    <errno.h>
#include    <me.h>
#include    <tr.h>
#ifdef NT_GENERIC
#include    <meprivate.h>   /* for key_t */
#include    <tchar.h>
#include    <lmcons.h>
#include    <lmmsg.h>       /* for NetMessage functions */
#include    <lmerr.h>
#include    <winnls.h>      /* for MultiByteToWide function */
#include    <ingmsg.h>
#endif /* NT_GENERIC */
#include    <er.h>
#include    <lo.h>
#include    <nm.h>
#include    <pm.h>
#include    <st.h>
#include    <pm.h>
#ifdef	xCL_074_WRITEV_READV_EXISTS
#include    <sys/uio.h>
#endif
#include    <cv.h>

/**
**
**  Name: ER.C - Implements the ER compatibility library routines.
**
**  Description:
**      The module contains the routines that implement the ER logging
**	portion	of the compatibility library.
**
**          ERlog     - Send message to errlog.log
**          ERsend    - Send message to errlog.log or system log
**          ERreceive - Receive message from  ERsend    - Now Obsolete
**
**
**  History:    
**      02-oct-1985 (derek)    
**          Created new for 5.0.
**	18-Jun-1986 (fred)
**	    Changed location of errmsg.msg.  Now, if xDEV_TEST is defined,
**	    ERlookup() will look for the errmsg.msg in jpt_files:errmsg.msg
**	    as opposed to simply looking in the current directory.  Users
**	    who wish to keep their own copy of errmsg.msg may, of course,
**	    locally redefine jpt_files to be where they wish it to be.
**	17-Sep-86 (ericj)
**	    Changed null-terminated string formatting in ERlookup().
**	13-mar-87 (peter)
**	    Kanji integration. ERlookup and ERreport are now in
**	    separate files.
**	06-Aug-1987 (fred)
**	    Change name of log file from ii_files:errlog.log to
**	    II_CONFIG:errlog.log.
**	03-Dec-1987 (anton)
**	    Reworked for UNIX.
**	28-Feb-1989 (fredv)
**	    Added ifdef xCL... around <sys/file.h>
**	    Include <fcntl.h> and <systypes.h>
**	15-may-90 (blaise)
**	    Integrated changes from ug : added logic for "writev".
**      28-sep-1992 (pholman)
**          Following CL committee of 19-sep-1992, mark ERsend and
**          ERreceive as obsolete, and introduce ERlog (which is
**          similar to ERsend before the new parameter was added.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing include
**	13-may-95 (emmag)
**	    sys/ipc.h and sys/msg.h don't exist on NT.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.  
**	03-mar-1997 (canor01)
**	    Enable the ER_OPER_MSG flag to ERsend().  Log messages
**	    with this flag both to the errlog and the system log.
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	25-jun-1997 (canor01)
**	    Extend the functionality of ER_OPER_MSG on Windows NT.
**	    In addition to writing the message to the error log and
**	    to the Application Event Log, it will also send a message
**	    to the messenger service on the server machine, which will
**	    pop up a message box with the error text in it.
**	3-jul-1997 (canor01)
**	    Previous change had an NT-specific 'wchar_t' that had
**	    migrated out of the NT_GENERIC block.
**	25-jul-1997 (canor01)
**	    Previous change would also not run under Windows'95.  Add
**	    test for Windows OS and skip the call to the messenger
**	    service if it's Windows'95.
**	10-apr-98 (mcgem01)
**	    Product name change from OpenIngres to Ingres.
**	13-aug-1998 (canor01)
**	    Add username for Rosetta.
**	31-jul-1998 (canor01)
**	    On NT, use GetComputerName() instead of GChostname(), since
**	    we need the name of the machine as registered on the network.
**      15-Mar-1999 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64).
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-jun-2001 (somsa01)
**	    For PCs, use GVosvers().
**      29-Nov-2001 (fanra01)
**          Bug 106529
**          Update ERsend to add host and timestamp to ER_OPER_MSG type
**          messages.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	15-Jun-2004 (schka24)
**	    Fix unsafe env variable handling on Windows.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	16-Mar-2005 (mutma03)
**	    star issue 13958829: nickname not honouredr.Added code to translate
**	    node name to nickname.
**    	24-Jan-2006 (drivi01)
**	    SIR 115615
**	    Replaced references to SystemProductName with SystemServiceName
**	    and SYSTEM_PRODUCT_NAME with SYSTEM_SERVICE_NAME due to the fact
**	    that we do not want changes to product name effecting service name.
**	07-jan-2008 (joea)
**	    Reverse 16-mar-2005 cluster nicknames change.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	06-Aug-2009 (drivi01)
**	    In efforts to port to Visual Studio 2008, clean up warnings.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for CVal definition.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.  Delete obsolete/ifdef'ed out ERreceive.
**/

/*
** External variables 
*/
static bool ERsysinit = FALSE;
static void ERinitsyslog( void );
# ifdef NT_GENERIC
static HANDLE EventLog = NULL;
static NET_API_STATUS
       (*pNetMessageNameAdd)(    
	    LPCWSTR  servername,
            LPCWSTR  msgname
);
static NET_API_STATUS
       (*pNetMessageNameDel)(
    	    LPCWSTR   servername,
            LPCWSTR   msgname
);
static NET_API_STATUS
       (*pNetMessageBufferSend)(
            LPCWSTR  servername,
            LPCWSTR  msgname,
            LPCWSTR  fromname,
            LPBYTE   buf,
            DWORD    buflen
);

/*
** The messenger service will put a box on the server console
** with From MACHINENAME, To MSGNAME.  MSGNAME must be less
** than 15 characters long.
*/
static wchar_t msgname[] = L"Ingres DBA";         /* must be Unicode */
static wchar_t whostname[GL_MAXNAME*sizeof(wchar_t)] ZERO_FILL; /* must be Unicode */
# endif /* NT_GENERIC */


/*{
** Name: ERsend	- Send message to log.
**
** Description:
**      This procedure writes a message to the log file.
**
** Inputs:
**	flag
**	    ER_ERROR_MSG		write message to errlog.log
**	    ER_AUDIT_MSG		obsolete
**	    ER_OPER_MSG			write to operator (system log, event
**					log, or system-configurable log)
**      message                         Address of buffer containing the message.
**      msg_length                      Length of the message.
**
** Outputs:
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**	    ER_BADSEND
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-oct-1985 (derek)
**          Created new for 5.0.
**      07-oct-1990 (pholman)
**          Re-integrate ER_AUDIT_MSG support by derek.
**      08-Nov-90 (jennifer - integrated pholman)
**          Don't return an error message if this is a null
**          Audit message.
**      29-sep-1992 (pholman)
**          The use of ERsend with an extra parameter (flag) was stillborn,
**          and the function is now obsolete.  Use ERlog instead.
**	20-jun-95 (tutto01)
** 	    Removed invalid ifdef.
**	03-mar-1997 (canor01)
**	    Restored use of ERsend with extra parameter.  While removing
**	    the stillborn ER_AUDIT_MSG functionality, add in the previously
**	    incomplete ER_OPER_MSG usage.
**      29-Nov-2001 (fanra01)
**          Change the named event source to reflect the registry key under
**          SYSTEM\Services\EventLog\Application.
**          Add inclusion of the message header.
**          Change message from fixed 2003 to I_ING_INFO constant. Also
**          Change the event type to EVENTLOG_INFORMATION_TYPE for w32
**          events.  The ER_OPER_MSG is only used from CIvalidate.
**	16-Mar-2005 (mutma03)
**	    Added code to translate node name to nickname.
**	07-jan-2008 (joea)
**	    Revert 16-mar-2005 change.
*/

STATUS
ERsend(i4 flag, char *message, i4 msg_length, CL_ERR_DESC *err_code)
{
# ifdef NT_GENERIC
    static bool		er_init = FALSE;
    static bool		is_w95 = FALSE;
# else /* !NT_GENERIC */
    static int		er_ifi = -2;
    static int          ar_ifi = -2;
# endif /* !NT_GENERIC */
    STATUS		status;
    char                tmp_buf[ER_MAX_LEN];
    char*               logmsg = message;

    /*	Check for bad paramters. */

    CL_CLEAR_ERR( err_code );

    if ((message == 0 || msg_length == 0) && flag != ER_AUDIT_MSG)
        return (ER_BADPARAM);

    if ((flag != ER_ERROR_MSG) && (flag != ER_AUDIT_MSG) &&
        ( flag != ER_OPER_MSG))
        return (ER_BADPARAM);

# ifndef NT_GENERIC
    if (flag & ER_AUDIT_MSG)
    {
        key_t msg_key;
        char  *ipc_number;
        struct
        {
            long    mtype;
            char    mtext[ER_MAX_LEN];
        }   msg;

        if (ar_ifi == -2)
        {
            NMgtAt("II_AUDIT_IPC", &ipc_number);
            if (ipc_number && ipc_number[0])
            {
                CVal(ipc_number, &msg_key);
                ar_ifi = msgget(msg_key, 0);
                if (ar_ifi == -1)
                {
                    SETCLERR(err_code, 0, ER_open);
                    return(ER_NO_AUDIT);
                }
            }
            else
            {
                SETCLERR(err_code, 0, ER_open);
                return(ER_NO_AUDIT);
            }

        }
 
        /*  Handle special case to connect only but not send message. */
 
        if (msg_length == 0 && message == 0)
                return (OK);

        MEcopy(message, msg_length, msg.mtext);
        msg.mtype = 1;
        if (msgsnd(ar_ifi, &msg, msg_length, 0))
        {
            SETCLERR(err_code, 0, ER_open);
            return(ER_BADSEND);
        }
        return (OK);
    }
    else
# endif /* ! NT_GENERIC */
    if (flag & ER_OPER_MSG)
    {
        char    hostname[GL_MAXNAME];
        STATUS  status;
 
        message[msg_length] = EOS;
        TRdisplay("ER Operator:\"%s\"\n",message);
	if (!ERsysinit)
	    ERinitsyslog();
# ifdef NT_GENERIC
        {
        wchar_t *wmessage = NULL;

        /*
        ** Update the ReportEvent to report information in the event log.
        */
        if ( ReportEvent( EventLog,
                        (WORD) EVENTLOG_INFORMATION_TYPE,
                        (WORD) 0,             /* event category */
                        (DWORD) I_ING_INFO,   /* event identifier */
                        (PSID) NULL,
                        (WORD) 1,             /* number of strings */
                        (DWORD) 0,
                        &message,
                        NULL ) == FALSE)   
                status = GetLastError();
	if ( !er_init )
	{
	    char		VersionString[256];
	    FUNC_EXTERN BOOL	GVosvers(char *OSVersionString);

	    GVosvers(VersionString);
	    is_w95 = ( STstrindex(VersionString, "Microsoft Windows 9",
				  0, FALSE) != NULL ) ? TRUE : FALSE;

	    if ( !is_w95 ) /* netapi32 only on NT */
	    {
		HANDLE hDll;
                if ((hDll = LoadLibrary(TEXT("netapi32.dll"))) != NULL)
                {
                    pNetMessageNameAdd = 
		     (NET_API_STATUS (*)(LPCWSTR,LPCWSTR))
		     GetProcAddress(hDll, TEXT("NetMessageNameAdd"));
                    pNetMessageNameDel = 
		     (NET_API_STATUS (*)(LPCWSTR,LPCWSTR))
		     GetProcAddress(hDll, TEXT("NetMessageNameDel"));
                    pNetMessageBufferSend = 
		      (NET_API_STATUS (*)(LPCWSTR,LPCWSTR,LPCWSTR,LPBYTE,DWORD))
		      GetProcAddress(hDll, TEXT("NetMessageBufferSend"));
		}
		/* if any problem, pretend we don't support it */
		if ( pNetMessageNameAdd == NULL ||
		     pNetMessageNameDel == NULL ||
		     pNetMessageBufferSend == NULL )
		    is_w95 = TRUE;
	    }
	}

	if ( !is_w95 )
	{
	    /*
	    ** Now, send the message to the server console,
	    ** putting up a message box (if the messenger service
	    ** is running.  Everything must be in Unicode.
	    */

	    if ( whostname[0] == 0 )
	    {
		unsigned int len = sizeof(hostname);
                /* 
		** get the hostname in Unicode format for use 
		** by messenger service 
		*/
                GetComputerName( (char *)hostname, &len );
		MultiByteToWideChar( GetACP(), 0,
				     hostname, sizeof(hostname),
				     whostname, sizeof(whostname) );
	    }
            /* initialize the messenger service */
            status = (*pNetMessageNameAdd)( whostname, msgname );
            if ( status != NERR_Success )
	        status = GetLastError();

	    /* Allocate a buffer for the Unicode */
	    wmessage = (wchar_t *) MEreqmem( 0, msg_length * sizeof(wchar_t), 
				             TRUE, &status );
	    if ( wmessage )
	    {
	        /* copy the message to the Unicode buffer */
		MultiByteToWideChar( GetACP(), 0,
				     message, msg_length,
				     wmessage, msg_length * sizeof(wchar_t) );
                status = (*pNetMessageBufferSend)( whostname, 
					       msgname, 
					       NULL, 
				               (LPBYTE) wmessage, 
					       msg_length*sizeof(wchar_t) );
                if ( status != NERR_Success )
	            status = GetLastError();
	        MEfree( (PTR)wmessage );
	    }

            /* re-initialize the messenger service */
            status = (*pNetMessageNameDel)( whostname, msgname );
            if ( status != NERR_Success )
	        status = GetLastError();

	}
	}
# elif defined(OS_THREADS_USED) && defined(any_aix)
	syslog_r( LOG_ALERT|LOG_ERR, message );
# else
	syslog( LOG_ALERT|LOG_ERR, message );
# endif /* NT_GENERIC */
    }

    if (flag & ER_OPER_MSG)
    {
        i4 msglen = 0;
	char* host = PMhost();

        MEfill( ER_MAX_LEN, 0, tmp_buf );

        /*
        ** Format the message string for the event log.  As the source is
        ** not known a fixed string of INGSYSLOG is used.
        */
        TRformat( NULL, 0, tmp_buf, ER_MAX_LEN - 1,
            "%8.8t::[INGSYSLOG         , 00000000]: %@ ", STlength(host),
            host );
        msglen = STlength(tmp_buf);
        STcat( tmp_buf, message );  /* append original message */
        msg_length += msglen;
        logmsg = tmp_buf;
    }
    status = ERlog( logmsg, msg_length, err_code );
    return( status );
}


/*{
** Name: ERlog  - Send message to the error logger.
**
** Description:
**      This procedure sends a message to the system specific error
**      logger (currently an error log file). 
**
** Inputs:
**      message                         Address of buffer containing the message.
**      msg_length                      Length of the message.
**
** Outputs:
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**	    ER_BADSEND
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-sep-1992 (pholman)
**          First created, taken from original (6.4) version of ERsend
**	    CL Committee approved, 18-Sep-1992
*/
STATUS
ERlog(char *message, i4 msg_length, CL_ERR_DESC *err_code)
{
    static int		er_ifi = -2;
    auto LOCATION	loc;
    auto char		*fname;
#ifdef	xCL_074_WRITEV_READV_EXISTS
    char		buf2 = '\n';
    struct iovec	iov[2];
    i4			iovlen = 2;
#endif
    
    /*	Check for bad paramters. */

    CL_CLEAR_ERR( err_code );

    if (message == 0 || msg_length == 0)
	return (ER_BADPARAM);

    if (er_ifi == -2)
    {
	if (NMloc(LOG, FILENAME, "errlog.log", &loc) != OK)
	{
	    er_ifi = -1;
	    return(ER_BADSEND);
	}
	LOtos(&loc, &fname);
	er_ifi = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666);
	if (er_ifi == -1)
	{
	    SETCLERR(err_code, 0, ER_open);
	    return(ER_BADSEND);
	}
    }

#ifdef	xCL_074_WRITEV_READV_EXISTS
    iov[0].iov_base = (caddr_t)message;
    iov[0].iov_len = msg_length;
    iov[1].iov_base = (caddr_t)&buf2;
    iov[1].iov_len = 1;

    if( writev(er_ifi, iov, iovlen) != (msg_length + 1))
#else
    if (write(er_ifi, message, msg_length) != msg_length ||
	write(er_ifi, "\n", 1) != 1)
#endif
    {
	SETCLERR(err_code, 0, ER_write);
	return(ER_BADSEND);
    }

    return(OK);
}

static void
ERinitsyslog(void)
{
# ifdef NT_GENERIC
    ERsysinit = TRUE;
    if (EventLog == NULL)
    {
        /*
        ** Register the event source according to the product and
        ** installation code.
        */
        TCHAR  tchServiceName[ _MAX_PATH ];
        CHAR   *inst_id;

        NMgtAt("II_INSTALLATION", &inst_id);
	STlpolycat(3, sizeof(tchServiceName)-1, SystemServiceName,
		"_Database_", inst_id, &tchServiceName[0]);

        EventLog = RegisterEventSource (
            (LPTSTR)NULL,          /* Use Local Machine                    */
            tchServiceName );   /* eventlog app name to find in registry*/
    }

# else /* NT_GENERIC */
    ERsysinit = TRUE;
# if defined(OS_THREADS_USED) && defined(any_aix)
    openlog_r( SystemProductName, LOG_CONS, LOG_USER );
# else
    openlog( SystemProductName, LOG_CONS, LOG_USER );
# endif
# endif /* NT_GENERIC */
}

