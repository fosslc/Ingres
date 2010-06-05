/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpTb.cpp
**
**  Description:
**
**  History:
**	16-feb-2000 (somsa01)
**	    In OnUpdateData(), separated the adding of IDS_NO_CONSTRAINT
**	    into two statements to avoid compiler error.
**	24-Oct-2000 (schph01)
**	    sir 102821 Comment on table and columns.
** 10-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 25-Mar-2010 (drivi01)
**    Add Vectorwise structures as case statements for
**    displaying correct structures in the properties for the table.
**    Disable journaling for VectorWise tables in the properties page
**    b/c it doesn't apply to VectorWise tables.
*/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"      // FILTER_DOM_XYZ

#include "dgdptb.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static CTime GetCTime(CString& strDate)
{
	TCHAR tchsz[6][5];
	//
	// Actual format date to ba parsed is: dd-mmm-yyyy   hh:ss
	// ex: 12-dec-2002   00:00
	
	//
	// In this conversion, we assume that the date format is always in ENGLISH
	// Otherwise this conversion will not work 
	int i;
	TCHAR tchszMonth  [12][4] = 
	{
		_T("Jan"),
		_T("Feb"),
		_T("Mar"),
		_T("Apr"),
		_T("May"),
		_T("Jun"),
		_T("Jul"),
		_T("Aug"),
		_T("Sep"),
		_T("Oct"),
		_T("Nov"),
		_T("Dec")
	};


	if (strDate.GetLength() < 11)
	{
		throw (int)100;
	}

	// Year:
	tchsz[0][0] = strDate.GetAt( 7);
	tchsz[0][1] = strDate.GetAt( 8);
	tchsz[0][2] = strDate.GetAt( 9);
	tchsz[0][3] = strDate.GetAt(10);
	tchsz[0][4] = _T('\0');
	// Month:
	tchsz[1][0] = strDate.GetAt(3);
	tchsz[1][1] = strDate.GetAt(4);
	tchsz[1][2] = strDate.GetAt(5);
	tchsz[1][3] = _T('\0');
	BOOL bMonthOK = FALSE;
	for (i=0; i<12; i++)
	{
		if (_tcsicmp (tchsz[1], tchszMonth[i]) == 0)
		{
			wsprintf (tchsz[1], _T("%02d"), i+1);
			bMonthOK = TRUE;
			break;
		}
	}
	if (!bMonthOK)
	{
		// Default to the current month:
		CTime t = CTime::GetCurrentTime();
		wsprintf (tchsz[1], _T("%02d"), t.GetMonth());
	}
	// Day:
	tchsz[2][0] = strDate.GetAt(0);
	tchsz[2][1] = strDate.GetAt(1);
	tchsz[2][2] = _T('\0');

	CTime t(_ttoi (tchsz[0]), _ttoi (tchsz[1]), _ttoi (tchsz[2]), 0, 0, 0);

	return t;
}



IMPLEMENT_DYNCREATE(CuDlgDomPropTable, CFormView)

/////////////////////////////////////////////////////////////////////////////
// utility functions

static CString GetStorageStructureName(int structType)
{
  switch (structType) {
   case IDX_BTREE:
     return "Btree";
     break;
   case IDX_ISAM:
     return "Isam";
     break;
   case IDX_HASH:
     return "Hash";
     break;
   case IDX_HEAP:
     return "Heap";
     break;
   case IDX_HEAPSORT:
     return "Heapsort";
     break;
   case IDX_VW:
     return "vectorwise";
	 break;
   case IDX_VWIX:
     return "vectorwise_ix";
	 break;
   default :
     ASSERT (FALSE);
     return "Unknown";
     break;
  }
  ASSERT (FALSE);
  return "";
}

static int PgSizeInK(long lPageSize)
{
  ASSERT (! (lPageSize % 1024) );

  long lKSize = lPageSize / 1024;

  return (int)lKSize;
}


