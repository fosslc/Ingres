/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/* 
**    Source   : selogdlg.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**               Blattes Emmanuel (Custom Implemenations)
**    Purpose  : Modeless Dialog, Page (Log File) of Security 
**               See the CONCEPT.DOC for more detail
**
**  History:
**	xx-Sep-1997 (uk$so01)
**	    created
**	06-Jun-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 16-Mar-2005 (shaha03)
**	  BUG #114089, Display the confirmation check for any tab switch in security, 
**	  if it is edited.Removed security type check.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "selogdlg.h"
#include "crightdg.h"
#include "cbfitem.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define LAYOUT_NUMBER	2
/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityLogFile dialog

CuDlgSecurityLogFile::CuDlgSecurityLogFile(CWnd* pParent)
	: CDialog(CuDlgSecurityLogFile::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSecurityLogFile)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgSecurityLogFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSecurityLogFile)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSecurityLogFile, CDialog)
	//{{AFX_MSG_MAP(CuDlgSecurityLogFile)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnButton4)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnButton5)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CuDlgSecurityLogFile message handlers

void CuDlgSecurityLogFile::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

static void DestroyItemData (CuEditableListCtrlAuditLog* pList)
{
	AUDITLOGINFO* lData = NULL;
	int i, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		lData = (AUDITLOGINFO*)pList->GetItemData (i);	
		if (lData)
			delete lData;
	}

}

void CuDlgSecurityLogFile::OnDestroy() 
{
	DestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}


void CuDlgSecurityLogFile::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}


BOOL CuDlgSecurityLogFile::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);

	//
	// Initalize the Column Header of CListCtrl
	//
	LV_COLUMN lvcolumn;
	UINT strHeaderID[LAYOUT_NUMBER] = { IDS_COL_CROSSHATCH, IDS_COL_AUDIT_LOG};
	int       i, hWidth   [LAYOUT_NUMBER]     = {50, 300};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		m_ListCtrl.InsertColumn (i, &lvcolumn); 
	}
	return TRUE;
}



LONG CuDlgSecurityLogFile::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);

	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_EDIT_VALUE);
	pButton1->SetWindowText (csButtonTitle);

	csButtonTitle.LoadString(IDS_BUTTON_RESTORE);
	pButton2->SetWindowText (csButtonTitle);

	pButton1->ShowWindow (SW_SHOW);
	pButton2->ShowWindow (SW_SHOW);
	//
	// First of all:
	// manage potential change between left group and right group of property pages
	//
		CuSECUREItemData *pSecureData = (CuSECUREItemData*)lParam;
		ASSERT (pSecureData);
		// terminate old page family
		pSecureData->LowLevelDeactivationWithCheck();
		// set page family
		pSecureData->SetSpecialType(GEN_FORM_SECURE_C2);
		// Initiate new page family
		BOOL bOk = pSecureData->LowLevelActivationWithCheck();
		ASSERT (bOk);
	
	try 
	{
		//
		// Do something low-level: Retrieve data and display it.
		int  iCur, iIndex    = 0;
		BOOL bContinue = TRUE;
		AUDITLOGINFO  data;
		AUDITLOGINFO* pData;
		CString strItemMain;

		DestroyItemData (&m_ListCtrl);
		m_ListCtrl.DeleteAllItems();
		memset (&data, 0, sizeof (data));

		bContinue = VCBFllGetFirstAuditLogLine(&data);
		while (bContinue)
		{
			strItemMain.Format (_T("%03d"), data.log_n);
			iCur = m_ListCtrl.InsertItem (iIndex,    (LPTSTR)(LPCTSTR)strItemMain);
			if (iCur != -1)
			{
				m_ListCtrl.SetItemText(iIndex, 1, (LPTSTR)(LPCTSTR)data.szvalue);
				pData = new AUDITLOGINFO;
				memcpy (pData, &data, sizeof(AUDITLOGINFO));
				m_ListCtrl.SetItemData (iCur, (DWORD_PTR)pData);
			}
			bContinue = VCBFllGetNextAuditLogLine(&data);
			iIndex++;
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgSecurityLogFile::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("CuDlgSecurityLogFile::OnUpdateData has caught exception... \n");
	}
	return 0L;
}

LONG CuDlgSecurityLogFile::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_ListCtrl.HideProperty();
	return 0L;
}

LONG CuDlgSecurityLogFile::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgSecurityLogFile::OnButton1...\n");
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 1;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_ListCtrl.EditValue (iIndex, iNameCol, rCell);
	}
	return 0;
}


LONG CuDlgSecurityLogFile::OnButton2(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgSecurityLogFile::OnButton2...\n");
	UINT nState = 0;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.HideProperty();
	AUDITLOGINFO* pData = (AUDITLOGINFO*)m_ListCtrl.GetItemData(iIndex);
	if (pData && VCBFllResetAuditValue (pData))
	{
		m_ListCtrl.SetItemText(iIndex, 1, (LPTSTR)(LPCTSTR)pData->szvalue);
	}
	return 0;
}


LONG CuDlgSecurityLogFile::OnButton3(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgSecurityLogFile::OnButton3...\n");
	return 0;
}

LONG CuDlgSecurityLogFile::OnButton4(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgSecurityLogFile::OnButton4...\n");
	return 0;
}

LONG CuDlgSecurityLogFile::OnButton5(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgSecurityLogFile::OnButton5...\n");
	return 0;
}





//
// Editable List Control (Special Code)
//
/////////////////////////////////////////////////////////////////////////////
CuEditableListCtrlAuditLog::CuEditableListCtrlAuditLog()
	:CuEditableListCtrl()
{
}

CuEditableListCtrlAuditLog::~CuEditableListCtrlAuditLog()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlAuditLog, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlAuditLog)
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK, OnEditDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlAuditLog message handlers

LONG CuEditableListCtrlAuditLog::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;

	AUDITLOGINFO* pData = (AUDITLOGINFO*)GetItemData (iItem);
	if (pData)
	{
		int iOldNumber = pData->log_n;
		VCBFllAuditLogOnEdit (pData, (char*)(LPTSTR)(LPCTSTR)s);
		ASSERT (iOldNumber == pData->log_n);
		SetItemText (iItem, iSubItem, (LPTSTR)pData->szvalue);
	}
	return 0L;
}



void CuEditableListCtrlAuditLog::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber = 1;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlAuditLog::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}


void CuEditableListCtrlAuditLog::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	AUDITLOGINFO* lData = (AUDITLOGINFO*)GetItemData(iItem);
	if (!lData)
		return;
	SetEditText   (iItem, iSubItem, rcCell);
}




void CuDlgSecurityLogFile::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Recallculate)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}
