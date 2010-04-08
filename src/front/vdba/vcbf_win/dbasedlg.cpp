/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dbasedlg.cpp, Implementation File
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**               Blattes Emannuel
**    Purpose  : Special Custom List Box to provide on line editing. 
**               DBMS Database page
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 04-Jun-2002: (uk$so01) 
**    (BUG #107927)
**    Show the bottons "Add" and "Remove" for this tab.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "vcbf.h"
#include "dbasedlg.h"
#include "crightdg.h"
#include "ll.h"
#include "cbfitem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsDatabase dialog
IMPLEMENT_DYNCREATE(CuDlgDbmsDatabase, CFormView)


CuDlgDbmsDatabase::CuDlgDbmsDatabase()
	: CFormView(CuDlgDbmsDatabase::IDD)
{
	m_pDbmsItemData = NULL;
	//{{AFX_DATA_INIT(CuDlgDbmsDatabase)
	//}}AFX_DATA_INIT
}


void CuDlgDbmsDatabase::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDbmsDatabase)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDbmsDatabase, CFormView)
	//{{AFX_MSG_MAP(CuDlgDbmsDatabase)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnAdd)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnDelete)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,        OnEditOK)
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,    OnEditCancel)
END_MESSAGE_MAP()

#ifdef _DEBUG
void CuDlgDbmsDatabase::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgDbmsDatabase::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgDbmsDatabase message handlers




void CuDlgDbmsDatabase::OnInitialUpdate() 
{
	VERIFY (m_DbList.SubclassDlgItem (IDC_LIST_DATABASES, this));
}

LONG CuDlgDbmsDatabase::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  //
  // Preliminary: Get ItemData for future use on buttons
  //
  // Fix Oct 27, 97: lParam is NULL when OnUpdateData() called
  // after activation of "Preferences" tag followed by activation of "Configure" tag
  if (lParam) {
    CuDBMSItemData* pDbmsItemData = (CuDBMSItemData*)lParam;
    m_pDbmsItemData = pDbmsItemData;
  }
  else
    ASSERT (m_pDbmsItemData);   // check contained something valid

	//
	// Set labels for buttons
	//
	CWnd* pParent1 = GetParent ();            // CuDlgViewFrame
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();  // CTabCtrl
	ASSERT_VALID (pParent2);
	CWnd* pParent3 = pParent2->GetParent ();  // CConfRightDlg
	ASSERT_VALID (pParent3);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON2);
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_ADD);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_REMOVE);
	pButton2->SetWindowText (csButtonTitle);

	pButton1->EnableWindow (TRUE);
	pButton2->EnableWindow (TRUE);
	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);
	m_DbList.HideProperty();

  // Fill listbox ONLY IF NECESSARY
  if (m_pDbmsItemData->m_bMustFillListbox) {
    m_pDbmsItemData->m_bMustFillListbox = FALSE;
    // Clear list
    m_DbList.ResetContent();

    // fill listbox according to low level
    ASSERT (m_pDbmsItemData->m_initialDbList);
    CString csDbList(m_pDbmsItemData->m_initialDbList);
    while (1) {
      csDbList.TrimLeft();    // suppress potential leading spaces, each tour
	  if (csDbList.IsEmpty())
		  break; // contained only blank characters before TrimLeft
      int blank = csDbList.Find(_T(' '));
      if (blank == -1) {
        m_DbList.AddString(csDbList);
        break;  // break the while
      }
      //
      // DBCS Compliant:
      CString csOneDb = csDbList.Left(blank);
      csDbList = csDbList.Right(csDbList.GetLength() - blank - 1);
      ASSERT (!csOneDb.IsEmpty());
      m_DbList.AddString(csOneDb);
    }
    if (m_DbList.GetCount())
      m_DbList.SetCurSel(0);

    // build result string according to what's in the listbox
    m_pDbmsItemData->m_resultDbList = BuildResultList();
  }

	return 0L;
}


LONG CuDlgDbmsDatabase::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_DbList.HideProperty(TRUE);
	return 0L;
}


LONG CuDlgDbmsDatabase::OnAdd (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsDatabase::OnAdd...\n");
	m_DbList.SetEditText (); // Add Item;
	return 0L;
}

LONG CuDlgDbmsDatabase::OnDelete (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsDatabase::OnDelete...\n");
	int index = m_DbList.GetCurSel();
	if (index != -1) {
		m_DbList.DeleteItem (index);
		// rebuild result string and flag modification
		m_pDbmsItemData->m_resultDbList = BuildResultList();
		m_pDbmsItemData->m_bDbListModified = TRUE;
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);
	return 0L;
}

LONG CuDlgDbmsDatabase::OnEditOK (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsDatabase::OnEditOK...\n");
	//
	// Rebuild result string and flag modification
	m_pDbmsItemData->m_resultDbList = BuildResultList();
	m_pDbmsItemData->m_bDbListModified = TRUE;
	return 0;
}


LONG CuDlgDbmsDatabase::OnEditCancel (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgDbmsDatabase::OnEditCancel...\n");
	return 0;
}

CString CuDlgDbmsDatabase::BuildResultList()
{
  CString resultString = _T("");
  int count = m_DbList.GetCount();
  BOOL bFirst = TRUE;
  for (int index=0; index < count; index++) {
    CString oneS;
    m_DbList.GetText(index, oneS);
    if (bFirst)
      bFirst = FALSE;
    else
      resultString += _T(" ");
    resultString += oneS;
  }
  return resultString;
}




