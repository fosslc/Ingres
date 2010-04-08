// DgDpTbIg.cpp : implementation file
//
//  23-Dec-1999 (schph01)
//  bug #99804
//  Add case OT_SCHEMAUSER_TABLE in OnUpdateData() and OnDblclkMfcList1()
//  functions.

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdptbig.h"
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
  #include "domdisp.h"
  #include "resource.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   1


CuDlgDomPropTblInteg::CuDlgDomPropTblInteg(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTblInteg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropTblInteg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropTblInteg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTblInteg)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropTblInteg::AddTblInteg (CuNameWithOwner *pTblInteg)
{
  ASSERT (pTblInteg);
  BOOL  bSpecial = pTblInteg->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - image index always 0
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pTblInteg->GetStrName(), 0);

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pTblInteg);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTblInteg, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropTblInteg)
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
// CuDlgDomPropTblInteg message handlers

void CuDlgDomPropTblInteg::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTblInteg::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  int i, hWidth   [LAYOUT_NUMBER] = {150 + 16};  // +16 for image

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
  m_ImageList.AddIcon(IDI_TREE_STATIC_INTEGRITY);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTblInteg::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropTblInteg::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropTblInteg::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_INTEGRITY)
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
  // Get list of Tbl for the replication
  //
  m_Data.m_uaTblInteg.RemoveAll();

  int     iret;
  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  //
  // CHECK EXISTENCE SINCE REGISTERED TABLE MIGHT NOT REALLY EXIST YET
  //
  int   resultType;
  UCHAR resultObjectName[MAXOBJECTNAME];
  UCHAR resultOwnerName[MAXOBJECTNAME];
  UCHAR resultExtraData[MAXOBJECTNAME];
  char* parentStrings[3];
  parentStrings[0] = (char*)lpRecord->extra;  // DBName
  parentStrings[1] = parentStrings[2] = NULL;

  // preparation for get limited info - added static type - added all related granted types
  LPUCHAR lpName = NULL;
  LPUCHAR lpOwner = NULL;
  int basicType = GetBasicType(lpRecord->recType);
  m_Data.m_objType = basicType;
  switch (basicType) {
    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      lpName = (LPUCHAR)lpRecord->objName;
      lpOwner = (LPUCHAR)lpRecord->ownerName;
      break;
    case OT_STATIC_INTEGRITY:
      lpName = (LPUCHAR)lpRecord->extra2;
      lpOwner = (LPUCHAR)lpRecord->tableOwner;
      break;
    default:
      ASSERT(FALSE);
      return 0L;
  }

  iret = DOMGetObjectLimitedInfo(nNodeHandle,
                                 lpName,                           // object name,
                                 lpOwner,                          // object owner
                                 OT_TABLE,                         // iobjecttype
                                 1,                                // level
                                 (unsigned char **)parentStrings,  // parenthood
                                 TRUE,                             // bwithsystem
                                 &resultType,
                                 resultObjectName,
                                 resultOwnerName,
                                 resultExtraData
                                 );
  if (iret == RES_ENDOFDATA)
    iret = RES_ERR;           // Non-existent: ERROR !!!
  if (iret == RES_SUCCESS) {
    LPUCHAR aparentsTemp[MAXPLEVEL];

    // prepare parenthood with schema where relevant
    aparentsTemp[0] = lpRecord->extra;  // DBName

    UCHAR bufParent1[MAXOBJECTNAME];
    aparentsTemp[1] = bufParent1;
    x_strcpy((char *)buf, (const char *)resultObjectName);
    StringWithOwner(buf, resultOwnerName, aparentsTemp[1]); // schema.name

    aparentsTemp[2] = NULL;

    memset (&bufComplim, '\0', sizeof(bufComplim));
    memset (&bufOwner, '\0', sizeof(bufOwner));
    iret =  DOMGetFirstObject(nNodeHandle,
                              OT_INTEGRITY,
                              2,                            // level,
                              aparentsTemp,                 // Temp mandatory!
                              pUps->pSFilter->bWithSystem,  // bwithsystem
                              NULL,                         // lpowner
                              buf,
                              bufOwner,
                              bufComplim);
  }
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameWithOwner errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));//"<Data Unavailable>"
    m_Data.m_uaTblInteg.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      CuNameWithOwner item((const char*)buf, _T(""));   // no owner
      m_Data.m_uaTblInteg.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaTblInteg.GetCount() == 0) {
    CuNameWithOwner noItem(VDBA_MfcResourceString (IDS_E_NO_INTEGRITY));//"<No Integrity>"
    m_Data.m_uaTblInteg.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTblInteg::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataTblInteg") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataTblInteg* pData = (CuDomPropDataTblInteg*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataTblInteg)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropTblInteg::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataTblInteg* pData = new CuDomPropDataTblInteg(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropTblInteg::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaTblInteg.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddTblInteg(m_Data.m_uaTblInteg[cnt]);
}

void CuDlgDomPropTblInteg::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgDomPropTblInteg::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

        // tree organization reflected in FastItemList, different according to record type
        switch (m_Data.m_objType) {
          case OT_TABLE:
          case OT_SCHEMAUSER_TABLE:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_INTEGRITY));
            break;
          case OT_STATIC_INTEGRITY:
            break;
          default:
            ASSERT(FALSE);
            return;
        }
        domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_INTEGRITY,
                                                          (LPCTSTR)pFastObject->GetStrName(),
                                                          _T("")    // no owner
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

void CuDlgDomPropTblInteg::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}