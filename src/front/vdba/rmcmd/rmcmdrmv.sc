/*
**
**  Copyright (C) 1995,2003 Ingres Corporation
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : rmcmdrmv.sc
**    Removes (drops) tables/views/procedures for remote commands
**
**
** History:
**
**	23-Sep-95 (johna)
**		Added MING hints and moved the 'main' to the start of the 
**		start of the line so that mkming would recognise it.
**	21-Nov-95 (hanch04)
**		Added dest to utility
**
**      04-Jan-96 (spepa01)
**          	Add CL headers so that STxcompare()/STlength() can be used.
**	25-Jan-96 (boama01)
**		Added VMS-specific and CL includes; first cut at CL-izing code;
**		see "*CL*" line markings.  Also improved error reporting.
**  25-Mar-98 (noifr01)  
**      -Added removal of dbevents that now are created for speed up of rmcmd
**      -simplified error management by usage of exec sql whenever sqlerror...
**   9-Apr-98 (noifr01)
**      -don't fail if the dbevents are not found, because they didn't exist
**       on older installations
**    11-sep-1998 (hanch04)
**        C++ comments are against the coding standards.
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
**  11-sep-2000 (somsa01)
**     For embedded Ingres, connect to iidbdb as the 'ingres' user.
**  11-oct-2000 (mcgem01)
**      The Ingres II SDK can be installed and run as any user, so it needs
**      to mimic the Embedded Ingres version.
**  03-Jan-2001 (noifr01)
**   (SIR #103625) now have rmcmd work against imadb rather than iidbdb
**  27-Apr-2001 (horda03)
**     Allow +u parameter. Required on VMS as this utility may be
**     called during an Install of Ingres by a user which isn't the
**     Ingres administrator. (Bug 100039)
**  16-May-2001 (noifr01)
**     manage new -d<dbname> flag for specifying a different database
**     where to remove the rmcmd catalogs. This will be used upon an
**     upgrade operation from 2.5 (or older) to 2.6
**  21-May-2001 (hanje04)
**	Removed the ifdefs from around the defn of main(), making the 
**	return type int across all platforms and adding the arc argv[] 
**	parameters.
**  21-Jul-2003 (devjo01)
**	Have rmcmdrmv continue without complaint if it fails to delete
**	an rmcmd object because it does not exist. (b105785)
**  22-Oct-2003 (schph01)
**  (SIR #111153) now have rmcmd run under any user id
**  05-Jan-2004 (schph01)
**  (corrective fix for SIR #111153) replaced LPTSTR with char * for building
**   on any platform.
**  03-May-2004 (bonro01)
**	Back out incorrect change.
**	07-Jul-2004 (hanje04)
**	    Move main() to start of line so mkjam/mkming can detect it.
*/
   
/*
PROGRAM =       rmcmdrmv

NEEDLIBS =      RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
                UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
                LIBQGCALIB CUFLIB GCFLIB ADFLIB SQLCALIB \
                COMPATLIB 

DEST =          utility

*/
/* 
        Note that MALLOCLIB should be added when this code has been CLized
*/

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
#include <pc.h>
#include <id.h>
#ifdef VMS
#include <gc.h>
#include <iicommon.h>
#include <mu.h>
#include <gca.h>
#endif /*VMS*/
#include "dba.h"
#include "rmcmd.h"
#include "generr.h"

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

static void
error_handler(void)
{
    int	errcode;

    errcode = - sqlca.sqlcode;

    if ( E_GE7594_TABLE_NOT_FOUND == errcode ||
	 E_GE75B2_NOT_FOUND == errcode ||
	 E_GE75BC_UNKNOWN_OBJECT == errcode )
	return;

    exec sql rollback;
    SIeqinit();		
    SIprintf( ERx("Drop rmcmd objects failed\n:") );
    SIprintf( ERx("%*s\n"), sqlca.sqlerrm.sqlerrml,
			    sqlca.sqlerrm.sqlerrmc );
    exec sql disconnect;
    exit(RES_ERR);
} /* error_handler */


int 
main(int argc, char * argv[])
{
  int ires;
  char *user = 0;
  char szName[MAX_USER_NAME_LEN];
  BOOL bDBFlag = FALSE;

  exec sql begin declare section;
    char stmt[2048];
    char *username = 0;
    char connectstring[100];
  exec sql end declare section;

  /* Check for -u<user> specification */
  if ( (argc == 2) && (argv [1][0] == '-') && (argv [1][1] == 'u'))
  {
    username = argv [1];
  }

  if (argc > 1 && argv[1][0]=='-' && argv[1][1]=='d')
  {
    STprintf(connectstring,"%s/ingres",argv[1]+2);
    bDBFlag = TRUE;
  }
  else
    STcopy("imadb/ingres", connectstring);


/*********** Retrieve the user executing this process ******************/
  user = szName;
  IDname(&user);
/*********** Check user privileges *************************************/

  PMinit();
  if( PMload( NULL, (PM_ERR_FUNC *)NULL ) != OK)
  {
    SIeqinit();
    SIprintf( ERx("Read error in config.dat.\n") );
    PCexit(FAIL);
  }
  PMsetDefault( 0, ERx( "ii" ) );
  PMsetDefault( 1, PMhost() );

  if (chk_priv( user, GCA_PRIV_SERVER_CONTROL ) != OK)
  {
     SIeqinit();
     SIprintf( ERx("User '%s' has insufficient privileges for removing the RMCMD objects.\n"),user );
     PCexit(FAIL);
  }

  if (username)
    exec sql connect :connectstring OPTIONS = :username;
  else if (bDBFlag) {
#ifndef VMS
    exec sql connect :connectstring identified by 'ingres';
#else
    exec sql connect :connectstring;
#endif
  }
  else
    exec sql connect :connectstring identified by '$ingres';

  if (sqlca.sqlcode < 0) {
    SIeqinit();
    SIprintf( ERx("Could not connect to system catalog:\n") );
    SIprintf( ERx("%*s\n"), sqlca.sqlerrm.sqlerrml,
                            sqlca.sqlerrm.sqlerrmc );
    PCexit(FAIL);
  }

  ires =RES_ERR;

  exec sql whenever sqlerror call error_handler;

  /************ drop dbevents ***************/
  STprintf(stmt,ERx("drop dbevent rmcmdnewcmd"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;
  STprintf(stmt,ERx("drop dbevent rmcmdnewoutputline"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;
  STprintf(stmt,ERx("drop dbevent rmcmdnewinputline"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;
  STprintf(stmt,ERx("drop dbevent rmcmdcmdend"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;
  STprintf(stmt,ERx("drop dbevent rmcmdstp"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;

  /************ drop remotecmdview view *************************/
  exec sql drop remotecmdview;

  /************ drop remotecmd table  ****************************/
  exec sql drop remotecmd;

  /************ drop remotecmdoutview view *************************/
  exec sql drop remotecmdoutview;

  /************ drop remotecmdout table **********************/
  exec sql drop remotecmdout;

  /************ drop remotecmdinview view *************************/
  exec sql drop remotecmdinview;

  /************ drop remotecmdin table *************************/
  exec sql drop table remotecmdin;

  /************ drop launchremotecmd procedure *********/
  STprintf(stmt,ERx("drop procedure launchremotecmd"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;

  /************ drop sendrmcmdinputprocedure procedure *********/
  STprintf(stmt,ERx("drop procedure sendrmcmdinput"));
  EXEC SQL EXECUTE IMMEDIATE :stmt;


  ires =RES_SUCCESS;

  exec sql commit;

  exec sql disconnect;

  PCexit( (ires == RES_SUCCESS) ? OK : FAIL);
}


