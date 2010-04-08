/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipdb.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Header page..
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgiphead.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmPageHeader, CFormView)

CuDlgIpmPageHeader::CuDlgIpmPageHeader()
	: CFormView(CuDlgIpmPageHeader::IDD)
{
	m_bMustInitialize = TRUE;

	//{{AFX_DATA_INIT(CuDlgIpmPageHeader)
	m_strBlockCount = _T("");
	m_strBlockSize = _T("");
	m_strBufferCount = _T("");
	m_strLogFullInterval = _T("");
	m_strAbortInterval = _T("");
	m_strCPInterval = _T("");
	m_strBlockInUse = _T("");
	m_strBlockAvailable = _T("");
	m_strBegin = _T("");
	m_strEnd = _T("");
	m_strPreviousCP = _T("");
	m_strExpectedNextCP = _T("");
	m_strStatus = _T("");
	m_strReserved = _T("");
	m_strAvailable = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmPageHeader::~CuDlgIpmPageHeader()
{
}

void CuDlgIpmPageHeader::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageHeader)
	DDX_Control(pDX, IDC_STATIC_LOGDIAGRAM_CONTROL, m_LogDiagramIndicator);
	DDX_Text(pDX, IDC_EDIT1, m_strBlockCount);
	DDX_Text(pDX, IDC_EDIT2, m_strBlockSize);
	DDX_Text(pDX, IDC_EDIT3, m_strBufferCount);
	DDX_Text(pDX, IDC_EDIT4, m_strLogFullInterval);
	DDX_Text(pDX, IDC_EDIT5, m_strAbortInterval);
	DDX_Text(pDX, IDC_EDIT6, m_strCPInterval);
	DDX_Text(pDX, IDC_EDIT7, m_strBlockInUse);
	DDX_Text(pDX, IDC_EDIT8, m_strBlockAvailable);
	DDX_Text(pDX, IDC_EDIT9, m_strBegin);
	DDX_Text(pDX, IDC_EDIT10, m_strEnd);
	DDX_Text(pDX, IDC_EDIT11, m_strPreviousCP);
	DDX_Text(pDX, IDC_EDIT12, m_strExpectedNextCP);
	DDX_Text(pDX, IDC_EDIT13, m_strStatus);
	DDX_Text(pDX, IDC_EDIT_RESERVED, m_strReserved);
	DDX_Text(pDX, IDC_STATIC_AVAILABLE, m_strAvailable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageHeader, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmPageHeader)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageHeader diagnostics

#ifdef _DEBUG
void CuDlgIpmPageHeader::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmPageHeader::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageHeader message handlers