void CuDlgDomPropTable::AddTblConstraint(CuTblConstraint* pTblConstraint)
{
  ASSERT (pTblConstraint);
  BOOL  bSpecial = pTblConstraint->IsSpecialItem();

  CString csTmp;

  int nCount, index;
  nCount = m_cListConstraints.GetItemCount ();

  if (bSpecial) {
    // Special : Name in "type" column, and that's all
    index = m_cListConstraints.InsertItem (nCount, (LPCSTR)pTblConstraint->GetStrName(), 0);
  }
  else {
    // Type of constraint
    index = m_cListConstraints.InsertItem (nCount, (LPCSTR)pTblConstraint->CaptionFromType(), 0);

    // name
    m_cListConstraints.SetItemText (index, 1, (LPCSTR)pTblConstraint->GetStrName());
    CString strIndex = pTblConstraint->GetIndex();
    if (strIndex.IsEmpty())
       m_cListConstraints.SetItemText (index, 2, _T("No Index"));
    else
       m_cListConstraints.SetItemText (index, 2, strIndex);
  }

  // Attach pointer on constraint, for future sort purposes
  m_cListConstraints.SetItemData(index, (DWORD)pTblConstraint);
}



/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTable dialog


CuDlgDomPropTable::CuDlgDomPropTable()
  : CFormView(CuDlgDomPropTable::IDD)
{
  //{{AFX_DATA_INIT(CuDlgDomPropTable)
  //}}AFX_DATA_INIT

  m_bNeedSubclass = TRUE;
  m_Data.m_refreshParams.InitializeRefreshParamsType(OT_TABLE);
}

CuDlgDomPropTable::~CuDlgDomPropTable()
{
}

void CuDlgDomPropTable::DoDataExchange(CDataExchange* pDX)
{
  CFormView::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CuDlgDomPropTable)
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_edComment);
	DDX_Control(pDX, IDC_STATIC_UNIQ_STMT, m_stUniqStmt);
	DDX_Control(pDX, IDC_STATIC_UNIQ_ROW, m_stUniqRow);
	DDX_Control(pDX, IDC_STATIC_UNIQ_NO, m_stUniqNo);
	DDX_Control(pDX, IDC_STATIC_DUPROWS, m_stDupRows);
	DDX_Control(pDX, IDC_ST_STRUCTUNIQ, m_stStructUniq);
  DDX_Control(pDX, IDC_EXPIRE, m_sticon_expire);
  DDX_Control(pDX, IDC_EDIT_EXPIREDATE, m_edExpireDate);
  DDX_Control(pDX, IDC_EDIT_TBLTOTSIZE, m_edTotalSize);
  DDX_Control(pDX, IDC_BIGTABLE, m_sticon_bigtable);
  DDX_Control(pDX, IDC_STATIC_STRUCT2, m_stStruct2);
  DDX_Control(pDX, IDC_STATIC_STRUCT1, m_stStruct1);
  DDX_Control(pDX, IDC_STATIC_PGSIZE, m_stPgSize);
  DDX_Control(pDX, IDC_STATIC_FILLFACTOR, m_stFillFact);
  DDX_Control(pDX, IDC_EDIT_STRUCT2, m_edStruct2);
  DDX_Control(pDX, IDC_EDIT_STRUCT1, m_edStruct1);
  DDX_Control(pDX, IDC_EDIT_STRUCT, m_edStructure);
  DDX_Control(pDX, IDC_EDIT_SCHEMA, m_edSchema);
  DDX_Control(pDX, IDC_EDIT_PGSIZE, m_edPgSize);
  DDX_Control(pDX, IDC_EDIT_PGALLOCATED, m_edPgAlloc);
  DDX_Control(pDX, IDC_EDIT_NBROWS, m_edEstimNbRows);
  DDX_Control(pDX, IDC_EDIT_FILLFACTOR, m_edFillFact);
  DDX_Control(pDX, IDC_EDIT_EXTEND, m_edExtendPg);
  DDX_Control(pDX, IDC_EDIT_ALLOCATEDPAGES, m_edAllocatedPages);
  DDX_Control(pDX, IDC_CHECK_DUPROWS, m_chkDupRows);
  DDX_Control(pDX, IDC_READONLY, m_ReadOnly);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTable, CFormView)
  //{{AFX_MSG_MAP(CuDlgDomPropTable)
    ON_WM_SIZE()
  //}}AFX_MSG_MAP
    ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
    ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
    ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
    ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTable diagnostics

