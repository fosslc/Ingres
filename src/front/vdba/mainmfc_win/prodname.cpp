/**************************************************************************
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**   Source   : prodname.cpp
**
**   Project : Ingres Visual DBA
**   Author  : Francois Noirot-Nerin
**
**   generic functions for getting misc names: environment variables / products /
**   executables files / help files (principally because of the EDBC project) and
**   some  windows titles (for the "callable in context" feature)
**
**  History after 01-01-2000
**   21-Jan-2000 (noifr01)
**   (SIR 100103) re-enabled the application specific title in the case VDBA
**   is invoked in the context with one monitor window , through the (exact)
**   parameters combination used when invoked from INGNET
**  03-Feb-2000 (noifr01)
**   (SIR 100329) changed the string of the command to be executed for starting
**   winstart
**  27-Sep-2000 (noifr01)
**   (SIR 102711) customized the caption of the application when invoked 
**   "in the context", typically from ManageeIT
**  20-Dec-2000 (noifr01)
**   (SIR 103548) removed functions returning DBMS and gateways executable 
**   file names, since they are no more needed because the criteria for 
**   determining whether the DBMS or gateways are started has changed
**  22-Dec-2000 (noifr01)
**   (SIR 103548) appended the installation ID in the application title, 
**   and created a new function returning the application title without the
**   installation ID (used at some particular places)
**  06-Jan-2002) (noifr01)
**   (additional change for SIR 106290) for consistency of product names in
**   the product and the splash screen and "about" boxes, restore "Ingres"
**   at the left of "Visual DBA" in the product caption
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 22-Sep-2004 (uk$so01)
**    BUG #113105 / ISSUE 13690334
**    Use the full path help file name instead of just "vdba.chm"
***************************************************************************/

#include "stdafx.h"
#include "resmfc.h"
#include "prodname.h"
#include "resmfc.h"     // ressource symbols
#include "mainmfc.h"
#include "cmdline.h"
extern "C"
{
#include <compat.h>
#include <er.h>
#include <nm.h>

#include "main.h"       // BUFSIZE
#include "dlgres.h"
#include "dba.h"
#include "dbaginfo.h"
#include "msghandl.h"   // hdr directory 
};

///////////////////////////////////////////////////////////////////////////////////////
//

static char * szedbc_install_path_var    = "EDBC_ROOT";
static char * szingres_install_path_var = "II_SYSTEM";

char * VDBA_GetInstallPathVar()
{
#ifdef EDBC
  return szedbc_install_path_var;
#else
  return szingres_install_path_var;
#endif
}


static char * szedbc_install_name    = "EDBC";
static char * szingres_install_name = "Ingres";

char * VDBA_GetInstallName4Messages()
{
#ifdef EDBC
  return szedbc_install_name;
#else
  return szingres_install_name;
#endif
}


static char * szedbc_interm_string    = "edbc";
static char * szingres_interm_string = "ingres";

char * VDBA_GetIntermPathString()
{
#ifdef EDBC
  return szedbc_interm_string;
#else
  return szingres_interm_string;
#endif
}


static char * szedbc_gcn_exename    = "edbcgcn.exe";
static char * szingres_gcn_exename = "iigcn.exe";

char * VDBA_GetGcnExeName()
{
#ifdef EDBC
  return szedbc_gcn_exename;
#else
  return szingres_gcn_exename;
#endif
}

static char * szedbc_helpfile    = "edbcvdba.chm";
static char * szingres_helpfile  = "vdba.chm";

static char szHelpFile[512];
char * VDBA_GetHelpFileName()
{
#ifdef EDBC
  return szedbc_helpfile;
#else
	TCHAR* pEnv = _tgetenv(_T("II_SYSTEM"));
	if (pEnv)
	{
		CString strPath = pEnv;
		strPath += consttchszIngVdba;
		strPath += szingres_helpfile;
		_tcscpy (szHelpFile, strPath);
		return szHelpFile;
	}
  return szingres_helpfile;
#endif
}

#if defined (MAINWIN)
static char * szedbc_vcbf_exename    = "edbcvcbf";
static char * szingres_vcbf_exename  = "vcbf";
#else
static char * szedbc_vcbf_exename    = "edbcvcbf.exe";
static char * szingres_vcbf_exename  = "vcbf.exe";
#endif

