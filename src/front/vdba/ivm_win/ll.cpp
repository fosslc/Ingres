/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Ingres Visual Manager (IVM)
**
**  Source : ll.cpp
**	low-level for getting component list,and a few utilities using the CL
**
**  History (after 01-01-2000):
**	02-02-2000 (noifr01)
**	    (bug 100315) changed the logic for deducting the component
**	    type from the instance name for RMCMD messages
**	10-feb-2000 (somsa01)
**	    Enabled RMCMD for UNIX platforms.
**	07-mar-2000 (somsa01)
**	    Added putting the PID inside the config.lck . Also, if the
**	    config.lck file exists and the PID does not exist anymore,
**	    then successfully lock the file.
**	15-mar-2000 (somsa01)
**	    In mylock_config_data(), pid needs to be of type PID on UNIX.
**	07-Jul-2000 (uk$so01)
**	    BUG #102010
**	    Added ExecCmdLine() and replace some called to Mysystem by
**	    ExecCmdLine() to avoid Resource (HANDLE) leak.
**  20-Jul-2000 (noifr01)
**      additional (emergency) fix for bug 100779:
**      if config.lck already exists and was created by another instance
**      of IVM (the info is available within the file itself), that instance
**      of IVM must have died, otherwise, when starting the current instance
**      of IVM, it would just have activated the other instance.
**      The previous fix, relying on the PID only, did not take into account
**      the fact that upon reboot of the machine, PIDs could get reassigned 
**      with values that existed before the shutdown.
**  05-Sep-2000 (hanch04)
**      replace nat and longnat with i4
**  05-sep-2000 (somsa01)
**	    Corrected missing '}' from last change.
**  26-Sep-2000 (noifr01)
**      (bug 99242) cleanup for DBCS compliance
**  27-Oct-2000 (noifr01)
**     (bug 102276) removed the DESKTOP,NT_GENERIC and IMPORT_DLL_DATA 
**     preprocessor definitions that were hardcoded for NT platforms specifically,
**     given that this is now done at the .dsp project files level
**  21-Dec-2000 (noifr01)
**   (SIR 103548) removed obsolete code
**  18-Jan-2001 (noifr01)
**   (SIR 103727) manage JDBC server
**  21-sep-2001 (somsa01)
**	Added InitMessageSystem(), which basically calls ERinit().
**  27-May-2002 (noifr01)
**   (sir 107879) don't work any more with config.lck. IVM now refreshes the
**   list of components in the background, rather than synchronizing with the
**   start and stop of VCBF
**  02-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management)
**  04-Oct-2002 (noifr01)
**    (bug 108868) have gateway names appear correctly in the IVM tree
**  25-Nov-2002 (noifr01)
**    (bug 109143) management of gateway messages
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 07-Sep-2004 (uk$so01)
**    BUG #112979, The gateways can be up without having DBMS server installed.
**    So, show the gateways independently on DBMS server.
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 01-Apr-2005 (komve01)
**    Bug# 114212/Issue# 13997583. 
**	  Added a retry loop so that VCBFllinit will now retry to
**	  open the config.dat file. Incase anyone else like iisetres has locked this
**	  file, we will wait and try to open it again for MAX_RETRY_COUNT times.
** 20-April-2005 (zhahu02)
**    Updated VCBFllinit (b114354/INGDBA309).
** 20-Sep-2005 (fanra01)
**    Add missing declaration for das_server_name.
** 21-Feb-2007 (bonro01)
**    Remove JDBC package.
** 19-Mar-2009 (drivi01)
**    Remove ui.h includes, it's causing compile problem because of DBSTATUS
**    redefinition.
******************************************************************************/
#include "stdafx.h"
#include "ivm.h"
#include "fileioex.h"
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <share.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL ExecCmdLine (LPCTSTR lpszCmd);

#ifndef MAINWIN
#define NT_INTEL 
#define DEVL 
#define INCLUDE_ALL_CL_PROTOS  
#endif

#define CL_DONT_SHIP_PROTOS   // For parameters of MEfree
#define MAX_RETRY_COUNT		20

#define SEP
#define SEPDEBUG

#ifdef __cplusplus

#ifdef C42
typedef int bool;
#endif // C42

extern "C"
{
#endif

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <ci.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <te.h>
# include <gc.h>
# include <gcccl.h>
# include <erst.h>
#ifndef OPING20
# include <uigdata.h>
#endif
# include <stdprmpt.h>
# include <pc.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include <cm.h>
# include <cv.h>
# include <ug.h>
# include <id.h>
# include "cr.h"
# include "config.h"

FUNC_EXTERN STATUS	CRresetcache();
FUNC_EXTERN bool	unary_constraint( CR_NODE *n, u_i4 constraint_id );
FUNC_EXTERN u_i4	CRtype( i4 idx );

#ifdef __cplusplus
}
#endif

#include "ll.h"
#include <tchar.h>
static char *stdtitle = "Ingres Configuration Manager";
static char          dbms_server_name[100]="server";
static char          net_server_name[100]="server";
static char          jdbc_server_name[100]="server";
static char          star_server_name[100]="server";
static char          bridge_server_name[100]="server";
static char          das_server_name[100]="server";

