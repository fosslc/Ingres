/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpRoXV.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_GRANTED_VIEW into two statements to avoid compiler
**	    error.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdproxv.h"
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

static int aGrantedType[NBVIEWGRANTED] = {
                                          OTR_GRANTEE_SEL_VIEW,
                                          OTR_GRANTEE_INS_VIEW,
                                          OTR_GRANTEE_UPD_VIEW,
                                          OTR_GRANTEE_DEL_VIEW,
                                         };
#define LAYOUT_NUMBER   ( NBVIEWGRANTED + 3 )

CuDlgDomPropRoleXGrantedView::CuDlgDomPropRoleXGrantedView(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropRoleXGrantedView::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropRoleXGrantedView)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropRoleXGrantedView::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropRoleXGrantedView)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropRoleXGrantedView::AddRoleXGrantedView (CuGrantedView* pGrantedView)
{
  ASSERT (pGrantedView);
  BOOL  bSpecial = pGrantedView->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Name
  index  = m_cListCtrl.InsertItem (nCount, pGrantedView->GetStrName(), 0);

  // Owner - parent View
  m_cListCtrl.SetItemText (index, 1, (LPCSTR)pGrantedView->GetOwnerName());
  m_cListCtrl.SetItemText (index, 2, (LPCSTR)pGrantedView->GetParentName());

  for (int cpt = 0; cpt < NBVIEWGRANTED; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+3, _T(""));
    else {
      if (pGrantedView->GetFlag(aGrantedType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+3, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+3, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on granted object, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pGrantedView);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropRoleXGrantedView, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropRoleXGrantedView)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRoleXGrantedView message handlers

void CuDlgDomPropRoleXGrantedView::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropRoleXGrantedView::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_VIEW);
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T(""),               FALSE, 80 + 16+4 },   // 16+4 for image
    { _T(""),               FALSE, 80 },
    { _T(""),               FALSE, 80 },
    { _T("Select"),         TRUE,  -1 },
    { _T("Insert"),         TRUE,  -1 },
    { _T("Update"),         TRUE,  -1 },
    { _T("Delete"),         TRUE,  -1 },
  };
  lstrcpy(aColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_OWNER));//_T("Owner")
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_PARENT_DB));
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropRoleXGrantedView::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropRoleXGrantedView::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropRoleXGrantedView::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      case FILTER_DOM_SYSTEMOBJECTS:  // can be system objects (replicator dd_xyz)
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_VIEW)
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
  // Get list of granted objects
  //
  m_Data.m_uaRoleXGrantedView.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // Xref return code
  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBVIEWGRANTED; index++) {
    iret =  DOMGetFirstRelObject(nNodeHandle,
                                 aGrantedType[index],
                                 1,                           // level,
                                 aparentsTemp,                // Temp mandatory!
                                 pUps->pSFilter->bWithSystem, // bwithsystem
                                 NULL,                        // base owner
                                 NULL,                        // other owner
                                 aparentsResult,
                                 buf,
                                 bufOwner,
                                 bufComplim);
     if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
       bError = TRUE;
       continue;
    }
    else {
      while (iret == RES_SUCCESS) {
        CuGrantedView grantedView((const char *)buf,                // obj name
                              (const char *)bufOwner,           // obj owner
                              (const char *)aparentsResult[0],  // obj parent
                              aGrantedType[index]               // grant type
                              );
        CuMultFlag *pRefGrantedView = m_Data.m_uaRoleXGrantedView.Find(&grantedView);
        if (pRefGrantedView)
          m_Data.m_uaRoleXGrantedView.Merge(pRefGrantedView, &grantedView);
        else
          m_Data.m_uaRoleXGrantedView.Add(grantedView);

        iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuGrantedView grantedView1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaRoleXGrantedView.Add(grantedView1);
  }

  // Manage no granted object
  if (m_Data.m_uaRoleXGrantedView.GetCount() == 0)
  {
    /* "<No GrantedView>" */
    CuGrantedView grantedView2(VDBA_MfcResourceString (IDS_E_GRANTED_VIEW));
    m_Data.m_uaRoleXGrantedView.Add(grantedView2);
  }

  ASSERT (m_Data.m_uaRoleXGrantedView.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropRoleXGrantedView::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataRoleXGrantedView") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataRoleXGrantedView* pData = (CuDomPropDataRoleXGrantedView*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataRoleXGrantedView)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropRoleXGrantedView::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataRoleXGrantedView* pData = new CuDomPropDataRoleXGrantedView(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropRoleXGrantedView::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaRoleXGrantedView.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuGrantedView* pGrantedView = m_Data.m_uaRoleXGrantedView[cnt];
    AddRoleXGrantedView(pGrantedView);
  }
  // No Initial sort
}

void CuDlgDomPropRoleXGrantedView::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuGranted::CompareNames, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuGranted::CompareOwners, 0L);
      break;
    case 2:
      m_cListCtrl.SortItems(CuGranted::CompareParents, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 3);
      break;
  }
  *pResult = 0;
}

void CuDlgDomPropRoleXGrantedView::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
