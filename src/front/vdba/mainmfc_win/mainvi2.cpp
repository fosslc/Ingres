/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project : Ingres Visual DBA
**
**    MainVi2.cpp : implementation file for right pane of DOM
**
**    History after 01-01-2000
**      25-04-2000 (schph01)
**      bug 101246 Verify the Ingres database version before selected the 
**      title for the right pane properties 
**      12-Mar-2001 (schph01)
**      SIR 97068 For view in QUEL language, create a rigth pane with function
**      AllocateNotOiViewPageInfo().
**  21-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 20-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Apr-2003 (schph01)
**    SIR #107523, Add management for the Sequences.
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 06-Feb-2004 (schph01)
**    SIR 111752 Change the prototype for AllocateDatabasePageInfo() function.
**    Add new right pane when DBEVENTS flag is enabled,
**    for managing DBEvent on Gateway.
** 28-Apr-2004 (uk$so01)
**    BUG #112221 / ISSUE 13403162
**    VC7 (.Net) does not call OnInitialUpdate first for this Frame/View/Doc but it does call
**    OnUpdate. So I write void InitUpdate(); that is called once either in OnUpdate or
**    OnInitialUpdate() to resolve this problem.
** 15-Sep-2004 (uk$so01)
**    VDBA BUG #113047,  Vdba load/save does not function correctly with the 
**    Right Pane of DOM/Table/Rows page.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"

#include "mainvi2.h"
#include "maindoc.h"
#include "childfrm.h"
#include "mainview.h"

#include "starutil.h"   // IsLocalNodeName
#include "cmdline.h"    // unicenter-driven things
#include "wmusrmsg.h"
#include "getdata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dba.h"
#include "main.h"
#include "dom.h"
#include "dbafile.h"

  // for dom tree access
#include "tree.h"
#include "treelb.e"

typedef struct  _sDomCreateData
{
  int       domCreateMode;                // dom creation mode: tearout,...
  HWND      hwndMdiRef;                   // referred mdi dom doc
  FILEIDENT idFile;                       // if loading: file id to read from
  LPDOMNODE lpsDomNode;                   // node created from previous name
} DOMCREATEDATA, FAR *LPDOMCREATEDATA;

extern DOMCREATEDATA globDomCreateData;

#include "dbaginfo.h"   // GWName related functions
#include "domdata.h"
};

//
// Local function: check to see if Microsoft Web Browser is installed.
// This function only looks up the UUID in the registry !
static BOOL IsInternetExplorerRigistered()
{
	HKEY hkResult = 0;
	long lRes = RegOpenKey (
		HKEY_CLASSES_ROOT,
		_T("CLSID\\{8856F961-340A-11D0-A96B-00C04FD705A2}"),
		&hkResult);
	if (hkResult)
		RegCloseKey(hkResult);
	if (lRes != ERROR_SUCCESS)
	{
		return FALSE;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView2

IMPLEMENT_DYNCREATE(CMainMfcView2, CView)

CMainMfcView2::CMainMfcView2()
{
  m_pDomTabDialog = NULL;
  m_bAlreadyInit = FALSE;
}

CMainMfcView2::~CMainMfcView2()
{
}

//
// Manage the different Help IDs if the IDD is shared between IPM
// and DOM:
UINT CMainMfcView2::GetHelpID() 
{
  UINT nIDD = m_pDomTabDialog? m_pDomTabDialog->GetCurrentHelpID(): 0;
  switch (nIDD)
  {
  case IDD_IPMDETAIL_LOCATION:
  case IDD_IPMPAGE_LOCATION:
    return (HELPID_DOM + nIDD);
  default:
    return nIDD;
  }
  return nIDD;
}


BEGIN_MESSAGE_MAP(CMainMfcView2, CView)
	//{{AFX_MSG_MAP(CMainMfcView2)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_QUERY_OPEN_CURSOR, OnQueryOpenCursor)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView2 drawing

void CMainMfcView2::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView2 diagnostics

#ifdef _DEBUG
void CMainMfcView2::AssertValid() const
{
	CView::AssertValid();
}

void CMainMfcView2::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainMfcView2 message handlers


int CMainMfcView2::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;

  // TODO: Add your specialized creation code here

  return 0;
}

void CMainMfcView2::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

  // adjust tab dialog to client area
  if (!(m_pDomTabDialog && IsWindow (m_pDomTabDialog->m_hWnd)))
    return;
  CRect r;
  GetClientRect (r);
  m_pDomTabDialog->MoveWindow (r);
}

BOOL CMainMfcView2::PreCreateWindow(CREATESTRUCT& cs) 
{
  // NEED to clip children
  cs.style |= WS_CLIPCHILDREN;

  return CView::PreCreateWindow(cs);
}

void CMainMfcView2::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	InitUpdate();
  if (m_pDomTabDialog) {
    // access hNode
    CSplitterWnd* pParent = (CSplitterWnd*)GetParent();    // The Splitter Window
    ASSERT (pParent);
    CMainMfcView*    pMainMfcView= (CMainMfcView*)pParent->GetPane (0,0);
    ASSERT (pMainMfcView);
    HWND hwndDoc = GetVdbaDocumentHandle(pMainMfcView->m_hWnd);
    ASSERT (hwndDoc);
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndDoc, 0);
    ASSERT (lpDomData);
    int hDomNode = lpDomData->psDomNode->nodeHandle;
    ASSERT (hDomNode != -1);
    CMainMfcDoc* pDoc = (CMainMfcDoc *)GetDocument();
	if (pDoc && pDoc->IsLoadedDoc())
		return;

    // notify tab dialog for update
    m_pDomTabDialog->SendMessage(WM_USER_IPMPAGE_UPDATEING, (WPARAM)hDomNode, (LPARAM)lHint);
  }
}

void CMainMfcView2::OnInitialUpdate() 
{
  CView::OnInitialUpdate();
  InitUpdate();
}

extern "C" void FreeDomPageInfo(HWND hwndDom, DWORD m_pPageInfo)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDom);
  ASSERT (pWnd);
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);
  CuDlgDomTabCtrl* pTabDlg  = pMainMfcDoc->GetTabDialog();
  ASSERT (pTabDlg);

  CuDomPageInformation* pPageInfo = (CuDomPageInformation*)m_pPageInfo;
  ASSERT (pPageInfo);
  delete pPageInfo;

  // flag for no more page if necessary
  if (pTabDlg->GetCurrentProperty())
    if (pTabDlg->GetCurrentProperty()->GetDomPageInfo() == pPageInfo)
      pTabDlg->GetCurrentProperty()->SetDomPageInfo(NULL);
}

static CuDomPageInformation* AllocateDomItemPageInfo(int iobjecttype, LPTREERECORD pItemData, int ingresVer, int hDomNode);
static CuDomPageInformation* AllocateDatabasePageInfo(int dbType, int ingresVer, int iDomNode);
static CuDomPageInformation* AllocateTablePageInfo(LPTREERECORD pItemData, int ingresVer, BOOL bRowsNotDisplayable);
static CuDomPageInformation* AllocateLocationPageInfo(int hDomNode);
static CuDomPageInformation* AllocateUserPageInfo(LPTREERECORD pItemData);
static CuDomPageInformation* AllocateRolePageInfo();
static CuDomPageInformation* AllocateProfilePageInfo();
static CuDomPageInformation* AllocateIndexPageInfo(LPTREERECORD pItemData, int ingresVer);
static CuDomPageInformation* AllocateIntegrityPageInfo();
static CuDomPageInformation* AllocateRulePageInfo();
static CuDomPageInformation* AllocateProcedurePageInfo(LPTREERECORD pItemData);
static CuDomPageInformation* AllocateSequencePageInfo();
static CuDomPageInformation* AllocateGroupPageInfo();
static CuDomPageInformation* AllocateViewPageInfo(LPTREERECORD pItemData, int ingresVer, BOOL bRowsNotDisplayable);
static CuDomPageInformation* AllocateCddsPageInfo();
static CuDomPageInformation* AllocateDbEventPageInfo();
static CuDomPageInformation* AllocateReplicPageInfo(int hDomNode, LPTREERECORD pItemData);
static CuDomPageInformation* AllocateLocationsPageInfo(int hDomNode);

// enhancement : pages on dom static items for security alarms and grants
static CuDomPageInformation* AllocateDbGranteesPageInfo();
static CuDomPageInformation* AllocateDbAlarmsPageInfo();
static CuDomPageInformation* AllocateTblAlarmsPageInfo();
static CuDomPageInformation* AllocateTblGranteesPageInfo();
static CuDomPageInformation* AllocateViewGranteesPageInfo();
static CuDomPageInformation* AllocateProcGranteesPageInfo();
static CuDomPageInformation* AllocateSeqGranteesPageInfo();

static CuDomPageInformation* AllocateConnectionPageInfo();
static CuDomPageInformation* AllocateSchemaPageInfo(int ingresVer);

// enhancement: pages on most static items
static CuDomPageInformation* AllocateStaticTablePageInfo(int dbType);
static CuDomPageInformation* AllocateStaticViewPageInfo(int dbType);
static CuDomPageInformation* AllocateStaticProcPageInfo(int dbType);
static CuDomPageInformation* AllocateStaticSeqPageInfo(int dbType);
static CuDomPageInformation* AllocateStaticSchemaPageInfo();
static CuDomPageInformation* AllocateStaticSynonymPageInfo();
static CuDomPageInformation* AllocateStaticDbEventPageInfo();

static CuDomPageInformation* AllocateStaticIntegrityPageInfo();
static CuDomPageInformation* AllocateStaticRulePageInfo();
static CuDomPageInformation* AllocateStaticIndexPageInfo(int dbType);
static CuDomPageInformation* AllocateStaticTableLocPageInfo();
static CuDomPageInformation* AllocateStaticGroupuserPageInfo();

