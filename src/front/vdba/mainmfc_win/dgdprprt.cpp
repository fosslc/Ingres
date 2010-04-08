/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

// DgDPRpRt.cpp : implementation file
//
// 12-Dec-2001 (noifr01)
//   (sir 99596) removal of obsolete code and resources

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdprprt.h"
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


CuDlgDomPropReplicRegTbl::CuDlgDomPropReplicRegTbl(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropReplicRegTbl::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropReplicRegTbl)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropReplicRegTbl::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropReplicRegTbl)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropReplicRegTbl::AddRegTbl (CuNameWithOwner *pReplicRegTbl)
{
  ASSERT (pReplicRegTbl);
  BOOL  bSpecial = pReplicRegTbl->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pReplicRegTbl->GetStrName(), 0);

  // Owner
  if (!bSpecial)
    m_cListCtrl.SetItemText (index, 1, pReplicRegTbl->GetOwnerName());

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pReplicRegTbl);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropReplicRegTbl, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropReplicRegTbl)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropReplicRegTbl message handlers

void CuDlgDomPropReplicRegTbl::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropReplicRegTbl::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  lstrcpy(strHeader[1], VDBA_MfcResourceString(IDS_TC_OWNER));//_T("Owner")
  int i, hWidth   [LAYOUT_NUMBER] = {120 + 16, 120};   // + 16 for image

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
  m_ImageList.AddIcon(IDI_TREE_STATIC_TABLE);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropReplicRegTbl::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropReplicRegTbl::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropReplicRegTbl::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_SYSTEMOBJECTS:
      //case FILTER_DOM_BASEOWNER:
    case FILTER_DOM_OTHEROWNER:
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
  // Get list of RegTbl for the replication
  //
  m_Data.m_uaReplicRegTbl.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  aparentsTemp[0] = lpRecord->extra;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  // Emb Feb.2, 97: ensure tables preloaded in cache, otherwise system error may occur
  // taken from "FNN 080295" modification in domdisp
  int iloc;
  DOMGetObjectLimitedInfo(nNodeHandle, (unsigned char *)"ii_tables",
                          (unsigned char *)"$ingres", OT_TABLE, 1,
                          aparentsTemp, TRUE, &iloc, buf, bufOwner, bufComplim);

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_REPLIC_REGTABLE,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            (LPUCHAR)pUps->pSFilter->lpOtherOwner, // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameWithOwner errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));//"<Data Unavailable>"
    m_Data.m_uaReplicRegTbl.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      CuNameWithOwner item((const char*)buf, (const char*)bufOwner);
      m_Data.m_uaReplicRegTbl.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaReplicRegTbl.GetCount() == 0) {
    CuNameWithOwner noItem(VDBA_MfcResourceString (IDS_NO_REGISTERED_TABLE));//"<No Registered Table>"
    m_Data.m_uaReplicRegTbl.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropReplicRegTbl::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataReplicRegTbl") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataReplicRegTbl* pData = (CuDomPropDataReplicRegTbl*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataReplicRegTbl)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropReplicRegTbl::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataReplicRegTbl* pData = new CuDomPropDataReplicRegTbl(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropReplicRegTbl::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaReplicRegTbl.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddRegTbl(m_Data.m_uaReplicRegTbl[cnt]);
}

void CuDlgDomPropReplicRegTbl::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgDomPropReplicRegTbl::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
