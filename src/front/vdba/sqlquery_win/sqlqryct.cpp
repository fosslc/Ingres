/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlqryct.cpp : Implementation of the CSqlqueryCtrl ActiveX Control class.
**    Project  : sqlquery.ocx
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Control Class for drawing the ocx control
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**    depending on the Ingres II_CHARSETxx.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 17-Oct-2003 (uk$so01)
**    SIR #106648, Additional change to change #464605 (role/password)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 01-Sept-2004 (schph01)
**    SIR #106648 Add method SetErrorFileName() for managing the sql
**    errors
** 09-Sept-2004 (schph01)
**    BUG #112993 Add the try catch statements implement exception handling
**    in IsCommitEnable() method.
** 15-Sep-2004 (uk$so01)
**    VDBA BUG #113047,  Vdba load/save does not function correctly with the
**    Right Pane of DOM/Table/Rows page.
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527,
**    Add method: short GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
**    that return 1 if there is an SQL Session.
** 18-Nov-2004 (uk$so01)
**    BUG #113491 / ISSUE #13755178: Manage to Release & disconnect the DBMS session.
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
**    Product year is returned by INGRESII_BuildVersionInfo() so does
**    not need to be set in here.
** 23-Jun-2010 (maspa05) b123847
**    In CSqlqueryCtrl::GetConnected amend the check for lpszDatabase == NULL
**    to check for an empty string instead. In terms of the ocx control the 
**    parameter is VTS_BSTR where NULL gets treated as an empty string.
**/


#include "stdafx.h"
#include "sqlquery.h"
#include "rcdepend.h"
#include "sqlqrypg.h" // Property Page (SQL Session)
#include "sqlpgtab.h" // Property Page (Tab Layout)
#include "sqlpgdis.h" // Property Page (Display)
#include "qpageres.h"
#include "tlsfunct.h"
#include "ingobdml.h"
#include "a2stream.h"
#include ".\sqlqryct.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSqlqueryCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map
BEGIN_MESSAGE_MAP(CSqlqueryCtrl, COleControl)
	//{{AFX_MSG_MAP(CSqlqueryCtrl)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSqlqueryCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSqlqueryCtrl)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "ShowGrid", m_showGrid, OnShowGridChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "AutoCommit", m_autoCommit, OnAutoCommitChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "ReadLock", m_readLock, OnReadLockChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "TimeOut", m_timeOut, OnTimeOutChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "SelectLimit", m_selectLimit, OnSelectLimitChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "SelectMode", m_selectMode, OnSelectModeChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "MaxTab", m_maxTab, OnMaxTabChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "MaxTraceSize", m_maxTraceSize, OnMaxTraceSizeChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "ShowNonSelectTab", m_showNonSelectTab, OnShowNonSelectTabChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "TraceTabActivated", m_traceTabActivated, OnTraceTabActivatedChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "TraceTabToTop", m_traceTabToTop, OnTraceTabToTopChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F4Width", m_f4Width, OnF4WidthChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F4Scale", m_f4Scale, OnF4ScaleChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F4Exponential", m_f4Exponential, OnF4ExponentialChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F8Width", m_f8Width, OnF8WidthChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F8Scale", m_f8Scale, OnF8ScaleChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "F8Exponential", m_f8Exponential, OnF8ExponentialChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "QepDisplayMode", m_qepDisplayMode, OnQepDisplayModeChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CSqlqueryCtrl, "XmlDisplayMode", m_xmlDisplayMode, OnXmlDisplayModeChanged, VT_I4)
	DISP_FUNCTION(CSqlqueryCtrl, "Initiate", Initiate, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "SetDatabase", SetDatabase, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "Clear", Clear, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Open", Open, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Save", Save, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "SqlAssistant", SqlAssistant, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Run", Run, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Qep", Qep, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Xml", Xml, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsRunEnable", IsRunEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsQepEnable", IsQepEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsXmlEnable", IsXmlEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Print", Print, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsPrintEnable", IsPrintEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "EnableTrace", EnableTrace, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsClearEnable", IsClearEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsTraceEnable", IsTraceEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsSaveAsEnable", IsSaveAsEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsOpenEnable", IsOpenEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "PrintPreview", PrintPreview, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "IsPrintPreviewEnable", IsPrintPreviewEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Storing", Storing, VT_ERROR, VTS_PUNKNOWN)
	DISP_FUNCTION(CSqlqueryCtrl, "Loading", Loading, VT_ERROR, VTS_UNKNOWN)
	DISP_FUNCTION(CSqlqueryCtrl, "IsUsedTracePage", IsUsedTracePage, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "SetIniFleName", SetIniFleName, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "SetSessionDescription", SetSessionDescription, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "SetSessionStart", SetSessionStart, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CSqlqueryCtrl, "InvalidateCursor", InvalidateCursor, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "Commit", Commit, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CSqlqueryCtrl, "IsCommitEnable", IsCommitEnable, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "CreateSelectResultPage", CreateSelectResultPage, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "SetDatabaseStar", SetDatabaseStar, VT_EMPTY, VTS_BSTR VTS_I4)
	DISP_FUNCTION(CSqlqueryCtrl, "CreateSelectResultPage4Star", CreateSelectResultPage4Star, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4 VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "Initiate2", Initiate2, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CSqlqueryCtrl, "SetConnectParamInfo", SetConnectParamInfo, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CSqlqueryCtrl, "GetConnectParamInfo", GetConnectParamInfo, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "ConnectIfNeeded", ConnectIfNeeded, VT_I4, VTS_I2)
	DISP_FUNCTION(CSqlqueryCtrl, "GetSessionAutocommitState", GetSessionAutocommitState, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "GetSessionCommitState", GetSessionCommitState, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "GetSessionReadLockState", GetSessionReadLockState, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CSqlqueryCtrl, "SetHelpFile", SetHelpFile, VT_EMPTY, VTS_BSTR)
	DISP_STOCKPROP_FONT()
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSqlqueryCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CSqlqueryCtrl, "SetErrorFileName", dispidSetErrorFileName, SetErrorFileName, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CSqlqueryCtrl, "GetConnected", dispidGetConnected, GetConnected, VT_I2, VTS_BSTR VTS_BSTR)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSqlqueryCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSqlqueryCtrl)
	EVENT_CUSTOM("PropertyChange", FirePropertyChange, VTS_NONE)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSqlqueryCtrl, 4)
	PROPPAGEID(CSqlqueryPropPage::guid)
	PROPPAGEID(CSqlqueryPropPageTabLayout::guid)
	PROPPAGEID(CSqlqueryPropPageDisplay::guid)
	PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CSqlqueryCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

