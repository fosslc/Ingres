/********************************************************************
**
**  Copyright (C) 1995, 2008 Ingres Corporation
**
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmd.sc
**    remote commands daemon process
**
**
** History:
**
**	23-Sep-95 (johna)
**		Added MING hints and moved the 'main' to the start of the 
**		line so that mkming would recognize it.
**	05-Oct-95 (johna)
**		Removed the 'Pre-QA' message and start check. 
**		Added a call to FEcopyright
**	17-Oct-95 (Emmanuel Blattes)
**		Moved the call to FEcopyright so it does not appear twice in NT version
**		Kept the 'Pre-QA' message in comments for debug purposes
**  18-Oct-95 (Emmanuel Blattes)
**    For windows NT version :
**    Don't spawn in detached so we can kill rmcmd "by hand"
**  26-Oct-95 (Emmanuel Blattes)
**    Remove count system that made rmcmd stop after 100 commands
**  02-Nov-95 (Sotheavut Uk)
**    Remove termination tuple before exiting (RMCMDSTATUS_REQUEST4TERMINATE)
**
**      04-Jan-96 (spepa01)
**          	Add CL headers so that STxcompare()/STlength() can be used.
**	25-Jan-1996 (boama01)
**		Converted code for OpenVMS (entire new startup section).
**		Also first cut at CL-ization of code; see "*CL*" line
**		markings.  Also improve error sensing/recovery in command
**		servicing loop.
**	16-jul-96 (stoli02) bug 77934
**		Patch to support replicator 1.1 : Add imagerep and ddmovcfg.
**		Sorted the commands by product and alphabetic order.
**      16-oct-96 (noifr01 and uk$so01) bug 78600
**              Fixed the problem of remaining zombie processes 
**	02-jun-1997 (muhpa01)
**		Moved compat.h to head of include list to fix compile error
**	        complaint on sigaction struct (POSIX_DRAFT_4, compat.h contains
**	        #include for pthread.h)
**	15-sep-1997 (hanch04)
**		moved compat.h to be after time.h
**	24-sep-1997 (kinte01)
**		Added an include of compat.h to #else VMS since change of 2-jun 
**		moved compat.h so it was included by all platforms except VMS
**		which caused this routine to no longer compile on VMS
**      03-Nov-97 (fanra01)
**              Modified CreateProcess calls to use DETACHED_PROCESS flag.
**	08-dec-1997 (popri01)
**		Use Ingres systypes, a types.h wrapper, instead of including
**		types.h directly in order to avoid some OS typef collisions.
**	06-mar-1998 (somsa01)
**	    On NT, certain commands such as ckpdb spawn separate commands to
**	    get their work done. This, coupled with the DETACHED_PROCESS
**	    flag, cut-off the output to the VDBA window. Therefore, spawn
**	    the child process not as a detached process, but as a process
**	    with a minimized window.  (Bug 89435)
**
**  13-Mar-1998 (noifr01)
**      Added management for 3 "dummy" new commands (there is no executable 
**      for these. Instead, they are interpreted by rmcmd and code is executed 
**      directly (for speed reasons)):
**      -dddispf <databasename> <1|2> <serverno>:
**       outputs (in the rmcmd tables) the contents of a flat file:
**        * replicat.log if parameter 2 is FILE_REPLIC_LOG (1)
**        * runrepl.opt (for appropriate server subdirectory) if it is
**           FILE_RUNREPL (2)
**      -ddstoref <databasename> <serverno>   :
**       writes runrepl.opt (for corresponding server path) 
**      -ddstart <serverno> :
**       starts replicator server <serverno>
**      The connected user (username passed through the launchremotecmd procedure) 
**        must be INGRES or the DBA of the passed database, and the database needs
**		to have replic objects installed, for the ddispf and ddstoref commands to be
**      accepted.
**		For the ddstart command, the connected user must be INGRES or the DBA of the
**		localDB. If the runnode is different from the localDB Node (usually not
**      recommended), only INGRES will be allowed to perform the command.
**      Also removed the suppspace() function (did the same as STtrmwhite(), calls
**      to suppspace were replaced with calls to STtrmwhite)
**  25-Mar-1998 (noifr01)
**      -Added usage of dbevents to speed up rmcmd  
**  02-apr-1998 (noifr01)
**      added removal of rows with RMCMDSTATUS_REQUEST4TERMINATE
**      at startup, since if rmcmdstp has been executed before rmcmd, 
**      rmcmd should not exit immediatly at startup
**  26-jun-1998 (schph01)
**      ignore case in testing the existence of the dd_cdds table,
**      in order to work under SQL92
**  03-jul-1998 (noifr01)
**      -added 2 new replicator commands (repmod and arcclean) to the list
**       of commands accepted by rmcmd
**      -removed the limitation to the INGRES user of 4 replicator commands that 
**       now support the -u option (and apparently didn't support them at a point of time
**       in the past).
**       The commands are: repcat , dereplic, reconcil, and  makerep.
**       (this second change is only #ifdef INGRESII (in fact, these 4 commands
**       are in the list of "ingres user ony" commands #ifndef INGRESII)
**  16-jul-1998 (noifr01)
**      - added error management for some SQL statements where it was missing
**        (in particular in the "get dbvent with wait" statement, and a few others)
**      - added display of missing error messages under NT and UNIX, both to 
**        stdout and the error log file
**      - removed some obsolete commented code (additional cleanup remains to be done)
**	26-Aug-1998 (hanch04)
**	    Removed fork call for unix.  Let ingstart spawn it off.
**  03-Sep-1998 (noifr01) 
**      bug #92747
**		under UNIX : execute rpserver <serverno> to start a replicator
**      server (with wait), instead of "launching (with NOWAIT) replicat
**      after moving to the appropriate directory":
**      The old code (invoking replicat directly) was there because in
**      a first step, the usual "pipe" process between the command and
**      "rmcmdout" was used: but the & sign at the end of rpserver under
**		UNIX resulted in VDBA to hang because the pipe wasn't closed
**		(probably until the replic server was stopped).
**      But now rmcmdout and the pipe aren't used any more, due to
**		architectural (and visual) changes.	
**  25-Sep-1998 (schph01)
**      accept 2 new commands (relocatedb and upgradefe), needed by some
**      new vdba functionalities (altering database characteristics)
**  05-oct-1998 (kinte01)
**	  For VMS only, if a user has the SERVER CONTROL he can setup the
**	  RMCMD objects & start/stop the RMCMD server. 
**	  The above changes meant that all SQL queries referencing 
**	  ingres.<whatever> had to be rewritten in dynamic sql to be
**	  the user setting up RMCMD. For UNIX & NT this will always be
**	  ingres. For VMS this may or may not be ingres but will be the
**	  Ingres System Admin User.
**  07-Oct-1998 (schph01>
**      fastload and infodb added as well, for other new VDBA (2.6) functionalities
**  01-dec-1998 (chash01)
**        rewrite main() to remove extraneous create process code in VMS
**        part.  Also, rearrange code to make it easier to read.
**  06-Jan-1999 (noifr01)
**      accept the new "repcfg" replicator utility (used for the "activate"
**      and "createkeys" replicator functionalities in VDBA)
**  14-Dec-1998 (bonro01)
**      Fixed the problem of remaining zombie processes in UNIX.
**      Wait for the three child processes to exit. Check their return codes.
**      The VMS code that fixed this problem has been modified for UNIX.     
**  13-Jan-1999 (somsa01)
**	On NT (WIN32), sys\wait.h does not exist.
**  02-Feb-99 (noifr01)
**     specify the ingres server class when connecting, in order to support 
**     the case where the default server class is not ingres
**  18-feb-1999 (chash01)
**     revert back to old two-step startup process.  The reason being that the
**     single-step startup process, although clearer and obviously better, has
**     one major caveat, it does not work when put as a detached process.
**  11-Mar-1999 (bonro01)
**	Back out my previous change to fix the zombie process problem.
**	The VMS code that I enabled forced the RMCMD daemon to wait for a
**	command to complete before executing the next command.
**	The correct fix was originally made on 16-oct-96  by (noifr01 and
**	uk$so01). This fix was accidentally removed on 26-Aug-98.
**	This fix will replace the SIGCHLD signal handler for UNIX which
**	will issue wait() calls for child processes asynchronously.
**	Eliminate checking for -1 return code from execvp.  In all cases
**	just exit child process.  Checking for -1 return code was just
**	an invitation for more problems.  If execvp ever returned anything
**	besides -1 the child processes would begin forking new processes.
**  16-Mar-1999 (popri01)
**      On Unixware (usl_us5) types.h now included in stdio.h - include 
**	Ingres wrapper (systypes.h) first
**  22-Apr-1999 (noifr01)
**      if DD_RSERVERS is not found (when accessing replicator startup
**      settings runrepl.opt files), use  II_SYSTEM/ingres/rep, in order
**      to be consistent with the new behavior of replicator.
**  28-Apr-1999 (noifr01)
**      One remaining occurence of DD_RSERVERS was not managed 
**      consistently with the previous change (22-Apr-1999)
**      (when starting a replication server) Fixed
**  08-jun-1999 (somsa01)
**	Added a new function, log_message(), which will write messages to
**	the errlog. For now, startup and shutdown messages (as formatted
**	in a new message file, errm.msg) are written to it.
**  30-jun-1999 (somsa01)
**	Changed E_RM... messages to E_RE... to match new facility code for
**	RMCMD messages.
**  01-jul-1999 (somsa01)
**	Changed errm.h to erre.h .
**  14-Sep-1999 (marro11)
**	Relocated system includes to after ingres includes (as is typically 
**	done).  Drop in compat.h include.  Resolves compilation problem for 
**	rux_us5.
**  19-oct-1999 (somsa01)
**	Removed startup_error1() and daemon_error1() functions. Their uses
**	have been changed so that we now call log_message() in those
**	situations. Messages that were written by those functions have now
**	been formatted properly.
**  29-oct-1999 (mcgem01)
**      Add net1 to the list of commands that can be executed by rmcmd
**      for the TNG group.   This change is only built into Black Box Ingres.
**  29-nov-1999 (somsa01)
**	For the TNG group, connect to iidbdb as the 'ingres' user when
**	starting up RMCMD. This change is only built into Black Box Ingres.
** 31-jan-2000 (noifr01)
** (bug 100244) use the rpserver utility, rather than repstart, under NT,
** to start a replicator server
**  01-feb-2000 (somsa01)
**	Register the server with the Name server.
**  04-May-2000 (noifr01)
**  (bug 101430) when inserting a row in remotecmdout, now insert the value
**  for the new column that specifies if a carriage return is supposed to
**  end the line, or not
**  05-May-2000 (noifr01)
**  (bug 101440) added the new adbtofst utility in the list of accepted
**  commands
**  (also fixed timestamp error in the comment for the previous change)
**  11-may-2000 (somsa01)
**	Set up the PM defaults for the server.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-sep-2000 (somsa01)
**	Since we don't use TNG_EDITION anymore, we have to change this to
**	embed_user when deciding on the connection user.
**  11-oct-2000 (mcgem01)
**	The Ingres II SDK can be installed and run as any user, so it needs
**	to mimic the Embedded Ingres version.
**  06-oct-2000 (somsa01)
**	Increased the max size of a server id to 18.
**  03-Jan-2001 (noifr01)
**   (SIR #103625) now have rmcmd work against imadb rather than iidbdb
**   also fixed build warnings (removed unused variables, and fixed a 
**   missing cast of an u_i2 variable)
**  12-Apr-2001 (schph01)
**      add new command "usermod".
**  26-Apr-2001 (noifr01)
**   (RMCMD counterpart of SIR 103299) increased max lenght of command lines
**   to be executed, and renamed the corresponding column in the remotecmd 
**   table in order to ensure rmcmd under 2.6 doesn't work on tables that
**   wouldn't have been updated (otherwise the commands to be executed could
**   get truncated )
**  27-Apr-2001 (horda03)
**     On VMS determine the identity of the Ingres System Administrator and
**     connect to the database as that user. (Bug 100039).
**  02-May-2001 (noifr01)
**   (additional fix for the RMCMD counterpart of SIR 103299) provide a new,
**   specific  message in errlog.log when a wrong column name has been 
**   detected in the remotecmd table
**  11-May-2001 (noifr01)
**   (additional fix for SIR #103625) some queries(for replication management)
**   are doing a select in the iidatabase catalog of iidbdb.  These queries
**   were previously done in the main, current session since it was in iidbdb.
**   But now the main session is in imadb, and therefore we now switch to a
**   new session (in iidbdb) for these few "select" queries
**  26-Jun-2001 (noifr01
**   (SIR 105128) enforce the run path to II_TEMPORARY
**  06-Mar-2001 (kinte01)
**      Remove duplicate header definitions for VMS now that the files
**      are included generically
**  07-mar-2002 (chash01)
**      Bug 107279.  Inclusion of PRC$M_NOUAF resulted in hanging of entire
**      RMCMD processes when user entered a createdb from vdba.
**  04-Sep-2002 (schph01)
**      Bug 108642 Remove the call to TEname() and fill the static
**      variable strLocalVnode, with the PMload, PMget functions.
**      corresponding to the "gcn.local_vnode" parameter in config.dat file.
**  19-Sep-2002 (schph01)
**      Bug 106063 due to change in LOinfo function, now return errno
**      instead FAIL when the filename or path could not be found.
**  25-Jun-2003 (devjo01)
**	-  Handle case where II_TEMPORARY references a job/proc level
**	   intermediate logical leading to E_RE0018 startup failures
**	   under VMS. (b110474)
**	-  Change location of generated log files from BIN to FILES
**	   to match every other log file for Ingres.
**	17-sep-2003 (abbjo03)
**	   Changes to use VMS headers provided by HP.
**  22-Oct-2003 (schph01)
**     (SIR #111153) now have rmcmd run under any user id
**  01-Dec-2003 (schph01)
**     (SIR #111153) Change the E_RE001C_NO_PRIVILEGES error message.
**	11-dec-2003 (abbjo03)
**	    Reapply 7-mar-2002 change (bug 107279) which was overwritten by
**	    late cross-integration of 27-apr-2001 change. Fix 25-jun-2003
**	    change which inadvertantly caused rmcmd.exe to look for rmcmd.com
**	    in [ingres.files] rather than [ingres.bin] on VMS. Fix
**	    ManageFileDisplay() and ManageFileStorage() so they work
**	    correctly if II_SYSTEM is defined as a rooted logical. Fix
**	    ManageFileStorage so that it will write the VMS runrepl.opt.
**	21-Jan-2004 (rigka01)
**	    starting rmcmd results in E_UL0022 on Windows (2003) due to
**          uninitialized variable, user. 
**  10-Mar-2004 (fanra01)
**      SIR 111718
**      Add initialization to register configuration parameters
**      as IMA objects.
**	22-mar-2004 (abbjo03)
**	    Fix ManageStartReplicServer so it works correctly if II_SYSTEM is
**	    defined as a rooted logical.
**	18-Jun-2004 (raodi01)- INGDBA271(111643)
**    	    Uninitialized variable char *host initialized		
**  17-Jun-2004 (lazro01) (Problem 282 - Bug 112505)
**     Part of changes for SIR 111153 was applied only in #ifdef WIN32 
**     sections, and therefore the equivalent new functionality were missing on
**     UNIX platforms.
**  08-Jul-2004 (noifr01)
**     (sir 111718) added iisetres to the list of accepted commands
**  20-Jul-2004 (noifr01)
**     (sir 111718)
**     - (WIN32 only) fixed a performance issue that appeared only
**       on WIN32 platforms when using the IFFgetConfigValues() API
**       function
**     - added support of new iigetmdb dummy command, and of iigetres
**  19-Aug-2004 (hweho01)
**     Star #13608350
**     The MDB name will be included in MO object, remove iigetmdb
**     dummy command and reference of iigetres.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. Added NEEDLIBSW hint which is 
**	    used for compiling on windows and compliments NEEDLIBS.
**  12-Oct-2004 (noifr01)
**     (bug 113216) updated query that could fail with error -40100
**     (query is used to get only one row, and the fact that there
**     was no select loop resulted in the error if more than one
**     row was present. The fix ensures that no more than one row
**     is returned)
**  14-Oct-2004 (noifr01)
**     (bug 113217)
**     in order to avoid deadlock situations upon concurrent rmcmd requests,
**     set lockmode session where readlock = exclusive
**  23-feb-2005 (stial01)
**     Explicitly set isolation level after connect
**  16-mar-2005 (mutma03)
**     Star issue 13958829 nicknames not honoured, added code to translate
**     nodename to nickname for clustered nodes.
**	23-jun-2005 (abbjo03)
**	    Increase the size of the command mailbox device name buffer to
**	    prevent failures on VMS 8.2, which can allocate mailbox numbers
**	    with more than four digits.
**	14-sep-2005 (abbjo03)
**	    Remove extra declaration of 'user' variable in main() which was
**	    made redundant by cross-integration of 21-jan-2004 change to have
**	    the variable initialized.
**  04-oct-2005 (devjo01) 14421999;01
**     Added timestamp to error message.  Note: rmcmd messages should
**     be localized at some point.
**  24-Oct-2005 (fanch01) 14335872;01
**     rmcmdstp shuts down rmcmd processes on all nodes regardless of where
**     it was executed.  Added dbevents and table to act as moderation
**     object.  The original message is provided for compatibility purposes
**     and is turned into an "all node rmcmd stop" dbevent.  The new message
**     can target specific nodes and is used by rmcmdstp.  The message is
**     also turned into a dbevent.
**
**     Only ONE rmcmd process processes rmcmd commands at any given time in
**     the cluster.  The rmcmd process who owns the exclusive lock on
**     remotecmdlock is considered the 'master' and performs rmcmd actions.
**
**     On VMS #ifdef's were wrapped around log_message calls but also
**     'goto end' statements.  This made VMS ignore sql error codes.
**     Bug: 115442
**  12-Mar-2007 (hanal04) Bug 117859
**     Ensure ERRORTYPE is set to genericerror after connecting to
**     avoid unexpected failures. This is consistent with other
**     CONNECTs in Ingres utilities.
**  26-Apr-2007 (hanal04) Bug 118196
**     Include PID in error message headers. On Linux we need to make
**     sure we do not pick up iirun's pid.
**  07-jan-2008 (joea)
**     Reverse 16-mar-2005 cluster nicknames change.
**  23-May-2008 (drivi01)
**     Added CVlower statement before MEcmp to make sure that
**     on SQL-92 installation, MEcmp returns correct results.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**  15-Apr-2009 (hanal04) Bug 121931
**     Make sure we disconnect all sessions in order to ensure 
**     we clean-up socket files when using Unix sockets.
**     If we could not connect make sure we GCA_TERMINATE against the
**     gca_cb for the same reason.
**  03-Sep-2009 (frima01) 122490
**     Add various includes
**	09-Sep-2009 (bonro01) bug 122490
**	    Previous change causes build problems on Windows
**	    Exclude the headers from Windows.
**  25-Aug-2010 (drivi01) Bug #124306
**      Remove hard coded length of szCommand buffer.
**      Rmcmd buffer should be able to handle long ids, expand
**      the buffer accordingly.
*/
/*
PROGRAM =       rmcmd

UNDEFS = 	II_copyright

NEEDLIBS =      SHFRAMELIB SHQLIB SHCOMPATLIB SHEMBEDLIB

NEEDLIBSW = 	SHGCFLIB
*/
/*
		Note that MALLOCLIB should be added when this code 
		has been CLized
*/


