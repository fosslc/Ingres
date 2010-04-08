/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/
SQLSTATE_MAPPING_REQUIRED

/*
** Name: ergcf.msg
**
** Description:
**	Message file for Facility code: _GC
**
** History:
**      12-May-89 (pasker) 1.001
**          added messages to support discrimination of PL input event
**          errors
**      16-Jul-89 (seiwald)
**          Added GCN errlog.log messages
**      16-Jul-89 (jorge)
**          reapplied Generic error support
**      01-Aug-89 (cmorris)
**          added error for GCA request response failure
**      25-Sep-89 (seiwald)
**          New GCFFFE_INCOMPLETE status to indicate service must
**          be reinvoked with GCA_RESUME indicator.
**      02-oct-89 (pasker) 1.002
**          added error for when GCusrpwd fails.
**      08-Nov-89 (cmorris)
**          added error for disassociate failure.
**      24-Nov-89 (seiwald)
**          Errors for GCN_RESOLVE operation - all parameterless.
**      26-Nov-89 (seiwald)
**          New E_GC0104_GCN_USERS_FILE.
**      31-Dec-89 (seiwald)
**          New E_GC0032_NO_PEER.
**      05-Feb-90 (seiwald)
**          Made GC0101 like GC0125, and added advice about checking II_CONFIG.
**          New E_GC2219_GCA_REGISTER_FAIL.
**      06-Feb-90 (seiwald)
**          New E_GC2815_NTWK_OPEN.
**      23-Mar-90 (seiwald)
**          Changed text of E_GC0103_GCN_NOT_USER.
**      20-Jun-90 (seiwald)
**          New E_GC2816_NOT_OPEN.
**      31-Dec-90 (seiwald)
**          Fixed E_GC0018_RSTR_READ to say "read" not "write" error.
**      10-jan-91 (stevet)
**          New E_GC2009_CHAR_INIT and E_GC0105_GCN_CHAR_INIT.
**      16-Apr-91 (seiwald)
**          New E_GC2818_PROT_ID for invalid names in II_GCC_PROTOCOLS.
**          Meddled with the text of other, related messages.
**      16-Apr-91 (seiwald)
**          New E_GC2819_NTWK_CONNECTION_FAIL to be returned to user
**          for connection initiation failure rather than the generic
**          E_GC280D_NTWK_ERROR.
**      2-May-91 (seiwald)
**          Changed some wording in E_GC0001_ASSOC_FAIL and
**          E_GC0023_ASSOC_RLSED.
**      20-May-91 (seiwald)
**          New errors for character set negotiation.
**      31-May-91 (seiwald)
**          Spruced up save/restore error messages.  The text for
**          E_GC0015_BAD_SAVE_NAME will be misleading in a 6.3 installation,
**          since 6.3 used that error when it couldn't open the restore file!
**      19-Jun-91 (seiwald)
**          Added some words to the E_GC0137, 138, and 139 error messages
**          to include remote connections and servers registered for db's
**          or vnode's other than the one requested.
**      10-Jul-91 (cmorris)
**          Added messages for protocol bridge.
**      23-Jul-91 (seiwald)
**          Rename II_GCC_CHARSET to II_CHARSET and moved the files
**          gcccset.xlt and gcccset.nam down underneath files/charset.
**      09-Aug-91 (cmorris)
**          Make sure all references to connection id's are in hex!
**      12-Dec-91 (cmorris)
**          Cleaned up confusion between "open" and "listen" in the
**          text of several error messages.
**      19-Dec-91 (cmorris)
**          Added E_GC2A08 for protocol bridge.
**      23-Jan-92 (seiwald)
**         Support for installation passwords: new GCN error messages in
**         the E_GC0140 range.
**      23-Mar-92 (seiwald)
**          Installation password error message shuffled.
**      10-Apr-92 (cmorris)
**          Added E_GC2A09_MAX_PB_CONNS for exceeding max. connections
**  	    protocol bridge can support.
**	08-Jul-92 (brucek)
**	    Added E_GC0033_GCM_NOSUPP for no GCM support.
**	09-Jul-92 (brucek)
**	    Added E_GC0034_GCM_PROTERR for GCM protocol error.
**	14-Oct-92 (brucek)
**	    Added numerous specific GCM error messages.
**	18-Nov-92 (gautam)
**	    Added password prompt error E_GC003C_PWPROMPT_NOTSUP
**  	10-Dec-92 (cmorris)
**  	    Try to make use of "association" and "connection" a little
**  	    more consistent.
**  	15-Dec-92 (gautam)
**          Added PM error messages.
**	29-Jan-93 (brucek)
**	    Added E_GC0040_CS_OK;
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	08-Mar-93 (seiwald)
**	    Cleared up the text of E_GC003C_PWPROMPT_NOTSUP a bit.
**	22-Mar-93 (edg)
**	    Changed "see Comm. Server Log for details" to "errlog.log".
**	7-apr-93 (robf)
**	    Added Secure 2.0 error messages:
**	    E_GC0041_INVALID_SEC_LABEL
**	    E_GC0042_AGENT_MISSING_REMOTE
**	    E_GC0043_AGENT_PARTNER_NOTSUP
**	    E_GC221A_AL_SECLABEL_TYPE 
**	13-May-93 (brucek)
**	    Renumbered GCN_NO_GCN message (formerly E_GC0145) to E_GC0126.
**	31-Aug-93 (brucek)
**	    Changed PM-related message texts to remove references to "PM file".
**	6-Oct-93 (seiwald) bug #52271
**	    A few more words for E_GC0140_GCN_INPW_NOPROTO.
**	1-Apr-94 (seiwald)
**	    New error messages for perform encoding/decoding.
**	11-Apr-94 (seiwald)
**	    More new error messages for perform encoding/decoding.
**      9-Nov-94 (tutto01)
**          Changed text of message E_GC0131 to be more accurate.(bug #62466)
**      9-Nov-94 (tutto01)
**          Changed text of messages E_GC0132, E_GC0133, and E_GC2804 to refer
**          to  NETUTIL, rather than NETU. (bug #60900)
**      7-Dec-94 (liibi01)
**          Cross integration from 6.4 (Bug 52771). Change text of E_GC000B,
**          since it is also returned when trying to use the installation 
**          password against untrusted server(ie server started by different
**          user from iigcn).
**	24-mar-95 (forky01)
**	    E_GC2414_PL_COMP_MSGS_EXCEED added.  Fixed administrator
**	    misspelling.
**	06-feb-96 (rajus01).
**	    Added error msg.. strings for Protocol Bridge.
**	19-jun-1996 (sweeney)
**	    Fix typo.
**      15-oct-96 (mcgem01)
**          Remove hard-coded Ingres references so that these
**          messages can be shared by both Ingres and Jasmine.
**	21-May-97 (gordy)
**	    New GCF security handling.  Added GCS messages E_GC1???.
**	 9-Jun-97 (gordy)
**	    MISC_ING_ERRORS changed to MISC_ERRORS.
**	30-Jun-97 (rajus01)
**	    Added GCS messages for handling encryption/decryption in TL.
**	 5-Aug-97 (gordy)
**	    Added GCS messages for dynamic loading of mechanisms.
**	20-Aug-97 (gordy)
**	    Reworked GCS/TL message for encryption.  Dropped 1013, 
**	    1014, 1015.  Added 200A, 281A, 281B, 281C, 281D.
**	24-Sep-97 (rajus01)
**	    Added E_GC0057_RMTACCESS_FAIL error message. 
**	12-Jan-98 (gordy)
**	    Added direct connection error messages: E_GC016x.
**	25-Feb-98 (gordy)
**	    Added E_GC1013_NO_MEMORY.
**	 5-Jun-98 (gordy)
**	    Added filename parameter to E_GC0124_GCN_NO_IINAME.
**	10-Jun-98 (gordy)
**	    Generalized the message for E_GC0121_GCN_NOMEM.
**	04-May-99 (rajus01)
**	    Added GCS messages for Kerberos.
**	09-jun-1999 (somsa01)
**	    Added E_GC2A10_LOAD_CONFIG.
**	01-jul-1999 (somsa01)
**	    For E_GC2A10_LOAD_CONFIG, print out the server type also.
**	26-oct-1999 (thaju02)
**	    Added E_GC0057_NOTMOUT_DLOCK. (b76625)
**	19-nov-1999 (thaju02)
**	    Modified E_GC0057_NOTMOUT_DLOCK to E_GC0058_NOTMOUT_DLOCK.
**	16-mar-2001 (somsa01)
**	    Added E_GC000E_RMT_LOGIN_FAIL_INFO to supplement
**	    E_GC000B_RMT_LOGIN_FAIL, which prints out the remote machine
**	    address and userid.
**      25-Apr-2001 (musro02)
**          Actually implemented thaju02's intended change of 19-mar-1999. This
**          necessitated a change of E_GC0058_RMTACCESS_FAIL to E_GC0057...
**	07-jun-2001 (abbjo03)
**	    Make the text for E_GC0025 more understandable.
**      26-oct-2001 (loera01) Bug 106183
**          Added text to E_GC0126.
**	18-Dec-02 (gordy)
**	    Added GCD error codes.
**	26-Dec-02 (gordy)
**	    Added E_GC4009.
**	14-Feb-03 (gordy)
**	    Added E_GC401F, E_GC4020.  Use INV_XACT_STATE sqlstate 
**	    where appropriate.  
**      27-Mar-2003 (inifa01) changed in connection to bug 109921 (INGREP135).
**          Changed mapping of E_GC2210 from SS50000_MISC_ERROR to
**          SS08006_CONNECTION_FAILURE.
**	29-May-2003 (inifa01) changed in connection to bug 110326 (INGREP141)
**	    Changed mapping of E_GC0032 from SS50000_MISC_ERROR
**	    to SS08006_CONNECTION_FAILURE.
**	15-Jul-03 (gordy)
**	    Added E_GC4817_CONN_PARM.
**	 1-Dec-03 (gordy)
**	    Added E_GC4021_INVALID_ROW for updatable cursors.
**	28-Jan-04 (gordy)
**	    Added E_GC0145_GCN_INPW_NOT_ALLOWED.  Cleaned up error 
**	    message text related to passwords.
**	31-Mar-04 (wansh01)
**	    Added E_GC5000 thru GC5009 msgs to support GCN/GCADM
**	28-May-04 (gordy)
**	    Added E_GC000F_NO_REGISTER for re-register request.
**	23-Jun-04 (gordy)
**	    Added E_GC100A_AUTH_EXPIRED and E_GC100B_NO_EXPIRATION.
**	 9-Jul-04 (gordy)
**	    Added E_GC100C_RESTRICTED_AUTH and E_GC1026_RESTRICTED_AUTH
**	13-jul-04 (devjo01)
**	    Corrected spelling error in E_GC4016_RS_CHANGED.
**	19-Sept-04 (wansh01)
**	    Added E_GC481D_GCD_ADM_SVR_CLOSED, E_GC481E_GCD_ADM_SESS_ABORT
**	    to support gcd/admin. 
**	 9-Oct-04 (wansh01)
**	    Added E_GC200B, E_GC200C, E_GC200D, E_GC221B to support gcc/admin
**	17-OCT-04 (wansh01)
**	    Deleted (GCN GCD GCC )_ADM_AUTH  error messages.
**	13-May-05 (gordy)
**	    Added E_GC4818_XACT_SAVEPOINT, E_GC4819_ABORT_STATE.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**      27-Jul-2004 (inifa01) changed in connection to bug 110479 (INGREP144).
**          Changed mapping of E_GC0001 from SS50000_MISC_ERROR to
**          SS08006_CONNECTION_FAILURE.
**	 6-Dec-05 (gordy)
**	    Added E_GC100D_SRV_AUTH_FAIL, E_GC0146_GCN_INPW_SRV_NOSUPP.
**	 1-Mar-06 (gordy)
**	    Added E_GC001A_DUP_REGISTER for invalid addition GCA_REGISTER,
**	    E_GC0153_GCN_SRV_STARTUP, E_GC0154_GCN_SRV_SHUTDOWN,
**	    E_GC0155_GCN_SRV_FAILURE to log registered server status,
**	    and E_GC0156_GCN_MAX_SESSIONS.
**	14-Jul-06 (gordy)
**	    Added E_GC481A_XACT_END_STATE.
**      26-Oct-06 (Ralph Loen) Bug 116972
**          Add E_GC0157_GCN_BCHK_INFO, E_GC0158_GCN_BCHK_ACCESS, and
**          E_GC0159_GCN_BCHK_FAIL to report Name Server deregistration.
**	8-Jun-07 (kibro01) b118226
**	    Added E_GC0164_GCN_CHAINED_VNODE message
**      03-Aug-07 (Ralph Loen) SIR 118898
**          Added E_GC1200_KRB_GSS_API_ERR, E_GC0070_HOST_TRUNCATED,
**          E_GC0071_NO_HOSTS, and E_GC0072_INVALID_ARGS.
**      31-Oct-07 (Ralph Loen) Bug 119358
**          Added E_GC1027_GCF_MECH_DISABLED and E_GC1028_GCF_MECH_UNSUPPORTED.
**      15-Feb-08 (rajus01) SD issue 121381, Bug 119928.
**          Use more generic term LOB instead of BLOB/CLOB in error messages.
**      04-Apr-08 (rajus01) SD issue 126704, Bug 120136
**          Added E_GC0170_GCN_USERNAME_LEN, E_GC0171_GCN_PASSWORD_LEN,
**	    and E_GC1029_GCS_PASSWORD_LEN.
**      14-Apr-08 (rajus01) SD issue 126704, Bug 120137
**          Added E_GC0172_GCN_PARTNER_LEN.
**	15-Jul-09 (rajus01) SD issue 137207, Bug 122295.
**	    Modified the E_GC0163 error message which kind of mislead
**	    the test engineers while testing vnode configured for direct
**	    connects from netutil/Test menu option.
**
**	 3-Aug-09 (gordy)
**	    Added E_GC011F_GCN_BAD_RECFMT.
*/

