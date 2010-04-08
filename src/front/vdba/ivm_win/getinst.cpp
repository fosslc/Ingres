/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : CA/Ingres Visual Manager
**
**  Source : getinst.cpp
**    
**  Description:
**	-low-level functions for getting the list of instances of
**	started ingres components,
**	initially derived from the ingstop.c source
**	adapted by : Noirot-Nerin Francois
**	-misc low level functions / classes
** 
**  History:
**	27-Jan-99 (noifr01)
**	    created
**	02-Feb-2000 (noifr01)
**	    (bug 100315) get the rmcmd instance from the iinamu output
**	09-Feb-2000 (somsa01)
**	    On UNIX (MAINWIN), do not run logstat or lockstat if the
**	    shared memory segment is not installed.
**	05-Sep-2000 (hanch04)
**	    replace nat and longnat with i4
**	26-Sep-2000 (uk$so01 and noifr01)
**	    bug 99242 cleanup for DBCS compliance
**	27-Oct-2000 (noifr01)
**	    (bug 102276) removed the DESKTOP,NT_GENERIC and IMPORT_DLL_DATA 
**	    preprocessor definitions that were hardcoded for NT platforms
**	    specifically, given that this is now done at the .dsp project
**	    files level
**	08-dec-2000 (somsa01)
**	    In UNIXProcessRunning(), corrected the proper substitute for
**	    strstr().
**	21-Dec-2000 (noifr01)
**	    (SIR 103548) removed obsolete IsProcessRunning() function, and 
**	    #ifdef OPING20'd code (which was the only place where this
**	    function was called from).(OPING20 shouldn't be used any more
**	    in this codeline)
**	18-Jan-2001 (noifr01)
**	    (SIR 103727) manage JDBC server
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**  28-May-2002 (noifr01)
**   (sir 107879) moved the IVM_GetFileSize() function
**  02-Oct-2002 (noifr01)
**    (SIR 107587) have IVM manage DB2UDB gateway (mapping the GUI to
**    oracle gateway code until this is moved to generic management)
** 09-Mar-2004 (uk$so01)
**    SIR #110013 (Handle the DAS Server)
** 18-Oct-2004 (uk$so01)
**    BUG #113234 / ISSUE 13736006 (Ivm unable to report Ingres status, 
**    if OS user lacks the privilege): in general, the user cannot write
**    to the II_SYSTEM\Ingres\temp directory.
** 10-Nov-2004 (noifr01)
**    (bug 113412) replaced usage of hardcoded strings (such as "(default)"
**    for a configuration name), with that of data from the message file
** 12-Nov-2004 (noifr01)
**    (bug 113412) additional fix: lists of messages of an Ingres category
**    (for example in the "Define Messages and Categories Level" dialog)
**    weren't displayed properly with the japanese language pack for ingres
** 11-Oct-2005 (drivi01)
**     After installation code is retrieved added routines to copy it to a local
**     variable to avoid any modifications to the buffer.
** 02-jun-2006 (drivi01)
**     In Ingres 2007 the format of iinamu has changed and since ivm requires
**     some changes to understand new format I've added routines to skip the master
**     name server displayed in ivm to avoid errors.  
** 15-jun-2006 (drivi01)
**     This change is to complement previous change, after 
**     we strip out master name server from iinamu output, 
**     we need to assign iinamu output to p (refresh)
**     before next server is searched for.
** 25-Jul-2007 (drivi01)
**     On Vista, ivm shows logging system to be online even when it returns errors.
**     Added "ERsend" keyword to be recognized as error when querying
**     logging system and as a result mark it as red to avoid falsly reporting
**     that it's online.
** 24-Oct-2007 (drivi01)
**     Add two variables logstat and lockstat. On Windows, retrieve
**     full path to the binaries to verify that logstat and lockstat
**     are called from correct installation.
** 19-Mar-2009 (drivi01)
**     In effort to port to Visual Studio 2008, clean up the warning.
** 10-Nov-2009 (drivi01)
**     Fix the way IVM detects logging system status. Get it to recognize 
**     the LGinitialize error that is a new behavior due to change #500778.
**     Improve parsing of the ivmlog.inf file in detection of status.
** 12-Nov-2009 (drivi01)
**     Add the missing equal sign for previous change.  The change still 
**     works correctly as it's looking for ONLINE keyword next, the 
**     currently broken line will not actually break the logic of the 
**     routine.
**     Cleanup warning in this file for unreferenced pathBuf.
** 
*/

//#define WORKGROUP_EDITION //to be #defined at the makefile or environment level
#include "stdafx.h"
#include "mainfrm.h"
#include "ivmdml.h"
#include "ivm.h"
#include "getinst.h"
#include "wmusrmsg.h"
#include "ll.h"
#include "evtcats.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <share.h>

#ifndef MAINWIN
#define NT_INTEL 
#define DEVL 
#define INCLUDE_ALL_CL_PROTOS  
#endif

#define CL_DONT_SHIP_PROTOS   // For parameters of MEfree

#define SEP
#define SEPDEBUG

#ifdef __cplusplus

#ifdef C42
typedef int bool;
#endif // C42

#ifdef OIDSK
char *SystemLocationVariable     = "II_SYSTEM";
char *SystemLocationSubdirectory = "ingres";
char *SystemVarPrefix            = "II";
char *SystemCfgPrefix            ="ii";
char *SystemExecName             ="ing";
#endif
extern "C"
{
#endif

#if defined (MAINWIN)
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <me.h>
# include <lo.h>
# include <si.h>
# include <cv.h>
# include <nm.h>
# include <er.h>
# include <cm.h>
# include <st.h>
# include <pm.h>
# include <ci.h>
# include <winbase.h>

#ifdef __cplusplus
}  // extern "C"
#endif




static int  get_procs( );
static void find_procs( );
static char *find_key( char *buf, char *key );
static BOOL start_process( char *cmdbuf );
static void get_server_classnames( void );
static void get_sys_dependencies( void );
static BOOL gateway_configured (char * gateway_class);


# define MAX_SERVER_CLASSES 32

