// DgDpScPr.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpscpr.h"
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


CuDlgDomPropSchemaProc::CuDlgDomPropSchemaProc(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropSchemaProc::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropSchemaProc)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropSchemaProc::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropSchemaProc)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropSchemaProc::AddProc (CuNameWithOwner *pSchemaProc)
{
  ASSERT (pSchemaProc);
  BOOL  bSpecial = pSchemaProc->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - image index always 0
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pSchemaProc->GetStrName(), 0);

  // Owner
  if (!bSpecial)
    m_cListCtrl.SetItemText (index, 1, pSchemaProc->GetOwnerName());

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pSchemaProc);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropSchemaProc, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropSchemaProc)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSchemaProc message handlers

void CuDlgDomPropSchemaProc::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropSchemaProc::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  lstrcpy(strHeader[1], VDBA_MfcResourceString(IDS_TC_OWNER));//_T("Owner")
  int i, hWidth   [LAYOUT_NUMBER] = {120, 120};

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
  m_ImageList.AddIcon(IDI_TREE_STATIC_PROCEDURE);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropSchemaProc::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropSchemaProc::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropSchemaProc::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_SYSTEMOBJECTS:    // eligible
      //case FILTER_DOM_BASEOWNER:
    case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_PROCEDURE)
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
  // Get list of Proc for the replication
  //
  m_Data.m_uaSchemaProc.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  aparentsTemp[0] = lpRecord->extra;
  aparentsTemp[1] = lpRecord->objName;
  aparentsTemp[2] = NULL;

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_SCHEMAUSER_PROCEDURE,
                            2,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            (LPUCHAR)pUps->pSFilter->lpOtherOwner, // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameWithOwner errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));//"<Data Unavailable>"
    m_Data.m_uaSchemaProc.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      CuNameWithOwner item((const char*)buf, (const char*)bufOwner);
      m_Data.m_uaSchemaProc.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaSchemaProc.GetCount() == 0) {
    CuNameWithOwner noItem(VDBA_MfcResourceString (IDS_E_NO_PROCEDURE));//"<No Procedure>"
    m_Data.m_uaSchemaProc.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropSchemaProc::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataSchemaProc") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataSchemaProc* pData = (CuDomPropDataSchemaProc*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataSchemaProc)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropSchemaProc::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataSchemaProc* pData = new CuDomPropDataSchemaProc(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropSchemaProc::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaSchemaProc.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddProc(m_Data.m_uaSchemaProc[cnt]);
}

void CuDlgDomPropSchemaProc::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuNameWithOwner::CompareNames, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuNameWithOwner::CompareOwners, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 2);
      break;
  }
  *pResult = 0;
}

void CuDlgDomPropSchemaProc::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}