#define _GC_CLASS 12

/*
** GCA errors
*/

E_GC0001_ASSOC_FAIL:SS08006_CONNECTION_FAILURE
"Association failure: partner abruptly released association"
E_GC0002_INV_PARM:SS50000_MISC_ERRORS
"Invalid service request parameter"
E_GC0003_INV_SVC_CODE:SS50000_MISC_ERRORS
"Invalid service code in call sequence"
E_GC0004_INV_PLIST_PTR:SS50000_MISC_ERRORS
"Invalid parameter list pointer"
E_GC0005_INV_ASSOC_ID:SS50000_MISC_ERRORS
"Invalid association identifier"
E_GC0006_DUP_INIT:SS50000_MISC_ERRORS
"Duplicate initiate request"
E_GC0007_NO_PREV_INIT:SS50000_MISC_ERRORS
"No previous initiation request"
E_GC0008_INV_MSG_TYPE:SS50000_MISC_ERRORS
"Invalid message type identifier"
E_GC0009_INV_BUF_ADDR:SS50000_MISC_ERRORS
"Invalid buffer address"
E_GC000A_INT_PROT_LVL:SS50000_MISC_ERRORS
"Internal IPC protocol level mismatch between client and server"
E_GC000B_RMT_LOGIN_FAIL:SS50000_MISC_ERRORS
"Login failure: invalid username/password."
E_GC000C_API_VERSION_INVALID:SS50000_MISC_ERRORS
"API version specified is invalid."
E_GC000D_ASSOCN_REFUSED:SS50000_MISC_ERRORS
"Association request refused by server"
E_GC000E_RMT_LOGIN_FAIL_INFO:SS50000_MISC_ERRORS
"A remote login attempt failed. The remote machine was '%0c', and
the userid was '%1c'."
E_GC000F_NO_REGISTER:SS50000_MISC_ERRORS
"Name Server only registration requires a prior complete registration."
E_GC0010_BUF_TOO_SMALL:SS50000_MISC_ERRORS
"Buffer is too small: no room for data"
E_GC0011_INV_CONTENTS:SS50000_MISC_ERRORS
"Invalid buffer contents"
E_GC0012_LISTEN_FAIL:SS50000_MISC_ERRORS
"Failure to issue listen"
E_GC0013_ASSFL_MEM:SS50000_MISC_ERRORS
"Association failure (memory allocation)"
E_GC0014_SAVE_FAIL:SS50000_MISC_ERRORS
"Save failure: unable to save association information"
E_GC0015_BAD_SAVE_NAME:SS50000_MISC_ERRORS
"Restore failure: calling program may be of an incompatible release"
E_GC0016_RESTORE_FAIL:SS50000_MISC_ERRORS
"Restore failure: unable to recover calling program's association information"
E_GC0017_RSTR_OPEN:SS50000_MISC_ERRORS
"Restore failure: error opening restore file"
E_GC0018_RSTR_READ:SS50000_MISC_ERRORS
"Restore failure: error reading from restore file"
E_GC0019_RSTR_CLOSE:SS50000_MISC_ERRORS
"Restore failure: error closing restore file"
E_GC001A_DUP_REGISTER:SS50000_MISC_ERRORS
"Duplicate registration request"
E_GC0020_TIME_OUT:SS50000_MISC_ERRORS
"Time_out expiration: service request incomplete"
E_GC0021_NO_PARTNER:SS50000_MISC_ERRORS
"Requested association partner is unavailable"
E_GC0022_NOT_IACK:SS50000_MISC_ERRORS
"Association is in purge state due to expedited data receipt.
Only GCA_IACK may be sent."
E_GC0023_ASSOC_RLSED:SS50000_MISC_ERRORS
"I/O request was canceled when association was released"
E_GC0024_DUP_REQUEST:SS50000_MISC_ERRORS
"A duplicate request was currently outstanding on this association"
E_GC0025_NM_SRVR_ID_ERR:SS50000_MISC_ERRORS
"Unable to connect to Name Server: Blank or incorrect Name Server ID.\n
The Name Server may not be running. Check the status of your installation."
E_GC0026_NM_SRVR_ERR:SS50000_MISC_ERRORS
"Unable to connect to Name Server: system error during connection"
E_GC0027_RQST_PURGED:SS50000_MISC_ERRORS
"Request was purged by interrupt message sent or received"
E_GC0028_LISTEN_RESOURCE:SS50000_MISC_ERRORS
"GCA_LISTEN failed: server resource shortage.  See system administrator"
E_GC0029_RQST_FAIL:SS50000_MISC_ERRORS
"GCA_REQUEST failed: unable to associate with server"
E_GC002A_RQST_RESOURCE:SS50000_MISC_ERRORS
"GCA_REQUEST failed: server resource shortage.  See system administrator"
E_GC002B_SND1_FAIL:SS50000_MISC_ERRORS
"GCA_SEND failed: see system error status."
E_GC002C_SND2_FAIL:SS50000_MISC_ERRORS
"GCA_SEND failed: see system error status."
E_GC002D_SND3_FAIL:SS50000_MISC_ERRORS
"GCA_SEND failed: see system error status."
E_GC002E_RCV1_FAIL:SS50000_MISC_ERRORS
"GCA_RECEIVE failed: see system error status."
E_GC002F_RCV2_FAIL:SS50000_MISC_ERRORS
"GCA_RECEIVE failed: see system error status."
E_GC0030_RCV3_FAIL:SS50000_MISC_ERRORS
"GCA_RECEIVE failed: see system error status."
E_GC0031_USRPWD_FAIL:SS50000_MISC_ERRORS
"Unable to validate username/password. See system error status."
E_GC0032_NO_PEER:SS08006_CONNECTION_FAILURE
"GCA_LISTEN peer read failed: probably Name Server connection."
E_GC0033_GCM_NOSUPP:SS50000_MISC_ERRORS
"An attempt was made to establish a management association to a process which
does not support the management protocol."
E_GC0034_GCM_PROTERR:SS50000_MISC_ERRORS
"A protocol error was detected on a management association."
E_GC0035_GCM_INVLVL:SS50000_MISC_ERRORS
"Invalid GCA protocol level specified on GCM message."
E_GC0036_GCM_NOEOD:SS50000_MISC_ERRORS
"End-of-data flag was not specified on GCM message."
E_GC0037_GCM_INVTYPE:SS50000_MISC_ERRORS
"A non-GCM message was received on a management association."
E_GC0038_GCM_INVCOUNT:SS50000_MISC_ERRORS
"Invalid element or row count specified in GCM message."
E_GC0039_GCM_NOTRAP:SS50000_MISC_ERRORS
"GCM_SET_TRAP message was received by a process which does not support traps."
E_GC003A_GCM_INVMON:SS50000_MISC_ERRORS
"Unknown monitor handle or inconsistent monitor description specified in
GCM message."
E_GC003B_GCM_INVTRAP:SS50000_MISC_ERRORS
"Unknown trap handle specified in GCM message."
E_GC003C_PWPROMPT_NOTSUP:SS50000_MISC_ERRORS
"The Name Server does not support setting remote username and password."
E_GC003D_BAD_PMFILE:SS50000_MISC_ERRORS
"Syntax error detected in config.dat file. Please correct it and rerun this 
program."
E_GC003E_PMLOAD_ERR:SS50000_MISC_ERRORS
"Operating system error in loading config.dat file. Please check to see if 
file exists and is readable."
E_GC003F_PM_NOPERM:SS50000_MISC_ERRORS
"User does not have the required permission in the config.dat file for the
requested operation."
E_GC0040_CS_OK:SS50000_MISC_ERRORS
"A requested GCA command service was executed successfully."
E_GC0041_INVALID_SEC_LABEL:SS50000_MISC_ERRORS
"An invalid security label or label type was detected while establishing
the connection."
E_GC0042_AGENT_MISSING_REMOTE:SS50000_MISC_ERRORS
"Connection information for client is missing for an agent connection."
E_GC0043_AGENT_PARTNER_NOTSUP:SS50000_MISC_ERRORS
"An Agent connection request was attempted but the remote partner does not
support the agent protocol level."
E_GC0050_PDD_ERROR:SS50000_MISC_ERRORS
"Internal error in perform_decoding routine."
E_GC0051_PDD_BADSIZE:SS50000_MISC_ERRORS
"The output buffer handed to perform_decoding is too small to handle the fixed
part of the message."
E_GC0055_PDE_ERROR:SS50000_MISC_ERRORS
"Internal error in perform_encoding routine."
E_GC0056_PDE_BADSIZE:SS50000_MISC_ERRORS
"The input message handed to perform_encoding is not whole."
E_GC0057_RMTACCESS_FAIL:SS50000_MISC_ERRORS
"A client on a remote host attempted to connect directly without using
Ingres/Net but direct connections have not been enabled.  The connection
was rejected."
E_GC0058_NOTMOUT_DLOCK:SS50000_MISC_ERRORS
"Override normal channel blocking (no timeout), to process data detected on the 
expedited channel, avoiding deadlock."