char * VDBA_GetVCBF_ExeName()
{
#ifdef EDBC
  return szedbc_vcbf_exename;
#else
  return szingres_vcbf_exename;
#endif
}

#if defined (MAINWIN)
static char * szedbc_winstart_exename    = "edbcwstart";
static char * szingres_winstart_exename  = "winstart";
#else
static char * szedbc_winstart_exename    = "edbcwstart.exe";
static char * szingres_winstart_exename  = "winstart.exe";
#endif

char * VDBA_GetWinstart_CmdName()
{
#ifdef EDBC
  return szedbc_winstart_exename;
#else
  return szingres_winstart_exename;
#endif
}

void VDBA_GetBaseProductCaption(char* buf, int bufsize)
{
	CString csVdbaCaption;
	csVdbaCaption.LoadString(AFX_IDS_APP_TITLE);
	fstrncpy((LPUCHAR) buf, (LPUCHAR)(LPCTSTR)csVdbaCaption, bufsize);
}

static char bufInstallID[100];

BOOL InitInstallIDInformation()
{
	char *ii_installation;
#ifdef EDBC
	NMgtAt( ERx( "ED_INSTALLATION" ), &ii_installation );
#else
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
#endif
	if( ii_installation == NULL || *ii_installation == EOS ){
		_tcscpy(bufInstallID, _T("<error>"));
		return FALSE;
	}
	_tcscpy(bufInstallID, ii_installation);
	return TRUE;
}
char *GetInstallationID() { return bufInstallID; }

void VDBA_GetProductCaption(char* buf, int bufsize)
{
	CString csVdbaCaption;
	UINT nIDSCaption = IDS_PRODNAME_CAPTION;
#if defined (EDBC)
	nIDSCaption = IDS_PRODNAME_CAPTION_EDBC;
#endif

	if (IsInvokedInContextWithOneWinExactly()) {
		char bufNode[100];
		char bufGW[100];
		BOOL bNode = GetContextDrivenNodeName(bufNode);
		BOOL bGW   = GetContextDrivenServerClassName(bufGW);
		BOOL bFound = FALSE;
		int i = OneWinExactlyGetWinType();
		switch (i) {
			case TYPEDOC_DOM:
				{
					CuWindowDesc* pDesc = GetWinDescPtr();
					if (pDesc) {
						if (pDesc->HasObjDesc()) {
							csVdbaCaption.LoadString(IDS_OBJ_PROPS);
							break;
						}
					}
				}
				csVdbaCaption.LoadString(IDS_INCONTEXT_DOMTITLE);
				break;
			case TYPEDOC_SQLACT:
				csVdbaCaption.LoadString(IDS_INCONTEXT_SQL_TITLE);
				break;
			case TYPEDOC_MONITOR:
				csVdbaCaption.LoadString(IDS_INCONTEXT_MONITORTITLE);
				break;
			case TYPEDOC_DBEVENT:
				csVdbaCaption.LoadString(IDS_INCONTEXT_DBEVENT_TITLE);
				break;
			default:
				csVdbaCaption.LoadString(nIDSCaption);
				break;
		}
	}
	else
		csVdbaCaption.LoadString(nIDSCaption);

	x_strcpy(buf, (LPCSTR)(LPCTSTR)csVdbaCaption);
	x_strcat(buf, " [");
	x_strcat(buf, GetInstallationID());
	x_strcat(buf,"]");
}

