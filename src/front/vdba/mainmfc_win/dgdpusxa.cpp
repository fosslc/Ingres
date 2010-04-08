/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpUsXA.cpp
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
#include "dgdpusxa.h"
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

static int aAlarmType[NBUSERXTBLALARMS] = { OTR_ALARM_SELSUCCESS_TABLE,
                                            OTR_ALARM_SELFAILURE_TABLE,
                                            OTR_ALARM_DELSUCCESS_TABLE,
                                            OTR_ALARM_DELFAILURE_TABLE,
                                            OTR_ALARM_INSSUCCESS_TABLE,
                                            OTR_ALARM_INSFAILURE_TABLE,
                                            OTR_ALARM_UPDSUCCESS_TABLE,
                                            OTR_ALARM_UPDFAILURE_TABLE };
#define LAYOUT_NUMBER   ( NBUSERXTBLALARMS + 3 )

CuDlgDomPropUserXAlarms::CuDlgDomPropUserXAlarms(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropUserXAlarms::IDD, pParent)
{
    //{{AFX_DATA_INIT(CuDlgDomPropUserXAlarms)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CuDlgDomPropUserXAlarms::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropUserXAlarms)
      // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

void CuDlgDomPropUserXAlarms::AddUserXAlarm (CuUserXAlarm* pUserXAlarm)
{
  ASSERT (pUserXAlarm);
  BOOL  bSpecial = pUserXAlarm->IsSpecialItem();

  int nCount, index;
  nCount = m_cListCtrl.GetItemCount ();

  // Alarm number
  if (bSpecial)
    index  = m_cListCtrl.InsertItem (nCount, "", 0);
  else {
    CString csLong;
    csLong.Format("%ld", pUserXAlarm->GetNumber());
    index  = m_cListCtrl.InsertItem (nCount, (LPCSTR)csLong, 0);
  }

  // Alarm table and db name
  if (bSpecial) {
    m_cListCtrl.SetItemText (index, 1, (LPCSTR)pUserXAlarm->GetStrName());
    m_cListCtrl.SetItemText (index, 2, (LPCSTR)pUserXAlarm->GetStrName());
  }
  else {
    m_cListCtrl.SetItemText (index, 1, (LPCSTR)pUserXAlarm->GetStrDbName());
    m_cListCtrl.SetItemText (index, 2, (LPCSTR)pUserXAlarm->GetStrTblName());
  }

  for (int cpt = 0; cpt < NBUSERXTBLALARMS; cpt++) {
    if (bSpecial)
      m_cListCtrl.SetItemText (index, cpt+3, _T(""));
    else {
      if (pUserXAlarm->GetFlag(aAlarmType[cpt]))
				m_cListCtrl.SetCheck (index, cpt+3, TRUE);
			else
				m_cListCtrl.SetCheck (index, cpt+3, FALSE);
      // don't disable the item,
      // since derived class CuListCtrlCheckmarks takes care of it
      //m_cListCtrl.EnableCheck(index, cpt+3, FALSE);
    }
  }

  // Attach pointer on alarm, for future sort purposes
  m_cListCtrl.SetItemData(index, (DWORD)pUserXAlarm);
}


BEGIN_MESSAGE_MAP(CuDlgDomPropUserXAlarms, CDialog)
    //{{AFX_MSG_MAP(CuDlgDomPropUserXAlarms)
    ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnClickList1)
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUserXAlarms message handlers

