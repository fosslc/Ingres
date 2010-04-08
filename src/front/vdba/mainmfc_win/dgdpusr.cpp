/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project : CA/OpenIngres Visual DBA
**    Source : dgdpusr.cpp
**
**  History:
**  15-Jun-2001(schph01)
**     SIR 104991 manage new controls to display the rmcmd grants.
** 31-Jul-2001 (schph01)
**  (bug 105349) when opening a session in imadb for getting the grants on
**  rmcmd, use SESSION_TYPE_INDEPENDENT instead of SESSION_TYPE_CACHENOREADLOCK
**  because SESSION_TYPE_CACHENOREADLOCK, in the case of imadb, executes the
**  ima procedures (set_server_domain and ima_set_vnode_domain) which are not needed for 
**  managing the grants on rmcmd, and result in grant errors if the connected
**  user has insufficient rights on imadb
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
** 10-Mar-2010 (drivi01)
**  As part of Visual DBA Usability changes, expand the columns
**  in "Create User" dialog to fit new headers on permissions.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdpusr.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
  #include "dbaginfo.h"
  #include "msghandl.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int LAYOUT_NUMBER = 3;


IMPLEMENT_DYNCREATE(CuDlgDomPropUser, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUser dialog


//
// NOTE: m_profile (0, TRUE, NULL) used here is to expose the list of preveleges only
CuDlgDomPropUser::CuDlgDomPropUser()
	: CFormView(CuDlgDomPropUser::IDD),  m_profile (0, TRUE, NULL)
{
	//{{AFX_DATA_INIT(CuDlgDomPropUser)
	//}}AFX_DATA_INIT
	m_Data.m_refreshParams.InitializeRefreshParamsType(OT_USER);
	m_bNeedSubclass = TRUE;
}

CuDlgDomPropUser::~CuDlgDomPropUser()
{
}

void CuDlgDomPropUser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropUser)
	DDX_Control(pDX, IDC_STATIC_RMCMD_TITLE, m_RmcmdTitle);
	DDX_Control(pDX, IDC_GRANT_REVOKE_RMCMD, m_ctrlCheckRmcmd);
	DDX_Control(pDX, IDC_ICON_PARTIALGRANT, m_ctrlIconPartial);
	DDX_Control(pDX, IDC_STATIC_USER_GRANTED, m_ctrlUserGranted);
	DDX_Control(pDX, IDC_PARTIAL_GRANT, m_ctrlPartialGrant);
	DDX_Control(pDX, IDC_LIST_GROUPS, m_lbGroupsContainingUser);
	DDX_Control(pDX, IDC_EDIT_LIMITSECLABEL, m_edLimSecureLabel);
	DDX_Control(pDX, IDC_EDIT_EXPIREDATE, m_edExpireDate);
	DDX_Control(pDX, IDC_EDIT_DEFPROFILE, m_edDefProfile);
	DDX_Control(pDX, IDC_EDIT_DEFGROUP, m_edDefGroup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropUser, CFormView)
	//{{AFX_MSG_MAP(CuDlgDomPropUser)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUser diagnostics

#ifdef _DEBUG
void CuDlgDomPropUser::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropUser::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropUser message handlers

void CuDlgDomPropUser::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropUser::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_BOTH;      // detail, plus list of OTR_USERGROUP
}

