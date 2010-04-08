/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpDbAl.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_ALARM into two statements to avoid compiler error.
**  10-Dec-2001 (noifr01)
**      (sir 99596) removal of obsolete code and resources
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpdbal.h"
#include "vtree.h"

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

static int aAlarmType[NBDBALARMS] = { OT_S_ALARM_CO_SUCCESS_USER,
                                      OT_S_ALARM_CO_FAILURE_USER,
                                      OT_S_ALARM_DI_SUCCESS_USER,
                                      OT_S_ALARM_DI_FAILURE_USER };
#define LAYOUT_NUMBER   ( NBDBALARMS + 3 )


CuDlgDomPropDbAlarms::CuDlgDomPropDbAlarms(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropDbAlarms::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropDbAlarms)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropDbAlarms::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropDbAlarms)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropDbAlarms::AddDbAlarm (CuDbAlarm* pDbAlarm)
{
  ASSERT (pDbAlarm);
  BOOL  bSpecial = pDbAlarm->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Alarm number - image id always 0
  if (bSpecial)
    index  = m_cListCtrl.InsertItem (nCount, "", 0);
  else {
    CString csLong;
    csLong.Format("%ld", pDbAlarm->GetNumber());
    index  = m_cListCtrl.InsertItem (nCount, (LPCSTR)csLong, 0);
  }

  // Alarm name and Alarmee name
  m_cListCtrl.SetItemText (index, 1, (LPCSTR)pDbAlarm->GetStrName());
  m_cListCtrl.SetItemText (index, 2, (LPCSTR)pDbAlarm->GetStrAlarmee());

  for (int cpt = 0; cpt < NBDBALARMS; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+3, "");
    else {
      if (pDbAlarm->GetFlag(aAlarmType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+3, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+3, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on alarm, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pDbAlarm);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropDbAlarms, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropDbAlarms)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDbAlarms message handlers

void CuDlgDomPropDbAlarms::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropDbAlarms::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_DBALARM);
  m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  // Initalize the Column Header of CListCtrl (CuListCtrl)
  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T("#"),  FALSE,  30 + 16 },    // + 16 since image
    { _T(""),   FALSE,  80 },
    { _T(""),   FALSE,  60 },
    { _T(""),   TRUE,   -1 },
    { _T(""),   TRUE,   -1 },
    { _T(""),   TRUE,   -1 },
    { _T(""),   TRUE,   -1 },
  };
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_NAME));
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_BY));
  lstrcpy(aColumns[3].szCaption, VDBA_MfcResourceString(IDS_TC_CONNECT_SUCCESS));
  lstrcpy(aColumns[4].szCaption, VDBA_MfcResourceString(IDS_TC_CONNECT_FAILURE));
  lstrcpy(aColumns[5].szCaption, VDBA_MfcResourceString(IDS_TC_DISCONNECT_SUCCESS));
  lstrcpy(aColumns[6].szCaption, VDBA_MfcResourceString(IDS_TC_DISCONNECT_FAILURE));
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropDbAlarms::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropDbAlarms::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropDbAlarms::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      case FILTER_DOM_SYSTEMOBJECTS:  // can be $ingres
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

      case FILTER_DOM_BKREFRESH:
      // eligible if UpdateType is compatible with DomGetFirst/Next object type,
      // or is ot_virtnode, which means refresh for all whatever the type is
      if (pUps->pSFilter->UpdateType != OT_VIRTNODE &&
          pUps->pSFilter->UpdateType != OTLL_SECURITYALARM)
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
  // Get list of alarms
  //
  m_Data.m_uaDbAlarms.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];
  UCHAR   emptyParent[] = "";

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  int basicType = GetBasicType(lpRecord->recType);
  switch (basicType) {
    case OT_DATABASE:
      aparentsTemp[0] = lpRecord->objName;  // DBName
      break;
    case OT_STATIC_DBALARM:
      aparentsTemp[0] = lpRecord->extra;    // DBName
      break;
    case OT_STATIC_INSTALL_ALARMS:
      aparentsTemp[0] = emptyParent;        // Installation level, no Db name
      break;
    default:
      ASSERT (FALSE);
      return 0L;
  }
  aparentsTemp[1] = NULL;
  aparentsTemp[2] = NULL;

  BOOL bError = FALSE;
  for (int index = 0; index < NBDBALARMS; index++) {
    iret =  DOMGetFirstObject(nNodeHandle,
                              aAlarmType[index],
                              1,                            // level,
                              aparentsTemp,                 // Temp mandatory!
                              pUps->pSFilter->bWithSystem,  // bwithsystem
                              NULL,                         // lpowner
                              buf,
                              bufOwner,
                              bufComplim);
     if (iret != RES_SUCCESS && iret != RES_ENDOFDATA) {
       bError = TRUE;
       continue;
    }
    else {
      while (iret == RES_SUCCESS) {
        // received data:
        // - optional alarm name in bufComplim + STEPSMALLOBJ
        // - Alarmee name in buf
        // - Alarm number bufComplim by getint()
        CuDbAlarm dbAlarm((long)getint(bufComplim),                 // Alarm number,
                          (const char *)bufComplim + STEPSMALLOBJ,  // Optional Alarm name
                          (const char *)buf,                        // Alarmee name (user/group/role/"public"
                          aAlarmType[index]                          // alarm type
                          );
        CuMultFlag *pRefAlarm = m_Data.m_uaDbAlarms.Find(&dbAlarm);
        if (pRefAlarm)
          m_Data.m_uaDbAlarms.Merge(pRefAlarm, &dbAlarm);
        else
          m_Data.m_uaDbAlarms.Add(dbAlarm);

        iret = DOMGetNextObject(buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case 
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuDbAlarm dbAlarm2(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaDbAlarms.Add(dbAlarm2);
  }

  // Manage no alarms
  if (m_Data.m_uaDbAlarms.GetCount() == 0)
  {
    /* "<No Alarm>" */
    CuDbAlarm dbAlarm3(VDBA_MfcResourceString (IDS_E_NO_ALARM));
    m_Data.m_uaDbAlarms.Add(dbAlarm3);
  }

  ASSERT (m_Data.m_uaDbAlarms.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropDbAlarms::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataDbAlarms") == 0);
  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataDbAlarms* pData = (CuDomPropDataDbAlarms*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataDbAlarms)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropDbAlarms::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataDbAlarms* pData = new CuDomPropDataDbAlarms(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropDbAlarms::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaDbAlarms.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuDbAlarm* pAlarm = m_Data.m_uaDbAlarms[cnt];
    AddDbAlarm(pAlarm);
  }
  // Initial sort on alarmees, secondary on names, ternary on numbers
  m_cListCtrl.SortItems(CuAlarm::CompareNumbers, 0L);
  m_cListCtrl.SortItems(CuAlarm::CompareNames, 0L);
  m_cListCtrl.SortItems(CuAlarm::CompareAlarmees, 0L);
}

void CuDlgDomPropDbAlarms::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuAlarm::CompareNumbers, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuAlarm::CompareNames, 0L);
      break;
    case 2:
      m_cListCtrl.SortItems(CuAlarm::CompareAlarmees, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 3);
      break;
  }

  *pResult = 0;
}


void CuDlgDomPropDbAlarms::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