/*
** GCdns_hostname() errors
*/

E_GC0070_HOST_TRUNCATED:SS50000_MISC_ERRORS
"Returned hostname is larger than provided input buffer.  Result is truncated."
E_GC0071_NO_HOSTS:SS50000_MISC_ERRORS
"No host name could be found in the network database."
E_GC0072_INVALID_ARGS:SS50000_MISC_ERRORS
"One or more of the input arguments was out of range."

/*
** GCN errors
*/

E_GC0100_GCN_SVRCLASS_NOTFOUND:SS50000_MISC_ERRORS
"Name Service failed: Server Class %0c is not found."
E_GC0101_GCN_FILE_OPEN:SS50000_MISC_ERRORS
"Name server can't read/write file %0c.
Check II_CONFIG and name subdirectory."
E_GC0103_GCN_NOT_USER:SS50000_MISC_ERRORS
"%0c is not a name server superuser."
E_GC0104_GCN_USERS_FILE:SS50000_MISC_ERRORS
"Cannot open users file to verify user's privileges."
E_GC0105_GCN_CHAR_INIT:SS50000_MISC_ERRORS
"Error while processing character set attribute file."
E_GC0110_GCN_SYNTAX_ERR:SS50000_MISC_ERRORS
"Syntax Error: %0c"
E_GC011F_GCN_BAD_RECFMT:SS50000_MISC_ERRORS
"Invalid record format."
E_GC0120_GCN_BAD_RECLEN:SS50000_MISC_ERRORS
"Name server file %0c has wrong record length."
E_GC0121_GCN_NOMEM:SS50000_MISC_ERRORS
"Name server memory allocator call failed: %0c."
E_GC0122_GCN_2MANY:SS50000_MISC_ERRORS
"Name server startup failed - another name server already running."
E_GC0123_GCN_NSID_FAIL:SS50000_MISC_ERRORS
"Name server startup failed - can't set name server public listen id."
E_GC0124_GCN_NO_IINAME:SS50000_MISC_ERRORS
"Name server startup failed - can't access file %0c.
Check II_CONFIG and name subdirectory."
E_GC0125_GCN_BAD_FILE:SS50000_MISC_ERRORS
"Name server can't read/write file %0c.
Check II_CONFIG and name subdirectory."
E_GC0126_GCN_NO_GCN:SS50000_MISC_ERRORS
"There is no Name Server running in this installation.  
The installation may not have been started."
E_GC0130_GCN_BAD_SYNTAX:SS50000_MISC_ERRORS
"User supplied database name is not formatted properly.
Format for database name is: vnode::dbname/class.
Vnode:: and /class are optional."
E_GC0131_GCN_BAD_RMT_VNODE:SS50000_MISC_ERRORS
"A default remote vnode has been defined (by config file entry:
ii.<host>.gcn.remote_vnode), but connection or login information for
that vnode is missing."
E_GC0132_GCN_VNODE_UNKNOWN:SS50000_MISC_ERRORS
"User provided a vnode as part of the database name (vnode::dbname),
but connection information for that vnode is missing.
Enter connection information for the vnode using NETUTIL."
E_GC0133_GCN_LOGIN_UNKNOWN:SS50000_MISC_ERRORS
"User provided a vnode as part of the database name (vnode::dbname),
but login information for that vnode is missing.
Enter login information for the vnode using NETUTIL."
E_GC0134_GCN_INT_NQ_ERROR:SS50000_MISC_ERRORS
"The Name Server was unable to access the internal name queue
of available servers."
E_GC0135_GCN_BAD_SVR_TYPE:SS50000_MISC_ERRORS
"A default server class has been defined (by II_GCN_SVR_TYPE),
but that server class is not known to the Name Server."
E_GC0136_GCN_SVRCLASS_UNKNOWN:SS50000_MISC_ERRORS
"User provided a server class as part of the database name (dbname/class),
but that server class is not known to the Name Server."
E_GC0137_GCN_NO_GCC:SS50000_MISC_ERRORS
"User attempted network connection by providing a vnode as part of
the database name (vnode::dbname), but no Communication Servers (for the
specified vnode) are running in this installation."
E_GC0138_GCN_NO_SERVER:SS50000_MISC_ERRORS
"User provided a server class as part of the database name (dbname/class),
but no servers of that class (for the specified database) are running in
the target installation."
E_GC0139_GCN_NO_DBMS:SS50000_MISC_ERRORS
"No DBMS servers (for the specified database) are running in the
target installation."