#if defined (EDBC)
// {7A9941A3-9987-4914-AB55-F7F630CF8B38}
IMPLEMENT_OLECREATE_EX(CSqlqueryCtrl, "EDBQUERY.SqlqueryCtrl.1",
	0x7a9941a3, 0x9987, 0x4914, 0xab, 0x55, 0xf7, 0xf6, 0x30, 0xcf, 0x8b, 0x38)
#else
// {634C383D-A069-11D5-8769-00C04F1F754A}
IMPLEMENT_OLECREATE_EX(CSqlqueryCtrl, "SQLQUERY.SqlqueryCtrl.1",
	0x634c383d, 0xa069, 0x11d5, 0x87, 0x69, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)
#endif

/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSqlqueryCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs
#if defined (EDBC)
// {C026183C-1383-40eb-8A70-DD05B3073091}
const IID BASED_CODE IID_DSqlquery =
	{ 0xc026183c, 0x1383, 0x40eb, { 0x8a, 0x70, 0xdd, 0x05, 0xb3, 0x07, 0x30, 0x91 } };
// {850E614C-A94B-4764-83DA-940E522E11DC}
const IID BASED_CODE IID_DSqlqueryEvents =
	{ 0x850e614c, 0xa94b, 0x4764, { 0x83, 0xda, 0x94, 0x0e, 0x52, 0x2e, 0x11, 0xdc } };
