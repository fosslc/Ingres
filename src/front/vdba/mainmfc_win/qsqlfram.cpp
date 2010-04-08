/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsqlfram.cpp, Implementation file    (MDI Child Frame)
**    Project  : ingresII / vdba.exe.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : MDI Frame window for the SQL Test.
**
** History:
**
** 19-Feb-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate SQL Test ActiveX Control
** 15-Mar-2002 (schph01)
**    Bug (107331) Update the ingres version on the WM_MDIACTIVATE message.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 14-Apr-2004: (uk$so01)
**    BUG #112150 /ISSUE 13350768 
**    VDBA, Load/Save fails to function correctly with .NET environment.
** 02-Aug-2004 (uk$so01)
**    BUG #112765, ISSUE 13364531 (Activate the print of sql test)
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
*/

#include "stdafx.h"
#include "rcdepend.h"
#include <comdef.h>
#include "qsqldoc.h"
#include "qsqlfram.h"
#include "qsqlview.h"
#include "sqlasinf.h" // sql assistant interface
#include "cmdline.h"  // Unicenter-driven features
#include "extccall.h"
#include "property.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//#define LOCAL_PROPERTIES_SQLQUERY // Force the MDI to have its own proerties. 


CfSqlQueryFrame::CfSqlQueryFrame()
{
	m_pSqlDoc = NULL;
	m_strNone.LoadString(IDS_DATABASE_NONE);
}

CfSqlQueryFrame::~CfSqlQueryFrame()
{
}


CuSqlQueryControl* CfSqlQueryFrame::GetSqlqueryControl()
{
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (pCtrl && IsWindow (pCtrl->m_hWnd))
		return pCtrl;
	return NULL;
}

BOOL CfSqlQueryFrame::SetConnection(BOOL bDisplayMessage)
{
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pDoc);
	if (!pDoc)
		return FALSE;
	CString strNode = pDoc->GetNode();
	CString strServer = pDoc->GetServer();
	CString strUser = pDoc->GetUser();
	CString strDatabase = pDoc->GetDatabase();
	if (strDatabase.IsEmpty() || strDatabase.CompareNoCase(m_strNone) == 0)
	{
		if (bDisplayMessage)
			AfxMessageBox (IDS_PLEASE_SELECT_DATABASE);
		return FALSE;
	}

	CuSqlQueryControl* pCtrl = pDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (pCtrl && IsWindow (pCtrl->m_hWnd))
	{
		pCtrl->Initiate(strNode, strServer, strUser);
		UINT nFlag = pDoc->GetDbFlag();
		if (nFlag > 0)
			pCtrl->SetDatabaseStar(strDatabase, nFlag);
		else
			pCtrl->SetDatabase(strDatabase);
		return TRUE;
	}
	return FALSE;
}


void CfSqlQueryFrame::CommitOldSession()
{
	int nAutocommitOFF= 0;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	short nAutoCommit = (short)pCtrl->GetAutoCommit();
	if (pCtrl && nAutoCommit == nAutocommitOFF)
	{
		if (!pCtrl->IsCommitEnable())
			return;
		int nAnsw = AfxMessageBox (IDS_MSG_COMMIT_INVITATION, MB_ICONQUESTION|MB_YESNO);
		if (nAnsw == IDYES)
			pCtrl->Commit(1);
		else
			pCtrl->Commit(0);
	}
}

//
// This function might thorw the CMemoryExecption
// If lpszDatabase is not Null, then the function tries to selected that Database
BOOL CfSqlQueryFrame::RequeryDatabase (LPCTSTR lpszDatabase)
{
	int   index;
	int   ires;
	TCHAR buf       [MAXOBJECTNAME];
	TCHAR buffilter [MAXOBJECTNAME];
	CString strCurSel = _T("");
	CString strNone;
	CWaitCursor waitCursor;
	if (!m_pSqlDoc)
		m_pSqlDoc = (CdSqlQuery*)GetActiveDocument();
	ASSERT (m_pSqlDoc);
	
	m_ComboDatabase.ResetContent ();
	m_ComboDatabase.AddString (m_strNone);
	
	memset (buf, 0, sizeof (buf));
	memset (buffilter, 0, sizeof (buffilter));
	ires    = DOMGetFirstObject (m_pSqlDoc->GetNodeHandle(), OT_DATABASE, 0, NULL, TRUE, NULL, (LPUCHAR)buf, NULL, NULL);
	if (ires != RES_SUCCESS)
		return FALSE;
	while (ires == RES_SUCCESS)
	{
		m_ComboDatabase.AddString (buf);
		ires  = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, NULL);
	}
	if (!lpszDatabase)
		return TRUE;
	index = m_ComboDatabase.FindStringExact (-1, lpszDatabase);
	if (index != CB_ERR)
		m_ComboDatabase.SetCurSel (index);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame

IMPLEMENT_DYNCREATE(CfSqlQueryFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CfSqlQueryFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CfSqlQueryFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTON_QEP, OnButtonQueryQep)
	ON_COMMAND(ID_BUTTON_QUERYGO, OnButtonQueryGo)
	ON_COMMAND(ID_BUTTON_QUERYCLEAR, OnButtonQueryClear)
	ON_COMMAND(ID_BUTTON_QUERYOPEN, OnButtonQueryOpen)
	ON_COMMAND(ID_BUTTON_QUERYSAVEAS, OnButtonQuerySaveas)
	ON_COMMAND(ID_BUTTON_SQLWIZARD, OnButtonSqlWizard)
	ON_COMMAND(ID_BUTTON_SETTING, OnButtonSetting)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_QUERYCLEAR, OnUpdateButtonQueryClear)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_QUERYGO, OnUpdateButtonQueryGo)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_QEP, OnUpdateButtonQep)
	ON_WM_CLOSE()
	ON_COMMAND(ID_BUTTON_RAW, OnButtonRaw)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_RAW, OnUpdateButtonRaw)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PRINT, OnUpdateButtonPrint)
	ON_COMMAND(ID_BUTTON_PRINT, OnButtonPrint)
	ON_WM_DESTROY()
	ON_COMMAND(ID_BUTTON_XML, OnButtonXml)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_XML, OnUpdateButtonXml)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SETTING, OnUpdateButtonSetting)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SPACECALC, OnUpdateButtonSpaceCalc)
	ON_WM_MDIACTIVATE()
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(ID_BUTTON_DATABASE, OnSelChangeDatabase)
	ON_CBN_DROPDOWN (ID_BUTTON_DATABASE, OnDropDownDatabase)

	ON_MESSAGE(WM_USER_GETMFCDOCTYPE, OnGetMfcDocType)
	ON_MESSAGE(WM_USER_GETNODEHANDLE, OnGetNodeHandle)

	//
	// Toolbar management
	ON_MESSAGE(WM_USER_HASTOOLBAR,        OnHasToolbar      )
	ON_MESSAGE(WM_USER_ISTOOLBARVISIBLE,  OnIsToolbarVisible)
	ON_MESSAGE(WM_USER_SETTOOLBARSTATE,   OnSetToolbarState )
	ON_MESSAGE(WM_USER_SETTOOLBARCAPTION, OnSetToolbarCaption)
END_MESSAGE_MAP()

LONG CfSqlQueryFrame::OnGetMfcDocType(UINT uu, LONG ll)
{
	CDocument* pDoc = GetActiveDocument();
	if (pDoc)
		return TYPEDOC_SQLACT;
	else
		return TYPEDOC_UNKNOWN;
}

LONG CfSqlQueryFrame::OnGetNodeHandle(UINT uu, LONG ll)
{
	if (!m_pSqlDoc)
		m_pSqlDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT (m_pSqlDoc);
	if (m_pSqlDoc)
	{
		return (LONG)m_pSqlDoc->GetNodeHandle();
	}

	return -1L;
}


void CfSqlQueryFrame::OnSelChangeDatabase()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdSqlQuery* pDoc = (CdSqlQuery*)GetSqlDoc();
	CString strDatabase = pDoc->GetDatabase();

	CString strNewDatabase;
	nIdx = m_ComboDatabase.GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_ComboDatabase.GetLBText (nIdx, strNewDatabase);
	if (strNewDatabase.CompareNoCase(strDatabase) != 0)
	{
		UINT nFlag = 0;
		if ( IsStarDatabase(pDoc->GetNodeHandle(), (LPUCHAR)(LPCTSTR)strNewDatabase))
			nFlag = DBFLAG_STARNATIVE;
		pDoc->SetDatabase(strNewDatabase, nFlag);
		CommitOldSession();
	}
}