E_GC0140_GCN_INPW_NOPROTO:SS50000_MISC_ERRORS
"An installation password has been defined for the remote vnode, but that
installation doesn't support the installation password protocol.  Make sure
all servers are 6.4/02 or newer and that they have been started by the
installation owner."
E_GC0141_GCN_INPW_NOSUPP:SS50000_MISC_ERRORS
"An installation password has been defined for the remote vnode, but that
installation is not allowing use of installation passwords."
E_GC0142_GCN_INPW_INVALID:SS50000_MISC_ERRORS
"The installation password for the remote vnode is invalid."
E_GC0143_GCN_INPW_BADTICKET:SS50000_MISC_ERRORS
"An installation password is being used for remote authentication, but
the authentication ticket passed is not a valid one.  It may have expired
prematurely due to internal errors."
E_GC0144_GCN_INPW_BROKEN:SS50000_MISC_ERRORS
"An internal error occurred while trying to use an installation password."
E_GC0145_GCN_INPW_NOT_ALLOWED:SS50000_MISC_ERRORS
"Installation paswords may only be used in vnodes."
E_GC0146_GCN_INPW_SRV_NOSUPP:SS50000_MISC_ERRORS
"Target server cannot validate installation passwords.
Server needs to be re-registered with Name Server (restarted)."
E_GC0151_GCN_STARTUP:SS50000_MISC_ERRORS
"Name Server normal startup."
E_GC0152_GCN_SHUTDOWN:SS50000_MISC_ERRORS
"Name Server normal shutdown."
E_GC0153_GCN_SRV_STARTUP:SS50000_MISC_ERRORS
"Server Registration: class %0c, address %1c"
E_GC0154_GCN_SRV_SHUTDOWN:SS50000_MISC_ERRORS
"Registered server shutdown: class %0c, address %1c"
E_GC0155_GCN_SRV_FAILURE:SS50000_MISC_ERRORS
"Registered server failure: class %0c, address %1c"
E_GC0156_GCN_MAX_SESSIONS:SS50000_MISC_ERRORS
"Name Server has reached maximum number of user sessions."
E_GC0157_GCN_BCHK_INFO:SS50000_MISC_ERRORS
"Removing server registration: class %0c, address %1c.  Different server found: installation ID %2c, class %3c" 
E_GC0158_GCN_BCHK_ACCESS:SS50000_MISC_ERRORS
"Removing server registration: class %0c, address %1c.  Server not responding: status 0x%2x" 
E_GC0159_GCN_BCHK_FAIL:SS50000_MISC_ERRORS
"Removing server registration: class %0c, address %1c.  Could not connect to server: status 0x%2x" 
E_GC0160_GCN_NO_DIRECT:SS50000_MISC_ERRORS
"An error has occurred while attempting to make a remote direct
connection.  A normal connection using the communication server
will be attempted instead."
E_GC0161_GCN_DC_HET:SS50000_MISC_ERRORS
"A direct connection request was made to a remote installation 
with a numeric or character set representation different than
the local installation."
E_GC0162_GCN_DC_VERSION:SS50000_MISC_ERRORS
"A direct connection request was made to a remote installation
which does not support the direct connection protocol"
E_GC0163_GCN_DC_NO_SERVER:SS50000_MISC_ERRORS
"The Name Server on a remote host was unable to provide authorization 
for a registered server to handle a direct connection request.  The
request may have been directed at the remote Name Server or a server
which did not register with the remote Name Server. Because a direct
connection to a different server may be possible, it is safe to ignore
this error and the subsequent E_GC0160 error when testing direct
connection type vnodes from a vnode configuration utility."
E_GC0164_GCN_CHAINED_VNODE:SS50000_MISC_ERRORS
"Chaining vnodes is not supported."
E_GC0165_GCN_ADM_CMD:SS50000_MISC_ERRORS
"Invalid Name Server command."
E_GC0166_GCN_ADM_INT_ERR:SS50000_MISC_ERRORS
"An internal Name Server admin error has been detected."
E_GC0167_GCN_ADM_INIT_ERR:SS50000_MISC_ERRORS
"An error occurred during init GCADM inteface."
E_GC0168_GCN_ADM_SESS_ERR:SS50000_MISC_ERRORS
"An internal Name Server admin session error has been detected."
E_GC0169_E_GC0169_GCN_ADM_SVR_CLOSED:SS50000_MISC_ERRORS
"Server closed. user connections disabled "
E_GC0170_GCN_USERNAME_LEN:SS50000_MISC_ERRORS
"The username length exceeded the limit supported by the server."
E_GC0171_GCN_PASSWORD_LEN:SS50000_MISC_ERRORS
"The password length exceeded the limit supported by the server."
E_GC0172_GCN_PARTNER_LEN:SS50000_MISC_ERRORS
"The partner/target length exceeded the limit supported by the server."

