// DgDPRpCo.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdprpco.h"
#include "vtree.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   2


CuDlgDomPropReplicConn::CuDlgDomPropReplicConn(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropReplicConn::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropReplicConn)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropReplicConn::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropReplicConn)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropReplicConn::AddConn (CuNameWithNumber *pReplicConn)
{
  ASSERT (pReplicConn);
  BOOL  bSpecial = pReplicConn->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Number
  CString csTmp;
  if (bSpecial)
    csTmp = _T("");
  else
    csTmp.Format(_T("%d"), pReplicConn->GetNumber());
  index  = m_cListCtrl.InsertItem (nCount, csTmp, 0);

  // Name
  m_cListCtrl.SetItemText (index, 1, (LPCTSTR)pReplicConn->GetStrName());

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pReplicConn);
}



BEGIN_MESSAGE_MAP(CuDlgDomPropReplicConn, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropReplicConn)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropReplicConn message handlers

void CuDlgDomPropReplicConn::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropReplicConn::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32] = 
  {
      _T("#"),
      _T(""),
  };
  lstrcpy(strHeader[1], VDBA_MfcResourceString(IDS_TC_VNODE));
  int i, hWidth   [LAYOUT_NUMBER] = {30 + 16, 200};   // + 16 for image

  memset (&lvcolumn, 0, sizeof (LV_COLUMN));
  lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
  for (i=0; i<LAYOUT_NUMBER; i++)
  {
      //CString strHeader;
      //strHeader.LoadString (strHeaderID[i]);
      lvcolumn.fmt = LVCFMT_LEFT;
      lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader[i];
      lvcolumn.iSubItem = i;
      lvcolumn.cx = hWidth[i];
      m_cListCtrl.InsertColumn (i, &lvcolumn); 
  }

  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_REPLIC_CONNECTION);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropReplicConn::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropReplicConn::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropReplicConn::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
  // cast received parameters
  int nNodeHandle = (int)wParam;
  LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
  ASSERT (nNodeHandle != -1);
  ASSERT (pUps);

  // ignore selected actions on filters
  switch (pUps->nIpmHint)
  {
    case 0:
      //case FILTER_DOM_SYSTEMOBJECTS:
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_REPLIC_CONNECTION)
        return 0L;
      break;
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListCtrl);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();

  //
  // Get list of Conn for the replication
  //
  m_Data.m_uaReplicConn.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  aparentsTemp[0] = lpRecord->extra;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_REPLIC_CONNECTION,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameWithNumber unavail(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));//_T("<Data Unavailable>")
    m_Data.m_uaReplicConn.Add(unavail);
  }
  else {
    while (iret == RES_SUCCESS) {
      int iNumber = (int)(long)getint(bufOwner);
      CString cs = buf;
      int idxSpace = cs.Find(_T(' '));
      ASSERT (idxSpace != -1);
      if (idxSpace != -1) {
        int len = cs.GetLength();
        CString cs2 = cs.Right(len - idxSpace);
        cs2.TrimLeft();
        ASSERT (!cs2.IsEmpty());
        cs = cs2;
      }
      CuNameWithNumber conn(cs, iNumber);
      m_Data.m_uaReplicConn.Add(conn);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaReplicConn.GetCount() == 0) {
    CuNameWithNumber noConn(VDBA_MfcResourceString (IDS_E_NO_CONNECTION));//"<No Connection>"
    m_Data.m_uaReplicConn.Add(noConn);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropReplicConn::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataReplicConn") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataReplicConn* pData = (CuDomPropDataReplicConn*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataReplicConn)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropReplicConn::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataReplicConn* pData = new CuDomPropDataReplicConn(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropReplicConn::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaReplicConn.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddConn(m_Data.m_uaReplicConn[cnt]);
}

void CuDlgDomPropReplicConn::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuNameWithNumber::CompareNumbers, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuNameWithNumber::CompareNames, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 2);
      break;
  }
  *pResult = 0;
}

void CuDlgDomPropReplicConn::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}