#ifdef _DEBUG
void CuDlgDomPropTable::AssertValid() const
{
    CFormView::AssertValid();
}

void CuDlgDomPropTable::Dump(CDumpContext& dc) const
{
    CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTable message handlers

void CuDlgDomPropTable::OnSize(UINT nType, int cx, int cy) 
{
    CFormView::OnSize(nType, cx, cy);
}

LONG CuDlgDomPropTable::OnQueryType(WPARAM wParam, LPARAM lParam)
{
  ASSERT (wParam == 0);
  ASSERT (lParam == 0);
  return DOMPAGE_DETAIL;
}

LONG CuDlgDomPropTable::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
    case FILTER_SETTING_CHANGE:
        VDBA_OnGeneralSettingChange(&m_cListConstraints);
        return 0L;

    default:
      return 0L;    // nothing to change on the display
  }

  // Get info on the object
  TABLEPARAMS tableparams;
  memset (&tableparams, 0, sizeof (tableparams));

  LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
  ASSERT (lpRecord);
  ResetDisplay();
  x_strcpy ((char *)tableparams.DBName,     (const char *)lpRecord->extra);
  x_strcpy ((char *)tableparams.objectname, RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName)));
  x_strcpy ((char *)tableparams.szSchema,   (const char *)lpRecord->ownerName);
  tableparams.detailSubset = TABLE_SUBSET_NOCOLUMNS;  // minimize query duration

  int dummySesHndl;
  int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(nNodeHandle),
                               OT_TABLE,
                               &tableparams,
                               FALSE,
                               &dummySesHndl);
  if (iResult != RES_SUCCESS) {
    ASSERT (FALSE);

    // Reset m_Data
    CuDomPropDataTable tempData;
    tempData.m_refreshParams = m_Data.m_refreshParams;
    m_Data = tempData;

    // Refresh display
    RefreshDisplay();

    return 0L;
  }

  TCHAR *lpObjComment = NULL;
  LPOBJECTLIST lpObjCol = NULL;

  iResult = VDBAGetCommentInfo ( (LPTSTR)(LPUCHAR)GetVirtNodeName(nNodeHandle),
                                 (LPTSTR)tableparams.DBName, (LPTSTR)tableparams.objectname,
                                 (LPTSTR)tableparams.szSchema, &lpObjComment, lpObjCol);
  if (iResult!=RES_SUCCESS)
    m_Data.m_csTableComment.LoadString(IDS_NOT_AVAILABLE);
  else if (lpObjComment)
  {
      m_Data.m_csTableComment = lpObjComment;
      ESL_FreeMem(lpObjComment);
  }
  else
      m_Data.m_csTableComment.Empty();

  // Update refresh info
  m_Data.m_refreshParams.UpdateRefreshParams();

  // update member variables, for display/load/save purpose
  m_Data.m_szSchema      = tableparams.szSchema;

  m_Data.m_cJournaling   = tableparams.cJournaling;
  m_Data.m_bDupRows      = tableparams.bDuplicates;
  m_Data.m_nStructure    = tableparams.StorageParams.nStructure;
  m_Data.m_nEstimNbRows  = tableparams.StorageParams.nNumRows;      // can be -1
  m_Data.m_nFillFact     = tableparams.StorageParams.nFillFactor;
  m_Data.m_nMinPages     = tableparams.StorageParams.nMinPages;     // Only for Hash
  m_Data.m_nMaxPages     = tableparams.StorageParams.nMaxPages;     // Only for Hash
  m_Data.m_nLeafFill     = tableparams.StorageParams.nLeafFill;     // Only for Btree
  m_Data.m_nNonLeafFill  = tableparams.StorageParams.nNonLeafFill;  // Only for Btree
  m_Data.m_lPgAlloc      = tableparams.StorageParams.lAllocation;
  m_Data.m_lExtend       = tableparams.StorageParams.lExtend;
  m_Data.m_lAllocatedPages = tableparams.StorageParams.lAllocatedPages;
  m_Data.m_nUnique       = tableparams.StorageParams.nUnique;       // No, row or statement
  m_Data.m_lPgSize       = tableparams.uPage_size;                  // Only for 2.0 and more
  m_Data.m_bReadOnly     = tableparams.bReadOnly;
  m_Data.m_bExpire = tableparams.bExpireDate;
  if (m_Data.m_bExpire)
    m_Data.m_csExpire = tableparams.szExpireDate;
  else
    m_Data.m_csExpire = _T("None");

  LPOBJECTLIST ls;

  // list of Constraints
  m_Data.m_uaConstraints.RemoveAll();
  ls = tableparams.lpConstraint;
  while (ls) {
    LPCONSTRAINTPARAMS lpCnst = (LPCONSTRAINTPARAMS)ls->lpObject;
    ASSERT (lpCnst);

    // Constraint name and type
    CString csName = lpCnst->lpText;
    TCHAR cType = lpCnst->cConstraintType;

    // item on the stack
    CuTblConstraint tblConstraint(cType, csName, lpCnst->tchszIndex);
    CuMultFlag *pRefConstr = m_Data.m_uaConstraints.Find(&tblConstraint);
    ASSERT (!pRefConstr);
    m_Data.m_uaConstraints.Add(tblConstraint);

    // Loop on next Constraint
    ls = (LPOBJECTLIST)ls->lpNext;
  }
  if (m_Data.m_uaConstraints.GetCount() == 0)
  {
    /* "< No Constraint >" */
    CuTblConstraint tblConstraint1(VDBA_MfcResourceString(IDS_NO_CONSTRAINT));
    m_Data.m_uaConstraints.Add(tblConstraint1);
  }
  ASSERT (m_Data.m_uaConstraints.GetCount());

  // liberate detail structure
  FreeAttachedPointers (&tableparams,  OT_TABLE);

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTable::OnLoad (WPARAM wParam, LPARAM lParam)
{
  // Precheck consistency of class name
  LPCTSTR pClass = (LPCTSTR)wParam;
  ASSERT (lstrcmp (pClass, "CuDomPropDataTable") == 0);

  // ensure controls subclassed
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }

  ResetDisplay();

  // Get class pointer, and check for it's class runtime type
  CuDomPropDataTable* pData = (CuDomPropDataTable*)lParam;
  ASSERT (pData->IsKindOf(RUNTIME_CLASS(CuDomPropDataTable)));

  // Update current m_Data using operator = overloading
  m_Data = *pData;

  // Refresh display
  RefreshDisplay();

  return 0L;
}

