// DgDpTRNA.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdptrna.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableRowsNA dialog


CuDlgDomPropTableRowsNA::CuDlgDomPropTableRowsNA(CWnd* pParent /*=NULL*/)
    : CDialog(CuDlgDomPropTableRowsNA::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropTableRowsNA)
        // NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CuDlgDomPropTableRowsNA::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropTableRowsNA)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableRowsNA, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropTableRowsNA)
    ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableRowsNA message handlers

void CuDlgDomPropTableRowsNA::PostNcDestroy() 
{
  delete this;    
  CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropTableRowsNA::OnInitDialog() 
{
  CDialog::OnInitDialog();

  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

LONG CuDlgDomPropTableRowsNA::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_SPECIAL;
}

LONG CuDlgDomPropTableRowsNA::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
      //case FILTER_DOM_BKREFRESH:
      //case FILTER_DOM_BKREFRESH_DETAIL:
      break;
    default:
      return 0L;    // nothing to change on the display
  }

  // NOTHING TO GET FROM LOW-LEVEL!

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTableRowsNA::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataTableRowsNA") == 0);

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataTableRowsNA* pData = (CuDomPropDataTableRowsNA*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataTableRowsNA)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTableRowsNA::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataTableRowsNA* pData = new CuDomPropDataTableRowsNA(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropTableRowsNA::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // NOTHING TO DISPLAY!
}