void CuDlgIpmPageHeader::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{

	if (m_bMustInitialize) {
		VERIFY (m_cListLegend.SubclassDlgItem (IDC_LIST1, this));
		m_bMustInitialize = FALSE;
	}

	if (bUndefined)
	{
		CString csUndefined = lpszNA;
		m_strBlockCount = csUndefined;
		m_strBlockSize = csUndefined;
		m_strBufferCount = csUndefined;
		m_strLogFullInterval = csUndefined;
		m_strAbortInterval = csUndefined;
		m_strCPInterval = csUndefined;
		m_strBlockInUse = csUndefined;
		m_strBlockAvailable = csUndefined;
		m_strBegin = csUndefined;
		m_strEnd = csUndefined;
		m_strPreviousCP = csUndefined;
		m_strExpectedNextCP = csUndefined;
		m_strStatus = csUndefined;

		// Added Emb 5/5/97
		m_strReserved   = csUndefined;
		m_strAvailable = csUndefined;

		// Log bof/eof and percentage in use
		m_LogDiagramIndicator.SetBofEofPercent(0, 0, 0, TRUE);

		// display N/A legend along with color symbols
		m_cListLegend.ResetContent();
		CString cs;
		cs.LoadString(IDS_I_RESERVED);   // = _T("Reserved: n/a");
		m_cListLegend.AddItem (cs, RGB(255, 255, 0)); // Yellow
		cs.LoadString(IDS_I_IN_USE);     // = _T("In use: n/a");
		m_cListLegend.AddItem (cs, RGB(0, 0, 192));   // Blue
		cs.LoadString(IDS_I_AVAILABLE);  // = _T("Available: n/a");
		m_cListLegend.AddItem (cs, RGB(0, 192, 0));   // Green
	}
	else
	{
		m_strBlockCount.Format (_T("%ld"), m_headStruct.block_count); 
		m_strBlockSize.Format (_T("%ld"), m_headStruct.block_size);
		m_strBufferCount.Format (_T("%ld"), m_headStruct.buf_count);
		m_strLogFullInterval.Format (_T("%ld"), m_headStruct.logfull_interval);
		m_strAbortInterval.Format (_T("%ld"), m_headStruct.abort_interval);
		m_strCPInterval.Format (_T("%ld"), m_headStruct.cp_interval);
		m_strBlockInUse.Format (_T("%ld"), m_headStruct.inuse_blocks);
		m_strBlockAvailable.Format (_T("%ld"), m_headStruct.avail_blocks);
		m_strBegin.Format (_T("%ld"), m_headStruct.begin_block);
		m_strEnd.Format (_T("%ld"), m_headStruct.end_block);
		m_strPreviousCP.Format (_T("%ld"), m_headStruct.cp_block);
		m_strExpectedNextCP.Format (_T("%ld"), m_headStruct.next_cp_block);
		m_strStatus = m_headStruct.status;

		// Log bof/eof and percentage in use
		// Use associativity left to right for * and /
		// Added Emb 5/5/97 "in use OR reserved" feature
		// obsolete: int pct = 100L * (m_headStruct.inuse_blocks + m_headStruct.reserved_blocks) / m_headStruct.block_count;
		double dblBof = (double)m_headStruct.begin_block * 100.0 / (double)m_headStruct.block_count;
		double dblEof = (double)m_headStruct.end_block	 * 100.0 / (double)m_headStruct.block_count;
		double dblRsv = (double)m_headStruct.reserved_blocks  * 100.0 / (double)m_headStruct.block_count;
		m_LogDiagramIndicator.SetBofEofPercent(dblBof, dblEof, dblRsv);

		// calculate percentages in double values
		double dblPctInUse = 0.0;
		if (m_headStruct.inuse_blocks > 0)
			dblPctInUse = (double)m_headStruct.inuse_blocks * 100.0 / (double)m_headStruct.block_count;
		double dblPctReserved = 0.0;
		if (m_headStruct.reserved_blocks > 0)
			dblPctReserved = (double)m_headStruct.reserved_blocks * 100.0 / (double)m_headStruct.block_count;
		double dblPctAvail = 100.0 - dblPctInUse - dblPctReserved;

		// display legend along with color symbols
		m_cListLegend.ResetContent();
		CString cs;
		cs.Format(IDS_F_RESERVED, dblPctReserved);//_T("Reserved (%0.2f%%)")
		m_cListLegend.AddItem (cs, RGB(255, 255, 0)); // Yellow
		cs.Format(IDS_F_IN_USED, dblPctInUse);//_T("In use (%0.2f%%)")
		m_cListLegend.AddItem (cs, RGB(0, 0, 192));   // Blue
		cs.Format(IDS_F_AVAILABLE, dblPctAvail);//_T("Available (%0.2f%%)")
		m_cListLegend.AddItem (cs, RGB(0, 192, 0));   // Green

		m_strReserved.Format(_T("%ld"), m_headStruct.reserved_blocks);
		BOOL bSuccess;
		if (m_headStruct.inuse_blocks + m_headStruct.reserved_blocks > m_headStruct.logfull_interval)
			bSuccess = m_strAvailable.LoadString(IDS_STATIC_AVAIL_LOGFULL);
		else if (m_headStruct.inuse_blocks + m_headStruct.reserved_blocks > m_headStruct.abort_interval)
			bSuccess = m_strAvailable.LoadString(IDS_STATIC_AVAIL_ABORT);
		else
			bSuccess = m_strAvailable.LoadString(IDS_STATIC_AVAIL_OK);
		ASSERT (bSuccess);
		if (!bSuccess)
			m_strAvailable = _T("   ");
	}
}

LONG CuDlgIpmPageHeader::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	LPIPMUPDATEPARAMS  pUps = (LPIPMUPDATEPARAMS)lParam;
	
	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_IPM_EXPRESS_REFRESH:
		break;
	default:
		return 0L;
	}

	try
	{
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		memset (&m_headStruct, 0, sizeof (m_headStruct));
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOGHEADER);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_headStruct);

		if (!bOK)
		{
			InitClassMembers (TRUE);
		}
		else
		{
			InitClassMembers ();
		}
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		InitClassMembers ();
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgIpmPageHeader::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOGHEADERDATAMIN pLh = (LPLOGHEADERDATAMIN)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageHeaders")) == 0);
	memcpy (&m_headStruct, pLh, sizeof (m_headStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageHeader::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageHeaders* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataPageHeaders (&m_headStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageHeader::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