/*
** Various defaults for system names.  These can be
** overridden in get_sys_dependencies()
*/
static char iigcn_key[GL_MAXNAME]       = "IINMSVR";
static char gcn_server_class[GL_MAXNAME]  = "NMSVR";
static char iigcc_key[GL_MAXNAME]       = "COMSVR";
static char iijdbc_key[GL_MAXNAME]       = "JDBC";
static char iidas_key[GL_MAXNAME]       = "DASVR";
static char iigcb_key[GL_MAXNAME]       = "BRIDGE";
static char iigws_key[GL_MAXNAME]       = "OPINGDT";
static char rmcmd_key[GL_MAXNAME]       = "RMCMD";
static int  num_dbms_classes;
static char iidbms_key[MAX_SERVER_CLASSES][GL_MAXNAME];
static char iirecovery_key[GL_MAXNAME]  = "IUSVR";
static int  num_star_classes;
static char iistar_key[MAX_SERVER_CLASSES][GL_MAXNAME];
static char icesvr_key[GL_MAXNAME]      = "ICESVR";
static char oracle_key[GL_MAXNAME]      = "ORACLE";
static char db2udb_key[GL_MAXNAME]      = "DB2UDB";
static char informix_key[GL_MAXNAME]    = "INFORMIX";
static char mssql_key[GL_MAXNAME]       = "MSSQL";
static char sybase_key[GL_MAXNAME]      = "SYBASE";
static char odbc_key[GL_MAXNAME]        = "ODBC";
static char ingstop[GL_MAXNAME]         = "ingivmp"; // changed in order to avoid conflicts if ingstop running concurrently
static char iinamu[GL_MAXNAME]          = "iinamu";
static char iimonitor[GL_MAXNAME]       = "iimonitor";
static char iigwstop[GL_MAXNAME]        = "iigwstop";
static char iigcn[GL_MAXNAME]           = "iigcn";
static char iigcc[GL_MAXNAME]           = "iigcc";
static char iigcb[GL_MAXNAME]           = "iigcb";
static char iidbms[GL_MAXNAME]          = "iidbms";
static char icesvr[GL_MAXNAME]           = "icesvr";
static char dmfrcp[GL_MAXNAME]          = "dmfrcp";
static char iiclean[MAX_LOC+1]          = "ipcclean";
static char iinetu[GL_MAXNAME]          = "netu";
static char gwstop[GL_MAXNAME]          = "gwstop";
static char logstat[MAX_LOC+1]		= "logstat";
static char lockstat[MAX_LOC+1]		= "lockstat";

char        ii_install[3];
char        *pii_install;
char        *ii_temp;
BOOL	    dbms_up = FALSE;
BOOL	    star_up = FALSE;
BOOL	    rcp_up = FALSE;
int         iigcn_count = 0;
char        iigcn_id[64];
int         iigcc_count = 0;
char        iigcc_id[256];
int         iijdbc_count = 0;
char        iijdbc_id[256];
int         iidas_count = 0;
char        iidas_id[256];
int         iigcb_count = 0;
char        iigcb_id[256];
int         iigws_count = 0;
char        iigws_id[256];
int         iidbms_count = 0;
char        iidbms_id[256];
int         iirecovery_count = 0;
char        iirecovery_id[256];
int         iistar_count = 0;
char        iistar_id[256];
int         iirmcmd_count = 0;
char        iirmcmd_id[256];
int         icesvr_count = 0;
char        icesvr_id[256];
int         oracle_count = 0;
char        oracle_id[256];
int         db2udb_count = 0;
char        db2udb_id[256];
int         informix_count = 0;
char        informix_id[256];
int         mssql_count = 0;
char        mssql_id[256];
int         sybase_count = 0;
char        sybase_id[256];
int         odbc_count = 0;
char        odbc_id[256];

// added for logging/locking system info
BOOL	bLockingOn    = FALSE;
BOOL	bLoggingOn    = FALSE;
BOOL	bPrimaryLog   = FALSE;
BOOL	bSecondaryLog = FALSE;
BOOL	bArchive      = FALSE;


//BOOL        service   = FALSE;
//BOOL        showonly  = FALSE;
//BOOL        kill      = FALSE;
//BOOL        force     = FALSE;
//BOOL        immediate = FALSE;
BOOL	    Is_w95    = FALSE;
BOOL	    Is_Jasmine= FALSE;
int         minutes   = 0;
char        readbuf[8192];
char        readbuf2[8192];

static BOOL bCompListHasBeenRequested = FALSE;

static BOOL ESL_FillNewFileFromBuffer(char * filename,char * buffer, int buflen, BOOL bAdd0D0A)
{
   HFILE hf;
   UINT uiresult;
   char bx[] = {0x0D, 0x0A, '\0'};
   BOOL bResult = TRUE;

   hf = _lcreat (filename, 0);
   if (hf == HFILE_ERROR)
   {
      DWORD dwLastError = GetLastError();
      LPVOID lpMsgBuf;
      FormatMessage (
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          dwLastError,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
          (LPTSTR)&lpMsgBuf,
          0,
          NULL);
      CString strMsg = (LPCTSTR)lpMsgBuf; // Message to be used if needed
      LocalFree(lpMsgBuf);
      IVM_ShellNotification (NIM_DELETE, 1, NULL);
      theApp.m_pMainWnd->PostMessage(WM_QUIT);
      return FALSE;
   }

   uiresult =_lwrite(hf, buffer, buflen);
   if ((int)uiresult!=buflen)
      bResult = FALSE;

   if (bAdd0D0A && bResult == TRUE) {
     uiresult=_lwrite(hf, bx, 2);
     if ((int)uiresult!=2)
        bResult = FALSE;
   }
   
   hf =_lclose(hf);
   if (hf == HFILE_ERROR)
        bResult = FALSE;

   return bResult;
}

static char * stristr(char * lpstring, char * lpString2Find)
{
   int len=(int)_tcslen(lpString2Find);
   while(*lpstring) {
      if (!strnicmp(lpstring,lpString2Find,len))
         return lpstring;
      lpstring=_tcsinc(lpstring);
   }
   return (char *)0;
}


static BOOL bSvrClassNamesGotten = FALSE;
void Request4SvrClassRefresh() { bSvrClassNamesGotten = FALSE; }

static BOOL bsysdepsgotten = FALSE;