#else
// {634C383B-A069-11D5-8769-00C04F1F754A}
const IID BASED_CODE IID_DSqlquery =
	{ 0x634c383b, 0xa069, 0x11d5, { 0x87, 0x69, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
// {634C383C-A069-11D5-8769-00C04F1F754A}
const IID BASED_CODE IID_DSqlqueryEvents =
	{ 0x634c383c, 0xa069, 0x11d5, { 0x87, 0x69, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
#endif

/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSqlqueryOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSqlqueryCtrl, IDS_xSQLQUERY, _dwSqlqueryOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::CSqlqueryCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSqlqueryCtrl

BOOL CSqlqueryCtrl::CSqlqueryCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_xSQLQUERY,
			IDB_SQLQUERY,
			afxRegApartmentThreading,
			_dwSqlqueryOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::CSqlqueryCtrl - Constructor

CSqlqueryCtrl::CSqlqueryCtrl()
{
	m_pSqlQueryFrame = NULL;
	m_pSelectResultPage = NULL;
	InitializeIIDs(&IID_DSqlquery, &IID_DSqlqueryEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::~CSqlqueryCtrl - Destructor

CSqlqueryCtrl::~CSqlqueryCtrl()
{
	// TODO: Cleanup your control's instance data here.
}

/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::OnDraw - Drawing function

void CSqlqueryCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->MoveWindow(rcBounds);
	}
	else
	if (m_pSelectResultPage)
	{
		m_pSelectResultPage->MoveWindow(rcBounds);
	}
}



/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::DoPropExchange - Persistence support

void CSqlqueryCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	//
	// SQL Session
	PX_Bool (pPX, _T("AutoCommit"),       m_autoCommit,        DEFAULT_AUTOCOMMIT);
	PX_Bool (pPX, _T("ReadLock"),         m_readLock,          DEFAULT_READLOCK);
	PX_Long (pPX, _T("TimeOut"),          m_timeOut,           DEFAULT_TIMEOUT);
	PX_Long (pPX, _T("SelectMode"),       m_selectMode,        DEFAULT_SELECTMODE);
	PX_Long (pPX, _T("SelectLimit"),      m_selectLimit,       DEFAULT_SELECTBLOCK);
	//
	// Tab Layout
	PX_Long (pPX, _T("MaxTab"),           m_maxTab,            DEFAULT_MAXTAB);
	PX_Long (pPX, _T("MaxTraceSize"),     m_maxTraceSize,      DEFAULT_MAXTRACE);
	PX_Bool (pPX, _T("ShowNonSelectTab"), m_showNonSelectTab,  DEFAULT_SHOWNONSELECTTAB);
	PX_Bool (pPX, _T("TraceTabActivated"),m_traceTabActivated, DEFAULT_TRACEACTIVATED);
	PX_Bool (pPX, _T("TraceTabToTop"),    m_traceTabToTop,     DEFAULT_TRACETOTOP);
	//
	// Display
	PX_Bool (pPX, _T("ShowGrid"),         m_showGrid,          DEFAULT_GRID);
	PX_Long (pPX, _T("F4Width"),          m_f4Width,           DEFAULT_F4WIDTH);
	PX_Long (pPX, _T("F4Scale"),          m_f4Scale,           DEFAULT_F4DECIMAL);
	PX_Bool (pPX, _T("F4Exponential"),    m_f4Exponential,     DEFAULT_F4EXPONENTIAL);
	PX_Long (pPX, _T("F8Width"),          m_f8Width,           DEFAULT_F8WIDTH);
	PX_Long (pPX, _T("F8Scale"),          m_f8Scale,           DEFAULT_F8DECIMAL);
	PX_Bool (pPX, _T("F8Exponential"),    m_f8Exponential,     DEFAULT_F8EXPONENTIAL);
	PX_Long (pPX, _T("QepDisplayMode"),   m_qepDisplayMode,    DEFAULT_QEPDISPLAYMODE);
	PX_Long (pPX, _T("XmlDisplayMode"),   m_xmlDisplayMode,    DEFAULT_XMLDISPLAYMODE);

	// TODO: Call PX_ functions for each persistent custom property.

	if (pPX->IsLoading())
	{
		UINT nMask = SQLMASK_READLOCK|SQLMASK_TIMEOUT|SQLMASK_FETCHBLOCK;
		nMask |= SQLMASK_TRACETAB|SQLMASK_MAXTAB|SQLMASK_MAXTRACESIZE|SQLMASK_SHOWNONSELECTTAB;
		nMask |= SQLMASK_FLOAT4|SQLMASK_FLOAT8|SQLMASK_QEPPREVIEW|SQLMASK_XMLPREVIEW|SQLMASK_SHOWGRID;
		nMask |= SQLMASK_LOAD;
		NotifySettingChange(nMask);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::OnResetState - Reset control to default state

void CSqlqueryCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl::AboutBox - Display an "About" box to the user

void CSqlqueryCtrl::AboutBox()
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif

	HINSTANCE hLib = LoadLibrary(strDllName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)(LPCTSTR, LPCTSTR, short, UINT);
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "AboutBox");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			CString strTitle;
			CString strAbout;
			int year;
			CString strVer;
			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;
			INGRESII_BuildVersionInfo (strVer, year);

			strAbout.Format (IDS_PRODUCT_VERSION, strVer);
			strTitle.LoadString (AFX_IDS_APP_TITLE);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, strDllName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryCtrl message handlers

BOOL CSqlqueryCtrl::Initiate(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser)
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->Initiate(lpszNode, lpszServer, lpszUser);
	}
	return TRUE;
}

void CSqlqueryCtrl::SetDatabase(LPCTSTR lpszDatabase)
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->SetDatabase(lpszDatabase);
	}
}

