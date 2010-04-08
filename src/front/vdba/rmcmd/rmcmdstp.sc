/********************************************************************
**
**  Copyright (C) 1995,2000 Ingres Corporation
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmdstp.sc
**    Stop the remote deamon process
**
**    Originally written by Sotheavut Uk
**
**
** History:
**
**	26-Oct-95 (Emmanuel Blattes)
**		Added MING hints and moved the 'main' to the start of the 
**		line so that mkming would recognize it.
**  02-Nov-95 (Sotheavut Uk)
**    Insert with explicit values for all columns
**
**      04-Jan-96 (spepa01)
**          	Add CL headers so that STxcompare()/STlength() can be used.
**	25-Jan-96 (boama01)
**		Added VMS-specific and CL includes; first cut at CL-izing code;
**		see "*CL*" line markings.  Also improved error reporting.
**		Also, use next-higher handle than highest currently in use (to
**		prevent dupl. key).  Also validate that user is INGRES.
**  25-Mar-98 (noifr01)  
**      -Added usage of dbevents to speed up rmcmd  
**    05-oct-1998 (kinte01)
**	  For VMS only, if a user has the SERVER CONTROL he can setup the
**	  RMCMD objects & start/stop the RMCMD server. 
**	  The above changes meant that all SQL queries referencing 
**	  ingres.<whatever> had to be rewritten in dynamic sql to be
**	  the user setting up RMCMD. For UNIX & NT this will always be
**	  ingres. For VMS this may or may not be ingres but will be the
**	  Ingres System Admin User.
**  02-Feb-99 (noifr01)
**     specify the ingres server class when connecting, in order to support 
**     the case where the default server class is not ingres
**  29-nov-1999 (somsa01)
**	For the TNG group, connect to iidbdb as the 'ingres' user when
**	stopping RMCMD. This change is only built into Black Box Ingres.
**  11-sep-2000 (somsa01)
**	TNG_EDITION has been superseded by the use of embed_user.
**  11-oct-2000 (mcgem01)
**      The Ingres II SDK can be installed and run as any user, so it needs
**      to mimic the Embedded Ingres version.
**  16-Oct-2000 (horda03)
**      The RMCMDSTP utility should use PCexit to terminate the program.
**      On VMS the utility causes the INGSTOP to fail, as the program
**      terminates with an error. Bug 102742
**  03-Jan-2001 (noifr01)
**   (SIR #103625) now have rmcmd work against imadb rather than iidbdb
**  26-Apr-2001 (noifr01)
**   (RMCMD counterpart of SIR 103299) increased max lenght of command lines
**   to be executed, and renamed the corresponding column in the remotecmd 
**   table in order to ensure rmcmd under 2.6 doesn't work on tables that
**   wouldn't have been updated (otherwise the commands to be executed could
**   get truncated )
**  27-Feb-2003 (fanra01)
**      Bug 109284
**      Add -force command line option processing to enable sessions to be 
**      dropped.  Also add -drop_only flag for executing drop session
**      procedure.
**  30-jun-2003 (horda03) Bug 110501
**      Need to connect as the Ingres System Account, sp that RMCMD can
**      be stopped.
**  22-Oct-2003 (schph01)
**  (SIR #111153) now have rmcmd run under any user id
**  05-Jan-2004 (schph01)
**  (corrective fix for SIR #111153) replaced LPTSTR with char * for building
**   on any platform.
**  23-feb-2005 (stial01)
**     Explicitly set isolation level after connect
**  24-Oct-2005 (fanch01) 14341370;01
**     When multiple rmcmdstps are issued simultaneously this exposed a race
**     in the logic to obtain a 'handle' and use it later.  Set the readlock
**     to exclusive to ensure no other process can get that handle.
**     Bug: 115442
**	3-Feb-2006 (kschendel)
**	    We won't find 'remotecmd' in an ANSI imadb.  Discovered by
**	    accident during upgrade testing.  Also turned out that userid
**	    had not been initialized.
** 08-Feb-2008 (horda03) Bug 119901
**     If there are more than 1 remotecmd table in the DB, then it is a
**     lottery as to whether the right table will be populated with the
**     Shutdown command. We should now use the $ingres owned table.
** 10-Sep-2009 (frima01) 122490
**     Add include of cx.h and cs.h to avoid warnings from gcc 4.3
*/

/*
PROGRAM =       rmcmdstp

NEEDLIBS =      RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB SQLCALIB \
                COMPATLIB 
*/
/*
	Note that MALLOCLIB should be added when this code has been CLized
*/

/* #include <stdio.h>		*CL* replaced w. CL funcs */
#include <ctype.h>

/** Make sure SQL areas are included before compat.h to prevent GLOBALDEF
 ** conversion of their 'extern' definitions for VMS:
 **/
exec sql include sqlca;
exec sql include sqlda;

/* *CL* -- New includes for CL routines -- */
#include <compat.h>
#include <er.h>
#include <gl.h>
#include <lo.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <id.h>
#include <pc.h>
#include <cs.h>
#include <cx.h>
#ifdef VMS
#include <gc.h>
#include <iicommon.h>
#include <mu.h>
#include <gca.h>
#endif /*VMS*/
#include "dba.h"
#include "rmcmd.h"

/*
** Structure for command line options.
** name         Option string passed on command line
** mask         Bit field to set when option enabled
** message      Message string associated with option
*/
typedef struct _option
{
    char* name;
    i4    mask;
# define OPTMSK_FORCE 0x0001
# define OPTMSK_DROP  0x0002
# define OPTMSK       (OPTMSK_FORCE | OPTMSK_DROP)
    char* message;
}OPT;
enum { OPT_FORCE, OPT_DROP_ONLY };      /* define offsets into option array */