/*
** GCS errors
*/

E_GC1000_GCS_FAILURE:SS50000_MISC_ERRORS
"An internal GCS failure occurred."
E_GC1001_GCS_NOT_INITIALIZED:SS50000_MISC_ERRORS
"GCF security system not initialized."
E_GC1002_GCS_OP_UNKNOWN:SS50000_MISC_ERRORS
"Requested GCS operation is not defined."
E_GC1003_GCS_OP_UNSUPPORTED:SS50000_MISC_ERRORS
"GCS operation not supported by requested security mechanism."
E_GC1004_SEC_MECH_UNKNOWN:SS50000_MISC_ERRORS
"Requested GCF security mechanism is not defined."
E_GC1005_SEC_MECH_DISABLED:SS50000_MISC_ERRORS
"Requested GCF security mechanism has been disabled."
E_GC1006_SEC_MECH_FAIL:SS50000_MISC_ERRORS
"GCF security mechanism could not be loaded.  See error log for details."
E_GC1007_NO_AUTHENTICATION:SS50000_MISC_ERRORS
"Client failed to provide user authentication."
E_GC1008_INVALID_USER:SS50000_MISC_ERRORS
"Unable to authenticate client's user ID."
E_GC1009_INVALID_SERVER:SS50000_MISC_ERRORS
"Unable to authenticate server.  Server is either running
under an untrusted user ID or failed to properly register
itself with the Name Server."
E_GC100A_AUTH_EXPIRED:SS50000_MISC_ERRORS
"Authentication certificate has expired."
E_GC100B_NO_EXPIRATION:SS50000_MISC_ERRORS
"Authentication certificate did not contain expiration time when required."
E_GC100C_RESTRICTED_AUTH:SS50000_MISC_ERRORS
"Request to validate authentication certificate failed because the
certificate did not belong to the configured security mechanism."
E_GC100D_SRV_AUTH_FAIL:SS50000_MISC_ERRORS
"Validation of Server authentication failed.
This may be caused by an improperly registered server
or an invalid attempt to change the client user ID."
E_GC1010_INSUFFICIENT_BUFFER:SS50000_MISC_ERRORS
"Buffer too small to hold GCS data object."
E_GC1011_INVALID_DATA_OBJ:SS50000_MISC_ERRORS
"GCS data object contents invalid."
E_GC1012_INVALID_PARM_VALUE:SS50000_MISC_ERRORS
"Invalid GCS parameter value."
E_GC1013_NO_MEMORY:SS50000_MISC_ERRORS
"Unable to allocate memory for GCS data object."
E_GC1020_GCF_MECH_INIT:SS50000_MISC_ERRORS
"GCF security mechanism '%0c' initialization failed.  Status 0x%1x"
E_GC1021_GCF_MECH_INFO:SS50000_MISC_ERRORS
"GCF security mechanism '%0c' failed to properly identify capabilities."
E_GC1022_GCF_MECH_INSTALL:SS50000_MISC_ERRORS
"Configured GCF security mechanism '%0c' not available."
E_GC1023_GCF_MECH_LOAD:SS50000_MISC_ERRORS
"Error loading GCF security mechanism '%0c' from %1c (module %2c).
  Status 0x%3x"
E_GC1024_GCF_MECH_BIND:SS50000_MISC_ERRORS
"GCF security mechanism '%0c' entry point %1c not defined:
 module %2c in %3c.  Status 0x%4x"
E_GC1025_GCF_MECH_DUP_ID:SS50000_MISC_ERRORS
"GCF security mechanisms '%0c' and '%1c' have the same mechanism ID: %2d."
E_GC1026_GCF_RESTRICTED_AUTH:SS50000_MISC_ERRORS
"'%0c' authentication certificate belonging to GCF security mechanism '%1c'
could not be validated.  Validation restricted to mechanism '%2c'."
E_GC1027_GCF_MECH_DISABLED:SS50000_MISC_ERRORS
"GCF Security Mechanism %0c is disabled and cannot be configured for use."
E_GC1028_GCF_MECH_UNSUPPORTED:SS50000_MISC_ERRORS
"GCF Security Mechanism %0c does not provide the %1c authentication capability"
E_GC1029_GCS_PASSWORD_LEN:SS50000_MISC_ERRORS
"The password length exceeded the supported limit."
/*
** Kerberos errors
*/
E_GC1100_KRB_FQDNHOST:SS50000_MISC_ERRORS
"Please define the Fully Qualified Domain Name for the host using CBF."
E_GC1101_KRB_WINSOCK_INIT:SS50000_MISC_ERRORS
"Error initializing winsock."
E_GC1102_KRB_GSS_ACQUIRE_CRED:SS50000_MISC_ERRORS
"Unable to acquire credentials handle for the ingres service principal.  See errlog.log for detail."
E_GC1103_KRB_GSS_INIT_SEC_CTX:SS50000_MISC_ERRORS
"Initiating security context failed.  See errlog.log for detail."
E_GC1104_KRB_GSS_ACCEPT_SEC_CTX:SS50000_MISC_ERRORS
"Accepting security context failed.  See errlog.log for detail."
E_GC1105_KRB_GSS_IMPORT_NAME:SS50000_MISC_ERRORS
"Error converting '%0c'principal name into internal form.  See errlog.log for detail."
E_GC1106_KRB_GSS_DISPLAY_NAME:SS50000_MISC_ERRORS
"Error converting principal in internal form into text.  See errlog.log for detail."
E_GC1107_KRB_GSS_ENCRYPT:SS50000_MISC_ERRORS
"Data encryption failed.  See errlog.log for detail."
E_GC1108_KRB_GSS_DECRYPT:SS50000_MISC_ERRORS
"Data decryption failed.  See errlog.log for detail."
E_GC1109_KRB_GSS_INQUIRE_CRED:SS50000_MISC_ERRORS
"Invalid delegated credential.  See errlog.log for detail."

/*
** GSS API errors
*/
E_GC1200_KRB_GSS_API_ERR:SS50000_MISC_ERRORS
"GSS API error status %0d: '%1c'"

/*
** GCC errors
*/

E_GC2001_STARTUP:SS50000_MISC_ERRORS
"Communication Server normal startup."
E_GC2002_SHUTDOWN:SS50000_MISC_ERRORS
"Communication Server normal shutdown."
E_GC2003_STARTUP_PARM:SS50000_MISC_ERRORS
"A Communication Server startup parameter was incorrectly specified."
E_GC2004_ALLOCN_FAIL:SS50000_MISC_ERRORS
"Memory allocation failure in Communication Server."
E_GC2005_INIT_FAIL:SS50000_MISC_ERRORS
"Communication Server initialization failed."
E_GC2006_STARTUP:SS50000_MISC_ERRORS
"Communication Server normal startup: server rev. level %0c."
E_GC2007_NO_AUTH_INET:SS50000_MISC_ERRORS
"Communication Server startup failure: No authorization for NET product."
E_GC2008_OUT_OF_MEMORY:SS50000_MISC_ERRORS
"Comm Server memory shortage: unable to allocate object type %0c."
E_GC2009_CHAR_INIT:SS50000_MISC_ERRORS
"Error while processing character set attribute file."
E_GC200A_ENCRYPT_MODE:SS50000_MISC_ERRORS
"Invalid encryption mode, '%0c', specified in config file or VNODE entry."
E_GC200B_ADM_SESS_ERR:SS50000_MISC_ERRORS
"An internal GCC admin session error has been detected."
E_GC200C_ADM_INIT_ERR:SS50000_MISC_ERRORS
"An error occurred during init GCC/GCADM inteface."
E_GC200E_ADM_SESS_ABT:SS50000_MISC_ERRORS
"User session is removed by admin."