static CuDomPageInformation* AllocateStaticDbPageInfo(int ingresVer);
static CuDomPageInformation* AllocateStaticProfilePageInfo();
static CuDomPageInformation* AllocateStaticUserPageInfo();
static CuDomPageInformation* AllocateStaticGroupPageInfo();
static CuDomPageInformation* AllocateStaticRolePageInfo();

// Ice
static CuDomPageInformation* AllocateIceDbuserPageInfo();
static CuDomPageInformation* AllocateIceDbconnectionPageInfo();
static CuDomPageInformation* AllocateIceRolePageInfo();
static CuDomPageInformation* AllocateIceWebuserPageInfo();
static CuDomPageInformation* AllocateIceProfilePageInfo();
static CuDomPageInformation* AllocateIceServerLocationPageInfo();
static CuDomPageInformation* AllocateIceServerVariablePageInfo();
static CuDomPageInformation* AllocateIceBunitPageInfo();
static CuDomPageInformation* AllocateStaticIceSecurityPageInfo();
static CuDomPageInformation* AllocateStaticIceRolePageInfo();
static CuDomPageInformation* AllocateStaticIceDbuserPageInfo();
static CuDomPageInformation* AllocateStaticIceConnectionPageInfo();
static CuDomPageInformation* AllocateStaticIceWebuserPageInfo();
static CuDomPageInformation* AllocateStaticIceProfilePageInfo();
static CuDomPageInformation* AllocateStaticIceWebuserRolePageInfo();
static CuDomPageInformation* AllocateStaticIceWebuserDbconnPageInfo();
static CuDomPageInformation* AllocateStaticIceProfileRolePageInfo();
static CuDomPageInformation* AllocateStaticIceProfileDbconnPageInfo();
static CuDomPageInformation* AllocateStaticIceServerPageInfo();
static CuDomPageInformation* AllocateStaticIceServerApplicationPageInfo();
static CuDomPageInformation* AllocateStaticIceServerLocationPageInfo();
static CuDomPageInformation* AllocateStaticIceServerVariablePageInfo();
static CuDomPageInformation* AllocateStaticInstallationSecurityPageInfo();
static CuDomPageInformation* AllocateStaticInstallationGranteesPageInfo();
static CuDomPageInformation* AllocateStaticInstallationAlarmsPageInfo();
static CuDomPageInformation* AllocateIceBunitFacetPageInfo();
static CuDomPageInformation* AllocateIceBunitPagePageInfo();
static CuDomPageInformation* AllocateIceBunitAccessDefInfo(int iobjecttype);
static CuDomPageInformation* AllocateStaticIceBunitPageInfo();
static CuDomPageInformation* AllocateIceBunitRolePageInfo();
static CuDomPageInformation* AllocateIceBunitDbuserPageInfo();
static CuDomPageInformation* AllocateStaticIceFacetPageInfo();
static CuDomPageInformation* AllocateStaticIcePagePageInfo();
static CuDomPageInformation* AllocateStaticIceLocationPageInfo();
static CuDomPageInformation* AllocateStaticIceBunitRolePageInfo();
static CuDomPageInformation* AllocateStaticIceBunitUserPageInfo();

static CuDomPageInformation* AllocateStaticIceFacetRolePageInfo();
static CuDomPageInformation* AllocateStaticIceFacetUserPageInfo();
static CuDomPageInformation* AllocateStaticIcePageRolePageInfo();
static CuDomPageInformation* AllocateStaticIcePageUserPageInfo();

CuDomPageInformation* GetDomItemPageInfo(int iobjecttype, LPTREERECORD pItemData, int ingresVer, int hDomNode)
{
  if (pItemData->bPageInfoCreated) {
    // Can be null if no right pane ---> no ASSERT on pItemData->m_pPageInfo;
    return (CuDomPageInformation*)pItemData->m_pPageInfo;
  }
  else {
    CuDomPageInformation* pPageInfo = AllocateDomItemPageInfo(iobjecttype, pItemData, ingresVer, hDomNode);    // can be null

    // if unicenter driven and tabonly requested: build a second pPageInfo with requested tab only
    if (pPageInfo) {
      if (IsUnicenterDriven()) {
        CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
        ASSERT (pDescriptor);
        ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
        if (pDescriptor->HasSpecifiedTabOnly()) {
          ASSERT (pDescriptor->HasTabCaption());
          if (pDescriptor->HasTabCaption()) {
            CString csTabCaption = pDescriptor->GetTabCaption();
            ASSERT (!csTabCaption.IsEmpty());
            int nbpages = pPageInfo->GetNumberOfPage();
            int relevantTabId = -1;
            for (int cpt=0; cpt < nbpages; cpt++) {
              UINT id = pPageInfo->GetTabID (cpt);
	            CString str;
	            str.LoadString (id);
              if (str.CompareNoCase(csTabCaption) == 0) {
                relevantTabId = cpt;
                break;
              }
            }
            ASSERT (relevantTabId != -1);
            if (relevantTabId != -1) {
              CuDomPageInformation* pOldPageInfo = pPageInfo;
              pPageInfo = NULL;

              UINT nTabID [1];
              nTabID [0] = pOldPageInfo->GetTabID(relevantTabId);
              UINT nDlgID [1];
              nDlgID [0] = pOldPageInfo->GetDlgID(relevantTabId);
              UINT nIDTitle = pOldPageInfo->GetIDTitle();
              CString csClassName = pOldPageInfo->GetClassName();  // "CuDomXYZ"
              pPageInfo = new CuDomPageInformation ((LPCTSTR)csClassName, 1, nTabID, nDlgID, nIDTitle);

              delete pOldPageInfo;
            }
          }
        }
      }
    }

    if (pPageInfo)
      pItemData->m_pPageInfo = (DWORD)pPageInfo;
    pItemData->bPageInfoCreated = TRUE;
    return pPageInfo;
  }
}

static BOOL HasUserInConnectName(int hDomNode)
{
  char szconnectuser[200];
  LPCTSTR nodeName = (LPCTSTR)GetVirtNodeName (hDomNode);
  BOOL bHasUserInConnectName = GetConnectUserFromString((LPUCHAR)nodeName, (LPUCHAR)szconnectuser);
  return bHasUserInConnectName;
}