void CuDlgDomPropUserXAlarms::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropUserXAlarms::OnInitDialog() 
{
  CDialog::OnInitDialog();

  // subclass list control and connect image list
  VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageList.Create(16, 16, TRUE, 1, 1);
  m_ImageList.AddIcon(IDI_TREE_STATIC_DBALARM);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);

  CHECKMARK_COL_DESCRIBE  aColumns[LAYOUT_NUMBER] = {
    { _T("#"),              FALSE, 30 + 16},    // 16 for image width
    { _T("Database"),       FALSE, 80 },
    { _T("Table"),          FALSE, 80 },
    { _T("Select Success"), TRUE,  -1 },
    { _T("Select Failure"), TRUE,  -1 },
    { _T("Delete Success"), TRUE,  -1 },
    { _T("Delete Failure"), TRUE,  -1 },
    { _T("Insert Success"), TRUE,  -1 },
    { _T("Insert Failure"), TRUE,  -1 },
    { _T("Update Success"), TRUE,  -1 },
    { _T("Update Failure"), TRUE,  -1 },
  };
  InitializeColumns(m_cListCtrl, aColumns, LAYOUT_NUMBER);

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropUserXAlarms::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  if (!IsWindow (m_cListCtrl.m_hWnd))
      return;
  CRect r;
  GetClientRect (r);
  m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropUserXAlarms::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_LIST;
}

LONG CuDlgDomPropUserXAlarms::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
          pUps->pSFilter->UpdateType != OT_TABLE)
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
  m_Data.m_uaUserXAlarm.RemoveAll();

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
  for (int index = 0; index < NBUSERXTBLALARMS; index++) {
    iret =  DOMGetFirstRelObject(nNodeHandle,
                                 aAlarmType[index],
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
        // received data:
        // Number in geting(bufcomplim)
        // db name in aparentsresult[0]
        // table name in buf
        // Alarm name NOT CURRENTLY returned by low level call
        CuUserXAlarm alarm((long)getint(bufComplim),                // Alarm number,
                           VDBA_MfcResourceString (IDS_NOT_AVAILABLE), // Alarm name _T("<N/A>")
                           (const char *)aparentsResult[0],         // Db Name
                           (const char *)buf,                       // Table Name
                           aAlarmType[index]                        // alarm type
                          );
        CuMultFlag *pRefAlarm = m_Data.m_uaUserXAlarm.Find(&alarm);
        if (pRefAlarm)
          m_Data.m_uaUserXAlarm.Merge(pRefAlarm, &alarm);
        else
          m_Data.m_uaUserXAlarm.Add(alarm);

        iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
      }
    }
  }

  // Manage error case
  if (bError)
  {
    /* "<Data Unavailable>" */
    CuUserXAlarm alarm1(VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE));
    m_Data.m_uaUserXAlarm.Add(alarm1);
  }

  // Manage no alarms
  if (m_Data.m_uaUserXAlarm.GetCount() == 0)
  {
    /* "<No Alarm>" */
    CuUserXAlarm alarm2(VDBA_MfcResourceString (IDS_E_NO_ALARM));
    m_Data.m_uaUserXAlarm.Add(alarm2);
  }

  ASSERT (m_Data.m_uaUserXAlarm.GetCount() > 0 );

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropUserXAlarms::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataUserXAlarm") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataUserXAlarm* pData = (CuDomPropDataUserXAlarm*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataUserXAlarm)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;

}

LONG CuDlgDomPropUserXAlarms::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataUserXAlarm* pData = new CuDomPropDataUserXAlarm(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}


void CuDlgDomPropUserXAlarms::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh

  int cnt;
  int size;

  m_cListCtrl.DeleteAllItems();
  size = m_Data.m_uaUserXAlarm.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuUserXAlarm* pAlarm = m_Data.m_uaUserXAlarm[cnt];
    AddUserXAlarm(pAlarm);
  }
  // No Initial sort
}

void CuDlgDomPropUserXAlarms::OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  switch (pNMListView->iSubItem) {
    case 0:
      m_cListCtrl.SortItems(CuUserXAlarm::CompareNumbers, 0L);
      break;
    case 1:
      m_cListCtrl.SortItems(CuUserXAlarm::CompareDbNames, 0L);
      break;
    case 2:
      m_cListCtrl.SortItems(CuUserXAlarm::CompareTblNames, 0L);
      break;
    default:
      m_cListCtrl.SortItems(CuMultFlag::CompareFlags, pNMListView->iSubItem - 3);
      break;
  }
	*pResult = 0;
}

void CuDlgDomPropUserXAlarms::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!
  m_cListCtrl.DeleteAllItems();
  VDBA_OnGeneralSettingChange(&m_cListCtrl);
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