/** Make sure SQL areas are included before compat.h to prevent GLOBALDEF
 ** conversion of their 'extern' definitions on VMS:
 **/
/* includes SQL */
exec sql include sqlca;
exec sql include sqlda;

#include <compat.h>

/* *CL* -- New includes for CL routines -- */
#include <cv.h>
#include <er.h>
#include <gc.h>
#include <gl.h>
#include <gv.h>
#include <id.h>
#include <me.h>
#include <nm.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <te.h>
#include <tm.h>
#include <lo.h>
#include <pc.h>  
#include <iicommon.h>  
#include <gca.h>  
#include <gcn.h>  
#include <gcu.h>  

#ifndef NT_GENERIC
#include <clconfig.h>
#include <cscl.h>
#include <csnormal.h>
#include <cx.h>
#include <gvcl.h>
#endif

#ifndef VMS

static STATUS chk_priv( char *user_name, char *priv_name )
{
        char    pmsym[128], *value, *valueptr ;
        char    *strbuf = 0;
        int     priv_len;

        /* 
        ** privileges entries are of the form
        **   ii.<host>.privileges.<user>:   SERVER_CONTROL, NET_ADMIN ...
        */

        STpolycat(2, "$.$.privileges.user.", user_name, pmsym);

        /* check to see if entry for given user */
        /* Assumes PMinit() and PMload() have already been called */

        if( PMget(pmsym, &value) != OK )
            return E_GC003F_PM_NOPERM;

        valueptr = value ;
        priv_len = STlength(priv_name) ;

        /*
        ** STindex the PM value string and compare each individual string
        ** with priv_name
        */
        while ( *valueptr != EOS && (strbuf=STindex(valueptr, "," , 0)))
        {
            if (!STscompare(valueptr, priv_len, priv_name, priv_len))
                return OK ;

            /* skip blank characters after the ','*/
            valueptr = STskipblank(strbuf+1, 10); 
        }

        /* we're now at the last or only (or NULL) word in the string */
        if ( *valueptr != EOS  && 
              !STscompare(valueptr, priv_len, priv_name, priv_len))
                return OK ;

        return E_GC003F_PM_NOPERM;
}
#endif

#ifndef VMS		/* (Eventually, omit for all platforms) */
#include <time.h>
#include <compat.h>
#include <systypes.h>
#include <stdio.h> 
#ifndef WIN32
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>	/* S.UK Oct 14' 1996 */
#else
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#endif

#else			/* (Temporarily for VMS until full CL-ization) */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <processes.h>
#define __STDIO_LOADED 1   /*** Fool unixio into omitting stdio.h ***/
#include <unixio.h>
#include <signal.h>
#include <compat.h>
#endif  /* VMS */

/* -- New includes for VMS system services -- */
#ifdef VMS		/* (Temporarily for VMS until full CL-ization) */
#include <mu.h>
#include <descrip.h>
#include <accdef.h>
#include <acldef.h>
#include <armdef.h>
#include <chpdef.h>
#include <climsgdef.h>
#include <dvidef.h>
#include <efndef.h>
#include <iledef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <jpidef.h>
#include <lnmdef.h>
#include <prcdef.h>
#include <pscandef.h>
#include <psldef.h>
#include <ssdef.h>
#include <uaidef.h>
#include <starlet.h>
#include <lib$routines.h>
#include <vmstypes.h>
#ifdef __ALPHA
#pragma member_alignment __save
#pragma nomember_alignment
#endif  /* __ALPHA */

# define $DESCALLOC( name ) struct dsc$descriptor_s \
        (name) = {0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL}

# define $DESCINIT( name, string ) \
        ((void)((name).dsc$a_pointer = (char *)(string),\
        (name).dsc$w_length = STlength(string)))

#define DEVNAM_L	64

#endif  /* VMS */


#include "rmcmd.h"
#include "rmcmdgca.h"
#include "erre.h"

FUNC_EXTERN STATUS	IIgcn_check(void);

static void  ManageFileDisplay(char *,int);
static void  ManageFileStorage(char *,int);
static bool  ManageStartReplicServer(char *, int);
static bool  ManageVerifyRmcmdStartUser(char *, int);
static bool  ManageRmcmdStopNode(char * szcommand, int hdl);
static void  log_message(i4, int);
static void  gca_sm(i4);
static STATUS	rmcmd_bedcheck(void);
static i4	gcn_response(i4 msg_type, char *buf, i4 len);


#define MAX_LOCAL_VNODE 64
static char  strLocalVnode[MAX_LOCAL_VNODE+1];
static char  RmcmdStartUser[MAX_USER_NAME_LEN];

#define  rmcmdddispf    "dddispf"
#define  rmcmdddstoref  "ddstoref"
#define  rmcmddddstart  "ddstart"
#define  rmcmdprocuser  "ddprocessuser"
#define  rmcmdstpnode	"rmcmdstpnode"

/*
** Static defines used for GCA processing.
*/
static char	ServerID[19];
static char     ServerPID[10];
static PTR	gca_cb;
static bool	listening = FALSE;
static GCA_LCB	gca_lcb_tab[LCB_MAX] ZERO_FILL;
static i4	labels[LBL_MAX] ZERO_FILL;
static bool	ServerUp = FALSE;

static char * rmcommands[]={ "alterdb",		/* Ingres commands */
                             "auditdb",
                             "ckpdb",
                             "copydb",
                             "createdb",
                             "destroydb",
                             "optimizedb",
                             "rollforwarddb",
                             "statdump",
                             "sysmod",
                             "unloaddb" ,
                             "verifydb",
                             "ddbldsvr",	/* Replicator commands */
                             "ddgensup",
                             "ddgenrul",
                             "ddmovcfg",
                             "dereplic",
                             "imagerep",
                             "makerep",
                             "reconcil",
                             "repcat",
                             "repmod",
                             "arcclean",
                             "upgradefe",	 /* used when "altering a database" in VDBA */
                             "relocatedb",	 /* used when "altering a database" in VDBA */
                             "fastload",
                             "infodb",
                             "repcfg",
                             "adbtofst",
                             "net1",
                             "usermod",
                             "iisetres",
                             rmcmdddispf,    /* just for information, because of special */
                             rmcmdddstoref,  /* security management (IscommandAccepted() */
                             rmcmddddstart,  /* not invoked for these 3 "dummy" commands */
			     rmcmdprocuser,
                             rmcmdstpnode    /* rmcmd on node stop command */
                             };

static char * rmingrescommands[]={"alterdb" ,
                                  "sysmod"  ,
                                  "relocatedb", /* used when "altering a database" in VDBA */
                                  "net1"  ,
                                  "iisetres",
#ifndef INGRESII
                                  "repcat"  ,
                                  "dereplic",
                                  "reconcil",
                                  "makerep" ,
#endif
                                 };

#ifndef WIN32		/* S.UK Oct 14' 1996 */
#ifndef VMS
struct sigaction action;
void EliminateZombie (int sig) {wait (NULL);}
#endif
#endif

/* Following are possible return codes from daemon failing to start up */
#ifdef VMS
#define DAEMON_FAILED_NO_IIDBDB		100
#define DAEMON_FAILED_NO_TABLES		101
#define DAEMON_FAILED_BAD_USER		RES_NOTINGRES
static char rmlogbuf[1024];

/* Name: get_username  - Gets the username from a UIC.
**
** Description:
**       This routine converts the UIC pased in into a username.
**
** Inputs:
**      uic   - UIC value
**
** Outputs
**      username - The username.
*/
static STATUS
get_username(
unsigned long uic,
char *username)
{
   $DESCALLOC( userdsc );

   userdsc.dsc$a_pointer = username;
   userdsc.dsc$w_length  = 50;

   return sys$idtoasc( uic, 0, &userdsc, 0, 0, 0);
}

   
/*
** Name: iics_cvtuic    - Convert string to VMS User Identification Code (UIC)
**
**                        An exact copy of the function in IIRUNDBMS.C
**
** Description:
**      This routine converts the string passed in to a longword binary UIC 
**      value.  UIC's can come in a number of forms, namely: 
**              [<octal_number>, <octal_number>]
**              [<string_identifier>, <string_identifier>]
**              [<string_identifier>]
**              <string_identifier>
**
** Inputs:
**      uic_dsc                         String descriptor passed in
**
** Outputs:
**      uic_ptr                         place to put output answer
**      Returns:
**          {@return_description@}
**      Exceptions:
**          none
**
** Side Effects:
**          none
*/
static STATUS
iics_cvtuic(
struct dsc$descriptor *uic_dsc,
i4                *uic_ptr)
{
    char                *uic_str = uic_dsc->dsc$a_pointer;
    STATUS              status;
    int                 group, owner;
    int                 count;
    char                c;
    struct dsc$descriptor itmlst[] = {
                            { sizeof(owner), UAI$_UIC, &owner, 0},      
                            { 0, 0, 0, 0}
                        };
    
    /*
    ** In the new world of PM configuration parameters, a user may set the
    ** vms_uic field to a string of length 0, thusly:
    **      ii.*.dbms.*.vms_uic:
    ** In this case, we need to handle this as valid, and meaning "uic=0"
    */
    if (uic_dsc->dsc$w_length == 0)
    {
        *uic_ptr = 0;
        return (SS$_NORMAL);
    }

    for (;;)
    {
        if ((*uic_str != '[') && (*uic_str != '<'))
        {
            /* This is the "username" case. */
            status = sys$getuai(0, 0, uic_dsc, itmlst, 0, 0, 0);
            *uic_ptr = owner;
            break;
        }
        uic_str++;
        uic_dsc->dsc$a_pointer++;
        for (c = *uic_str, count = 0;
                (c != ',' && c != ']' && c != '>' && c != 0);
                c = *(++uic_str), count++)
            ;
        uic_dsc->dsc$w_length = count;
        status = ots$cvt_to_l(uic_dsc, &group, sizeof(group), 0);
        if ((status & 1) == 0)
        {
            status = sys$asctoid(uic_dsc, &group, 0);
            if ((status & 1) == 0)
            {
                break;
            }
        }
        else
        {
            group <<= 16;
        }
        if (c && c != ',')
        {
            /* [ alpha ] or [ nnn ] */
            *uic_ptr = group;
            status = SS$_NORMAL;
            break;
        }
        uic_dsc->dsc$a_pointer = ++uic_str;
        for (count = 0;
                *uic_str && *uic_str != ']' && *uic_str != '>';
                uic_str++, count++)
            ;
        uic_dsc->dsc$w_length = count;
        status = ots$cvt_to_l(uic_dsc, &owner, sizeof(owner), 0);
        if ((status & 1) == 0)
        {
            status = sys$asctoid(uic_dsc, &owner, 0);
            if ((status & 1) == 0)
            {
                break;
            }
            if ((group & 0xffff0000) != (owner & 0xffff0000))
            {
                status = SS$_IVIDENT;
                break;
            }
        }
        *uic_ptr = ((group & 0xffff0000) | (owner & 0x0000ffff));
        return(SS$_NORMAL);
    }
    return(status);
}
#endif  /* VMS */

/* *CL* Following rtn rendered obsolete by CVlower ...
void stringtolower(char *pc)
{
  while (*pc){
    *pc=tolower(*pc);
    pc++;
  }
}
... */


BOOL isCommandAccepted(char * lpusername, char * lpcommand)
{
  char command[2000];
  char buf[100];
  int i;
  BOOL bLocal;

  if ( ( STlength(lpcommand)+1 ) > sizeof(command) )
    return FALSE;

  STcopy( lpcommand, command );
  CVlower( command );

  /*** check if command in the list ***/
  bLocal=FALSE;
  for (i=0;i< (sizeof(rmcommands)/sizeof (char *));i++)
  {
      STprintf( buf, ERx("%s "), rmcommands[i] );
      if ( !MEcmp( buf, command, STlength(buf) ) )
          bLocal=TRUE;
  }
  if (!bLocal)
      return FALSE;

  /*** check if user should be ingres ***/
  bLocal=FALSE;
  for (i=0;i< (sizeof(rmingrescommands)/sizeof (char *));i++)
  {
      STprintf( buf, ERx("%s "), rmingrescommands[i] );
      if ( !MEcmp( buf, command, STlength(buf) ) )
      bLocal=TRUE;
  }
  if (bLocal) 
  {
      if ( STbcompare( lpusername, 0, RmcmdStartUser, 0, TRUE) )
           return FALSE;
  }
  else 
  {
      char * pc1;
      char * pc2;

      STprintf( buf, ERx(" -u%s"), lpusername );
      CVlower( buf );
      pc1=STindex( command, ERx("\""), 0 );
      pc2=strstr(command,buf);
      if (!pc2)
          return FALSE;
      if (pc1)
      {
          if (pc2>pc1)
          return FALSE;
      }
  }
  return TRUE;
}

#ifdef VMS		/* Startup mgmt routines for VMS follow... */
#include <rms.h>
/*{
** Name: rmcmd_log	- Send message to the rmcmd logger.
**
** Description:
**	This procedure sends a message to the rmcmd specific error
**	logger.
**
** Inputs:
**      message                         Address of buffer containing the message.
**      msg_length                      Length of the message.
**
** Outputs:
**	Returns:
**	    OK or !OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-sep-1992 (pholman)
**	    First created, taken from original (6.4) version of ERsend
**      07-dec-1998 (chash01)
**          steal and modified from ERlog()
*/
STATUS
rmcmd_log(message, msg_length)
char               *message;
i4		   msg_length;
{
    static int		er_ifi = 0;
    static int		er_isi = 0;
    static int		er_open_attempted = 0;
    STATUS		status;
    struct FAB		fab;
    struct RAB		rab;

    /*	Check for bad paramters. */

    if (message == 0 || msg_length == 0)
	return (!OK);

    /*	Make sure that the channel is open. */

    if (er_open_attempted == 0 && er_ifi == 0)
    {
	/*  Dont't try and open every time if the first time failed. */

	er_open_attempted++;

	/*  Open RMS file. */

	MEfill(sizeof(fab), 0, &fab);
	MEfill(sizeof(rab), 0, &rab);
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$l_fna = "II_SYSTEM:[INGRES.FILES]RMCMD.LOG";
	fab.fab$b_fns = STlength(fab.fab$l_fna);
	fab.fab$b_rfm = FAB$C_VAR;
	fab.fab$b_rat = FAB$M_CR;
	fab.fab$l_fop = FAB$M_CIF;
	fab.fab$b_shr = FAB$M_SHRPUT | FAB$M_SHRGET;
	status = sys$create(&fab);
	if ((status & 1) == 0)
	    return (!OK);
	rab.rab$b_bid = RAB$C_BID;
	rab.rab$b_bln = RAB$C_BLN;
	rab.rab$l_fab = &fab;
	rab.rab$l_rop = RAB$M_EOF;
	status = sys$connect(&rab);
	if ((status & 1) == 0)
	{
	    sys$close(&fab);
	    return (!OK);
	}
	er_ifi = fab.fab$w_ifi;
	er_isi = rab.rab$w_isi;
    }

    if (er_isi)
    {
	MEfill(sizeof(rab), 0, &rab);
	rab.rab$b_bid = RAB$C_BID;
	rab.rab$b_bln = RAB$C_BLN;
	rab.rab$w_isi = er_isi;
	rab.rab$l_rbf = message;
	rab.rab$w_rsz = msg_length;
	status = sys$put(&rab);
	if ((status & 1) == 0)
	    return (!OK);
	status = sys$flush(&rab);
	if ((status & 1) == 0)
	    return (!OK);
	return (OK);
    }

    return (!OK);	
}
/*
** Name: startup_error     - write error from daemon startup phase to stdout
**
** Description:
**	This routine writes error messages from the daemon startup phase
**	to the stdout device.  This is the only phase during which this
**	can be done (while the interactive user waits for a response on
**	the terminal).  If a VMS status code is supplied, it is translated
**	and written out on a second line.
**
** Inputs:
**	code		- VMS status code, or zero
**	usrbuf		- user-supplied message string
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	25-Jan-1996 (boama01)
**      23-feb-1999 (chash01)
**          write (error) message into termination mailbox.
*/
static void
startup_error( const unsigned int code,
	       const char *usrbuf )
{
    unsigned int status;
    unsigned short len;
    IOSB	iosb;
    i4    	unit_num = 0;
    II_VMS_CHANNEL	term_chan;
    char	term_name[16], buf[16];
    ACCDEF	accmsg;
    ACCDEF2	*acc2;
    $DESCRIPTOR( term_dsc, term_name );
    ILE3  itmlst[] =
    {
        { sizeof(i4), JPI$_TMBU, &unit_num, 0},
        { 0, 0, 0, 0}
    };
 
    /*
    ** Signal startup program - RUNDBMS - that we have encountered 
    ** problems. startup_error is always followed by return
    */

    status = sys$getjpiw(EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, NULL);
    if ( status & 1 ) 
         status = iosb.iosb$w_status;
    if ( !(status & 1) )
	return;

    CVla( unit_num, buf );
    (*((long *)term_name)) = 'MBA\0';
    STcopy(buf, &term_name[3]);
    term_dsc.dsc$w_length = STlength(term_name) + 1;
    term_name[term_dsc.dsc$w_length - 1] = ':';
    term_dsc.dsc$a_pointer = term_name;
    term_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    term_dsc.dsc$b_class = DSC$K_CLASS_S;

    status = sys$assign( &term_dsc, &term_chan, NULL, NULL );
    if ( status & 1 )
    {
        i4  len;

        len = STlength(usrbuf);
	accmsg.acc$l_finalsts = code;
        acc2 = (ACCDEF2 *)&accmsg;
        len = min(len, sizeof(acc2->acc$t_user_data)-1); 
	STcopy( usrbuf, &acc2->acc$t_user_data);
	status = sys$qiow(EFN$C_ENF, term_chan, IO$_WRITEVBLK | IO$M_NOW, &iosb,
			   NULL, NULL, &accmsg, sizeof(accmsg), 0, 0, 0, 0 );

	(VOID)SYS$DASSGN( term_chan );
    }
    return;
}

