/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  dgdpseq.cpp : implementation file
**
** 02-Apr-2003 (schph01)
**    SIR #107523, Display the detail of sequence in property pane.
** 09-Apr-2003 (noifr01)
**   (sir 107523) management of sequences: background refresh of the
**   right pane
**
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgdpseq.h"
#include "domseri.h"
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

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeq dialog


CuDlgDomPropSeq::CuDlgDomPropSeq(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropSeq::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropSeq)
	m_csEdCache = _T("");
	m_csEdIncrement = _T("");
	m_csEdMaxVal = _T("");
	m_EdMinVal = _T("");
	m_csEdPrecision = _T("");
	m_csEdStartWith = _T("");
	m_bCache = FALSE;
	m_bCycle = FALSE;
	//}}AFX_DATA_INIT
	m_Data.m_refreshParams.InitializeRefreshParamsType(OT_SEQUENCE);
}


void CuDlgDomPropSeq::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropSeq)
	DDX_Text(pDX, IDC_EDIT_CACHE, m_csEdCache);
	DDX_Text(pDX, IDC_EDIT_INCREMENT, m_csEdIncrement);
	DDX_Text(pDX, IDC_EDIT_MAX_VALUE, m_csEdMaxVal);
	DDX_Text(pDX, IDC_EDIT_MIN_VALUE, m_EdMinVal);
	DDX_Text(pDX, IDC_EDIT_PRECISION, m_csEdPrecision);
	DDX_Text(pDX, IDC_EDIT_START_W, m_csEdStartWith);
	DDX_Check(pDX, IDC_CHECK_CACHE, m_bCache);
	DDX_Check(pDX, IDC_CHECK_CYCLE, m_bCycle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropSeq, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropSeq)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropSeq message handlers

void CuDlgDomPropSeq::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
}

void CuDlgDomPropSeq::PostNcDestroy() 
{
  delete this;
  CDialog::PostNcDestroy();
}

LONG CuDlgDomPropSeq::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropSeq::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (_tcscmp (pClass, "CuDomPropDataSequence") == 0);
  ResetDisplay();
  // Get class pointer, and check for it's class runtime type
  CuDomPropDataSequence* pData = (CuDomPropDataSequence*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataSequence)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropSeq::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW, and
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataSequence* pData = new CuDomPropDataSequence(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

LONG CuDlgDomPropSeq::OnUpdateData (WPARAM wParam, LPARAM lParam)
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

  // Get info on the object
  SEQUENCEPARAMS SequenceParams;
  memset (&SequenceParams, 0, sizeof (SEQUENCEPARAMS));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  //
  // Get Detail Info
  //
  x_strcpy ((char *)SequenceParams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)SequenceParams.objectowner, (const char *)lpRecord->ownerName);
  x_strcpy ((char *)SequenceParams.DBName, (const char *)lpRecord->extra);
  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_SEQUENCE,
                               &SequenceParams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataSequence tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_csNameSeq     = SequenceParams.objectname;
  m_Data.m_csOwnerSeq    = SequenceParams.objectowner;
  m_Data.m_bDecimalType  = SequenceParams.bDecimalType;
  m_Data.m_csPrecSeq     = SequenceParams.szDecimalPrecision;
  m_Data.m_csStartWith   = SequenceParams.szStartWith;
  m_Data.m_csIncrementBy = SequenceParams.szIncrementBy;
  m_Data.m_csMaxValue    = SequenceParams.szMaxValue;
  m_Data.m_csMinValue    = SequenceParams.szMinValue;
  if (_tcscmp((LPTSTR)SequenceParams.szCacheSize, _T("0")) != 0)
  {
    m_Data.m_bCache = TRUE;
    m_Data.m_csCacheSize = SequenceParams.szCacheSize;
  }
  else
  {
    m_Data.m_bCache = FALSE;
    m_Data.m_csCacheSize = _T("");
  }
  m_Data.m_bCycle        = SequenceParams.bCycle;

  // liberate detail structure
  FreeAttachedPointers (&SequenceParams, OT_SEQUENCE);

  //
  // Refresh display
  //
  RefreshDisplay();

  return 0L;
}

void CuDlgDomPropSeq::RefreshDisplay()
{
  // Exclusively use member variables of m_Data for display refresh

  m_csEdCache     = m_Data.m_csCacheSize;
  m_csEdIncrement = m_Data.m_csIncrementBy;
  m_csEdMaxVal    = m_Data.m_csMaxValue;
  m_EdMinVal      = m_Data.m_csMinValue;
  m_csEdPrecision = m_Data.m_csPrecSeq;
  m_csEdStartWith = m_Data.m_csStartWith;
  m_bCache        = m_Data.m_bCache;
  m_bCycle        = m_Data.m_bCycle;
  // Check Integer or Decimal radio button
  if (!m_Data.m_bDecimalType)
  {
    CheckRadioButton( IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL, IDC_RADIO_INTEGER );
    m_csEdPrecision = _T("");
  }
  else
    CheckRadioButton( IDC_RADIO_INTEGER, IDC_RADIO_DECIMAL, IDC_RADIO_DECIMAL );
  UpdateData (FALSE);   // Mandatory!

}

void CuDlgDomPropSeq::ResetDisplay()
{
  m_csEdCache     = _T("");
  m_csEdIncrement = _T("");
  m_csEdMaxVal    = _T("");
  m_EdMinVal      = _T("");
  m_csEdPrecision = _T("");
  m_csEdStartWith = _T("");
  m_bCache        = FALSE;
  m_bCycle        = FALSE;

  CheckDlgButton ( IDC_RADIO_INTEGER, BST_UNCHECKED);
  CheckDlgButton ( IDC_RADIO_DECIMAL, BST_UNCHECKED);

  UpdateData (FALSE);   // Mandatory!

  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}
