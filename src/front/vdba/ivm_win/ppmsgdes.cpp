/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppmsgdes.cpp : implementation file
**    Project  : Visual Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page of message description
**
** History:
**
** 06-Mar-2003 (uk$so01)
**    created
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 11-Mar-2003 (uk$so01)
**    SIR #109773, Add wait cursor when querying messages.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "ppmsgdes.h"
#include "evtcats.h"
#include "getinst.h"
#include "xfindmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuPropertyPageMessageDescription, CPropertyPage)

CuPropertyPageMessageDescription::CuPropertyPageMessageDescription() : CPropertyPage(CuPropertyPageMessageDescription::IDD)
{
	//{{AFX_DATA_INIT(CuPropertyPageMessageDescription)
	m_strParameter = _T("");
	m_strExplanation = _T("");
	m_strSystemStatus = _T("");
	m_strRecommendation = _T("");
	//}}AFX_DATA_INIT
	m_uMsgAlreadyQueried = 0;
}

CuPropertyPageMessageDescription::~CuPropertyPageMessageDescription()
{
}

void CuPropertyPageMessageDescription::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPropertyPageMessageDescription)
	DDX_Control(pDX, IDC_COMBO2, m_cComboMessage);
	DDX_Control(pDX, IDC_COMBO1, m_cComboCategory);
	DDX_Text(pDX, IDC_EDIT1, m_strParameter);
	DDX_Text(pDX, IDC_EDIT2, m_strExplanation);
	DDX_Text(pDX, IDC_EDIT3, m_strSystemStatus);
	DDX_Text(pDX, IDC_EDIT4, m_strRecommendation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPropertyPageMessageDescription, CPropertyPage)
	//{{AFX_MSG_MAP(CuPropertyPageMessageDescription)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboCategory)
	ON_CBN_DROPDOWN(IDC_COMBO2, OnDropdownComboMessage)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSelchangeComboMessage)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonFind)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonHelp)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageMessageDescription message handlers

BOOL CuPropertyPageMessageDescription::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	int nIdx = -1;
	CString strDesc;
	long lCat;
	BOOL bOK = GetFirstIngClass4MessageExplanation(lCat, strDesc);
	while (bOK)
	{
		nIdx = m_cComboCategory.AddString(strDesc);
		if (nIdx != CB_ERR)
			m_cComboCategory.SetItemData(nIdx, lCat);
		bOK = GetNextIngClass4MessageExplanation(lCat, strDesc);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LONG CuPropertyPageMessageDescription::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	MSGCLASSANDID* pMsg = (MSGCLASSANDID*)lParam;
	m_uMsgAlreadyQueried = 0;

	if (pMsg)
	{
		BOOL bFound = FALSE;
		int  nFound = -1;
		long lCat = 0;

		if (pMsg->lMsgClass != -1 && pMsg->lMsgFullID != -1)
		{
			int i, nCount = m_cComboCategory.GetCount();
			for (i=0; i<nCount; i++)
			{
				lCat = (long)m_cComboCategory.GetItemData(i);
				if (pMsg->lMsgClass == lCat)
				{
					m_cComboCategory.SetCurSel(i);
					bFound = TRUE;
					break;
				}
			}
			if (!bFound)
				m_cComboCategory.SetCurSel(-1);
			else
			{
				int nIdx = -1;
				TCHAR tchszMsg [512+1];
				BOOL bOK = GetAndFormatIngMsg(pMsg->lMsgFullID, tchszMsg, sizeof(tchszMsg) -1);
				if (bOK)
				{
					m_cComboMessage.ResetContent();
					nIdx = m_cComboMessage.AddString(tchszMsg);
					if (nIdx != CB_ERR)
					{
						m_cComboMessage.SetItemData(nIdx, (DWORD)pMsg->lMsgFullID);
						nFound = nIdx;
					}
				}
				m_cComboMessage.SetCurSel(nFound);
			}
		}
	}

	if (IsWindowVisible() && theApp.m_pMessageDescriptionManager)
	{
		long lCat = -1;
		long lCode= -1;
		int nSel = m_cComboCategory.GetCurSel();
		if (nSel != CB_ERR)
		{
			lCat = (long)m_cComboCategory.GetItemData(nSel);
			nSel = m_cComboMessage.GetCurSel();
			if (nSel != CB_ERR)
				lCode = (long)m_cComboMessage.GetItemData(nSel);

			CaMessageDescription dsc;
			theApp.m_pMessageDescriptionManager->Lookup(pMsg, dsc);
			m_strParameter      = dsc.GetParemeter();
			m_strExplanation    = dsc.GetDescription();
			m_strSystemStatus   = dsc.GetStatus();
			m_strRecommendation = dsc.GetRecommendation();
			UpdateData(FALSE);
		}
	}
	return 0;
}