static CuDomPageInformation* AllocateDomItemPageInfo(int iobjecttype, LPTREERECORD pItemData, int ingresVer, int hDomNode)
{
  BOOL bRowsNotDisplayable = HasUserInConnectName(hDomNode);
  switch (iobjecttype) {
    case OT_DATABASE:
      return AllocateDatabasePageInfo(pItemData->parentDbType, ingresVer, hDomNode);
      break;
    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      return AllocateTablePageInfo( pItemData, ingresVer, bRowsNotDisplayable);
      break;
    case OT_LOCATION:
      return AllocateLocationPageInfo(hDomNode);
      break;
    case OT_USER:
      return AllocateUserPageInfo(pItemData);
    case OT_ROLE:
      return AllocateRolePageInfo();
      break;
    case OT_PROFILE:
      return AllocateProfilePageInfo();
      break;
    case OT_INDEX:
      return AllocateIndexPageInfo(pItemData, ingresVer);
      break;
    case OT_INTEGRITY:
      return AllocateIntegrityPageInfo();
      break;
    case OT_RULE:
      return AllocateRulePageInfo();
      break;
    case OT_PROCEDURE:
    case OT_SCHEMAUSER_PROCEDURE:
      return AllocateProcedurePageInfo(pItemData);
      break;
    case OT_SEQUENCE:
      return AllocateSequencePageInfo();
      break;
    case OT_GROUP:
      return AllocateGroupPageInfo();
      break;
    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      return AllocateViewPageInfo(pItemData, ingresVer, bRowsNotDisplayable);
      break;
    case OT_REPLIC_CDDS:
      return AllocateCddsPageInfo();
      break;
    case OT_DBEVENT:
      return AllocateDbEventPageInfo();
      break;
    case OT_STATIC_REPLICATOR:
      return AllocateReplicPageInfo(hDomNode, pItemData);
      break;
    case OT_STATIC_LOCATION:
      return AllocateLocationsPageInfo(hDomNode);
      break;

    case OT_STATIC_DBGRANTEES:
      return AllocateDbGranteesPageInfo();
      break;
    case OT_STATIC_DBALARM:
      return AllocateDbAlarmsPageInfo();
      break;
    case OT_STATIC_SECURITY:
      return AllocateTblAlarmsPageInfo();
      break;
    case OT_STATIC_TABLEGRANTEES:
      return AllocateTblGranteesPageInfo();
      break;
    case OT_STATIC_VIEWGRANTEES:
      return AllocateViewGranteesPageInfo();
      break;
    case OT_STATIC_PROCGRANT_EXEC_USER:
      return AllocateProcGranteesPageInfo();
      break;
    case OT_STATIC_SEQGRANT_NEXT_USER:
      return AllocateSeqGranteesPageInfo();
      break;
    case OT_REPLIC_CONNECTION:
      return AllocateConnectionPageInfo();
      break;
    case OT_SCHEMAUSER:
      return AllocateSchemaPageInfo(ingresVer);
      break;

    // enhancement: pages on most static items
    // Static items of database
    case OT_STATIC_TABLE:
      return AllocateStaticTablePageInfo(pItemData->parentDbType);
      break;
    case OT_STATIC_VIEW:
      return AllocateStaticViewPageInfo(pItemData->parentDbType);
      break;
    case OT_STATIC_PROCEDURE:
      return AllocateStaticProcPageInfo(pItemData->parentDbType);
      break;
    case OT_STATIC_SEQUENCE:
      return AllocateStaticSeqPageInfo(pItemData->parentDbType);
      break;
    case OT_STATIC_SCHEMA:
      return AllocateStaticSchemaPageInfo();
      break;
    case OT_STATIC_SYNONYM:
      return AllocateStaticSynonymPageInfo();
      break;
    case OT_STATIC_DBEVENT:
      return AllocateStaticDbEventPageInfo();
      break;

    // Static items of table
    case OT_STATIC_INTEGRITY:
      return AllocateStaticIntegrityPageInfo();
      break;
    case OT_STATIC_RULE:
      return AllocateStaticRulePageInfo();
      break;
    case OT_STATIC_INDEX:
      return AllocateStaticIndexPageInfo(pItemData->parentDbType);
      break;
    case OT_STATIC_TABLELOCATION:
      return AllocateStaticTableLocPageInfo();
      break;
    case OT_STATIC_GROUPUSER:
      return AllocateStaticGroupuserPageInfo();
      break;

    // root branches
    case OT_STATIC_DATABASE:
      return AllocateStaticDbPageInfo(ingresVer);
      break;
    case OT_STATIC_PROFILE:
      return AllocateStaticProfilePageInfo();
      break;
    case OT_STATIC_USER:
      return AllocateStaticUserPageInfo();
      break;
    case OT_STATIC_GROUP:
      return AllocateStaticGroupPageInfo();
      break;
    case OT_STATIC_ROLE:
      return AllocateStaticRolePageInfo();
      break;

    // ICE branches
    case OT_ICE_DBUSER:
      return AllocateIceDbuserPageInfo();
      break;
    case OT_ICE_DBCONNECTION:
    case OT_ICE_WEBUSER_CONNECTION:
    case OT_ICE_PROFILE_CONNECTION:
      return AllocateIceDbconnectionPageInfo();
      break;
    case OT_ICE_ROLE:
    case OT_ICE_WEBUSER_ROLE:
    case OT_ICE_PROFILE_ROLE:
      return AllocateIceRolePageInfo();
      break;
    case OT_ICE_WEBUSER:
      return AllocateIceWebuserPageInfo();
      break;
    case OT_ICE_PROFILE:
      return AllocateIceProfilePageInfo();
      break;
    case OT_ICE_SERVER_LOCATION:
    case OT_ICE_BUNIT_LOCATION:
      return AllocateIceServerLocationPageInfo();
      break;
    case OT_ICE_SERVER_VARIABLE:
      return AllocateIceServerVariablePageInfo();
      break;
    case OT_ICE_BUNIT:
      return AllocateIceBunitPageInfo();
      break;
    case OT_STATIC_ICE_SECURITY:
      return AllocateStaticIceSecurityPageInfo();
      break;
    case OT_STATIC_ICE_ROLE:
      return AllocateStaticIceRolePageInfo();
      break;
    case OT_STATIC_ICE_DBUSER:
      return AllocateStaticIceDbuserPageInfo();
      break;
    case OT_STATIC_ICE_DBCONNECTION:
      return AllocateStaticIceConnectionPageInfo();
      break;
    case OT_STATIC_ICE_WEBUSER:
      return AllocateStaticIceWebuserPageInfo();
      break;
    case OT_STATIC_ICE_PROFILE:
      return AllocateStaticIceProfilePageInfo();
      break;
    case OT_STATIC_ICE_WEBUSER_ROLE:
      return AllocateStaticIceWebuserRolePageInfo();
      break;
    case OT_STATIC_ICE_WEBUSER_CONNECTION:
      return AllocateStaticIceWebuserDbconnPageInfo();
      break;
    case OT_STATIC_ICE_PROFILE_ROLE:
      return AllocateStaticIceProfileRolePageInfo();
      break;
    case OT_STATIC_ICE_PROFILE_CONNECTION:
      return AllocateStaticIceProfileDbconnPageInfo();
      break;
    case OT_STATIC_ICE_SERVER:
      return AllocateStaticIceServerPageInfo();
      break;
    case OT_STATIC_ICE_SERVER_APPLICATION:
      return AllocateStaticIceServerApplicationPageInfo();
      break;
    case OT_STATIC_ICE_SERVER_LOCATION:
      return AllocateStaticIceServerLocationPageInfo();
      break;
    case OT_STATIC_ICE_SERVER_VARIABLE:
      return AllocateStaticIceServerVariablePageInfo();
      break;
    case OT_STATIC_INSTALL_SECURITY:
      return AllocateStaticInstallationSecurityPageInfo();
      break;
    case OT_STATIC_INSTALL_GRANTEES:
      return AllocateStaticInstallationGranteesPageInfo();
      break;
    case OT_STATIC_INSTALL_ALARMS:
      return AllocateStaticInstallationAlarmsPageInfo();
      break;
    case OT_ICE_BUNIT_FACET:
      return AllocateIceBunitFacetPageInfo();
      break;
    case OT_ICE_BUNIT_PAGE:
      return AllocateIceBunitPagePageInfo();
      break;
    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_ROLE:
    case OT_ICE_BUNIT_PAGE_USER:
      return AllocateIceBunitAccessDefInfo(iobjecttype);
      break;
    case OT_STATIC_ICE_BUNIT:
      return AllocateStaticIceBunitPageInfo();
      break;
    case OT_ICE_BUNIT_SEC_ROLE:
      return AllocateIceBunitRolePageInfo();
      break;
    case OT_ICE_BUNIT_SEC_USER:
      return AllocateIceBunitDbuserPageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_FACET:
      return AllocateStaticIceFacetPageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_PAGE:
      return AllocateStaticIcePagePageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_LOCATION:
      return AllocateStaticIceLocationPageInfo();
      break;

    case OT_STATIC_ICE_BUNIT_SEC_ROLE:
      return AllocateStaticIceBunitRolePageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_SEC_USER:
      return AllocateStaticIceBunitUserPageInfo();
      break;

    case OT_STATIC_ICE_BUNIT_FACET_ROLE:
      return AllocateStaticIceFacetRolePageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_FACET_USER:
      return AllocateStaticIceFacetUserPageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
      return AllocateStaticIcePageRolePageInfo();
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_USER:
      return AllocateStaticIcePageUserPageInfo();
      break;

    default:
      return NULL;
  }
  return NULL;
}

static CuDomPageInformation* AllocateNotOiDatabasePageInfo(int iMdiNodeHandle)
{
  CuDomPageInformation *pPageInfo = NULL;
  UINT nLayout;
  UINT *lTab,*lDlg;
  UINT nTabIDEvt [4] = {
                     IDS_DOMPROP_DB_TBLS,
                     IDS_DOMPROP_DB_VIEWS,
                     IDS_DOMPROP_DB_SCHEMAS,
                     IDS_DOMPROP_DB_EVTS,
                    };
  UINT nDlgIDEvt [4] = {
                     IDD_DOMPROP_DB_TBLS,
                     IDD_DOMPROP_DB_VIEWS,
                     IDD_DOMPROP_DB_SCHEMAS,
                     IDD_DOMPROP_DB_EVTS,
                    };
  UINT nTabID [3] = {
                     IDS_DOMPROP_DB_TBLS,
                     IDS_DOMPROP_DB_VIEWS,
                     IDS_DOMPROP_DB_SCHEMAS,
                    };
  UINT nDlgID [3] = {
                     IDD_DOMPROP_DB_TBLS,
                     IDD_DOMPROP_DB_VIEWS,
                     IDD_DOMPROP_DB_SCHEMAS,
                    };

  NODESVRCLASSDATAMIN NodeInfo;
  LPCTSTR nodeName = (LPCTSTR)GetVirtNodeName (iMdiNodeHandle);
  lstrcpy((LPTSTR)NodeInfo.NodeName,nodeName);
  RemoveGWNameFromString(NodeInfo.NodeName);
  GetGWClassNameFromString((LPUCHAR)nodeName, NodeInfo.ServerClass);

  if (INGRESII_IsDBEventsEnabled ( &NodeInfo ))
  {
    lTab = nTabIDEvt;
    lDlg = nDlgIDEvt;
    nLayout = 4;
  }
  else
  {
    lTab = nTabID;
    lDlg = nDlgID;
    nLayout = 3;
  }
    UINT nIDTitle = IDS_DOMPROP_DB_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiDb", nLayout, lTab, lDlg, nIDTitle);
  }
  catch (...) {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateRegularDatabasePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [10] = {
                     IDS_DOMPROP_DB,
                     IDS_DOMPROP_DB_TBLS,
                     IDS_DOMPROP_DB_VIEWS,
                     IDS_DOMPROP_DB_PROCS,
                     IDS_OBJTYPES_SEQUENCES,
                     IDS_DOMPROP_DB_SCHEMAS,
                     IDS_DOMPROP_DB_SYNS,
                     IDS_DOMPROP_DB_GRANTEES,
                     IDS_DOMPROP_DB_EVTS,
                     IDS_DOMPROP_DB_ALARMS,
                    };
  UINT nDlgID [10] = {
                     IDD_DOMPROP_DB,
                     IDD_DOMPROP_DB_TBLS,
                     IDD_DOMPROP_DB_VIEWS,
                     IDD_DOMPROP_DB_PROCS,
                     IDD_DOMPROP_DB_SEQS,
                     IDD_DOMPROP_DB_SCHEMAS,
                     IDD_DOMPROP_DB_SYNS,
                     IDD_DOMPROP_DB_GRANTEES,
                     IDD_DOMPROP_DB_EVTS,
                     IDD_DOMPROP_DB_ALARMS,
                    };
  UINT nIDTitle = IDS_DOMPROP_DB_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomDb", 10, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateDistributedDatabasePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [5] = {
                     IDS_DOMPROP_STARDB_COMPONENT,
                     IDS_DOMPROP_DDB,
                     IDS_DOMPROP_DDB_TBLS,
                     IDS_DOMPROP_DDB_VIEWS,
                     IDS_DOMPROP_DDB_PROCS,
                    };
  UINT nDlgID [5] = {
                     IDD_DOMPROP_STARDB_COMPONENT,
                     IDD_DOMPROP_DDB,
                     IDD_DOMPROP_DDB_TBLS,
                     IDD_DOMPROP_DDB_VIEWS,
                     IDD_DOMPROP_DDB_PROCS,
                    };
  UINT nIDTitle = IDS_DOMPROP_DDB_TITLE;
  pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomDDb", 5, nTabID, nDlgID, nIDTitle);

 return pPageInfo;
}

