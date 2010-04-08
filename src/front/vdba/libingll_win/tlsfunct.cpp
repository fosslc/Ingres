/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : tlsfunct.cpp, implementation file 
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : General function prototypes
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**    depending on the Ingres II_CHARSETxx.
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
** 10-Feb-2004 (schph01)
**   (SIR 108139)  additional change for retrieved the year by parsing information 
**   from the gv.h file 
** 17-Feb-2004 (schph01)
**    BUG 99242,  cleanup for DBCS compliance
** 20-Oct-2004 (uk$so01)
**    BUG #113271 / ISSUE #13751480 -- The string GV_VER does not contain 
**    information about Year anymore. So hardcode Year to 2004.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2007 (drivi01)
**    Update the year to 2007.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 20-Mar-2009 (smeke01) b121832
**    Tidied up and simplified INGRESII_BuildVersionInfo(). 
*/

#include "stdafx.h"
#include "tlsfunct.h"
#include "ingobdml.h"
#include "copyright.h"

extern "C"
{
#include <compat.h>
#include <cm.h>
#include <st.h>
#include <gl.h>
#include <lc.h>
#include <er.h>
#include <nm.h>
#include <gv.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TCHAR* fstrncpy(TCHAR* pu1, TCHAR* pu2, int n)
{
	// don't use STlcopy (may split the last char)
	TCHAR* bufret = pu1;
	while ( n >= CMbytecnt(pu2) +1 ) { // copy char only if it fits including the trailing \0
		n -=CMbytecnt(pu2);
		if (*pu2 == EOS) 
			break;
		CMcpyinc(pu2,pu1);
	}
	*pu1=EOS;
	return bufret;
}


char* _fstrncpy(char* pu1, char* pu2, int n)
{
	// don't use STlcopy (may split the last char)
	char* bufret = pu1;
	while ( n >= CMbytecnt(pu2) +1 ) { // copy char only if it fits including the trailing \0
		n -=CMbytecnt(pu2);
		if (*pu2 == EOS) 
			break;
		CMcpyinc(pu2,pu1);
	}
	*pu1=EOS;
	return bufret;
}


void RemoveEndingCRLF(LPTSTR lpszText)
{
	int nLen = _tcslen (lpszText);
	if (nLen >= 1)
	{
		if (lpszText[nLen-1] == 0x0D) /* DBCS checked */
		{
			lpszText[nLen-1] = _T('\0');
		}
		else
		{
			if (lpszText[nLen-1] == 0x0A) /* DBCS checked */
				lpszText[nLen-1] = _T('\0');
			if (nLen > 1 && lpszText[nLen-2] == 0x0D) /* DBCS checked */
				lpszText[nLen-2] = _T('\0');
		}
	}
}

//
// Replace '\n' by the {0x0D, 0x0A}.
void RCTOOL_CR20x0D0x0A(CString& item)
{
#if defined (MAINWIN)
	TCHAR tchszReturn[] = {0x0A, 0x00};
#else
	TCHAR tchszReturn[] = {0x0D, 0x0A, 0x00};
#endif
	CString strCopy = item;
	item = _T("");
	int ind = -1;
	ind = strCopy.Find (_T('\n'));

	while (ind != -1)
	{
		if (strCopy.GetLength() > 1)
		{
			item += strCopy.Left (ind);
			item += tchszReturn;
			strCopy = strCopy.Mid (ind+1);
		}
		else
		{
			item += tchszReturn;
			strCopy = _T("");
		}
		ind = strCopy.Find (_T('\n'));
	}
	if (!strCopy.IsEmpty())
		item += strCopy;
}

//
// Replace the set of {'\t', '\n', 0x0A, 0x0D} by the space.
void RCTOOL_EliminateTabs (CString& item)
{
	int i, nLength = item.GetLength();
	for (i=0; i<nLength; i+= _tclen((const TCHAR *)item + i))
	{
		if (item.GetAt(i) == _T('\t') || item.GetAt(i) == _T('\n') || item.GetAt(i) == 0x0A || item.GetAt(i) == 0x0D)
			item.SetAt (i, _T(' '));
	}
}

CString UNICODE_IngresMapCodePage()
{
	CString strMapChar = _T("");
	char encoding[128];
	encoding[0] = '\0';
#if defined (EDBC)
	strcpy (encoding, "UTF-8");
#else
	CL_ERR_DESC cl_err;
	if (LC_getStdLocale (0, encoding, &cl_err) != OK )
	{
		switch (cl_err.errnum)
		{
		case E_LC_LOCALE_NOT_FOUND:
			// IIUGerr(E_XF0055_No_locale_found, UG_ERR_ERROR, 0);
			break;
		case E_LC_CHARSET_NOT_SET:
			// IIUGerr(E_XF0056_Charset_not_set, UG_ERR_ERROR, 0);
			// PCexit (FAIL);
			break;
		case E_LC_FORMAT_INCORRECT:
			// IIUGerr(E_XF0058_LC_Format_incorrect, UG_ERR_ERROR, 1 , &cl_err.moreinfo[0].data._i4);
			// PCexit (FAIL);
			break;
		default:
			// IIUGerr(E_XF0057_Unknown_LC_error, UG_ERR_ERROR, 0);
			// PCexit (FAIL);
			break;
		}
		strcpy (encoding, "UTF-8");
	}
	else
	{
		strMapChar = encoding;
	}
#endif
	return strMapChar;
}

CString INGRESII_QueryInstallationID(BOOL bFormated)
{
	USES_CONVERSION;
	char* ii_installation;
	CString strInstallationID = _T(" [<error>]");
#ifdef EDBC
	NMgtAt( ERx( "ED_INSTALLATION" ), &ii_installation );
#else
	NMgtAt( ERx( "II_INSTALLATION" ), &ii_installation );
#endif

	if (bFormated)
	{
		if( ii_installation == NULL || *ii_installation == EOS )
			strInstallationID = _T(" [<error>]");
		else
			strInstallationID.Format(_T(" [%s]"), A2T(ii_installation));
	}
	else
	{
		if( ii_installation == NULL || *ii_installation == EOS )
			strInstallationID = _T("");
		else
			strInstallationID = A2T(ii_installation);
	}
	return strInstallationID;
}

void INGRESII_BuildVersionInfo (CString& strVersion, int& nYear)
{
	nYear = COPYRIGHT_YEAR;

	// Parse "II 9.x.x (int.w32/xxx)" style string from gv.h to generate version string.
	CString strVer = (LPCTSTR) GV_VER;

	int ipos = strVer.Find(_T('('));
	if (ipos>=0)
		strVer=strVer.Left(ipos);
	strVer.TrimRight();
	while ( (ipos = strVer.Find(_T(' ')) ) >=0)
		strVer=strVer.Mid(ipos+1);
	
	strVersion = strVer;
}

BOOL INGRESII_IsIngresGateway(LPCTSTR lpszServerClass)
{
	const int nSize = 16;
	TCHAR tchNotIngresGwTab[nSize][16] = 
	{
		_T("ALB"),
		_T("DB2"),
		_T("DCOM"),
		_T("IDMS"),
		_T("IMS"),
		_T("INFORMIX"),
		_T("MSSQL"),
		_T("ODBC"),
		_T("ORACLE"),
		_T("RDB"),
		_T("RMS"),
		_T("SYBASE"),
		_T("VANT"),
		_T("DB2UDB"),
		_T("VSAM"),
		_T("ADABAS")
	};
	int i = 0;
	for (i = 0; i < nSize; i++)
	{
		if (_tcsicmp (tchNotIngresGwTab[i], lpszServerClass) == 0)
			return FALSE;
	}
	return TRUE;
}

class CaWndFromPid
{
public:
	CaWndFromPid(DWORD pid): m_dwPID(pid){}
	~CaWndFromPid(){}

	CPtrList m_listWnd;
	DWORD m_dwPID;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	static TCHAR szBuffer[120];
	CaWndFromPid* pList = (CaWndFromPid*)lParam;

	DWORD pid = 0;
	if (GetWindowThreadProcessId( hwnd, &pid )) 
	{
		if (pid == pList->m_dwPID)
		{
			::GetWindowText(hwnd, szBuffer, sizeof(szBuffer));
			if (szBuffer[0] && _tcslen(szBuffer) > 0)
				pList->m_listWnd.AddTail(hwnd);
		}
	}
	return TRUE;
}

static void GetWindowFromPID(CaWndFromPid& listWnd)
{
	static TCHAR szBuffer[120];
	EnumWindows(EnumWindowsProc, (LPARAM)&listWnd);

	POSITION p = NULL, pos = listWnd.m_listWnd.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		HWND hWnd = (HWND)listWnd.m_listWnd.GetNext(pos);
		if (!IsWindow(hWnd))
			listWnd.m_listWnd.RemoveAt(p);
		else
		{
			HWND hParent = ::GetParent(hWnd);
			//
			// Keep only the handle that don't have parent:
			if (hParent)
				listWnd.m_listWnd.RemoveAt(p);
		}
	}
}