BOOL UpdInstanceLists()
{
    LOCATION 	loc;

	// execute the following section once only for speed optimization.
	// (could also be called at initialization of IVM, but the current code 
	//  remains closer to the ingstop code it is derived from. May be cleaned up
	if (!bsysdepsgotten) {  
		get_sys_dependencies();
		/*
		** If II_INSTALLATION isn't set, then we should exit.
		*/

		NMgtAt( ERx("II_INSTALLATION"), &pii_install );
		if(pii_install == NULL || *pii_install == EOS )
			return FALSE;
		STcopy(pii_install, ii_install);

		NMgtAt( ERx("II_TEMPORARY"), &ii_temp );
		if( ii_temp == NULL || *ii_temp == EOS ||
			( LOfroms(NODE&PATH, ii_temp, &loc) == OK && LOexist(&loc) == FAIL) ) 
		{
			//SIprintf( "The directory specified by II_TEMPORARY cannot be accessed.\nThe current directory will be used.\n"); 
			ii_temp = ".";
		}
		bsysdepsgotten = TRUE;
	}


    // Query the server class names from config.dat 
	// (only once as long as the config.dat remains locked)
	if (!bSvrClassNamesGotten) {
	    PMinit();
		if (PMload (NULL, NULL) == OK)
			get_server_classnames();
		bSvrClassNamesGotten = TRUE;
	}

    if ( get_procs() )
    {
		return FALSE;
		//PCexit(1);
    }

    find_procs();

	bCompListHasBeenRequested = TRUE;
	return TRUE;
}

static BOOL isInstanceInIINamuOutput( char * pinstance)
{
	char *p = _tcsstr ( readbuf2, pinstance );
	if (!p) 
		return FALSE;
	p+=_tcslen(pinstance);
	if (!isspace( *p ))
		return FALSE;
	return TRUE;
}


static BOOL bDispMsg4SpecNameNoMoreExist = TRUE;