static CuDomPageInformation* AllocateDatabasePageInfo(int dbType, int ingresVer, int iDomNode)
{
  // idms, datacomm, etc
  if (ingresVer == OIVERS_NOTOI)
    return AllocateNotOiDatabasePageInfo(iDomNode);

  // Open-Ingres
  switch(dbType) {
    case DBTYPE_REGULAR:
    case DBTYPE_COORDINATOR:
      return AllocateRegularDatabasePageInfo();
      break;
    case DBTYPE_DISTRIBUTED:
      return AllocateDistributedDatabasePageInfo();
      break;
    default:
      ASSERT (FALSE);
      return NULL;
  }
}

static CuDomPageInformation* AllocateRegularTablePageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [11] = {
                     IDS_DOMPROP_TABLE_COLUMNS,
                     IDS_DOMPROP_TABLE,
                     IDS_DOMPROP_TABLE_ROWS,
                     IDS_DOMPROP_TABLE_STATISTIC,
                     IDS_DOMPROP_TABLE_PAGES,
                     IDS_DOMPROP_TABLE_INTEG,
                     IDS_DOMPROP_TABLE_RULE,
                     IDS_DOMPROP_TABLE_ALARMS,
                     IDS_DOMPROP_TABLE_IDX,
                     IDS_DOMPROP_TABLE_LOC,
                     IDS_DOMPROP_TABLE_GRANTEES,
                    };
  UINT nDlgID [11] = {
                     IDD_DOMPROP_TABLE_COLUMNS,
                     IDD_DOMPROP_TABLE,
                     IDD_DOMPROP_TABLE_ROWS,
                     IDD_DOMPROP_TABLE_STATISTIC,
                     IDD_DOMPROP_TABLE_PAGES,
                     IDD_DOMPROP_TABLE_INTEG,
                     IDD_DOMPROP_TABLE_RULE,
                     IDD_DOMPROP_TABLE_ALARMS,
                     IDD_DOMPROP_TABLE_IDX,
                     IDD_DOMPROP_TABLE_LOC,
                     IDD_DOMPROP_TABLE_GRANTEES,
                    };
  UINT nIDTitle = IDS_DOMPROP_TABLE_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[2] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[2] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomTableNoRows", 11, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomTable", 11, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}


static CuDomPageInformation* AllocateStarNativeTablePageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {
                     IDS_DOMPROP_STARTBL_N,
                     IDS_DOMPROP_TABLE_ROWS,
                    };
  UINT nDlgID [2] = {
                     IDD_DOMPROP_STARTBL_L,
                     IDD_DOMPROP_TABLE_ROWS,
                    };
  UINT nIDTitle = IDS_DOMPROP_STARTBL_N_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarNativeTableNoRows", 2, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarNativeTable", 2, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStarLinkTablePageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {
                     IDS_DOMPROP_STARTBL_L,
                     IDS_DOMPROP_TABLE_ROWS,
                     IDS_DOMPROP_STARTBL_L_IDX,
                    };
  UINT nDlgID [3] = {
                     IDD_DOMPROP_STARTBL_L,
                     IDD_DOMPROP_TABLE_ROWS,
                     IDD_DOMPROP_STARTBL_L_IDX,
                    };
  UINT nIDTitle = IDS_DOMPROP_STARTBL_L_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarLinkTableNoRows", 3, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarLinkTable", 3, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateNotOiTablePageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {
                     IDS_DOMPROP_TABLE_COLUMNS,
                     //IDS_DOMPROP_TABLE_IDX,
                     IDS_DOMPROP_TABLE_ROWS
                    };
  UINT nDlgID [2] = {
                     IDD_DOMPROP_TABLE_COLUMNS,
                     //IDD_DOMPROP_TABLE_IDX,
                     IDD_DOMPROP_TABLE_ROWS
                    };
  UINT nIDTitle = IDS_DOMPROP_TABLE_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiTableNoRows", 2, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiTable", 2, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStarTablePageInfo(LPTREERECORD pItemData, BOOL bRowsNotDisplayable)
{
  int iType = getint(pItemData->szComplim + STEPSMALLOBJ);
  ASSERT (iType == OBJTYPE_STARNATIVE || iType == OBJTYPE_STARLINK);
  BOOL bNative = (iType == OBJTYPE_STARNATIVE ? TRUE: FALSE);
  if (bNative)
    return AllocateStarNativeTablePageInfo(bRowsNotDisplayable);
  else
    return AllocateStarLinkTablePageInfo(bRowsNotDisplayable);
}

static CuDomPageInformation* AllocateTablePageInfo(LPTREERECORD pItemData, int ingresVer, BOOL bRowsNotDisplayable)
{

  // Manage datacomm, idms, etc...
  if (ingresVer == OIVERS_NOTOI)
    return AllocateNotOiTablePageInfo(bRowsNotDisplayable);

  if (pItemData->parentDbType == DBTYPE_DISTRIBUTED)
    return AllocateStarTablePageInfo(pItemData, bRowsNotDisplayable);
  else
    return AllocateRegularTablePageInfo(bRowsNotDisplayable);
}

static CuDomPageInformation* AllocateLocationPageInfo(int hDomNode)
{
  CuDomPageInformation *pPageInfo = NULL;

  // Different panes whether the node is local or not
  BOOL bLocal = FALSE;
  ASSERT (hDomNode != -1);
  LPCTSTR nodeName = (LPCTSTR)GetVirtNodeName (hDomNode);

  if (IsLocalNodeName(nodeName, TRUE))
    bLocal = TRUE;

  if (bLocal) {
    //UINT nTabID [3] = {IDS_DOMPROP_LOC,      IDS_TAB_LOCATION_PAGE2, IDS_TAB_LOCATION_PAGE1};
    //UINT nDlgID [3] = {IDD_DOMPROP_LOCATION, IDD_IPMPAGE_LOCATION,   IDD_IPMDETAIL_LOCATION};
    UINT nTabID [3] = {IDS_TAB_LOCATION_PAGE1, IDS_TAB_LOCATION_PAGE2, IDS_TAB_LOCATION_PAGE3 };
    UINT nDlgID [3] = {IDD_IPMDETAIL_LOCATION, IDD_IPMPAGE_LOCATION,   IDD_DOMPROP_LOCATION   };
    UINT nIDTitle = IDS_DOMPROP_LOC_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomLocationLocal", 3, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
      throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_LOC};
    UINT nDlgID [1] = {IDD_DOMPROP_LOCATION};
    UINT nIDTitle = IDS_DOMPROP_LOC_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomLocationRemote", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
      throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateUserPageInfo(LPTREERECORD pItemData)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nIDTitle = IDS_DOMPROP_USER_TITLE;

  // Manage "Public"
  CString csPublic = lppublicdispstring();
  if (csPublic.CompareNoCase((LPCSTR)pItemData->objName) == 0) {
    UINT nTabID [8] = {
                       IDS_DOMPROP_USER_XALARMS,
                       IDS_DOMPROP_USER_XGRANTED_DB,
                       IDS_DOMPROP_USER_XGRANTED_TBL,
                       IDS_DOMPROP_USER_XGRANTED_VIEW,
                       IDS_DOMPROP_USER_XGRANTED_DBEV,
                       IDS_DOMPROP_USER_XGRANTED_PROC,
                       IDS_DOMPROP_USER_XGRANTED_ROLE,
                       IDS_DOMPROP_USER_XGRANTED_SEQ,
                      };
    UINT nDlgID [8] = {
                       IDD_DOMPROP_USER_XALARMS,
                       IDD_DOMPROP_USER_XGRANTED_DB,
                       IDD_DOMPROP_USER_XGRANTED_TBL,
                       IDD_DOMPROP_USER_XGRANTED_VIEW,
                       IDD_DOMPROP_USER_XGRANTED_DBEV,
                       IDD_DOMPROP_USER_XGRANTED_PROC,
                       IDD_DOMPROP_USER_XGRANTED_ROLE,
                       IDD_DOMPROP_USER_XGRANTED_SEQ,
                      };
    try
    {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomUserPublic",
                                             GetOIVers()>=OIVERS_30? 8: 7,
                                             nTabID, nDlgID, nIDTitle);
    }
    catch (...)
    {
        throw;
    }
  }
  else {
    UINT nTabID [9] = {
                       IDS_DOMPROP_USER,
                       IDS_DOMPROP_USER_XALARMS,
                       IDS_DOMPROP_USER_XGRANTED_DB,
                       IDS_DOMPROP_USER_XGRANTED_TBL,
                       IDS_DOMPROP_USER_XGRANTED_VIEW,
                       IDS_DOMPROP_USER_XGRANTED_DBEV,
                       IDS_DOMPROP_USER_XGRANTED_PROC,
                       IDS_DOMPROP_USER_XGRANTED_ROLE,
                       IDS_DOMPROP_USER_XGRANTED_SEQ,
                      };
    UINT nDlgID [9] = {
                       IDD_DOMPROP_USER,
                       IDD_DOMPROP_USER_XALARMS,
                       IDD_DOMPROP_USER_XGRANTED_DB,
                       IDD_DOMPROP_USER_XGRANTED_TBL,
                       IDD_DOMPROP_USER_XGRANTED_VIEW,
                       IDD_DOMPROP_USER_XGRANTED_DBEV,
                       IDD_DOMPROP_USER_XGRANTED_PROC,
                       IDD_DOMPROP_USER_XGRANTED_ROLE,
                       IDD_DOMPROP_USER_XGRANTED_SEQ,
                      };
    try
    {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomUser",
                                             GetOIVers()>=OIVERS_30? 9: 8,
                                             nTabID, nDlgID, nIDTitle);
    }
    catch (...)
    {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [7] = {
                     IDS_DOMPROP_ROLE,
                     IDS_DOMPROP_ROLE_XGRANTED_DB,
                     IDS_DOMPROP_ROLE_XGRANTED_TBL,
                     IDS_DOMPROP_ROLE_XGRANTED_VIEW,
                     IDS_DOMPROP_ROLE_XGRANTED_DBEV,
                     IDS_DOMPROP_ROLE_XGRANTED_PROC,
                     IDS_DOMPROP_ROLE_XGRANTED_SEQ,
                    };
  UINT nDlgID [7] = {
                     IDD_DOMPROP_ROLE,
                     IDD_DOMPROP_ROLE_XGRANTED_DB,
                     IDD_DOMPROP_ROLE_XGRANTED_TBL,
                     IDD_DOMPROP_ROLE_XGRANTED_VIEW,
                     IDD_DOMPROP_ROLE_XGRANTED_DBEV,
                     IDD_DOMPROP_ROLE_XGRANTED_PROC,
                     IDD_DOMPROP_ROLE_XGRANTED_SEQ,
                    };
  UINT nIDTitle = IDS_DOMPROP_ROLE_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomRole",
                                           GetOIVers()>=OIVERS_30? 7: 6,
                                           nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateProfilePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_PROF};
  UINT nDlgID [1] = {IDD_DOMPROP_PROF};
  UINT nIDTitle = IDS_DOMPROP_PROF_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomProfile", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateRegularIndexPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_INDEX, IDS_DOMPROP_INDEX_PAGES, IDS_DOMPROP_TABLE_STATISTIC};
  // IDD_DOMPROP_INDEX_PAGES becomes obsolete
  // The index uses the same IDD_DOMPROP_TABLE_STATISTIC
  UINT nDlgID [3] = {IDD_DOMPROP_INDEX, IDD_DOMPROP_TABLE_PAGES, IDD_DOMPROP_TABLE_STATISTIC};

  UINT nIDTitle = IDS_DOMPROP_INDEX_TITLE;
  pPageInfo = new CuDomPageInformation (_T("CuDomIndex"), 3, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}


