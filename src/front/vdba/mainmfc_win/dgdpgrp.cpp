// DgDPGrp.cpp : implementation file
//
//  23-Dec-1999 (schph01)
//  bug #99804
//  Add case OTR_USERGROUP in OnUpdateData() and OnDblclkMfcList1()
//  functions.

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpgrp.h"
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
#define LAYOUT_NUMBER   1


CuDlgDomPropGroup::CuDlgDomPropGroup(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropGroup::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropGroup)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropGroup::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropGroup)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropGroup::AddUser (LPCTSTR usrName)
{
  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // image id always 0
  index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)usrName, 0);

  // if additional columns:   m_cListCtrl.SetItemText (index, numcol, (LPTSTR)strXYZ);

  // Attach pointer on object, for double click purpose
  m_cListCtrl.SetItemData(index, (DWORD)usrName);

}


BEGIN_MESSAGE_MAP(CuDlgDomPropGroup, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropGroup)
    ON_WM_SIZE()
  ON_NOTIFY(NM_DBLCLK, IDC_MFC_LIST1, OnDblclkMfcList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropGroup message handlers

void CuDlgDomPropGroup::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropGroup::OnInitDialog() 
{
  CDialog::OnInitDialog();
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  LV_COLUMN lvcolumn;
  TCHAR strHeader[LAYOUT_NUMBER][32];
  lstrcpy(strHeader[0], VDBA_MfcResourceString(IDS_TC_NAME));// _T("Name")
  int i, hWidth   [LAYOUT_NUMBER] = {150};

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
  m_ImageList.AddIcon(IDI_TREE_STATIC_USER);
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropGroup::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropGroup::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropGroup::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_DOM_SYSTEMOBJECTS:    // because list of users can contain "$ingres"
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OT_GROUPUSER)
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
  // Get list of users in group
  //
  m_Data.m_acsUsersInGroup.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // parent preparation - added static type.
  switch (lpRecord->recType) {
    case OT_GROUP:
    case OTR_USERGROUP:
      aparentsTemp[0] = lpRecord->objName;
      break;
    case OT_STATIC_GROUPUSER:
      aparentsTemp[0] = lpRecord->extra;
      break;
    default:
      ASSERT(FALSE);
      return 0L;
  }
  m_Data.m_objType = lpRecord->recType;   // MANDATORY!

  iret =  DOMGetFirstObject(nNodeHandle,
                            OT_GROUPUSER,
                            1,                            // level,
                            aparentsTemp,                 // Temp mandatory!
                            pUps->pSFilter->bWithSystem,  // bwithsystem
                            NULL,                         // lpowner
                            buf,
                            bufOwner,
                            bufComplim);
  if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
    // Erroneous case!
    CString csUnavail = VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE);//"<Data Unavailable>"
    m_Data.m_acsUsersInGroup.Add(csUnavail);
  }
  else {
    while (iret == RES_SUCCESS) {
      m_Data.m_acsUsersInGroup.Add((LPCTSTR)buf);
      iret = DOMGetNextObject(buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_acsUsersInGroup.GetUpperBound() == -1) {
    CString csNoUsers = VDBA_MfcResourceString (IDS_E_NO_USERS);//"<No Users>"
    m_Data.m_acsUsersInGroup.Add(csNoUsers);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropGroup::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataGroup") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataGroup* pData = (CuDomPropDataGroup*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataGroup)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropGroup::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataGroup* pData = new CuDomPropDataGroup(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropGroup::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_acsUsersInGroup.GetSize();
  for (cnt = 0; cnt < size; cnt++)
    AddUser(m_Data.m_acsUsersInGroup[cnt]);
}

void CuDlgDomPropGroup::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  *pResult = 0;

  ASSERT (pNMHDR->code == NM_DBLCLK);

  // Get the selected item
  ASSERT (m_cListCtrl.GetSelectedCount() == 1);
  int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
  ASSERT (selItemId != -1);

  if (selItemId != -1) {
    LPCTSTR pFastName = (LPCTSTR)m_cListCtrl.GetItemData(selItemId);
    ASSERT (pFastName);
    if (pFastName) {
      if (pFastName[0] != _T('<')) {
        CTypedPtrList<CObList, CuDomTreeFastItem*>  domTreeFastItemList;

        // tree organization reflected in FastItemList, different according to record type
        switch (m_Data.m_objType) {
          case OT_GROUP:
          case OTR_USERGROUP:
            domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_STATIC_GROUPUSER));
            break;
          case OT_STATIC_GROUPUSER:
            break;
          default:
            ASSERT(FALSE);
            return;
        }
        domTreeFastItemList.AddTail(new CuDomTreeFastItem(OT_GROUPUSER,
                                                          pFastName
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

void CuDlgDomPropGroup::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}