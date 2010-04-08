/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijactrlc.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of the CIjaCtrl ActiveX Control class 
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Remove the IDD_ABOUTBOX_IJACTRL and use the about box
**    from tksplash.dll.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 13-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should be disconnected
**    if they ere not used.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Updated Copyright year to 2006.
** 08-Jan-2007 (drivi01)
**    Updated copyright year to 2007.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 20-Mar-2009 (smeke01) b121832
**    Product year is returned by INGRESII_BuildVersionInfo() so does 
**    not need to be set in here.
*/

#include "stdafx.h"
#include "ijactrl.h"
#include "ijactrlc.h"
#include "ijactppg.h"
#include "tlsfunct.h"
#include "usrexcep.h"
#include "copyright.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MODE_DATABASE 0
#define MODE_TABLE    1
static BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight = FW_NORMAL, int bItalic= FALSE);
static BOOL UxCreateFont (CFont& font, LPCTSTR lpFaceName, int size, int weight, int bItalic)
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight        = size; 
	lf.lfWidth         = 0;
	lf.lfEscapement    = 0;
	lf.lfOrientation   = 0;
	lf.lfWeight        = weight;
	lf.lfItalic        = bItalic;
	lf.lfUnderline     = 0;
	lf.lfStrikeOut     = 0;
	lf.lfCharSet       = 0;
	lf.lfOutPrecision  = OUT_CHARACTER_PRECIS;
	lf.lfClipPrecision = 2;
	lf.lfQuality       = PROOF_QUALITY;
	_tcscpy(lf.lfFaceName, lpFaceName);
	return font.CreateFontIndirect(&lf);
}