static CuDomPageInformation* AllocateNotOiIndexPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_INDEX};
  UINT nDlgID [1] = {IDD_DOMPROP_INDEX};
  UINT nIDTitle = IDS_DOMPROP_INDEX_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiIndex", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateStarIndexPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_INDEX_L_SOURCE, IDS_DOMPROP_INDEX};
  UINT nDlgID [2] = {IDD_DOMPROP_INDEX_L_SOURCE, IDD_DOMPROP_INDEX};
  UINT nIDTitle = IDS_DOMPROP_STARINDEX_L_TITLE;

  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarIndex", 2, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIndexPageInfo(LPTREERECORD pItemData, int ingresVer)
{
  // Manage datacom, idms, etc...
  if (ingresVer == OIVERS_NOTOI)
    return AllocateNotOiIndexPageInfo();

  if (pItemData->parentDbType == DBTYPE_DISTRIBUTED)
    return AllocateStarIndexPageInfo();
  else
    return AllocateRegularIndexPageInfo();
}

static CuDomPageInformation* AllocateIntegrityPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_INTEG};
  UINT nDlgID [1] = {IDD_DOMPROP_INTEG};
  UINT nIDTitle = IDS_DOMPROP_INTEG_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIntegrity", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateRulePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_RULE};
  UINT nDlgID [1] = {IDD_DOMPROP_RULE};
  UINT nIDTitle = IDS_DOMPROP_RULE_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomRule", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
    throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateRegularProcedurePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_PROC, IDS_DOMPROP_PROC_GRANTEES };
  UINT nDlgID [2] = {IDD_DOMPROP_PROC, IDD_DOMPROP_PROC_GRANTEES };
  UINT nIDTitle = IDS_DOMPROP_PROC_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomProcedure", 2, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateStarProcedurePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_PROC_L_SOURCE };  // Obsolete: IDS_DOMPROP_STARPROC_L 
  UINT nDlgID [1] = {IDD_DOMPROP_PROC_L_SOURCE };  // Obsolete: IDD_DOMPROP_STARPROC_L 
  UINT nIDTitle = IDS_DOMPROP_STARPROC_L_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarProcedure", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


static CuDomPageInformation* AllocateProcedurePageInfo(LPTREERECORD pItemData)
{
  if (pItemData->parentDbType == DBTYPE_DISTRIBUTED)
    return AllocateStarProcedurePageInfo();
  else
    return AllocateRegularProcedurePageInfo();
}

static CuDomPageInformation* AllocateSequencePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_SEQ_DETAIL, IDS_DOMPROP_SEQ_GRANTEES,};
  UINT nDlgID [2] = {IDD_DOMPROP_SEQ, IDD_DOMPROP_SEQ_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_SEQ_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomSequence", 2, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}


static CuDomPageInformation* AllocateGroupPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [7] = {
                     IDS_DOMPROP_GROUP,
                     IDS_DOMPROP_GROUP_XGRANTED_DB,
                     IDS_DOMPROP_GROUP_XGRANTED_TBL,
                     IDS_DOMPROP_GROUP_XGRANTED_VIEW,
                     IDS_DOMPROP_GROUP_XGRANTED_DBEV,
                     IDS_DOMPROP_GROUP_XGRANTED_PROC,
                     IDS_DOMPROP_GROUP_XGRANTED_SEQ,
                    };
  UINT nDlgID [7] = {
                     IDD_DOMPROP_GROUP,
                     IDD_DOMPROP_GROUP_XGRANTED_DB,
                     IDD_DOMPROP_GROUP_XGRANTED_TBL,
                     IDD_DOMPROP_GROUP_XGRANTED_VIEW,
                     IDD_DOMPROP_GROUP_XGRANTED_DBEV,
                     IDD_DOMPROP_GROUP_XGRANTED_PROC,
                     IDD_DOMPROP_GROUP_XGRANTED_SEQ,
                    };
  UINT nIDTitle = IDS_DOMPROP_GROUP_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomGroup",
                                           GetOIVers()>=OIVERS_30? 7: 6,
                                           nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateRegularViewPageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_VIEW, IDS_DOMPROP_VIEW_GRANTEES, IDS_DOMPROP_TABLE_ROWS};
  UINT nDlgID [3] = {IDD_DOMPROP_VIEW, IDD_DOMPROP_VIEW_GRANTEES, IDD_DOMPROP_TABLE_ROWS};
  UINT nIDTitle = IDS_DOMPROP_VIEW_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[2] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[2] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomViewNoRows", 3, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomView", 3, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStarNativeViewPageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_STARVIEW_N, IDS_DOMPROP_TABLE_ROWS};   // N/A -> removed: IDS_DOMPROP_VIEW_N_SOURCE, 
  UINT nDlgID [2] = {IDD_DOMPROP_STARVIEW_N, IDD_DOMPROP_TABLE_ROWS};   // N/A -> removed: IDD_DOMPROP_VIEW_N_SOURCE, 
  UINT nIDTitle = IDS_DOMPROP_STARVIEW_N_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarNativeViewNoRows", 2, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarNativeView", 2, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStarLinkViewPageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_VIEW_L_SOURCE, IDS_DOMPROP_TABLE_ROWS};    // Obsolete: IDS_DOMPROP_STARVIEW_L
  UINT nDlgID [2] = {IDD_DOMPROP_VIEW_L_SOURCE, IDD_DOMPROP_TABLE_ROWS };   // Obsolete: IDD_DOMPROP_STARVIEW_L
  UINT nIDTitle = IDS_DOMPROP_STARVIEW_L_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarLinkViewNoRows", 2, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStarLinkView", 2, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStarViewPageInfo(LPTREERECORD pItemData, BOOL bRowsNotDisplayable)
{
  int iType = getint(pItemData->szComplim + 4 + STEPSMALLOBJ);
  ASSERT (iType == OBJTYPE_STARNATIVE || iType == OBJTYPE_STARLINK);
  BOOL bNative = (iType == OBJTYPE_STARNATIVE ? TRUE: FALSE);
  if (bNative)
    return AllocateStarNativeViewPageInfo(bRowsNotDisplayable);
  else
    return AllocateStarLinkViewPageInfo(bRowsNotDisplayable);
}