/////////////////////////////////////////////////////////////////////////////
// CuDbmsDatabaseListBox

CuDbmsDatabaseListBox::CuDbmsDatabaseListBox()
{
	m_pEditDlg = NULL;
}

CuDbmsDatabaseListBox::~CuDbmsDatabaseListBox()
{
}

void CuDbmsDatabaseListBox::HideProperty(BOOL bValidate)
{
	if (m_pEditDlg && m_pEditDlg->IsWindowVisible())
	{
		if (bValidate)
			m_pEditDlg->OnOK();
		else
			m_pEditDlg->OnCancel();
	}
}

void CuDbmsDatabaseListBox::DeleteItem  (int iIndex)
{
	if (m_pEditDlg && m_pEditDlg->IsWindowVisible())
	{
		m_pEditDlg->OnCancel();
	}
	else
	{
		int iIndex = GetCurSel();
		if (iIndex != LB_ERR) 
		{
			DeleteString (iIndex);
		}
		else
		{
			MessageBeep(MB_ICONEXCLAMATION);
			return;
		}
	}
	if (iIndex < GetCount())
		SetCurSel(iIndex);
	else
		SetCurSel(GetCount() - 1);
}


BEGIN_MESSAGE_MAP(CuDbmsDatabaseListBox, CListBox)
	//{{AFX_MSG_MAP(CuDbmsDatabaseListBox)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelchange)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,     OnEditOK)
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL, OnEditCancel)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDbmsDatabaseListBox message handlers

LONG CuDbmsDatabaseListBox::OnEditOK (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDbmsDatabaseListBox::OnEditOK...\n");
	int iMain, iSub, iIndex = GetCurSel();
	m_pEditDlg->GetEditItem (iMain, iSub);
	CString strItem = m_pEditDlg->GetText();
	strItem.TrimLeft();
	strItem.TrimRight();
	DeleteString (iMain);

	// replacement code (back to the previous behaviour
	// manage empty string
	if (strItem.IsEmpty()) {
		AfxMessageBox(IDS_EMPTY_DBNAME, MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	// manage name validity
	if (!IsValidDatabaseName(strItem)) {
		AfxMessageBox(IDS_INVALID_DBNAME, MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	// We can add the string
	AddString (strItem);
	GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, 0, 0);
	return 0;
}

LONG CuDbmsDatabaseListBox::OnEditCancel (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDbmsDatabaseListBox::OnEditCancel...\n");
	int iIndex = GetCurSel();
	int iMain, iSub;
	m_pEditDlg->GetEditItem(iMain, iSub);
	CString strItem = m_pEditDlg->GetText();
	if (iMain != LB_ERR)
		DeleteString (iMain);
	if (iMain == iIndex)
	{
		if (iIndex < GetCount())
			SetCurSel(iIndex);
		else
			SetCurSel(GetCount() - 1);
	}
	GetParent()->SendMessage (WM_LAYOUTEDITDLG_CANCEL, 0, 0);
	return 0;
}

void CuDbmsDatabaseListBox::PreSubclassWindow() 
{
	if (!m_pEditDlg)
	{
		m_pEditDlg = new CuLayoutEditDlg2 (this);
		m_pEditDlg->Create (IDD_LAYOUTCTRLEDIT2, this);
		m_pEditDlg->SetExpressionDialog(FALSE);
	}
	CListBox::PreSubclassWindow();
}

void CuDbmsDatabaseListBox::SetEditText (int iIndex, LPCTSTR lpszText)
{
	if (iIndex == -1)
	{
		if (m_pEditDlg && m_pEditDlg->IsWindowVisible())
		{
			m_pEditDlg->SetFocus(); 
			return;
		}
		iIndex = AddString(_T(""));
		if (iIndex == LB_ERR)
			return;
		SetCurSel (iIndex);
		RECT r;
		GetItemRect (iIndex, &r);
		r.top    -= 2;
		r.bottom += 4;
		m_pEditDlg->MoveWindow (&r);
		m_pEditDlg->SetData (_T(""), iIndex, 0);
		m_pEditDlg->ShowWindow (SW_SHOW);
	}
	else
	{
		RECT r;
		CString strItem;
		GetItemRect (iIndex, &r);
		r.top    -= 2;
		r.bottom += 4;
		m_pEditDlg->MoveWindow (&r);
		GetText (iIndex, strItem);
		m_pEditDlg->SetData ((LPCTSTR)strItem, iIndex, 0);
		m_pEditDlg->ShowWindow (SW_SHOW);
	}
}

void CuDbmsDatabaseListBox::OnSelchange() 
{
	if (m_pEditDlg && m_pEditDlg->IsWindowVisible())
	{
		CString strItem = m_pEditDlg->GetText();
		strItem.TrimLeft();
		strItem.TrimRight();
		if (strItem.IsEmpty())
			m_pEditDlg->OnCancel();
		else
			m_pEditDlg->OnOK();
		m_pEditDlg->ShowWindow (SW_HIDE);
		return;
	}
	if (m_pEditDlg)
		m_pEditDlg->ShowWindow (SW_HIDE);
}

void CuDbmsDatabaseListBox::OnDestroy() 
{
	if (m_pEditDlg)
		m_pEditDlg->DestroyWindow ();
	CListBox::OnDestroy();
}