void RCTOOL_GetWindowTitle (long PID, CStringList& listCaption)
{
	static TCHAR szBuffer[120];
	CaWndFromPid listWnd((DWORD)PID);
	GetWindowFromPID(listWnd);
	POSITION pos = listWnd.m_listWnd.GetHeadPosition();
	while (pos != NULL)
	{
		HWND hWnd = (HWND)listWnd.m_listWnd.GetNext(pos);
		if (IsWindow(hWnd))
		{
			::GetWindowText(hWnd, szBuffer, sizeof(szBuffer));
			if (szBuffer[0] && _tcslen(szBuffer) > 0 && _tcsicmp(szBuffer, _T("OleMainThreadWndName")) != 0)
				
				listCaption.AddTail(szBuffer);
		}
	}
}

void RCTOOL_GetWindowTitle (long PID, CArray <HWND, HWND>& arrayWnd)
{
	static TCHAR szBuffer[120];
	CaWndFromPid listWnd((DWORD)PID);
	GetWindowFromPID(listWnd);

	POSITION pos = listWnd.m_listWnd.GetHeadPosition();
	while (pos != NULL)
	{
		HWND hWnd = (HWND)listWnd.m_listWnd.GetNext(pos);
		if (IsWindow(hWnd))
		{
			::GetWindowText(hWnd, szBuffer, sizeof(szBuffer));
			if (szBuffer[0] && _tcslen(szBuffer) > 0 && _tcsicmp(szBuffer, _T("OleMainThreadWndName")) != 0)
				
				arrayWnd.Add(hWnd);
		}
	}
}