static CuDomPageInformation* AllocateNotOiViewPageInfo(BOOL bRowsNotDisplayable)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_VIEW, IDS_DOMPROP_TABLE_ROWS};
  UINT nDlgID [2] = {IDD_DOMPROP_VIEW, IDD_DOMPROP_TABLE_ROWS};
  UINT nIDTitle = IDS_DOMPROP_VIEW_TITLE;
  if (bRowsNotDisplayable) {
    nTabID[1] = IDS_DOMPROP_TABLE_ROWSNA;
    nDlgID[1] = IDD_DOMPROP_TABLE_ROWSNA;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiViewNoRows", 2, nTabID, nDlgID, nIDTitle);
  }
  else
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiView", 2, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateViewPageInfo(LPTREERECORD pItemData, int ingresVer, BOOL bRowsNotDisplayable)
{
  // Manage datacomm, idms, etc...
  if (ingresVer == OIVERS_NOTOI)
    return AllocateNotOiViewPageInfo(bRowsNotDisplayable);

  if (pItemData->parentDbType == DBTYPE_DISTRIBUTED)
    return AllocateStarViewPageInfo(pItemData, bRowsNotDisplayable);
  else
  {
    int iSqlType = getint(pItemData->szComplim + 4 + (STEPSMALLOBJ*2));
    if (iSqlType)
      return AllocateRegularViewPageInfo(bRowsNotDisplayable);
    else
      return AllocateNotOiViewPageInfo(bRowsNotDisplayable);
  }
}


static CuDomPageInformation* AllocateCddsPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [2] = {IDS_DOMPROP_CDDS, IDS_DOMPROP_CDDS_TBL};
  UINT nDlgID [2] = {IDD_DOMPROP_CDDS, IDD_DOMPROP_CDDS_TBL};
  UINT nIDTitle = IDS_DOMPROP_CDDS_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomCdds", 2, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateDbEventPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DBEV_GRANTEES};
  UINT nDlgID [1] = {IDD_DOMPROP_DBEV_GRANTEES};
  UINT nIDTitle = IDS_DOMPROP_DBEV_TITLE;
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomDbEvent", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}


