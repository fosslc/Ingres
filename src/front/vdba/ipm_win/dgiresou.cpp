/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiresou.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Resource
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgiresou.h"
#include "ipmprdta.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgIpmDetailResource, CFormView)

CuDlgIpmDetailResource::CuDlgIpmDetailResource()
	: CFormView(CuDlgIpmDetailResource::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailResource)
	m_strGrantMode = _T("");
	m_strType = _T("");
	m_strDatabase = _T("");
	m_strPage = _T("");
	m_strConvertMode = _T("");
	m_strTable = _T("");
	m_strLockCount = _T("");
	//}}AFX_DATA_INIT
	m_strDatabase.LoadString(IDS_TAB_DATABASE); // = _T("Database");
	m_strTable .LoadString(IDS_TAB_TABLE);      //= _T("Table");
}

CuDlgIpmDetailResource::~CuDlgIpmDetailResource()
{
}

void CuDlgIpmDetailResource::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailResource)
	DDX_Control(pDX, IDC_IMAGE1, m_transparentBmp);
	DDX_Text(pDX, IDC_EDIT1, m_strGrantMode);
	DDX_Text(pDX, IDC_EDIT2, m_strType);
	DDX_Text(pDX, IDC_EDIT3, m_strDatabase);
	DDX_Text(pDX, IDC_EDIT4, m_strPage);
	DDX_Text(pDX, IDC_EDIT5, m_strConvertMode);
	DDX_Text(pDX, IDC_EDIT6, m_strTable);
	DDX_Text(pDX, IDC_EDIT7, m_strLockCount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailResource, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailResource)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailProcess diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailResource::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailResource::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailResource message handlers

void CuDlgIpmDetailResource::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailResource::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}
void CuDlgIpmDetailResource::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strGrantMode = lpszNA;
		m_strType = lpszNA;
		m_strDatabase = lpszNA;
		m_strPage = lpszNA;
		m_strConvertMode = lpszNA;
		m_strTable = lpszNA;
		m_strLockCount = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];
		m_strGrantMode = m_rcStruct.resourcedatamin.resource_grant_mode;
		m_strType = FindResourceString(m_rcStruct.resourcedatamin.resource_type);
		m_strDatabase = m_rcStruct.resourcedatamin.res_database_name;
		m_strPage = IPM_GetResPageNameStr(&(m_rcStruct.resourcedatamin),tchszBuffer);
		m_strConvertMode = m_rcStruct.resourcedatamin.resource_convert_mode;
		m_strTable = m_rcStruct.resourcedatamin.res_table_name;
		m_strLockCount.Format (_T("%d"), m_rcStruct.number_locks);
		
		// custom bitmap
		UINT bmpId = GetResourceImageId(& (m_rcStruct.resourcedatamin) );
		CImageList img;
		img.Create(bmpId, 16, 1, RGB(255,0,255));
		HICON h = img.ExtractIcon(0);
		m_transparentBmp.SetWindowText(_T(""));
		m_transparentBmp.ResetBitmap(h);
	}
}

LONG CuDlgIpmDetailResource::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	LPIPMUPDATEPARAMS  pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_NULL_RESOURCES:
	case FILTER_RESOURCE_TYPE:
	case FILTER_IPM_EXPRESS_REFRESH:
		break;
	default:
		return 0L;
	}

	try
	{
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_rcStruct, 0, sizeof (m_rcStruct));
			m_rcStruct.resourcedatamin = *(LPRESOURCEDATAMIN)pUps->pStruct;
		}
		CaIpmQueryInfo queryInfo(pDoc, OTLL_MON_RESOURCE);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_rcStruct);

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
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		InitClassMembers ();
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgIpmDetailResource::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPRESOURCEDATAMAX pRc = (LPRESOURCEDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailResource")) == 0);
	memcpy (&m_rcStruct, pRc, sizeof (m_rcStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmDetailResource::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailResource* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailResource (&m_rcStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailResource::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
