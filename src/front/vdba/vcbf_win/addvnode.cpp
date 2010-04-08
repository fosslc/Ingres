/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : addvnode.cpp : implementation file
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Add VNode (in VNode Page of Bridge)
** 
** History:
**
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf, Created 
** 30-Apr-2004 (uk$so01)
**    SIR #111701, New Help MAP ID is available.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "addvnode.h"
#include "ll.h"
#include ".\addvnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgAddVNode dialog


CxDlgAddVNode::CxDlgAddVNode(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgAddVNode::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgAddVNode)
	m_nIndex = -1;
	m_strVNode = _T("");
	m_strListenAddress = _T("");
	//}}AFX_DATA_INIT
	m_strProtocol = _T("");
}


void CxDlgAddVNode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgAddVNode)
	DDX_Control(pDX, IDC_EDIT2, m_cListenAddress);
	DDX_Control(pDX, IDC_EDIT1, m_cVnode);
	DDX_Control(pDX, IDC_COMBO1, m_cComboProtocol);
	DDX_CBIndex(pDX, IDC_COMBO1, m_nIndex);
	DDX_Text(pDX, IDC_EDIT1, m_strVNode);
	DDX_Text(pDX, IDC_EDIT2, m_strListenAddress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgAddVNode, CDialog)
	//{{AFX_MSG_MAP(CxDlgAddVNode)
	//}}AFX_MSG_MAP
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgAddVNode message handlers

BOOL CxDlgAddVNode::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CStringList listProtocols;
	BRIDGE_protocol_list(_T("*"), listProtocols);
	POSITION pos = listProtocols.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = listProtocols.GetNext(pos);
		m_cComboProtocol.AddString(strItem);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgAddVNode::OnOK() 
{
	UpdateData(TRUE);
	if (m_nIndex == -1)
		m_cComboProtocol.GetWindowText(m_strProtocol);
	else
		m_cComboProtocol.GetLBText(m_nIndex, m_strProtocol);
	int nValide = BRIDGE_CheckValide(m_strVNode, m_strProtocol, m_strListenAddress);
	switch (nValide)
	{
	case 0:
		break;
	case 1:
		m_cVnode.SetFocus();
		return;
	case 2:
		m_cComboProtocol.SetFocus();
		return;
	case 3:
		m_cListenAddress.SetFocus();
		return;
	default:
		return;
	}
	CDialog::OnOK();
}

BOOL CxDlgAddVNode::OnHelpInfo(HELPINFO* pHelpInfo)
{
	APP_HelpInfo(m_hWnd, 10194);
	return TRUE;
}
