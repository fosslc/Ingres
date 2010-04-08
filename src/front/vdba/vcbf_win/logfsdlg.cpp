/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : logfsdlg.cpp, Implementation File
**
**  Project  : OpenIngres Configuration Manager
**
**  Author   : Blattes Emmanuel
**
**  Purpose  : Modeless Dialog, Page (Secondary Log) of Transaction Log
**		See the CONCEPT.DOC for more detail
**
**  History  :
**	22-nov-98 (cucjo01)
**	    Added multiple transaction log file support
**	    Added animations during creation of log files
**	02-dec-98 (cucjo01)
**	    Added progress bar for creating log files
**	18-dec-98 (cucjo01)
**	    Added warning message if transactions are pending and user tries
**	    to erase the transaction log
**	21-dec-98 (cucjo01)
**	    Added transaction pending warnings to all buttons
**	    Added transaction log status bitmap next to on-line / off-line text
**	04-feb-2000 (somsa01)
**	    Due to problems building on HP, changed second definition of
**	    OnDestroy() to be called OnDestroyLog().
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
** 03-Jun-2003 (uk$so01)
**    SIR #110344, Show the Raw Log information on the Unix Operating System.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 19-May-2004 (lakvi01)
**	  BUG #112292, Made the GUI display the physical log size. Also does not display when there are 
**    unequal log-files. 
*/

#include "stdafx.h"
#include "vcbf.h"
#include "logfsdlg.h"
#include "wmusrmsg.h"
#include "crightdg.h"
#include "cbfitem.h"
#include "dlglogsz.h"
#include "conffrm.h"
#include "waitdlg.h"
#include "waitdlgprogress.h"

extern "C"
{
#include "compat.h"
#include "cv.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static LPVOID ptrClass;
static void WaitDlg_StepProgressBar();
static void WaitDlg_SetText(LPCTSTR szText);

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogfileSecondary dialog
IMPLEMENT_DYNCREATE(CuDlgLogfileSecondary, CFormView)

CuDlgLogfileSecondary::CuDlgLogfileSecondary()
	: CFormView(CuDlgLogfileSecondary::IDD)
{
	m_pLogFileData = NULL;
	m_TimerID = 0;

	//{{AFX_DATA_INIT(CuDlgLogfileSecondary)
	//}}AFX_DATA_INIT
}


void CuDlgLogfileSecondary::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgLogfileSecondary)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckRaw);
	DDX_Control(pDX, IDC_STATUS_BITMAP, m_StatusBitmap);
	DDX_Control(pDX, IDC_ENABLED, m_CheckEnabled);
	DDX_Control(pDX, IDC_FILE, m_EditPath);
	DDX_Control(pDX, IDC_SIZEINK, m_EditSize);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_StaticState);
	DDX_Control(pDX, IDC_FILENAME_LIST, m_FileNameList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgLogfileSecondary, CFormView)
	//{{AFX_MSG_MAP(CuDlgLogfileSecondary)
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_LBN_SELCHANGE(IDC_FILENAME_LIST, OnSelchangeFilenameList)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnReformat)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnDisable)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnErase)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnDestroyLog)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnCreate)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

#ifdef _DEBUG
void CuDlgLogfileSecondary::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgLogfileSecondary::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogfileSecondary message handlers

void CuDlgLogfileSecondary::OnDestroy() 
{
	if (m_TimerID != 0)
		KillTimer(m_TimerID);
	m_TimerID = 0;
	CFormView::OnDestroy();
}

LONG CuDlgLogfileSecondary::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  //
  // Preliminary: store ItemData for future use in buttons handlers
  //
  // Fix Oct 27, 97: lParam is NULL when OnUpdateData() called
  // after activation of "Preferences" tag followed by activation of "Configure" tag
  if (lParam)
  {
    CuLOGFILEItemData *pLogfileData = (CuLOGFILEItemData*)lParam;
    m_pLogFileData = pLogfileData;    // must always be done! (example: following selection change on left side)
  }
  else
    ASSERT (m_pLogFileData);    // check contained something valid
#if defined (MAINWIN)
	m_cCheckRaw.ShowWindow (SW_SHOW);
