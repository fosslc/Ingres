/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgitrafi.cpp : implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Sup-Page of Page Net Server(CuDlgIpmDetailServer)
**
** History:
**
** 14-May-2003 (uk$so01)
**    Created
**    SIR #110269, Add Network trafic monitoring.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgitrafi.h"
#include "ipmframe.h"
#include "ipmdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgIpmDetailNetTrafic::CuDlgIpmDetailNetTrafic(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmDetailNetTrafic::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailNetTrafic)
	m_strTotalPacketIn = _T("");
	m_strTotalDataIn = _T("");
	m_strTotalPacketOut = _T("");
	m_strTotalDataOut = _T("");
	m_strCurrentPacketIn = _T("");
	m_strCurrentDataIn = _T("");
	m_strCurrentPacketOut = _T("");
	m_strCurrentDataOut = _T("");
	m_strAveragePacketIn = _T("");
	m_strAverageDataIn = _T("");
	m_strAveragePacketOut = _T("");
	m_strAverageDataOut = _T("");
	m_strPeekPacketIn = _T("");
	m_strPeekDataIn = _T("");
	m_strPeekPacketOut = _T("");
	m_strPeekDataOut = _T("");
	m_strDate1PacketIn = _T("");
	m_strDate2PacketIn = _T("");
	m_strDate1DataIn = _T("");
	m_strDate2DataIn = _T("");
	m_strDate1PacketOut = _T("");
	m_strDate2PacketOut = _T("");
	m_strDate1DataOut = _T("");
	m_strDate2DataOut = _T("");
	//}}AFX_DATA_INIT

	m_bStartup = TRUE;
	m_nCounter = 0;
	m_bRefresh = TRUE; // Now and Startup button cause to call the refresh 
}

void CuDlgIpmDetailNetTrafic::SetTimeStamp(LPCTSTR lpszTimeStamp)
{
	m_cEditTimeStamp.SetWindowText(lpszTimeStamp);
}

CString CuDlgIpmDetailNetTrafic::GetTimeStamp()
{
	CString s;
	m_cEditTimeStamp.GetWindowText(s);
	return s;
}

void CuDlgIpmDetailNetTrafic::GetNetTraficData(CaIpmPropertyNetTraficData& data)
{
	data.m_TraficInfo = m_TraficInfo;
	data.m_bStartup = m_bStartup ;
	data.m_nCounter = m_nCounter; 
	data.m_strTimeStamp = GetTimeStamp();

	data.m_strTotalPacketIn = m_strTotalPacketIn;
	data.m_strTotalDataIn = m_strTotalDataIn;
	data.m_strTotalPacketOut = m_strTotalPacketOut;
	data.m_strTotalDataOut = m_strTotalDataOut;
	data.m_strCurrentPacketIn = m_strCurrentPacketIn;
	data.m_strCurrentDataIn = m_strCurrentDataIn;
	data.m_strCurrentPacketOut = m_strCurrentPacketOut;
	data.m_strCurrentDataOut = m_strCurrentDataOut;
	data.m_strAveragePacketIn = m_strAveragePacketIn;
	data.m_strAverageDataIn = m_strAverageDataIn;
	data.m_strAveragePacketOut = m_strAveragePacketOut;
	data.m_strAverageDataOut = m_strAverageDataOut;
	data.m_strPeekPacketIn = m_strPeekPacketIn;
	data.m_strPeekDataIn = m_strPeekDataIn;
	data.m_strPeekPacketOut = m_strPeekPacketOut;
	data.m_strPeekDataOut = m_strPeekDataOut;
	data.m_strDate1PacketIn = m_strDate1PacketIn;
	data.m_strDate2PacketIn = m_strDate2PacketIn;
	data.m_strDate1DataIn = m_strDate1DataIn;
	data.m_strDate2DataIn = m_strDate2DataIn;
	data.m_strDate1PacketOut = m_strDate1PacketOut;
	data.m_strDate2PacketOut = m_strDate2PacketOut;
	data.m_strDate1DataOut = m_strDate1DataOut;
	data.m_strDate2DataOut = m_strDate2DataOut;
}