LONG CuDlgDomPropTable::OnGetData (WPARAM wParam, LPARAM lParam)
{
  // Allocate data class instance WITH NEW
  // DUPLICATE current m_Data in the allocated instance using copy constructor
  CuDomPropDataTable* pData = new CuDomPropDataTable(m_Data);

  // return it WITHOUT FREEING - will be freed by caller
  return (LRESULT)pData;
}

void CuDlgDomPropTable::RefreshDisplay()
{
  UpdateData (FALSE);   // Mandatory!

  // Must have already been subclassed by OnInitialUpdate()
  ASSERT (!m_bNeedSubclass);
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }

  //
  // Exclusively use member variables of m_Data for display refresh
  //
  CString csTxt;

  m_edSchema.SetWindowText(m_Data.m_szSchema);
  m_edComment.SetWindowText(m_Data.m_csTableComment);

  int idcRad = -1;
  switch (m_Data.m_cJournaling) {
    case _T('Y'):
      idcRad = IDC_RAD_JNL_ACTIVE;
      break;
    case _T('C'):
      idcRad = IDC_RAD_JNL_AFTERCKPDB;
      break;
    case _T('N'):
      idcRad = IDC_RAD_JNL_INACTIVE;
      break;
    default:
      ASSERT (FALSE);
      idcRad = -1;
  }
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, idcRad);

  m_chkDupRows.SetCheck(m_Data.m_bDupRows ? 1 : 0);
  m_ReadOnly.SetCheck(m_Data.m_bReadOnly ? 1 : 0);
  m_edStructure.SetWindowText(GetStorageStructureName(m_Data.m_nStructure));
  if (m_Data.m_nEstimNbRows == -1)
    m_edEstimNbRows.SetWindowText(VDBA_MfcResourceString (IDS_NOT_AVAILABLE));//_T("n/a")
  else {
    csTxt.Format(_T("%d"), m_Data.m_nEstimNbRows);
    m_edEstimNbRows.SetWindowText(csTxt);
  }

  // Data structure specific
  // hide all, then unmask specific
  m_stFillFact.ShowWindow(SW_HIDE);
  m_edFillFact.ShowWindow(SW_HIDE);
  m_stStruct1.ShowWindow(SW_HIDE);
  m_edStruct1.ShowWindow(SW_HIDE);
  m_stStruct2.ShowWindow(SW_HIDE);
  m_edStruct2.ShowWindow(SW_HIDE);
  // Show all controls for jounraling
  // only will be hidden for VectorWise tables
  CButton *m_cActive = (CButton *)GetDlgItem(IDC_RAD_JNL_ACTIVE);
  CButton *m_cInactive = (CButton *)GetDlgItem(IDC_RAD_JNL_INACTIVE);
  CButton *m_cAfterCKPDB = (CButton *)GetDlgItem(IDC_RAD_JNL_AFTERCKPDB);
  CStatic *m_sActive = (CStatic *)GetDlgItem(IDC_STATIC_JNL_ACTIVE);
  CStatic *m_sInactive = (CStatic *)GetDlgItem(IDC_STATIC_JNL_INACTIVE);
  CStatic *m_sAfterCKPDB = (CStatic *)GetDlgItem(IDC_STATIC_JNL_AFTERCKP);
  CStatic *m_sGroupJnl = (CStatic *)GetDlgItem(IDC_STATIC_JNL);
  m_cActive->ShowWindow(SW_SHOW);
  m_cInactive->ShowWindow(SW_SHOW);
  m_cAfterCKPDB->ShowWindow(SW_SHOW);
  m_sActive->ShowWindow(SW_SHOW);
  m_sInactive->ShowWindow(SW_SHOW);
  m_sAfterCKPDB->ShowWindow(SW_SHOW);
  m_sGroupJnl->ShowWindow(SW_SHOW);
  switch(m_Data.m_nStructure) {
    case IDX_BTREE:
      m_stFillFact.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nFillFact);
      m_edFillFact.ShowWindow(SW_SHOW);
      m_edFillFact.SetWindowText(csTxt);

      m_stStruct1.ShowWindow(SW_SHOW);
      m_stStruct1.SetWindowText(_T("Leaffill:"));
      m_edStruct1.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nLeafFill);
      m_edStruct1.SetWindowText(csTxt);

      m_stStruct2.ShowWindow(SW_SHOW);
      m_stStruct2.SetWindowText(_T("Non Leaffill:"));
      m_edStruct2.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nNonLeafFill);
      m_edStruct2.SetWindowText(csTxt);
      break;

    case IDX_ISAM:
      m_stFillFact.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nFillFact);
      m_edFillFact.ShowWindow(SW_SHOW);
      m_edFillFact.SetWindowText(csTxt);
      break;

    case IDX_HASH:
      m_stFillFact.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nFillFact);
      m_edFillFact.ShowWindow(SW_SHOW);
      m_edFillFact.SetWindowText(csTxt);

      m_stStruct1.ShowWindow(SW_SHOW);
      m_stStruct1.SetWindowText(_T("Min Pages:"));
      m_edStruct1.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nMinPages);
      m_edStruct1.SetWindowText(csTxt);

      m_stStruct2.ShowWindow(SW_SHOW);
      m_stStruct2.SetWindowText(_T("Max Pages:"));
      m_edStruct2.ShowWindow(SW_SHOW);
      csTxt.Format(_T("%d"), m_Data.m_nMaxPages);
      m_edStruct2.SetWindowText(csTxt);
      break;

    case IDX_HEAP:
    case IDX_HEAPSORT:
      // nothing appears
      break;
	case IDX_VW:
	case IDX_VWIX:
		m_cActive->ShowWindow(SW_HIDE);
		m_cInactive->ShowWindow(SW_HIDE);
		m_cAfterCKPDB->ShowWindow(SW_HIDE);
		m_sActive->ShowWindow(SW_HIDE);
		m_sInactive->ShowWindow(SW_HIDE);
		m_sAfterCKPDB->ShowWindow(SW_HIDE);
		m_sGroupJnl->ShowWindow(SW_HIDE);
		break;
    default :
      ASSERT (FALSE);
      break;
  }

  // Unicity related new section
  int nCmdShowUniq   = SW_HIDE;
  if (m_Data.m_nStructure != IDX_HEAP && m_Data.m_nStructure != IDX_HEAPSORT)
    nCmdShowUniq = SW_SHOW;
  int nCmdShowDuplic = SW_HIDE;
  if (!nCmdShowUniq || m_Data.m_nUnique == IDX_UNIQUE_NO)
    nCmdShowDuplic = SW_SHOW;

	m_stDupRows.ShowWindow(nCmdShowDuplic);
  m_chkDupRows.ShowWindow(nCmdShowDuplic);

  m_stStructUniq.ShowWindow(nCmdShowUniq);
	m_stUniqNo.ShowWindow(nCmdShowUniq);
  GetDlgItem(IDC_RAD_UNIQ_NO)->ShowWindow(nCmdShowUniq);
	m_stUniqRow.ShowWindow(nCmdShowUniq);
  GetDlgItem(IDC_RAD_UNIQ_ROW)->ShowWindow(nCmdShowUniq);
	m_stUniqStmt.ShowWindow(nCmdShowUniq);
  GetDlgItem(IDC_RAD_UNIQ_STMT)->ShowWindow(nCmdShowUniq);
  if (nCmdShowUniq == SW_SHOW) {
    int radId = -1;
    switch (m_Data.m_nUnique) {
      case IDX_UNIQUE_ROW:
        radId = IDC_RAD_UNIQ_ROW;
        break;
      case IDX_UNIQUE_STATEMENT:
        radId = IDC_RAD_UNIQ_STMT;
        break;
      case IDX_UNIQUE_NO:
        radId = IDC_RAD_UNIQ_NO;
        break;
      default:
        ASSERT (FALSE);
        radId = -1;
        break;
    }
    CheckRadioButton(IDC_RAD_UNIQ_NO, IDC_RAD_UNIQ_STMT, radId);
  }

  csTxt.Format(_T("%ld"), m_Data.m_lPgAlloc);
  m_edPgAlloc.SetWindowText(csTxt);

  csTxt.Format(_T("%ld"), m_Data.m_lExtend);
  m_edExtendPg.SetWindowText(csTxt);

  csTxt.Format(_T("%ld"), m_Data.m_lAllocatedPages);
  m_edAllocatedPages.SetWindowText(csTxt);

  int pgSizeInK = PgSizeInK(m_Data.m_lPgSize);
  csTxt.Format(_T("%d K"), pgSizeInK);
  m_edPgSize.SetWindowText(csTxt);

  // expiration date
  BOOL bExpire = m_Data.m_bExpire;
  m_sticon_expire.ShowWindow(bExpire? SW_SHOW: SW_HIDE);
  CString strExpireDate = m_Data.m_csExpire;
  try
  {
    CTime t = GetCTime(strExpireDate);
    strExpireDate = t.Format(_T("%x"));
    if (m_Data.m_csExpire.GetLength() > 11)
        strExpireDate += m_Data.m_csExpire.Mid(11);
  }
  catch(...)
  {
  }
  m_edExpireDate.SetWindowText(strExpireDate);

  // total size
  BOOL bInK = TRUE;
  BOOL bVeryBig = FALSE;
  long lTotal = m_Data.m_lAllocatedPages * (long)pgSizeInK;
  if (lTotal > 10l * 1024l) { // More than 10000K / 10 Megs ?
    lTotal /= 1024; // lTotal in Megs
    bInK = FALSE;   // unit is not K
    if (lTotal > 2l * 1024l)  // more than 2 gigs ?
      bVeryBig = TRUE;
  }
  if (bInK)
    csTxt.Format(_T("%ld K"), lTotal);
  else
    csTxt.Format(_T("%ld Mb"), lTotal);
  m_edTotalSize.SetWindowText(csTxt);
  m_sticon_bigtable.ShowWindow(bVeryBig? SW_SHOW: SW_HIDE);

  // List of constraints
  int size, cnt;
  m_cListConstraints.DeleteAllItems();
  size = m_Data.m_uaConstraints.GetCount();
  ASSERT (size > 0);
  for (cnt = 0; cnt < size; cnt++) {
    CuTblConstraint* pTblConstraint = m_Data.m_uaConstraints[cnt];
    AddTblConstraint(pTblConstraint);
  }

}