void CSqlqueryCtrl::Clear()
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->Clear();
	}
}

void CSqlqueryCtrl::Open()
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->Open();
	}

}

void CSqlqueryCtrl::Save()
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->Saveas();
	}
}

void CSqlqueryCtrl::SqlAssistant()
{
	try
	{
		if (m_pSqlQueryFrame)
		{
			m_pSqlQueryFrame->SqlWizard();
		}
	}
	catch (long lError)
	{
		// Nothing to do:
		// Normally, the container should call ConnectIfNeeded() handles the error befor calling SqlAssistant()
		lError = 0;
	}
	catch (...)
	{
	}
}



void CSqlqueryCtrl::Run()
{
	try
	{
		if (m_pSqlQueryFrame && m_pSqlQueryFrame->IsRunEnable())
		{
			m_pSqlQueryFrame->Execute (CfSqlQueryFrame::RUN);
		}
	}
	catch (long lError)
	{
		// Nothing to do:
		// Normally, the container should call ConnectIfNeeded() handles the error befor calling Run()
		lError = 0;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
	}
}

void CSqlqueryCtrl::Qep()
{
	try
	{
		if (m_pSqlQueryFrame && m_pSqlQueryFrame->IsQepEnable())
		{
			m_pSqlQueryFrame->Execute (CfSqlQueryFrame::QEP);
		}
	}
	catch (long lError)
	{
		// Nothing to do:
		// Normally, the container should call ConnectIfNeeded() handles the error befor calling Qep()
		lError = 0;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
	}
}

void CSqlqueryCtrl::Xml()
{
	try
	{
		if (m_pSqlQueryFrame && m_pSqlQueryFrame->IsXmlEnable())
		{
			CString strEncoding = UNICODE_IngresMapCodePage();
			if (strEncoding.IsEmpty())
			{
				strEncoding = _T("UTF-8");
				AfxMessageBox (IDS_MSG_IICHAREST_ERROR);
			}
			theApp.SetCharacterEncoding(strEncoding);
			m_pSqlQueryFrame->Execute (CfSqlQueryFrame::XML);
		}
	}
	catch (long lError)
	{
		// Nothing to do:
		// Normally, the container should call ConnectIfNeeded() handles the error befor calling Qep()
		lError = 0;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
	}
}

BOOL CSqlqueryCtrl::IsRunEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsRunEnable();
}

BOOL CSqlqueryCtrl::IsQepEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsQepEnable();
}

BOOL CSqlqueryCtrl::IsXmlEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsXmlEnable();
}

void CSqlqueryCtrl::Print()
{
	if (!m_pSqlQueryFrame)
		return;
	m_pSqlQueryFrame->DoPrint();
}

BOOL CSqlqueryCtrl::IsPrintEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsPrintEnable();
}

void CSqlqueryCtrl::EnableTrace()
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->TracePage();
	}
}

int CSqlqueryCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!m_pSqlQueryFrame)
	{
		CWnd* pParent = this;
		CRect r;
		GetWindowRect (r);
		CdSqlQueryRichEditDoc* pDoc = new CdSqlQueryRichEditDoc();
		//
		// Initialize the properties:
		if (pDoc)
		{
			CaSqlQueryProperty& property = pDoc->GetProperty();
			ConstructPropertySet (property);
			CTypedPtrList < CObList, CaDocTracker* >& ldoc = theApp.GetDocTracker();
			ldoc.AddTail(new CaDocTracker(pDoc, (LPARAM)this));
		}

		m_pSqlQueryFrame = new CfSqlQueryFrame(pDoc);
		m_pSqlQueryFrame->Create (
			NULL,
			NULL,
			WS_CHILD,
			r,
			pParent);
		m_pSqlQueryFrame->InitialUpdateFrame(NULL, TRUE);
		m_pSqlQueryFrame->ShowWindow(SW_SHOW);
	}
	return 0;
}

BOOL CSqlqueryCtrl::IsClearEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsClearEnable();
}

BOOL CSqlqueryCtrl::IsTraceEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsTraceEnable();
}

BOOL CSqlqueryCtrl::IsSaveAsEnable()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsSaveAsEnable();
}


BOOL CSqlqueryCtrl::IsOpenEnable()
{
	return TRUE;
}

void CSqlqueryCtrl::PrintPreview()
{
	if (!m_pSqlQueryFrame)
		return;
	m_pSqlQueryFrame->DoPrintPreview();
}

BOOL CSqlqueryCtrl::IsPrintPreviewEnable()
{
	// TODO: Add your dispatch handler code here

	return TRUE;
}

BOOL CSqlqueryCtrl::IsUsedTracePage()
{
	if (!m_pSqlQueryFrame)
		return FALSE;
	return m_pSqlQueryFrame->IsUsedTracePage();
}


SCODE CSqlqueryCtrl::Storing(LPUNKNOWN FAR* ppStream)
{
	IStream* pSream = NULL;
	if (m_pSqlQueryFrame)
		m_pSqlQueryFrame->GetData(&pSream);
	else
	if (m_pSelectResultPage)
	{
		CaQuerySelectPageData* pData = (CaQuerySelectPageData*)(LRESULT)m_pSelectResultPage->SendMessage (WM_SQL_GETPAGE_DATA, 0, 0);
		if (pData)
		{
			BOOL bOk = CObjectToIStream (pData, &pSream);
			delete pData;
			if (!bOk)
			{
				pSream = NULL;
				return S_FALSE;
			}
		}
	}
	else
		return S_FALSE;
	*ppStream = (LPUNKNOWN)pSream;
	return S_OK;
}

SCODE CSqlqueryCtrl::Loading(LPUNKNOWN pStream)
{
	IStream* pSream = (IStream*)pStream;
	if (!pSream)
		return S_FALSE;
	if (m_pSqlQueryFrame)
		m_pSqlQueryFrame->SetData(pSream);
	else
	if (m_pSelectResultPage)
	{
		CaQuerySelectPageData* pData = new CaQuerySelectPageData();
		BOOL bOk = CObjectFromIStream (pData, pSream);
		if (bOk)
		{
			m_pSelectResultPage->NotifyLoad(pData);
		}

		delete pData;
	}
	else
		return S_FALSE;
	return S_OK;
}


void CSqlqueryCtrl::SetIniFleName(LPCTSTR lpszFileIni)
{
	if (!m_pSqlQueryFrame)
		return;
	m_pSqlQueryFrame->SetIniFleName(lpszFileIni);
}

void CSqlqueryCtrl::SetSessionDescription(LPCTSTR lpszSessionDescription)
{
	theApp.GetSessionManager().SetDescription(lpszSessionDescription);
}

void CSqlqueryCtrl::SetSessionStart(long nStart)
{
	if (!m_pSqlQueryFrame)
		return;
	m_pSqlQueryFrame->SetSessionStart(nStart);
}

void CSqlqueryCtrl::InvalidateCursor()
{
	if (!m_pSqlQueryFrame)
		return;
	m_pSqlQueryFrame->InvalidateCursor();
}

