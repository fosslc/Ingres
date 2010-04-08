// Toolbars.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"
#include "toolbars.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Utility functions

BOOL HasToolbar(CMDIChildWnd *pCurFrame)
{
  return (BOOL)pCurFrame->SendMessage(WM_USER_HASTOOLBAR, 0, 0);
}

BOOL IsToolbarVisible(CMDIChildWnd *pCurFrame)
{
  return (BOOL)pCurFrame->SendMessage(WM_USER_ISTOOLBARVISIBLE, 0, 0);
}

BOOL SetToolbarVisible(CMDIChildWnd *pCurFrame, BOOL bUpdateImmediate)
{
  return (BOOL)pCurFrame->SendMessage(WM_USER_SETTOOLBARSTATE, TRUE, bUpdateImmediate);
}

BOOL SetToolbarInvisible(CMDIChildWnd *pCurFrame, BOOL bUpdateImmediate)
{
  return (BOOL)pCurFrame->SendMessage(WM_USER_SETTOOLBARSTATE, FALSE, bUpdateImmediate);
}

BOOL SetToolbarCaption(CMDIChildWnd *pCurFrame, LPCSTR caption)
{
  return (BOOL)pCurFrame->SendMessage(WM_USER_SETTOOLBARCAPTION, 0, (LPARAM)caption);
}

/////////////////////////////////////////////////////////////////////////////
// CDlgToolbars dialog


CDlgToolbars::CDlgToolbars(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgToolbars::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgToolbars)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgToolbars::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgToolbars)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgToolbars, CDialog)
	//{{AFX_MSG_MAP(CDlgToolbars)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgToolbars message handlers

BOOL CDlgToolbars::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  VERIFY (m_CheckListBox.SubclassDlgItem (IDC_TOOLBARS_LIST, this));

  int index;
  m_CheckListBox.ResetContent();

  // global toolbar
  index = m_CheckListBox.AddString ("Main Toolbar");
  m_CheckListBox.SetItemData(index, 0);             // null data means main toolbar
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  if (pMain->GetPMainToolBar()->IsVisible())
    m_CheckListBox.SetCheck(index, 1);

  // other toolbars (opened documents except node doc in mdi mode
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CMDIChildWnd *pActiveMDI = pMainFrame->MDIGetActive();
  if (pActiveMDI) {
    CWnd *pMDIClient = pActiveMDI->GetParent();
    ASSERT (pMDIClient);
    
    // get first document handle (with loop to skip the icon title windows)
    CWnd *pCurDoc = pMDIClient->GetWindow(GW_CHILD);
    while (pCurDoc && pCurDoc->GetWindow(GW_OWNER))
      pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
    
    while (pCurDoc) {
      CMDIChildWnd *pCurFrame = (CMDIChildWnd *)FromHandlePermanent(pCurDoc->m_hWnd);
      ASSERT (pCurFrame);
      if (pCurFrame) {
        // Manage document
        if (HasToolbar(pCurFrame)) {
          // Add line in tree
          CDocument* pTheDoc = pCurFrame->GetActiveDocument();
          ASSERT (pTheDoc);
    
          index = m_CheckListBox.AddString(pTheDoc->GetTitle());
          m_CheckListBox.SetItemData(index, (DWORD)pCurFrame);
    
          // manage checkbox state
          if (IsToolbarVisible(pCurFrame))
            m_CheckListBox.SetCheck(index, 1);
        }
      }
    
      // get next document handle (with loop to skip the icon title windows)
      pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
      while (pCurDoc && pCurDoc->GetWindow(GW_OWNER))
        pCurDoc = pCurDoc->GetWindow(GW_HWNDNEXT);
    }
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgToolbars::OnOK()
{
  int index;

  // global toolbar
  index = 0;
  ASSERT (m_CheckListBox.GetItemData(index) == 0);
  CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
  if (m_CheckListBox.GetCheck(index))
    pMain->ShowControlBar(pMain->GetPMainToolBar(), TRUE, TRUE);
  else
    pMain->ShowControlBar(pMain->GetPMainToolBar(), FALSE, TRUE);  // hides

  // other toolbars (opened documents except node doc in mdi mode
  for (index = 1; index < m_CheckListBox.GetCount(); index++) {
    CMDIChildWnd *pCurFrame = (CMDIChildWnd *)m_CheckListBox.GetItemData(index);
    ASSERT (pCurFrame);
    if (m_CheckListBox.GetCheck(index))
      SetToolbarVisible(pCurFrame);
    else
      SetToolbarInvisible(pCurFrame);
  }

	CDialog::OnOK();
}