LONG CuDlgDomPropUser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      //case FILTER_DOM_SYSTEMOBJECTS:
      //case FILTER_DOM_BASEOWNER:
      //case FILTER_DOM_OTHEROWNER:
      break;

    case FILTER_DOM_BKREFRESH_DETAIL:
      if (m_Data.m_refreshParams.MustRefresh(pUps->pSFilter->bOnLoad, pUps->pSFilter->refreshtime))
        break;    // need to update
      else
        return 0; // no need to update
      break;

    default:
      return 0L;    // nothing to change on the display
  }

  m_ctrlIconPartial.ShowWindow(SW_HIDE);
  m_ctrlPartialGrant.ShowWindow(SW_HIDE);
  m_RmcmdTitle.ShowWindow(SW_HIDE);

  // Get info on the object
  USERPARAMS userParams;
  memset (&userParams, 0, sizeof (userParams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Manage "Public" special case
  //
  CString csPublic(lppublicdispstring());
  if (csPublic.CompareNoCase ((const char *)lpRecord->objName) == 0) {
    // Unacceptable for this property page since managed differently in AllocateUserPageInfo() (mainvi2.cpp)
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataUser tempData;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // starting from here, "Normal" case

  //
  // Get Detail Info
  //
  x_strcpy ((char *)userParams.ObjectName, (const char *)lpRecord->objName);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_USER,
                               &userParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data, except refresh info
    CuDomPropDataUser tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  if (userParams.DefaultGroup[0])
    m_Data.m_csDefGroup = userParams.DefaultGroup;
  else
    m_Data.m_csDefGroup.LoadString(IDS_DEFAULT_GROUP);// = "< No Default Group >";
  if (userParams.szProfileName[0])
    m_Data.m_csDefProfile = userParams.szProfileName;
  else
    m_Data.m_csDefProfile.LoadString(IDS_DEF_PROFILE);// = "< No Default Profile >";
  m_Data.m_csExpireDate = userParams.ExpireDate;
  if (userParams.lpLimitSecLabels)
    m_Data.m_csLimSecureLabel = userParams.lpLimitSecLabels;
  else
    m_Data.m_csLimSecureLabel = "";

  ASSERT (sizeof(m_Data.m_aPrivilege) == sizeof (userParams.Privilege) );
  memcpy(m_Data.m_aPrivilege, userParams.Privilege, sizeof (userParams.Privilege));
  ASSERT (sizeof(m_Data.m_aDefaultPrivilege) == sizeof (userParams.bDefaultPrivilege) );
  memcpy(m_Data.m_aDefaultPrivilege, userParams.bDefaultPrivilege, sizeof (userParams.bDefaultPrivilege));
  ASSERT (sizeof(m_Data.m_aSecAudit) == sizeof (userParams.bSecAudit) );
  memcpy(m_Data.m_aSecAudit, userParams.bSecAudit, sizeof (userParams.bSecAudit));

  char szConnectName[MAXOBJECTNAME*3];
  int iSession;

  if ( GetOIVers() >= OIVERS_26)
    wsprintf(szConnectName,"%s::imadb",GetVirtNodeName(nNodeHandle));
  else
    wsprintf(szConnectName,"%s::iidbdb",GetVirtNodeName(nNodeHandle));

  iResult = Getsession((LPUCHAR)szConnectName,SESSION_TYPE_INDEPENDENT, &iSession);
  if (iResult != RES_SUCCESS)
  {
    ErrorMessage((UINT) IDS_E_GET_SESSION, iResult);
  }
  else
  {
    iResult = GetInfGrantUser4Rmcmd(&userParams);
    if (iResult==RES_SUCCESS)
      ReleaseSession(iSession, RELEASE_COMMIT);
    else
    {
      ReleaseSession(iSession, RELEASE_ROLLBACK);
      ErrorMessage ((UINT) IDS_E_RETRIEVE_RMCMD_GRANT_FAILED, iResult);
    }
    m_Data.m_bGrantUsr4Rmcmd      = userParams.bGrantUser4Rmcmd;
    m_Data.m_bPartialGrantDefined = userParams.bPartialGrant;
  }

  // liberate detail structure
  FreeAttachedPointers (&userParams, OT_USER);

  //
  // Get list of groups containing user
  //
  m_Data.m_acsGroupsContainingUser.RemoveAll();

  int     iret;
  LPUCHAR aparentsTemp[MAXPLEVEL];

  LPUCHAR aparentsResult[MAXPLEVEL];
  UCHAR   bufParResult0[MAXOBJECTNAME];
  UCHAR   bufParResult1[MAXOBJECTNAME];
  UCHAR   bufParResult2[MAXOBJECTNAME];

  UCHAR   buf[MAXOBJECTNAME];
  UCHAR   bufOwner[MAXOBJECTNAME];
  UCHAR   bufComplim[MAXOBJECTNAME];

  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  memset (&bufComplim, '\0', sizeof(bufComplim));
  memset (&bufOwner, '\0', sizeof(bufOwner));
  aparentsTemp[0] = lpRecord->objName;
  aparentsTemp[1] = aparentsTemp[2] = NULL;

  iret =  DOMGetFirstRelObject(nNodeHandle,
                               OTR_USERGROUP,
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
    // Erroneous case!
    CString csUnavail = VDBA_MfcResourceString (IDS_DATA_UNAVAILABLE);//"<Data Unavailable>"
    m_Data.m_acsGroupsContainingUser.Add(csUnavail);
  }
  else {
    while (iret == RES_SUCCESS) {
      m_Data.m_acsGroupsContainingUser.Add((LPCTSTR)buf);
      iret = DOMGetNextRelObject(aparentsResult, buf, bufOwner, bufComplim);
    }
  }
  if (m_Data.m_acsGroupsContainingUser.GetUpperBound() == -1) {
    CString csNoGrp = VDBA_MfcResourceString (IDS_E_NO_GROUP);//"<No Group>"
    m_Data.m_acsGroupsContainingUser.Add(csNoGrp);
  }

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropUser::OnLoad (WPARAM wParam, LPARAM lParam)
{
  SubclassControls();
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataUser") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataUser* pData = (CuDomPropDataUser*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataUser)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropUser::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataUser* pData = new CuDomPropDataUser(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropUser::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Exclusively use member variables of m_Data for display refresh
	SubclassControls();
	//
	// m_cListPrivilege must have been subclassed !!!
	ASSERT (m_bNeedSubclass == FALSE);
	if (!m_bNeedSubclass)
	{
		int i=0;
		CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = m_profile.m_listPrivileges;
		POSITION pos = lp.GetHeadPosition();
		while (pos != NULL && i<USERPRIVILEGES)
		{
			CaURPPrivilegesItemData* pItem = lp.GetNext (pos);
			pItem->m_bUser = m_Data.m_aPrivilege[i];
			pItem->m_bDefault = m_Data.m_aDefaultPrivilege[i];
			i++;
		}
		m_cListPrivilege.DisplayPrivileges();
	}

  m_edDefGroup.SetWindowText(m_Data.m_csDefGroup);
  m_edDefProfile.SetWindowText(m_Data.m_csDefProfile);
  m_edExpireDate.SetWindowText(m_Data.m_csExpireDate);
  m_edLimSecureLabel.SetWindowText(m_Data.m_csLimSecureLabel);
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(m_Data.m_aSecAudit[USERALLEVENT]? 1: 0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(m_Data.m_aSecAudit[USERQUERYTEXTEVENT]? 1: 0);

  int cnt;
  int size;

  m_lbGroupsContainingUser.ResetContent();
  size = m_Data.m_acsGroupsContainingUser.GetSize();
  for (cnt = 0; cnt < size; cnt++)
    m_lbGroupsContainingUser.AddString(m_Data.m_acsGroupsContainingUser[cnt]);

  if ( m_Data.m_bGrantUsr4Rmcmd )
	{
    m_ctrlCheckRmcmd.SetCheck(BST_CHECKED);
	}
  else
    m_ctrlCheckRmcmd.SetCheck(BST_UNCHECKED);

  if (m_Data.m_bPartialGrantDefined)
  {
    HICON hIcon = theApp.LoadStandardIcon(IDI_EXCLAMATION);
    m_ctrlIconPartial.SetIcon(hIcon);
    m_ctrlIconPartial.Invalidate();
    DestroyIcon(hIcon);
    m_ctrlPartialGrant.ShowWindow(SW_SHOW);
    m_ctrlIconPartial.ShowWindow(SW_SHOW);
    m_RmcmdTitle.ShowWindow(SW_SHOW);
  }
  else
  {
    m_ctrlIconPartial.ShowWindow(SW_HIDE);
    m_ctrlPartialGrant.ShowWindow(SW_HIDE);
    m_RmcmdTitle.ShowWindow(SW_HIDE);
  }


}

void CuDlgDomPropUser::ResetDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  m_edDefGroup.SetWindowText(_T(""));
  m_edDefProfile.SetWindowText(_T(""));
  m_edExpireDate.SetWindowText(_T(""));
  m_edLimSecureLabel.SetWindowText(_T(""));
  ((CButton *)GetDlgItem(IDC_AUDIT_ALLEVENTS))->SetCheck(0);
  ((CButton *)GetDlgItem(IDC_AUDIT_QUERYTEXT))->SetCheck(0);

  m_lbGroupsContainingUser.ResetContent();
  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}

void CuDlgDomPropUser::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	SubclassControls();
	if (GetOIVers() >= OIVERS_30)
	{
		m_edLimSecureLabel.ShowWindow(SW_HIDE);
		CWnd* pWnd = GetDlgItem(IDC_MFC_STATIC1);
		if (pWnd && pWnd->m_hWnd)
			pWnd->ShowWindow(SW_HIDE);
	}
}


void CuDlgDomPropUser::SubclassControls()
{
	// Subclass HERE!
	if (m_bNeedSubclass) 
	{
		m_cListPrivilege.m_bReadOnly = TRUE;
		VERIFY (m_cListPrivilege.SubclassDlgItem (IDC_MFC_LIST3, this));
		m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
		m_cListPrivilege.SetCheckImageList(&m_ImageCheck);
		m_bNeedSubclass = FALSE;

		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		//
		LV_COLUMN lvcolumn;
		LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
		{
			{IDS_URP_PRIVILEGE,   100,  LVCFMT_LEFT,   FALSE},
			{IDS_URP_USER,         87,  LVCFMT_CENTER, TRUE },
			{IDS_URP_DEFAULT,      87,  LVCFMT_CENTER, TRUE },
		};

		CString strHeader;
		memset (&lvcolumn, 0, sizeof (LV_COLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (int i=0; i<LAYOUT_NUMBER; i++)
		{
			strHeader.LoadString (lsp[i].m_nIDS);
			lvcolumn.fmt      = lsp[i].m_fmt;
			lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx       = lsp[i].m_cxWidth;
			m_cListPrivilege.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
		}

		FillPrivileges();
	}
}

void CuDlgDomPropUser::FillPrivileges()
{
	TCHAR tchszText[] =_T("");
	int nCount = 0;
	LV_ITEM lvitem;
	CaURPPrivilegesItemData* pItem = NULL;
	CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = m_profile.m_listPrivileges;
	memset (&lvitem, 0, sizeof(lvitem));
	m_cListPrivilege.DeleteAllItems();
	POSITION pos = lp.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = lp.GetNext (pos);

		lvitem.mask   = LVIF_PARAM|LVIF_TEXT;
		lvitem.pszText= tchszText;
		lvitem.iItem  = nCount;
		lvitem.lParam = (LPARAM)pItem;

		m_cListPrivilege.InsertItem (&lvitem);
		nCount++;
	}
	m_cListPrivilege.DisplayPrivileges();
}
