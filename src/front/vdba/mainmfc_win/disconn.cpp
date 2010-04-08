// Disconn.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "disconn.h"
#include "mainfrm.h"

extern "C" {
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnect dialog


CDlgDisconnect::CDlgDisconnect(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDisconnect::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgDisconnect)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgDisconnect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDisconnect)
	DDX_Control(pDX, IDC_DISCONNECT_WINDOWSLIST, m_windowslist);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDisconnect, CDialog)
	//{{AFX_MSG_MAP(CDlgDisconnect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnect message handlers

void CDlgDisconnect::OnOK() 
{
  BOOL bFullDisconnect  = TRUE;

  // Replication monitoring special management
  if (IsNodeUsed4ExtReplMon(m_pNodeDataMin->NodeName)) {
    //"The current node is referenced by Windows monitoring replication on other nodes."
    //"In addition to closing the opened windows on the current node,"
    //"Do you want to perform a full disconnect breaking the references?",
    //IDS_E_TITLE_NODE = "Disconnect"
    int ires = MessageBox(VDBA_MfcResourceString(IDS_E_NODE_REFERENCED),VDBA_MfcResourceString(IDS_E_TITLE_NODE), MB_ICONQUESTION | MB_YESNOCANCEL);
    switch (ires) {
      case IDCANCEL:
        CDialog::OnCancel();
        return;
      case IDYES:
        bFullDisconnect = TRUE;
        break;
      case IDNO:
        bFullDisconnect = FALSE;
        break;
    }
  }

  int cpt;
  int count = m_windowslist.GetCount();
  ForgetDelayedUpdates();
  for (cpt = 0; cpt < count; cpt++) {
    HWND hWnd = (HWND)m_windowslist.GetItemData(cpt);
    CWnd *pWnd;
    pWnd = CWnd::FromHandlePermanent(hWnd);
    ASSERT (pWnd);
    CMDIChildWnd *pMdiChild = (CMDIChildWnd *)pWnd;
    pMdiChild->MDIDestroy();
  }
  AcceptDelayedUpdates();

  if (bFullDisconnect) {
    CWaitCursor hourglass;
//    int hNode = GetVirtNodeHandle(m_pNodeDataMin->NodeName);
    CloseCachesExtReplicInfo(m_pNodeDataMin->NodeName);
    FreeNodeStructsForName(m_pNodeDataMin->NodeName,TRUE,TRUE);
	DBACloseNodeSessions(m_pNodeDataMin->NodeName,TRUE,TRUE);

  }

	CDialog::OnOK();
}

BOOL CDlgDisconnect::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CString rsCaption;
  BOOL bSuccess = rsCaption.LoadString(IDS_DISCONNECT_CAPTION);
  ASSERT (bSuccess);

  CString caption;
  caption.Format(rsCaption, m_pNodeDataMin->NodeName);
  SetWindowText(caption);

  // fill list with opened windows
  int ires;
  WINDOWDATAMIN openwin;
  memset (&openwin, 0, sizeof (openwin));
  ires = GetFirstMonInfo (0,
                          OT_NODE,
                          m_pNodeDataMin,
                          OT_NODE_OPENWIN,
                          (void *)&openwin,
                          NULL);
  while (ires == RES_SUCCESS) {
    CString docName = openwin.Title;
    int index = m_windowslist.AddString(docName);
    m_windowslist.SetItemData(index, (DWORD)openwin.hwnd);
    ires = GetNextMonInfo((void *)&openwin);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