void CfSqlQueryFrame::OnDropDownDatabase()
{
	int nCurSel = m_ComboDatabase.GetCurSel();
	SetExpandingCombobox();
	RequeryDatabase ();
	ResetExpandingCombobox();
	m_ComboDatabase.SetCurSel(nCurSel);
}



BOOL CfSqlQueryFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// if unicenter driven: preset maximized state, if requested
	if (IsUnicenterDriven()) 
	{
		CuCmdLineParse* pCmdLineParse = GetAutomatedGeneralDescriptor();
		ASSERT (pCmdLineParse);
		if (pCmdLineParse->DoWeMaximizeWin()) 
		{
			// Note: Maximize MUST be combined with VISIBLE
			cs.style |= WS_MAXIMIZE | WS_VISIBLE;
			if (!theApp.CanCloseContextDrivenFrame())
			{
				cs.style &=~WS_SYSMENU;
			}
		}
	}
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame diagnostics

#ifdef _DEBUG
void CfSqlQueryFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CfSqlQueryFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CfSqlQueryFrame message handlers

int CfSqlQueryFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	const int ID_BUTTON_DATABASE_POS = 6;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!m_pSqlDoc)
		m_pSqlDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(m_pSqlDoc);
	if (!m_pSqlDoc)
		return -1;

	if (!m_wndToolBar.Create(this))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	
	if (!m_wndToolBar.LoadToolBar(IDR_SQLACT))
	{
		TRACE0("Failed to load toolbar\n");
		return -1;      // fail to create
	}
#if !defined (LOCAL_PROPERTIES_SQLQUERY)
#if defined (EDBC)
	int nPos = 16;
#else
	int nPos = 14;
#endif
	m_wndToolBar.GetToolBarCtrl().DeleteButton (nPos);
#endif

	//
	// Create a ComboBox in the toolbar (Connect to the Database)
	CRect rect(-150, -160, 0, 0);
	if (!m_ComboDatabase.Create(WS_CHILD | CBS_DROPDOWNLIST |
		CBS_AUTOHSCROLL | WS_VSCROLL | CBS_HASSTRINGS, rect, &m_wndToolBar,
		ID_BUTTON_DATABASE))
	{
		return -1;
	}
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	m_ComboDatabase.SendMessage(WM_SETFONT, (WPARAM)hFont);

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_ALIGN_TOP);
	m_wndToolBar.SetButtonInfo(ID_BUTTON_DATABASE_POS, ID_BUTTON_DATABASE, TBBS_SEPARATOR, 150);
	if (m_ComboDatabase.m_hWnd != NULL)
	{
		m_wndToolBar.GetItemRect(ID_BUTTON_DATABASE_POS, rect);
		m_ComboDatabase.SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
		m_ComboDatabase.ShowWindow(SW_SHOW);
	}
	//
	// The Raw Button is a Check Button:
	int nIndex = m_wndToolBar.CommandToIndex (ID_BUTTON_RAW);
	m_wndToolBar.SetButtonStyle (nIndex, TBBS_CHECKBOX);   

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	// TODO: Delete these three lines if you don't want the toolbar to
	// be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);

	//
	// The Raw Button is a Check Button:
	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();
	nIndex = m_wndToolBar.CommandToIndex (ID_BUTTON_RAW);
	m_wndToolBar.SetButtonStyle (nIndex, TBBS_CHECKBOX);
	CuSqlQueryControl* pCtrl = m_pSqlDoc->GetSqlQueryCtrl();
	ASSERT(pCtrl && IsWindow (pCtrl->m_hWnd));
	if (!(pCtrl || IsWindow (pCtrl->m_hWnd)))
		return -1;
	if (pCtrl->IsUsedTracePage())
		tbctrl.CheckButton(ID_BUTTON_RAW, TRUE);
	else
		tbctrl.CheckButton(ID_BUTTON_RAW, FALSE);
	//
	// Set caption to toolbar
	CString str;
	str.LoadString (IDS_SQLACTBAR_TITLE);
	m_wndToolBar.SetWindowText (str);
	CString strNone;
	if (!strNone.LoadString (IDS_DATABASE_NONE))
		strNone = _T("(None)");
	m_ComboDatabase.AddString (strNone);
	m_ComboDatabase.SetCurSel (0);
	m_ComboDatabase.SetFocus();

	//
	// Handle command line input database:
	CString strInputDatabase = strNone;
	if (!m_pSqlDoc)
		m_pSqlDoc = (CdSqlQuery*)GetSqlDoc();
	if (!m_pSqlDoc->GetDatabase().IsEmpty())
		strInputDatabase = m_pSqlDoc->GetDatabase();
	if (m_pSqlDoc->IsLoadedDoc())
	{
		UpdateLoadedData(m_pSqlDoc);
	}
	else
	{
		RequeryDatabase (strInputDatabase);
	}

	return 0;
}