static i4 embed_installation = -1;
static i4 optflgs = 0;                  /* flags for command line options */
static OPT option[] =                   /* available options */
{ 
    { "-force", OPTMSK_FORCE, "Force requested" },
    { "-drop_only", OPTMSK_DROP, "Drop sessions only" },
    { NULL, 0, NULL }
};

#ifndef VMS
#include <gc.h>
#include <iicommon.h>
#include <mu.h>
#include <gca.h>

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

/*
** To stop the remote deamon process, we only need
** inserting a tuple that has a status equal to RMCMDSTATUS_REQUEST4TERMINATE
**
*/

main (int argc, char* argv [])
{

    exec sql begin declare section;
    int  status_val;
    int  handle_val;
    char userid    [40];
    char sessionid [40];
    char cmds      [40];
       char tbl_owner [40];
    int no_remotecmds;
    int ires;
    char stmt [2048];
    exec sql end declare section;
    i4  i;
    i4  j;
    char *user = 0;
    char szName[MAX_USER_NAME_LEN];

    SIeqinit();		/* *CL* Added */

    /*
    ** Parse command line to determine options supplied
    */
    for(i=1; (i < argc) && (argv[i] != NULL); i+=1)
    {
        for(j=0; option[j].name != NULL; j+=1)
        {
            if (!STbcompare( argv[i], 0, option[j].name, 0, TRUE ))
            {
                optflgs |= option[j].mask;
                break;
            }
        }
    }
    /*
    ** Retrieve the user executing this process
    */
    user = szName;
    IDname(&user);
    /*
    ** Check user privileges
    */

    PMinit();
    if( PMload( NULL, (PM_ERR_FUNC *)NULL ) != OK)
    {
      SIeqinit();
      SIprintf( ERx("Error to get information in Config.dat.\n") ); //  log_message(E_RE001A_CONFIG_OPEN_ERROR, 0);
      PCexit(FAIL);
    }
    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );

    if (chk_priv( user, GCA_PRIV_SERVER_CONTROL ) != OK)
    {
       SIeqinit();
       SIprintf( ERx("User '%s' has insufficient privileges for stopping the RMCMD server.\n"),user );
       PCexit(FAIL);
    }

    exec sql connect 'imadb/ingres' identified by '$ingres';
    STcopy("$ingres", userid);

    if (sqlca.sqlcode !=0)
    {
        SIprintf ( ERx("Cannot connect to 'imadb' \n") );
        PCexit(0);
    }

    exec sql set lockmode session where readlock=exclusive;
    exec sql set session isolation level serializable;

    exec sql select count(*) into :no_remotecmds from iirelation
         where relid IN ('remotecmd', 'REMOTECMD');

    if (no_remotecmds > 1)
    {
       SIprintf ( ERx("Multiple 'remotecmd' tables in 'imadb'\n"
                      "Will use table owned by '$ingres'\n") );
    }

    /*
    ** If the option to drop sessions only isn't set carry on as normal
    */
    if ((optflgs & OPTMSK_DROP) == 0)
    {
        /****** Get highest used handle and set new one to one greater ******/
	i4 cx_node_number = CXnode_number(NULL);
	if (cx_node_number < 0) {
	    cx_node_number = 0;
	}

        if (no_remotecmds == 1)
        {
           /* The uppercase version is for an ANSI-uppercase installation */
           exec sql select relowner into :tbl_owner from iirelation
                where relid IN ('remotecmd', 'REMOTECMD');

           STtrmwhite(tbl_owner);

           if (STcasecmp(userid, tbl_owner) != 0)
           {
              /* Need to connect as a different user */

              exec sql disconnect;

              exec sql connect 'imadb/ingres' identified by :tbl_owner;

              if (sqlca.sqlcode !=0)
              {
                 SIprintf ( ERx("Cannot connect to 'imadb' as '%s'\n"), tbl_owner );
                 PCexit(0);
              }

              STcopy(tbl_owner, userid);
           }
        }

        handle_val = -1;

        exec sql select max(handle) into :handle_val from remotecmd;
        status_val = RMCMDSTATUS_SENT;
        handle_val++;	/* (If no existing handles, we will use zero) */

        STcopy( szName, userid );

        STcopy( ERx("ingres_session"), sessionid );
        STprintf(cmds, ERx("rmcmdstpnode %d"), cx_node_number);

        exec sql insert into remotecmd (handle, status, user_id, session_id, cmd1)
            values (:handle_val, :status_val, :userid, :sessionid, :cmds);
        if (sqlca.sqlcode !=0)
        {
            SIprintf ( ERx("Insert of terminate command failed; \n") );
            SIprintf( ERx("%*s\n"), sqlca.sqlerrm.sqlerrml,
                    sqlca.sqlerrm.sqlerrmc );
        }
        else
            ires = RES_SUCCESS;

        STprintf(stmt,ERx("raise dbevent rmcmdstp"));
        EXEC SQL EXECUTE IMMEDIATE :stmt;

        if (sqlca.sqlcode < 0) 
        {
            SIprintf( ERx("Cannot raise dbevent rmcmdstp \n") );
            goto end;
        }
    }
    /*
    ** If options set to drop sessions execute the procedure
    */
    if (optflgs & OPTMSK)
    {
        exec sql execute procedure ima_drop_sessions;
        for (i=0; i < BITS_IN(i4); i+=1)
        {
            if ( optflgs & (1<<i) )
            {
                SIprintf( "%s\n", option[i].message);
            }
        }
    }
    exec sql commit;

    exec sql disconnect;

    PCexit(OK);

end:
    if (ires == RES_SUCCESS)
        exec sql commit;
    else
        exec sql rollback;
    exec sql disconnect;

    PCexit(FAIL);
}