/*
** Name: rmcmd_started - write startup message to termination mailbox and
**                       send message back to iirundbms.
**
** Description:  This routine is similar to startup_error execpt this one
**               sends successful startup message (process name and process
**               id) to the starting process iirundbms.
**
** Inputs:
**	prcname		- process name
**	pid		- process id
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**      19-feb-1999 (chash01)
**          write startup messages into termination mailbox
*/
static void
rmcmd_started(char *prcname, u_i4 pid)
{
    unsigned int status;
    unsigned short len;
    char msgbuf[256];
    IOSB	iosb;
    i4    	unit_num = 0;
    II_VMS_CHANNEL	term_chan;
    char	term_name[16], buf[16];
    ACCDEF	accmsg;
    ACCDEF2	*acc2;
    $DESCRIPTOR( msgdesc, msgbuf );
    $DESCRIPTOR( term_dsc, term_name );
    ILE3  itmlst[] =
    {
        { sizeof(i4), JPI$_TMBU, &unit_num, 0},
        { 0, 0, 0, 0}
    };
 
    /*
    ** Signal startup program - RUNDBMS - that we have encountered 
    ** problems. startup_error is always followed by return
    */

    status = sys$getjpiw(EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, NULL);
    if ( status & 1 ) 
         status = iosb.iosb$w_status;
    if ( !(status & 1) )
	return;

    CVla( unit_num, buf );
    (*((long *)term_name)) = 'MBA\0';
    STcopy(buf, &term_name[3]);
    term_dsc.dsc$w_length = STlength(term_name) + 1;
    term_name[term_dsc.dsc$w_length - 1] = ':';
    term_dsc.dsc$a_pointer = term_name;
    term_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    term_dsc.dsc$b_class = DSC$K_CLASS_S;

    status = sys$assign( &term_dsc, &term_chan, NULL, NULL );
    if ( status & 1 )
    {
        /*
        ** put both pid and prcname into t_user_data field
        ** iirundbms must hnalde this special case for rmcmd.
        */
	accmsg.acc$l_finalsts = 0;
        acc2 = (ACCDEF2 *)&accmsg;
        STprintf(acc2->acc$t_user_data, "%d %s", pid, prcname);       
	status = sys$qiow(EFN$C_ENF, term_chan, IO$_WRITEVBLK | IO$M_NOW, &iosb,
			   NULL, NULL, &accmsg, sizeof(accmsg), 0, 0, 0, 0 );

	(VOID)SYS$DASSGN( term_chan );
    }
    return;
}


/*
** Name: daemon_error	- write error from daemon command exec to Ingres errlog
**
** Description:
**	This routine writes errors encountered during daemon remote command
**	execution to the Ingres error log.  It writes two lines; one to
**	identify the daemon and the client command being processed, and the
**	second to provide specifics as supplied to this routine.
** IMPORTANT NOTE! This function is temporarily scaffolded to write to the
** daemon SYS$OUTPUT device (see "daemon_output" below).  Eventually,
** it should interface with ER* routines to output to the Ingres errlog.
**
** Inputs:
**	hdl		- identifier "handle" for command in remotecmd table
**	cmd		- string of command to be run
**	user		- string of userid for whom command was being run
**      usrbuf		- user-supplied string for 2nd message line
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	25-Jan-1996 (boama01)
**	    Created.
**	06-mar-1998 (somsa01)
**	    On NT, certain commands such as ckpdb spawn separate commands to
**	    get their work done. This, coupled with the DETACHED_PROCESS
**	    flag, cut-off the output to the VDBA window. Therefore, spawn
**	    the child process not as a detached process, but as a process
**	    with a minimized window.  (Bug 89435)
**	04-oct-2005 (devjo01) 14421999;01
**	    Added timestamp to error message.
*/
static void
daemon_error( int hdl,
	    char *cmd,
	    char *user,
	    char *usrbuf )
{
	STprintf( rmlogbuf, "%@ Error for handle(%d) user(%s) command(%s): %s",
		hdl, user, cmd, usrbuf );
        rmcmd_log(rmlogbuf, STlength(rmlogbuf));
}
/*
** Name: cleanup     - terminate mailbox channels, events
**
** Description:
**	This routine terminates the mailbox and event flag associations made
**	during the daemon startup phase.  It should be called before the
**	logic for this phase returns to the user.
**
** Inputs:
**	mbox1, mbox2	- optional mailbox(es) to be deassigned
**      cef		- optional common event flag to dissociate cluster
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	25-Jan-1996 (boama01)
*/
static void
cleanup( unsigned short mbox1,
	 unsigned short mbox2,
	 unsigned int   cef )
{
	if (mbox1)	SYS$DASSGN( mbox1 );
	if (mbox2)	SYS$DASSGN( mbox2 );
	if (cef)	SYS$DACEFC( cef );
}
#endif /* VMS */

/*
** Name: SetPath2ii_temporary - change current working dir to a scratch area.
**
** Description:
**	This routine sets the CWD to one specified by II_TEMPORARY, or failing
**	that, II_SYSTEM:[INGRES.FILES].  This is to allow the user to specify
**	where any temporary files generated by the remote command being serviced
**	will be created.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	25-Jun-2003 (devjo01)
**	   Added banner.  Allow II_SYSTEM:[INGRES.FILES] as alternate
**	   location, JIC user has defined II_TEMPORARY to a logical
**	   (E.g.) SYS$SCRATCH which is not available to the detached
**	   RMCMD process.
*/
#ifdef VMS
static bool SetPath2ingres_files()
{
    LOCATION	loc;
    LOINFORMATION locinfo;
    char	path   [MAX_LOC+1];
    i4	locflags;

	STcopy(ERx("II_SYSTEM:[INGRES.FILES]"), path);
	LOfroms(PATH, path, &loc);


	locflags = LO_I_PERMS;
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
	    log_message(E_RE0017_CANNOT_GET_PATH_PERM, 0);
		return FALSE;
	}
	if (! (locflags & LO_I_PERMS) ) {
	    log_message(E_RE0018_NO_PATH_PERMISSION, 0);
		return FALSE;
	}

    /* change path */
    if (LOchange(&loc)!=OK) {
	    log_message(E_RE0015_CANNOT_SET_PATH, 0);
		return FALSE;
	}

	return TRUE;

}
#endif /* VMS */
static bool SetPath2ii_temporary()
{
    LOCATION	loc;
    LOINFORMATION locinfo;
    char	path   [MAX_LOC+1];
    i4	locflags;
	char *p;
    
    NMgtAt(ERx("II_TEMPORARY"), &p);
	if (p == NULL || *p == EOS)
		return FALSE;

	STlcopy(p, path, sizeof(path)-1);
	LOfroms(PATH, path, &loc);


	locflags = LO_I_PERMS;
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
#ifdef VMS
	return SetPath2ingres_files();
#else
	log_message(E_RE0017_CANNOT_GET_PATH_PERM, 0);
	return FALSE;
#endif /* VMS */
	}
    if (! (locflags & LO_I_PERMS) ) {
#ifdef VMS
	return SetPath2ingres_files();
#else
	log_message(E_RE0018_NO_PATH_PERMISSION, 0);
	return FALSE;
#endif /* VMS */
	}

    /* change path */
    if (LOchange(&loc)!=OK) {
#ifdef VMS
	return SetPath2ingres_files();
#else
        log_message(E_RE0015_CANNOT_SET_PATH, 0);
	return FALSE;
#endif /* VMS */
	}

	return TRUE;

}

