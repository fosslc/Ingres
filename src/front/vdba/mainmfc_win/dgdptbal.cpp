/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  Source   : DgDpTbAl.cpp : implementation file
**
**  History after 04-May-1999:
**	24-Jan-2000 (schph01)
**	    BUG #99804 
**	    Add case OTR_REPLIC_CDDS_TABLE in OnUpdateData() fonction.      
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_DATA_UNAVAILABLE
**	    and IDS_E_NO_ALARM into two statements to avoid compiler error.
**  10-Dec-2001 (noifr01)
**      (sir 99596) removal of obsolete code and resources
**
*****************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "dgdptbal.h"
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

static int aAlarmType[NBTBLALARMS] = { OT_S_ALARM_SELSUCCESS_USER,
                                       OT_S_ALARM_SELFAILURE_USER,
                                       OT_S_ALARM_DELSUCCESS_USER,
                                       OT_S_ALARM_DELFAILURE_USER,
                                       OT_S_ALARM_INSSUCCESS_USER,
                                       OT_S_ALARM_INSFAILURE_USER,
                                       OT_S_ALARM_UPDSUCCESS_USER,
                                       OT_S_ALARM_UPDFAILURE_USER };
#define LAYOUT_NUMBER   ( NBTBLALARMS + 3 )

CuDlgDomPropTableAlarms::CuDlgDomPropTableAlarms(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTableAlarms::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropTableAlarms)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropTableAlarms::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTableAlarms)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropTableAlarms::AddTableAlarm (CuTableAlarm* pTableAlarm)
{
  ASSERT (pTableAlarm);
  BOOL  bSpecial = pTableAlarm->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Alarm number - image id always 0
  if (bSpecial)
    index  = m_cListCtrl.InsertItem (nCount, "", 0);
  else {
    CString csLong;
    csLong.Format("%ld", pTableAlarm->GetNumber());
    index  = m_cListCtrl.InsertItem (nCount, (LPCSTR)csLong, 0);
  }

  // Alarm name and Alarmee name
  m_cListCtrl.SetItemText (index, 1, (LPCSTR)pTableAlarm->GetStrName());
  m_cListCtrl.SetItemText (index, 2, (LPCSTR)pTableAlarm->GetStrAlarmee());

  for (int cpt = 0; cpt < NBTBLALARMS; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+3, "");
    else {
      if (pTableAlarm->GetFlag(aAlarmType[cpt]))
        m_cListCtrl.SetCheck (index, cpt+3, TRUE);
      else
        m_cListCtrl.SetCheck (index, cpt+3, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on alarm, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pTableAlarm);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableAlarms, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropTableAlarms)
    ON_WM_SIZE()
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableAlarms message handlers

void CuDlgDomPropTableAlarms::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTableAlarms::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));

  m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_DBALARM);  // Same image as for dbalarm.
  m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

  m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T("#"),              FALSE, 30 + 16},    // +16 for image
    { _T(""),               FALSE, 80 },
    { _T(""),               FALSE, 60 },
    { _T("Select Success"), TRUE,  -1 },
    { _T("Select Failure"), TRUE,  -1 },
    { _T("Delete Success"), TRUE,  -1 },
    { _T("Delete Failure"), TRUE,  -1 },
    { _T("Insert Success"), TRUE,  -1 },
    { _T("Insert Failure"), TRUE,  -1 },
    { _T("Update Success"), TRUE,  -1 },
    { _T("Update Failure"), TRUE,  -1 },
  };
  lstrcpy(aColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_NAME)); // _T("Name")
  lstrcpy(aColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_BY));   // _T("By")
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTableAlarms::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropTableAlarms::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropTableAlarms::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  // Get info on the current item
  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get list of alarms
  //
  m_Data.m_uaTableAlarms.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));

  // prepare parenthood with schema where relevant
  aparentsTemp[0] = lpRecord->extra;  // parent DB Name

  UCHAR bufParent1[MAXOBJECTNAME];
  aparentsTemp[1] = bufParent1;
  switch (lpRecord->recType) {
    case OT_TABLE:
    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_CPI_TABLE:
    case OTR_GRANTEE_CPF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
    case OTR_LOCATIONTABLE:
    case OT_REPLIC_REGTABLE:
    case OT_SCHEMAUSER_TABLE:
    case OTR_REPLIC_CDDS_TABLE:
      x_strcpy((char *)buf, (const char *)lpRecord->objName);
      break;
    case OT_STATIC_SECURITY:
      x_strcpy((char *)buf, (const char *)lpRecord->extra2);
      break;
    default:
      ASSERT (FALSE);
      buf[0] = '\0';
  }
  ASSERT (lpRecord->ownerName);
  StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[1]); // schema.name

  aparentsTemp[2] = NULL;

  // loop on alarms
  BOOL bError = FALSE;
  for (int index = 0; index < NBTBLALARMS; index++) {
    iret =  DOMGetFirstObject(nNodeHandle,
                              aAlarmType[index],
                              2,                            // level,
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
        CuTableAlarm alarm((long)getint(bufComplim),                 // Alarm number,
                           (const char *)bufComplim + STEPSMALLOBJ,  // Optional Alarm name
                           (const char *)buf,                        // Alarmee name (user/group/role/"public"
                           aAlarmType[index]                          // alarm type
                          );
        CuMultFlag *pRefAlarm = m_Data.m_uaTableAlarms.Find(&alarm);
        if (pRefAlarm)
          m_Data.m_uaTableAlarms.Merge(pRefAlarm, &alarm);
        else
          m_Data.m_uaTableAlarms.Add(alarm);

        iret = DOMGetNextObject(buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case 
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuTableAlarm alarm1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaTableAlarms.Add(alarm1);
  }

  // Manage no alarms
  if (m_Data.m_uaTableAlarms.GetCount() == 0)
  {
    /* "<No Alarm>" */
    CuTableAlarm alarm2(VDBA_MfcResourceString (IDS_E_NO_ALARM));
    m_Data.m_uaTableAlarms.Add(alarm2);
  }

  ASSERT (m_Data.m_uaTableAlarms.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTableAlarms::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataTableAlarms") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataTableAlarms* pData = (CuDomPropDataTableAlarms*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataTableAlarms)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropTableAlarms::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataTableAlarms* pData = new CuDomPropDataTableAlarms(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropTableAlarms::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaTableAlarms.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuTableAlarm* pAlarm = m_Data.m_uaTableAlarms[cnt];
    AddTableAlarm(pAlarm);
  }
  // Initial sort on alarmees, secondary on names, ternary on numbers
  m_cListCtrl.SortItems(CuAlarm::CompareNumbers, 0L);
  m_cListCtrl.SortItems(CuAlarm::CompareNames, 0L);
  m_cListCtrl.SortItems(CuAlarm::CompareAlarmees, 0L);
}

void CuDlgDomPropTableAlarms::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgDomPropTableAlarms::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