#endif

    // 1) Make buttons visibles with the "good" texts in them
    CWnd* pParent1 = GetParent ();				// CuDlgViewFrame
    ASSERT_VALID (pParent1);
    CWnd* pParent2 = pParent1->GetParent ();	// CTabCtrl
    ASSERT_VALID (pParent2);
    CWnd* pParent3 = pParent2->GetParent ();	// CConfRightDlg
    ASSERT_VALID (pParent3);

    CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON1);
    CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON2);
    CButton* pButton3 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON3);
    CButton* pButton4 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON4);
    CButton* pButton5 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON5);
    
    CString csButtonTitle;
    csButtonTitle.LoadString(IDS_BUTTON_REFORMAT);
    pButton1->SetWindowText (csButtonTitle);
    csButtonTitle.LoadString(IDS_BUTTON_DISABLE);
    pButton2->SetWindowText (csButtonTitle);
    csButtonTitle.LoadString(IDS_BUTTON_ERASE);
    pButton3->SetWindowText (csButtonTitle);
    csButtonTitle.LoadString(IDS_BUTTON_DESTROY);
    pButton4->SetWindowText (csButtonTitle);
    csButtonTitle.LoadString(IDS_BUTTON_CREATE);
    pButton5->SetWindowText (csButtonTitle);
    
    pButton1->ShowWindow(SW_SHOW);
    pButton2->ShowWindow(SW_SHOW);
    pButton3->ShowWindow(SW_SHOW);
    pButton4->ShowWindow(SW_SHOW);
    pButton5->ShowWindow(SW_SHOW);
	
    try
	{
		// 2) Call low-level function OnActivateSecondaryField()
		BOOL bDontCare = VCBFll_tx_OnActivateSecondaryField(&m_pLogFileData->m_transactionInfo);
		
		// 3) update buttons and controls states
		UpdateButtonsStates(FALSE);
		UpdateControlsStates();
	}
	catch (CeVcbfException e)   // Catch critical error
	{    
		TRACE1 ("CuDlgLogfileSecondary::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}	
	return 0L;
}

void CuDlgLogfileSecondary::UpdateButtonsStates(BOOL bUpdateLeftPane)
{
    //
    // Enable/Disable buttons according to what's in the structure
    //
    ASSERT (m_pLogFileData);
    LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;

    CWnd* pParent1 = GetParent ();				// CuDlgViewFrame
    ASSERT_VALID (pParent1);
    CWnd* pParent2 = pParent1->GetParent ();	// CTabCtrl
    ASSERT_VALID (pParent2);
    CWnd* pParent3 = pParent2->GetParent ();	// CConfRightDlg
    ASSERT_VALID (pParent3);

	if (bUpdateLeftPane)
    {
       CWnd* pParent4 = pParent3->GetParent ();    // CConfViewRight
       ASSERT_VALID (pParent4);
       CWnd* pParent5 = pParent4->GetParent ();    // CSplitterWnd
       ASSERT_VALID (pParent5);
       CWnd* pParent6 = pParent5->GetParent ();    // CConfigFrame
       ASSERT_VALID (pParent6);
       CConfigFrame* pFrm = (CConfigFrame*) pParent6;
       CConfLeftDlg* pLeftDlg = pFrm->GetCConfLeftDlg();
		
       HTREEITEM  hti = pLeftDlg->m_tree_ctrl.GetSelectedItem();
       if (!hti)
         return;

       CuCbfListViewItem* pData = NULL;
       pData = (CuCbfListViewItem*)pLeftDlg->m_tree_ctrl.GetItemData (hti);

       ASSERT(pData);

       BOOL bModified = pData->LowLevelDeactivationWithCheck();
	   bModified = pData->LowLevelActivationWithCheck();

       try
       {
    	  BOOL bDontCare = VCBFll_tx_OnActivateSecondaryField(lpS);
       }
	   catch (CeVcbfException e)
	   {
	     TRACE1 ("VCBFll_tx_OnActivateSecondaryField has caught exception: %s\n", e.m_strReason);
	     CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
	     pMain->CloseApplication (FALSE);
	   }
	   catch (...)
	   {
	     TRACE0 ("Other error occured ...\n");
	   }

	}  // if (bUpdateLeftPane) ...
    
    CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON1);
    CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON2);
    CButton* pButton3 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON3);
    CButton* pButton4 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON4);
    CButton* pButton5 = (CButton*)((CConfRightDlg*)pParent3)->GetDlgItem (IDC_BUTTON5);
    
    pButton1->EnableWindow(lpS->bEnableReformat);
    pButton2->EnableWindow(lpS->bEnableDisable );
    pButton3->EnableWindow(lpS->bEnableErase   );
    pButton4->EnableWindow(lpS->bEnableDestroy );
    pButton5->EnableWindow(lpS->bEnableCreate  );
 
}