#ifdef WIN32
void main (int argc, char * argv[])
#else
#  ifdef VMS
int main (int argc, char * argv[])
#  else
main()
#  endif
#endif
{
    exec sql begin declare section;
        int hdl;
        char szCommand [RMCMDLINELEN*15+4000];
        char    stmt [2048];
        char  username[100];
        char  *user=NULL;
        char  *processuser;
        int   status1, status2;
        int   status = 0;
    exec sql end declare section;

    GCA_PARMLIST	parms;
    DB_STATUS		func_stat;
    char config_string[256];
    char *host = NULL;
    char *value = NULL;
    i4 cx_node_number;
    int master = 0;
    PID proc_id;
    char fmt[20];

#ifdef WIN32
		i4 i4HighReactivity = 0;
#endif

    cx_node_number = CXnode_number(NULL);
    if (cx_node_number < 0) {
	cx_node_number = 0;
    }

#ifdef VMS
/** The entire VMS prologue code is designed so that this executable can be
 ** run either to start the RMCMD daemon, or as the daemon itself.  This
 ** replicates the function of the "fork" and "setsid" calls, which are not
 ** implemented the same on VMS.  Full technical specs are available
 ** separately.  (boama01, 25-Jan-1996)
 **/
    char	buf [4100];
    long	procid;
    char	*inst, *inst2, instcode[2];
    char	ii_system[256], logdev[256], natdev[256];
    char	procname[16], thisproc[16];
    char	uicstr [256], *uicptr;
    int 	context = 0, pidret, namret, grpid, grpidret;
    SYSTIME	time_now;
    IOSB	iosb;
    /* 
    ** Following names a common event flag cluster and flag numbers for
    ** daemon/invoker communication:
    */
    unsigned int uic = 0;
    $DESCALLOC( uicdsc );
    $DESCRIPTOR( rmcmd_cluster, "RMCMD");
    unsigned int flag_base = 64;
    II_VMS_EF_NUMBER	success_efn = flag_base;
    II_VMS_EF_NUMBER	term_efn = (flag_base+1);

    ILE3 jpigetpid[] = 
    {           /* (for GETJPI calls) */
	{ sizeof( procid ), JPI$_PID, &procid, 0 },
        { sizeof( grpid ), JPI$_GRP, &grpid, &grpidret },
	{ 0, 0, 0, 0 }
    };

    ILE3 jpigetnam[] = 
    {	/* (for GETJPI calls) */
	{ sizeof( thisproc )-1, JPI$_PRCNAM, &thisproc, &namret },
	{ 0, 0, 0, 0 }
    };



    /*
    ** Following items for translating 
    ** logical device name to native device:
    */
    unsigned short natdevlen;
    $DESCRIPTOR( logdevdesc, logdev );
    $DESCRIPTOR( lnmsearch, "LNM$FILE_DEV" );
    $DESCRIPTOR( prcnamdsc, procname );
    ILE3 trnlnm[] = 
    {	/* (for TRNLNM calls) */
	{ sizeof( natdev ), LNM$_STRING, &natdev, &natdevlen },
	{ 0, 0, 0, 0 }
    };

    struct
    {			/* (for PROCESS_SCAN call) */
	unsigned short	pscn$w_buflen;
	unsigned short	pscn$w_itmcod;
	void		*pscn$pv_bufadr;
	unsigned int	pscn$i_flags;
    } pscanitm[3];	/* (just enough for process name prefix scan) */
# ifdef __ALPHA
# pragma member_alignment __restore
# endif

#endif  /* VMS */

#ifdef WIN32
  if (argc > 1 && strcmp(argv[1], "background") != 0)
  	return;
#endif

	if (!SetPath2ii_temporary())
		return;

#ifdef VMS
/** Here is the VMS prologue code, whose job is to start the RMCMD daemon
 ** and establish its initial startup completion.
 **
 ** Determine the current value of II_SYSTEM.  This will be used to tell the
 ** daemon where its initial setup COMfile is located, and where the log will
 ** be put (it must be in II_SYSTEM:[INGRES.FILES]).  If a concealed logical is
 ** used, it must be translated until a native device and dir is found
 ** (since the daemon must use a raw file spec to locate his logfile).
 ** Reduce any compound directory (root+dir) to a single directory spec.
 **/

	NMpathIng( &inst );
	if (!inst)
        {
	    SIprintf( ERx("II_SYSTEM is not defined.\n") );
	    SIflush( stdout );
	    return (SS$_IVLOGNAM);
	}
	logdev[0] = natdev[0] = ii_system[0] = '\0';
	inst2 = STindex( inst, ERx(":"), 0 );
	if (inst2)
        {
	    STlcopy( inst, logdev, (i4)(inst2 - inst) );
	    STcopy( (inst2+1), ii_system );
	}
	else
	    STcopy( inst, logdev );
	while (1)
        {
	    logdevdesc.dsc$w_length = STlength( logdev );
	    status = SYS$TRNLNM( 0, &lnmsearch, &logdevdesc, 0, &trnlnm );
	    if ( (status & 1) == 0 )
		break;
	    natdev[natdevlen] = '\0';
	    inst2 = STindex( natdev, ERx(":"), 0 );
	    if (inst2)
            {
		STcat( natdev, ii_system );
		STcopy( (inst2+1), ii_system );
		STlcopy( natdev, logdev, (i4)(inst2 - natdev) );
	    }
	    else
		STcopy( natdev, logdev);
	}
	inst = STindex( logdev, ERx(":"), 0 );
	if (inst)
	    *(inst+1) = '\0';
	else
	    STcat( logdev, ERx(":") );
	STcat( logdev, ii_system );
	ii_system[0] = '\0';
	inst = logdev;
	while (inst)
        {
	    inst2 = STindex( inst, ERx("]"), 0 );
	    if (inst2)
            {
		*inst2 = '\0';
		STcat( ii_system, inst );
		if (*(inst2+1) == '[')	/* (remove "][") */
		    inst = inst2+2;
		else			/* (final "]") */
		    inst = NULL;
	    }
	    else
            {
		STcat( ii_system, inst );
		inst = NULL;
	    }
	}
	inst = ii_system + STlength( ii_system ) - 1;
	if (*inst == ']')
        {
	    if (*(inst-1) == '.')
            {		/* (ends w. ".]") */
		*(inst-1) = *(inst);
		*(inst) = '\0';
	    }
	}
	else					/* (needs ending "]") */
	    STcat( ii_system, ERx("]") );

/** The daemon process name is constructed similar to those of Ingres servers
 ** within an Ingres installation; ie., "RMCMD_ii_xxxx", where "ii" is the
 ** installation code (or null) and "xxxx" is the hex value of the lower 16
 ** bits of the process ID.  This provides a unique process name but still
 ** allows for a separate daemon for each Ingres installation.  Normally,
 ** the GCA/CL service GCregister does this for servers; we will just copy
 ** the same logic here:
 **/
	status = sys$getjpiw(EFN$C_ENF, 0, 0, &jpigetpid, &iosb, 0, 0);
	if ( status != SS$_NORMAL || iosb.iosb$w_status !=SS$_NORMAL ) {
            if ( iosb.iosb$w_status != SS$_NORMAL )
                status = iosb.iosb$w_status;
            startup_error(status, "Cannot get process ID");
            exit(status);
	}
	status = sys$getjpiw(EFN$C_ENF, &procid, 0, &jpigetnam, &iosb, 0, 0);
	if ( status != SS$_NORMAL || iosb.iosb$w_status !=SS$_NORMAL ) {
            if ( iosb.iosb$w_status != SS$_NORMAL )
                status = iosb.iosb$w_status;
            startup_error(iosb.iosb$w_status, "Cannot get process name");
            exit(status);
	}

	NMgtAt( ERx("II_INSTALLATION"), &inst );
	STlcopy( inst ? inst : ERx(""), instcode, 2 );
	CVupper( instcode );
	CVupper( thisproc );

	STprintf( procname,
		ERx("RMCMD%s%s_%x"),
		instcode[0] ? "_" : "",
		instcode,
		(u_i2)procid ); /* (truncated to low 16 bits) */
	CVupper( procname );

	PMinit();

	STprintf( uicstr,
		  ERx("%s.%s.*.*.vms_uic"),
		  SystemCfgPrefix, PMhost() );

	if ( (PMload( NULL, (PM_ERR_FUNC *)NULL ) == OK) &&
             (PMget(uicstr, &uicptr) == OK) )
        {
           $DESCINIT( uicdsc, uicptr);

           status = iics_cvtuic( &uicdsc, &uic );

           if ( !(status & 1) )
           {
              startup_error( status, ERx("vms_uic invalid"));
              PCexit( FAIL );
           }

           status = get_username( uic, username);

           if ( (status & 1) )
           {
              user = username;
           }
        }

	/* Determine invoker (user or daemon) from process name */
	if ( STbcompare( procname, 0, thisproc, 0, TRUE) != 0 )
        {
	    /* 
            ** Names don't match (invoked by user command); see if daemon is
	    ** already running by scanning for procname prefix and group 
            ** UIC, since thr process name prefix may not be sufficient
            ** If there are both system level and group level installation.
            */
	    status = sys$getjpiw(EFN$C_ENF,0,0, &jpigetpid[1], &iosb,0,0);
	    if ( ( (status & 1) == 0 ) || ( (iosb.iosb$w_status & 1) == 0 ) )
            {
	        if ( (status & 1) )
                    status = iosb.iosb$w_status;
	        startup_error( status,
			  ERx("SYS$GETJPIW failure in rmcmd startup") );
		return (status);
	    }
	    STprintf( thisproc, ERx("RMCMD%s%s_"),
		      instcode[0] ? "_" : "",instcode );
	    pscanitm[0].pscn$w_buflen = STlength(thisproc);
	    pscanitm[0].pscn$w_itmcod = PSCAN$_PRCNAM;
	    pscanitm[0].pscn$pv_bufadr = thisproc;
	    pscanitm[0].pscn$i_flags = PSCAN$M_PREFIX_MATCH;
	    pscanitm[1].pscn$w_itmcod = PSCAN$_GRP;
	    pscanitm[1].pscn$w_buflen = 0;
	    pscanitm[1].pscn$pv_bufadr = grpid;
	    pscanitm[1].pscn$i_flags = PSCAN$M_EQL;
	    pscanitm[2].pscn$w_buflen = 0;
	    pscanitm[2].pscn$w_itmcod = 0;
	    status = SYS$PROCESS_SCAN( &context, &pscanitm );
	    if ( (status & 1 ) == 0 )
            {
		startup_error( status,
			ERx("SYS$PROCESS_SCAN failure in rmcmd startup") );
		return (status);
	    }
	    status = sys$getjpiw(EFN$C_ENF, &context,0, &jpigetnam, &iosb,0,0);
	    if ( status == SS$_NOMOREPROC ) 
            {
	    /* 
            ** No daemon process found in scan; 
            ** fall through to daemon startup
            */
            }
 	    else if ( ( (status & 1) == 0 ) || ( (iosb.iosb$w_status & 1) == 0 ) )
            {
	        if ( (status & 1) )
                    status = iosb.iosb$w_status;
	        startup_error( status,
			  ERx("SYS$GETJPIW failure in rmcmd startup") );
		return (status);
	    }
 	    else
            {
             	startup_error (SS$_DEVACTIVE,
		     ERx("RMCMD daemon already running for this installation"));
                return (SS$_DEVACTIVE);
	    }
            /*
            ** We verified that daemon is not yet running.
            ** Start up a detached daemon process that re-runs
            ** this program.  The process runs LOGINOUT to establish
            ** a separate session with the same UIC/privileges as
            ** the current user, and also to provide a DCL CLI 
            ** capability which is necessary to use C runtime
            ** functions such as "vfork". SYS$INPUT for the process 
            ** is a temp mailbox, over which DCL commands are sent to 
            ** start this program as a foreign command. SYS$OUTPUT is 
            ** set to RMCMD.LOG in the II_SYSTEM:[INGRES.FILES] directory;
            ** eventually this should be set to a null device (NLA0:) 
            ** when the  daemon  messages are directed to the Ingres error log.
            ** SYS$ERROR is unused since LOGINOUT is being run.
            ** To provide invoker/daemon synchronization, we define a 
            ** "common" event flag which the daemon sets on if it starts
            **  correctly, and a "termination message" mailbox, through 
            ** which the system can send a message if the daemon aborts.
            */
	    {	/* (Blocked only to isolate variable scopes) */
			/* SYS$INPUT and SYS$OUTPUT devices: */
	        char daemon_input[DEVNAM_L+1]  = "MBcuuuu:";
		char daemon_output[65] = "\0";
		char daemon_userid[15], *daemon_userptr = daemon_userid;
		$DESCRIPTOR( daemon_image, "SYS$SYSTEM:LOGINOUT.EXE");
		$DESCRIPTOR( daemon_sysinput, daemon_input );
		$DESCRIPTOR( daemon_sysoutput, daemon_output );
		$DESCRIPTOR( daemon_starter, daemon_userid );
		$DESCRIPTOR( daemon_procname, procname );
		unsigned int daemon_priority = 4;
		unsigned int dvi_item, i;
		unsigned int chkacc_acc, chkacc_flag;
		int term_mbox_unit_l;
		II_VMS_CHANNEL chan;
		unsigned short term, term_mbox_unit;
		ACCDEF accmsg;	/* QIO acknowledgement msg buf */
		struct
                {	/* (for CHECK_ACCESS call) */
		    unsigned short	chk$w_buflen;
                    unsigned short	chk$w_itmcod;
		    void		*chk$pv_bufadr;
		    void		*chk$pv_rlenadr;
		} chkacc[] = {
		    { sizeof( chkacc_acc ), CHP$_ACCESS, &chkacc_acc, 0 },
		    { sizeof( chkacc_flag ), CHP$_FLAGS, &chkacc_flag, 0 },
		    { 0, 0, 0, 0 } };

		char *start_commands[ ] = 
                {
	            { "$ SET NOCONTROL=Y"},
		    /* { "$ SET NOCONTROL=C"}, */
		    { "$ SET NOON"},
		    { "$ SET PROCESS/NAME=%s"},
#define SET_PROC_NAME_CMD 2	/* (cmd to be modified before use) */
		    { "$ SET PROCESS/PRIVILEGE=ALL"},
		    { "$ @%sRMCMD.COM"},
#define SET_COM_PATH_CMD 4	/* (cmd to be modified before use) */
		    { "$ DEF/PROC VAXC$PATH II_SYSTEM:[INGRES.BIN]"},
		    { "$ rmcmdin :== $VAXC$PATH:RMCMDIN.EXE"},
		    { "$ rmcmdout :== $VAXC$PATH:RMCMDOUT.EXE"},
		    { "$ RUN VAXC$PATH:RMCMD.EXE"},
		    { "$ LOGOUT"},
		    { 0 }
                };


	        /*
                ** First check to make sure the user can create the 
                ** output log file.  For now, assume the current userID 
                ** is used for daemon execution. (In the future, this may
                ** be a different user/UIC.)
                */
		STpolycat( 2, ii_system, ERx("FILES.DIR"), daemon_output );
		IDname( &daemon_userptr );
		CVupper( daemon_userid );
		daemon_starter.dsc$w_length = strlen(daemon_userid);
		daemon_sysoutput.dsc$w_length = strlen(daemon_output);
		dvi_item = ACL$C_FILE;
		chkacc_acc = ARM$M_WRITE;
		chkacc_flag = CHP$M_WRITE;
		status = SYS$CHECK_ACCESS( &dvi_item,		/* Obj type */
					&daemon_sysoutput,	/* Obj name */
					&daemon_starter,    /* Access userid */
					chkacc,			/* Item list */
					0, 0, 0, 0 );
		if ( status == SS$_NOPRIV )
                {
		    startup_error(status,
                          ERx("User has no access to create daemon logfile"));
		    return (status);
		}
		else if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to check logfile create access") );
		    return (status);
		}
		*(ii_system + STlength( ii_system ) - 1) = '\0';
		STpolycat( 2, ii_system, ERx(".FILES]RMCMDSTART.LOG"),
		    daemon_output );
		STcat( ii_system, ERx(".BIN]") );
		daemon_sysoutput.dsc$w_length = strlen(daemon_output);

	        /* 
                ** Create mailbox for passing commands to the new 
                ** process. Then get the device name of the mailbox
                ** to be used as the input stream for new process.
                */
		status = SYS$CREMBX( 0, &chan, 0, 0, 0, PSL$C_USER, NULL );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to create rmcmd input mailbox") );
		    return (status);
		}

		dvi_item = DVI$_DEVNAM;
		status = LIB$GETDVI( &dvi_item, &chan, NULL, NULL,
			             &daemon_sysinput, &daemon_sysinput );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to evaluate input mailbox device") );
		    cleanup( chan, 0, 0 );
		    return ( status );
		}

	        /* 
                ** Create another mailbox for passing back a termination
                ** message from the new process (if it fails before fully 
                ** starting). Then get the unit number to pass to SYS$CREPRC.
                */
		status = SYS$CREMBX( 0, &term, sizeof(accmsg), 0, 0,
			            PSL$C_USER, NULL );
		if ( (status & 1) == 0 ) 
                {
		    startup_error( status,
			    ERx("Failed to create rmcmd term mailbox") );
		    return (status);
		    cleanup( chan, 0, 0 );
		}

		dvi_item = DVI$_UNIT;
		status = LIB$GETDVI( &dvi_item, &term, NULL,
			             &term_mbox_unit_l, NULL );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to evaluate term mailbox unit") );
		    cleanup( chan, term, 0 );
		    return ( status );
		}
		term_mbox_unit = term_mbox_unit_l;

	        /*
                ** Before we start the daemon, define the event flag
                ** it must use to notify us of successful start.  
                ** This must be a "common" event flag (one known to 
                ** separate processes).
                */
		status = SYS$ASCEFC( success_efn, &rmcmd_cluster, 0, 0 );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
	            ERx("Failed to associate success event flag") );
		    cleanup( chan, term, 0 );
		    return ( status );
		}
		status = SYS$CLREF( success_efn );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to clear success event flag") );
		    cleanup( chan, term, success_efn );
		    return ( status );
		}

	        /*
                ** Post an asynchronous read to the termination mailbox;
                ** if this gets satisfied, we know the daemon died.  
                ** Note that this uses another common event flag to 
                ** notify us of satisfaction.
                */
		status = SYS$QIO( term_efn, term, IO$_READVBLK, &iosb,
			NULL, NULL, &accmsg, sizeof(accmsg), 0, 0, 0, 0 );
		if ( (status & 1) == 0 )
                {
		    startup_error( status,
			  ERx("Failed to read rmcmd term mailbox") );
		    cleanup( chan, term, success_efn );
		    return (status);
		}

	        /* Now start the detached daemon process. */
                /* 07-mar-2002 (chash01)
                **   Inclusion of PRC$M_NOAUF resulted in hanging of entire 
                **   RMCMD processes because certain job level logicals are
                **   not set.  
                */
		status = SYS$CREPRC( &procid,
			&daemon_image,
			&daemon_sysinput,
			&daemon_sysoutput,
			NULL,		/* no SYS$ERROR for LOGINOUT */
			NULL,		/* use creator's privs */
			NULL,		/* use default quotas */
			&daemon_procname,
			daemon_priority,
			uic,		/* use UIC of Ingres DBA */
			term_mbox_unit,	/* termination mssg mailbox */
			(unsigned int) PRC$M_DETACH);
		if ( (status & 1) == 0 )
                {
		      startup_error( status,
			  ERx("Failed to create rmcmd server process") );
		      cleanup( chan, term, success_efn );
		      return (status);
		}

	        /* Reformat proc name w. new id for proc renamimg */
		STprintf( procname,
			ERx("RMCMD%s%s_%x"),
			instcode[0] ? "_" : "",
			instcode,
			(u_i2)procid ); /* (truncated to low 16 bits) */
		CVupper( procname );

	        /* 
                ** Write all startup commands to the mailbox; process
                ** will pick them up thru SYS$INPUT and execute them. 
                ** This will change the procname and start the RMCMD image.
                ** While writing these, continue to check for daemon failure.
                */
		for ( i = 0; start_commands[i] != NULL; i++ )
		{
		      status = SYS$READEF( term_efn, (unsigned long *)buf );
		      if ( (status & 1) == 0 )
                      {
		          startup_error( status,
				ERx("Failed to read rmcmd term event flag") );
		          cleanup( chan, term, success_efn );
			  return (status);
		      }

		      if ( status == SS$_WASSET )
                      {
		          if (accmsg.acc$l_finalsts == DAEMON_FAILED_NO_IIDBDB)
			       startup_error( SS$_UNSUPPORTED,
			         ERx("RMCMD Daemon could not connect to imadb") );
			  else if ( accmsg.acc$l_finalsts ==
                                                   DAEMON_FAILED_NO_TABLES )
			       startup_error( SS$_UNSUPPORTED,
	                         ERx("Visual DBA tables have not been added to imadb; run RMCMDGEN") );
			  else if ( accmsg.acc$l_finalsts ==
					          DAEMON_FAILED_BAD_USER )
			       startup_error( SS$_UNSUPPORTED,
	                         ERx("RMCMD Daemon must be started by Ingres' System Administrator") );
			  else
			       startup_error( accmsg.acc$l_finalsts,
		                 ERx("RMCMD Daemon aborted with system error; status:") );

			  cleanup( chan, term, success_efn );
			  return (SS$_ABORT);
		      }

		      if ( i == SET_PROC_NAME_CMD )
			  STprintf( buf, start_commands[i], procname );
		      else if ( i == SET_COM_PATH_CMD )
			  STprintf( buf, start_commands[i], ii_system );
		      else
			  STcopy( start_commands[i], buf );

		      status = sys$qiow(EFN$C_ENF, chan, IO$_WRITEVBLK|IO$M_NOW,
				         &iosb, NULL, NULL,
			                 buf, STlength( buf ), 0, 0, 0, 0 );
		      if ( status & 1 )
                          status = iosb.iosb$w_status;
		      if ( (status & 1) == 0 )
                      {
			   STprintf( buf,
		                ERx("Failed to send rmcmd startup command %u"),
                                i );
			   startup_error( status, buf );
			   /* (Cancel subprocess?) */
			   cleanup( chan, term, success_efn );
			   return ( status );
		      }
		} /* (end for) */

	        /*
                ** Now wait for either the daemon to raise the 
                ** successful start event or the termination mailbox
                ** message.  Issue messages for either case.
                */
		status = SYS$WFLOR( success_efn,
			(unsigned int) (success_efn - flag_base + 1) |
					(term_efn - flag_base + 1) );
		if ( (status & 1) == 0 )
                {
		      startup_error( status,
			        ERx("Failed to wait on event flags") );
		      /* (Cancel subprocess?) */
		      cleanup( chan, term, success_efn );
		      return (status);
		}
		status = SYS$READEF( term_efn, (unsigned long *)buf );
		if ( (status & 1) == 0 )
                {
		      startup_error( status,
			      ERx("Failed to read rmcmd term event flag") );
		      /* (Cancel subprocess?) */
		      cleanup( chan, term, success_efn );
		      return (status);
		}

		if ( status == SS$_WASSET )
                {
                      /*
                      ** Daemon terminated; read final status code from
                      ** termination msg.
                      */
		      if (accmsg.acc$l_finalsts == DAEMON_FAILED_NO_IIDBDB)
			  startup_error( SS$_UNSUPPORTED,
			      ERx("RMCMD Daemon could not connect to imadb") );
		      else if (accmsg.acc$l_finalsts == DAEMON_FAILED_NO_TABLES)
			  startup_error( SS$_UNSUPPORTED,
	                      ERx("Visual DBA tables have not been added to imadb; run RMCMDGEN") );
		      else if (accmsg.acc$l_finalsts == DAEMON_FAILED_BAD_USER)
			  startup_error( SS$_UNSUPPORTED,
	                      ERx("RMCMD Daemon must be started by Ingres' System Administrator") );
		      else
			  startup_error( accmsg.acc$l_finalsts,
		              ERx("RMCMD Daemon aborted with system error; status:") );

		      cleanup( chan, term, success_efn );
		      return (SS$_ABORT);
		}

	        /*
                ** Daemon started successfully; issue confirmation msg.
	        ** Deassign mailbox channels, dissociate common event
                ** flags
                */
                rmcmd_started(procname,procid);
		cleanup( chan, term, success_efn );
	        return (OK);
            }  /* arbitrary blocking end */
	} /* if STbcompare(procname, thisproc) */

#else

	/*
	** Now, let's register the server with the Name server.
	*/
	MEmove( 7, "IIRMCMD", ' ', sizeof(ServerID), ServerID);
	ServerID[16] = '\0';

#ifdef LNX
        PCsetpgrp();
