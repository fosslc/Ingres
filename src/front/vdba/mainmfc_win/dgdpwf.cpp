// DgDpwf.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpwf.h"
#include "vtree.h"
#include "domfast.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  //#include "monitor.h"
  #include "resource.h"
  #include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   1


CuDlgDomPropIceFacets::CuDlgDomPropIceFacets(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropIceFacets::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropIceFacets)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropIceFacets::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropIceFacets)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropIceFacets::AddFacet (CuNameOnly *pIceFacets)
{
  ASSERT (pIceFacets);
  BOOL  bSpecial = pIceFacets->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - last parameter is Image Index
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pIceFacets->GetStrName(), 0);

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pIceFacets);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceFacets, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropIceFacets)
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
// CuDlgDomPropIceFacets message handlers

void CuDlgDomPropIceFacets::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropIceFacets::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  int i, hWidth   [LAYOUT_NUMBER] = {120};

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
  m_ImageList.AddIcon(IDI_TREE_STATIC_ICE_BUNIT);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropIceFacets::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropIceFacets::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropIceFacets::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OT_ICE_BUNIT_FACET)
        return 0L;
      break;
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListCtrl);
        return 0L;
    default:
      return 0L;    // nothing to change on the display
  }

  ResetDisplay();
  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get list of Facets
  //
  m_Data.m_uaIceFacets.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  // no parent preparation - check type
  ASSERT (lpRecord->recType == OT_STATIC_ICE_BUNIT_FACET || lpRecord->recType == OT_ICE_BUNIT);
  m_Data.m_objType = lpRecord->recType;   // MANDATORY!

  // parent bunit name in aparentsTemp[0]
  switch (lpRecord->recType) {
    case OT_ICE_BUNIT:
      aparentsTemp[0] = lpRecord->objName;
      break;
    case OT_STATIC_ICE_BUNIT_FACET:
      aparentsTemp[0] = lpRecord->extra;
      break;
    default:
      ASSERT(FALSE);
      return 0L;
  }
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_ICE_BUNIT_FACET,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuNameOnly errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE), TRUE);   // Special item"<Data Unavailable>"
    m_Data.m_uaIceFacets.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      CuNameOnly item((const char*)buf, FALSE);   // Regular item
      m_Data.m_uaIceFacets.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaIceFacets.GetCount() == 0) {
    CuNameOnly noItem(VDBA_MfcResourceString(IDS_TREE_ICE_BUNIT_FACET_NONE), TRUE); //"< No Facet >"     // Special item
    m_Data.m_uaIceFacets.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceFacets::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceFacets") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceFacets* pData = (CuDomPropDataIceFacets*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceFacets)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropIceFacets::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceFacets* pData = new CuDomPropDataIceFacets(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropIceFacets::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaIceFacets.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddFacet(m_Data.m_uaIceFacets[cnt]);
}

void CuDlgDomPropIceFacets::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuNameOnly::CompareNames, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 1);
      break;
  }
	*pResult = 0;
}

void CuDlgDomPropIceFacets::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

  ASSERT (pNMHDR->code == NM_DBLCLK);

  // Get the selected item
  ASSERT (m_cListCtrl.GetSelectedCount() == 1);
  int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
  ASSERT (selItemId != -1);

  if (selItemId != -1) {
    CuNameOnly* pFastObject = (CuNameOnly*)m_cListCtrl.GetItemData(selItemId);
    ASSERT (pFastObject);
    ASSERT (pFastObject->IsKindOf(RUNTIME_CLASS(CuNameOnly)));
    if (pFastObject) {
      if (!pFastObject->IsSpecialItem()) {
        CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;

        // tree organization reflected in FastItemList, different according to record type
        switch (m_Data.m_objType) {
        case OT_ICE_BUNIT:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_ICE_BUNIT_FACET));
            break;
          case OT_STATIC_ICE_BUNIT_FACET:
            break;
          default:
            ASSERT(FALSE);
            return;
        }
        domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_ICE_BUNIT_FACET,
                                                          (LPCTSTR)pFastObject->GetStrName()
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

void CuDlgDomPropIceFacets::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