BOOL LLGetComponentInstances (CaTreeComponentItemData* pComponent, CTypedPtrList<CObList, CaTreeComponentItemData*>& listInstance)
{
	char * p1;
	int  nbinstances, i;
	CaTreeComponentInstanceItemData* pItem = NULL;

	if (!bCompListHasBeenRequested)
		UpdInstanceLists();

	// for component types that may have "special instances" (special config name or alternate server class), 
	// scan the "special list" first
	switch (pComponent->m_nComponentType) {
		case COMP_TYPE_DBMS         :
		case COMP_TYPE_STAR         :
		case COMP_TYPE_NET          :
		case COMP_TYPE_JDBC         :
		case COMP_TYPE_DASVR        :
		case COMP_TYPE_BRIDGE       :
		case COMP_TYPE_INTERNET_COM :
#ifdef MAINWIN // under UNIX, the name server is in the special list
		case COMP_TYPE_NAME         :
#endif
			{
				// loop on the "special instances"
				CaParseInstanceInfo * pInstance = NULL;
				POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
				while (pos != NULL)
				{
					pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);
					if (pInstance->m_csConfigName.CompareNoCase((LPCTSTR)pComponent->m_strComponentName)==0 &&
						pComponent->m_nComponentType == pInstance->m_iservertype) {
						// found special instance in the list, for that server type
						// and config name. check if the instance is running
						if (isInstanceInIINamuOutput( (char *) (LPCTSTR) pInstance->m_csInstance)) {
							pItem = new CaTreeComponentInstanceItemData();
							pItem->m_strComponentName = pComponent->m_strComponentName;
							pItem->m_strComponentType = pComponent->m_strComponentType;
							pItem->m_nComponentType   = pComponent->m_nComponentType;
							pItem->m_strComponentInstance = pInstance->m_csInstance;
							listInstance.AddTail (pItem);
						}
					}
				}
			}
			// if not the "default" sub-branch, there is no chance to find more, since the special
			// config names are availabler only through the "special instances" list
			if (pComponent->m_strComponentName.CompareNoCase(GetLocDefConfName())!=0) 
				return TRUE;

	}
	
	switch (pComponent->m_nComponentType) {
		case COMP_TYPE_NAME         :
			nbinstances = iigcn_count;
			p1=iigcn_id;
			break;
		case COMP_TYPE_DBMS         :
			nbinstances = iidbms_count;
			p1=iidbms_id;
			break;
		case COMP_TYPE_INTERNET_COM :
			nbinstances = icesvr_count;
			p1=icesvr_id;
			break;
		case COMP_TYPE_NET          :
			nbinstances = iigcc_count;
			p1=iigcc_id;
			break;
		case COMP_TYPE_JDBC          :
			nbinstances = iijdbc_count;
			p1=iijdbc_id;
			break;
		case COMP_TYPE_DASVR          :
			nbinstances = iidas_count;
			p1=iidas_id;
			break;
		case COMP_TYPE_BRIDGE       :
			nbinstances = iigcb_count;
			p1=iigcb_id;
			break;
		case COMP_TYPE_STAR         :
			nbinstances = iistar_count;
			p1=iistar_id;
			break;
		case COMP_TYPE_RECOVERY     :
			nbinstances = iirecovery_count;
			p1=iirecovery_id;
			break;
		case COMP_TYPE_RMCMD        :
			nbinstances = iirmcmd_count;
			p1=iirmcmd_id;
			break;
		case COMP_TYPE_GW_ORACLE    :
			nbinstances = oracle_count;
			p1=oracle_id;
			break;
		case COMP_TYPE_GW_DB2UDB    :
			nbinstances = db2udb_count;
			p1=db2udb_id;
			break;
		case COMP_TYPE_GW_INFORMIX  :
			nbinstances = informix_count;
			p1=informix_id;
			break;
		case COMP_TYPE_GW_SYBASE    :
			nbinstances = sybase_count;
			p1=sybase_id;
			break;
		case COMP_TYPE_GW_MSSQL     :
			nbinstances = mssql_count;
			p1=mssql_id;
			break;
		case COMP_TYPE_GW_ODBC      :
			nbinstances = odbc_count;
			p1=odbc_id;
			break;

		case COMP_TYPE_SECURITY :
			if (iidbms_count>0 )
				nbinstances = 1;
			else
				nbinstances = 0;
			p1=_T("Not Displayable");
			break;

		case COMP_TYPE_LOCKING_SYSTEM :
			nbinstances = bLockingOn? 1:0;
			p1=_T("Not Displayable");
			break;
		case COMP_TYPE_LOGGING_SYSTEM :
			nbinstances = bLoggingOn? 1:0;
			p1=_T("Not Displayable");
			break;
		case COMP_TYPE_TRANSACTION_LOG:
			nbinstances = 0;
			if (!strcmp(_T("Primary Transaction Log"),pComponent->m_strComponentType)) {
				if (bLoggingOn && bPrimaryLog)
					nbinstances =1;
			}
			else {
				if (bLoggingOn && bSecondaryLog)
					nbinstances =1;
			}
			p1=_T("Not Displayable");
			break;
		case COMP_TYPE_ARCHIVER       :
			nbinstances=bArchive ? 1:0;
			p1=_T("Not Displayable");
			break;
	}
	for (i=0;i<nbinstances;i++) {
		BOOL bKeepInDefaultBranch = TRUE;
		switch (pComponent->m_nComponentType) {
			case COMP_TYPE_DBMS         :
			case COMP_TYPE_STAR         :
			case COMP_TYPE_NET          :
			case COMP_TYPE_JDBC         :
			case COMP_TYPE_DASVR        :
			case COMP_TYPE_BRIDGE       :
			case COMP_TYPE_INTERNET_COM :
#ifdef MAINWIN // under UNIX, the name server is in the special list
			case COMP_TYPE_NAME         :
#endif
			{
				// don't take those of the "special instances" list (already managed)
				CaParseInstanceInfo * pInstance = NULL;
				POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
				while (pos != NULL)
				{
					pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);
					if (pInstance->m_iservertype == pComponent->m_nComponentType &&
					    pInstance->m_csInstance.CompareNoCase(p1) == 0
					   ) {
						bKeepInDefaultBranch=FALSE; 
						break;
					}
				}
			}
		}
		if (bKeepInDefaultBranch) {
			pItem = new CaTreeComponentInstanceItemData();
			pItem->m_strComponentName = pComponent->m_strComponentName;
			pItem->m_strComponentType = pComponent->m_strComponentType;
			pItem->m_nComponentType   = pComponent->m_nComponentType;
			pItem->m_strComponentInstance = p1;
			listInstance.AddTail (pItem);
		}
		if (i+1<nbinstances)
			p1+=_tcslen(p1)+1;
	}
	// add to "default" branch those servers of the "special list" with a config name
	// that doesn't exist any more, and provide a warning
	if (pComponent->m_strComponentName.CompareNoCase(GetLocDefConfName())==0)  {
		switch (pComponent->m_nComponentType) {
			case COMP_TYPE_DBMS         :
			case COMP_TYPE_STAR         :
			case COMP_TYPE_NET          :
			case COMP_TYPE_JDBC         :
			case COMP_TYPE_DASVR        :
			case COMP_TYPE_BRIDGE       :
			{
				// loop on the "special instances", and don't take those:
				//    -to be taken in another branch
				//    -already taken above in the same function call
				CaParseInstanceInfo * pInstance = NULL;
				POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
				while (pos != NULL)
				{
					pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);

					if (pInstance->m_iservertype == pComponent->m_nComponentType &&
						pInstance->m_csConfigName.CompareNoCase(GetLocDefConfName())!=0 &&
						isInstanceInIINamuOutput( (char *) (LPCTSTR) pInstance->m_csInstance)) {
						// found a running "special instance" of the given server type, where the
						// config name is not "(default)". If this config name doesn't exist
						// any more, add the instance to the "(default)" branch (currently being filled)
						// after a warning
						COMPONENTINFO comp;
						BOOL bFound = FALSE;

						// the caller may already within the same loop. 
						// call to Push4GettingCompList/Pop4GettingCompList is required
						
						Push4GettingCompList();
						for (BOOL bOK=VCBFllGetFirstComp(&comp);bOK;bOK=VCBFllGetNextComp(&comp)) {
							if (comp.itype == pComponent->m_nComponentType &&
								pInstance->m_csConfigName.CompareNoCase(comp.szname) == 0) {
								bFound = TRUE;
								break;
							}
						}
						Pop4GettingCompList();

						if (!bFound) {
							if (!pInstance->m_bMsgInstNoConfigNameShown) {
								CString strMsg;
								strMsg.Format(IDS_W_INST_CONFIGNAME_NOEXIST,
											(LPCTSTR) pInstance->m_csInstance,
											(LPCTSTR) pComponent->m_strComponentType,
											(LPCTSTR) pInstance->m_csConfigName,
											(LPCTSTR) pComponent->m_strComponentType);
								LPTSTR lpszError = new TCHAR [strMsg.GetLength() + 1];
								lstrcpy (lpszError, strMsg);
								::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 1, (LPARAM)lpszError);
								pInstance->m_bMsgInstNoConfigNameShown = TRUE;
							}
							pItem = new CaTreeComponentInstanceItemData();
							pItem->m_strComponentName = pComponent->m_strComponentName;
							pItem->m_strComponentType = pComponent->m_strComponentType;
							pItem->m_nComponentType   = pComponent->m_nComponentType;
							pItem->m_strComponentInstance = pInstance->m_csInstance;
							listInstance.AddTail (pItem);
						}
					}
				}
			}
		}
	}
	return TRUE;
}

static void MoveInstanceString(char * pdest, char * psrc)
{
	while (!_istspace(*psrc)) { /* normally DBCS compliant because of range of leading/trailing bytes */
		*pdest++ = *psrc;
		*psrc = ' ';
		psrc++;
	}
	*pdest = '\0';
	return;

}

static void FormatKey2Find(char * pdest, char * pinstall, char * pCompKey)
{
#ifdef MAINWIN
	sprintf(pdest," %s ",pCompKey);
#else
	sprintf(pdest,"%s\\%s\\",pinstall, pCompKey);
#endif
	return;
}

static char * Move2InstanceStringStart(char *p)
{
#ifdef MAINWIN
	p=_tcsinc(p); //skip leading space 
	while ( *p) { 
		if (isspace( *p )) { // found first space after the "key" (server class)
			while (isspace( *p )) // look for first non_blank character
					p=_tcsinc(p);
			return p;
		}
		p=_tcsinc(p);
	}
	// if nothing found, p points to the trailing \0
#endif // under NT, the pointer already points to the start of the instance string: nothing to do
	return p;
}