#endif
        PCpid(&proc_id);
        STprintf( fmt, "%s", PIDFMT );
        STprintf(ServerPID, fmt, proc_id);

	if ((func_stat = rmcmd_bedcheck()) == OK)
	{
	    if (ServerUp)
	    {
		log_message(E_RE0012_RMCMD_2MANY, 0);
		return;
	    }
	}
	else
	{
	    log_message(E_RE0013_BEDCHECK_FAIL, 0);
	    return;
	}

	/*
	** Next set up the first few default parameters for subsequent PMget
	** calls based upon the name of the local node...
	*/
	PMsetDefault( 0, ERx( "ii" ) );
	PMsetDefault( 1, PMhost() );
	PMsetDefault( 2, ERx( "rmcmd" ) );
	PMsetDefault( 3, ERx( "*" ) );

	parms.gca_in_parms.gca_expedited_completion_exit = gca_sm;
	parms.gca_in_parms.gca_normal_completion_exit = gca_sm;
	parms.gca_in_parms.gca_alloc_mgr = NULL;
	parms.gca_in_parms.gca_dealloc_mgr = NULL;
	parms.gca_in_parms.gca_p_semaphore = NULL;
	parms.gca_in_parms.gca_v_semaphore = NULL;
	parms.gca_in_parms.gca_modifiers = GCA_API_VERSION_SPECD;
	parms.gca_in_parms.gca_api_version = GCA_API_LEVEL_4;
	parms.gca_in_parms.gca_local_protocol = GCA_PROTOCOL_LEVEL_62;

	IIGCa_cb_call(&gca_cb, GCA_INITIATE, &parms, GCA_SYNC_FLAG,
		      (PTR)0, -1, &func_stat);

	if (func_stat != OK ||
	    (func_stat = parms.gca_in_parms.gca_status) != OK)
	{
	    gcu_erlog(0, 1, func_stat, &parms.gca_in_parms.gca_os_status,
		      0, NULL);
	    return;
	}

	parms.gca_rg_parms.gca_l_so_vector = 0;
	parms.gca_rg_parms.gca_served_object_vector = NULL;
	parms.gca_rg_parms.gca_srvr_class = (char *)"RMCMD";
	parms.gca_rg_parms.gca_installn_id = NULL;
	parms.gca_rg_parms.gca_process_id = NULL;
	parms.gca_rg_parms.gca_no_connected = 0;
	parms.gca_rg_parms.gca_no_active = 0;
	parms.gca_rg_parms.gca_modifiers = GCA_RG_INSTALL;
	parms.gca_rg_parms.gca_listen_id = NULL;

	IIGCa_cb_call(&gca_cb, GCA_REGISTER, &parms, GCA_SYNC_FLAG,
		      (PTR)0, (i4) -1, &func_stat);

	if (func_stat != OK ||
	    (status = parms.gca_rg_parms.gca_status) != OK )
	{
	    gcu_erlog(0, 1, func_stat, &parms.gca_rg_parms.gca_os_status,
		      0, NULL);
	    return;
	}

	MEmove( (u_i2) STlength(parms.gca_rg_parms.gca_listen_id),
		parms.gca_rg_parms.gca_listen_id,
		' ', sizeof(ServerID), ServerID);
	ServerID[16] = '\0';

#ifdef LNX
        PCsetpgrp();
#endif
        PCpid(&proc_id);
        STprintf( fmt, "%s", PIDFMT );
        STprintf(ServerPID, fmt, proc_id);

	gca_sm(0);

#endif  /* END OF VMS PROLOGUE CODE */
        /*
        ** The first thing we must do is test all the requirements for the 
        ** daemon to operate; if any of these cannot be met, we can still 
        ** send a failure status back to the invoker thru the termination
        ** mailbox message, and terminate.
        ** If requirements are all met, set on the common event flag to 
        ** notify the invoker process that we started successfully.  
        ** Any future problems must be  handled as rmcmd server errors,
        ** and require Ingres' error log entries.
        */

#ifdef UNIX
	/*
	** Setup SIGCHLD signal handler to eliminate Zombie processes
	*/
	action.sa_handler = EliminateZombie;
	sigaction (SIGCHLD, &action, NULL);
#endif

	/*
	** Get the host name.
	*/
	host = PMhost();

	PMinit();
	switch( PMload( NULL, (PM_ERR_FUNC *)NULL ) )
	{
		case OK :  break;

		case PM_FILE_BAD :
			log_message(E_RE0019_CONFIG_SYNTAX_ERROR, 0);
			return;

		default :
			log_message(E_RE001A_CONFIG_OPEN_ERROR, 0);
			return;
	}

    /*
    ** Add call to register configuration parameters as an MO class.
    */
    GVcnf_init();
	/*
	** Build the string we will search for in the config.dat file.
	*/
	STprintf( config_string,
		      ERx("%s.%s.gcn.local_vnode"),
		      SystemCfgPrefix, host );
	if ( PMget(config_string, &value) == OK &&
	     value != NULL && value[0] != '\0' ) {
		STlcopy(value, strLocalVnode, MAX_LOCAL_VNODE);
	}
	else
	{
		log_message(E_RE001B_LOCAL_VNODE_ERROR, 0);
		return;
	}

    /*
    ** Retrieve the user executing this process
    */
    processuser = RmcmdStartUser;
    IDname(&processuser);
    /*
    ** Check user privileges
    */
    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );

    if (chk_priv( processuser, GCA_PRIV_SERVER_CONTROL ) != OK)
    {
#ifdef VMS
        return (DAEMON_FAILED_BAD_USER);
#else
       /* E_RE001C_NO_PRIVILEGES
       ** "User has insufficient privileges for starting the RMCMD server."
       */
       log_message(E_RE001C_NO_PRIVILEGES, 0);
       return;
#endif /* VMS */
    }

    exec sql connect 'imadb/ingres' identified by '$ingres';

	if (sqlca.sqlcode < 0) 
        {
            MEfill(sizeof(parms), 0, (PTR) &parms);
            IIGCa_cb_call(&gca_cb, GCA_TERMINATE, &parms, GCA_SYNC_FLAG,
	  	          (PTR)0, -1, &func_stat);

#ifdef VMS
	    return (DAEMON_FAILED_NO_IIDBDB);
#else
	    log_message(E_RE0004_STARTUP_CONNECT_FAIL, 1);
	    return;
#endif /* VMS */
	}

   exec sql set_sql (errortype = 'genericerror');

   /* create connection used to lock remotecmdlock for mediation */
    exec sql connect 'imadb/ingres' session 3 identified by '$ingres';

	if (sqlca.sqlcode < 0) 
#ifdef VMS
        {
            exec sql set_sql(session= 1); /* restore main rmcmd session */
            exec sql disconnect; 
	    return (DAEMON_FAILED_NO_IIDBDB);
        }
#else
	{
	    log_message(E_RE0004_STARTUP_CONNECT_FAIL, 1);
	    return;
	}
#endif /* VMS */

   exec sql set_sql (errortype = 'genericerror');

    /* create rmcmdstpnode event to shut down specific node */
    exec sql create dbevent rmcmdstpnode;
    if (sqlca.sqlcode < 0 && sqlca.sqlcode != -30200)
    {
 #ifdef VMS
	 exec sql disconnect; 
	 exec sql set_sql(session= 1); /* restore main rmcmd session */
	 exec sql disconnect; 
	 return (DAEMON_FAILED_NO_TABLES);
 #else
	 log_message(E_RE0006_STARTUP_RMCMD_OBJ, 1);
	 goto end;
 #endif  /* VMS */
    }

   /* create dummy table used to mediate access to the remotecmd table */
   exec sql create table remotecmdlock (holder int);
   if (sqlca.sqlcode < 0 && sqlca.sqlcode != -30200)
   {
#ifdef VMS
	exec sql disconnect; 
	exec sql set_sql(session= 1); /* restore main rmcmd session */
	exec sql disconnect; 
	return (DAEMON_FAILED_NO_TABLES);
#else
	log_message(E_RE0006_STARTUP_RMCMD_OBJ, 1);
	goto end;
#endif  /* VMS */
   }

   /* set locking parameters for mediation session */
    exec sql commit;
    exec sql set lockmode on remotecmdlock where level=table, readlock=exclusive, timeout=nowait;
    exec sql set_sql(session= 1); /* restore main rmcmd session */


    exec sql set lockmode session where readlock=exclusive;
    exec sql set session isolation level serializable;

    if (sqlca.sqlcode < 0) 
#ifdef VMS
    {
       exec sql disconnect; 
       return (DAEMON_FAILED_NO_IIDBDB);
    }
#else
    {
       log_message(E_RE001D_READLOCK_MODE, 1);
        goto end;
    }
#endif /* VMS */

    /*
    ** check if Visual DBA tables have been genned 
    */
    exec sql select count(*) into :hdl from remotecmd;
    if (sqlca.sqlcode < 0) 
    {
#ifdef VMS
	exec sql disconnect; 
	return (DAEMON_FAILED_NO_TABLES);
#else
	log_message(E_RE0006_STARTUP_RMCMD_OBJ, 1);
	goto end;
#endif  /* VMS */
    }

    /************ register dbevents ************/
    STprintf(stmt,ERx("register dbevent rmcmdnewcmd"));
    EXEC SQL EXECUTE IMMEDIATE :stmt;

    if (sqlca.sqlcode < 0) 
    {
#ifdef VMS
	exec sql disconnect; 
	return (DAEMON_FAILED_NO_TABLES); /* TO DO : differentiate this specific error under VMS */
#else
	log_message(E_RE0007_STARTUP_DBEVNT_NEWCMD, 1);
	goto end;
#endif  /* VMS */
    }

    STprintf(stmt,ERx("register dbevent rmcmdstp"));
    EXEC SQL EXECUTE IMMEDIATE :stmt;
    if (sqlca.sqlcode < 0) 
    {
#ifdef VMS
	exec sql disconnect; 
	return (DAEMON_FAILED_NO_TABLES); /* TO DO : differentiate this specific error under VMS */
#else
	log_message(E_RE0008_STARTUP_DBEVNT_STP, 1);
	goto end;
#endif  /* VMS */
    }

    STprintf(stmt,ERx("register dbevent rmcmdstpnode"));
    EXEC SQL EXECUTE IMMEDIATE :stmt;
    if (sqlca.sqlcode < 0) 
    {
#ifdef VMS
	exec sql disconnect; 
	return (DAEMON_FAILED_NO_TABLES); /* TO DO : differentiate this specific error under VMS */
#else
	log_message(E_RE0008_STARTUP_DBEVNT_STP, 1);
	goto end;
#endif  /* VMS */
    }

    /*
    ** remove rows with RMCMDSTATUS_REQUEST4TERMINATE
    ** since if rmcmdstp has been executed before rmcmd,
    ** rmcmd should not exit immediatly
    */
    status1=RMCMDSTATUS_REQUEST4TERMINATE;
    exec sql delete from remotecmd where status = :status1;
    exec sql delete from remotecmd where cmd1 like 'rmcmdstpnode%';
    exec sql commit;

    if (sqlca.sqlcode < 0) 
    {
        exec sql disconnect;
#ifndef VMS  /* porting to VMS needed */
	log_message(E_RE0009_STARTUP_CLEANUP_FAIL, 1);
#endif  /* VMS */
	goto end;
    }
#ifdef VMS
    /*
    ** Special code for VMS so that daemon can signal successfull
    ** startup to the invoking user.
    ** Associate with and set on the common event flag to tell our
    ** invoking process that we started successfully
    */
    status = SYS$ASCEFC( success_efn, &rmcmd_cluster, 0, 0 );
    if ( (status & 1) == 0 )
    {
	exec sql disconnect;
	return ( status );
    }
    status = SYS$SETEF( success_efn );
    if ( (status & 1) == 0 )
    {
	exec sql disconnect;
	return ( status );
    }
    cleanup( 0, 0, success_efn );
    TMnow( &time_now );
    TMstr( &time_now, buf );
    STprintf(rmlogbuf, ERx("\nVisual DBA RMCMD Daemon starting at %s\n"), buf);
    rmcmd_log( rmlogbuf, STlength(rmlogbuf) );

#else
    log_message(E_RE0001_RMCMD_UP, 0);
