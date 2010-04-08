/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : dtagsett.cpp, Implementation File.
**    Project  : Ingres II / Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Data defintion for General Display Settings.
**
** History:
**
** xx-Nov-1998 (uk$so01)
**    Created
** 14-jan-2000 (uk$so01)
**    (bug #100013)
**    Change SYSTEM_FONT to DEFAULT_GUI_FONT
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 18-Apr-2000 (uk$so01)
**    (Bug #100013)
**    Default font for SQL Test under WIN 2000.
** 26-Apr-2000 (uk$so01)
**    SIR #101328
**    Set the autocomit option to FALSE in the SQL Test Preference Box.
**  28-May-2001(schph01)
**    BUG 104636 update method load(),save(), copy() and LoadDefault() to
**    manage new variables member for display float column type, defined in
**    "SQL Test Preferences"
** 05-Jul-2001 (uk$so01)
**    Integrate & Generate XML (Setting the XML Preview Mode).
** 19-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**    24-Jun-2002 (hanje04)
**      Cast all CStrings being passed to other functions or methods using %s
**      with (LPCTSTR) and made calls more readable on UNIX.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dtagsett.h"
#include "maindoc.h"
#include "mainfrm.h"

extern "C"
{
#include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define GENERAL_DISPLAY_NEWSECTION    _T("GENERAL DISPLAY SETTING")
#define GENERAL_DISPLAY_GRIGLISTCTRL  _T("GENERAL DISPLAY.GridListCtrl")
#define GENERAL_DISPLAY_MAXIMIZEDOCS  _T("GENERAL DISPLAY.Maximize Docs")
#define GENERAL_DISPLAY_DELAYDLGWAIT  _T("GENERAL DISPLAY.DlgWaitDelayExecution")

#define GENERAL_DISPLAY_LISTVIEW      _T("GENERAL DISPLAY.List View Mode")
#define GENERAL_DISPLAY_LISTCOLSORT   _T("GENERAL DISPLAY.List Column Sort")
#define GENERAL_DISPLAY_SORTMODE      _T("GENERAL DISPLAY.List Sort Mode")


IMPLEMENT_SERIAL (CaGeneralDisplaySetting, CObject, 1)
CaGeneralDisplaySetting::CaGeneralDisplaySetting()
{
	LoadDefault();
}

CaGeneralDisplaySetting::CaGeneralDisplaySetting(const CaGeneralDisplaySetting& c)
{
	Copy (c);
}

void CaGeneralDisplaySetting::Copy(const CaGeneralDisplaySetting& c)
{
	m_nDlgWaitDelayExecution = c.m_nDlgWaitDelayExecution;
	m_bListCtrlGrid = c.m_bListCtrlGrid;
	m_bMaximizeDocs = c.m_bMaximizeDocs;
	m_nGeneralDisplayView = c.m_nGeneralDisplayView;
	m_nSortColumn = c.m_nSortColumn;
	m_bAsc = c.m_bAsc;
}

CaGeneralDisplaySetting CaGeneralDisplaySetting::operator = (const CaGeneralDisplaySetting& c)
{
	Copy (c);
	return *this;
}

void CaGeneralDisplaySetting::Serialize (CArchive& ar)
{
	//
	// Only save/load of .ini file.
	ASSERT (FALSE);
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

BOOL CaGeneralDisplaySetting::Save()
{
	CString strAch;
	CString strFile;
	if (strFile.LoadString (IDS_INIFILENAME) == 0)
		return FALSE;
	//
	// Row Grid:
	strAch.Format (_T("%d"), m_bListCtrlGrid);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_GRIGLISTCTRL, strAch, strFile);
	//
	// Maximize documents:
	strAch.Format (_T("%d"), m_bMaximizeDocs);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_MAXIMIZEDOCS, strAch, strFile);
	//
	// Delay of DlgWait:
	strAch.Format (_T("%ld"), m_nDlgWaitDelayExecution);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_DELAYDLGWAIT, strAch, strFile);
	//
	// List View Mode:
	strAch.Format (_T("%d"), m_nGeneralDisplayView);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_LISTVIEW, strAch, strFile);
	//
	// List Column Sort:
	strAch.Format (_T("%d"), m_nSortColumn);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_LISTCOLSORT, strAch, strFile);
	//
	// List Sort Mode:
	strAch.Format (_T("%d"), m_bAsc);
	WritePrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_SORTMODE, strAch, strFile);

	return TRUE;
}