static void mymessage(char *buf)
{
   MessageBox(GetFocus(),buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}
static void mymessagewithtitle(char *buf)
{
   MessageBox(GetFocus(),buf, stdtitle, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}
static void mymessage2b(char *buf1,char * buf2)
{
   char buf[400];
   sprintf(buf,buf1,buf2);
   MessageBox(GetFocus(),buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
}

char * oneGW() 
{
	static char buf[40];
	CString cstemp;
	if (!cstemp.LoadString(IDS_GW_SINGLE))
		_tcscpy(buf, _T("Gateway"));
	else
		_tcscpy((LPTSTR)buf, (char *)(LPCTSTR)cstemp);
	return buf;
}

char * GWParentBranchString() 
{
	static char buf[40];
	CString cstemp;
	if (!cstemp.LoadString(IDS_GW_PARENTBRANCH))
		_tcscpy(buf, _T("Gateways"));
	else
		_tcscpy((LPTSTR)buf, (char *)(LPCTSTR)cstemp);
	return buf;
}


/*
** Tries to lock config.dat by creating config.lck.
*/
static void
myunlock_config_data( void )
{
	return; /* no more lock of config.dat */

	/*LOdelete( &mutex_loc );*/
}

#define MAXCOMPONENTS 200
static COMPONENTINFO VCBFComponents[MAXCOMPONENTS];
static int VCBFComponentCount=-1;

// this dual list (copy of the main list) is maintained for usage by the background thread, while
// config.lck is not locked any more, typically while VCBF has been invoked in the context, and/or
// just terminated and the main thread is currently in the process of refreshing the list
static COMPONENTINFO VCBFComponentsDualList[MAXCOMPONENTS];
static int VCBFDualListComponentCount=0;


GLOBALDEF char  *host;
GLOBALDEF char  *ii_system;
GLOBALDEF char  *ii_installation;
GLOBALDEF char  *ii_system_name;
GLOBALDEF char  *ii_installation_name;

GLOBALDEF LOCATION      config_file;    
GLOBALDEF LOCATION      protect_file;   
GLOBALDEF LOCATION      units_file;     
GLOBALDEF LOCATION      change_log_file;        
GLOBALDEF LOCATION      key_map_file;   
GLOBALDEF FILE          *change_log_fp;
GLOBALDEF char          change_log_buf[ MAX_LOC + 1 ];  
GLOBALDEF PM_CONTEXT    *config_data;
GLOBALDEF PM_CONTEXT    *protect_data;
GLOBALDEF PM_CONTEXT    *units_data;
GLOBALDEF bool          server;
GLOBALDEF char          tempdir[ MAX_LOC ];
GLOBALDEF bool          ingres_menu=TRUE;

static bool             bDBMS = FALSE;
static LOCATION         help_file;      
static PM_CONTEXT       *help_data;
static char             *syscheck_command = NULL;

static char				bufconfiglog[MAX_LOC];
static long				lconfiglog = 0L;

#define MAXEMBEDLEVEL4GETTINGCOMPLIST 10
static int ilev1 = 0;
static int icomp[MAXEMBEDLEVEL4GETTINGCOMPLIST];

BOOL isDBMS() { return bDBMS; } /* implemented/used for avoiding synchronization needs */

BOOL Push4GettingCompList()
{
	if (ilev1+1>=MAXEMBEDLEVEL4GETTINGCOMPLIST)
		return FALSE;
	ilev1++;
	return TRUE;
}

BOOL Pop4GettingCompList()
{
	if (ilev1<1)
		return FALSE;
	ilev1--;
	return TRUE;
}

static BOOL   bUnlockOnTerminate=FALSE;


static BOOL bReadFromAlternateList = FALSE;
BOOL VCBFllGetFirstComp(LPCOMPONENTINFO lpcomp)
{
	long ltemp =  IVM_GetFileSize( bufconfiglog);
	if (ltemp != lconfiglog) { /* recycle the list in memory */
		if (!VCBFllterminate())
			return FALSE;
		if (!VCBFllinit())
			return FALSE;
		theApp.ConfigureInAction(TRUE);

	}

	if (!bUnlockOnTerminate) { // if the list in the main array is not in a stable state, 
							   // because not complete, or VCBF is currently invoked by IVM, or
							   // it just exited but the refresh process in IVM is not complete,
							   // read from alternate list, which was saved just before VCBF has
							   // been launched
		bReadFromAlternateList = TRUE;
	}
	else
		bReadFromAlternateList = FALSE;

	icomp[ilev1]=-1;
	return VCBFllGetNextComp(lpcomp);
}
BOOL VCBFllGetNextComp(LPCOMPONENTINFO lpcomp)
{
	icomp[ilev1]++;
	if (bReadFromAlternateList) {
		if (icomp[ilev1] >= VCBFDualListComponentCount)
		   return FALSE;
		*lpcomp=VCBFComponentsDualList[icomp[ilev1]];
		return TRUE;
	}
	if (icomp[ilev1] >= VCBFComponentCount)
	   return FALSE;
	*lpcomp=VCBFComponents[icomp[ilev1]];
	return TRUE;
}
static BOOL bGenInfoAlreadyRead= FALSE;


static void myinternalerror()
{
   mymessage("internal error");
   VCBFRequestExit();
}
static void myEXdelete()
{
#ifdef VCBF_USE_EXCEPT_HDL
   EXdelete();
#endif
   return;
}

#ifdef VCBF_USE_EXCEPT_HDL
#ifdef __cplusplus
extern "C"
{
#endif
STATUS
cbf_handler( EX_ARGS *args )
{
       //       STATUS          FEhandler();

	myunlock_config_data();
       //       return( FEhandler( args ) );
       return OK; to be checked if handler needed in the future
}
#ifdef __cplusplus
}
#endif
#endif

static void VCBFInitComponents()
{
  int i;
  for (i=0;i<MAXCOMPONENTS;i++)
     VCBFComponents[i].sztype[0]='\0';
  VCBFComponentCount=0;
}

static int VCBFAddComponent(int itype,char *lptype,char *lpname,char *lpenabled,BOOL blog1,BOOL blog2)
{
  if (VCBFComponentCount>=MAXCOMPONENTS || VCBFComponentCount<0)
    return -1;
  strcpy(VCBFComponents[VCBFComponentCount].sztypesingle,   lptype);
  strcpy(VCBFComponents[VCBFComponentCount].szname,			lpname);
  strcpy(VCBFComponents[VCBFComponentCount].szcount,		lpenabled);
  
  switch (itype) {
		case COMP_TYPE_DBMS:
		  STcopy(ERget(S_ST063C_dbms_servers), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		case COMP_TYPE_NET:
		  STcopy(ERget(S_ST063D_net_servers), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		case COMP_TYPE_DASVR:
		  STcopy(ERget(S_ST069B_das_servers), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		case COMP_TYPE_STAR:
		  STcopy(ERget(S_ST063E_star_servers), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		case COMP_TYPE_BRIDGE:
		  STcopy(ERget(S_ST063F_bridge_servers), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		case COMP_TYPE_GW_ORACLE:
		case COMP_TYPE_GW_INFORMIX:
		case COMP_TYPE_GW_SYBASE:
		case COMP_TYPE_GW_MSSQL:
		case COMP_TYPE_GW_ODBC:
		case COMP_TYPE_GW_DB2UDB:
		  STcopy( GWParentBranchString(), VCBFComponents[VCBFComponentCount].sztype);
		  break;
		default:
		  STcopy(lptype, VCBFComponents[VCBFComponentCount].sztype);
		  break;
  }
  VCBFComponents[VCBFComponentCount].itype = itype;
  VCBFComponents[VCBFComponentCount].blog1 = blog1;
  VCBFComponents[VCBFComponentCount].blog2 = blog2;
  VCBFComponentCount++;
  return VCBFComponentCount;
}

static char szbuf1[400],szbuf2[400],szbuf3[400];
static char * dupstr1(char * p)
{
	strcpy(szbuf1,p);
	return szbuf1;
}
static char * dupstr2(char * p)
{
	strcpy(szbuf2,p);
	return szbuf2;
}
static char * dupstr3(char * p)
{
	strcpy(szbuf3,p);
	return szbuf3;
}

static BOOL updateComponentList( void )
{
	char *name, *value;
	char *enabled1;
	char *enabled2;
	i4 log1_row, log2_row, row = 0;
	char temp_buf[ BIG_ENOUGH ];

	PM_SCAN_REC state;
	STATUS status;
	char *regexp;
	char expbuf[ BIG_ENOUGH ];
	bool use_secure;
	VCBFInitComponents();

	VCBFAddComponent(COMP_TYPE_NAME,
			         dupstr1(ERget( S_ST0300_name_server )),
// next parameter set to empty string for Visual Manager. May need to be #ifdef'ed if the
// source is made common again
			         dupstr2(""/*ERget( S_ST030F_default_settings )*/),
			         dupstr3(ERx( "1" )),FALSE,FALSE) ;
	
	if(!ingres_menu) 
	   VCBFAddComponent(COMP_TYPE_LOCKING_SYSTEM,
			            dupstr1(ERget( S_ST0304_locking_system )),
			            dupstr2(ERget( S_ST030F_default_settings )),
			            dupstr3(ERx( "1" )),FALSE,FALSE) ;

	if( server )
	{
# ifdef UNIX
		bool csinstall = FALSE; 
# endif /* UNIX */
		CL_ERR_DESC cl_err;

		/* examine transaction logs */
# ifndef JASMINE
#ifdef VCBF_FUTURE
      replace it => exec frs message :ERget( S_ST038A_examining_logs ); 
#endif
# ifdef UNIX
		if( PCcmdline((LOCATION *) NULL,
			ERx( "csinstall >/dev/null" ),
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK )
		{
			csinstall = TRUE;
		}
# endif /* UNIX */

		if( PCcmdline( (LOCATION *) NULL,
			ERx( "rcpstat -exist -silent" ),
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK && 
			PCcmdline( (LOCATION *) NULL,
			ERx( "rcpstat -enable -silent" ),
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK )
		{
			enabled1 = ERx( "1" );  
		}
		else
			enabled1 = ERx( "0" );  

		if( PCcmdline( (LOCATION *) NULL,
			ERx( "rcpstat -exist -dual -silent" ),
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK &&
			PCcmdline( (LOCATION *) NULL,
			ERx( "rcpstat -enable -dual -silent" ),
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK )
		{
			enabled2 = ERx( "1" );  
		}
		else
			enabled2 = ERx( "0" );  

# ifdef UNIX
		if( csinstall) 
		{
			(void) PCcmdline((LOCATION *) NULL, ERx( "ipcclean" ),
				PC_WAIT, (LOCATION *) NULL, &cl_err );
		}
# endif /* UNIX */
# endif /* JASMINE */
	}


	/* disable "Security" menu item if security auditing is not setup */ 
	STprintf( temp_buf, ERx( "%s.$.c2.%%" ), SystemCfgPrefix );
	regexp = PMmExpToRegExp( config_data, temp_buf );
	if( !server || 
		PMmScan( config_data, regexp, &state, NULL, &name, &value )
		!= OK )
	{
		use_secure=FALSE;
	}
	else
		use_secure=TRUE;

	/* check for ICE definition */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.icesvr" ),
                   SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	if ( PMmScan( config_data, regexp, &state, NULL, &name, &value )
		== OK )
	{ name = PMmGetElem( config_data, 3, name );
      if( STcompare( name, ERx( "*" ) ) == 0 )
          name = dupstr1(ERget( S_ST030F_default_settings));

	  VCBFAddComponent(COMP_TYPE_INTERNET_COM,
				       ERget( S_ST02FF_internet_comm ),
				       name,value,FALSE,FALSE);
	}

    if (/*server*/1) /*Gateway can be up without having DBMS Server !*/
    { 
      /* check for Oracle Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.oracle" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget(S_ST055C_oracle_gateway));

		VCBFAddComponent(COMP_TYPE_GW_ORACLE,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }
      /* check for DB2UDB Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.db2udb" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST0677_db2udb_gateway));

		VCBFAddComponent(COMP_TYPE_GW_DB2UDB,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }

      /* check for Informix Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.informix" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST055F_informix_gateway));

		VCBFAddComponent(COMP_TYPE_GW_INFORMIX,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }

      /* check for Sybase Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.sybase" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST0563_sybase_gateway));

		VCBFAddComponent(COMP_TYPE_GW_SYBASE,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }

      /* check for MSSQL Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.mssql" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget( S_ST0566_mssql_gateway));

		VCBFAddComponent(COMP_TYPE_GW_MSSQL,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }

      /* check for ODBC Gateway definition */
      STprintf( expbuf, ERx( "%s.%s.%sstart.%%.odbc" ), SystemCfgPrefix, host, SystemExecName );
	  regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
	  &name, &value ); status == OK; status =
	  PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
      { name = PMmGetElem( config_data, 3, name );
	    if( STcompare( name, ERx( "*" ) ) == 0 )
	       name = dupstr1(ERget(S_ST0569_odbc_gateway));

		VCBFAddComponent(COMP_TYPE_GW_ODBC,
                         oneGW(),
                         name,value,FALSE,FALSE);
	  }

    }

	if (server)       /* scan Star definitions */
	{ STprintf( expbuf, ERx( "%s.%s.%sstart.%%.star" ), 
		  SystemCfgPrefix, host, SystemExecName );
      regexp = PMmExpToRegExp( config_data, expbuf );
	  for( status = PMmScan( config_data, regexp, &state, NULL,
 	       &name, &value ); status == OK; status =
		   PMmScan( config_data, NULL, &state, NULL, &name,
		   &value ) )
	  {
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_STAR,
				         ERget( S_ST0309_star_server ),
				         name,value,FALSE,FALSE);
	  }
    }
  
	/* scan Net definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcc" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_NET,
				         ERget( S_ST0308_net_server ), 
				         name,value,FALSE,FALSE);
	}

	   	/* scan DAS definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcd" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
    {  
        name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));
		VCBFAddComponent(COMP_TYPE_DASVR,
				         ERget( S_ST0690_das_server ), 
				         name,value,FALSE,FALSE);
	}

	if (server)       /* scan DBMS definitions */
	{ 
	   STprintf( expbuf, ERx( "%s.%s.%sstart.%%.dbms" ), SystemCfgPrefix, host, SystemExecName );
	   regexp = PMmExpToRegExp( config_data, expbuf );
	   for( status = PMmScan( config_data, regexp, &state, NULL,
            &name, &value ); status == OK; status =
		    PMmScan( config_data, NULL, &state, NULL, &name, &value ) )
	   {
	      name = PMmGetElem( config_data, 3, name );
	      if( STcompare( name, ERx( "*" ) ) == 0 )
              name = dupstr1(ERget( S_ST030F_default_settings ));

          VCBFAddComponent(COMP_TYPE_DBMS,
		                   ERget( S_ST0307_dbms_server ), 
				           name, value,FALSE,FALSE);
		  bDBMS=TRUE;
	   }       
	}

	/* scan Bridge definitions */
	STprintf( expbuf, ERx( "%s.%s.%sstart.%%.gcb" ), 
		  SystemCfgPrefix, host, SystemExecName );
	regexp = PMmExpToRegExp( config_data, expbuf );
	for( status = PMmScan( config_data, regexp, &state, NULL,
		&name, &value ); status == OK; status =
		PMmScan( config_data, NULL, &state, NULL, &name,
		&value ) )
	{
		name = PMmGetElem( config_data, 3, name );
		if( STcompare( name, ERx( "*" ) ) == 0 )
			name = dupstr1(ERget( S_ST030F_default_settings));

		VCBFAddComponent(COMP_TYPE_BRIDGE,
				         ERget( S_ST052B_bridge_server ),
				         name,value,FALSE,FALSE);
	}


# ifndef JASMINE
	if( server )
	{

#ifndef OIDSK		
		/* scan Remote Command definitions */
		STprintf( expbuf, ERx( "%s.%s.%sstart.%%.rmcmd" ), 
			  SystemCfgPrefix, host, SystemExecName );
		regexp = PMmExpToRegExp( config_data, expbuf );
		status=PMmScan( config_data, regexp, &state, NULL,
			&name, &value); 
		if (status)
			value="0";
//		name = dupstr1(ERget( S_ST030A_not_configurable ));
		name = ERx(""); // because of the filtering (since an empty string should appear in /name columns
						// instead of 
		VCBFAddComponent(COMP_TYPE_RMCMD,
					     ERget( S_ST02FE_rmcmd ),
					     name,value,FALSE,FALSE);

#endif
		if(use_secure)
		{
#if 0 // disabled for Visual Manager, since this "oomponent" seems to
	  // be there principally for configuration purpose
		    VCBFAddComponent(COMP_TYPE_SECURITY,
				     dupstr1(ERget( S_ST034E_security_menu )), 
				     dupstr2(ERget( S_ST030F_default_settings)),
				     dupstr3(ERx( "1" )),FALSE,FALSE) ; 
#endif
		}

		VCBFAddComponent(COMP_TYPE_LOCKING_SYSTEM,
				 dupstr1(ERget( S_ST0304_locking_system )), 
				 dupstr2(ERget( S_ST030F_default_settings)),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

		VCBFAddComponent(COMP_TYPE_LOGGING_SYSTEM,
				 dupstr1(ERget( S_ST0303_logging_system )), 
				 dupstr2(ERget( S_ST030F_default_settings)),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

		STprintf( temp_buf, "%s_LOG_FILE", SystemVarPrefix );
		log1_row= VCBFAddComponent(COMP_TYPE_TRANSACTION_LOG,
					   ERget( S_ST0305_transaction_log ),
					   temp_buf,enabled1,TRUE,FALSE);

		STprintf( temp_buf, "%s_DUAL_LOG", SystemVarPrefix );
		log2_row= VCBFAddComponent(COMP_TYPE_TRANSACTION_LOG,
					   ERget( S_ST0305_transaction_log ),
					   temp_buf,enabled2,FALSE,TRUE);

		VCBFAddComponent(COMP_TYPE_RECOVERY,
				 dupstr1(ERget( S_ST0301_recovery_server )),
//				 dupstr2(ERget( S_ST030A_not_configurable )), // [Comment] 14-Aug-1998: UK Sotheavut (UK$SO01)
				 dupstr2(ERget( S_ST030F_default_settings)),  // [Add    ] 14-Aug-1998: UK Sotheavut (UK$SO01)
				 dupstr3(ERx( "1" )),FALSE,FALSE);

		VCBFAddComponent(COMP_TYPE_ARCHIVER,
				 dupstr1(ERget( S_ST0302_archiver_process )),
// next parameter set to empty string for Visual Manager. May need to be #ifdef'ed if the
// source is made common again
				 dupstr2("" /*ERget(S_ST030A_not_configurable)*/),
				 dupstr3(ERx( "1" )),FALSE,FALSE) ;

	}
#ifdef OIDSK
	{
		char buff[200];
    	VCBFAddComponent(COMP_TYPE_OIDSK_DBMS,
	            		 "DBMS server",
                         buff,
						 dupstr3(ERx( "1" )),FALSE,FALSE) ;
	}
# endif /* OIDSK */
# endif /* JASMINE */
       return TRUE;
}

static char   config_buf[ MAX_LOC + 1 ];        
static char   protect_buf[ MAX_LOC + 1 ];       
static char   units_buf[ MAX_LOC + 1 ]; 
static char   help_buf[ MAX_LOC + 1 ];  
static char   locbuf[ BIG_ENOUGH ];

BOOL VCBFllinit(void)
{
	STATUS          status;
	i4             info;
	//CL_ERR_DESC     cl_err;
	char            *value;
	i4			   nRetryCount=0;
//      char            *ii_keymap;
#ifndef OIDSK
	char            *name;
	PM_SCAN_REC     state;
#endif
	char            *temp;   
	char            tmp_buf[ BIG_ENOUGH ];
	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise( ME_INGRES_ALLOC );

# ifdef JASMINE
	ingres_menu=FALSE;
# endif /* JASMINE */
# ifdef NT_GENERIC
	NMgtAt( ERx("II_TEMPORARY"), &temp );
	if( temp == NULL || *temp == EOS )
	{
	    temp = ".";
	}
	STcopy(temp, tempdir);
	STcat(tempdir, ERx("\\"));
# else 
	STcopy(TEMP_DIR, tempdir);
# endif

	/* get local host name */
#if 0   /* remote configuration will be done differently */
	switch( argc )
	{
		case 1:
#endif
			host = PMhost();
#if 0
			break;

		case 2:
			host = argv[ 1 ];
			break;

		default:
			ERROR( ERget( S_ST0427_cbf_usage ) );
	}
#endif
	bDBMS = FALSE;

        /* initialize server names */ 
        STcopy(ERget( S_ST0307_dbms_server ),dbms_server_name);
        STcopy(ERget( S_ST0308_net_server ),net_server_name);
        STcopy(ERget( S_ST0656_jdbc_server ),jdbc_server_name);
        STcopy(ERget( S_ST0309_star_server ),star_server_name);
        STcopy(ERget( S_ST052B_bridge_server ),bridge_server_name);

	/* prepare LOCATION for config.dat */
	NMloc( FILES, FILENAME, ERx( "config.dat" ), &config_file );
	LOcopy( &config_file, config_buf, &config_file );

	/* prepare LOCATION for protect.dat */
	NMloc( FILES, FILENAME, ERx( "protect.dat" ), &protect_file );
	LOcopy( &protect_file, protect_buf, &protect_file );

	/* prepare LOCATION for cbfunits.dat */
	NMloc( FILES, FILENAME, ERx( "cbfunits.dat" ), &units_file );
	LOcopy( &units_file, units_buf, &units_file );

	/* prepare LOCATION for cbfhelp.dat */
	NMloc( FILES, FILENAME, ERx( "cbfhelp.dat" ), &help_file );
	LOcopy( &help_file, help_buf, &help_file );

	/* prepare LOCATION for config.log */
	NMloc( FILES, FILENAME, ERx( "config.log" ), &change_log_file );
	LOcopy( &change_log_file, change_log_buf, &change_log_file );
	LOtos(&change_log_file, &temp);
	STcopy(temp, bufconfiglog);
	lconfiglog = IVM_GetFileSize( bufconfiglog);

	/* load cbfhelp.dat */
	(void) PMmInit( &help_data );
	status = PMmLoad( help_data, &help_file, NULL );
	if( status != OK )
	{
		mymessage2b(ERget( S_ST0319_unable_to_load ), help_buf );
		return FALSE;
	}

	/* load cbfunits.dat */
	(void) PMmInit( &units_data );
	status = PMmLoad( units_data, &units_file, NULL );
	if( status != OK )
	{
		mymessage2b( ERget( S_ST0319_unable_to_load ), units_buf );
		return FALSE;
	}

	/* load config.dat */
	(void) PMmInit( &config_data );
	PMmLowerOn( config_data );
	status = ~OK;
	while( ((status = PMmLoad( config_data, &config_file, NULL )) != OK ) 
		&& nRetryCount < MAX_RETRY_COUNT )
	{
		Sleep(500);//Wait for a while a retry
		nRetryCount++;
	}
	nRetryCount = 0; //Reset retry count in case someone uses it down the
					//line for another retry operation in future.
	if(status != OK)
	{
		//Regret that we have retried but failed. Give up!!
		mymessage(ERget( S_ST0315_bad_config_read ));
		return FALSE;
	}


	/* load protect.dat */
	(void) PMmInit( &protect_data );
	PMmLowerOn( protect_data );
	status = PMmLoad( protect_data, &protect_file, NULL );

	/* intialize configuration rules system */
	CRinit( config_data );

	status = CRload( NULL, &info );

	switch( status )
	{
		case CR_NO_RULES_ERR:
			mymessage(ERget( S_ST0418_no_rules_found ));
			return FALSE;

		case CR_RULE_LIMIT_ERR:
			{
			   char buf[200];
			   sprintf(buf,ERget( S_ST0416_rule_limit_exceeded ), 
				       info );
			   mymessage(buf);
			}
			return FALSE;

		case CR_RECURSION_ERR:
			mymessage2b(
				ERget( S_ST0417_recursive_definition ),
				CRsymbol( info ) );
			return FALSE;
	}
	Request4SvrClassRefresh();	// at this point, serverclasses will be refreshed
								// (only) once (until we unlock/relock for invoking 
								// vcbf). This optimization relies on the fact that
								// config.lck will remain existing until then, and 
								// therefore server classes shouldn't be able to change
								// until then .
	
/*      PCatexit( cbf_exit );  all return FALSE must now be preceeded by */
/*                             a call to unlock_config_data();           */

#ifdef VCBF_USE_EXCEPT_HDL // can be discussed: wouldn't do more than VCBFllterminate()
	if( EXdeclare( cbf_handler, &context ) != OK )
	{
		myEXdelete();
		mymessage("initialization error");
		myunlock_config_data();    
		return FALSE;
	}
#endif

	/* get name of syscheck program if available */
	STprintf( tmp_buf, ERx( "%s.*.cbf.syscheck_command" ),
		  SystemCfgPrefix );
	(void) PMmGet( config_data, tmp_buf, &syscheck_command );

	PMmSetDefault( config_data, 1, host );

	STprintf(tmp_buf, ERx( "%s.$.cbf.syscheck_mode" ), SystemCfgPrefix);
	if( syscheck_command != NULL && PMmGet( config_data,
		tmp_buf, &value ) == OK &&
		ValueIsBoolTrue( value ) )
	{
		syscheck_command = NULL;
	}

	/* determine whether this is server */
	STprintf(tmp_buf, ERx("%s.$.%sstart.%%.dbms"), SystemCfgPrefix, 
		     SystemExecName);
#ifndef OIDSK
	if( PMmScan( config_data, PMmExpToRegExp( config_data, tmp_buf ), 
		     &state, NULL, &name, &value ) == OK ) 
	{
		server = TRUE; 
	}
	else
#endif
		server = FALSE;

	PMmSetDefault( config_data, 1, NULL );

	STprintf (locbuf, "%s_INSTALLATION", SystemVarPrefix);

	ii_installation_name = STalloc (locbuf);

	ii_system_name = STalloc (SystemLocationVariable);

	/* get value of II_SYSTEM */
	NMgtAt( SystemLocationVariable, &ii_system );
	if( ii_system == NULL || *ii_system == EOS )
	{
		myEXdelete();
#ifdef OIDSK
		mymessage(ERget(S_ST0312_no_ii_system));
#else
		mymessage2b(ERget(S_ST0312_no_ii_system),
			    SystemLocationVariable );
#endif
		myunlock_config_data();    
		return FALSE;
	}
	ii_system = STalloc( ii_system );
	
	/* get value of II_INSTALLATION */
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
	if( ii_installation == NULL || *ii_installation == EOS )
	{
#ifdef VMS
		/*
		** VMS system level installations do not
		** have II_INSTALLATION set, but have an 
		** internal installation code of ...
		*/
		ii_installation = ERx( "AA" );
#else
		/*
		** all other environments must have 
		** II_INSTALLATION set properly.
		*/
		myEXdelete();
#ifdef OIDSK
		mymessage( ERget( S_ST0313_no_ii_installation ));
#else
		mymessage2b( ERget( S_ST0313_no_ii_installation ), 
			     SystemVarPrefix );
#endif
		myunlock_config_data();    
		return FALSE;
#endif
	}
	else
		ii_installation = STalloc( ii_installation );

       updateComponentList();

  bUnlockOnTerminate=TRUE;
  return TRUE;
}
BOOL VCBFllterminate(void)
{

	myEXdelete();
        if (bUnlockOnTerminate) {
		// copy list to dual list, which will become the reference until the next VCBFllinit()
		// function invocation will be completed, i.e. typically after VCBF has been called,
		// and IVM has refreshed the corresponding data accordingly
		VCBFDualListComponentCount = VCBFComponentCount;
		for (int i = 0;i<VCBFComponentCount;i++)
			VCBFComponentsDualList[i] = VCBFComponents[i];
        	myunlock_config_data();
                bUnlockOnTerminate=FALSE;
        }
	return TRUE;
}

#ifdef __cplusplus
extern "C"    /* because these are callbacks */
{
#endif
/*
** Returns TRUE if value is a valid directory.  Prompts for
** creation if it doesn't exist.
*/
static bool
is_directory( char *value, bool *canceled )
{
	LOCATION        loc;
	i2              flag;
//        char            tmp_val[ BIG_ENOUGH ];
//        char            *ptv;
//        char            *pv;

	*canceled = FALSE;

	LOfroms( PATH, value, &loc );

	if ( LOisdir( &loc, &flag ) != OK )
	{
	       int ret = MessageBox(GetFocus(),
			       "Directory not found. Do you want to create it?",
			       stdtitle,
			       MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
	       if (ret == IDYES) {
		   if ( LOcreate( &loc ) != OK )
		       return ( FALSE );
	       }
	       else
	       {
		   *canceled = TRUE;
		   return ( FALSE );
	       }
	}
	else if ( !(flag & ISDIR) )
		return ( FALSE );

	return ( TRUE );
}


/*
** Returns TRUE if value is a valid file
*/
static bool
is_file( char *value )
{
	LOCATION        loc;
	i4             flag;
	LOINFORMATION   locinfo;

	LOfroms( PATH & FILENAME, value, &loc );

	flag = LO_I_TYPE;
	if( LOinfo( &loc, &flag, &locinfo ) != OK )
	{
		if ( LOcreate( &loc ) != OK )
			return ( FALSE );
	}
	else if ( !( locinfo.li_type == LO_IS_FILE ) )
		return ( FALSE );

	return ( TRUE );
}



/* return pointer to functions that verify valid pathnames */
BOOLFUNC *
CRpath_constraint(CR_NODE *n, u_i4 constraint_id )
{
//        PTR ptr;
	bool has_constraint = FALSE;

	if ( unary_constraint( n, constraint_id ) == TRUE )
	{
		if ( constraint_id == ND_DIRECTORY )
			return ((BOOLFUNC *)is_directory);
		else
			return ((BOOLFUNC *)is_file);
	}
	return NULL;
	
}
#ifdef __cplusplus
}
#endif

BOOL VCBFGetHostInfos(LPSHOSTINFO lphostinfo)
{
    lphostinfo->host                    =host;
    lphostinfo->ii_system               =ii_system;
    lphostinfo->ii_installation         =ii_installation;
    lphostinfo->ii_system_name          =ii_system_name;
    lphostinfo->ii_installation_name    =ii_installation_name;
    return TRUE;
}


BOOL VCBFllCanCompBeDuplicated(LPCOMPONENTINFO lpcomponent)
{
   if (lpcomponent->itype==COMP_TYPE_OIDSK_DBMS  )
      return FALSE;

   if(
#ifndef WORKGROUP_EDITION
        STcompare( lpcomponent->sztype, dbms_server_name )
        == 0 ||
#endif
        STcompare( lpcomponent->sztype, net_server_name )
        == 0 ||
        STcompare( lpcomponent->sztype, jdbc_server_name )
        == 0 ||
        STcompare( lpcomponent->sztype, das_server_name )
        == 0 ||
        STcompare( lpcomponent->sztype, star_server_name )
        == 0
#ifndef OIDSK
             ||
        STcompare( lpcomponent->sztype, bridge_server_name )
        == 0
#endif
      )
        return TRUE;
   return FALSE;
}

char * GetNameSvrCompName()
{
	return dupstr1(ERget( S_ST0300_name_server ));
}

char * GetICESvrCompName()
{
	return dupstr1(ERget( S_ST02FF_internet_comm ));
}

char * GetLogSysCompName()
{
	return dupstr1(ERget( S_ST0303_logging_system));
}

char * GetLockSysCompName()
{
	return dupstr1(ERget( S_ST0304_locking_system));
}

char * GetArchProcessCompName()
{
	return dupstr1(ERget( S_ST0302_archiver_process));
}

char * GetTransLogCompName()
{
	return dupstr1(ERget( S_ST0305_transaction_log));
}

char * GetSecurityCompName()
{
	return dupstr1(ERget( S_ST034E_security_menu));
}

char * GetRecoverySvrCompName()
{
	return dupstr1(ERget( S_ST0301_recovery_server));
}

char * GetDBMSCompName()
{
		return dupstr1(ERget( S_ST0307_dbms_server));
}

char * GetNetSvrCompName()
{
	return dupstr1(ERget( S_ST0308_net_server ));
}

char * GetDASVRSvrCompName()
{
	return dupstr1(ERget( S_ST0690_das_server ));
}
char * GetStarServCompName()
{
	return dupstr1(ERget( S_ST0309_star_server ));
}

char * GetBridgeSvrCompName()
{
	return dupstr1(ERget( S_ST052B_bridge_server));
}

char * GetRmcmdCompName()
{
	return dupstr1(ERget( S_ST02FE_rmcmd));
}

char * GetLoggingSystemCompName()
{
	return dupstr1(ERget( S_ST0303_logging_system ));
}

char * GetLockingSystemCompName()
{
	return dupstr1(ERget( S_ST0304_locking_system ));
}

char * GetOracleGWName()
{
	return dupstr1(ERget(S_ST055C_oracle_gateway));
}
char * GetDB2UDBGWName()
{
	return dupstr1(ERget( S_ST0677_db2udb_gateway));
}
char * GetInformixGWName()
{
	return dupstr1(ERget(S_ST055F_informix_gateway));
}
char * GetSybaseGWName()
{
	return dupstr1(ERget(S_ST0563_sybase_gateway));
}
char * GetMSSQLGWName()
{
	return dupstr1(ERget(S_ST0566_mssql_gateway));
}
char * GetODBCGWName()
{
	return dupstr1(ERget(S_ST0569_odbc_gateway));
}


BOOL GetCompTypeAndNameFromInstanceName(char * pinstancename, char *bufResType, char * bufResName)
{
	if (!stricmp(pinstancename,"II_ACP")) { // archiver process
		strcpy(bufResType, GetArchProcessCompName());
		strcpy(bufResName,"");
		return TRUE;
	}
	if (!stricmp(pinstancename,"II_RCP")) { // recovery process
		strcpy(bufResType, GetRecoverySvrCompName());
		strcpy(bufResName,GetLocDefConfName());
		return TRUE;
	}
	if (!stricmp(pinstancename,"II_RMCMD")) { // remote command
		strcpy(bufResType, GetRmcmdCompName());
		strcpy(bufResName,"");
		return TRUE;
	}

	if (!stricmp(pinstancename,"DASVR")) { // DASVR
		strcpy(bufResType, GetDASVRSvrCompName());
		strcpy(bufResName,"");
		return TRUE;
	}

	// if instance with special config name or server class, the info is directly
	// available in the special servers list
	if (IsSpecialInstance(pinstancename,bufResType,bufResName))
		return TRUE;

	char * pc = _tcschr(pinstancename,'\\');
	if (pc!=NULL) {
		char bufClass[100];
		strcpy(bufClass,pc+1);
		pc=_tcschr(bufClass,'\\');
		if (pc) {
			*pc='\0';
			if (!stricmp(bufClass,"INGRES")) {  // INGRES -> DBMS
				strcpy(bufResType, GetDBMSCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"IUSVR")) {  // IUSVR -> RECOVERY SERVER
				strcpy(bufResType, GetRecoverySvrCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"COMSVR")) {  // COMSVR -> NET
				strcpy(bufResType, GetNetSvrCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"DASVR")) {  // DASVR -> DASVR
				strcpy(bufResType, GetDASVRSvrCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}

			if (!stricmp(bufClass,"STAR")) {  // STAR -> STAR
				strcpy(bufResType, GetStarServCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"ICESVR")) { // ICESVR -> ICE
				strcpy(bufResType, GetICESvrCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"NMSVR")) {  // NMSVR -> NAME SERVER
				strcpy(bufResType, GetNameSvrCompName());
				strcpy(bufResName,"");  // don't put "default" any more, since there is no other choice
				return TRUE;
			}
			if (!stricmp(bufClass,"BRIDGE")) {  // BRIDGE -> BRIDGE
				strcpy(bufResType, GetBridgeSvrCompName());
				strcpy(bufResName,GetLocDefConfName());
				return TRUE;
			}
			if (!stricmp(bufClass,"RMCMD")) {  // RMCMD -> RMCMD
				strcpy(bufResType, GetRmcmdCompName());
				strcpy(bufResName,"");
				return TRUE;
			}
			if (!stricmp(bufClass,"ORACLE")) {  // ORACLE -> ORACLE Gateway
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetOracleGWName());
				return TRUE;
			}
			if (!stricmp(bufClass,"DB2UDB")) {
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetDB2UDBGWName());
				return TRUE;
			}
			if (!stricmp(bufClass,"INFORMIX")) {
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetInformixGWName());
				return TRUE;
			}
			if (!stricmp(bufClass,"SYBASE")) {
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetSybaseGWName());
				return TRUE;
			}
			if (!stricmp(bufClass,"MSSQL")) {
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetMSSQLGWName());
				return TRUE;
			}
			if (!stricmp(bufClass,"ODBC")) {  // ORACLE -> ORACLE Gateway
				strcpy(bufResType, oneGW());
				strcpy(bufResName,GetODBCGWName());
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL Mysystem(char* lpCmd)
{
#ifdef MAINWIN
	CL_ERR_DESC cl_err;
	return ( PCcmdline( (LOCATION *) NULL,
			lpCmd,PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK);
#else
	return ExecCmdLine ((LPCTSTR)lpCmd);
#endif
}

static BOOL ExecCmdLine (LPCTSTR lpszCmd)
{
	UINT nFlag = 0;
	CString strCmd   = _T("");   // Command line
	CString strInArg = _T("");   // Input argument
	CString strOutArg= _T("");   // Output argument
	CString strText = lpszCmd;

	//
	// lpszCmd is always the form  <executable> [-arg1 ...-argn] [<input file>] [<output file>]
	int nFound1 = -1;
	int nFound2 = -1;

	//
	// find the input file name:
	nFound1 = strText.Find (_T('<'));
	if (nFound1 != -1)
	{
		strCmd = strText.Left (nFound1);
		strCmd.TrimRight();
		nFlag |= INPUT_ASFILE;
	}

	nFound2 = strText.Find (_T('>'));
	if (nFound2 != -1)
	{
		if (nFound1 == -1)
		{
			strCmd = strText.Left (nFound2);
			strCmd.TrimRight();
		}
		else
		{
			strInArg = strText.Mid (nFound1+1, nFound2 - nFound1 -1);
			strInArg.TrimLeft();
			strInArg.TrimRight();
		}

		nFlag |= OUTPUT_ASFILE;
		strOutArg = strText.Mid (nFound2+1);
		strOutArg.TrimLeft();
		strOutArg.TrimRight();
	}
	
	CaFileIOExecParam param;
	param.m_strCommand =  strCmd;
	param.m_strInputArg=  strInArg;
	param.m_strOutputArg= strOutArg;
	param.m_nFlag = nFlag;

	BOOL bOk = FILEIO_ExecCommand (&param);
	//
	// Handle error here if needed:
	return bOk;
}

static char bufLocDefConfname[100];


VOID
InitMessageSystem()
{
    ERinit(ER_SEMAPHORE | ER_MU_SEMFUNCS, 0, 0, 0, 0);
	STcopy(ERget(S_ST030F_default_settings), bufLocDefConfname);
}

char *GetLocDefConfName() { return bufLocDefConfname; }

static long Get95FileSize(char * Filename)
{
	int ihdl;

#ifdef MAINWIN
	ihdl  = _open (Filename, _O_RDONLY | _O_BINARY);
#else
	ihdl = _sopen (Filename,_O_RDONLY | _O_BINARY  ,_SH_DENYNO );
#endif /* MAINWIN */

	if (ihdl<0)
		return (-1L);

	long lres =_lseek( ihdl, 0L, SEEK_END );
	_close( ihdl );
	return lres;
}

long IVM_GetFileSize( char *FileName)
{
	if (theApp.GetPlatformID() == (DWORD) VER_PLATFORM_WIN32_WINDOWS)
			return Get95FileSize(FileName);

	struct _stat buffer;
	int	result = _stat( FileName, &buffer);
	if (result !=0 )
		return -1L;
	return (long)buffer.st_size ;
}