#endif  /* VMS */


    while (1)
    {
	if (!master)
	{
	    exec sql set_sql(session= 3); /* switch to remotecmdlock session */
	    exec sql select * into :hdl from remotecmdlock;
	    if (sqlca.sqlcode >= 0)
	    {
		/* got exclusive lock on remotecmdlock, now master */
		master = 1;
	    }
	    exec sql set_sql(session= 1); /* restore main rmcmd session */
	}

	if (master)
	{
        hdl = -1;
	*szCommand = EOS;
        status1=RMCMDSTATUS_SENT;
        status2=RMCMDSTATUS_SERVERPRESENT;

  	exec sql update remotecmd
		 set status=:status2
		 where status = :status1;
#ifndef VMS  /* porting to VMS needed */
	if (sqlca.sqlcode < 0) 
	{
	    log_message(E_RE000A_DAEMON_UPDATE_FAIL, 1);
	    goto end;
	}
#endif  /* VMS */

  	exec sql select handle, cmd1, user_id
		 into :hdl, :szCommand, :username
		 from remotecmd
		 where status = :status2 and handle=(select min(handle) from remotecmd where status=:status2);
	if (sqlca.sqlcode < 0) 
	{
#ifndef VMS  /* porting to VMS needed */
	    if (sqlca.sqlcode == -30110) {
	      log_message(E_RE0014_WRONG_COLUMN, 0);
	      goto end;
	    }
	    log_message(E_RE000B_DAEMON_READ_FAIL, 1);
#endif  /* VMS */
	    goto end;
	}

#ifndef WIN32
	if ( hdl > -1)
        {
            char * com1[200];
            char * comin[4], * comout[4];
            char buf1[10],buf2[10];
            char bufin[100],bufout[100];
            int pin[2],pout[2];
            int cstatus = 0, procid[3];
            STtrmwhite(username);
	    /*
            ** special "dummy" commands for replicator 
            ** are managed before invocation of
            ** isCommandAccepted() because they have a 
            ** special management for security
            */
	    if ( !MEcmp( szCommand, rmcmdddispf, STlength(rmcmdddispf) ) )
            {
		ManageFileDisplay(szCommand,hdl);
		continue;		
	    }
            if ( !MEcmp( szCommand, rmcmdddstoref, STlength(rmcmdddstoref) ) )
            {
	        ManageFileStorage(szCommand,hdl);
		continue;		
	    }
	    if ( !MEcmp( szCommand, rmcmddddstart, STlength(rmcmddddstart) ) )
            {
		if (!ManageStartReplicServer(szCommand,hdl))
			break;
		continue;		
	    }
            if ( !MEcmp( szCommand, rmcmdprocuser, STlength(rmcmdprocuser) ) )
            {
                if (!ManageVerifyRmcmdStartUser(szCommand,hdl))
                    break;
                continue;
            }
	    /*
	    ** special dummy command request for rmcmd to stop on
	    ** a specific node
	    */
            if ( !MEcmp( szCommand, rmcmdstpnode, STlength(rmcmdstpnode) ) )
            {
                if (!ManageRmcmdStopNode(szCommand,hdl))
                    break;
                continue;
            }
            if (isCommandAccepted(username, szCommand))
                status1 = RMCMDSTATUS_ACCEPTED;
            else
                status1 = RMCMDSTATUS_ERROR;
            {
                char * pc=szCommand;
                char * pc1;
                int i=0;
                STtrmwhite(pc);
	 
                while (*pc)
                {
                    while (*pc==' ')
                        pc++;
                    pc1=STindex( pc, ERx(" "), 0 );
                    if (!pc1)
                    {
                        com1[i++]=pc;
                        break;
                    }
                    *pc1='\0';
                    com1[i++]=pc;
                    pc=pc1+1;
                    if (i>=(sizeof(com1)/sizeof(char *)))
                    {
                        status1 = RMCMDSTATUS_ERROR;
                        break;
                    }
                }
                com1[i]=(char *)0;
            }

            exec sql update remotecmd
                set status = :status1
                where handle = :hdl;
            exec sql commit;
            status2 = status1;

	    if (sqlca.sqlcode < 0) 
	    {
#ifndef VMS  /* porting to VMS needed */
		log_message(E_RE000A_DAEMON_UPDATE_FAIL, 1);
#endif  /* VMS */
		goto end;
	    }

            if (status2 == RMCMDSTATUS_ACCEPTED)
            {

                STcopy( ERx("rmcmdin"), bufin );
                STcopy( ERx("rmcmdout"), bufout );

                comin[0]=bufin;
                STprintf( buf1, ERx("%d"), hdl );
                comin[1]=buf1;
                comin[2]=(char *)0;

                comout[0]=bufout;
                STprintf( buf2, ERx("%d"), hdl );
                comout[1]=buf2;
                comout[2]=(char *)0;
                if (pipe(pin)<0 || pipe(pout)<0)
                {
#ifdef VMS
		    daemon_error( hdl, szCommand, username,
	    	    ERx("Failure to create pipe(s)") );
#endif  /* VMS */
                    status2 = RMCMDSTATUS_ERROR;
                }
            } /* (end if(status2 == _ACCEPTED)) */

            if (status2 == RMCMDSTATUS_ACCEPTED)
            {
#ifdef VMS
                status = vfork();
                if (status < 0) 
                {
                    daemon_error( hdl, szCommand, username,
                    ERx("Failure to create rmcmdin fork 1") );
                    status2 = RMCMDSTATUS_ERROR;
                }
#else
                status = fork();
                if (status < 0)
                {
                    status2 = RMCMDSTATUS_ERROR;
                }
#endif /* VMS */
                else if (status == 0) 
                {  /* child executes rmcmdin and exits */
#ifdef VMS
                    dup2( 1, 19 );  /* Save stdout descriptor */
#endif
                    dup2(pin[1],1);

#ifndef VMS
                    close(pin[0]);
                    close(pin[1]);
                    close(pout[0]);
                    close(pout[1]);
#endif

#ifdef VMS
/* For VMS, invoke 'rmcmdv' instead of actual pgm; it will simulate a
   "foreign command" to satisfy the Ingres FRONTMAIN stub: */
                    status = execvp("rmcmdv",comin);
                    if (status == -1)
                    {
                        dup2( 19, 1 ); /* Restore proper sysout */
                        daemon_error( hdl, szCommand, username,
                        ERx("Failure to execute rmcmdin") );
                        status2 = RMCMDSTATUS_ERROR;
                        _exit(1);
                    }
#else
                    status = execvp(comin[0],comin);
		    exit(2);
#endif /* VMS */
                }  /* (end if(status==0)) */
#ifdef VMS
	        /* 
                ** On VMS, execvp returns thru vfork w. process ID of 
                ** 1st subprocess
                */
                procid[0] = status;
#endif
            } /* (end if(status2 == _ACCEPTED)) */
            if (status2 == RMCMDSTATUS_ACCEPTED)
            {
#ifdef VMS
                status = vfork();
                if (status < 0)
                {
                    dup2( 19, 1 ); /* Restore proper sysout */
                    daemon_error( hdl, szCommand, username,
                    ERx("Failure to create utility fork 2") );
                    status2 = RMCMDSTATUS_ERROR;
                }
#else
                status = fork();
                if (status < 0)
                {
                    status2 = RMCMDSTATUS_ERROR;
                }
#endif /* VMS */
                else if (status == 0)
                {  /* child executes command and exits */
                    dup2(pin[0],0);
                    dup2(pout[1],1);
                    dup2(pout[1],2);

#ifndef VMS
                    close(pin[0]);
                    close(pin[1]);
                    close(pout[0]);
                    close(pout[1]);
#endif
#ifdef VMS
/* For VMS, invoke 'rmcmdv' instead of actual pgm; it will simulate a
   "foreign command" to satisfy the Ingres FRONTMAIN stub: */
                    status = execvp("rmcmdv",com1);
                    if (status == -1)
                    {
                        dup2( 19, 1 ); /* Restore proper sysout */
                        daemon_error( hdl, szCommand, username,
                        ERx("Failure to execute utility") );
                        status2 = RMCMDSTATUS_ERROR;
                        _exit(1);
                    }
#else
                    status = execvp(com1[0],com1);
		    exit(2);
#endif /* VMS */
                }  /* (end if(status==0)) */
#ifdef VMS
	        /* 
                ** On VMS, execvp returns thru vfork w. 
                ** process ID of 2nd subprocess
                */
                procid[1] = status;
#endif
            } /* (end if(status2 == _ACCEPTED)) */

            if (status2 == RMCMDSTATUS_ACCEPTED)
            {
#ifdef VMS
                status = vfork();
                if (status < 0)
                {
                    dup2( 19, 1 ); /* Restore proper sysout */
                    daemon_error( hdl, szCommand, username,
                    ERx("Failure to create rmcmdout fork 3") );
                    status2 = RMCMDSTATUS_ERROR;
                }
#else
                status = fork();
                if (status < 0)
                {
                    status2 = RMCMDSTATUS_ERROR;
                }
#endif /* VMS */
                else if (status == 0)
                {  /* child executes rmcmdout and exits */
                    dup2(pout[0],0);

#ifndef VMS
                    close(pin[0]);
                    close(pin[1]);
                    close(pout[0]);
                    close(pout[1]);
#endif
#ifdef VMS
/* For VMS, invoke 'rmcmdv' instead of actual pgm; it will simulate a
   "foreign command" to satisfy the Ingres FRONTMAIN stub: */
                    status = execvp("rmcmdv",comout);
                    if (status == -1)
                    {
                        dup2( 19, 1 ); /* Restore proper sysout */
                        daemon_error( hdl, szCommand, username,
                        ERx("Failure to execute rmcmdout") );
                        status2 = RMCMDSTATUS_ERROR;
                        _exit(1);
                    }
#else
                    status = execvp(comout[0],comout);
		    exit(2);
#endif /* VMS */
                }  /* (end if(status==0)) */
#ifdef VMS
	        /* 
                ** On VMS, execvp returns thru vfork w. process ID of
                ** 3rd subprocess
                */
                procid[2] = status;
#endif
            } /* (end if(status2 == _ACCEPTED)) */
            close(pin[0]);
            close(pin[1]);
            close(pout[0]);
            close(pout[1]);
#ifdef VMS
            dup2( 19, 1 ); /* Restore proper sysout */
            close( 19 );

/** On VMS,we must wait for all three subprocesses to finish. Check their
 ** return codes for errors, or to see if their programs couldn't be located.
 **/
            if (status2 == RMCMDSTATUS_ACCEPTED)
            {
                int i, p;
                char *prog, *errmsg;
                for (i = 0; i < 3; i++)
                {
                    status = wait(&cstatus);
                    if (status == -1)
                    {
                        daemon_error( hdl, szCommand, username,
                                      ERx("Failure to wait for subprocess") );
                        status2 = RMCMDSTATUS_ERROR;
                    }
                    else
                    {
		        /* 
                        ** Locate appropriate subprocess ID 
                        ** (the order in which WAIT returns them
                        ** is unpredictable)
                        */
                        for (p = 0; p < 3; p++)
                             if (status == procid[p])
                                 break;
                        switch (p)
                        {
                            case 0: prog = comin[0]; break;
                            case 1: prog = com1[0]; break;
                            case 2: prog = comout[0]; break;
                            default: prog = ERx("(unknown)"); break;
                        }
                        errmsg = NULL;
                        if (cstatus == CLI$_IMAGEFNF)
                            errmsg = STprintf( buf,
                               ERx("Could not locate executable for subprocess: %s"),
                               prog );
                        /*
                        ** For VMS, a successful return is signalled
                        ** by status=1, not 0
                        */
                        else if (cstatus != 1)
                            errmsg = STprintf( buf,
                                 ERx("Subprocess for %s ended with status %d"),
                                 prog, cstatus );
                        if (errmsg)
                        {
                            daemon_error( hdl, szCommand, username, errmsg );
                            status2 = RMCMDSTATUS_ERROR;
                        }
                    } /* (end else) */
                } /* (end for) */
            } /* (end if(status2 == _ACCEPTED)) */
#endif /* VMS */

/** If any error was encountered at any stage, set the error status in the
 ** remotecmd table.
 **/
            if (status1 != status2)
            {
                exec sql update remotecmd
                     set status = :status2
                     where handle = :hdl;
                exec sql commit;
#ifndef VMS  /* porting to VMS needed */
		log_message(E_RE000C_DAEMON_EXEC_FAIL, 1);
#endif  /* VMS */

		if (sqlca.sqlcode < 0) 
		{
#ifndef VMS  /* porting to VMS needed */
		    log_message(E_RE000D_DAEMON_STATUS_UPDATE, 1);
#endif  /* VMS */
		    goto end;
		}

            }
	}		     /* old code -> system (buf); */
#else  /* def WIN32 */
    if ( hdl > -1)
    {
            char * com1[200];
            char * comin[4], * comout[4];
            char buf1[10],buf2[10];
            char bufin[100],bufout[100];
            int cstatus = 0;
            STtrmwhite(username);
        /*
        ** special "dummy" commands for replicator 
        ** are managed before invocation of
        */
        /*
        ** isCommandAccepted() because they have a 
        ** special management for security
        */
            if ( !MEcmp( szCommand, rmcmdddispf, STlength(rmcmdddispf) ) )
            {
                ManageFileDisplay(szCommand,hdl);
                continue;
            }
            if ( !MEcmp( szCommand, rmcmdddstoref, STlength(rmcmdddstoref) ) )
            {
                ManageFileStorage(szCommand,hdl);
                continue;
            }
            if ( !MEcmp( szCommand, rmcmddddstart, STlength(rmcmddddstart) ) )
            {
                if (!ManageStartReplicServer(szCommand,hdl))
                    break;
                continue;
            }
            if ( !MEcmp( szCommand, rmcmdprocuser, STlength(rmcmdprocuser) ) )
            {
                if (!ManageVerifyRmcmdStartUser(szCommand,hdl))
                    break;
                continue;
            }
	    /*
	    ** special dummy command request for rmcmd to stop on
	    ** a specific node
	    */
            if ( !MEcmp( szCommand, rmcmdstpnode, STlength(rmcmdstpnode) ) )
            {
                if (!ManageRmcmdStopNode(szCommand,hdl))
                    break;
                continue;
            }

            if (isCommandAccepted(username, szCommand))
                status1 = RMCMDSTATUS_ACCEPTED;
            else
                status1 = RMCMDSTATUS_ERROR;
            {
                char * pc=szCommand;
                char * pc1;
                int i=0;
                STtrmwhite(pc);
	 
                while (*pc)
                {
                    while (*pc==' ')
                        pc++;
                    pc1=STindex( pc, ERx(" "), 0 );
                    if (!pc1)
                    {
                        com1[i++]=pc;
                        break;
                    }
                    *pc1='\0';
                    com1[i++]=pc;
                    pc=pc1+1;
                    if (i>=(sizeof(com1)/sizeof(char *)))
                    {
                        status1 = RMCMDSTATUS_ERROR;
                        break;
                    }
                }
                com1[i]=(char *)0;
            }

            exec sql update remotecmd
                set status = :status1
                where handle = :hdl;
            exec sql commit;
            status2 = status1;

	    if (sqlca.sqlcode < 0) 
	    {
		log_message(E_RE000A_DAEMON_UPDATE_FAIL, 1);
		goto end;
	    }

            if (status2 == RMCMDSTATUS_ACCEPTED)
            {

                STcopy( ERx("rmcmdin"), bufin );
                STcopy( ERx("rmcmdout"), bufout );

                comin[0]=bufin;
                STprintf( buf1, ERx("%d"), hdl );
                comin[1]=buf1;
                comin[2]=(char *)0;

                comout[0]=bufout;
                STprintf( buf2, ERx("%d"), hdl );
                comout[1]=buf2;
                comout[2]=(char *)0;
	        {
		    HANDLE hReadIn;
		    HANDLE hWriteIn;
		    HANDLE hReadOut;
		    HANDLE hWriteOut;
                    HANDLE hStdIn;
                    HANDLE hStdOut;
                    HANDLE hStdErr;
		    SECURITY_ATTRIBUTES sa;
		    STARTUPINFO si;
		    PROCESS_INFORMATION pi;
		    PROCESS_INFORMATION pidb;
		    char szCmdLine[801];
                    char szHandle[20];
                    int i;

                    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
                    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
                    hStdErr = GetStdHandle(STD_ERROR_HANDLE);

		    sa.nLength = sizeof(sa);
		    sa.lpSecurityDescriptor = NULL;
		    sa.bInheritHandle = TRUE;

		    CreatePipe(&hReadIn, &hWriteIn, &sa, 0);

		    memset(&si, 0, sizeof(si));
		    si.cb = sizeof(si);
		    si.dwFlags = STARTF_USESTDHANDLES;

		    strcpy(szCmdLine, comin[0]);
		    strcat(szCmdLine, " ");
		    strcat(szCmdLine, comin[1]);

                    si.hStdInput = hStdIn;
		    si.hStdOutput = hWriteIn;
                    si.hStdError = hStdErr;
		    if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE,
				DETACHED_PROCESS, NULL, NULL, &si, &pi))
                    { 
                        status2 = RMCMDSTATUS_ERROR;
			log_message(E_RE000E_DAEMON_CREATE_PROC, 0);
		    }
		    CloseHandle(pi.hProcess);
		    CloseHandle(pi.hThread);

		    CreatePipe(&hReadOut, &hWriteOut, &sa, 0);
                    strcpy(szCmdLine, com1[0]);
                    i = 1;
                    while (com1[i] != (char *)NULL)
                    {
		        strcat(szCmdLine, " ");
			strcat(szCmdLine, com1[i]);
                        i++;
                    }

		    si.hStdInput = hReadIn;
		    si.hStdOutput = hWriteOut;
		    si.hStdError = hWriteOut;
		    si.dwFlags |= STARTF_USESHOWWINDOW;
		    si.wShowWindow = SW_HIDE;
		    if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE,
				NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pidb))
                    { 
                        status2 = RMCMDSTATUS_ERROR;
			log_message(E_RE000E_DAEMON_CREATE_PROC, 0);
		    }
		    si.dwFlags &= ~(STARTF_USESHOWWINDOW);
		    CloseHandle(pidb.hThread);
                    CloseHandle(pidb.hProcess);

		    strcpy(szCmdLine, comout[0]);
		    strcat(szCmdLine, " ");
		    strcat(szCmdLine, comout[1]);

                    /* Add the handle to the database process to the 
                       command line */

                    itoa((int)pidb.dwProcessId, szHandle, 10);
                    strcat(szCmdLine, " ");
                    strcat(szCmdLine, szHandle);

                    itoa((int)hWriteOut, szHandle, 10);
                    strcat(szCmdLine, " ");
                    strcat(szCmdLine, szHandle);

		    si.hStdInput = hReadOut;
                    si.hStdOutput = hStdOut;
                    si.hStdError = hStdErr;
		    if (!CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE,
				DETACHED_PROCESS, NULL, NULL, &si, &pi))
                    { 
                        status2 = RMCMDSTATUS_ERROR;
			log_message(E_RE000E_DAEMON_CREATE_PROC, 0);
		    }
		    CloseHandle(pi.hProcess);
		    CloseHandle(pi.hThread);

		    CloseHandle(hReadIn);
		    CloseHandle(hReadOut);
		    CloseHandle(hWriteIn);
	            CloseHandle(hWriteOut);

		    /*
                    ** If any error was encountered at any stage, set the 
                    ** error status in remotecmd table.
		    ** this code has been duplicated from the ifndef WIN32
                    ** section for better readability 
                    */
  	            if (status1 != status2)
                    {
                        exec sql update remotecmd
                                 set status = :status2
                                 where handle = :hdl;
			exec sql commit;
			if (sqlca.sqlcode < 0) 
			{
			    log_message(E_RE000D_DAEMON_STATUS_UPDATE, 1);
			    goto end;
		        }
                
                    }
		}
            } /* (end if(status2 == _ACCEPTED)) */
        } /* (end if(hdl > -1)) */
#endif /* WIN32 */
        else
        {
            status1=RMCMDSTATUS_REQUEST4TERMINATE;
            hdl = -1;
  	    exec sql select status
                 into :hdl
                 from remotecmd
                 where status = :status1;
	    exec sql commit;

	    if (sqlca.sqlcode < 0) 
	    {
#ifndef VMS  /* porting to VMS needed */
		log_message(E_RE000B_DAEMON_READ_FAIL, 1);
#endif  /* VMS */
		goto end;
	    }

            if (hdl >= 0)
            {
		exec sql raise dbevent rmcmdstpnode;
		if (sqlca.sqlcode < 0) 
		{
#ifndef VMS  /* porting to VMS needed */
		    log_message(E_RE000B_DAEMON_READ_FAIL, 1);
#endif  /* VMS */
		    goto end;
	        }

                exec sql delete from remotecmd where status = :status1;
		if (sqlca.sqlcode < 0) 
		{
#ifndef VMS  /* porting to VMS needed */
		    log_message(E_RE000B_DAEMON_READ_FAIL, 1);
#endif  /* VMS */
		    goto end;
	        }

                /*
                ** exec sql commit; 
                ** not needed there since xiting the loop ,
                ** and commit will be done
                */
                goto end;
            }
        }
        exec sql whenever sqlerror goto er0;

	}
       
#ifdef WIN32
#define H_TOTAL_MS 6000
#define H_LOOP_MS    50
#define H_NB_LOOP ( H_TOTAL_MS / H_LOOP_MS)
	if (i4HighReactivity >0)
	{
		exec sql get dbevent;
		exec sql inquire_sql(:stmt=dbeventname);
		if (stmt[0]==EOS)
		{
			i4HighReactivity --;
			Sleep(H_LOOP_MS);
		}
		else
		{
			i4HighReactivity = H_NB_LOOP;
		}
	}
	else
	{
		exec sql get dbevent with wait = 1;
		exec sql inquire_sql(:stmt=dbeventname);
		if (stmt[0]!=EOS)
			i4HighReactivity = H_NB_LOOP;
	}
#else
	exec sql get dbevent with wait = 1;
	exec sql inquire_sql(:stmt=dbeventname);
#endif

	/* node stop event */
	CVlower(stmt);
	if ( !MEcmp( stmt, rmcmdstpnode, STlength(rmcmdstpnode) ) )
	{
	    i4 node_arg;
	    exec sql inquire_sql(:stmt=dbeventtext);
	    CVal(stmt, &node_arg);
	    /* stop all nodes? stop all nodes? stop specific node? */
	    if (stmt[0]==EOS || node_arg==0 || node_arg==cx_node_number) {
		goto end;
	    }
	}
	exec sql commit;

	exec sql whenever sqlerror continue;
    } /* while (1) */
    goto end; /* in case of remaining "break" within the loop */
er0:
    if (sqlca.sqlcode < 0) 
    {
#ifndef VMS  /* porting to VMS needed */
	log_message(E_RE000F_DAEMON_GET_DBEVENT, 1);
#endif  /* VMS */
	goto end;
    }

end:
    exec sql commit;
    exec sql disconnect; 

    exec sql set_sql(session=3); /* restore main rmcmd session */
    exec sql disconnect; 

    MEfill(sizeof(parms), 0, (PTR) &parms);
    IIGCa_cb_call(&gca_cb, GCA_TERMINATE, &parms, GCA_SYNC_FLAG,
		  (PTR)0, -1, &func_stat);

    /*
    ** Remove registrations for config parameters.
    */
    GVcnf_term();
#ifdef VMS
    /* Send message to errlog */
    TMnow( &time_now );
    TMstr( &time_now, buf );
    STprintf(rmlogbuf, "Visual DBA RMCMD Daemon ending at %s", buf );
    rmcmd_log(rmlogbuf, STlength(rmlogbuf));
#else
    log_message(E_RE0002_RMCMD_DOWN, 0);
#endif
}

#define LOCALMAXARGS   10

#define FILE_REPLIC_LOG 1
#define FILE_RUNREPL    2

exec sql begin declare section;
	int iRMInputLine ;
    int iRMOutputLine;
	char buf_userid[100];
	char buf_sessionid[100];
	int  iqHdl;
exec sql end declare section;


static void AddLine(buf)
exec sql begin declare section;
	char *buf;
exec sql end declare section;
{
   iRMOutputLine++;
   exec sql insert into remotecmdout
            values(:iqHdl,:iRMOutputLine,:buf_userid,:buf_sessionid,:buf,'N');
#ifndef VMS  /* porting to VMS needed */
   if (sqlca.sqlcode < 0) 
	log_message(E_RE0010_DAEMON_INSERT_FAIL, 1);
#endif  /* VMS */

}

