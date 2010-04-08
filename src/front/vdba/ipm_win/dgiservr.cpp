/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiservr.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Server
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgiservr.h"
#include "ipmdoc.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CuDlgIpmDetailServer, CFormView)

CuDlgIpmDetailServer::CuDlgIpmDetailServer()
	: CFormView(CuDlgIpmDetailServer::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailServer)
	m_strListenAddress = _T("");
	m_strClass = _T("");
	m_strConnectingDB = _T("");
	m_strSession = _T("");
	m_strTotal = _T("");
	m_strCompute = _T("");
	m_strCurrent = _T("");
	m_strV1x1 = _T("");
	m_strV2x1 = _T("");
	m_strMax = _T("");
	m_strV1x2 = _T("");
	m_strV2x2 = _T("");
	m_strListenState = _T("");
	//}}AFX_DATA_INIT
	// m_ImageList.Create (IDB_TM_SERVER_TYPE_INGRES, 16, 0, RGB(255,0,255));	
	m_strSession.LoadString(IDS_TAB_SESSION);      // = _T("Sessions");
	m_strTotal.LoadString(IDS_TOTAL);              // = _T("Total");
	m_strCompute.LoadString(IDS_COMPUTABLE_STATE); // = _T("Computable State");
	m_strCurrent.LoadString(IDS_CURRENT);          // = _T("Current");
	m_strMax.LoadString(IDS_MAX);                  // = _T("Max");

	int width = IMAGE_WIDTH;
	int height = IMAGE_HEIGHT;
	m_ImageList.Create(width, height, TRUE, 10, 10);

	m_pSubDialog = NULL;
	m_bAfterLoad = FALSE;
	m_bStartup   = TRUE;
	m_bInitial   = TRUE;
}

CuDlgIpmDetailServer::~CuDlgIpmDetailServer()
{
	m_ImageList.DeleteImageList();
}

void CuDlgIpmDetailServer::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailServer)
	DDX_Control(pDX, IDC_STATIC_SUBDIALOG1, m_cStaticSubDialog);
	DDX_Control(pDX, IDC_STATIC3, m_transparentBmp);
	DDX_Text(pDX, IDC_EDIT1, m_strListenAddress);
	DDX_Text(pDX, IDC_EDIT2, m_strClass);
	DDX_Text(pDX, IDC_EDIT3, m_strConnectingDB);
	DDX_Text(pDX, IDC_EDIT4, m_strSession);
	DDX_Text(pDX, IDC_EDIT5, m_strTotal);
	DDX_Text(pDX, IDC_EDIT6, m_strCompute);
	DDX_Text(pDX, IDC_EDIT7, m_strCurrent);
	DDX_Text(pDX, IDC_EDIT8, m_strV1x1);
	DDX_Text(pDX, IDC_EDIT9, m_strV2x1);
	DDX_Text(pDX, IDC_EDIT10, m_strMax);
	DDX_Text(pDX, IDC_EDIT11, m_strV1x2);
	DDX_Text(pDX, IDC_EDIT12, m_strV2x2);
	DDX_Text(pDX, IDC_LISTEN_STATE_EDIT, m_strListenState);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailServer, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailServer)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailServer diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailServer::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailServer::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailServer message handlers


void CuDlgIpmDetailServer::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmDetailServer::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strListenAddress= lpszNA;
		m_strClass = lpszNA;
		m_strConnectingDB = lpszNA;
		m_strV1x1 = lpszNA;
		m_strV2x1 = lpszNA;
		m_strV1x2 = lpszNA;
		m_strV2x2 = lpszNA;
		m_strListenState = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];
		m_strListenAddress = m_svrStruct.serverdatamin.listen_address;
		m_strClass = m_svrStruct.serverdatamin.server_class;
		m_strConnectingDB = m_svrStruct.serverdatamin.server_dblist;
		if (m_svrStruct.serverdatamin.servertype == SVR_TYPE_NET)
		{
			m_strV1x1.Format (_T("%d"), m_infoNetTrafic.GetInbound());
			m_strV2x1.Format (_T("%d"), m_infoNetTrafic.GetOutbound());
			m_strV1x2.Format (_T("%d"), m_infoNetTrafic.GetInboundMax());
			m_strV2x2.Format (_T("%d"), m_infoNetTrafic.GetOutboundMax());
		}
		else
		{
			m_strV1x1 = FormatServerSession(m_svrStruct.current_connections ,tchszBuffer);
			m_strV2x1 = FormatServerSession(m_svrStruct.active_current_sessions ,tchszBuffer);
			m_strV1x2 = FormatServerSession(m_svrStruct.current_max_connections ,tchszBuffer);
			m_strV2x2 = FormatServerSession(m_svrStruct.active_max_sessions ,tchszBuffer);
		}
		m_strListenState = m_svrStruct.serverdatamin.listen_state;
		// custom bitmap
		UINT bmpId = GetServerImageId(& (m_svrStruct.serverdatamin) );
		CImageList img;
		img.Create(bmpId, 16, 1, RGB(255,0,255));
		HICON h = img.ExtractIcon(0);
		m_transparentBmp.SetWindowText(_T(""));
		m_transparentBmp.ResetBitmap(h);
	}
}