BOOL CfSqlQueryFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	return CMDIChildWnd::OnCreateClient(lpcs, pContext);
}



void CfSqlQueryFrame::OnButtonQueryQep() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	pCtrl->Qep();
}



//
// The string in the edit box might contain more than one statement.
// If so, the statement must be seperated by a ';'.
// We parse the the string to get each statement to execute.
void CfSqlQueryFrame::OnButtonQueryGo() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	pCtrl->Run();
}

void CfSqlQueryFrame::OnButtonRaw() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->EnableTrace();
}


void CfSqlQueryFrame::OnButtonQueryClear() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Clear();
}

//
//  Queries for a file name containing a sql statement,
//  and loads the statement into the edit control
void CfSqlQueryFrame::OnButtonQueryOpen() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Open();
}

void CfSqlQueryFrame::OnButtonQuerySaveas() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	pCtrl->Save();
}

void CfSqlQueryFrame::OnButtonSqlWizard() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	pCtrl->SqlAssistant();
}

void CfSqlQueryFrame::OnButtonSetting() 
{
#if defined (LOCAL_PROPERTIES_SQLQUERY)
	USES_CONVERSION;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	CString strSqlSettingCaption = _T("SqlTest");
	strSqlSettingCaption.LoadString(IDS_CAPT_SQLTEST);
	LPCOLESTR lpCaption = T2COLE((LPTSTR)(LPCTSTR)strSqlSettingCaption);

	IUnknown* pUk = pCtrl->GetControlUnknown();
	ISpecifyPropertyPagesPtr pSpecifyPropertyPages = pUk;
	CAUUID pages;
	HRESULT hr = pSpecifyPropertyPages->GetPages( &pages );
	if(FAILED(hr ))
		return;

	PushHelpId ((UINT)SQLACTPREFDIALOG);
	theApp.PropertyChange(FALSE);
	hr = OleCreatePropertyFrame(
		m_hWnd,
		10,
		10,
		lpCaption,
		1,
		(IUnknown**) &pUk,
		pages.cElems,
		pages.pElems,
		0, 
		0L,
		NULL );
	PopHelpId ();

	CoTaskMemFree( (void*) pages.pElems );
	if (theApp.IsPropertyChange())
	{
		//
		// Save to Global:
		IPersistStreamInit* pPersistStreammInit = NULL;
		hr = pUk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			IStream* pStream = theApp.m_sqlStreamFile.Detach();
			pStream->Release();
			if (theApp.m_sqlStreamFile.CreateMemoryStream())
			{
				pStream = theApp.m_sqlStreamFile.GetStream();
				hr = pPersistStreammInit->Save(pStream, TRUE);
				pPersistStreammInit->Release();
			}
		}

		if (theApp.IsSavePropertyAsDefault())
		{
			BOOL bSave = SaveStreamInit(pCtrl->GetControlUnknown(), _T("SqlQuery"));
			if (!bSave)
				AfxMessageBox (IDS_FAILED_2_SAVESTORAGE);
		}
	}
#endif
}

