/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgIpUsag.cpp, Implementation file.                                    //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control                                                 //              
//               The Location usage page.                                              //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "dgipusag.h"
#include "ipmprdta.h"
#include "vtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "monitor.h"
#include "dbaginfo.h"
#include "domdata.h"
}

IMPLEMENT_DYNCREATE(CuDlgIpmPageLocationUsage, CFormView)

void CuDlgIpmPageLocationUsage::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strName = lpszNA;
		m_strArea = lpszNA;
		m_bDatabase = FALSE;
		m_bWork = FALSE;
		m_bJournal = FALSE;
		m_bCheckpoint = FALSE;
		m_bDump = FALSE;
	}
	else
	{
		m_strName = (LPCTSTR)m_lcusageStruct.objectname;
		m_strArea = (LPCTSTR)m_lcusageStruct.AreaName;
		m_bDatabase = m_lcusageStruct.bLocationUsage[LOCATIONDATABASE];
		m_bWork = m_lcusageStruct.bLocationUsage[LOCATIONWORK];
		m_bJournal = m_lcusageStruct.bLocationUsage[LOCATIONJOURNAL];
		m_bCheckpoint = m_lcusageStruct.bLocationUsage[LOCATIONCHECKPOINT];
		m_bDump = m_lcusageStruct.bLocationUsage[LOCATIONDUMP];
	}
}

CuDlgIpmPageLocationUsage::CuDlgIpmPageLocationUsage()
	: CFormView(CuDlgIpmPageLocationUsage::IDD)
{
	memset (&m_lcusageStruct, 0, sizeof (m_lcusageStruct));
	//{{AFX_DATA_INIT(CuDlgIpmPageLocationUsage)
	m_strName = _T("");
	m_strArea = _T("");
	m_bDatabase = FALSE;
	m_bWork = FALSE;
	m_bJournal = FALSE;
	m_bCheckpoint = FALSE;
	m_bDump = FALSE;
	//}}AFX_DATA_INIT
}

CuDlgIpmPageLocationUsage::~CuDlgIpmPageLocationUsage()
{

}


void CuDlgIpmPageLocationUsage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLocationUsage)
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strName);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strArea);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bDatabase);
	DDX_Check(pDX, IDC_MFC_CHECK2, m_bWork);
	DDX_Check(pDX, IDC_MFC_CHECK3, m_bJournal);
	DDX_Check(pDX, IDC_MFC_CHECK4, m_bCheckpoint);
	DDX_Check(pDX, IDC_MFC_CHECK5, m_bDump);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLocationUsage, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmPageLocationUsage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLocationUsage message handlers

#ifdef _DEBUG
void CuDlgIpmPageLocationUsage::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmPageLocationUsage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

LONG CuDlgIpmPageLocationUsage::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int isess, ires = RES_ERR, nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

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
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			LPLOCATIONDATAMIN lpLoc= (LPLOCATIONDATAMIN)pUps->pStruct;
			memset (&m_lcusageStruct, 0, sizeof (m_lcusageStruct));
			lstrcpy ((LPTSTR)m_lcusageStruct.objectname, (LPCTSTR)lpLoc->LocationName);
		}
		LPCTSTR nodeName = GetVirtNodeName (nNodeHandle);
		ires = GetDetailInfo ((LPUCHAR)nodeName, OT_LOCATION, &m_lcusageStruct, FALSE, &isess);
		if (ires == RES_SUCCESS)
		{
			InitClassMembers ();
		}
		else
		{
			InitClassMembers (TRUE);
		}
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLocationUsage::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOCATIONPARAMS pLoc = (LPLOCATIONPARAMS)lParam;
	ASSERT (lstrcmp (pClass, "CuIpmPropertyDataPageLocationUsage") == 0);
	memcpy (&m_lcusageStruct, pLoc, sizeof (m_lcusageStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLocationUsage::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLocationUsage* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataPageLocationUsage (&m_lcusageStruct);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageLocationUsage::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