static void TerminateQuickCmd()
{
  exec sql begin declare section;
      int  status1;
  exec sql end declare section;
   
  /* termination */

  status1=RMCMDSTATUS_FINISHED;
  exec sql update remotecmd
           set status=:status1
           where handle=:iqHdl;
  exec sql raise dbevent rmcmdcmdend;
  exec sql commit;
#ifndef VMS  /* porting to VMS needed */
  if (sqlca.sqlcode < 0) 
	log_message(E_RE000A_DAEMON_UPDATE_FAIL, 1);
#endif  /* VMS */
}

static BOOL isReplicInstalled(szdatabase)
exec sql begin declare section;
char * szdatabase;
exec sql end declare section;
{
exec sql begin declare section;
    int icnt;
	char connectionname[100];
exec sql end declare section;
	STprintf(connectionname,ERx("%s/ingres"),szdatabase);
    exec sql whenever sqlerror goto er1;
	exec sql commit; 
	exec sql connect :connectionname session 2;
	exec sql select count(*) into :icnt from iitables where LOWERCASE(table_name)='dd_cdds';
	exec sql commit;
	exec sql disconnect;
	exec sql set_sql(session= 1); /* restore main rmcmd session */
	exec sql whenever sqlerror continue;
	if (icnt>0)
		return TRUE;
	else
		return FALSE;
er1:
   	exec sql set_sql(session= 1); /* restore main rmcmd session in case error would have occured before that step*/
	AddLine(ERx("* Error in checking"));
	return FALSE;
} 

static bool switch2iidbdbsession()
{
	bool bResult = TRUE;
	exec sql connect 'iidbdb/ingres' session 2;
	if (sqlca.sqlcode <0) {
		exec sql set_sql(session= 1); 
		bResult = FALSE;
	}
	return bResult;
} 
static bool RestoreMainSession()
{
	BOOL bResult = TRUE;
	exec sql disconnect;
	if (sqlca.sqlcode<0)
		bResult = FALSE;
	exec sql set_sql(session= 1); /* restore main rmcmd session */
	if (sqlca.sqlcode<0)
		bResult = FALSE;
	return bResult;
}	  

static void  ManageFileDisplay(char *szcommand,int hdl)
{
	BOOL bFromII_SYSTEM;
exec sql begin declare section;
	char dbaname[100];
	char dbname [100];
	long error_no = 0;
	char error_text[257];
exec sql end declare section;

	char * lpwords[LOCALMAXARGS];
	int count;
	i4 FileType,svr_no;
	LOCATION loc;
	char	path[MAX_LOC+1];

    i4	locflags;
	char buff[81];
	char *p;
    LOINFORMATION locinfo;
	FILE	*fp;
    char	buf[SI_MAX_TXT_REC];

    /*** init hdl, userid, and sessionid for inserts into remotecmdout ***/
    iqHdl = hdl;
    exec sql select user_id,     session_id
             into   :buf_userid, :buf_sessionid
             from   remotecmd
             where  handle=:iqHdl;
   if (sqlca.sqlcode < 0)  {
    	AddLine(ERx("*dddispf: session/userid error"));
		TerminateQuickCmd();
		return;
   }
   iRMOutputLine = -1;

   STtrmwhite(buf_userid);

	/***** parse command and check parameters ******/
    count=LOCALMAXARGS;
    STgetwords(szcommand, &count, lpwords);
	if (count!=4) {
		AddLine("* dddispf: Invalid Parameters");
		TerminateQuickCmd();
		return;
    }

    /*** check if session user is DBA of database ***/
    STcopy(lpwords[1],dbname);
    if (! switch2iidbdbsession()) {
		exec sql commit;
		AddLine("* dddispf: Failure in executing command");
		TerminateQuickCmd();
		return;
    }
    EXEC SQL SELECT own into :dbaname from iidatabase where name=:dbname;
	EXEC SQL INQUIRE_SQL (:error_no = ERRORNO,:error_text = ERRORTEXT);
    if (! RestoreMainSession()) {
		exec sql commit;
		AddLine("* dddispf: Failure in executing command");
		TerminateQuickCmd();
		return;
    }

	if (error_no)   {
		exec sql commit;
		AddLine("* dddispf: Failure in executing command");
		TerminateQuickCmd();
		return;
    }
	STtrmwhite(dbaname);
	if (STbcompare(buf_userid, 0, RmcmdStartUser, 0, TRUE) &&    /* connected as ingres : OK */
	    STbcompare(buf_userid, 0, dbaname    , 0, TRUE)   ) { /* not real session user */ 
	    AddLine("*dddispf can only be launched by the DBA");
		TerminateQuickCmd();
		return;
    }

	/* check if replic objects are installed on requested database */
	if (!isReplicInstalled(dbname)) { 
	    STprintf(buf,"Replication Objects are not installed on %s.",dbname);
        AddLine(buf);
		TerminateQuickCmd();
		return;
    }


     
    /*** get and check FileType ***/
    if (CVan(lpwords[2],&FileType) != OK || 
	    (FileType != FILE_REPLIC_LOG  && FileType !=  FILE_RUNREPL)
	   )   {
        AddLine("*dddispf: Invalid File Type as 2nd Parameter");
	    TerminateQuickCmd();
		return;
    }

    /*** get server no ***/
    if ( CVan(lpwords[3],&svr_no) != OK)   {
    	AddLine("*dddispf: Invalid Server Number as 3rd Parameter");
	  	TerminateQuickCmd();
	    return;
       }
    
    /***fill the location ***/    
	bFromII_SYSTEM = FALSE;
    NMgtAt(ERx("DD_RSERVERS"), &p);
	if (p == NULL || *p == EOS) {
		bFromII_SYSTEM = TRUE;
		NMgtAt(ERx("II_SYSTEM"), &p);
		if (p == NULL || *p == EOS) {
			STprintf(buf,"* DD_RSERVERS and II_SYSTEM are not defined");
			AddLine(buf);
			TerminateQuickCmd();
			return;
		}
    }
	STlcopy(p, path, sizeof(path)-20-1);
	LOfroms(PATH, path, &loc);
	if (bFromII_SYSTEM == TRUE)
	    NMloc(SUBDIR, PATH, ERx("rep"), &loc);
    LOfaddpath(&loc, ERx("servers"), &loc);
    STprintf(buff, ERx("server%d"), svr_no);
    LOfaddpath(&loc, buff, &loc);
	locflags = LO_I_PERMS;
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
        LOtos(&loc, &p);
        STprintf(buf,"*Cannot access Directory %s.\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return;
    }

    if (FileType == FILE_REPLIC_LOG)
       	LOfstfile(ERx("replicat.log"), &loc);
    else
       	LOfstfile(ERx("runrepl.opt"),  &loc);

    locflags = ( LO_I_ALL );
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
        LOtos(&loc, &p);
        STprintf(buf,"**Cannot access file %s .\n",p);
		AddLine(buf);
	    TerminateQuickCmd();
		return;
	}
    if ( (locinfo.li_perms & LO_P_READ) == 0 )   {
        LOtos(&loc, &p);
		STprintf(buf,"*Permission Error on file %s \n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return;
    }
    if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &fp) != OK)	{
	    LOtos(&loc, &p);
        STprintf(buf,"**Cannot Open File %s.\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return;
    }
    /* Read each line of file*/
    while ( SIgetrec(buf, SI_MAX_TXT_REC, fp) != ENDFILE )    {
	    int len;
		while ((len=STlength(buf))>0) {
		    char c = buf[len-1];
			if (c=='\r' || c=='\n')
			   buf[len-1]='\0';
			else
				break;
		}
        AddLine(buf);
	}
    SIclose(fp);
	TerminateQuickCmd();
    return;
}

static void  ManageFileStorage(char *szcommand,int hdl)
{
	BOOL bFromII_SYSTEM;
exec sql begin declare section;
	char dbaname[100];
	char dbname [100];
	long error_no = 0;
	char error_text[257];
    int ordno;
    
exec sql end declare section;

	char * lpwords[LOCALMAXARGS];
	int count;
	i4 svr_no;
	LOCATION loc;
	char	path[MAX_LOC+1];

    i4	locflags;
	char buff[81];
	char *p;
    LOINFORMATION locinfo;
	FILE	*fp;
    char	buf[SI_MAX_TXT_REC];

    /*** init hdl, userid, and sessionid for inserts into remotecmdout ***/
    iqHdl = hdl;
    exec sql select user_id,     session_id
             into   :buf_userid, :buf_sessionid
             from   remotecmd
             where  handle=:iqHdl;
   if (sqlca.sqlcode < 0)  {
    	AddLine(ERx("*dddispf: session/userid error"));
		TerminateQuickCmd();
		return;
   }
   iRMOutputLine = -1;
   iRMInputLine  = -1;

    STtrmwhite(buf_userid);

    
	/***** parse command and check parameters ******/
    count=LOCALMAXARGS;
    STgetwords(szcommand, &count, lpwords);
	if (count!=3) {
		AddLine("* ddstoref: Invalid Parameters");
		TerminateQuickCmd();
		return;
    }

    /*** check if session user is DBA of database ***/
    STcopy(lpwords[1],dbname);
    if (! switch2iidbdbsession()) {
		exec sql commit;
		AddLine(ERx("* ddstoref: Failure in executing command"));
		TerminateQuickCmd();
		return;
    }
    EXEC SQL SELECT own into :dbaname from iidatabase where name=:dbname;
	EXEC SQL INQUIRE_SQL (:error_no = ERRORNO,:error_text = ERRORTEXT);
    if (! RestoreMainSession()) {
		exec sql commit;
		AddLine(ERx("* ddstoref: Failure in executing command"));
		TerminateQuickCmd();
		return;
    }
	if (error_no)   {
		exec sql commit;
		AddLine(ERx("* ddstoref: Failure in executing command"));
		TerminateQuickCmd();
		return;
    }
	STtrmwhite(dbaname);
	if (STbcompare(buf_userid, 0, RmcmdStartUser, 0, TRUE) &&    /* connected as ingres : OK */
	    STbcompare(buf_userid, 0, dbaname,     0, TRUE)   ){  /* not real session user */ 
	    AddLine("*ddstoref available only to the DBA");
		TerminateQuickCmd();
		return;
    }
     
    /* check if replic objects are installed on requested database */
	if (!isReplicInstalled(dbname)) { 
	    STprintf(buf,"Replication Objects are not installed on %s.",dbname);
        AddLine(buf);
		TerminateQuickCmd();
		return;
    }

    /*** get and check serverno ***/
    if ( CVan(lpwords[2],&svr_no) != OK)   {
    	AddLine("*dddispf: Invalid Server Number as 4rd Parameter");
	   	TerminateQuickCmd();
	    return;
    }
   
    /*** fill the location ***/    

	bFromII_SYSTEM = FALSE;
    NMgtAt(ERx("DD_RSERVERS"), &p);
	if (p == NULL || *p == EOS) {
		bFromII_SYSTEM = TRUE;
		NMgtAt(ERx("II_SYSTEM"), &p);
		if (p == NULL || *p == EOS) {
			STprintf(buf,"* DD_RSERVERS and II_SYSTEM are not defined");
			AddLine(buf);
			TerminateQuickCmd();
			return;
		}
    }

	STlcopy(p, path, sizeof(path)-20-1);
	LOfroms(PATH, path, &loc);
	if (bFromII_SYSTEM == TRUE)
	    NMloc(SUBDIR, PATH, ERx("rep"), &loc);
    LOfaddpath(&loc, ERx("servers"), &loc);
    STprintf(buff, ERx("server%d"), svr_no);
    LOfaddpath(&loc, buff, &loc);
	locflags = LO_I_PERMS;
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
        LOtos(&loc, &p);
        STprintf(buf,"*Can't access Directory %s.\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return;
    }
	LOfstfile(ERx("runrepl.opt"), &loc);

	if (SIfopen(&loc, ERx("w"), SI_TXT, 0, &fp) != OK) {
        LOtos(&loc, &p);
        STprintf(buf,"*Cannot Open File %s .\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return;
	}
	AddLine(ERx("Open File OK.Wait :\n"));

    while (1) {
	    BOOL bDone=FALSE;
		sleep (2);
		ordno = -1;
		exec sql  select orderno, instring
			      into       :ordno,  :buf
                  from remotecmdin
                  where handle = :hdl and orderno>:iRMInputLine
                  order by orderno;
        EXEC SQL BEGIN;
        if (ordno>=0) {
           iRMInputLine++;
           if (iRMInputLine!=ordno){
	      STprintf(buf,"*Error in Writing runrepl.opt.\n",p);
              AddLine(buf);
	      SIclose(fp);
              TerminateQuickCmd();
		      return;
           }
	   if (!STlength(buf))
           {
			  bDone=TRUE;
			  exec sql endselect;
	   }
    	   SIfprintf(fp,ERx("%s\n"),buf);
	   SIflush(fp);
       }
       EXEC SQL END;
	
       if (sqlca.sqlcode < 0)  {
		  STprintf(buf,"*Error in Writing runrepl.opt.\n",p);
          AddLine(buf);
	      SIclose(fp);
          TerminateQuickCmd();
	      return;
       }
       exec sql commit;
	   if (bDone)
	      break;
    }
	SIclose(fp);
	TerminateQuickCmd();
    return;
}

static bool ManageStartReplicServer(char * szcommand, int hdl)
{
	BOOL bFromII_SYSTEM;
exec sql begin declare section;
	char dbaname[100];
	char dbname [100];
	long error_no = 0;
	char error_text[257];
exec sql end declare section;
    CL_ERR_DESC     err_code;
	char * lpwords[LOCALMAXARGS];
	int count;
	i4 svr_no;
	LOCATION curloc,svrloc,loc;

	char	curpath[MAX_LOC+1];
    char	svrpath[MAX_LOC+1];
    char	path   [MAX_LOC+1];

    char    repliccmd[100];
    
    i4	locflags;
	char buff[81];
	char *p;
    LOINFORMATION locinfo;
	FILE	*fp;
    char	buf[SI_MAX_TXT_REC];

    /*** init hdl, userid, and sessionid for inserts into remotecmdout ***/
    iqHdl = hdl;
    exec sql select user_id,     session_id
             into   :buf_userid, :buf_sessionid
             from   remotecmd
             where  handle=:iqHdl;
   if (sqlca.sqlcode < 0)  {
    	AddLine(ERx("*dddispf: session/userid error"));
		TerminateQuickCmd();
		return TRUE;
   }
   iRMOutputLine = -1;

   STtrmwhite(buf_userid);
   
   /* get current path, to be restored after changing the path to start the server */
   if (LOgt(curpath, &curloc)!=OK) {
		AddLine("* unable to get current path on server. Replication server not started");
		TerminateQuickCmd();
		return TRUE;
    }
	/***** parse command and check parameter ******/
    count=LOCALMAXARGS;
    STgetwords(szcommand, &count, lpwords);
	if (count!=2 ) {
		AddLine("* ddstart: Invalid Parameters");
		TerminateQuickCmd();
		return TRUE;
    }
    /*** get server no ***/
    if (CVan(lpwords[1],&svr_no) != OK)   {
        AddLine("*dddispf: Invalid server number as 2nd Parameter");
	    TerminateQuickCmd();
		return TRUE;
    }
    
     
    /* find the path */
    
	bFromII_SYSTEM = FALSE;
    NMgtAt(ERx("DD_RSERVERS"), &p);
	if (p == NULL || *p == EOS) {
		bFromII_SYSTEM = TRUE;
		NMgtAt(ERx("II_SYSTEM"), &p);
		if (p == NULL || *p == EOS) {
			STprintf(buf,"* DD_RSERVERS and II_SYSTEM are not defined");
			AddLine(buf);
			TerminateQuickCmd();
			return TRUE;
		}
    }

	STlcopy(p, path, sizeof(path)-20-1);
	LOfroms(PATH, path, &loc);
	if (bFromII_SYSTEM)
	    NMloc(SUBDIR, PATH, ERx("rep"), &loc);
    LOfaddpath(&loc, ERx("servers"), &loc);
    STprintf(buff, ERx("server%d"), svr_no);
    LOfaddpath(&loc, buff, &loc);

	/* store path of runrepl.opt for later changing of directory to start the server*/
	LOcopy(&loc, svrpath, &svrloc);


	locflags = LO_I_PERMS;
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
        LOtos(&loc, &p);
        STprintf(buf,"*Cannot access Directory %s.\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return TRUE;
    }
	
    LOfstfile(ERx("runrepl.opt"),  &loc);

    locflags = ( LO_I_ALL );
    if (LOinfo(&loc,&locflags,&locinfo) != OK)   {
        LOtos(&loc, &p);
        STprintf(buf,"**Cannot access file %s .\n",p);
		AddLine(buf);
	    TerminateQuickCmd();
		return TRUE;
	}
    if ( (locinfo.li_perms & LO_P_READ) == 0 )   {
        LOtos(&loc, &p);
		STprintf(buf,"*Permission Error on file %s \n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return TRUE;
    }
	dbname[0]=EOS;
    if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &fp) != OK)	{
	    LOtos(&loc, &p);
        STprintf(buf,"**Cannot Open File %s.\n",p);
        AddLine(buf);
	    TerminateQuickCmd();
		return TRUE;
    }
    /* Read each line of file*/
    while ( SIgetrec(buf, SI_MAX_TXT_REC, fp) != ENDFILE )    {
	    int len;
		while ((len=STlength(buf))>0) {
		    char c = buf[len-1];
			if (c=='\r' || c=='\n')
			   buf[len-1]='\0';
			else
				break;
		}
	    STlcopy(buf,buff,4);
		if (! STbcompare(buff, 0, "-IDB",0,TRUE)) {
			STcopy(buf+4,dbname);
			break;
		}
	}
    SIclose(fp);
	if (dbname[0]==EOS) {
        STprintf(buf,"** Database parameter not found in runrepl.opt .\n");
		AddLine(buf);
	    TerminateQuickCmd();
		return TRUE;
	}
	
	if ( STbcompare(buf_userid, 0, RmcmdStartUser,0,TRUE)) { /* if user not the same user that was launched rmcmd process, perform additional checks */
		char * pc;
	    STcopy(dbname,buf);
	    pc = strstr (buf,"::")   ;
        if (pc) {
		    *pc=EOS;
			STcopy(pc+2,dbname);
		    if ( STbcompare(strLocalVnode, 0, buf,0,TRUE)) { /* ... and compare it to nodename of localDB */
				STprintf(buf,"** Only the INGRES user can start remotly a replicator server if the local DB is on a different node.\n");
		        AddLine(buf);
    		    TerminateQuickCmd();
				return TRUE;
			}
		}
		       /*** check if session user is DBA of the localDB found in runrepl.opt ***/
		if (! switch2iidbdbsession()) {
			exec sql commit;
		    AddLine("* dddispf: Failure in getting localDB owner");
			TerminateQuickCmd();
			return TRUE;
		}
        EXEC SQL SELECT own into :dbaname from iidatabase where name=:dbname;
	    EXEC SQL INQUIRE_SQL (:error_no = ERRORNO,:error_text = ERRORTEXT);
		if (! RestoreMainSession()) {
			exec sql commit;
		    AddLine("* dddispf: Failure in getting localDB owner");
			TerminateQuickCmd();
			return TRUE;
		}
	    if (error_no)   {
	    	exec sql commit;
		    AddLine("* dddispf: Failure in getting localDB owner");
		    TerminateQuickCmd();
		    return TRUE;
        }
	    STtrmwhite(dbaname);
	    if (STbcompare(buf_userid, 0, RmcmdStartUser, 0, TRUE) && /* connected as same user that launched rmcmd : OK */
	        STbcompare(buf_userid, 0, dbaname,0,TRUE ))  {
 		   AddLine("*The replicator server can only be started by Ingres or the DBA of the local DB"); 
		   TerminateQuickCmd();
		   return TRUE;
        }
	}


    /* change current path before starting server */
    if (LOchange(&svrloc)!=OK) {
    	  AddLine("*Cannot Change directory to start Replication Server");
		  TerminateQuickCmd();
		  return SetPath2ii_temporary(); /* in case the path would be "partially" changed... */
    }

#ifdef WIN32
    /* 31-Jan 2000 (noifr01) repstart no more available under NT. rpserver */
    /* seems now ok */
    STprintf(repliccmd,"rpserver %d",svr_no);
#else
	/* noifr01 03-Sep-1998: replicat replaced by rpserver under UNIX */
	/* (Although the code above, that changes the path,              */
	/* is no more required under UNIX, we keep it because            */
	/* of the the error message that would occur if the              */
	/* path doesn't exist                                            */
    STprintf(repliccmd,"rpserver %d",svr_no);
#endif

	LOcopy(&svrloc, path, &loc);
	LOfstfile(ERx("print.log"),  &loc);

#ifdef WIN32
    if (PCcmdline(NULL, repliccmd, PC_NO_WAIT, &loc, &err_code)!=OK) 
#else
    /* noifr01 03-Sep-98: rpserver under UNIX ends with the & sign    */
	/* and outputs to print.log. therefore PC_NO_WAIT is replaced by  */
	/* PC_WAIT and the location pointer is set to NULL(like in repmgr)*/
    if (PCcmdline(NULL, repliccmd, PC_WAIT, NULL, &err_code)!=OK) 
#endif
	   AddLine("*Error in Starting Replicator Server");

	TerminateQuickCmd();

    /* restore previous path */
    if (LOchange(&curloc)!=OK) {
    	  AddLine("*Cannot Change restore directory after starting Replication Server");
	      log_message(E_RE0016_CANNOT_RESTORE_PATH, 0);
		  return FALSE;
	}

    return TRUE;
}