IMPLEMENT_DYNCREATE(CIjaCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CIjaCtrl, COleControl)
	//{{AFX_MSG_MAP(CIjaCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CIjaCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CIjaCtrl)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "TransactionDelete", m_transactionColorDelete, OnColorTransactionDeleteChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "TransactionInsert", m_transactionColorInsert, OnColorTransactionInsertChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "TransactionAfterUpdate", m_transactionColorAfterUpdate, OnColorTransactionAfterUpdateChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "TransactionBeforeUpdate", m_transactionColorBeforeUpdate, OnColorTransactionBeforeUpdateChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "TransactionOther", m_transactionColorOther, OnColorTransactionOtherChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CIjaCtrl, "PaintForegroundTransaction", m_bPaintForegroundTransaction, OnPaintForegroundTransactionChanged, VT_BOOL)
	DISP_FUNCTION(CIjaCtrl, "SetMode", SetMode, VT_EMPTY, VTS_I2)
	DISP_FUNCTION(CIjaCtrl, "AddUser", AddUser, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "CleanUser", CleanUser, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIjaCtrl, "RefreshPaneDatabase", RefreshPaneDatabase, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "RefreshPaneTable", RefreshPaneTable, VT_EMPTY, VTS_BSTR VTS_BSTR VTS_BSTR  VTS_BSTR  VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "SetUserPos", SetUserPos, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CIjaCtrl, "SetUserString", SetUserString, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "SetAllUserString", SetAllUserString, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "AddNode", AddNode, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "RemoveNode", RemoveNode, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "CleanNode", CleanNode, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CIjaCtrl, "GetMode", GetMode, VT_I2, VTS_NONE)
	DISP_FUNCTION(CIjaCtrl, "EnableRecoverRedo", EnableRecoverRedo, VT_EMPTY, VTS_BOOL)
	DISP_FUNCTION(CIjaCtrl, "GetEnableRecoverRedo", GetEnableRecoverRedo, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(CIjaCtrl, "ShowHelp", ShowHelp, VT_I4, VTS_NONE)
	DISP_FUNCTION(CIjaCtrl, "SetHelpFilePath", SetHelpFilePath, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CIjaCtrl, "SetSessionStart", SetSessionStart, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CIjaCtrl, "SetSessionDescription", SetSessionDescription, VT_EMPTY, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CIjaCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CIjaCtrl, COleControl)
	//{{AFX_EVENT_MAP(CIjaCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CIjaCtrl, 2)
	PROPPAGEID(CIjaCtrlPropPage::guid)
	PROPPAGEID(CLSID_CColorPropPage)
END_PROPPAGEIDS(CIjaCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CIjaCtrl, "IJACTRL.IjaCtrlCtrl.1",
	0xc92b8427, 0xb176, 0x11d3, 0xa3, 0x22, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CIjaCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DIjaCtrl =
		{ 0xc92b8425, 0xb176, 0x11d3, { 0xa3, 0x22, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const IID BASED_CODE IID_DIjaCtrlEvents =
		{ 0xc92b8426, 0xb176, 0x11d3, { 0xa3, 0x22, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwIjaCtrlOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CIjaCtrl, IDS_IJACTRL, _dwIjaCtrlOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::CIjaCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CIjaCtrl

BOOL CIjaCtrl::CIjaCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_IJACTRL,
			IDB_IJACTRL,
			afxRegApartmentThreading,
			_dwIjaCtrlOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::CIjaCtrl - Constructor

CIjaCtrl::CIjaCtrl()
{
	m_nMode = -1;
	m_mDlgDatabase = NULL;
	m_mDlgTable = NULL;
	m_bEnableRecoverRedo = TRUE;

	InitializeIIDs(&IID_DIjaCtrl, &IID_DIjaCtrlEvents);
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::~CIjaCtrl - Destructor

CIjaCtrl::~CIjaCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::OnDraw - Drawing function

void CIjaCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!(m_mDlgTable && m_mDlgDatabase))
		return;
	if (!(IsWindow (m_mDlgTable->m_hWnd) && IsWindow  (m_mDlgDatabase->m_hWnd)))
		return;
	
	switch (m_nMode)
	{
	case MODE_DATABASE:
		m_mDlgTable->ShowWindow(SW_HIDE);
		m_mDlgDatabase->MoveWindow (rcBounds);
		m_mDlgDatabase->ShowWindow(SW_SHOW);
		break;
	case MODE_TABLE:
		m_mDlgDatabase->ShowWindow(SW_HIDE);
		m_mDlgTable->MoveWindow (rcBounds);
		m_mDlgTable->ShowWindow(SW_SHOW);
		break;
	default:
		{
			CRect r = rcBounds;
			m_mDlgTable->ShowWindow(SW_HIDE);
			m_mDlgDatabase->ShowWindow(SW_HIDE);
			pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			CFont font;
			if (!UxCreateFont (font, _T("Arial"), 20))
				return;

			CString strMsg;
			strMsg.LoadString(IDS_PLEASE_SELECTPANE);
			CFont* pOldFond = pdc->SelectObject (&font);
			COLORREF colorTextOld   = pdc->SetTextColor (RGB (0, 0, 255));
			COLORREF oldTextBkColor = pdc->SetBkColor (GetSysColor (COLOR_WINDOW));
			CSize txSize= pdc->GetTextExtent (strMsg, strMsg.GetLength());
			int iPrevMode = pdc->SetBkMode (TRANSPARENT);
			pdc->DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
			pdc->SetBkMode (iPrevMode);
			pdc->SelectObject (pOldFond);
			pdc->SetBkColor (oldTextBkColor);
			pdc->SetTextColor (colorTextOld);
		}
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::DoPropExchange - Persistence support

void CIjaCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	if (pPX->GetVersion() == (DWORD)MAKELONG(_wVerMinor, _wVerMajor))
	{
		PX_Bool(pPX, _T("PaintForegroundTransaction"), m_bPaintForegroundTransaction, theApp.m_property.m_bPaintForegroundTransaction);
		//
		// Defalt color:
		PX_Long (pPX, _T("TransactionDelete"),      (long &)m_transactionColorDelete, TRGBDEF_DELETE);
		PX_Long (pPX, _T("TransactionInsert"),      (long &)m_transactionColorInsert, TRGBDEF_INSERT);
		PX_Long (pPX, _T("TransactionAfterUpdate"), (long &)m_transactionColorAfterUpdate, TRGBDEF_AFTEREUPDATE);
		PX_Long (pPX, _T("TransactionBeforeUpdate"),(long &)m_transactionColorBeforeUpdate, TRGBDEF_BEFOREUPDATE);
		PX_Long (pPX, _T("TransactionOther"),       (long &)m_transactionColorOther, TRGBDEF_OTHERS);
	}
	else if (pPX->IsLoading())
	{
		// Skip over persistent data
		BOOL bDummy;
		long lDummy;
		CString strDummy;

		PX_Bool(pPX, _T("PaintForegroundTransaction"), bDummy, TRUE);

		PX_Long (pPX, _T("FlashColor"), lDummy, 0);
		PX_Long (pPX, _T("TransactionDelete"),      lDummy, TRGBDEF_DELETE);
		PX_Long (pPX, _T("TransactionInsert"),      lDummy, TRGBDEF_INSERT);
		PX_Long (pPX, _T("TransactionAfterUpdate"), lDummy, TRGBDEF_AFTEREUPDATE);
		PX_Long (pPX, _T("TransactionBeforeUpdate"),lDummy, TRGBDEF_BEFOREUPDATE);
		PX_Long (pPX, _T("TransactionOther"),       lDummy, TRGBDEF_OTHERS);

		// Force property values to these defaults
		m_bPaintForegroundTransaction = DEFPAINT_FKTRANSACTION;
		m_transactionColorDelete      = TRGBDEF_DELETE;
		m_transactionColorInsert      = TRGBDEF_INSERT;
		m_transactionColorAfterUpdate = TRGBDEF_AFTEREUPDATE;
		m_transactionColorBeforeUpdate= TRGBDEF_BEFOREUPDATE;
		m_transactionColorOther       = TRGBDEF_OTHERS;
	}

	theApp.m_property.m_bPaintForegroundTransaction = m_bPaintForegroundTransaction;
	theApp.m_property.m_transactionColorDelete      = m_transactionColorDelete;
	theApp.m_property.m_transactionColorInsert      = m_transactionColorInsert;
	theApp.m_property.m_transactionColorAfterUpdate = m_transactionColorAfterUpdate;
	theApp.m_property.m_transactionColorBeforeUpdate= m_transactionColorBeforeUpdate;
	theApp.m_property.m_transactionColorOther       = m_transactionColorOther;
	// TODO: Call PX_ functions for each persistent custom property.
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::OnResetState - Reset control to default state

void CIjaCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl::AboutBox - Display an "About" box to the user

void CIjaCtrl::AboutBox()
{
	BOOL bOK = TRUE;
	CString strLibraryName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strLibraryName  = _T("libtksplash.sl");
	#else
	strLibraryName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strLibraryName);
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
			strAbout.Format(IDS_PRODUCT_VERSION, strVer);
			strTitle.LoadString (AFX_IDS_APP_TITLE);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, strLibraryName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrl message handlers

void CIjaCtrl::SetMode(short nMode) 
{
	TRACE0 ("CIjaCtrl::SetMode \n");
	if (nMode == MODE_DATABASE || nMode == MODE_TABLE)
	{
		m_nMode = nMode;
	}
	else
		m_nMode = -1;
	if (m_nMode == -1)
		theApp.GetSessionManager().Cleanup();
	Invalidate();
}

long CIjaCtrl::AddUser(LPCTSTR strUser) 
{
	m_mDlgDatabase->AddUser (strUser);
	m_mDlgTable->AddUser (strUser);
	return 0;
}

void CIjaCtrl::CleanUser()
{
	m_mDlgDatabase->CleanUser();
	m_mDlgTable->CleanUser();
}

void CIjaCtrl::RefreshPaneDatabase(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner) 
{
	try
	{
		ASSERT (m_nMode == MODE_DATABASE);
		if (m_nMode != MODE_DATABASE)
			return;
		if (m_mDlgDatabase && IsWindow (m_mDlgDatabase->m_hWnd))
			m_mDlgDatabase->RefreshPaneDatabase(lpszNode, lpszDatabase, lpszDatabaseOwner);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error: raise exception at 'CIjaCtrl::RefreshPaneDatabase'."), MB_ICONEXCLAMATION|MB_OK);
	}
}

void CIjaCtrl::RefreshPaneTable(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner, LPCTSTR lpszTable, LPCTSTR lpszTableOwner) 
{
	try
	{
		ASSERT (m_nMode == MODE_TABLE);
		if (m_nMode != MODE_TABLE)
			return;
		if (m_mDlgTable && IsWindow (m_mDlgTable->m_hWnd))
			m_mDlgTable->RefreshPaneTable(lpszNode, lpszDatabase, lpszDatabaseOwner, lpszTable, lpszTableOwner);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error: raise exception at 'CIjaCtrl::RefreshPaneTable'."), MB_ICONEXCLAMATION|MB_OK);
	}
}

void CIjaCtrl::SetUserPos(long nPos) 
{
	switch (m_nMode)
	{
	case MODE_DATABASE:
		m_mDlgDatabase->m_cComboUser.SetCurSel(nPos);
		break;
	case MODE_TABLE:
		m_mDlgTable->m_cComboUser.SetCurSel(nPos);
		break;
	default:
		break;
	}
}


void CIjaCtrl::SetUserString(LPCTSTR lpszUser) 
{
	int nFound = CB_ERR;
	switch (m_nMode)
	{
	case MODE_DATABASE:
		nFound = m_mDlgDatabase->m_cComboUser.FindStringExact (-1, lpszUser);
		if (nFound != CB_ERR)
			m_mDlgDatabase->m_cComboUser.SetCurSel (nFound);
		break;
	case MODE_TABLE:
		nFound = m_mDlgTable->m_cComboUser.FindStringExact (-1, lpszUser);
		if (nFound != CB_ERR)
			m_mDlgTable->m_cComboUser.SetCurSel (nFound);
		break;
	default:
		break;
	}
}

void CIjaCtrl::SetAllUserString(LPCTSTR lpszAllUser) 
{
	theApp.m_strAllUser = lpszAllUser;
}

void CIjaCtrl::AddNode(LPCTSTR lpszNode) 
{
	theApp.m_listNode.AddTail(lpszNode);
}

void CIjaCtrl::RemoveNode(LPCTSTR lpszNode) 
{
	CString strItem;
	POSITION p, pos = theApp.m_listNode.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		strItem = theApp.m_listNode.GetNext (pos);
		if (strItem.CompareNoCase (lpszNode) == 0)
		{
			theApp.m_listNode.RemoveAt (p);
			break;
		}
	}
}

void CIjaCtrl::CleanNode() 
{
	theApp.m_listNode.RemoveAll();
}

short CIjaCtrl::GetMode() 
{
	return m_nMode;
}

int CIjaCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_mDlgDatabase)
	{
		m_mDlgDatabase = new CuDlgIjaDatabase(this);
		m_mDlgDatabase->Create (IDD_DATABASE, this);
	}

	if (!m_mDlgTable)
	{
		m_mDlgTable = new CuDlgIjaTable(this);
		m_mDlgTable->Create (IDD_TABLE, this);
	}

	return 0;
}

void CIjaCtrl::OnColorTransactionDeleteChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}

void CIjaCtrl::OnColorTransactionInsertChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}

void CIjaCtrl::OnColorTransactionAfterUpdateChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}

void CIjaCtrl::OnColorTransactionBeforeUpdateChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}