BOOL VDBA_InvokedInContextOneDocExactlyGetDocName(char* buf, int bufsize)
{
	if (!IsInvokedInContextWithOneWinExactly())
	return FALSE;
	
	char bufNode[100];
	char bufGW[100];
	BOOL bNode = GetContextDrivenNodeName(bufNode);
	BOOL bGW   = GetContextDrivenServerClassName(bufGW);

	if (!bNode)
		return FALSE;

	lstrcpy(buf, bufNode);
	CuWindowDesc* pDesc = GetWinDescPtr();
    if (!pDesc)
       return FALSE;
    if (pDesc->HasObjDesc()) {
		CString csObject = pDesc->GetObjectIdentifier();
		CString csObjType;
		CString csDatabase =_T("");
		int ilevel = 0;
		switch (pDesc->GetObjType()) {
			case  CMD_DATABASE   : csObjType.LoadString(IDS_DATABASE) ;ilevel=0;break;
			case  CMD_TABLE      : csObjType.LoadString(IDS_TABLE)    ;ilevel=1;break;
			case  CMD_INDEX      : csObjType.LoadString(IDS_INDEX)    ;ilevel=2;break;
//			case  CMD_RULE       : csObjType.LoadString(IDS_RULE)     ;ilevel=2;break;
//			case  CMD_INTEGRITY  : csObjType.LoadString(IDS_INTEGRITY);ilevel=2;break;
			case  CMD_VIEW       : csObjType.LoadString(IDS_VIEW)     ;ilevel=1;break;
			case  CMD_PROCEDURE  : csObjType.LoadString(IDS_PROCEDURE);ilevel=1;break;
			case  CMD_SYNONYM    : csObjType.LoadString(IDS_SYNONYM)  ;ilevel=1;break;
			case  CMD_USER       : csObjType.LoadString(IDS_USER)      ;break;
			case  CMD_GROUP      : csObjType.LoadString(IDS_GROUP)     ;break;
			case  CMD_ROLE       : csObjType.LoadString(IDS_ROLE)      ;break;
			case  CMD_PROFILE    : csObjType.LoadString(IDS_I_PROFILE) ;break;
			case  CMD_LOCATION   : csObjType.LoadString(IDS_LOCATION)  ;break;
			case  CMD_STATIC_INSTALL_SECURITY:csObjType.LoadString(IDS_DOMPROP_INSTALL_ALARMS_TITLE);break;
			case  CMD_STATIC_INSTALL_GRANTEES:csObjType.LoadString(IDS_DOMPROP_INSTALL_GRANTEES_TITLE);break;

			// IPM object types
			case CMD_MON_SERVER:        csObjType.LoadString(IDS_PROP_SERVER);   break;
			case CMD_MON_SESSION:       csObjType.LoadString(IDS_PROP_SESSION);ilevel=1;break;
			case CMD_STATIC_MON_LOGINFO:csObjType.LoadString(IDS_TAB_LOGINFO_TITLE);break;
			case CMD_MON_REPLICSERVER:  csObjType.LoadString(IDS_PROP_REPLICSERVER);break;
			default:csObjType="";
		}
		if (ilevel>0) {
			int isep = csObject.Find('/');
			if (isep>=0) {
				x_strcat(buf, "::");
				x_strcat(buf, (char *)(LPCTSTR)(csObject.Left(isep)));
				csObject=csObject.Mid(isep+1);
			}
		}
		if (bGW) {
			x_strcat(buf," /");
			x_strcat(buf, bufGW);
		}
		x_strcat(buf," ");
		x_strcat(buf,(char *)(LPCTSTR)(csObjType));
		x_strcat(buf," ");
		x_strcat(buf,(char *)(LPCTSTR)(csObject));

		return TRUE;
	}

	if (bGW) {
		x_strcat(buf," /");
		x_strcat(buf, bufGW);
	}
	return TRUE;
}



void VDBA_GetErrorCaption(char* buf, int bufsize)
{
  ASSERT (buf);
  ASSERT (bufsize >= BUFSIZE);

  CString csVdbaCaption;
  csVdbaCaption.Format(IDS_ERR_TITLE, (LPCTSTR)VDBA_GetInstallName4Messages());
  x_strcpy(buf, (LPCSTR)(LPCTSTR)csVdbaCaption);
}

UINT VDBA_GetSplashScreen_Bitmap_id()
{
#ifdef EDBC
  return IDB_SPLASH; // finally was #ifdef'ed in the resource
#else
  return IDB_SPLASH;
#endif
}

static char  buferriisystem[200];

char * VDBA_ErrMsgIISystemNotSet()
{
	wsprintf(buferriisystem,
			VDBA_MfcResourceString(IDS_E_II_SYSYTEM_SET),
			VDBA_GetInstallPathVar());
	return buferriisystem;
}