void CuDlgIpmDetailNetTrafic::UpdateLoadInfo(CaIpmPropertyNetTraficData& data)
{
	m_TraficInfo = data.m_TraficInfo;
	m_bStartup = data.m_bStartup;
	m_nCounter = data.m_nCounter; 
	SetTimeStamp(data.m_strTimeStamp);

	m_strTotalPacketIn = data.m_strTotalPacketIn;
	m_strTotalDataIn = data.m_strTotalDataIn;
	m_strTotalPacketOut = data.m_strTotalPacketOut;
	m_strTotalDataOut = data.m_strTotalDataOut;
	m_strCurrentPacketIn = data.m_strCurrentPacketIn;
	m_strCurrentDataIn = data.m_strCurrentDataIn;
	m_strCurrentPacketOut = data.m_strCurrentPacketOut;
	m_strCurrentDataOut = data.m_strCurrentDataOut;
	m_strAveragePacketIn = data.m_strAveragePacketIn;
	m_strAverageDataIn = data.m_strAverageDataIn;
	m_strAveragePacketOut = data.m_strAveragePacketOut;
	m_strAverageDataOut = data.m_strAverageDataOut;
	m_strPeekPacketIn = data.m_strPeekPacketIn;
	m_strPeekDataIn = data.m_strPeekDataIn;
	m_strPeekPacketOut = data.m_strPeekPacketOut;
	m_strPeekDataOut = data.m_strPeekDataOut;
	m_strDate1PacketIn = data.m_strDate1PacketIn;
	m_strDate2PacketIn = data.m_strDate2PacketIn;
	m_strDate1DataIn = data.m_strDate1DataIn;
	m_strDate2DataIn = data.m_strDate2DataIn;
	m_strDate1PacketOut = data.m_strDate1PacketOut;
	m_strDate2PacketOut = data.m_strDate2PacketOut;
	m_strDate1DataOut = data.m_strDate1DataOut;
	m_strDate2DataOut = data.m_strDate2DataOut;
	UpdateData(FALSE);
}

void CuDlgIpmDetailNetTrafic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailNetTrafic)
	DDX_Control(pDX, IDC_EDIT17, m_cEditTimeStamp);
	DDX_Text(pDX, IDC_EDIT1, m_strTotalPacketIn);
	DDX_Text(pDX, IDC_EDIT2, m_strTotalDataIn);
	DDX_Text(pDX, IDC_EDIT3, m_strTotalPacketOut);
	DDX_Text(pDX, IDC_EDIT4, m_strTotalDataOut);
	DDX_Text(pDX, IDC_EDIT5, m_strCurrentPacketIn);
	DDX_Text(pDX, IDC_EDIT6, m_strCurrentDataIn);
	DDX_Text(pDX, IDC_EDIT7, m_strCurrentPacketOut);
	DDX_Text(pDX, IDC_EDIT8, m_strCurrentDataOut);
	DDX_Text(pDX, IDC_EDIT9, m_strAveragePacketIn);
	DDX_Text(pDX, IDC_EDIT10, m_strAverageDataIn);
	DDX_Text(pDX, IDC_EDIT11, m_strAveragePacketOut);
	DDX_Text(pDX, IDC_EDIT12, m_strAverageDataOut);
	DDX_Text(pDX, IDC_EDIT13, m_strPeekPacketIn);
	DDX_Text(pDX, IDC_EDIT14, m_strPeekDataIn);
	DDX_Text(pDX, IDC_EDIT15, m_strPeekPacketOut);
	DDX_Text(pDX, IDC_EDIT16, m_strPeekDataOut);
	DDX_Text(pDX, IDC_STATIC1_DATE1, m_strDate1PacketIn);
	DDX_Text(pDX, IDC_STATIC1_DATE2, m_strDate2PacketIn);
	DDX_Text(pDX, IDC_STATIC2_DATE1, m_strDate1DataIn);
	DDX_Text(pDX, IDC_STATIC2_DATE2, m_strDate2DataIn);
	DDX_Text(pDX, IDC_STATIC3_DATE1, m_strDate1PacketOut);
	DDX_Text(pDX, IDC_STATIC3_DATE2, m_strDate2PacketOut);
	DDX_Text(pDX, IDC_STATIC4_DATE1, m_strDate1DataOut);
	DDX_Text(pDX, IDC_STATIC4_DATE2, m_strDate2DataOut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailNetTrafic, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmDetailNetTrafic)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonStartup)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonNow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailNetTrafic message handlers