BOOL CaGeneralDisplaySetting::Load()
{
	TCHAR   tchszAch [32];
	TCHAR   tchszDef [32];
	CString strFile;
	if (strFile.LoadString (IDS_INIFILENAME) == 0)
		return FALSE;
	//
	// Row Grid:
	wsprintf (tchszDef, "%d", TRUE);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_GRIGLISTCTRL, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_bListCtrlGrid = (BOOL)atoi (tchszAch);
	else
		m_bListCtrlGrid = TRUE;
	//
	// Maximize documents:
	wsprintf (tchszDef, "%d", TRUE);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_MAXIMIZEDOCS, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_bMaximizeDocs = (BOOL)atoi (tchszAch);
	else
		m_bMaximizeDocs = FALSE;  // same as in LoadDefault()
	//
	// Delay of DlgWait:
	wsprintf (tchszDef, "%d", 2);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_DELAYDLGWAIT, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_nDlgWaitDelayExecution = (long)atol (tchszAch);
	else
		m_nDlgWaitDelayExecution = 2;

	//
	// List View Mode:
	wsprintf (tchszDef, "%d", 0);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_LISTVIEW, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_nGeneralDisplayView = (int)atoi (tchszAch);
	else
		m_nGeneralDisplayView = 0;
	//
	// List Column Sort:
	wsprintf (tchszDef, "%d", 0);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_LISTCOLSORT, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_nSortColumn = (int)atoi (tchszAch);
	else
		m_nSortColumn = 0;
	//
	// List Sort Mode:
	wsprintf (tchszDef, "%d", TRUE);
	if (GetPrivateProfileString(GENERAL_DISPLAY_NEWSECTION, GENERAL_DISPLAY_SORTMODE, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		m_bAsc = (BOOL)atoi (tchszAch);
	else
		m_bAsc = TRUE;

	return TRUE;
}


BOOL CaGeneralDisplaySetting::LoadDefault()
{
	m_nDlgWaitDelayExecution = 2;
	m_bListCtrlGrid = TRUE;
	m_bMaximizeDocs = FALSE;
	m_nGeneralDisplayView = 0; // Big Icon
	m_nSortColumn = 0;
	m_bAsc = TRUE;
	return TRUE;
}

void CaGeneralDisplaySetting::NotifyChanges()
{
	CDocument* pDoc;
	CMainMfcApp* appPtr = (CMainMfcApp*)AfxGetApp ();
	POSITION pos, curTemplatePos = appPtr->GetFirstDocTemplatePosition();
	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate = appPtr->GetNextDocTemplate (curTemplatePos);
		pos = curTemplate->GetFirstDocPosition ();
		while (pos)
		{
			// When this setting is applied, 
			// actually, we do not differenciate between the changes are from SQLACT settings
			// or the changes are from the General Display settings.
			// But we take in to acount of SQLACT settings only for the DOM::Rows and DOM::SecurityAuditing.
			pDoc = curTemplate->GetNextDoc (pos);
			//
			// Dom Window Document:
			if(pDoc->IsKindOf (RUNTIME_CLASS (CMainMfcDoc)))
			{
				CMainMfcDoc* pDomDoc = (CMainMfcDoc*)pDoc;
				FilterCause because = FILTER_SETTING_CHANGE;
				pDomDoc->UpdateAllViews (NULL, (LPARAM)(int)because);
			}
		}
	}

	// No immediate management of "maximize doc" anymore
}


