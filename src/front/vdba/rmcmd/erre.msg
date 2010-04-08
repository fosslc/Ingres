/*
**  Copyright (c) 1999, 2000 Ingres Corporation
*/

/*
**  Name: errm.msg
**
**  Description:
**	Message definition file for the RMCMD Server.
**
**  History:
**	08-jun-1999 (somsa01)
**	    Created.
**	30-jun-1999 (somsa01)
**	    Changed RM to RE to match unique facility code for RMCMD.
**	19-oct-1999 (somsa01)
**	    Added more common messages from rmcmd.sc .
**	01-feb-2000 (somsa01)
**	    Added E_RE0011_NO_CLIENTS, E_RE0012_RMCMD_2MANY, and
**	    E_RE0013_BEDCHECK_FAIL, changed "Daemon" to "Server".
**  02-may-2001 (noifr01)
**      (RMCMD counterpart of SIR 103299) Added E_RE0014_WRONG_COLUMN 
**      (new message when rmcmd detects a wrong column name in the
**      remotecmd table: the tables should be upgraded in this case)
**  26-Jun-2001 (noifr01)
**      (SIR 105128) added messages related to the path change to
**      II_TEMPORARY
**  19-Sept-2002 (schph01)
**      (BUG 108642) added messages related to the 'local_vnode'
**      parameter in config.dat file.
**  01-Dec-2003 (schph01)
**     (SIR #111153) added messages related to the user privileges.
**  14-Oct-2004 (noifr01)
**     (bug 113217) added E_RE001D_READLOCK_MODE for the case the
**     set lockmode statement has failed
*/

#define _RE_CLASS 204

E_RE0001_RMCMD_UP
"Visual DBA RMCMD Server -- Normal Startup."
E_RE0002_RMCMD_DOWN
"Visual DBA RMCMD Server Normal Shutdown."
E_RE0003_STARTUP_USER
"The Visual DBA RMCMD Server must be started under the '%0c' account."
E_RE0004_STARTUP_CONNECT_FAIL
"Could not connect to the iidbdb database. The SQL error code was %0d."
E_RE0005_STARTUP_USER_INFO
"Could not retrieve user information. The SQL error code was %0d."
E_RE0006_STARTUP_RMCMD_OBJ
"Cannot access the RMCMD objects. The SQL error code was %0d."
E_RE0007_STARTUP_DBEVNT_NEWCMD
"Cannot register the rmcmdnewcmd dbevent. The SQL error code was %0d."
E_RE0008_STARTUP_DBEVNT_STP
"Cannot register the rmcmdstp dbevent. The SQL error code was %0d."
E_RE0009_STARTUP_CLEANUP_FAIL
"Cannot perform initial cleanup. The SQL error code was %0d."
E_RE000A_DAEMON_UPDATE_FAIL
"Cannot update the remotecmd table. The SQL error code was %0d."
E_RE000B_DAEMON_READ_FAIL
"Cannot read the remotecmd table. The SQL error code was %0d."
E_RE000C_DAEMON_EXEC_FAIL
"Error in executing a remote command. The SQL error code was %0d."
E_RE000D_DAEMON_STATUS_UPDATE
"Cannot update status to 'error' in the remotecmd table. The SQL error code was %0d."
E_RE000E_DAEMON_CREATE_PROC
"Create Process Error."
E_RE000F_DAEMON_GET_DBEVENT
"Cannot get dbevent. The SQL error code was %0d."
E_RE0010_DAEMON_INSERT_FAIL
"Cannot insert into the remotecmdout table. The SQL error code was %0d."
E_RE0011_NO_CLIENTS
"No clients are allowed to connect to the RMCMD Server."
E_RE0012_RMCMD_2MANY
"Visual DBA RMCMD server startup failed - another Visual DBA RMCMD server already running."
E_RE0013_BEDCHECK_FAIL
"Could not retrieve Name server registration information."
E_RE0014_WRONG_COLUMN
"rmcmd has detected that its catalogs have not been upgraded.  To perform this upgrade manually, please issue the following commands: rmcmdstp, rmcmdrmv, rmcmdgen and then restart the rmcmd server."
E_RE0015_CANNOT_SET_PATH
"could not set the rmcmd run path to II_TEMPORARY"
E_RE0016_CANNOT_RESTORE_PATH
"could not restore the rmcmd run path to II_TEMPORARY"
E_RE0017_CANNOT_GET_PATH_PERM
"Cannot get the path permission information for setting the rmcmd run path to II_TEMPORARY"
E_RE0018_NO_PATH_PERMISSION
"The II_TEMPORARY path doesn't have sufficient permissions for having rmcmd run on that path"
E_RE0019_CONFIG_SYNTAX_ERROR
"Syntax error detected in config.dat."
E_RE001A_CONFIG_OPEN_ERROR
"Unable to open config.dat file."
E_RE001B_LOCAL_VNODE_ERROR
"gcn.local_vnode resource was not found in config.dat."
E_RE001C_NO_PRIVILEGES
"User has insufficient privileges for starting the RMCMD server."
E_RE001D_READLOCK_MODE
"Cannot set the readlock mode. The SQL error code was %0d."


