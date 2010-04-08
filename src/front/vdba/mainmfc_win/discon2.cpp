// Discon2.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "discon2.h"
#include "mainfrm.h"
#include "vnitem.h"

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
// CDlgDisconnServer dialog


CDlgDisconnServer::CDlgDisconnServer(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDisconnServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgDisconnServer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgDisconnServer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDisconnServer)
	DDX_Control(pDX, IDC_DISCONNECT_WINDOWSLIST, m_windowslist);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDisconnServer, CDialog)
	//{{AFX_MSG_MAP(CDlgDisconnServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnServer message handlers

void CDlgDisconnServer::OnOK() 
{
  BOOL bFullDisconnect  = TRUE;

  CString csFullNodeName = BuildFullNodeName(m_pServerDataMin);

  // Replication monitoring special management
//  if (IsNodeUsed4ExtReplMon((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE)) {
//    int ires = MessageBox("The current node is referenced by Windows monitoring replication on other nodes."
//                          "In addition to closing the opened windows on the current node,"
//                          "Do you want to perform a full disconnect breaking the references?",
//                          "Disconnect",
//                          MB_ICONQUESTION | MB_YESNOCANCEL);
//    switch (ires) {
//      case IDCANCEL:
//        CDialog::OnCancel();
//        return;
//      case IDYES:
//        bFullDisconnect = TRUE;
//        break;
//      case IDNO:
//        bFullDisconnect = FALSE;
//        break;
//    }
//  }


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
    int hNode = GetVirtNodeHandle((LPUCHAR)(LPCTSTR)csFullNodeName);
//    CloseCachesExtReplicInfo((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE);// not with /gw for instance
    FreeNodeStructsForName((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE,TRUE);
    DBACloseNodeSessions  ((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE,TRUE);
//    FreeNodeStruct (hNode,FALSE);
  }

	CDialog::OnOK();
}

BOOL CDlgDisconnServer::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CString rsCaption;
  BOOL bSuccess = rsCaption.LoadString(IDS_DISCONNECTSVR_CAPTION);
  ASSERT (bSuccess);

  CString caption;
  CString csFullNodeName = BuildFullNodeName(m_pServerDataMin);
  caption.Format(rsCaption, (LPCTSTR)csFullNodeName);
  SetWindowText(caption);

  // fill list with opened windows
  int ires;
  WINDOWDATAMIN openwin;
  memset (&openwin, 0, sizeof (openwin));
  ires = GetFirstMonInfo (0,
                          OT_NODE_SERVERCLASS,
                          m_pServerDataMin,
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