void CSqlqueryCtrl::Commit(short nCommit)
{
	if (!m_pSqlQueryFrame)
		return;
	switch (nCommit)
	{
	case 0:
		m_pSqlQueryFrame->Rollback();
		break;
	case 1:
		m_pSqlQueryFrame->Commit();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
}

BOOL CSqlqueryCtrl::IsCommitEnable()
{
	try
	{
		if (!m_pSqlQueryFrame)
			return FALSE;
		return m_pSqlQueryFrame->IsCommittable();
	}
	catch(CeSqlException e)
	{
		return FALSE;
	}
}

void CSqlqueryCtrl::CreateSelectResultPage(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, LPCTSTR lpszStatement)
{
	CreateSelectResultPage4Star(lpszNode, lpszServer, lpszUser, lpszDatabase, 0, lpszStatement);
}

void CSqlqueryCtrl::ConstructPropertySet(CaSqlQueryProperty& property)
{
	//
	// SQL Session:
	property.SetAutoCommit(m_autoCommit);
	property.SetReadLock(m_readLock);
	property.SetTimeout(m_timeOut);
	property.SetSelectMode(m_selectMode);
	property.SetSelectBlock(m_selectLimit);
	//
	// Tab layout:
	property.SetMaxTab(m_maxTab);
	property.SetMaxTrace (m_maxTraceSize);
	property.SetShowNonSelect(m_showNonSelectTab);
	property.SetTraceActivated(m_traceTabActivated);
	property.SetTraceToTop(m_traceTabToTop);
	//
	// Display:
	property.SetF4Width(m_f4Width);
	property.SetF4Decimal(m_f4Scale);
	property.SetF4Exponential(m_f4Exponential);
	property.SetF8Width(m_f8Width);
	property.SetF8Decimal(m_f8Scale);
	property.SetF8Exponential(m_f8Exponential);
	property.SetQepDisplayMode(m_qepDisplayMode);
	property.SetXmlDisplayMode(m_xmlDisplayMode);
	property.SetGrid(m_showGrid);
	//
	// Font:
	CFontHolder& fontHolder = InternalGetFont();
	property.SetFont(fontHolder.GetFontHandle()); // Font
}


void CSqlqueryCtrl::NotifySettingChange(UINT nMask)
{
	FirePropertyChange();
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
	{
		CdSqlQueryRichEditDoc* pDoc = m_pSqlQueryFrame->GetSqlDocument();
		ASSERT(pDoc);
		if (pDoc)
		{
			CaSqlQueryProperty& property = pDoc->GetProperty();
			ConstructPropertySet (property);
			pDoc->UpdateAutocommit(property.IsAutoCommit());
			CuDlgSqlQueryResult* pSqlResult = (CuDlgSqlQueryResult*)m_pSqlQueryFrame->GetDlgSqlQueryResult();
			CWnd* pEditView  = (CWnd*)m_pSqlQueryFrame->GetRichEditView();
			if (pSqlResult)
				pSqlResult->SettingChange(nMask, &property);

			if (pEditView && IsWindow (pEditView->m_hWnd))
				pEditView->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&property);
		}
	}
	else
	if (m_pSelectResultPage && IsWindow (m_pSelectResultPage->m_hWnd))
	{
		CaSqlQueryProperty property;
		ConstructPropertySet (property);
		m_pSelectResultPage->SendMessage (WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)&property);
	}
}


void CSqlqueryCtrl::OnFontChanged()
{
	NotifySettingChange(SQLMASK_FONT);
	COleControl::OnFontChanged();
}