E_GC2201_ASSOC_END:SS50000_MISC_ERRORS
"Normal local association termination: session end."
E_GC2202_AL_FSM_INP:SS50000_MISC_ERRORS
"AL internal error: Invalid input event in AL FSM"
E_GC2203_AL_FSM_STATE:SS50000_MISC_ERRORS
"AL internal error: Invalid state transition in AL FSM"
E_GC2204_ASSOC_START:SS50000_MISC_ERRORS
"Normal local association initiation: session start."
E_GC2205_RMT_ABORT:SS50000_MISC_ERRORS
"Session failure: ABORT received from remote partner."
E_GC2206_LCL_ASSOC_FAIL:SS50000_MISC_ERRORS
"Session failure: association with local partner failed."
E_GC2207_RMT_INIT_FAIL:SS50000_MISC_ERRORS
"Session failure: remote association initiation failed, status follows."
E_GC2208_CONNCTN_ABORT:SS50000_MISC_ERRORS
"Session failure: releasing local association."
E_GC2209_SND_FAIL:SS50000_MISC_ERRORS
"GCA send failure: reason follows."
E_GC220A_NRM_RCV_FAIL:SS50000_MISC_ERRORS
"GCA normal receive failure: reason follows."
E_GC220B_EXP_RCV_FAIL:SS50000_MISC_ERRORS
"GCA expedited receive failure: reason follows."
E_GC220C_AL_INIT_FAIL:SS50000_MISC_ERRORS
"GCC AL initialization failure: reason follows."
E_GC220D_AL_PASSWD_FAIL:SS50000_MISC_ERRORS
"GCC AL received an invalid (null) password from a remote client."
E_GC220E_AL_FSM_STATE:SS50000_MISC_ERRORS
"AL internal error: Invalid state transition in AL FSM.\
 Input = %0d, state = %1d"
E_GC220F_AL_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Comm Server AL internal error. Please see system administrator."
E_GC2210_LCL_ABORT:SS08006_CONNECTION_FAILURE
"Session failure: Local ABORT condition."
E_GC2211_INCMG_ASSOC:SS50000_MISC_ERRORS
"Incoming association initiation from local user %0c, as remote user %1c."
E_GC2212_OUTGNG_ASSOC:SS50000_MISC_ERRORS
"Outgoing local association initiation for remote user %0c."
E_GC2213_AL_NO_CCB:SS50000_MISC_ERRORS
"GCC internal error: CCB for connection %0x cannot be found."
E_GC2214_MAX_IB_CONNS:SS50000_MISC_ERRORS
"GCC server has exceeded max inbound connections."
E_GC2215_MAX_OB_CONNS:SS50000_MISC_ERRORS
"GCC server has exceeded max outbound connections."
E_GC2216_GCA_LSN_FAIL:SS50000_MISC_ERRORS
"GCC AL processing of a GCA_LISTEN completion has failed."
E_GC2217_GCA_RQRESP_FAIL:SS50000_MISC_ERRORS
"GCA request response failure: reason follows."
E_GC2218_GCA_DISASSOC_FAIL:SS50000_MISC_ERRORS
"GCA disassociate failure: reason follows."
E_GC2219_GCA_REGISTER_FAIL:SS50000_MISC_ERRORS
"GCA registration with Name Server failed: reason follows."
E_GC221A_AL_SECLABEL_TYPE:SS50000_MISC_ERRORS
"GCC AL received an incompatible security label type from a remote client.
 NET may only be used to connect secure nodes which utilize 
 compatible security label formats. The connection is rejected."
E_GC221B_AL_SVR_CLSD:SS50000_MISC_ERRORS
"GCC server is not accepting new connection."

E_GC2401_CONNCTN_END:SS50000_MISC_ERRORS
"Normal Presentation Connection termination."
E_GC2402_PL_FSM_INP:SS50000_MISC_ERRORS
"PL internal error: Invalid input event in PL FSM"
E_GC2403_PL_FSM_STATE:SS50000_MISC_ERRORS
"PL internal error: Invalid state transition in PL FSM"
E_GC2404_PL_FSM_STATE:SS50000_MISC_ERRORS
"PL internal error: Invalid state transition in PL FSM.\
 Input = %0d, state = %1d"
E_GC2405_PL_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Comm Server PL internal error. Please see system administrator."

/*
** new codes added to discriminate between different PL input event failures
*/
E_GC2406_IE_BAD_PCON_RSP_RSLT:SS50000_MISC_ERRORS
"PL internal error. P_CONNECT response result not ACCEPT or REJECT.\
 Please see system administrator."
E_GC2407_IE_BAD_PCON_TYPE:SS50000_MISC_ERRORS
"PL internal error. P_CONNECT type not REQUEST or RESPONSE.\
 Please see system administrator."
E_GC2408_IE_BAD_PRSL_TYPE:SS50000_MISC_ERRORS
"PL internal error. P_RELEASE type not REQUEST or RESPONSE.\
 Please see system administrator."
E_GC2409_IE_BAD_SCON_RSP_RSLT:SS50000_MISC_ERRORS
"PL internal error. S_CONNECT confirm result not ACCEPT or REJECT.\
 Please see system administrator."
E_GC240A_IE_BAD_SCON_TYPE:SS50000_MISC_ERRORS
"PL internal error. S_CONNECT type not REQUEST or RESPONSE.\
 Please see system administrator."
E_GC240B_IE_BAD_SREL_CNF_RSLT:SS50000_MISC_ERRORS
"PL internal error. S_RELEASE confirm result not AFFIRMATIVE or NEGATIVE.\
 Please see system administrator."
E_GC240C_IE_BAD_SREL_TYPE:SS50000_MISC_ERRORS
"PL internal error. S_RELEASE type not INDICATION or CONFIRM.\
 Please see system administrator."
E_GC240D_IE_BAD_SUAB_PDU:SS50000_MISC_ERRORS
"PL internal error. S_U_ABORT PPDU_ID not ARP or ARU.\
 Please see system administrator."
E_GC240E_IE_BAD:SS50000_MISC_ERRORS
"PL internal error. Bad INPUT event.\
 Please see system administrator."
E_GC240F_INIT_DOC_STACK:SS50000_MISC_ERRORS
"PL internal error. INIT_DOC_STACK Failed.\
 Please see system administrator."
E_GC2410_PERF_CONV:SS50000_MISC_ERRORS
"PL internal error. PERFORM_CONVERSION Failed.\
 Please see system administrator."
E_GC2411_OD_PTR_NULL:SS50000_MISC_ERRORS
"PL internal error. OD_PTR is NULL in INIT_DOC_STACK.\
 Please see system administrator."
E_GC2412_DOC_EL_NULL:SS50000_MISC_ERRORS
"PL internal error. DOC_EL is NULL in PERFORM_CONVERSION.\
 Please see system administrator."
E_GC2413_GET_NEXT_ATOM:SS50000_MISC_ERRORS
"PL internal error. GET_NEXT_ATOM failed in PERFORM_CONVERSION.\
 Please see system administrator."
E_GC2414_PL_COMP_MSGS_EXCEED:SS50000_MISC_ERRORS
"PL internal error. GCC_PROGRAMS exceeded in GCC_COMP_MSGS.\
 Please see system administrator."

E_GC2451_CSET_FAIL:SS50000_MISC_ERRORS
"Character set initialization failed."
E_GC2452_BAD_CNAM:SS50000_MISC_ERRORS
"Errors found in GCCCSET.NAM file at line %0d."
E_GC2453_BAD_CXLT:SS50000_MISC_ERRORS
"Errors found in GCCCSET.XLT file at line %0d."
E_GC2454_BAD_LCL_CHARSET:SS50000_MISC_ERRORS
"II_CHARSET names a character set not found in GCCCSET.NAM."
E_GC2455_NO_LCL_MAPPING:SS50000_MISC_ERRORS
"GCCCSET.XLT has no mapping from the local character set to NSCS."

E_GC2461_CHARSET:SS50000_MISC_ERRORS
"Character set negotiation complete: local %0x, network %1x."
E_GC2462_NO_CHARSET:SS50000_MISC_ERRORS
"Character set negotiation failed: no common character set found."