BOOL bGetInstMsgSpecialInstanceDisplayed = FALSE;
static
void
find_procs( )
{
    char    kbuf[64];
    char    *kp;
    char    *kp1;
    char    *p = readbuf;
    char    *p2 = readbuf;
    int     i;

    iigcn_count = 0;
    iigcc_count = 0;
    iijdbc_count = 0;
    iidas_count = 0;
    iigcb_count = 0;
    iigws_count = 0;
    iidbms_count = 0;
    iirecovery_count = 0;
    iistar_count  = 0;
    oracle_count  = 0;
    db2udb_count  = 0;
    sybase_count  = 0;
    informix_count= 0;
    mssql_count   = 0;
    odbc_count    = 0;
	iirmcmd_count = 0;
	icesvr_count  = 0;
	bLockingOn = FALSE;
	bLoggingOn = FALSE;
	bPrimaryLog   = FALSE;
	bSecondaryLog = FALSE;
	bArchive	  = FALSE;

#ifdef MAINWIN // FormatKey2Find() function not called here because of the difference between iigcn_key and gcn_server_class
	sprintf(kbuf, " %s ",iigcn_key); 
#else
    sprintf( kbuf, "%s\\%s", ii_install, gcn_server_class );
#endif
    /*
     ** First, skip past the master name server if it's running
     */
    if (( p = find_key( p, gcn_server_class)) !=NULL )
    {
		p2 = find_key(p, ii_install);
		p = find_key(p, kbuf);
		/* Is this a regular name server, if installation id 
		**follows and it isn't server name then it's master
		*/
		if (strcmp(p, p2) != 0)
		{
			p = Move2InstanceStringStart(p);
			MoveInstanceString(iigcn_id, p);
		}
    }
    p = readbuf;
    while (( p = find_key( p, kbuf)) != NULL )
    {
		p = Move2InstanceStringStart(p);
		MoveInstanceString(iigcn_id, p);
	iigcn_count++;
    }
	FormatKey2Find(kbuf, ii_install, iigcc_key);
    kp = iigcc_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iigcc_count++;
    }

	FormatKey2Find(kbuf, ii_install, iijdbc_key);
    kp = iijdbc_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iijdbc_count++;
    }

	FormatKey2Find(kbuf, ii_install, iidas_key);
    kp = iidas_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iidas_count++;
    }


	FormatKey2Find(kbuf, ii_install, iigcb_key);
    kp = iigcb_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iigcb_count++;
    }

    if (Is_w95)
    {
		FormatKey2Find(kbuf, ii_install, iigws_key);
    	kp = iigws_id;
    	p = readbuf;
    	while (p = find_key( p, kbuf ))
    	{
			p = Move2InstanceStringStart(p);
            while (!isspace( *p ))
            {
				*kp++ = *p;
				*p=' ';
				p++;
            }
            *kp++ = '\0';
            iigws_count++;
    	}
    }

	FormatKey2Find(kbuf, ii_install, rmcmd_key);
    kp = iirmcmd_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iirmcmd_count++;
    }

    kp = iidbms_id;
    for (i = 0; i <= num_dbms_classes; i++)
    {
	FormatKey2Find(kbuf, ii_install, iidbms_key[i]);
	p = readbuf;
	while (p = find_key( p, kbuf ))
	{
		p = Move2InstanceStringStart(p);
		kp1 = kp;
	    while (!isspace( *p ))
	    {
            *kp++ = *p;
			*p=' ';
			p++;
	    }
	    *kp++ = '\0';
		if (stristr(kp1,"IINAMU.EXE")) //under UNIX, problem of path of IINAMU appearing in the output doesn't exist: this code will be transparent
			kp = kp1;
		else
			iidbms_count++;
	}
    }
	FormatKey2Find(kbuf, ii_install, iirecovery_key);
    kp = iirecovery_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        iirecovery_count++;
    }

    kp = iistar_id;
    for (i = 0; i <= num_star_classes; i++)
    {
	FormatKey2Find(kbuf, ii_install, iistar_key[i]);
	p = readbuf;
	while (p = find_key( p, kbuf ))
	{
		p = Move2InstanceStringStart(p);
		kp1 = kp;
	    while (!isspace( *p ))
	    {
            *kp++ = *p;
			*p=' ';
			p++;
	    }
	    *kp++ = '\0';
		if (stristr(kp1,"IINAMU.EXE")) //under UNIX, problem of path of IINAMU appearing in the output doesn't exist: this code will be transparent
			kp = kp1;
		else
		    iistar_count++;
	}
    }

	FormatKey2Find(kbuf, ii_install, icesvr_key);
    kp = icesvr_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        icesvr_count++;
    }

	FormatKey2Find(kbuf, ii_install, oracle_key);
    kp = oracle_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        oracle_count++;
    }
	FormatKey2Find(kbuf, ii_install, db2udb_key);
    kp = db2udb_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        db2udb_count++;
    }
	FormatKey2Find(kbuf, ii_install, informix_key);
    kp = informix_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        informix_count++;
    }
	FormatKey2Find(kbuf, ii_install, sybase_key);
    kp = sybase_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        sybase_count++;
    }
	FormatKey2Find(kbuf, ii_install, mssql_key);
    kp = mssql_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        mssql_count++;
    }
	FormatKey2Find(kbuf, ii_install, odbc_key);
    kp = odbc_id;
    p = readbuf;
    while (p = find_key( p, kbuf ))
    {
		p = Move2InstanceStringStart(p);
        while (!isspace( *p ))
        {
            *kp++ = *p;
			*p=' ';
			p++;
        }
        *kp++ = '\0';
        odbc_count++;
    }
