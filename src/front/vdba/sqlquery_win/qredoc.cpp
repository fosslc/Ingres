/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qredoc.cpp, Implementation file 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut
**    Purpose  : Document for (CRichEditView/CRichEditDoc) architecture.
**
** History:
**
** 31-Jan-2000 (uk$so01)
**    (Bug #100235)
**    Special SQL Test Setting when running on Context.
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 03-Oct-2003 (uk$so01)
**    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 19-Mar-2009 (drivi01)
**    Clean up warnings in effort to port to Visual Studio 2008.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "qredoc.h"
#include "qframe.h"
#include "qrecnitm.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// CLASS: CaSqlQueryMeasureData
// ************************************************************************************************
void CaSqlQueryMeasureData::Initialize()
{
	m_tStartTime    = 0; 
	m_tEndTime      = 0; 
	m_lBiocntStart  = 0;
	m_lCpuMsStart   = 0;
	m_lBiocntEnd    = 0;
	m_lCpuMsEnd     = 0;
	m_strCom      = _T("");
	m_strExec     = _T("");
	m_strCost     = _T("");
	m_strElap     = _T("");
	m_bAvailable  = TRUE;
	m_bStarted    = FALSE;
	m_bStartedEllapse = FALSE;
	m_lCom = 0;
	m_lExec= 0;
	m_lCost= 0;
	m_lElap= 0;
}


CaSqlQueryMeasureData::CaSqlQueryMeasureData()
{
	m_bGateway = FALSE;
	Initialize();
}


void CaSqlQueryMeasureData::SetAvailable (BOOL bAvailable)
{
	m_bAvailable  = bAvailable;
	if(m_bAvailable && !m_bGateway)
		m_strExec = _T("");
	else 
		m_strExec.LoadString(IDS_NOT_AVAILABLE);
	if (m_bAvailable && !m_bGateway)
		m_strCost = _T("");
	else
		m_strCost.LoadString (IDS_NOT_AVAILABLE);
	if (m_bAvailable)
		m_strElap = _T("");
	else
		m_strElap.LoadString (IDS_NOT_AVAILABLE);
}

void CaSqlQueryMeasureData::SetCompileTime (long lCompileTime)
{
	m_lCom = lCompileTime;
	if (m_lCom >= 0L)
		m_strCom.Format (_T("%ld ms"), m_lCom); 
	else
		m_strCom.LoadString (IDS_NOT_AVAILABLE);//_T("n/a")
}

void CaSqlQueryMeasureData::StopEllapse()
{
	ASSERT (m_bStarted);
	if (!m_bStarted)
		return;
	ASSERT (m_bStartedEllapse);
	if (!m_bStartedEllapse)
		return;
	m_bStartedEllapse = FALSE;
	m_tEndTime = CTime::GetCurrentTime();
	CTimeSpan tdiff  = m_tEndTime - m_tStartTime;
	m_lElap   += (long)tdiff.GetTotalSeconds();
}

void CaSqlQueryMeasureData::StartEllapse()
{
	ASSERT (m_bStarted);
	if (!m_bStarted)
		return;
	ASSERT (!m_bStartedEllapse);
	if (m_bStartedEllapse)
		return;
	m_bStartedEllapse = TRUE;
	m_tStartTime = CTime::GetCurrentTime();
}

static void SQLGetDbmsInfo(long& lplBio_cnt, long& lplCpu_ms)
{
	lplBio_cnt = -1;
	lplCpu_ms  = -1;
	CString strBio_cnt = INGRESII_llDBMSInfo(_T("_bio_cnt"));
	CString strBio_cpu = INGRESII_llDBMSInfo(_T("_cpu_ms"));

	lplBio_cnt = _ttol(strBio_cnt);
	lplCpu_ms  = _ttol(strBio_cpu);
}

void CaSqlQueryMeasureData::StartMeasure(BOOL bStartElap)
{
	Initialize();
	m_bStarted = TRUE;
	if (!m_bGateway)
		SQLGetDbmsInfo (m_lBiocntStart, m_lCpuMsStart);
	m_bStartedEllapse = bStartElap;
	if (m_bStartedEllapse)
		m_tStartTime = CTime::GetCurrentTime();
}

void CaSqlQueryMeasureData::StopMeasure (BOOL bElap)
{
	long ldiff = -1;
	ASSERT (m_bStarted);
	if (!m_bStarted)
		return;
	m_bStarted = FALSE;
	if (bElap)
	{
		ASSERT (m_bStartedEllapse);
		if (!m_bStartedEllapse)
			return;
		m_bStartedEllapse = FALSE;
		m_tEndTime = CTime::GetCurrentTime();
	}
	if (!m_bGateway)
		SQLGetDbmsInfo (m_lBiocntEnd, m_lCpuMsEnd);

	//
	// Exec
	if (m_bGateway)
	{
		m_strExec.LoadString (IDS_NOT_AVAILABLE);
		m_strCom.LoadString (IDS_NOT_AVAILABLE);
	}
	else
	{
		if (m_lCpuMsEnd>=0 && m_lCpuMsStart >=0) 
		{
			ldiff = m_lCpuMsEnd - m_lCpuMsStart;
			if (ldiff<0)
				ldiff=0;
			if (m_lExec > -1)
				m_lExec += ldiff;
		}
		else {
			ldiff = -1;
			m_lExec = -1;
		}

		if (m_lExec < 0)
			m_strExec.LoadString(IDS_NOT_AVAILABLE); //_T("n/a")
		else
			m_strExec.Format (_T("%ld ms "), m_lExec);
	}

	//
	// Elap
	if (bElap)
	{
		//
		// Elapse time available:
		m_tEndTime = CTime::GetCurrentTime();
		CTimeSpan tdiff  = m_tEndTime - m_tStartTime;

		// adjust elapsed time (granularity = second)
		// so that compile time (granularity = 1 ms) is not
		// bigger than the elapsed time because of granularity 
		// difference
		if (tdiff.GetTotalSeconds() < ((ldiff+999)/1000))
			m_lElap += (ldiff+999)/1000;
		else
			m_lElap += (long)tdiff.GetTotalSeconds();

		m_strElap.Format (_T("%ld s "), m_lElap);
	}
	else
		m_strElap.LoadString(IDS_NOT_AVAILABLE);//_T("n/a")

	//
	// Cost
	if (m_bGateway)
	{
		m_lCost = -1;
	}
	else
	{
		if (m_lBiocntEnd>=0 && m_lBiocntStart>=0) 
		{
			ldiff= m_lBiocntEnd-m_lBiocntStart;
			if (ldiff<0)
				ldiff=0;
			if (m_lCost > -1)
				m_lCost += ldiff;
		}
		else
			m_lCost = -1;
	}

	if (m_lCost < 0)
		m_strCost.LoadString(IDS_NOT_AVAILABLE);//_T("n/a")
	else
		m_strCost.Format (_T("%ld io"), m_lCost);
}


//
// CLASS: CdSqlQueryRichEditDoc
// ************************************************************************************************
IMPLEMENT_DYNCREATE(CdSqlQueryRichEditDoc, CRichEditDoc)
BEGIN_MESSAGE_MAP(CdSqlQueryRichEditDoc, CRichEditDoc)
	//{{AFX_MSG_MAP(CdSqlQueryRichEditDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Enable default OLE container implementation
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryRichEditDoc construction/destruction

CdSqlQueryRichEditDoc::CdSqlQueryRichEditDoc()
{
	m_bUseTracePage = FALSE;
	m_bLoaded  = FALSE;
	m_strDatabase        = _T("");
	m_crColorSelect      = RGB (255, 0, 0);
	m_crColorText        = GetSysColor (COLOR_WINDOWTEXT);
	m_nResultCounter = 1;
	m_nTabLastSel    =-1;
	m_bHighlight         = FALSE;
	m_nStart             = 0;
	m_nEnd               = 0;
	m_bSetQep            = FALSE;
	m_bSetOptimizeOnly   = FALSE;
	m_bAutoCommit        = m_property.IsAutoCommit();
	m_bQEPxTRACE         = TRUE;
	m_bHasBug            = TRUE;
	m_nDatabaseFlag      = 0;

	m_bQueryExecuted = FALSE;
	m_strNode = _T("(local)");
	m_strServerClass = _T("");
	m_strConnectionFlag = _T("");
	m_strUser = _T("");
	m_pCurrentSession = NULL;

	m_strIniFileName.LoadString(IDS_INIFILENAME);
	m_cyCurMainSplit = 10; 
	m_cyMinMainSplit = 10; 
	m_cyCurNestSplit = 10; 
	m_cyMinNestSplit = 10; 
	m_nSessionVersion= -1;
}

CdSqlQueryRichEditDoc::~CdSqlQueryRichEditDoc()
{
}

BOOL CdSqlQueryRichEditDoc::OnNewDocument()
{
	if (!CRichEditDoc::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


CRichEditCntrItem* CdSqlQueryRichEditDoc::CreateClientItem(REOBJECT* preo) const
{
	// cast away constness of this
	return new CuRichCtrlCntrItem(preo, (CdSqlQueryRichEditDoc*) this);
}


void CdSqlQueryRichEditDoc::CleanUp()
{
	m_bLoaded  = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryRichEditDoc serialization

void CdSqlQueryRichEditDoc::Serialize(CArchive& ar)
{
	CString caption;
	try
	{
		CleanUp();
		if (ar.IsStoring())
		{
			ar << m_nResultCounter;
			//
			// Visibility of the related frame toolbar
			POSITION pos = GetFirstViewPosition();
			CView* pView = GetNextView(pos);
			ASSERT (pView);
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)pView->GetParentFrame(); 
			ASSERT (pFrame);
			if (!pFrame)
				throw;
			//
			// Splitbar
			CSplitterWnd* pMainSplit = (CSplitterWnd *)pFrame->GetMainSplitterWnd();
			ASSERT (pMainSplit);
			if (!pMainSplit)
				throw;
			pMainSplit->GetRowInfo(0, m_cyCurMainSplit, m_cyMinMainSplit);
			ar << m_cyCurMainSplit;
			ar << m_cyMinMainSplit;

			CSplitterWnd* pNestSplit = (CSplitterWnd *)pFrame->GetNestSplitterWnd();
			ASSERT (pNestSplit);
			if (!pNestSplit)
				throw;
			pNestSplit->GetRowInfo(0, m_cyCurNestSplit, m_cyMinNestSplit);
			ar << m_cyCurNestSplit;
			ar << m_cyMinNestSplit;

			CvSqlQueryRichEditView* pRichEditView = pFrame->GetRichEditView();
			ASSERT (pRichEditView);
			if (pRichEditView)
			{
				CRichEditCtrl& redit = pRichEditView->GetRichEditCtrl();
				redit.GetWindowText(m_strStatement);
			}
			ar << GetTitle();
			ar << m_strDatabase;
			ar << m_strStatement;
			ar << m_bHighlight;  
			ar << m_nStart;
			ar << m_nEnd;
			//
			// Store the page of the Query Result.
			CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
			if (pDlgResult)
			{
				pDlgResult->GetData (); // Initialize the data of the document
			}
			ar << m_nTabCurSel;
			//
			// Store the Query Information:
			CuDlgSqlQueryStatement* pDlgQueryInfo = pFrame->GetDlgSqlQueryStatement();
			if (pDlgQueryInfo)
			{
				m_strCom = pDlgQueryInfo->m_strCom;
				m_strCost= pDlgQueryInfo->m_strCost;
				m_strElap= pDlgQueryInfo->m_strElap;
				m_strExec= pDlgQueryInfo->m_strExec;
			}
			else
			{
				m_strCom = _T("");
				m_strCost= _T("");
				m_strElap= _T("");
				m_strExec= _T("");
			}
			ar << m_strCom;
			ar << m_strCost;
			ar << m_strElap;
			ar << m_strExec;
			ar << m_bQEPxTRACE;
			ar << m_bHasBug;
			ar << m_bQueryExecuted;
			ar << m_nSessionVersion;

			m_listPageResult.Serialize (ar);
			while (!m_listPageResult.IsEmpty())
				delete m_listPageResult.RemoveHead();
		}
		else
		{
			m_bLoaded = TRUE;
			ar >> m_nResultCounter;
			//
			// Splitbar
			ar >> m_cyCurMainSplit;
			ar >> m_cyMinMainSplit;
			ar >> m_cyCurNestSplit;
			ar >> m_cyMinNestSplit;

			ar >> m_strTitle;
			ar >> m_strDatabase;
			ar >> m_strStatement;
			ar >> m_bHighlight;
			ar >> m_nStart;
			ar >> m_nEnd;
			ar >> m_nTabCurSel;
			//
			// Load the Query Information:
			ar >> m_strCom;
			ar >> m_strCost;
			ar >> m_strElap;
			ar >> m_strExec;
			ar >> m_bQEPxTRACE;

			ar >> m_bHasBug;
			ar >> m_bQueryExecuted;
			ar >> m_nSessionVersion;

			m_listPageResult.Serialize (ar);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CdSqlQueryRichEditDoc::Serialize, ungnown error"));
	}
}



/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryRichEditDoc diagnostics

#ifdef _DEBUG
void CdSqlQueryRichEditDoc::AssertValid() const
{
	CRichEditDoc::AssertValid();
}

void CdSqlQueryRichEditDoc::Dump(CDumpContext& dc) const
{
	CRichEditDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSqlQueryRichEditDoc commands

//
// Can multiple statements be executed against this gateway ?
BOOL CdSqlQueryRichEditDoc::IsMultipleSelectAllowed()
{
	ASSERT(m_pCurrentSession);
	if (!m_pCurrentSession)
		return TRUE;
	if (m_pCurrentSession->GetVersion() == INGRESVERS_NOTOI)
		return FALSE;
	if (m_pCurrentSession->GetDbmsType() != DBMS_INGRESII)
		return FALSE;
	return TRUE;
}


void CdSqlQueryRichEditDoc::UpdateAutocommit(BOOL bNew)
{
	m_bAutoCommit = bNew;
}