void CuDlgDomPropTable::SubclassControls()
{
  //
  // subclass constraints list control and connect image list
  //
  VERIFY (m_cListConstraints.SubclassDlgItem (IDC_LIST_CONSTRAINTS, this));
  m_ImageList2.Create(1, 18, TRUE, 1, 0);
  m_cListConstraints.SetImageList (&m_ImageList2, LVSIL_SMALL);
  m_ImageCheck2.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
  m_cListConstraints.SetCheckImageList(&m_ImageCheck2);

  #define CONSTR_NUMBER  3
  CHECKMARK_COL_DESCRIBE  aConstrColumns[CONSTR_NUMBER] = {
    { _T(""),     FALSE, 130 },
    { _T(""),     FALSE, 220 },
    { _T(""),     FALSE, 120 },
  };
  lstrcpy(aConstrColumns[0].szCaption, VDBA_MfcResourceString(IDS_TC_TYPE));
  lstrcpy(aConstrColumns[1].szCaption, VDBA_MfcResourceString(IDS_TC_TEXT));
  lstrcpy(aConstrColumns[2].szCaption, VDBA_MfcResourceString(IDS_TC_INDEX));
  InitializeColumns(m_cListConstraints, aConstrColumns, CONSTR_NUMBER);
}

void CuDlgDomPropTable::ResetDisplay()
{
  ASSERT (!m_bNeedSubclass);

  UpdateData (FALSE);   // Mandatory!

  m_edSchema.SetWindowText(_T(""));
  CheckRadioButton(IDC_RAD_JNL_ACTIVE, IDC_RAD_JNL_AFTERCKPDB, -1);
  m_chkDupRows.SetCheck(0);
  m_ReadOnly.SetCheck(0);
  m_edStructure.SetWindowText(_T(""));
  m_edEstimNbRows.SetWindowText(_T(""));

	m_stDupRows.ShowWindow(SW_HIDE);
  m_chkDupRows.ShowWindow(SW_HIDE);

  m_stStructUniq.ShowWindow(SW_HIDE);
	m_stUniqNo.ShowWindow(SW_HIDE);
  GetDlgItem(IDC_RAD_UNIQ_NO)->ShowWindow(SW_HIDE);
	m_stUniqRow.ShowWindow(SW_HIDE);
  GetDlgItem(IDC_RAD_UNIQ_ROW)->ShowWindow(SW_HIDE);
	m_stUniqStmt.ShowWindow(SW_HIDE);
  GetDlgItem(IDC_RAD_UNIQ_STMT)->ShowWindow(SW_HIDE);

  m_stFillFact.ShowWindow(SW_HIDE);
  m_edFillFact.ShowWindow(SW_HIDE);
  m_stStruct1.ShowWindow(SW_HIDE);
  m_edStruct1.ShowWindow(SW_HIDE);
  m_stStruct2.ShowWindow(SW_HIDE);
  m_edStruct2.ShowWindow(SW_HIDE);

  m_edPgAlloc.SetWindowText(_T(""));
  m_edExtendPg.SetWindowText(_T(""));
  m_edAllocatedPages.SetWindowText(_T(""));
  m_edPgSize.SetWindowText(_T(""));

  // expiration date
  m_sticon_expire.ShowWindow(SW_HIDE);
  m_edExpireDate.SetWindowText(_T(""));

  // total size
  m_edTotalSize.SetWindowText(_T(""));
  m_sticon_bigtable.ShowWindow(SW_HIDE);

  // List of constraints
  m_cListConstraints.DeleteAllItems();

  // Comment
  m_edComment.SetWindowText(_T(""));

  VDBA_OnGeneralSettingChange(&m_cListConstraints);

  // Force immediate update of all children controls
  RedrawAllChildWindows(m_hWnd);
}


void CuDlgDomPropTable::OnInitialUpdate() 
{
  CFormView::OnInitialUpdate();
  // Subclass HERE!
  if (m_bNeedSubclass) {
    SubclassControls();
    m_bNeedSubclass = FALSE;
  }
}
