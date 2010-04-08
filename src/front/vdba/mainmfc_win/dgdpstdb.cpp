// dgdpstdb.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpstdb.h"
#include "vtree.h"
#include "domfast.h"

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


CuDlgDomPropStaticDb::CuDlgDomPropStaticDb(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropStaticDb::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropStaticDb)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropStaticDb::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropStaticDb)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropStaticDb::AddDatabase (CuNameWithOwner *pDatabase, int dbType)
{
  ASSERT (pDatabase);
  BOOL  bSpecial = pDatabase->IsSpecialItem();

  // calculate image index
  int imageIndex = 0;
  switch(dbType) {
    case DBTYPE_REGULAR:
      imageIndex = 0;
      break;
    case DBTYPE_DISTRIBUTED:
      imageIndex = 1;
      break;
    case DBTYPE_COORDINATOR:
      imageIndex = 2;
      break;
    default:
      ASSERT (FALSE);
      imageIndex = 0;
  }

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - last parameter is Image Index
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pDatabase->GetStrName(), imageIndex);

  // Owner
  if (!bSpecial)
    m_cListCtrl.SetItemText (index, 1, pDatabase->GetOwnerName());

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pDatabase);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropStaticDb, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropStaticDb)
    ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickMfcList1)
	ON_NOTIFY(NM_DBLCLK, IDC_MFC_LIST1, OnDblclkMfcList1)
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropStaticDb message handlers

void CuDlgDomPropStaticDb::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropStaticDb::OnInitDialog() 
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
  m_ImageList.AddIcon(IDI_TREE_STATIC_DATABASE);
  m_ImageList.AddIcon(IDI_TREE_STAR_DB_DDB);
  m_ImageList.AddIcon(IDI_TREE_STAR_DB_CDB);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropStaticDb::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropStaticDb::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropStaticDb::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_DATABASE)
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
  // Get list of Databases
  //
  m_Data.m_uaStaticDb.RemoveAll();
  m_Data.m_awDbType.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  // parent preparation
  aparentsTemp[0] = aparentsTemp[1] = aparentsTemp[2] = NULL;

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_DATABASE,
                            0,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            (LPUCHAR)pUps->pSFilter->lpBaseOwner, // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameWithOwner errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));//"<Data Unavailable>"
    m_Data.m_uaStaticDb.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      int databaseType = getint(bufComplim+STEPSMALLOBJ);

      // specific for star: a CDB is considered as system
      BOOL bFits = TRUE;
      if (!pUps->pSFilter->bWithSystem) {
        if (databaseType == DBTYPE_COORDINATOR)
          bFits = FALSE;
      }

      if (bFits) {
        CuNameWithOwner item((const char*)buf, (const char*)bufOwner);
        m_Data.m_uaStaticDb.Add(item);
        m_Data.m_awDbType.Add(databaseType);
      }

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaStaticDb.GetCount() == 0) {
    CuNameWithOwner noItem(VDBA_MfcResourceString (IDS_E_NO_DATABASE));//"<No Database>"
    m_Data.m_uaStaticDb.Add(noItem);
    m_Data.m_awDbType.Add(DBTYPE_REGULAR);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropStaticDb::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataStaticDb") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataStaticDb* pData = (CuDomPropDataStaticDb*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataStaticDb)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropStaticDb::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataStaticDb* pData = new CuDomPropDataStaticDb(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropStaticDb::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaStaticDb.GetCount();

  ASSERT (size > 0);
  ASSERT (m_Data.m_awDbType.GetSize() == size);

  for (cnt = 0; cnt < size; cnt++)
    AddDatabase(m_Data.m_uaStaticDb[cnt], m_Data.m_awDbType[cnt]);
}

void CuDlgDomPropStaticDb::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgDomPropStaticDb::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

  ASSERT (pNMHDR->code == NM_DBLCLK);

  // Get the selected item
  ASSERT (m_cListCtrl.GetSelectedCount() == 1);
  int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
  ASSERT (selItemId != -1);

  if (selItemId != -1) {
    CuNameWithOwner *pFastObject = (CuNameWithOwner*)m_cListCtrl.GetItemData(selItemId);
    ASSERT (pFastObject);
    ASSERT (pFastObject->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
    if (pFastObject) {
      if (!pFastObject->IsSpecialItem()) {
        CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;

        domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_DATABASE,
                                                          (LPCTSTR)pFastObject->GetStrName(),
                                                          (LPCTSTR)pFastObject->GetOwnerName()
                                                          ));
        if (!ExpandUpToSearchedItem(this, domTreeFastItemList)) {
          AfxMessageBox (IDS_DOM_FAST_CANNOTEXPAND);
        }
        while (!domTreeFastItemList.IsEmpty())
          delete domTreeFastItemList.RemoveHead();
      }
    }
  }
}

void CuDlgDomPropStaticDb::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}