#ifndef MAINWIN
	// if special message not already having been displayed, look for "lost" instances 
	// (not found in the known components, nor in the special instance list)
	if (!bGetInstMsgSpecialInstanceDisplayed) {
		char buflostinstance[100];
		sprintf( kbuf, "%s\\", ii_install);
		p = readbuf;
		while (p = find_key( p, kbuf ))
		{
			kp = buflostinstance;
			while (!isspace( *p ))
			{
				*kp++ = *p;
				*p=' ';
				p++;
			}
			*kp++ = '\0';
			
			// loop on the "special instances"
			CaParseInstanceInfo * pInstance = NULL;
			BOOL bSpecialInstance = FALSE;
			POSITION  pos = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetHeadPosition();
			while (pos != NULL)
			{
				pInstance = theApp.GetSpecialInstanceInfo().GetSpecialInstancesList().GetNext(pos);
				if (!stricmp(buflostinstance,(char * ) (LPCTSTR) pInstance->m_csInstance)) {
					bSpecialInstance = TRUE;
					break;
				}
			}
			if (!bSpecialInstance) {
				CString strMsg;
				strMsg.Format(IDS_E_INSTANCE_BELONGS_NO_COMPONENT,	buflostinstance);
				LPTSTR lpszError = new TCHAR [strMsg.GetLength() + 1];
				lstrcpy (lpszError, strMsg);
				::PostMessage (theApp.m_pMainWnd->m_hWnd, WMUSRMSG_IVM_MESSAGEBOX, 0, (LPARAM)lpszError);
				bGetInstMsgSpecialInstanceDisplayed = TRUE;
			}
		}
	}
#endif
	// get list of "virtual" instances:
	{
		char buf[200], *pc, *pc1;

		//logging system

#ifdef MAINWIN
		sprintf(buf,"%s/ivmlog.inf", ii_temp);
#else
		sprintf(buf,"%s\\ivmlog.inf", ii_temp);
#endif
		FileContentBuffer FileContent(buf, 4000);
		pc = FileContent.GetBuffer();
		char * szLogNotOnline = "Logging system not ONLINE";
		char * szLogError     = "ERsend";
		char * szLogError2    = "LGinitialize failed";
		char * szActiveLogs   = "Active Log(s):";
		char * szPrimaryLog   = "II_LOG_FILE";
		char * szDualLog      = "II_DUAL_LOG";
		char * szStatus	      = "Status:";
		char * szOnline	      = "ONLINE";

		bPrimaryLog	   = FALSE;
		bSecondaryLog  = FALSE;

		if (!strnicmp(pc,szLogNotOnline,_tcslen(szLogNotOnline)) ||
			strstr(pc, szLogError) > 0 || strstr(pc, szLogError2) > 0 )
			bLoggingOn	   = FALSE;
		else {
			pc1=_tcsstr(pc, szStatus);
			if (pc1!=NULL)
			{
			     pc1+=_tcslen(szStatus);
			     while (*pc1==' ')
				pc1++;
			     if (_tcsstr(pc1, szOnline))
				bLoggingOn = TRUE;
			     pc1=_tcsstr(pc,szActiveLogs);
			     if (pc1!=NULL) {
				pc1+=_tcslen(szActiveLogs);
				while (*pc1==' ') 
					pc1++;
				if (_tcsstr(pc1,szPrimaryLog))
					bPrimaryLog = TRUE;
				if (_tcsstr(pc1,szDualLog))
					bSecondaryLog = TRUE;
				 }

			}
		}

		// archiver process

		char * szActiveProcesses = "List of active processes";
		char * szArchive         = " ARCHIV ";
		pc1 = _tcsstr(pc,szActiveProcesses);
		if (pc1) {
			if (_tcsstr(pc1,szArchive))
				bArchive	   = TRUE;
		}

		// locking system info

		bLockingOn	   = FALSE;

#ifdef MAINWIN
		sprintf(buf,"%s/ivmlock.inf", ii_temp);
#else
		sprintf(buf,"%s\\ivmlock.inf", ii_temp);
#endif
		FileContentBuffer FileContent3(buf, 2000);
		pc = FileContent3.GetBuffer();
		char * szinfo     = "Lock hash table  ";
		pc1=_tcsstr(pc,szinfo);
		if (pc1!=NULL) {
			pc1+=_tcslen(szinfo);
			while (*pc1==' ')
				pc1++;
			BOOL bDone = FALSE;
			while (!bDone) {
				char c = *pc1;
				switch (c) {
					case '0':
						break;
					case '1':case '2':case '3':case '4':case '5':
					case '6':case '7':case '8':case '9':
						bLockingOn = TRUE;
						break;
					default:
						bDone=TRUE;
						break;
				}
				pc1++;
			}
		}
	}





}


static
char *
find_key( char *buf, char *key )
{
    int     len = (int)_tcslen( key );

    while (*buf)
    {
        if (!memcmp( buf, key, len ))
            return buf;
        buf++;
    }
    return NULL;
}

static
int
get_procs( )
{
    char        cmdbuf[5000];
    FILE        *desc;
    i4     bytesread; //SIread() prototyped for a i4 (defined differently in OPING20)
    LOCATION    loc;
    int         ret;
    char	tempfilename[256];
    CWaitCursor	wait;

#ifdef MAINWIN
	wsprintf (tempfilename,"%s/%s.001",ii_temp, ingstop );
#else
	wsprintf (tempfilename,"%s\\%s.001",ii_temp, ingstop );
#endif

	wsprintf (cmdbuf,      "show servers\r\n\rquit\r\n");

	int ilen = (int)_tcslen(cmdbuf);
	if ( ! ESL_FillNewFileFromBuffer(tempfilename,cmdbuf, ilen, FALSE))
		return (2);

#ifdef MAINWIN
	sprintf( cmdbuf, "%s <%s/%s.001 >%s/%s.002",
		iinamu, ii_temp, ingstop, ii_temp, ingstop );
#else
	sprintf( cmdbuf, "%s <%s\\%s.001 >%s\\%s.002",
		iinamu, ii_temp, ingstop, ii_temp, ingstop );
#endif
    ret = Mysystem( cmdbuf );

#ifdef MAINWIN
    sprintf( cmdbuf, "%s/%s.002", ii_temp, ingstop );
#else
    sprintf( cmdbuf, "%s\\%s.002", ii_temp, ingstop );
#endif
    loc.string = cmdbuf;
    SIopen( &loc, "r", &desc );
    SIread( desc, sizeof(readbuf), &bytesread, readbuf);
    SIclose( desc );
    readbuf[bytesread] = '\0';

    ASSERT (sizeof(readbuf2) == sizeof(readbuf));
    memcpy(readbuf2,readbuf,sizeof(readbuf2));

    /* get output of "logstat -header -processes " into temporaryfile */

#ifdef MAINWIN
{
    char	buf[200], *pc;
    char	*szShmNotInstalled = "!Can't map system segment";

    /*
    ** See if the shared memory is installed.
    */
    sprintf( cmdbuf, "csreport > %s/ivmcsr.inf", ii_temp);
    Mysystem( cmdbuf );

    sprintf(buf, "%s/ivmcsr.inf", ii_temp);
    FileContentBuffer FileContent(buf, 4000);
    pc = FileContent.GetBuffer();
    if (!strnicmp(pc, szShmNotInstalled, _tcslen(szShmNotInstalled)))
    {
	/*
	** They're not online. Just touch the "logstat" and "lockstat" files.
	*/
	sprintf(cmdbuf, "echo \"Logging system not ONLINE\" > %s/ivmlog.inf",
		ii_temp);
	Mysystem(cmdbuf);
	sprintf(cmdbuf, "echo > %s/ivmlock.inf", ii_temp);
	Mysystem(cmdbuf);
	return (OK);
    }
}
#endif

#ifdef MAINWIN
    sprintf( cmdbuf, "logstat -header -processes>%s/ivmlog.inf", ii_temp);
#else
    sprintf( cmdbuf, "%s.exe -header -processes>%s\\ivmlog.inf", logstat, ii_temp);
#endif
    Mysystem( cmdbuf );
	// get output of "lockstat -summary"
#ifdef MAINWIN
    sprintf( cmdbuf, "lockstat -summary >%s/ivmlock.inf", ii_temp);
#else
    sprintf( cmdbuf, "%s.exe -summary >%s\\ivmlock.inf", lockstat, ii_temp);
#endif
    Mysystem( cmdbuf );

    return ( OK );
}