void CuDlgIpmDetailNetTrafic::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmDetailNetTrafic::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SinceStartup(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmDetailNetTrafic::UpdateInfo(CaNetTraficInfo& infoNetTrafic, BOOL bOK)
{
	CString strTime;
	if (bOK)
	{
		if (m_nCounter < 2)
			m_nCounter++;
		if (m_nCounter == 1)
			m_TraficInfo_0 = infoNetTrafic;
		if (m_bStartup)
			m_TraficInfo_0.Cleanup(); 
		UpdateTotalTrafic(infoNetTrafic);
		UpdatePerSecondTrafic(infoNetTrafic);
		m_TraficInfo = infoNetTrafic;
	}
	else
	{
		CString strNa = _T("n/a");
		m_strTotalPacketIn = strNa;
		m_strTotalDataIn = strNa;
		m_strTotalPacketOut = strNa;
		m_strTotalDataOut = strNa;
		m_strCurrentPacketIn = strNa;
		m_strCurrentDataIn = strNa;
		m_strCurrentPacketOut = strNa;
		m_strCurrentDataOut = strNa;
		m_strAveragePacketIn = strNa;
		m_strAverageDataIn = strNa;
		m_strAveragePacketOut = strNa;
		m_strAverageDataOut = strNa;
		m_strPeekPacketIn = strNa;
		m_strPeekDataIn = strNa;
		m_strPeekPacketOut = strNa;
		m_strPeekDataOut = strNa;

		m_strDate1PacketIn = _T("");
		m_strDate2PacketIn = _T("");
		m_strDate1DataIn = _T("");
		m_strDate2DataIn = _T("");
		m_strDate1PacketOut = _T("");
		m_strDate2PacketOut = _T("");
		m_strDate1DataOut = _T("");
		m_strDate2DataOut = _T("");
	}
	UpdateData(FALSE);
}


void CuDlgIpmDetailNetTrafic::OnButtonStartup() 
{
	m_cEditTimeStamp.SetWindowText("Startup");
	m_bStartup = TRUE;
	if (m_bRefresh)
	{
		CWnd* pParent1 = GetParentFrame(); // Frame of CFormView
		CfIpmFrame* pParent2 = (CfIpmFrame*)pParent1->GetParentFrame(); 
		CdIpmDoc* pDoc = pParent2->GetIpmDoc();
		if (pDoc)
		{
			if (!pDoc->ManageMonSpecialState())
				pDoc->ForceRefresh();
		}
	}
	UpdateData(FALSE);
}

void CuDlgIpmDetailNetTrafic::OnButtonNow() 
{
	CTime t = CTime::GetCurrentTime();
	CString strTime;
	strTime.Format(_T("%s"), (LPCTSTR)t.Format(_T("%c")));
	m_cEditTimeStamp.SetWindowText (strTime);
	m_TraficInfo.Cleanup();
	m_bStartup = FALSE;
	m_nCounter = 0;

	if (m_bRefresh)
	{
		CWnd* pParent1 = GetParentFrame(); // Frame of CFormView
		CfIpmFrame* pParent2 = (CfIpmFrame*)pParent1->GetParentFrame(); 
		CdIpmDoc* pDoc = pParent2->GetIpmDoc();
		if (pDoc)
		{
			if (!pDoc->ManageMonSpecialState())
				pDoc->ForceRefresh();
		}
	}
}


void CuDlgIpmDetailNetTrafic::UpdateTotalTrafic(CaNetTraficInfo& infoNetTrafic)
{
	//
	// Total:
	m_strTotalPacketIn.Format(_T("%d"), abs(infoNetTrafic.GetPacketIn() - m_TraficInfo_0.GetPacketIn()));
	m_strTotalDataIn.Format(_T("%d"), abs(infoNetTrafic.GetDataIn() - m_TraficInfo_0.GetDataIn()));
	m_strTotalPacketOut.Format(_T("%d"), abs(infoNetTrafic.GetPacketOut() - m_TraficInfo_0.GetPacketOut()));
	m_strTotalDataOut.Format(_T("%d"), abs(infoNetTrafic.GetDataOut() - m_TraficInfo_0.GetDataOut()));
}

void CuDlgIpmDetailNetTrafic::UpdatePerSecondTrafic(CaNetTraficInfo& infoNetTrafic)
{
	CTimeSpan t;
	int dt = 0;
	double dRate = 0.0;

	if (m_nCounter < 2)
	{
		//
		// Current:
		m_strCurrentPacketIn = _T("n/a");
		m_strCurrentDataIn = _T("n/a");
		m_strCurrentPacketOut = _T("n/a");
		m_strCurrentDataOut = _T("n/a");
	}
	else
	{
		t = infoNetTrafic.GetTimeStamp() - m_TraficInfo.GetTimeStamp();
		dt = t.GetSeconds();
		if (dt == 0)
			dt = 1;

		//
		// Current:
		dRate = (double)(abs(infoNetTrafic.GetPacketIn() - m_TraficInfo.GetPacketIn()));
		dRate = dRate / (double)dt;
		m_strCurrentPacketIn.Format(_T("%0.2f"), dRate);

		dRate = (double)(abs(infoNetTrafic.GetDataIn() - m_TraficInfo.GetDataIn()));
		dRate = dRate / (double)dt;
		m_strCurrentDataIn.Format(_T("%0.2f"), dRate);

		dRate = (double)(abs(infoNetTrafic.GetPacketOut() - m_TraficInfo.GetPacketOut()));
		dRate = dRate / (double)dt;
		m_strCurrentPacketOut.Format(_T("%0.2f"), dRate);

		dRate = (double)(abs(infoNetTrafic.GetDataOut() - m_TraficInfo.GetDataOut()));
		dRate = dRate / (double)dt;
		m_strCurrentDataOut.Format(_T("%0.2f"), dRate);
	}

	if (m_bStartup)
	{
		//
		// Average:
		m_strAveragePacketIn = _T("n/a");
		m_strAverageDataIn = _T("n/a");
		m_strAveragePacketOut = _T("n/a");
		m_strAverageDataOut = _T("n/a");
		//
		// Peek:
		m_strPeekPacketIn = _T("n/a");
		m_strPeekDataIn = _T("n/a");
		m_strPeekPacketOut = _T("n/a");
		m_strPeekDataOut = _T("n/a");
		//
		// Date/Time (reached between and):
		m_strDate1PacketIn = _T("n/a");
		m_strDate2PacketIn = _T("n/a");
		m_strDate1DataIn = _T("n/a");
		m_strDate2DataIn = _T("n/a");
		m_strDate1PacketOut = _T("n/a");
		m_strDate2PacketOut = _T("n/a");
		m_strDate1DataOut = _T("n/a");
		m_strDate2DataOut = _T("n/a");
	}
	else
	{
		if (m_nCounter < 2)
		{
			//
			// Average:
			m_strAveragePacketIn = _T("n/a");
			m_strAverageDataIn = _T("n/a");
			m_strAveragePacketOut = _T("n/a");
			m_strAverageDataOut = _T("n/a");
			//
			// Peek:
			m_strPeekPacketIn = _T("n/a");
			m_strPeekDataIn = _T("n/a");
			m_strPeekPacketOut = _T("n/a");
			m_strPeekDataOut = _T("n/a");
			//
			// Date/Time (reached between and):
			m_strDate1PacketIn = _T("n/a");
			m_strDate2PacketIn = _T("n/a");
			m_strDate1DataIn = _T("n/a");
			m_strDate2DataIn = _T("n/a");
			m_strDate1PacketOut = _T("n/a");
			m_strDate2PacketOut = _T("n/a");
			m_strDate1DataOut = _T("n/a");
			m_strDate2DataOut = _T("n/a");
		}
		else
		{
			t = infoNetTrafic.GetTimeStamp() - m_TraficInfo.GetTimeStamp();
			dt = t.GetSeconds();
			if (dt == 0)
				dt = 1;
			//
			// Average:
			t = infoNetTrafic.GetTimeStamp() - m_TraficInfo_0.GetTimeStamp();
			dt = t.GetSeconds();
			if (dt == 0)
				dt = 1;

			dRate = (double)(abs(infoNetTrafic.GetPacketIn() - m_TraficInfo_0.GetPacketIn()));
			dRate = dRate / (double)dt;
			m_strAveragePacketIn.Format(_T("%0.2f"), dRate);

			dRate = (double)(abs(infoNetTrafic.GetDataIn() - m_TraficInfo_0.GetDataIn()));
			dRate = dRate / (double)dt;
			m_strAverageDataIn.Format(_T("%0.2f"), dRate);

			dRate = (double)(abs(infoNetTrafic.GetPacketOut() - m_TraficInfo_0.GetPacketOut()));
			dRate = dRate / (double)dt;
			m_strAveragePacketOut.Format(_T("%0.2f"), dRate);

			dRate = (double)(abs(infoNetTrafic.GetDataOut() - m_TraficInfo_0.GetDataOut()));
			dRate = dRate / (double)dt;
			m_strAverageDataOut.Format(_T("%0.2f"), dRate);
			//
			// Peek:
			if (atof (m_strPeekPacketIn) < atof(m_strCurrentPacketIn))
			{
				m_strPeekPacketIn = m_strCurrentPacketIn;
				m_strDate1PacketIn = m_TraficInfo.GetTimeStamp().Format(_T("%c"));
				m_strDate2PacketIn = infoNetTrafic.GetTimeStamp().Format(_T("%c"));
			}
			if (atof (m_strPeekDataIn) < atof(m_strCurrentDataIn))
			{
				m_strPeekDataIn = m_strCurrentDataIn;
				m_strDate1DataIn = m_TraficInfo.GetTimeStamp().Format(_T("%c"));
				m_strDate2DataIn = infoNetTrafic.GetTimeStamp().Format(_T("%c"));
			}
			if (atof (m_strPeekPacketOut) < atof(m_strCurrentPacketOut))
			{
				m_strPeekPacketOut = m_strCurrentPacketOut;
				m_strDate1PacketOut = m_TraficInfo.GetTimeStamp().Format(_T("%c"));
				m_strDate2PacketOut = infoNetTrafic.GetTimeStamp().Format(_T("%c"));
			}
			if (atof (m_strPeekDataOut) < atof(m_strCurrentDataOut))
			{
				m_strPeekDataOut = m_strCurrentDataOut;
				m_strDate1DataOut = m_TraficInfo.GetTimeStamp().Format(_T("%c"));
				m_strDate2DataOut = infoNetTrafic.GetTimeStamp().Format(_T("%c"));
			}
		}
	}
}