E_GC2601_CONNCTN_END:SS50000_MISC_ERRORS
"Normal Session Connection termination."
E_GC2602_SL_FSM_INP:SS50000_MISC_ERRORS
"SL internal error: Invalid input event in SL FSM"
E_GC2603_SL_FSM_STATE:SS50000_MISC_ERRORS
"SL internal error: Invalid state transition in SL FSM"
E_GC2604_SL_FSM_STATE:SS50000_MISC_ERRORS
"SL internal error: Invalid state transition in SL FSM.\
 Input = %0d, state = %1d"
E_GC2605_SL_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Comm Server SL internal error. Please see system administrator."

E_GC2801_NTWK_CONNCTN_START:SS50000_MISC_ERRORS
"Normal Network Connection initiation."
E_GC2802_NTWK_CONNCTN_END:SS50000_MISC_ERRORS
"Normal Network Connection termination."
E_GC2803_PROT_ID:SS50000_MISC_ERRORS
"Invalid (unknown) protocol identifier %0c."
E_GC2804_PROT_ID:SS50000_MISC_ERRORS
"Invalid protocol identifier - user provided a vnode as part of the
database name (vnode::dbname), but the protocol identifier for that
vnode is invalid.  See the errlog.log for usable protocols and
enter a valid protocol using NETUTIL."
E_GC2805_TL_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Comm Server TL internal error. Please see system administrator."
E_GC2806_TL_FSM_INP:SS50000_MISC_ERRORS
"TL internal error: Invalid input event in TL FSM"
E_GC2807_TL_FSM_STATE:SS50000_MISC_ERRORS
"TL internal error: Invalid state transition in TL FSM"
E_GC2808_NTWK_OPEN_FAIL:SS50000_MISC_ERRORS
"Network open failed for protocol %0c, port %1c; status follows."
E_GC2809_NTWK_CONNECTION_FAIL:SS50000_MISC_ERRORS
"Network connection failed for protocol %0c, to node %1c, port %2c;\
 status follows."
E_GC280A_NTWK_INIT_FAIL:SS50000_MISC_ERRORS
"Network initialization failure: all protocol opens failed."
E_GC280B_TL_INIT_FAIL:SS50000_MISC_ERRORS
"GCC TL initialization failure."
E_GC280C_TL_FSM_STATE:SS50000_MISC_ERRORS
"TL internal error: Invalid state transition in TL FSM.\
 Input = %0d, state = %1d"
E_GC280D_NTWK_ERROR:SS50000_MISC_ERRORS
"Network connection lost: See errlog.log for detail."
E_GC280E_TL_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Comm Server TL internal error. Please see system administrator."
E_GC280F_OUTGNG_NTWK_CONNCTN:SS50000_MISC_ERRORS
"Outgoing network connection: remote connection id = %0x."
E_GC2810_INCMG_NTWK_CONNCTN:SS50000_MISC_ERRORS
"Incoming network connection: remote connection id = %0x."
E_GC2811_TL_INVALID_TPDU:SS50000_MISC_ERRORS
"TL internal error: Received invalid TPDU type %0x."
E_GC2812_TL_INVALID_CONNCTN:SS50000_MISC_ERRORS
"TL internal error: Received unknown connection id %0x from partner."
E_GC2813_NTWK_LSN_FAIL:SS50000_MISC_ERRORS
"Network listen failed for protocol %0c, port %1c; status follows."
E_GC2815_NTWK_OPEN:SS50000_MISC_ERRORS
"Network open complete for protocol %0c, port %1c."
E_GC2816_NOT_OPEN:SS50000_MISC_ERRORS
"Network connection not allowed on protocol %0c where the open had failed."
E_GC2817_NOT_OPEN:SS50000_MISC_ERRORS
"Network connection not allowed on a protocol where the open had failed."
E_GC2818_PROT_ID:SS50000_MISC_ERRORS
"Invalid (unknown) protocol(s) listed in II_GCC_PROTOCOLS; \
known protocols are: %0c."
E_GC2819_NTWK_CONNECTION_FAIL:SS50000_MISC_ERRORS
"Unable to establish network connection: See errlog.log for detail."
E_GC281A_TL_INV_TPDU_DATA:SS50000_MISC_ERRORS
"TL internal error: received invalid data format for TPDU type %0x."
E_GC281B_TL_INIT_ENCRYPT:SS50000_MISC_ERRORS
"Error initializing security mechanism %0d for encryption."
E_GC281C_TL_NO_ENCRYPTION:SS50000_MISC_ERRORS
"Unable to establish data encryption for session.  Network connection aborted."
E_GC281D_TL_ENCRYPTION_FAIL:SS50000_MISC_ERRORS
"Data encryption/decryption failure.  Network connection aborted.\
  See errlog.log for details."



E_GC2A01_INIT_FAIL:SS50000_MISC_ERRORS
"Protocol Bridge initialization failed."
E_GC2A02_NO_AUTH_INET:SS50000_MISC_ERRORS
"Protocol Bridge startup failure: No authorization for NET"
E_GC2A03_STARTUP:SS50000_MISC_ERRORS
"Protocol Bridge normal startup: rev. level %0c."
E_GC2A04_SHUTDOWN:SS50000_MISC_ERRORS
"Protocol Bridge normal shutdown."
E_GC2A05_PB_INTERNAL_ERROR:SS50000_MISC_ERRORS
"GCC Protocol Bridge internal error. Please see system administrator."
E_GC2A06_PB_FSM_STATE:SS50000_MISC_ERRORS
"PB internal error: Invalid state transition in PB FSM.\
 Input = %0d, state = %1d"
E_GC2A07_PB_FSM_INP:SS50000_MISC_ERRORS
"PB internal error: Invalid input event in PB FSM"
E_GC2A08_NTWK_INIT_FAIL:SS50000_MISC_ERRORS
"Network initialization failure: protocol open(s) failed."
E_GC2A09_MAX_PB_CONNS:SS50000_MISC_ERRORS
"GCC Protocol Bridge has exceeded max connections."
E_GC2A0A_PB_NO_VNODE:SS50000_MISC_ERRORS
"Protocol Bridge GCA_ERROR: gca_id_error %0x occurred while reading VNODE info."
E_GC2A0B_PB_GCN_INITIATE:SS50000_MISC_ERRORS
"Protocol Bridge: Call to  GCN_INITIATE failed."
E_GC2A0C_PB_GCN_RET_PRIV:SS50000_MISC_ERRORS
"Protocol Bridge: Call to  GCN_RET for private entries failed."
E_GC2A0D_PB_GCN_RET_PUB:SS50000_MISC_ERRORS
"Protocol Bridge: Call to  GCN_RET for public entries failed."
E_GC2A0E_PB_GCN_TERMINATE:SS50000_MISC_ERRORS
"Protocol Bridge: Call to  GCN_TERMINATE failed."
E_GC2A0F_NO_AUTH_BRIDGE:SS50000_MISC_ERRORS
"Protocol Bridge Server startup failure: No authorization for BRIDGE"
E_GC2A10_LOAD_CONFIG:SS50000_MISC_ERRORS
"Finished loading configuration parameters for configuration '%0c' of server
type '%1c'."

/*
** GCD Errors
*/

E_GC4000_BAD_URL:SS50000_MISC_ERRORS
"Target connection string improperly formatted."
E_GC4001_CONNECT_ERR:SS08001_CANT_GET_CONNECTION
"Communications error while establishing connection."
E_GC4002_PROTOCOL_ERR:SS40003_STMT_COMPL_UNKNOWN
"Connection aborted due to communications protocol error."
E_GC4003_CONNECT_FAIL:SS40003_STMT_COMPL_UNKNOWN
"Connection failed."
E_GC4004_CONNECTION_CLOSED:SS08003_NO_CONNECTION
"Connection closed."
E_GC4005_SEQUENCING:SS5000R_RUN_TIME_LOGICAL_ERROR
"New request made before prior request completed."
E_GC4006_TIMEOUT:SS5000R_RUN_TIME_LOGICAL_ERROR
"Operation cancelled due to time-out."
E_GC4007_BLOB_IO:SS50000_MISC_ERRORS
"I/O error during LOB processing."
E_GC4008_SERVER_ABORT:SS40003_STMT_COMPL_UNKNOWN
"Server aborted connection."
E_GC4009_BAD_CHARSET:SS50000_MISC_ERRORS
"Could not map server character set to local character encoding."