/*
** Name: get_server_classnames
**
** Description:
**      
**      Query config.dat to find the server class names for the DBMS and Star.
**      These are user configurable parameters, so we cannot depend upon the
**      default values of "STAR" and "INGRES".  
**
** Inputs:
**      none
**
** Outputs:
**      If the values are successfully
**      queried they are pointed at by the static variables iidbms_key and
**      iistar_key on return. These variables are not updated if the PMget
**      call fails.
**
** Returns:
**      void
**	
** History:
**      13-nov-96 (wilan06)
**          Created
**	25-mar-1998 (somsa01)
**	    Modified so that we can get multiple DBMS and STAR server
**	    classes.  (Bug #89972)
*/
static void get_server_classnames (void)
{
    char        buffer[256];
    char*       className = NULL;
    char*       host = PMhost();
    char	*regexp, *name;
    int		i;
    STATUS	status;
    PM_SCAN_REC	state1;
    BOOL	ClassExists;

    num_dbms_classes = -1;
    STprintf (buffer, "%s.%s.dbms.%%.server_class", SystemCfgPrefix, host);
    regexp = PMexpToRegExp (buffer);
    for (
	    status = PMscan( regexp, &state1, NULL, &name, &className );
	    status == OK;
	    status = PMscan( NULL, &state1, NULL, &name, &className ) )
    {
	ClassExists = FALSE;
	if (num_dbms_classes != -1)
	{
	    for (i = 0; i <= num_dbms_classes; i++)
		if (!STcompare(iidbms_key[i], className))
		{
		    ClassExists = TRUE;
		    break;
		}
	}
		    
	if (!ClassExists)
	    STcopy( className, iidbms_key[++num_dbms_classes] );
    }

    num_star_classes = -1;
    STprintf (buffer, "%s.%s.star.%%.server_class", SystemCfgPrefix, host);
    regexp = PMexpToRegExp (buffer);
    for (
	    status = PMscan( regexp, &state1, NULL, &name, &className );
	    status == OK;
	    status = PMscan( NULL, &state1, NULL, &name, &className ) )
    {
	ClassExists = FALSE;
	if (num_star_classes != -1)
	{
	    for (i = 0; i <= num_star_classes; i++)
		if (!STcompare(iistar_key[i], className))
		{
		    ClassExists = TRUE;
		    break;
		}
	}
		    
	if (!ClassExists)
	    STcopy( className, iistar_key[++num_star_classes] );
    }
}


/*
** Name: get_sys_dependencies -   Set up global variables
**
** Description:
**      Replace system-dependent names with the appropriate
**      values from gv.c.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Side effects:
**      initializes global variables
**
** History:
**      04-apr-1997 (canor01)
**          Created.
*/
static void 
get_sys_dependencies()
{
    OSVERSIONINFO lpvers;
    LOCATION      sysloc;
    char          *str;
 
/* this define for windows95 was not in c++ 2.0 headers */
 
# ifndef VER_PLATFORM_WIN32_WINDOWS
# define VER_PLATFORM_WIN32_WINDOWS 1
# endif /* VER_PLATFORM_WIN32_WINDOWS */
     
    lpvers.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
    GetVersionEx(&lpvers);
    Is_w95=(lpvers.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)? TRUE: FALSE;

    Is_Jasmine = (STcompare("jas", SystemExecName) == 0);

    STprintf( ingstop, "%sivmp", SystemExecName );  // changed in order to avoid conflicts with concurrent ingstopexecution
    STprintf( iinamu, "%snamu", SystemCfgPrefix );
    STprintf( iimonitor, "%smonitor", SystemCfgPrefix );
    STprintf( iigcn, "%sgcn", SystemCfgPrefix );
    STprintf( iigcc, "%sgcc", SystemCfgPrefix );
    STprintf( iigcb, "%sgcb", SystemCfgPrefix );
    STprintf( iidbms, "%sdbms", SystemCfgPrefix );
    STprintf( iigcn_key, "%sNMSVR", SystemVarPrefix );

    STprintf( gwstop, "%sgwstop", SystemCfgPrefix );

    if ( Is_Jasmine )
    {
	STcopy( "jasclean", iiclean );
	STcopy( "jasnetu", iinetu );
    }

    /* get the correct version of iiclean, logstat, lockstat with full path */
	NMloc(BIN, FILENAME, iiclean, &sysloc);
	LOtos(&sysloc, &str);
	STcopy(str, iiclean );
	
	NMloc(BIN, FILENAME, logstat, &sysloc);
	LOtos(&sysloc, &str);
	STcopy(str, logstat );
	
	NMloc(BIN, FILENAME, lockstat, &sysloc);
	LOtos(&sysloc, &str);
	STcopy(str, lockstat );
}