void CuDlgLogfileSecondary::UpdateControlsStates()
{ int rc, loop; 
  CString strText;
  CBitmap theBmp;
  
  ASSERT (m_pLogFileData);
  LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;

  m_CheckEnabled.EnableWindow(FALSE);
  if (_tcsicmp(lpS->szraw2, _T("yes")) == 0)
    m_cCheckRaw.SetCheck (1);
  else
    m_cCheckRaw.SetCheck (0);

  m_StaticState.SetWindowText((LPCTSTR)lpS->szstate);
  OnSelchangeFilenameList();
  
  for (loop = 0; loop < LG_MAX_FILE; loop++)
  { if (_tcscmp((LPCTSTR)lpS->szRootPaths2[loop], _T("")))
    { rc = m_FileNameList.GetTextLen(loop);
      if (rc == LB_ERR)   // Doesn't exist  /// move common str to bottom
      { m_FileNameList.InsertString(-1, (LPCTSTR)lpS->szRootPaths2[loop]);
      }
      else
      { m_FileNameList.GetText(loop, strText);
        if (strText.Compare((LPCTSTR)lpS->szRootPaths2[loop]) != 0)
        {  m_FileNameList.DeleteString(loop);
           m_FileNameList.InsertString(loop, (LPCTSTR)lpS->szRootPaths2[loop]);
        }
      }
    }
    else
      m_FileNameList.DeleteString(loop);
  } 

  if ((_tcsicmp((LPCTSTR)lpS->szenabled2, _T("yes")) == 0) && (lpS->bLogFilesizesEqual2))

  { m_CheckEnabled.SetCheck(1);
    m_FileNameList.EnableWindow(TRUE);

    int iCount = m_FileNameList.GetCount();

    i4 iSize = 0;
	CVal(lpS->szsize2,&iSize);

    strText=_T("");
    if (iCount > 0)
    { if (iSize % iCount == 0)
        strText.Format(_T("%d"), iSize/iCount);
      else
        strText.Format(_T("%.2f"), (float)iSize/iCount);
    }
    m_EditSize.SetWindowText(strText);
    m_EditPath.SetWindowText((LPCTSTR)lpS->szConfigDatLogFileName2);
  }
  else
  { m_CheckEnabled.SetCheck(0);
    m_FileNameList.SetCurSel(-1);
	m_FileNameList.EnableWindow(FALSE);
    m_EditPath.SetWindowText(_T(""));
    m_EditSize.SetWindowText(_T(""));
  }	

  // Set Status Bitmap
  if (!_tcscmp((LPCTSTR)lpS->szstate, _T("On-Line"))) /// S_ST0424_on_line
     theBmp.LoadBitmap(IDB_GREEN);
  else if (!_tcscmp((LPCTSTR)lpS->szstate, _T("Off-Line"))) /// S_ST0425_off_line
     theBmp.LoadBitmap(IDB_RED);
  else
     theBmp.LoadBitmap(IDB_YELLOW);
  
  m_StatusBitmap.SetBitmap(theBmp);

}