E_GC4010_PARAM_VALUE:SS22023_INV_PARAM_VAL
"Invalid parameter value."
E_GC4011_INDEX_RANGE:SS50000_MISC_ERRORS
"Invalid column or parameter index."
E_GC4012_INVALID_COLUMN_NAME:SS5000R_RUN_TIME_LOGICAL_ERROR
"Invalid column name."
E_GC4013_UNMATCHED:SS42000_SYN_OR_ACCESS_ERR
"Unmatched quote, parenthesis, bracket or brace."
E_GC4014_CALL_SYNTAX:SS42000_SYN_OR_ACCESS_ERR
"Invalid procedure call syntax."
E_GC4015_INVALID_OUT_PARAM:SS22023_INV_PARAM_VAL
"Output parameters not allowed with row-producing procedures"
E_GC4016_RS_CHANGED:SS22023_INV_PARAM_VAL
"Requested ResultSet type, concurrency and/or holdability not supported."
E_GC4017_NO_RESULT_SET:SS07005_STMT_NOT_CURS_SPEC
"Query or procedure does not return a result set."
E_GC4018_RESULT_SET_NOT_PERMITTED:SS21000_CARD_VIOLATION
"Query or procedure returns a result set"
E_GC4019_UNSUPPORTED:SS5000R_RUN_TIME_LOGICAL_ERROR
"Request is not supported."
E_GC401A_CONVERSION_ERR:SS07006_RESTR_DT_ATTR_ERR
"Invalid datatype conversion."
E_GC401B_INVALID_DATE:SS22007_INV_DATETIME_FMT
"DATE/TIME value not in standard format."
E_GC401C_BLOB_DONE:SS50000_MISC_ERRORS
"Request to retrieve LOB column which had already been accessed.\n
LOB columns may only be accessed once."
E_GC401D_RESULTSET_CLOSED:SS5000R_RUN_TIME_LOGICAL_ERROR
"ResultSet closed."
E_GC401E_CHAR_ENCODE:SS07006_RESTR_DT_ATTR_ERR
"Invalid character/string encoding."
E_GC401F_XACT_STATE:SS25000_INV_XACT_STATE
"Invalid transaction state for requested operation."
E_GC4020_NO_PARAM:SS07001_USING_PARM_MISMATCH
"The value for a dynamic parameter was not provided."
E_GC4021_INVALID_ROW:SS24000_INV_CURS_STATE
"Cursor is not positioned on a row for which the requested operation is supported."

E_GC4800_STARTUP:SS50000_MISC_ERRORS
"Data Access Server normal startup."
E_GC4801_SHUTDOWN:SS50000_MISC_ERRORS
"Data Access Server normal shutdown."
E_GC4802_LOAD_CONFIG:SS50000_MISC_ERRORS
"Finished loading configuration parameters for
configuration '%0c' of server type '%1c'."
E_GC4803_NTWK_OPEN:SS50000_MISC_ERRORS
"Network open complete for protocol %0c, port %1c."
E_GC4804_NTWK_CONFIG:SS50000_MISC_ERRORS
"No protocols enabled for Data Access Server."
E_GC4805_NTWK_RQST_FAIL:SS50000_MISC_ERRORS
"Network request '%0c' failed with error 0x%1x."
E_GC4806_NTWK_INIT_FAIL:SS50000_MISC_ERRORS
"Network initialization failure: all protocol opens failed."
E_GC4807_CONN_ABORT:SS50000_MISC_ERRORS
"Aborting client connection, reason follows."
E_GC4808_NO_MEMORY:SS50000_MISC_ERRORS
"Unable to dynamically allocate memory."
E_GC4809_INTERNAL_ERROR:SS50000_MISC_ERRORS
"An internal GCD Server error was detected."
E_GC480A_PROTOCOL_ERROR:SS50000_MISC_ERRORS
"Connection aborted due to a communications protocol error."
E_GC480B_NO_CLIENTS:SS50000_MISC_ERRORS
"The Data Access Server does not support this type of client connection."
E_GC480C_NO_AUTH:SS08004_CONNECTION_REJECTED
"A user ID and password are required to establish a connection."
E_GC480D_IDLE_LIMIT:SS50000_MISC_ERRORS
"Client has been idle longer than the configured limit: connection aborted."
E_GC480E_CLIENT_MAX:SS08004_CONNECTION_REJECTED
"Data Access Server has exceeded max client connections."
E_GC480F_EXCEPTION:SS50000_MISC_ERRORS
"Data Access Server shutting down due to a runtime exception."
E_GC4810_TL_FSM_STATE:SS50000_MISC_ERRORS
"GCD Server internal error:\n
     Invalid state transition in TL FSM.\n
     Input = %0d, state = %1d"
E_GC4811_NO_STMT:SS50000_MISC_ERRORS
"No open cursor exists matching the requested ID."
E_GC4812_UNSUPP_SQL_TYPE:SS50000_MISC_ERRORS
"Client sent an SQL datatype which is not supported by the server."
E_GC4813_XACT_AC_STATE:SS25000_INV_XACT_STATE
"Autocommit may not be enabled or disabled during a distributed transaction."
E_GC4814_XACT_BEGIN_STATE:SS25000_INV_XACT_STATE
"A distributed transaction can not be started in the current transaction state."
E_GC4815_XACT_PREP_STATE:SS25000_INV_XACT_STATE
"A 2PC PREPARE request was made without an active distributed transaction."
E_GC4816_XACT_REG_XID:SS50000_MISC_ERRORS
"An error occured registering a distributed transaction ID."
E_GC4817_CONN_PARM:SS50000_MISC_ERRORS
"Invalid connection parameter value."
E_GC4818_XACT_SAVEPOINT:SS25000_INV_XACT_STATE
"The savepoint specified does not exist."
E_GC4819_XACT_ABORT_STATE:SS25000_INV_XACT_STATE
"A distributed transaction can not be aborted in the current transaction state."
E_GC481A_XACT_END_STATE:SS25000_INV_XACT_STATE
"Association with an XA transaction can not end in the current transaction state."
E_GC481E_GCD_SESS_ABORT:SS50000_MISC_ERRORS
"Connection aborted by administrator request."  
E_GC481F_GCD_SVR_CLOSED:SS50000_MISC_ERRORS
"User connections disabled by an administrator."
E_GC4820_GCD_ADM_INIT_ERR:SS50000_MISC_ERRORS
"An error occurred during init GCADM inteface."
E_GC4821_GCD_ADM_SESS_ERR:SS50000_MISC_ERRORS
"An internal Data Access Server admin session error has been detected."

/*
** GCADM Errors
*/
E_GC5000_NOT_ACTIVE:SS50000_MISC_ERRORS
" GCADM is not active "
E_GC5001_NOT_IDLE:SS50000_MISC_ERRORS
" GCADM is already initialized. Cannot init again."
E_GC5002_NO_MEMORY:SS50000_MISC_ERRORS
"Unable to dynamically allocate memory."
E_GC5003_FSM_STATE:SS50000_MISC_ERRORS
"GCADM internal error:\n
     Invalid state transition in FSM.\n
     Input = %0d, state = %1d"
E_GC5004_INVALID_MSG:SS50000_MISC_ERRORS
"GCADM received unexpected message." 
E_GC5005_INTERNAL_ERROR:SS50000_MISC_ERRORS
"An internal GCADM  error was detected."	 
E_GC5007_INVALID_MD_PARMS:SS50000_MISC_ERRORS
"Invalid GCA_MD_ASSOC message parms detected."
E_GC5008_INVALID_MSG_STATE:SS50000_MISC_ERRORS
"Invalid message type received for the FSM state." 
E_GC5009_DUP_RQST:SS50000_MISC_ERRORS
"Duplicate requestes received."


/*
** General Processing Status
*/

E_GCFFFE_INCOMPLETE:SS50000_MISC_ERRORS
"Client must invoke service again."
E_GCFFFF_IN_PROCESS:SS50000_MISC_ERRORS
"Request is not complete"

NO_SQLSTATE_MAPPING
