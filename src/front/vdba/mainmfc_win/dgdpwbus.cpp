/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

// DgDpwbus.cpp : implementation file
//
// 13-Dec-2001 (noifr01)
//   (sir 99596) removal of obsolete code and resources

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpwbus.h"
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

static int aGrantType[NB_ICE_BUNIT_USER_COLUMNS] = {ICE_GRANT_TYPE_USER_UNITREAD  ,
                                                    ICE_GRANT_TYPE_USER_CREATEDOC,
                                                    ICE_GRANT_TYPE_USER_READDOC  ,
                                                   };
#define LAYOUT_NUMBER   (NB_ICE_BUNIT_USER_COLUMNS + 1)

CuDlgDomPropIceBunitUsers::CuDlgDomPropIceBunitUsers(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropIceBunitUsers::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropIceBunitUsers)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropIceBunitUsers::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropIceBunitUsers)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropIceBunitUsers::AddBunitUser (CuIceBunitUserGrant *pIceBunitUser)
{
  ASSERT (pIceBunitUser);
  BOOL  bSpecial = pIceBunitUser->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name - last parameter is Image Index
  index  = m_cListCtrl.InsertItem (nCount, (LPCTSTR)pIceBunitUser->GetStrName(), 0);

  // Grantee for what ?
  for (int cpt = 0; cpt < NB_ICE_BUNIT_USER_COLUMNS; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+1, "");
    else {
      if (pIceBunitUser->GetFlag(aGrantType[cpt]))
				m_cListCtrl.SetCheck (index, cpt+1, TRUE);
			else
				m_cListCtrl.SetCheck (index, cpt+1, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pIceBunitUser);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropIceBunitUsers, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropIceBunitUsers)
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
// CuDlgDomPropIceBunitUsers message handlers

void CuDlgDomPropIceBunitUsers::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropIceBunitUsers::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_ICE_WEBUSER);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    {  _T(""),  FALSE, 100 + 16 + 4 },  // + 16+4 since image
    {  _T(""),  TRUE,  -1 },
    {  _T(""),  TRUE,  -1 },
    {  _T(""),  TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME)); // _T("Name")
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_EXE_DOC));
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_CREAT_DOC));
  lstrcpy(aColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_READ_DOC));
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropIceBunitUsers::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropIceBunitUsers::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropIceBunitUsers::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OT_ICE_BUNIT_SEC_USER)
        return 0L;
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  ResetDisplay();
  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);

  //
  // Get list of BunitUsers
  //
  m_Data.m_uaIceBunitUsers.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  // no parent preparation - check type
  ASSERT (lpRecord->recType == OT_STATIC_ICE_BUNIT_SEC_USER || lpRecord->recType == OT_ICE_BUNIT);
  m_Data.m_objType = lpRecord->recType;   // MANDATORY!

  // parent bunit name in aparentsTemp[0]
  switch (lpRecord->recType) {
    case OT_ICE_BUNIT:
      aparentsTemp[0] = lpRecord->objName;
      break;
    case OT_STATIC_ICE_BUNIT_SEC_USER:
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
                            OT_ICE_BUNIT_SEC_USER,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CuIceBunitUserGrant errItem(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));   // Special item"<Data Unavailable>"
    m_Data.m_uaIceBunitUsers.Add(errItem);
  }
  else {
    while (iret == RES_SUCCESS) {
      // order in bufComplim: see relevant storeint() comment in dbaginfo.sc, line 3935
      BOOL bUnitRead  = getint(bufComplim );
      BOOL bCreateDoc = getint(bufComplim + (2*STEPSMALLOBJ));
      BOOL bReadDoc   = getint(bufComplim + STEPSMALLOBJ);
      CuIceBunitUserGrant item((const char*)buf, bUnitRead, bCreateDoc, bReadDoc);   // Regular item
      m_Data.m_uaIceBunitUsers.Add(item);

      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_uaIceBunitUsers.GetCount() == 0) {
    CuIceBunitUserGrant noItem(VDBA_MfcResourceString(IDS_NO_USER)); //"< No User >"     // Special item
    m_Data.m_uaIceBunitUsers.Add(noItem);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropIceBunitUsers::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataIceBunitUsers") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataIceBunitUsers* pData = (CuDomPropDataIceBunitUsers*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataIceBunitUsers)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropIceBunitUsers::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataIceBunitUsers* pData = new CuDomPropDataIceBunitUsers(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropIceBunitUsers::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaIceBunitUsers.GetCount();
  for (cnt = 0; cnt < size; cnt++)
    AddBunitUser(m_Data.m_uaIceBunitUsers[cnt]);
}

void CuDlgDomPropIceBunitUsers::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
void CuDlgDomPropIceBunitUsers::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

  ASSERT (pNMHDR->code == NM_DBLCLK);

  // Get the selected item
  //ASSERT (m_cListCtrl.GetSelectedCount() == 1);
  int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
  //ASSERT (selItemId != -1);

  if (selItemId != -1) {
    CuIceBunitUserGrant* pFastObject = (CuIceBunitUserGrant*)m_cListCtrl.GetItemData(selItemId);
    ASSERT (pFastObject);
    ASSERT (pFastObject->IsKindOf(RUNTIME_CLASS(CuIceBunitUserGrant)));
    if (pFastObject) {
      if (!pFastObject->IsSpecialItem()) {
        CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;

        // tree organization reflected in FastItemList, different according to record type
        switch (m_Data.m_objType) {
          case OT_ICE_BUNIT:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_ICE_BUNIT_SECURITY));
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_ICE_BUNIT_SEC_USER));
            break;
          case OT_STATIC_ICE_BUNIT_SEC_USER:
            break;
          default:
            ASSERT(FALSE);
            return;
        }
        domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_ICE_BUNIT_SEC_USER,
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

void CuDlgDomPropIceBunitUsers::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}