void CSqlqueryCtrl::OnShowGridChanged()
{
	NotifySettingChange(SQLMASK_SHOWGRID);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnAutoCommitChanged()
{
	NotifySettingChange(SQLMASK_AUTOCOMMIT);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnReadLockChanged()
{
	NotifySettingChange(SQLMASK_READLOCK);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnTimeOutChanged()
{
	NotifySettingChange(SQLMASK_TIMEOUT);
	SetModifiedFlag();
}


void CSqlqueryCtrl::OnSelectLimitChanged()
{
	NotifySettingChange(SQLMASK_FETCHBLOCK);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnSelectModeChanged()
{
	NotifySettingChange(SQLMASK_FETCHBLOCK);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnMaxTabChanged()
{
	NotifySettingChange(SQLMASK_MAXTAB);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnMaxTraceSizeChanged()
{
	NotifySettingChange(SQLMASK_MAXTRACESIZE);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnShowNonSelectTabChanged()
{
	NotifySettingChange(SQLMASK_SHOWNONSELECTTAB);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnTraceTabActivatedChanged()
{
	NotifySettingChange(SQLMASK_TRACETAB);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnTraceTabToTopChanged()
{
	NotifySettingChange(SQLMASK_TRACETAB);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF4WidthChanged()
{
	NotifySettingChange(SQLMASK_FLOAT4);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF4ScaleChanged()
{
	NotifySettingChange(SQLMASK_FLOAT4);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF4ExponentialChanged()
{
	NotifySettingChange(SQLMASK_FLOAT4);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF8WidthChanged()
{
	NotifySettingChange(SQLMASK_FLOAT8);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF8ScaleChanged()
{
	NotifySettingChange(SQLMASK_FLOAT8);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnF8ExponentialChanged()
{
	NotifySettingChange(SQLMASK_FLOAT8);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnQepDisplayModeChanged()
{
	NotifySettingChange(SQLMASK_QEPPREVIEW);
	SetModifiedFlag();
}

void CSqlqueryCtrl::OnXmlDisplayModeChanged()
{
	NotifySettingChange(SQLMASK_XMLPREVIEW);
	SetModifiedFlag();
}

void CSqlqueryCtrl::SetDatabaseStar(LPCTSTR lpszDatabase, long nFlag)
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->SetDatabase(lpszDatabase, nFlag);
	}
}

void CSqlqueryCtrl::CreateSelectResultPage4Star(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszDatabase, long nDbFlag, LPCTSTR lpszStatement)
{
	try
	{
		//
		// The function destroys the current frame of SQL pane, create
		// a single page of select statement:
		if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
		{
			m_pSqlQueryFrame->ShowWindow (SW_HIDE);
			m_pSqlQueryFrame->DestroyWindow();
			m_pSqlQueryFrame = NULL;
		}

		if (m_pSelectResultPage)
		{
			m_pSelectResultPage->ShowWindow(SW_HIDE);
			m_pSelectResultPage->DestroyWindow();
			m_pSelectResultPage = NULL;
		}

		if (!m_pSelectResultPage)
		{
			CaConnectInfo connectInfo (lpszNode, lpszDatabase);
			connectInfo.SetServerClass(lpszServer);
			connectInfo.SetUser(lpszUser);
			connectInfo.SetFlag((UINT)nDbFlag);

			CRect r;
			GetWindowRect (r);
			m_pSelectResultPage = new CuDlgSqlQueryPageResult();
			m_pSelectResultPage->SetStandAlone();
			m_pSelectResultPage->Create (IDD_SQLQUERY_PAGERESULT, this);
			m_pSelectResultPage->MoveWindow(r);
			m_pSelectResultPage->ShowWindow(SW_SHOW);

			CaSqlQueryProperty property;
			ConstructPropertySet(property);
			if (lpszStatement)
				m_pSelectResultPage->ExcuteSelectStatement (&connectInfo, lpszStatement, &property);
		}
	}
	catch (CeSqlQueryException e)
	{
		CString strText = e.GetReason();
		if (!strText.IsEmpty())
			AfxMessageBox (strText, MB_ICONEXCLAMATION|MB_OK);

	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_FAIL_2_EXECUTE_STATEMENT, MB_ICONEXCLAMATION|MB_OK);
	}
}

BOOL CSqlqueryCtrl::Initiate2(LPCTSTR lpszNode, LPCTSTR lpszServer, LPCTSTR lpszUser, LPCTSTR lpszOption)
{
	if (m_pSqlQueryFrame)
	{
		m_pSqlQueryFrame->Initiate(lpszNode, lpszServer, lpszUser, lpszOption);
	}
	return TRUE;
}

void CSqlqueryCtrl::OnDestroy()
{
	COleControl::OnDestroy();
	CTypedPtrList < CObList, CaDocTracker* >& ldoc = theApp.GetDocTracker();
	POSITION p, pos = ldoc.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaDocTracker* pObj = ldoc.GetNext(pos);
		if (pObj->GetControl() == (LPARAM)this)
		{
			ldoc.RemoveAt(p);
			delete pObj;
			break;
		}
	}
}

void CSqlqueryCtrl::SetConnectParamInfo(long nSessionVersion)
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
	{
		CdSqlQueryRichEditDoc* pDoc = m_pSqlQueryFrame->GetSqlDocument();
		if (pDoc)
		{
			pDoc->SetConnectParamInfo(nSessionVersion);
			//
			// If the TRACE output is being used, then remove the Trace Tab:
			if (nSessionVersion == INGRESVERS_NOTOI && pDoc->IsUsedTracePage())
			{
				CuDlgSqlQueryResult* pDlgResult  = m_pSqlQueryFrame->GetDlgSqlQueryResult();
				ASSERT(pDlgResult);
				if (pDlgResult)
					pDlgResult->DisplayRawPage (FALSE);
			}
		}
	}
}

long CSqlqueryCtrl::GetConnectParamInfo()
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
	{
		CdSqlQueryRichEditDoc* pDoc = m_pSqlQueryFrame->GetSqlDocument();
		if (pDoc)
			return pDoc->GetConnectParamInfo();
	}

	return -1;
}

long CSqlqueryCtrl::ConnectIfNeeded(short nDisplayError)
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
		return m_pSqlQueryFrame->ConnectIfNeeded((BOOL)nDisplayError);

	return 0;
}

BOOL CSqlqueryCtrl::GetSessionAutocommitState()
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
		return m_pSqlQueryFrame->GetTransactionState(1);

	return m_autoCommit;
}

BOOL CSqlqueryCtrl::GetSessionCommitState()
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
		return m_pSqlQueryFrame->GetTransactionState(2);

	return TRUE;
}

BOOL CSqlqueryCtrl::GetSessionReadLockState()
{
	if (m_pSqlQueryFrame && IsWindow (m_pSqlQueryFrame->m_hWnd))
		return m_pSqlQueryFrame->GetTransactionState(3);

	return m_readLock;
}

void CSqlqueryCtrl::SetHelpFile(LPCTSTR lpszFileWithoutPath)
{
	theApp.m_strHelpFile = lpszFileWithoutPath;

}

void CSqlqueryCtrl::SetErrorFileName(LPCTSTR lpszErrorFileName)
{
	INGRESII_llSetErrorLogFile((LPTSTR)(LPCTSTR)lpszErrorFileName);
}

SHORT CSqlqueryCtrl::GetConnected(LPCTSTR lpszNode, LPCTSTR lpszDatabase)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	BOOL bDisconnectUnusedSession = TRUE;
	CaSessionManager& sMgr = theApp.GetSessionManager();
	CTypedPtrArray< CObArray, CaSession* >&  lss = sMgr.GetListSessions();

	CString strDatabase = lpszDatabase;

	if (lpszDatabase == NULL || strDatabase.IsEmpty())
	{
		// Check to if there is at least one opened session with any database in the provided vnode:
		INT_PTR i, nSize = lss.GetSize();
		for (i=0; i<nSize; i++)
		{
			CaSession* pObj = lss.GetAt(i);
			if (pObj && pObj->IsConnected() && pObj->GetNode().CompareNoCase(lpszNode) == 0)
			{
				if (bDisconnectUnusedSession && pObj->IsReleased())
					pObj->Disconnect();

				if (pObj->IsConnected())
					return 1;
			}
		}
		return 0;
	}
	else
	{
		INT_PTR i, nSize = lss.GetSize();
		for (i=0; i<nSize; i++)
		{
			CaSession* pObj = lss.GetAt(i);
			if (pObj && pObj->IsConnected() &&
			    (pObj->GetNode().CompareNoCase(lpszNode) == 0 && pObj->GetDatabase().CompareNoCase(strDatabase) == 0))
			{
				if (bDisconnectUnusedSession && pObj->IsReleased())
					pObj->Disconnect();

				if (pObj->IsConnected())
					return 1;
			}
		}
		return 0;
	}

	return 0;
}