void CuPropertyPageMessageDescription::OnSelchangeComboCategory() 
{
	m_cComboMessage.ResetContent();
	m_uMsgAlreadyQueried = 0;
}

void CuPropertyPageMessageDescription::OnDropdownComboMessage() 
{
	if (m_uMsgAlreadyQueried & MSG_ALREADY_QUERIED)
		return;
	CWaitCursor doWaitCursor;
	CString strCurItem = _T("");
	int nSel = m_cComboMessage.GetCurSel();
	if (nSel != CB_ERR)
	{
		m_cComboMessage.GetLBText(nSel, strCurItem);
	}
	m_cComboMessage.ResetContent();
	nSel = m_cComboCategory.GetCurSel();
	if (nSel != CB_ERR)
	{
		int nIdx;
		long lCode;
		CString strMsgText;
		long lCat = (long)m_cComboCategory.GetItemData(nSel);
		BOOL bOK = GetFirstIngCatMessage(lCat, lCode, strMsgText);
		while (bOK)
		{
			nIdx = m_cComboMessage.AddString(strMsgText);
			if (nIdx != CB_ERR)
			{
				m_cComboMessage.SetItemData(nIdx, (DWORD)lCode);
			}
			bOK = GetNextIngCatMessage(lCode, strMsgText);
		}
		m_uMsgAlreadyQueried |= MSG_ALREADY_QUERIED;
		if (!strCurItem.IsEmpty())
		{
			nSel = m_cComboMessage.FindStringExact(-1, strCurItem);
			m_cComboMessage.SetCurSel(nSel);
		}
	}
}

void CuPropertyPageMessageDescription::OnSelchangeComboMessage() 
{
	int nSel = m_cComboCategory.GetCurSel();
	if (nSel != CB_ERR)
	{
		long lCat = (long)m_cComboCategory.GetItemData(nSel);
		nSel = m_cComboMessage.GetCurSel();
		if (nSel != CB_ERR)
		{
			long lCode = (long)m_cComboMessage.GetItemData(nSel);
			MSGCLASSANDID msg;
			msg.lMsgClass = lCat;
			msg.lMsgFullID= lCode;
			if (theApp.m_pMessageDescriptionManager)
			{
				CaMessageDescription dsc;
				theApp.m_pMessageDescriptionManager->Lookup(&msg, dsc);
				m_strParameter      = dsc.GetParemeter();
				m_strExplanation    = dsc.GetDescription();
				m_strSystemStatus   = dsc.GetStatus();
				m_strRecommendation = dsc.GetRecommendation();
				UpdateData(FALSE);
			}
		}
	}
}

void CuPropertyPageMessageDescription::OnButtonFind() 
{
	CxDlgFindMessage dlg;
	int nAnswer = dlg.DoModal();
	if (nAnswer == IDOK)
	{
		const int nCount = 512;
		TCHAR tchszText  [nCount];
		MSGCLASSANDID msg = getMsgStringClassAndId((LPTSTR)(LPCTSTR)dlg.m_strMessage, tchszText, sizeof(tchszText));
		if (msg.lMsgClass != CLASS_OTHERS && msg.lMsgFullID != MSG_NOCODE)
		{
			OnUpdateData(0, (LPARAM)&msg);
		}
	}
}

BOOL CuPropertyPageMessageDescription::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return APP_HelpInfo(40060);
}

void CuPropertyPageMessageDescription::OnButtonHelp() 
{
	APP_HelpInfo(40060);
}
