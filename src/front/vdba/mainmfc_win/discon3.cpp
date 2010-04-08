// Discon3.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "discon3.h"
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
// CDlgDisconnUser dialog


CDlgDisconnUser::CDlgDisconnUser(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDisconnUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgDisconnUser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgDisconnUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDisconnUser)
	DDX_Control(pDX, IDC_DISCONNECT_WINDOWSLIST, m_windowslist);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDisconnUser, CDialog)
	//{{AFX_MSG_MAP(CDlgDisconnUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnUser message handlers

void CDlgDisconnUser::OnOK() 
{
  BOOL bFullDisconnect  = TRUE;

  CString csFullNodeName = BuildFullNodeName(m_pUserDataMin);

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
	UCHAR buf[200];
	lstrcpy((char *) buf,(char *)(LPCTSTR)csFullNodeName);
	BOOL bIsSpecial = ReplaceLocalWithRealNodeName(buf);
	if (isNodeWithUserUsedByReplMon((LPUCHAR)(LPCTSTR)csFullNodeName) ||
		(bIsSpecial && isNodeWithUserUsedByReplMon(buf))
	   ) {
			//"An underlying connection with this user, on this node, "
			//"is used to monitor Replication in a Monitor Window. "
			//"This connection can be removed only if you close the monitor window, "
			//"or disconnect from this node at the node level.", 
			BfxMessageBox(VDBA_MfcResourceString(IDS_E_MONITOR_WINDOWS), MB_OK | MB_ICONEXCLAMATION);
	}
	else {
//    CloseCachesExtReplicInfo((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE);// not with /gw for instance
		FreeNodeStructsForName((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE,FALSE);
	    DBACloseNodeSessions  ((LPUCHAR)(LPCTSTR)csFullNodeName,FALSE,FALSE);
	}
//    FreeNodeStruct (hNode,FALSE);
  }

	CDialog::OnOK();
}

BOOL CDlgDisconnUser::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CString rsCaption;
  BOOL bSuccess = rsCaption.LoadString(IDS_DISCONNECTSVR_CAPTION);
  ASSERT (bSuccess);

  CString caption;
  CString csFullNodeName = BuildFullNodeName(m_pUserDataMin);
  caption.Format(rsCaption, (LPCTSTR)csFullNodeName);
  SetWindowText(caption);

  // fill list with opened windows
  int ires;
  WINDOWDATAMIN openwin;
  memset (&openwin, 0, sizeof (openwin));
  ires = GetFirstMonInfo (0,
                          OT_USER,
                          m_pUserDataMin,
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