static bool ManageVerifyRmcmdStartUser(char * szcommand, int hdl)
{
	/* the following query is required for setting static variables used */
	/* by the Addline() call in ts function                              */
	/* (init hdl, userid, and sessionid for inserts into remotecmdout )  */
    iqHdl = hdl;
    exec sql select user_id,     session_id
             into   :buf_userid, :buf_sessionid
             from   remotecmd
             where  handle=:iqHdl;
   if (sqlca.sqlcode < 0)  {
        AddLine(ERx("*ddprocessuser: session/userid error"));
        TerminateQuickCmd();
        return FALSE;
   }

    iRMOutputLine = -1;

    AddLine(RmcmdStartUser);

    TerminateQuickCmd();
    return TRUE;
}

/* Handle the stop node request.
**
** Turn the stop command into an event which can be processed by
** all nodes.
**
** History:
**   24-Oct-2005 (fanch01)
**     Created.
*/
static bool ManageRmcmdStopNode(char * szcommand, int hdl)
{
    exec sql begin declare section;
    char    stopstmt [64];
    exec sql end declare section;
    
    i4 temp = STlength(rmcmdstpnode);

    if (szcommand[temp++] != EOS)
	CVal(&szcommand[temp], &temp);
    else
	temp = 0;

    STprintf(stopstmt,ERx("raise dbevent rmcmdstpnode '%d'"), temp);
    EXEC SQL EXECUTE IMMEDIATE :stopstmt;

    iqHdl = hdl;

    exec sql delete from remotecmd where handle = :iqHdl;
    exec sql commit;

    return TRUE;
}

/*
**  Name: log_message
**
**  Description:
**	This procedure logs messages to the errlog, similar to ule_format().
**
**  Inputs:
**	message_num	- The message number to print.
**	ArgCode		- The message argument code, where 0 = no arguments,
**			  1 = SQL code argument, 2 = system owner argument
**  Output:
**	none
**
**	Returns:
**	    none
**
**  History:
**	08-jun-1999 (somsa01)
**	    Created.
**	19-oct-1999 (somsa01)
**	    Added option to print out the SQL error code or the system owner.
**    16-mar-2005 (mutma03)
**        Added code to translate nodename to nickname for clustered nodes.
**	07-jan-2008 (joea)
**	    Reverse 16-mar-2005 cluster nicknames change.
*/
static void
log_message (i4 message_num, int ArgCode)
{
    char	message[ER_MAX_LEN];
    i4		status, text_length, length;
    CL_ERR_DESC	err_code;

    GChostname(message, sizeof(message));
    if (STlength(message) < 8)
	STcat(message, "	");
    STcat(message, "::[");
    STcat(message, ServerID);
    STcat(message, ", ");
    STcat(message, ServerPID);
    STcat(message, ", ffffffff]: ");
    length = STlength(message);
    if (!ArgCode)
    {
	status = ERslookup( message_num, (CL_ERR_DESC *) NULL, ER_TIMESTAMP,
			    (char *) NULL, &message[length],
			    sizeof(message) - length, 1, &text_length,
			    &err_code, 0, NULL );
    }
    else
    {
	ER_ARGUMENT	er_args[1];

	/*
	** Place the SQL error code in the text.
	*/
	if (ArgCode == 1)
	{
	    /* Add the SQL error code. */
	    er_args[0].er_size = sizeof(i4);
	    er_args[0].er_value = (PTR)&sqlca.sqlcode;
	}
	else if (ArgCode == 2)
	{
	    /* Add the Ingres admin user. */
	    er_args[0].er_size = 0;
	    er_args[0].er_value = SystemAdminUser;
	}
	status = ERslookup( message_num, (CL_ERR_DESC *) NULL, ER_TIMESTAMP,
			    (char *) NULL, &message[length],
			    sizeof(message) - length, 1, &text_length,
			    &err_code, 1, &er_args[0] );
    }

    if (status != OK)
    {
	CL_ERR_DESC	dummy;

	STprintf(&message[length],
		    "LOG_MESSAGE: Couldn't look up message %x ",
		    message_num);
	length = STlength(message);

	STprintf(&message[length], "(reason: ER error %x)\n", status);
	length = STlength(message);
	status = ERslookup( status, &err_code, 0, (char *) NULL,
			    &message[length], sizeof(message) - length, 1,
			    &text_length, &dummy, 0, NULL );
	if (status != OK)
	{
	    STprintf(&message[length],
		    "... ERslookup failed twice:  status = %x", status);
	}
    }

    ERlog(message, STlength(message), &err_code);
}


/*
**  Name: gca_sm
**
**  Description:
**	State machine processing loop for the RMCMD server GCA listen
**	processing.
**
**  Inputs:
**	none
**
**  Outputs:
**	none
**
**  Returns:
**	void
**
**  History:
**	01-feb-2000 (somsa01)
**	    Created.
*/
static void
gca_sm(i4 sid)
{
    GCA_LCB	*lcb = &gca_lcb_tab[sid];
    bool	branch = FALSE;

top:

    branch = FALSE;

    switch (lsn_states[lcb->state++].action)
    {
	case LSA_INIT:			/* Initialize, branch if listening */
	    branch = listening;
	    lcb->sp = 0;
	    lcb->flags = 0;

	    if (!labels[1])
	    {
		i4	i;

		for (i=0; i<NumStates; i++)
		{
		    if (lsn_states[i].action == LSA_LABEL)
			labels[lsn_states[i].label] = i + 1;
		}
            }
            break;

	case LSA_GOTO:			/* Branch unconditionally */
	    branch = TRUE;
	    break;

	case LSA_GOSUB:			/* Call subroutine */
	    lcb->ss[lcb->sp++] = lcb->state;
	    branch = TRUE;
	    break;

	case LSA_RETURN:		/* Return from subroutine */
	    lcb->state = lcb->ss[--lcb->sp];
	    break;

	case LSA_EXIT:			/* Terminate thread */
	    lcb->state = 0;		/* Initialize state */

	    /*
	    ** Exit if there shutting down or there is an
	    ** active listen.  Otherwise, the current
	    ** thread continues as the new listen thread.
	    */
	    if (lcb->flags & LCB_SHUTDOWN || listening)
		return;
            break;

	case LSA_IF_RESUME:		/* Branch if INCOMPLETE */
	    branch = (*lcb->statusp == E_GCFFFE_INCOMPLETE);
	    break;

	case LSA_IF_TIMEOUT:		/* Branch if TIMEOUT */
	    branch = (*lcb->statusp == E_GC0020_TIME_OUT);
	    break;

	case LSA_IF_ERROR:		/* Branch if status not OK */
            branch = (*lcb->statusp != OK);
            break;

	case LSA_IF_SHUT:		/* Branch if SHUTDOWN requested */
	    branch = (lcb->flags & LCB_SHUTDOWN) ? TRUE : FALSE;
	    break;

	case LSA_SET_SHUT:		/* Prepare to shutdown server */
	    GCshut();
	    lcb->stat = E_GC0040_CS_OK;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_CLEAR_ERR:		/* Set status to OK */
	    lcb->stat = OK;
	    lcb->statusp = &lcb->stat;
	    break;

        case LSA_LOG :                  /* Log an error */
            if ((*lcb->statusp != OK) && (*lcb->statusp != FAIL) &&
                (*lcb->statusp != E_GC0032_NO_PEER))
	    {
		gcu_erlog(0, 1, *lcb->statusp, NULL, 0, NULL);
	    }
            break;

        case LSA_CHECK :                /* Background checks */
            break;

        case LSA_LISTEN:                        /* Post a listen */
            {
		MEfill(sizeof(lcb->parms), 0, (PTR)&lcb->parms);
		lcb->statusp = &lcb->parms.gca_ls_parms.gca_status;
		listening = TRUE;

		IIGCa_cb_call(&gca_cb, GCA_LISTEN, &lcb->parms,
                              GCA_ASYNC_FLAG, (PTR)sid, -1, &lcb->stat);
		if (lcb->stat != OK)
		    break;
            }
            return;

	case LSA_LS_RESUME:		/* Resume a listen */
	    IIGCa_cb_call(&gca_cb, GCA_LISTEN, &lcb->parms,
			  GCA_RESUME, (PTR)sid, -1, &lcb->stat);
            if (lcb->stat != OK)
		break;
	    return;

	case LSA_REPOST:		/* Repost a listen */
	    {
		i4	id;

		listening = FALSE;

		for (id=0; id<LCB_MAX; id++)
		{
		    if (!gca_lcb_tab[id].state)
		    {
			gca_sm(id);
			break;
                    }
		}
	    }
	    break;

	case LSA_LS_DONE:		/* Listen request has completed */
	    lcb->assoc_id = lcb->parms.gca_ls_parms.gca_assoc_id;
	    break;

	case LSA_NEGOTIATE:		/* Validate client */
	    lcb->protocol = min(lcb->parms.gca_ls_parms.gca_partner_protocol,
                                GCA_PROTOCOL_LEVEL_62);

	    /*
	    ** Check for shutdown/quiesce request.
	    */
	    while (lcb->parms.gca_ls_parms.gca_l_aux_data > 0)
	    {
		GCA_AUX_DATA	aux_hdr;
		char		*aux_data;
		i4		aux_len;

		MEcopy( lcb->parms.gca_ls_parms.gca_aux_data,
			sizeof(aux_hdr), (PTR)&aux_hdr );
		aux_data = (char *)lcb->parms.gca_ls_parms.gca_aux_data +
			   sizeof( aux_hdr );
		aux_len = aux_hdr.len_aux_data - sizeof(aux_hdr);

		switch (aux_hdr.type_aux_data)
		{
		    case GCA_ID_QUIESCE:
		    case GCA_ID_SHUTDOWN:
			lcb->flags |= LCB_SHUTDOWN;
			break;
                }

		lcb->parms.gca_ls_parms.gca_aux_data =
						(PTR)(aux_data + aux_len);
		lcb->parms.gca_ls_parms.gca_l_aux_data -=
						aux_hdr.len_aux_data;
	    }
	    break;

	case LSA_REJECT:		/* Reject a client request */
	    lcb->stat = E_RE0011_NO_CLIENTS;
	    lcb->statusp = &lcb->stat;
	    break;

	case LSA_RQRESP:		/* Respond to client request */
	    MEfill( sizeof(lcb->parms), 0, (PTR) &lcb->parms);
	    lcb->parms.gca_rr_parms.gca_assoc_id = lcb->assoc_id;
	    lcb->parms.gca_rr_parms.gca_request_status = *lcb->statusp;
	    lcb->parms.gca_rr_parms.gca_local_protocol = lcb->protocol;
	    lcb->statusp = &lcb->parms.gca_rr_parms.gca_status;

	    IIGCa_cb_call(&gca_cb, GCA_RQRESP, &lcb->parms,
			  GCA_ASYNC_FLAG, (PTR)sid, -1, &lcb->stat);
	    if (lcb->stat != OK)
		break;
	    return;

	case LSA_DISASSOC:		/* Disconnect association */
	    MEfill(sizeof(lcb->parms), 0, (PTR) &lcb->parms);
	    lcb->parms.gca_da_parms.gca_association_id = lcb->assoc_id;
	    lcb->statusp = &lcb->parms.gca_da_parms.gca_status;

	    IIGCa_cb_call(&gca_cb, GCA_DISASSOC, &lcb->parms,
			  GCA_ASYNC_FLAG, (PTR)sid, -1, &lcb->stat);
	    if (lcb->stat != OK)
		break;
	    return;

	case LSA_DA_RESUME:		/* Resume disassociate */
	    IIGCa_cb_call(&gca_cb, GCA_DISASSOC, &lcb->parms,
			  GCA_RESUME, (PTR)sid, -1, &lcb->stat);
	    if (lcb->stat != OK)
		break;
	    return;
    }

    if (branch)
	lcb->state = labels[lsn_states[lcb->state - 1].label];

    goto top;
}


/*
**  Name: rmcmd_bedcheck - check if RMCMD Server is running
**
**  Description:
**	This function queries the Name server to see if an RMCMD
**	server is running.
**
**  Input:
**	None.
**
**  Output:
**	None.
**
**  Returns:
**	STATUS
**
**  History:
**	02-feb-2000 (somsa01)
**	    Created.
*/
 
static STATUS
rmcmd_bedcheck()
{
    GCN_CALL_PARMS	gcn_parm;
    char		uid[GCN_UID_MAX_LEN];
    char		gcn_type[6] = "RMCMD";
    STATUS		status;

    if ((status = IIGCn_call(GCN_INITIATE, NULL)) != OK)
	return(status);

    if ((status = IIgcn_check()) != OK)
    {
	IIGCn_call(GCN_TERMINATE, NULL);
	return(status);
    }

    GCusername(uid, sizeof(uid));

    gcn_parm.gcn_host = NULL;
    gcn_parm.gcn_response_func = gcn_response;
    gcn_parm.gcn_flag = 0;
    gcn_parm.gcn_uid = uid;
    gcn_parm.gcn_install = SystemCfgPrefix;
    gcn_parm.gcn_type = gcn_type;
    gcn_parm.gcn_obj = "";
    gcn_parm.gcn_value = "";

    status = IIGCn_call(GCN_RET, &gcn_parm);

    IIGCn_call(GCN_TERMINATE, NULL);

    return(status);
}

/*
**  Name: gcn_response -- handle a response from the name server.
**
**  Description:
**      This function is called by the name server to handle the response to
**      any function.
**
**  History:
**	02-feb-2000 (somsa01)
**	    Created.
*/
static i4
gcn_response(i4 msg_type, char *buf, i4 len)
{
    i4  row_count, op = 0;

    /* Handle the message responses from name server. */
    switch(msg_type)
    {
        case GCN_RESULT:
            /* Pull off the results from the operation */
            buf += gcu_get_int(buf, &op);
            if (op == GCN_RET)
            {
                buf += gcu_get_int(buf, &row_count);
		if (row_count)
		    ServerUp = TRUE;
		else
		    ServerUp = FALSE;
            }
            break;

        case GCA_ERROR:
            /* The name server has flunked the request. */
            break;

        default:
            break;
    }

    return op;
}

/* cc -otst tst.c -lsd -lspat -lingres -lsocket -lelf -lcurses -lgen -lnsl -lm*/