LONG CuDlgIpmDetailServer::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
	//
	// Initialize the member variables: ...and call UpdateData();
	try
	{
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_svrStruct, 0, sizeof (m_svrStruct));
			m_svrStruct.serverdatamin = *(LPSERVERDATAMIN)pUps->pStruct;
		}
		CreateNetTraficControl();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_SERVER);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_svrStruct);
		if (m_bAfterLoad)
		{
			m_bAfterLoad = FALSE;
			if (!m_bStartup)
			{
				CString strMsg;
				AfxFormatString1(strMsg, IDS_E_REFRESH_NETTRAFIC, (LPCTSTR)m_pSubDialog->GetTimeStamp());
				AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
				m_pSubDialog->SinceNow(FALSE);
			}
			else
			{
				m_pSubDialog->SinceStartup(FALSE);
			}
		}
		//
		// Query Net trafic info:
		if (m_svrStruct.serverdatamin.servertype == SVR_TYPE_NET && m_pSubDialog && IsWindow (m_pSubDialog->m_hWnd))
		{
			CString strNode = pDoc->GetConnectionInfo().GetNode();
			CString strListenAddress = m_svrStruct.serverdatamin.listen_address;
			if (bOK)
				bOK = INGRESII_QueryNetTrafic (strNode, strListenAddress, m_infoNetTrafic);
			if (bOK)
			{
				if (m_bInitial)
				{
					m_bInitial = FALSE;
					m_pSubDialog->m_nCounter = 0;
				}
			}
			m_pSubDialog->UpdateInfo (m_infoNetTrafic, bOK);
		}
		if (!bOK)
			InitClassMembers (TRUE);
		else
			InitClassMembers ();
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

LONG CuDlgIpmDetailServer::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	CuIpmPropertyDataDetailServer* pData = (CuIpmPropertyDataDetailServer*)lParam;
	LPSERVERDATAMAX pSvr = (LPSERVERDATAMAX)(&pData->m_dlgData);

	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailServer")) == 0);
	memcpy (&m_svrStruct, pSvr, sizeof (m_svrStruct));
	try
	{
		m_bAfterLoad = TRUE;
		m_bStartup   = pData->m_netTraficData.m_bStartup;
		m_infoNetTrafic = pData->m_netTraficData.m_TraficInfo;
		CreateNetTraficControl();
		ResetDisplay();
		InitClassMembers ();
		m_pSubDialog->UpdateLoadInfo(pData->m_netTraficData);
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmDetailServer::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailServer* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailServer (&m_svrStruct);
		if (m_pSubDialog && IsWindow (m_pSubDialog->m_hWnd))
		{
			m_pSubDialog->GetNetTraficData(pData->m_netTraficData);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailServer::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}

void CuDlgIpmDetailServer::CreateNetTraficControl()
{
	CWnd* pWndStatic = GetDlgItem(IDC_STATIC_SUBDIALOG1);
	if (!m_pSubDialog && pWndStatic && IsWindow(pWndStatic->m_hWnd))
	{
		CRect r;
		pWndStatic->GetWindowRect(r);
		ScreenToClient (r);
		m_pSubDialog = new CuDlgIpmDetailNetTrafic(this);
		m_pSubDialog->Create (IDD_IPMDETAIL_NETTRAFIC, this);
		m_pSubDialog->MoveWindow(r);
	}

	if (m_svrStruct.serverdatamin.servertype == SVR_TYPE_NET && m_pSubDialog && IsWindow (m_pSubDialog->m_hWnd))
	{
		m_pSubDialog->ShowWindow(SW_SHOW);
		m_strTotal.LoadString(IDS_TC_INBOUND);
		m_strCompute.LoadString(IDS_TC_OUTBOUND); 

		return;
	}

	if (m_pSubDialog && IsWindow (m_pSubDialog->m_hWnd))
		m_pSubDialog->ShowWindow(SW_HIDE);
	m_strTotal.LoadString(IDS_TOTAL);              // = _T("Total");
	m_strCompute.LoadString(IDS_COMPUTABLE_STATE); // = _T("Computable State");
}