LONG CuDlgLogfileSecondary::OnReformat (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgLogfileSecondary::OnReformat...\n");

	ASSERT (m_pLogFileData);
	if (!m_pLogFileData)
		return 0;

    if (!VerifyIfTransactionsPending())
       return 0;
        
	try
	{  CWaitDlg dlgWait(_T("Reformatting the Dual Transaction Log..."), IDR_TRANSLOG_AVI);
	   BOOL bDontCare = VCBFll_tx_OnReformat(FALSE, &m_pLogFileData->m_transactionInfo);
		
       m_FileNameList.ResetContent();
       for (int loop = 0; loop < LG_MAX_FILE; loop++)
       { if (_tcscmp((LPCTSTR)m_pLogFileData->m_transactionInfo.szRootPaths2[loop], _T("")))
            m_FileNameList.InsertString(-1, (LPCTSTR)m_pLogFileData->m_transactionInfo.szRootPaths2[loop]);
       }

       UpdateButtonsStates();
       UpdateControlsStates();
       dlgWait.DestroyWindow();
	}
	catch (CeVcbfException e)   // Catch critical error
	{    
		TRACE1 ("CuDlgLogfileSecondary::OnReformat has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0;
}

LONG CuDlgLogfileSecondary::OnDisable(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgLogfileSecondary::OnDisable...\n");

	ASSERT (m_pLogFileData);
	if (!m_pLogFileData)
       return 0;

    if (!VerifyIfTransactionsPending())
       return 0;
	
	try
	{
		BOOL bDontCare = VCBFll_tx_OnDisable(FALSE, &m_pLogFileData->m_transactionInfo);
		UpdateButtonsStates();
		UpdateControlsStates();
	}
	catch (CeVcbfException e)   // Catch critical error
	{    
		TRACE1 ("CuDlgLogfileSecondary::OnDisable has caught exception: %s\n", e.m_strReason);
        CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0;
}

LONG CuDlgLogfileSecondary::OnErase(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgLogfileSecondary::OnErase...\n");

	ASSERT (m_pLogFileData);
	if (!m_pLogFileData)
       return 0;

    if (!VerifyIfTransactionsPending())
       return 0;
        
	try
	{
		CWaitDlg dlgWait(_T("Erasing the Dual Transaction Log..."), IDR_DELETE_AVI);
		BOOL bDontCare = VCBFll_tx_OnErase(FALSE, &m_pLogFileData->m_transactionInfo);
		UpdateButtonsStates();
		UpdateControlsStates();
	    dlgWait.DestroyWindow();
	}
	catch (CeVcbfException e)   // Catch critical error
	{    
		TRACE1 ("CuDlgLogfileSecondary::OnErase has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}

	return 0;
}

LONG CuDlgLogfileSecondary::OnDestroyLog(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgLogfileSecondary::OnDestroyLog...\n");

	ASSERT (m_pLogFileData);
	if (!m_pLogFileData)
       return 0;

    if (!VerifyIfTransactionsPending())
       return 0;
    
    try
	{
		BOOL bDontCare = VCBFll_tx_OnDestroy(FALSE, &m_pLogFileData->m_transactionInfo);
		UpdateButtonsStates();
		UpdateControlsStates();
	}
	catch (CeVcbfException e)   // Catch critical error 
	{   
		TRACE1 ("CuDlgLogfileSecondary::OnDestroyLog has caught exception: %s\n", e.m_strReason);
        CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}

	return 0;
}

LONG CuDlgLogfileSecondary::OnCreate(WPARAM wParam, LPARAM lParam)
{ CString strSize;
  TCHAR szSize[100];

  TRACE0 ("CuDlgLogfileSecondary::OnCreate...\n");

  ASSERT (m_pLogFileData);

  if (!VerifyIfTransactionsPending())
     return 0;

  LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;
  
  CuDlgLogSize dlg;
  dlg.m_iMaxLogFiles = LG_MAX_FILE;
  dlg.m_bPrimary = FALSE;
  dlg.m_lpS = lpS;
  
  if (dlg.DoModal() != IDOK)
	 return 0;

  _stprintf(szSize, _T("%d"), dlg.m_size);
  
  try
  { 
     CWaitDlgProgress dlgWait(_T("Creating the Dual Transaction Log..."), IDR_TRANSLOG_AVI);
     ptrClass=(LPVOID)&dlgWait;
     lpS->fnUpdateGraph = WaitDlg_StepProgressBar;
     lpS->fnSetText = WaitDlg_SetText;

     AfxGetApp()->DoWaitCursor(1);
     BOOL bSuccess = VCBFll_tx_OnCreate(FALSE, lpS, szSize);
     AfxGetApp()->DoWaitCursor(-1);
     if (bSuccess)
     {
        m_FileNameList.ResetContent();
        for (int loop = 0; loop < LG_MAX_FILE; loop++)
        { if (_tcscmp((LPCTSTR)lpS->szRootPaths2[loop], _T("")))
             m_FileNameList.InsertString(-1, (LPCTSTR)lpS->szRootPaths2[loop]);
        }
  
        m_FileNameList.EnableWindow(TRUE);
        UpdateButtonsStates();
	    UpdateControlsStates();
	 }
 	 dlgWait.DestroyWindow();
  }
  catch (CeVcbfException e)   // Catch critical error
  {    
     TRACE1 ("CuDlgLogfileSecondary::OnCreate has caught exception: %s\n", e.m_strReason);
     CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
     pMain->CloseApplication (FALSE);
  }
  catch (CMemoryException* e)
  {
     VCBF_OutOfMemoryMessage ();
     e->Delete();
  }
  catch (...)
  {
     TRACE0 ("Other error occured ...\n");
  }

  return 0;
}

void CuDlgLogfileSecondary::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	TRACE1 ("CuDlgLogfileSecondary::OnShowWindow(%d)\n", bShow);

    // SetTreeCtrlItem("Dual Transaction Log");

	CFormView::OnShowWindow(bShow, nStatus);
	if (bShow)
	{
		if (m_TimerID == 0)
			m_TimerID = SetTimer (1, 20*1000, NULL);
	}
	else
	{
		if (m_TimerID != 0)
			KillTimer(m_TimerID);
		m_TimerID = 0;
	}
}

void CuDlgLogfileSecondary::OnTimer(UINT nIDEvent) 
{
	LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;
	CWaitCursor GlassHour;
	try
	{
		BOOL bSuccess = VCBFll_tx_OnTimer(lpS);
		UpdateButtonsStates(FALSE);
		UpdateControlsStates();
	}
	catch (CeVcbfException e)   // Catch critical error
	{    
		TRACE1 ("CuDlgLogfileSecondary::OnTimer has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	CFormView::OnTimer(nIDEvent);
}

void CuDlgLogfileSecondary::OnSelchangeFilenameList()
{
#if 0
// We only show the prefix name in the edit control
  int iSel = m_FileNameList.GetCurSel();

  if (iSel >= 0)
  { LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;
    m_EditPath.SetWindowText(lpS->szLogFileNames2[iSel]);
  }
  else
    m_EditPath.SetWindowText(_T(""));
#endif
}

void CuDlgLogfileSecondary::SetTreeCtrlItem(LPCTSTR szItemText)
{ HTREEITEM hNewItem;
  CWnd* pParent1 = GetParent();              // CuDlgViewFrame
  ASSERT_VALID (pParent1);
  CWnd* pParent2 = pParent1->GetParent();    // CTabCtrl
  ASSERT_VALID (pParent2);
  CWnd* pParent3 = pParent2->GetParent();    // CConfRightDlg
  ASSERT_VALID (pParent3);
  CWnd* pParent4 = pParent3->GetParent();    // CConfViewRight
  ASSERT_VALID (pParent4);
  CWnd* pParent5 = pParent4->GetParent();    // CSplitterWnd
  ASSERT_VALID (pParent5);
  CWnd* pParent6 = pParent5->GetParent();    // CConfigFrame
  ASSERT_VALID (pParent6);
  CConfigFrame* pFrm = (CConfigFrame*) pParent6;
  CConfLeftDlg* pLeftDlg = pFrm->GetCConfLeftDlg();

  if (!pLeftDlg)
    return;

  hNewItem = pLeftDlg->m_tree_ctrl.GetChildItem(pLeftDlg->m_tree_ctrl.GetRootItem());
  while (hNewItem)
  { CString strItem; 
    strItem = pLeftDlg->m_tree_ctrl.GetItemText(hNewItem);
	
	if (strItem.CompareNoCase(szItemText) == 0)
       pLeftDlg->m_tree_ctrl.Select(hNewItem,TVGN_CARET);
	
    hNewItem = pLeftDlg->m_tree_ctrl.GetNextSiblingItem(hNewItem);
  }

  return;
}

void WaitDlg_StepProgressBar()
{
   if(ptrClass)
     ((CWaitDlgProgress *)ptrClass)->StepProgressBar();
}

void WaitDlg_SetText(LPCTSTR szText)
{  if(ptrClass)
     ((CWaitDlgProgress *)ptrClass)->SetText(szText);
}

BOOL CuDlgLogfileSecondary::VerifyIfTransactionsPending()
{ if (!m_pLogFileData)
     return FALSE;

  if (!strcmp(m_pLogFileData->m_transactionInfo.szstate, "Off-Line / Transactions-Pending"))
  { MessageBeep(MB_ICONEXCLAMATION);  // S_ST0426_off_line_transactions
    int rc=AfxMessageBox(IDS_CONTINUE_AND_LOSE, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
	
    if (rc != IDYES)
       return FALSE;
  }

  return TRUE;
}

void CuDlgLogfileSecondary::OnPaint() 
{   CBitmap theBmp;

	CPaintDC dc(this); // device context for painting
	
    if (!m_pLogFileData)
         return;

    LPTRANSACTINFO lpS = &m_pLogFileData->m_transactionInfo;

    // Set Status Bitmap
    if (!_tcscmp((LPCTSTR)lpS->szstate, _T("On-Line"))) /// S_ST0424_on_line
       theBmp.LoadBitmap(IDB_GREEN);
    else if (!_tcscmp((LPCTSTR)lpS->szstate, _T("Off-Line"))) /// S_ST0425_off_line
       theBmp.LoadBitmap(IDB_RED);
    else
       theBmp.LoadBitmap(IDB_YELLOW);
  
    m_StatusBitmap.SetBitmap(theBmp);
	
	// Do not call CFormView::OnPaint() for painting messages
}