static CuDomPageInformation* AllocateReplicPageInfo(int hDomNode, LPTREERECORD pItemData)
{
  CuDomPageInformation *pPageInfo = NULL;


  int replicVersion;
  CString dbName = pItemData->extra;
  replicVersion = MonGetReplicatorVersion(hDomNode, dbName);

  // Special case for replicator not installed
  if (replicVersion == REPLIC_NOINSTALL) {
    UINT nIDTitle = IDS_DOMPROP_REPLNO_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomReplicNO", 0, NULL, NULL, nIDTitle);
    }
    catch (...) {
      throw;
    }
    return pPageInfo;
  }

  if (replicVersion != REPLIC_V11) {
    ASSERT (FALSE);
    UINT nIDTitle = IDS_DOMPROP_REPLNA_TITLE; // Not available
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomReplicNA", 0, NULL, NULL, nIDTitle);
    }
    catch (...) {
      throw;
    }
    return pPageInfo;
  }

  UINT nTabID [4] = {IDS_DOMPROP_REPL_CDDS,
                     IDS_DOMPROP_REPL_CONN,
                     IDS_DOMPROP_REPL_MAILU,
                     IDS_DOMPROP_REPL_REGTBL };
  UINT nDlgID [4] = {IDD_DOMPROP_REPL_CDDS,
                     IDD_DOMPROP_REPL_CONN,
                     IDD_DOMPROP_REPL_MAILU,
                     IDD_DOMPROP_REPL_REGTBL };
  UINT nIDTitle;
  if (GetOIVers() == OIVERS_20)
      nIDTitle = IDS_DOMPROP_REPL_TITLE;
  else
  {
      if (GetOIVers() >= OIVERS_25)
          nIDTitle = IDS_DOMPROP_REPL_TITLE2;
  }
  try
  {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomReplic11", 4, nTabID, nDlgID, nIDTitle);
  }
  catch (...)
  {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateLocationsPageInfo(int hDomNode)
{
  CuDomPageInformation *pPageInfo = NULL;

  // Only if node is local
  BOOL bLocal = FALSE;
  ASSERT (hDomNode != -1);
  LPCTSTR nodeName = (LPCTSTR)GetVirtNodeName (hDomNode);

  if (IsLocalNodeName(nodeName, TRUE))
    bLocal = TRUE;

  if (bLocal) {
    UINT nTabID [2] = {IDS_TAB_SUMMARY_LOCATIONS, IDS_DOMPROP_ST_LOCATION, };
    UINT nDlgID [2] = {IDD_IPMDETAIL_LOCATIONS, IDD_DOMPROP_ST_LOCATION, };
    UINT nIDTitle = IDS_TAB_LOCATIONS_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomLocationsLocal", 2, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
      throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_ST_LOCATION, };
    UINT nDlgID [1] = {IDD_DOMPROP_ST_LOCATION, };
    UINT nIDTitle = IDS_TAB_LOCATIONS_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomLocationsNotLocal", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
      throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateDbGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_DB_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomDbGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateDbAlarmsPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_ALARMS, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_ALARMS, };
  UINT nIDTitle = IDS_DOMPROP_DB_ALARMS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomDbAlarms", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateTblAlarmsPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_TABLE_ALARMS, };
  UINT nDlgID [1] = {IDD_DOMPROP_TABLE_ALARMS, };
  UINT nIDTitle = IDS_DOMPROP_TABLE_ALARMS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomTblAlarms", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateTblGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_TABLE_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_TABLE_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_TABLE_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomTblGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateViewGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_VIEW_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_VIEW_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_VIEW_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomViewGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateProcGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_PROC_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_PROC_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_PROC_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomProcGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateSeqGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_SEQ_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_SEQ_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_SEQ_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomSeqGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateConnectionPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_CONNECTION, };
  UINT nDlgID [1] = {IDD_DOMPROP_CONNECTION, };
  UINT nIDTitle = IDS_DOMPROP_CONNECTION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomConnection", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateSchemaPageInfo(int ingresVer)
{
  CuDomPageInformation *pPageInfo = NULL;

  // idms, datacomm, etc
  if (ingresVer == OIVERS_NOTOI) {
    UINT nTabID [2] = {IDS_DOMPROP_SCHEMA_TBLS,
                       IDS_DOMPROP_SCHEMA_VIEWS };
    UINT nDlgID [2] = {IDD_DOMPROP_SCHEMA_TBLS,
                       IDD_DOMPROP_SCHEMA_VIEWS };
    UINT nIDTitle = IDS_DOMPROP_SCHEMA_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomNotOiSchema", 2, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [3] = {IDS_DOMPROP_SCHEMA_TBLS,
                       IDS_DOMPROP_SCHEMA_VIEWS,
                       IDS_DOMPROP_SCHEMA_PROCS };
    UINT nDlgID [3] = {IDD_DOMPROP_SCHEMA_TBLS,
                       IDD_DOMPROP_SCHEMA_VIEWS,
                       IDD_DOMPROP_SCHEMA_PROCS };
    UINT nIDTitle = IDS_DOMPROP_SCHEMA_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomSchema", 3, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticTablePageInfo(int dbType)
{
  CuDomPageInformation *pPageInfo = NULL;

  if (dbType == DBTYPE_DISTRIBUTED) {
    UINT nTabID [1] = {IDS_DOMPROP_DDB_TBLS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DDB_TBLS, };
    UINT nIDTitle = IDS_DOMPROP_ST_STAR_TABLE_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticStarTable", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_DB_TBLS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DB_TBLS, };
    UINT nIDTitle = IDS_DOMPROP_ST_TABLE_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticTable", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticViewPageInfo(int dbType)
{
  CuDomPageInformation *pPageInfo = NULL;

  if (dbType == DBTYPE_DISTRIBUTED) {
    UINT nTabID [1] = {IDS_DOMPROP_DDB_VIEWS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DDB_VIEWS, };
    UINT nIDTitle = IDS_DOMPROP_ST_STAR_VIEW_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticStarView", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_DB_VIEWS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DB_VIEWS, };
    UINT nIDTitle = IDS_DOMPROP_ST_VIEW_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticView", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticProcPageInfo(int dbType)
{
  CuDomPageInformation *pPageInfo = NULL;

  if (dbType == DBTYPE_DISTRIBUTED) {
    UINT nTabID [1] = {IDS_DOMPROP_DDB_PROCS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DDB_PROCS, };
    UINT nIDTitle = IDS_DOMPROP_ST_STAR_PROC_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticStarProc", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_DB_PROCS, };
    UINT nDlgID [1] = {IDD_DOMPROP_DB_PROCS, };
    UINT nIDTitle = IDS_DOMPROP_ST_PROC_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticProc", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticSeqPageInfo(int dbType)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_OBJTYPES_SEQUENCES, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_SEQS, };
  UINT nIDTitle = IDS_DOMPROP_ST_DB_SEQ_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticSeq", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticSchemaPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_SCHEMAS, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_SCHEMAS, };
  UINT nIDTitle = IDS_DOMPROP_ST_SCHEMA_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticSchema", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateStaticSynonymPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_SYNS, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_SYNS, };
  UINT nIDTitle = IDS_DOMPROP_ST_SYN_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticSynonym", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateStaticDbEventPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_EVTS, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_EVTS, };
  UINT nIDTitle = IDS_DOMPROP_ST_EVT_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticDbEvent", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
 return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIntegrityPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_TABLE_INTEG, };
  UINT nDlgID [1] = {IDD_DOMPROP_TABLE_INTEG, };
  UINT nIDTitle = IDS_DOMPROP_ST_INTEG_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIntegrity", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticRulePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_TABLE_RULE, };
  UINT nDlgID [1] = {IDD_DOMPROP_TABLE_RULE, };
  UINT nIDTitle = IDS_DOMPROP_ST_RULE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticRule", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIndexPageInfo(int dbType)
{
  CuDomPageInformation *pPageInfo = NULL;

  if (dbType == DBTYPE_DISTRIBUTED) {
    UINT nTabID [1] = {IDS_DOMPROP_STARTBL_L_IDX, };
    UINT nDlgID [1] = {IDD_DOMPROP_STARTBL_L_IDX, };
    UINT nIDTitle = IDS_DOMPROP_ST_STAR_INDEX_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticStarIndex", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_TABLE_IDX, };
    UINT nDlgID [1] = {IDD_DOMPROP_TABLE_IDX, };
    UINT nIDTitle = IDS_DOMPROP_ST_INDEX_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIndex", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticTableLocPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_TABLE_LOC, };
  UINT nDlgID [1] = {IDD_DOMPROP_TABLE_LOC, };
  UINT nIDTitle = IDS_DOMPROP_ST_LOC_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticLocations", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


static CuDomPageInformation* AllocateStaticGroupuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_GROUP, };
  UINT nDlgID [1] = {IDD_DOMPROP_GROUP, };
  UINT nIDTitle = IDS_DOMPROP_ST_GROUPUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticGroupuser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticDbPageInfo(int ingresVer)
{
  CuDomPageInformation *pPageInfo = NULL;

  // Desktop, and idms, datacomm, etc
  if (ingresVer == OIVERS_NOTOI) {
    UINT nTabID [1] = {IDS_DOMPROP_ST_GWDB, };
    UINT nDlgID [1] = {IDD_DOMPROP_ST_GWDB, };
    UINT nIDTitle = IDS_DOMPROP_ST_GWDB_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticGwDb", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  else {
    UINT nTabID [1] = {IDS_DOMPROP_ST_DB, };
    UINT nDlgID [1] = {IDD_DOMPROP_ST_DB, };
    UINT nIDTitle = IDS_DOMPROP_ST_DB_TITLE;
    try {
      pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticDb", 1, nTabID, nDlgID, nIDTitle);
    }
    catch (...) {
        throw;
    }
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticProfilePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ST_PROFILE, };
  UINT nDlgID [1] = {IDD_DOMPROP_ST_PROFILE, };
  UINT nIDTitle = IDS_DOMPROP_ST_PROFILE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticProfile", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticUserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ST_USER, };
  UINT nDlgID [1] = {IDD_DOMPROP_ST_USER, };
  UINT nIDTitle = IDS_DOMPROP_ST_USER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticUser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


static CuDomPageInformation* AllocateStaticGroupPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ST_GROUP, };
  UINT nDlgID [1] = {IDD_DOMPROP_ST_GROUP, };
  UINT nIDTitle = IDS_DOMPROP_ST_GROUP_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticGroup", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


static CuDomPageInformation* AllocateStaticRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ST_ROLE, };
  UINT nDlgID [1] = {IDD_DOMPROP_ST_ROLE, };
  UINT nIDTitle = IDS_DOMPROP_ST_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

// Ice
static CuDomPageInformation* AllocateIceDbuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_DBUSER, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_DBUSER, };
  UINT nIDTitle = IDS_DOMPROP_ICE_DBUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceDbuser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceDbconnectionPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_DBCONNECTION, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_DBCONNECTION, };
  UINT nIDTitle = IDS_DOMPROP_ICE_DBCONNECTION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceDbconnection", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_ROLE, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_ROLE, };
  UINT nIDTitle = IDS_DOMPROP_ICE_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceWebuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_ICE_WEBUSER,
                     IDS_DOMPROP_ICE_WEBUSER_ROLES,
                     IDS_DOMPROP_ICE_WEBUSER_CONNECTIONS, };
  UINT nDlgID [3] = {IDD_DOMPROP_ICE_WEBUSER,
                     IDD_DOMPROP_ICE_WEBUSER_ROLES,
                     IDD_DOMPROP_ICE_WEBUSER_CONNECTIONS, };
  UINT nIDTitle = IDS_DOMPROP_ICE_WEBUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceWebuser", 3, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceProfilePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_ICE_PROFILE,
                     IDS_DOMPROP_ICE_PROFILE_ROLES,
                     IDS_DOMPROP_ICE_PROFILE_CONNECTIONS, };
  UINT nDlgID [3] = {IDD_DOMPROP_ICE_PROFILE,
                     IDD_DOMPROP_ICE_PROFILE_ROLES,
                     IDD_DOMPROP_ICE_PROFILE_CONNECTIONS, };
  UINT nIDTitle = IDS_DOMPROP_ICE_PROFILE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceProfile", 3, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceServerLocationPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SERVER_LOCATION, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SERVER_LOCATION, };
  UINT nIDTitle = IDS_DOMPROP_ICE_SERVER_LOCATION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceServerLocation", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceServerVariablePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SERVER_VARIABLE, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SERVER_VARIABLE, };
  UINT nIDTitle = IDS_DOMPROP_ICE_SERVER_VARIABLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceServerVariable", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceBunitPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [6] = {IDS_DOMPROP_ICE_BUNIT,
                     IDS_DOMPROP_ICE_BUNIT_ROLES,
                     IDS_DOMPROP_ICE_BUNIT_USERS,
                     IDS_DOMPROP_ICE_FACETS,
                     IDS_DOMPROP_ICE_PAGES,
                     IDS_DOMPROP_ICE_LOCATIONS, };
  UINT nDlgID [6] = {IDD_DOMPROP_ICE_BUNIT,
                     IDD_DOMPROP_ICE_BUNIT_ROLES,
                     IDD_DOMPROP_ICE_BUNIT_USERS,
                     IDD_DOMPROP_ICE_FACETS,
                     IDD_DOMPROP_ICE_PAGES,
                     IDD_DOMPROP_ICE_LOCATIONS, };
  UINT nIDTitle = IDS_DOMPROP_ICE_BUNIT_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunit", 6, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceSecurityPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [5] = {IDS_DOMPROP_ICE_SEC_ROLES,
                     IDS_DOMPROP_ICE_SEC_DBUSERS,
                     IDS_DOMPROP_ICE_SEC_DBCONNS,
                     IDS_DOMPROP_ICE_SEC_WEBUSERS,
                     IDS_DOMPROP_ICE_SEC_PROFILES,
                     };
  UINT nDlgID [5] = {IDD_DOMPROP_ICE_SEC_ROLES,
                     IDD_DOMPROP_ICE_SEC_DBUSERS,
                     IDD_DOMPROP_ICE_SEC_DBCONNS,
                     IDD_DOMPROP_ICE_SEC_WEBUSERS,
                     IDD_DOMPROP_ICE_SEC_PROFILES,
                     };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_SEC_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceSecurity", 5, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceDbuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_DBUSERS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_DBUSERS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_DBUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceDbuser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceConnectionPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_DBCONNS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_DBCONNS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_DBCONN_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceDbconn", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceWebuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_WEBUSERS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_WEBUSERS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_WEBUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceWebuser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceProfilePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_PROFILES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_PROFILES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PROFILE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceProfiles", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceWebuserRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_WEBUSER_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_WEBUSER_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_WEBUSER_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceWebuserRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceWebuserDbconnPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_WEBUSER_CONNECTIONS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_WEBUSER_CONNECTIONS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_WEBUSER_CONNECTION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceWebuserDbconn", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceProfileRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_PROFILE_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_PROFILE_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PROFILE_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceProfileRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceProfileDbconnPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_PROFILE_CONNECTIONS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_PROFILE_CONNECTIONS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PROFILE_CONNECTION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceProfileDbconn", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceServerPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_ICE_SERVER_APPLICATIONS,
                     IDS_DOMPROP_ICE_SERVER_LOCATIONS,
                     IDS_DOMPROP_ICE_SERVER_VARIABLES,
                     };
  UINT nDlgID [3] = {IDD_DOMPROP_ICE_SERVER_APPLICATIONS,
                     IDD_DOMPROP_ICE_SERVER_LOCATIONS,
                     IDD_DOMPROP_ICE_SERVER_VARIABLES,
                     };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_SERVER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceServer", 3, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceServerApplicationPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SERVER_APPLICATIONS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SERVER_APPLICATIONS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_SERVER_APPLICATION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceServerApplication", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceServerLocationPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SERVER_LOCATIONS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SERVER_LOCATIONS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_SERVER_LOCATION_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceServerLocation", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceServerVariablePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SERVER_VARIABLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SERVER_VARIABLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_SERVER_VARIABLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceServerApplication", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation*AllocateStaticInstallationSecurityPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [3] = {IDS_DOMPROP_INSTLEVEL_ENABLELEVEL, IDS_DOMPROP_INSTLEVEL_AUDITALLUSERS, IDS_DOMPROP_INSTLEVEL_AUDITLOG};
  UINT nDlgID [3] = {IDD_DOMPROP_INSTLEVEL_ENABLEDLEVEL, IDD_DOMPROP_INSTLEVEL_AUDITUSERS, IDD_DOMPROP_INSTLEVEL_AUDITLOG};
  UINT nIDTitle = IDS_DOMPROP_INSTLEVEL_TITLE;
    
  pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticInstallationSecurity", 3, nTabID, nDlgID, nIDTitle);
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticInstallationGranteesPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_GRANTEES, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_GRANTEES, };
  UINT nIDTitle = IDS_DOMPROP_INSTALL_GRANTEES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticInstallationGrantees", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticInstallationAlarmsPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_DB_ALARMS, };
  UINT nDlgID [1] = {IDD_DOMPROP_DB_ALARMS, };
  UINT nIDTitle = IDS_DOMPROP_INSTALL_ALARMS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticInstallationAlarms", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