void CIjaCtrl::OnColorTransactionOtherChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
}

void CIjaCtrl::OnPaintForegroundTransactionChanged() 
{
	InvalidateControl();

	SetModifiedFlag();
}

void CIjaCtrl::EnableRecoverRedo(BOOL bEnable) 
{
	m_bEnableRecoverRedo = bEnable;
	if ((m_mDlgDatabase && m_mDlgTable) && (IsWindow (m_mDlgDatabase->m_hWnd) && IsWindow (m_mDlgTable->m_hWnd)))
	{
		m_mDlgDatabase->EnableRecoverRedo(m_bEnableRecoverRedo);
		m_mDlgTable->EnableRecoverRedo(m_bEnableRecoverRedo);
	}
}

BOOL CIjaCtrl::GetEnableRecoverRedo() 
{
	return m_bEnableRecoverRedo;
}

long CIjaCtrl::ShowHelp() 
{
	long lHelpID = -1;
	switch (m_nMode)
	{
	case MODE_DATABASE:
		lHelpID = IDD_DATABASE;
		break;
	case MODE_TABLE:
		lHelpID = IDD_TABLE;
		break;
	default:
		break;
	}
	if (lHelpID != -1)
	{
		APP_HelpInfo (m_hWnd, lHelpID);
	}
	return lHelpID;
}


void CIjaCtrl::SetHelpFilePath(LPCTSTR lpszPath) 
{
	//
	// Due to the MFC documentation, we should use the two instructions below,
	// but it causes the heap error: HEAP[Ija.exe]: 
	// "Heap block at a52708 modified at a5273f past requested size of 2f".
	// This is why I create a new member m_strNewHelpPath.

#if 0 // Heap error
	free((void*)theApp.m_pszHelpFilePath);
	theApp.m_pszHelpFilePath = _tcsdup(lpszPath);
#endif

	theApp.m_strNewHelpPath = lpszPath;
}

void CIjaCtrl::SetSessionStart(long nStart) 
{
	theApp.GetSessionManager().SetSessionStart(nStart);
}

void CIjaCtrl::SetSessionDescription(LPCTSTR lpszDescription) 
{
	theApp.GetSessionManager().SetDescription(lpszDescription);
}