static BOOL IVM_FillBufferFromFile(long startpos, char * filename,char *buffer,int buflen, int * pretlen, BOOL bAppend0)
{
   int ihdl;
   int inumread;
   BOOL bResult = TRUE;

   if (bAppend0) {
      buflen--;
      if (buflen<0)
         return FALSE;
   }

#ifdef MAINWIN  // mainsoft
   ihdl  = _open (filename, _O_RDONLY | _O_BINARY  );
#else
   ihdl  = _sopen (filename, _O_RDONLY | _O_BINARY  ,_SH_DENYNO);
#endif
   if (ihdl <0)
      return FALSE;

	long lres =_lseek( ihdl, startpos, SEEK_SET );
	if (lres!=startpos) {
		_close(ihdl);
		return FALSE;
	}

   inumread =_read(ihdl , buffer, buflen);
   if (inumread <0)
      bResult = FALSE;

   *pretlen= inumread;

   	for (int i=0;i<(int)inumread;i++) { // NULL chars will be displayed as spaces. allows to handle '\0' terminated strings
		if (buffer[i]=='\0')
			buffer[i]=' ';
	}

   if (bAppend0 && bResult == TRUE)
      buffer[inumread]='\0';
   
   _close(ihdl);
 
   return bResult;
}
BOOL IVM_RemoveFile(char * filename)
{
   OFSTRUCT ofFileStruct;
   HFILE hf;
   hf = OpenFile((LPCSTR)filename, &ofFileStruct, OF_DELETE);
   if (hf==HFILE_ERROR)
      return FALSE;
   return TRUE;
}


#define MARGIN_FOR_TRUNC_MESSAGE 120
FileContentBuffer::FileContentBuffer(char * filename, long lmaxbufsize, BOOL bEndOnlyIfTooBig)
{
	int cbread;
	m_pbuffer = NULL;
	long lstartpos = 0L;
	strcpy(m_filename,filename);
	long lBytes2Read = IVM_GetFileSize(filename);
	if (lBytes2Read<0L) 
		return;

	m_pbuffer = (char *) malloc(lmaxbufsize+1);
	if (!m_pbuffer)
		return;

	char * pbuffer2Read = m_pbuffer;
	m_bEndBytesOnly = FALSE;
	m_bWasTruncated2MaxBufSize = FALSE;
	if (lBytes2Read+1>lmaxbufsize) {
		if (bEndOnlyIfTooBig) {
			m_bEndBytesOnly = TRUE;
			wsprintf(m_pbuffer,"<Begin of File is Not Available>\r\n\r\n");
			pbuffer2Read +=_tcslen(m_pbuffer);
			lstartpos = lBytes2Read - lmaxbufsize + MARGIN_FOR_TRUNC_MESSAGE;
		}
		else {
			m_bWasTruncated2MaxBufSize = TRUE;
		}
		lBytes2Read = lmaxbufsize-MARGIN_FOR_TRUNC_MESSAGE; // a warning will be added at the end if truncated
	}



	BOOL bRes =IVM_FillBufferFromFile(lstartpos, filename,pbuffer2Read,lBytes2Read+1, &cbread, TRUE);
	if (!bRes || cbread!=lBytes2Read) {
		strcpy(m_pbuffer,_T("File Read Error"));
		return;
	}
	if (m_bWasTruncated2MaxBufSize == TRUE) 
		strcat(m_pbuffer, _T("\r\n <File too big to be displayed>"));
	
}
FileContentBuffer::~FileContentBuffer()
{
	if (m_pbuffer)
		free(m_pbuffer);

}

char * FileContentBuffer::GetBuffer()
{
	if (m_pbuffer)
		return m_pbuffer;
	else
		return _T("Error");
};


#define MAXPARMS 10
#define MAXPARMSIZE 80
static BOOL bIngErrInit = FALSE;
static i4 language;
static ER_ARGUMENT		param[MAXPARMS];
static char bufparms[MAXPARMS] [MAXPARMSIZE];

BOOL GetAndFormatIngMsg(long lID, char * bufresult, int bufsize)
{
	if (lID==MSG_NOCODE) {
		strcpy(bufresult,"< message or line with no E_XXnnnn code >");
		return TRUE;
	}
	if (!bIngErrInit) {
		int i;
		if( ERlangcode( NULL, &language ) != OK )
			return FALSE;
		for (i=0;i<MAXPARMS;i++) {
			sprintf(bufparms[i],"<var>");
			param[i].er_size  = (i4) 8;
			param[i].er_value = (PTR) bufparms[i];
		}
		bIngErrInit = TRUE;
	}

    CL_ERR_DESC		clerror;
    i4			rslt_msglen=0;
    i4			buf_len	    = ER_MAX_LEN;
    i4			tmp_status;
	char buf[1200];
	if (bufsize>sizeof(buf))
		bufsize=sizeof(buf);

    tmp_status  = ERslookup(lID, NULL, ER_NOPARAM, NULL,
					buf, bufsize, language,
					&rslt_msglen, &clerror, MAXPARMS, param);
	

    if (tmp_status == OK){
		char * pc1 = buf;
		char * pc2 = bufresult;
		char c;
		while ( (c = *pc1) !='\0') {
			if (c=='%') {
				char c1=*(pc1+1);
				int ipos = c1-'0';
				if (ipos< 0 || ipos>9) {
					// regular C parameter substitution
					char * pc3 = pc1+1;
					while ((c= *(pc3)) != '\0') {
						if (  ! _tcschr("-#.0123456789hl",c)) // non trailing chars in %xyz syntax
						  break;
						pc3=_tcsinc(pc3);
					}
					if ( c=='\0' ||  ! _tcschr("cCsSduixX",c)) {
						// was a single % sign, to be copied
						c='%';
						// will continue through normal copy of character
					}
					else {
						// pc3 points on the first character after the substitution expression
						lstrcpy(pc2,"<var>");
						pc2+=5;
						pc1=pc3;
						continue;
					}
				}
				else {
					// ingres variable subsitution
					wsprintf(pc2,"<var%c>",c1);
					pc2+=6;
					pc1+=2; /* DBCS fix fixes also minor potential display error */
					continue;
				}
			}
			// normal copy of character
			if ((unsigned char)c<' ') // not in the range of DBCS trailing bytes
				*pc1=' ';
			CMcpyinc(pc1, pc2);
			if (pc2 > (bufresult+bufsize-12) )
				break;
		}
		*pc2='\0';
		return TRUE;
	}
	return FALSE;




}