static CuDomPageInformation* AllocateIceBunitFacetPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  if (IsInternetExplorerRigistered())
  {
    UINT nTabID [4] = {IDS_DOMPROP_ICE_TABHTML_DISPLAY, IDS_DOMPROP_ICE_FACET,      IDS_DOMPROP_ICE_PAGE_ROLES,       IDS_DOMPROP_ICE_PAGE_USERS,       };
    UINT nDlgID [4] = {IDD_DOMPROP_ICE_HTML_IMAGE, IDD_DOMPROP_ICE_FACETNPAGE, IDD_DOMPROP_ICE_FACETNPAGE_ROLES, IDD_DOMPROP_ICE_FACETNPAGE_USERS, };
    UINT nIDTitle = IDS_DOMPROP_ICE_FACET_TITLE;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitFacet", 4, nTabID, nDlgID, nIDTitle);
  }
  else
  {
    UINT nTabID [3] = {IDS_DOMPROP_ICE_FACET,      IDS_DOMPROP_ICE_PAGE_ROLES,       IDS_DOMPROP_ICE_PAGE_USERS,       };
    UINT nDlgID [3] = {IDD_DOMPROP_ICE_FACETNPAGE, IDD_DOMPROP_ICE_FACETNPAGE_ROLES, IDD_DOMPROP_ICE_FACETNPAGE_USERS, };
    UINT nIDTitle = IDS_DOMPROP_ICE_FACET_TITLE;
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitFacet", 3, nTabID, nDlgID, nIDTitle);
  }

  return pPageInfo;
}

static CuDomPageInformation* AllocateIceBunitPagePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [4] = {IDS_DOMPROP_ICE_TABHTML_DISPLAY, IDS_DOMPROP_ICE_PAGE,       IDS_DOMPROP_ICE_PAGE_ROLES,       IDS_DOMPROP_ICE_PAGE_USERS,       };
  UINT nDlgID [4] = {IDD_DOMPROP_ICE_HTML_SOURCE, IDD_DOMPROP_ICE_FACETNPAGE, IDD_DOMPROP_ICE_FACETNPAGE_ROLES, IDD_DOMPROP_ICE_FACETNPAGE_USERS, };
  UINT nIDTitle = IDS_DOMPROP_ICE_PAGE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitPage", 4, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceBunitAccessDefInfo(int iobjecttype)
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1];
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_BUNIT_ACCESSDEF, };
  UINT nIDTitle;
  switch (iobjecttype) {
    case OT_ICE_BUNIT_FACET_ROLE:
      nTabID[0] = IDS_DOMPROP_ICE_BUNIT_FACETROLE;
      nIDTitle  = IDS_DOMPROP_ICE_BUNIT_FACETROLE_TITLE;
      break;
    case OT_ICE_BUNIT_FACET_USER:
      nTabID[0] = IDS_DOMPROP_ICE_BUNIT_FACETUSER;
      nIDTitle  = IDS_DOMPROP_ICE_BUNIT_FACETUSER_TITLE;
      break;
    case OT_ICE_BUNIT_PAGE_ROLE:
      nTabID[0] = IDS_DOMPROP_ICE_BUNIT_PAGEROLE;
      nIDTitle  = IDS_DOMPROP_ICE_BUNIT_PAGEROLE_TITLE;
      break;
    case OT_ICE_BUNIT_PAGE_USER:
      nTabID[0] = IDS_DOMPROP_ICE_BUNIT_PAGEUSER;
      nIDTitle  = IDS_DOMPROP_ICE_BUNIT_PAGEUSER_TITLE;
      break;
    default:
      ASSERT (FALSE);
      return NULL;
  }

  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitAccessDefPage", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceBunitPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_BUNITS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_BUNITS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_BUNIT_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceBunit", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceBunitRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_ROLE, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_ROLE, };
  UINT nIDTitle = IDS_DOMPROP_ICE_SEC_ROLE_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitRole", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateIceBunitDbuserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_SEC_DBUSER, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_SEC_DBUSER, };
  UINT nIDTitle = IDS_DOMPROP_ICE_SEC_DBUSER_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomIceBunitDbuser", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceFacetPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_FACETS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_FACETS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_FACETS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceFacets", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIcePagePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_PAGES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_PAGES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PAGES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIcePages", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceLocationPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_LOCATIONS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_LOCATIONS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_LOCATIONS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceLocations", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceBunitRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_BUNIT_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_BUNIT_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_BUNIT_ROLES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceBunitRoles", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceBunitUserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_BUNIT_USERS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_BUNIT_USERS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_BUNIT_USERS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceBunitUsers", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceFacetRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_FACET_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_FACETNPAGE_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_FACET_ROLES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceFacetRoles", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIceFacetUserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_FACET_USERS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_FACETNPAGE_USERS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_FACET_USERS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIceFacetUsers", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIcePageRolePageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_PAGE_ROLES, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_FACETNPAGE_ROLES, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PAGE_ROLES_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIcePageRoles", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}

static CuDomPageInformation* AllocateStaticIcePageUserPageInfo()
{
  CuDomPageInformation *pPageInfo = NULL;

  UINT nTabID [1] = {IDS_DOMPROP_ICE_PAGE_USERS, };
  UINT nDlgID [1] = {IDD_DOMPROP_ICE_FACETNPAGE_USERS, };
  UINT nIDTitle = IDS_DOMPROP_ST_ICE_PAGE_USERS_TITLE;
  try {
    pPageInfo = new CuDomPageInformation ((LPCTSTR)"CuDomStaticIcePageUsers", 1, nTabID, nDlgID, nIDTitle);
  }
  catch (...) {
      throw;
  }
  return pPageInfo;
}


long CMainMfcView2::OnQueryOpenCursor(WPARAM wParam, LPARAM lParam)
{
	BOOL bExit = FALSE;
	if (!m_pDomTabDialog)
		return bExit;
	bExit = (BOOL)m_pDomTabDialog->SendMessage (WM_QUERY_OPEN_CURSOR, wParam, lParam);

	return (BOOL)bExit;
}


void CMainMfcView2::InitUpdate()
{
	if (m_bAlreadyInit)
		return;
  CMainMfcDoc* pDoc = (CMainMfcDoc*)GetDocument();	
  ASSERT (pDoc);

  // Create a child modeless dialog
  // and reference it in the Dom document
  try
  {
    m_pDomTabDialog = new CuDlgDomTabCtrl (this);

    if (!m_pDomTabDialog->Create (IDD_DOMDETAIL, this))
      AfxThrowResourceException();

    pDoc->SetTabDialog (m_pDomTabDialog);
	m_bAlreadyInit = TRUE;
  }
  catch (...)
  {
      throw;
  }

  // adjust the dialog to the view's client area
  CRect r;
  GetClientRect (r);
  m_pDomTabDialog->MoveWindow (r);
  m_pDomTabDialog->ShowWindow (SW_SHOW);

  //
  // manage right pane duplication when tearout and newwindow
  //
  if (globDomCreateData.domCreateMode == DOM_CREATE_NEWWINDOW
      || globDomCreateData.domCreateMode == DOM_CREATE_TEAROUT) {
    //
    // 1) Get current tab selection in reference dom
    //
    CWnd *pRefWnd;
    pRefWnd = CWnd::FromHandlePermanent(globDomCreateData.hwndMdiRef);
    ASSERT (pRefWnd);
    CMainMfcView *pRefView = (CMainMfcView*)pRefWnd;
    CMainMfcDoc *pRefMainMfcDoc = (CMainMfcDoc *)pRefView->GetDocument();
    ASSERT (pRefMainMfcDoc);
    CuDlgDomTabCtrl* pRefTabCtrl = pRefMainMfcDoc->GetTabDialog();
    ASSERT (pRefTabCtrl);
    int refCurSel = -1;
    refCurSel = pRefTabCtrl->m_cTab1.GetCurSel();
    // Note: refCurSel -1 means no tab at all

    //
    // 2) Update right pane according to current selection, PREVENTING low level call
    //
    ASSERT (!pDoc->m_pCurrentProperty);
    CSplitterWnd* pParent = (CSplitterWnd*)GetParent();    // The Splitter Window
    ASSERT (pParent);
    CMainMfcView*    pMainMfcView= (CMainMfcView*)pParent->GetPane (0,0);
    ASSERT (pMainMfcView);
    HWND hwndDoc = GetVdbaDocumentHandle(pMainMfcView->m_hWnd);
    ASSERT (hwndDoc);
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndDoc, 0);
    ASSERT (lpDomData);
    if (refCurSel != -1) {
      UpdateRightPane(hwndDoc, lpDomData, FALSE, refCurSel); // FALSE ---> won't call low level
      lpDomData->detailSelBeforeRClick = refCurSel;
    }
    else {
      UpdateRightPane(hwndDoc, lpDomData, FALSE, 0);         // FALSE ---> won't call low level
      lpDomData->detailSelBeforeRClick = 0;
    }

    //
    // 3) get serialisation class instance from reference window
    //
    CuIpmPropertyData* pRefData;
    // replaced GetDialogData() with other method in order to duplicate
    // ipm special stuff that was not originally designed for newwindow/tearout
    pRefData = pRefTabCtrl->GetDialogDataWithDup();
    // Note: pRefData null means no tabs

    //
    // 4) update the current right pane if necessary, and free serialization class instance
    //
    if (pRefData) {
      CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)GetDocument();
      ASSERT (pMainMfcDoc);
      CuDlgDomTabCtrl* pTabCtrl = pMainMfcDoc->GetTabDialog();
      ASSERT (pTabCtrl);
      pRefData->NotifyLoad(pTabCtrl->m_pCurrentPage);
      pTabCtrl->m_cTab1.SetCurSel(refCurSel);
      delete pRefData;

      CuDomProperty* pCurrentProperty = pDoc->GetTabDialog()->GetCurrentProperty();
      ASSERT (pCurrentProperty);
      pCurrentProperty->SetCurSel(refCurSel); // for future save
    }
  }
}
