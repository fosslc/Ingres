/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

// DgDpwfpr.cpp : implementation file
//
// 13-Dec-2001 (noifr01)
//   (sir 99596) removal of obsolete code and resources

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpwfpr.h"
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

static int aGrantType[NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS] = {
            ICE_GRANT_FACETNPAGE_ACCESSDEF_EXECUTE,
            ICE_GRANT_FACETNPAGE_ACCESSDEF_READ   ,
            ICE_GRANT_FACETNPAGE_ACCESSDEF_UPDATE ,
            ICE_GRANT_FACETNPAGE_ACCESSDEF_DELETE
};

#define LAYOUT_NUMBER   (NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS + 1)

CuDlgDomPropIceFacetNPageRoles::CuDlgDomPropIceFacetNPageRoles(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropIceFacetNPageRoles::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropIceFacetNPageRoles)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropIceFacetNPageRoles::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropIceFacetNPageRoles)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropIceFacetNPageRoles::AddFacetNPageAccessDef (CuIceFacetNPageAccessdef *pIceFacetNPageAccessDef)
{
  ASSERT (pIceFacetNPageAccessDef);
  BOOL  bSpecial = pIceFacetNPageAccessDef->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - last parameter is Image Index
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pIceFacetNPageAccessDef->GetStrName(), 0);

  // Grantee for what ?
  for (int cpt = 0; cpt < NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, "");
    else {
      if (pIceFacetNPageAccessDef->GetFlag(aGrantType[cpt]))
				m_cListCtrl.SetCheck (index, cpt+1, TRUE);
			else
				m_cListCtrl.SetCheck (index, cpt+1, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pIceFacetNPageAccessDef);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceFacetNPageRoles, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropIceFacetNPageRoles)
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
// CuDlgDomPropIceFacetNPageRoles message handlers

void CuDlgDomPropIceFacetNPageRoles::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropIceFacetNPageRoles::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_ICE_ROLE);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    {  _T(""),    FALSE, 100 + 16 + 4 },  // + 16+4 since image
    {  _T("Execute"), TRUE,  -1 },
    {  _T("Read"),    TRUE,  -1 },
    {  _T("Update"),  TRUE,  -1 },
    {  _T("Delete"),  TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME)); // _T("Name")
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropIceFacetNPageRoles::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropIceFacetNPageRoles::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropIceFacetNPageRoles::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OT_ICE_GENERIC)
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
  // Get list of FacetNPageRoles
  //
  m_Data.m_uaIceFacetNPageAccessdef.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  // no parent preparation - check type
  ASSERT (lpRecord->recType == OT_STATIC_ICE_BUNIT_FACET_ROLE ||
          lpRecord->recType == OT_ICE_BUNIT_FACET             ||
          lpRecord->recType == OT_STATIC_ICE_BUNIT_PAGE_ROLE ||
          lpRecord->recType == OT_ICE_BUNIT_PAGE
         );
  m_Data.m_objType = lpRecord->recType;   // MANDATORY!

  // parent bunit name in aparentsTemp[0]
  // parent facet or page name in aparentsTemp[1]
  aparentsTemp[0] = lpRecord->extra;
  switch (lpRecord->recType) {
    case OT_ICE_BUNIT_FACET:
    case OT_ICE_BUNIT_PAGE:
      aparentsTemp[1] = lpRecord->objName;
      break;
    case OT_STATIC_ICE_BUNIT_FACET_ROLE:
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
      aparentsTemp[1] = lpRecord->extra2;
      break;
    default:
      ASSERT(FALSE);
      return 0L;
  }
  aparentsTemp[2] = NULL;

  // request type:
  int reqType = -1;
  switch (lpRecord->recType) {
    case OT_ICE_BUNIT_FACET:
    case OT_STATIC_ICE_BUNIT_FACET_ROLE:
      reqType = OT_ICE_BUNIT_FACET_ROLE;
      break;
    case OT_ICE_BUNIT_PAGE:
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
      reqType = OT_ICE_BUNIT_PAGE_ROLE;
      break;
    default:
      ASSERT(FALSE);
      return 0L;
  }

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  iret =  DOMGetFirstObject(nNodeHandle,
                            reqType,
                            2,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuIceFacetNPageAccessdef errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));   // Special item"<Data Unavailable>"
    m_Data.m_uaIceFacetNPageAccessdef.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      BOOL bExecute = getint(bufComplim);
      BOOL bRead    = getint(bufComplim + STEPSMALLOBJ);
      BOOL bUpdate  = getint(bufComplim + (2*STEPSMALLOBJ));
      BOOL bDelete  = getint(bufComplim + (3*STEPSMALLOBJ));
      CuIceFacetNPageAccessdef item((const char*)buf, bExecute, bRead, bUpdate, bDelete);   // Regular item
      m_Data.m_uaIceFacetNPageAccessdef.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaIceFacetNPageAccessdef.GetCount() == 0) {
    CuIceFacetNPageAccessdef noItem(VDBA_MfcResourceString(IDS_NO_ROLE));//"< No Role >"      // Special item
    m_Data.m_uaIceFacetNPageAccessdef.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceFacetNPageRoles::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceFacetNPageAccessdef") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceFacetNPageAccessdef* pData = (CuDomPropDataIceFacetNPageAccessdef*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceFacetNPageAccessdef)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropIceFacetNPageRoles::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceFacetNPageAccessdef* pData = new CuDomPropDataIceFacetNPageAccessdef(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropIceFacetNPageRoles::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaIceFacetNPageAccessdef.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddFacetNPageAccessDef(m_Data.m_uaIceFacetNPageAccessdef[cnt]);
}

void CuDlgDomPropIceFacetNPageRoles::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

// DOUBLE CLICK ASSETS SINCE SELECTION NOT AVAILABLE - MASQUED OUT ASSERTIONS
void CuDlgDomPropIceFacetNPageRoles::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

  ASSERT (pNMHDR->code == NM_DBLCLK);

  // Get the selected item
  //ASSERT (m_cListCtrl.GetSelectedCount() == 1);
  int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
  //ASSERT (selItemId != -1);

  if (selItemId != -1) {
    CuIceFacetNPageAccessdef* pFastObject = (CuIceFacetNPageAccessdef*)m_cListCtrl.GetItemData(selItemId);
    ASSERT (pFastObject);
    ASSERT (pFastObject->IsKindOf(RUNTIME_CLASS(CuIceFacetNPageAccessdef)));
    if (pFastObject) {
      if (!pFastObject->IsSpecialItem()) {
        CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;

        // tree organization reflected in FastItemList, different according to record type
        int finalType = -1;
        switch (m_Data.m_objType) {
          case OT_ICE_BUNIT_FACET:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_ICE_BUNIT_FACET_ROLE));
            // NOTE: No break to chain on reqType
          case OT_STATIC_ICE_BUNIT_FACET_ROLE:
            finalType = OT_ICE_BUNIT_FACET_ROLE;
            break;

          case OT_ICE_BUNIT_PAGE:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_ICE_BUNIT_PAGE_ROLE));
            // NOTE: No break to chain on reqType
          case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
            finalType = OT_ICE_BUNIT_PAGE_ROLE;
            break;

          default:
            ASSERT(FALSE);
            return;
        }
        domTreeFastItemList.AddTail(new CuDomTreeFastItem(finalType,
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

void CuDlgDomPropIceFacetNPageRoles::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}