void CfSqlQueryFrame::OnUpdateButtonQueryClear(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
	{
		bEnable = pCtrl->IsClearEnable();
	}
	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnUpdateButtonQueryGo(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsRunEnable();
	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnUpdateButtonQep(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsQepEnable();
	pCmdUI->Enable(bEnable);

}

void CfSqlQueryFrame::OnUpdateButtonRaw(CCmdUI* pCmdUI) 
{
	BOOL bEnable = TRUE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = pCtrl->IsTraceEnable();
	CToolBarCtrl& tbctrl = m_wndToolBar.GetToolBarCtrl();
	if (pCtrl->IsUsedTracePage())
		tbctrl.CheckButton(ID_BUTTON_RAW, TRUE);
	else
		tbctrl.CheckButton(ID_BUTTON_RAW, FALSE);

	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnClose() 
{
	if (m_pSqlDoc)
		m_pSqlDoc->SetModifiedFlag (FALSE); // Do not want the default message.
	if (!theApp.CanCloseContextDrivenFrame())
		return;
	CMDIChildWnd::OnClose();
}


LONG CfSqlQueryFrame::OnHasToolbar(WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}

LONG CfSqlQueryFrame::OnIsToolbarVisible(WPARAM wParam, LPARAM lParam)
{
	return (LONG)m_wndToolBar.IsWindowVisible();
}

LONG CfSqlQueryFrame::OnSetToolbarState(WPARAM wParam, LPARAM lParam)
{
	//
	// lParam : TRUE means UpdateImmediate
	BOOL bDelay = (lParam ? FALSE : TRUE);
	if (wParam)
		ShowControlBar(&m_wndToolBar, TRUE, bDelay);
	else
		ShowControlBar(&m_wndToolBar, FALSE, bDelay);
	return (LONG)TRUE;
}


LONG CfSqlQueryFrame::OnSetToolbarCaption(WPARAM wParam, LPARAM lParam)
{
	m_wndToolBar.SetWindowText((LPCSTR)lParam);
	return (LONG)TRUE;
}



void CfSqlQueryFrame::OnUpdateButtonPrint(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = pCtrl->IsPrintEnable();

	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnButtonPrint() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		pCtrl->Print();
}


void CfSqlQueryFrame::OnDestroy() 
{
	theApp.SetLastDocMaximizedState(IsZoomed());

	CMDIChildWnd::OnDestroy();
}

void CfSqlQueryFrame::OnButtonXml() 
{
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (!pCtrl)
		return;
	if (!SetConnection())
		return;
	pCtrl->Xml();
}

void CfSqlQueryFrame::OnUpdateButtonXml(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl && SetConnection(FALSE))
		bEnable = pCtrl->IsXmlEnable();
	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnUpdateButtonSetting(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CuSqlQueryControl* pCtrl = GetSqlqueryControl();
	if (pCtrl)
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CfSqlQueryFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	// maintain gateway type and oivers
	if (bActivate) {
		ASSERT (pActivateWnd == this);
		CdSqlQuery *pSqlDoc = (CdSqlQuery*)GetSqlDoc();
		ASSERT(pSqlDoc);
		if (pSqlDoc) {
			int iVers = pSqlDoc->GetIngresVersion();
			if (iVers != -1)
				SetOIVers(iVers);
		}
	}
}

void CfSqlQueryFrame::UpdateLoadedData(CdSqlQuery* pDoc)
{
	try
	{
		CString strDatabase = pDoc->GetDatabase();
		if (!strDatabase.IsEmpty()) 
		{
			int nIdx = m_ComboDatabase.AddString (strDatabase);
			m_ComboDatabase.SetCurSel (nIdx);
		}
		WINDOWPLACEMENT& wplj = pDoc->GetWindowPlacement();
		SetWindowPlacement(&wplj);
	}
	catch(...)
	{

	}
}

void CfSqlQueryFrame::OnUpdateButtonSpaceCalc(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CdSqlQuery *pSqlDoc = (CdSqlQuery*)GetSqlDoc();
	ASSERT(pSqlDoc);
	if (pSqlDoc) {
		if (pSqlDoc->GetIngresVersion() != OIVERS_NOTOI)
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}