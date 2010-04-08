/*
**  Copyright (C) 2005-2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DgVNData.cpp, Implementation File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut
** 
**    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Connection Data)
**               The Advanced Concept of Virtual Node.
** History
** 14-sep-2000 (somsa01)
**    Corrected typo left in here from cross-integration, and added
**    missing function InitProtocol().
** 14-may-2001 (zhayu01) SIR 104724
**    Vnode becomes Server for EDBC
** 27-aug-2001 (zhayu01) SIR 105616 for EDBC
**    Make 51 the maximum length for Host and 5 the maximum length for Listen Address.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 16-Oct-2006 (wridu01)
**    Bug #116863 Increase maximum vnodeaddress length from 16 characters to
**                63 characters for IPV6 Project
** 16-Oct-2006 (wridu01)
**    Bug #116838 Change the default protocol from wintcp to tcp_ip for IPV6
**                project.
** 23-Sep-2008 (lunbr01) Sir 119985
**    Move "wintcp" to bottom of protocol dropdown list from 2nd position to
**    reduce chance of being chosen and for consistency with netutil.
**    For EDBC, change from "wintcp" to "tcp_ip" which means MAINWIN #idef
**    is the same and was removed.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgvndata.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgVirtualNodeData::CxDlgVirtualNodeData(CWnd* pParent /*=NULL*/)
    : CDialog(CxDlgVirtualNodeData::IDD, pParent)
{
	m_bAlter = FALSE;
	m_bDataAltered = FALSE;
	//{{AFX_DATA_INIT(CxDlgVirtualNodeData)
	m_bPrivate = FALSE;
	m_strVNodeName = _T("");
	m_strRemoteAddress = _T("");
	m_strListenAddress = _T("");
	m_strProtocol = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgVirtualNodeData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgVirtualNodeData)
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cCombo1);
	DDX_Control(pDX, IDOK, m_cOK);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strVNodeName);
	DDV_MaxChars(pDX, m_strVNodeName, 32);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strRemoteAddress);
	DDX_CBString(pDX, IDC_MFC_COMBO1, m_strProtocol);
	//}}AFX_DATA_MAP
#ifdef	EDBC
	DDV_MaxChars(pDX, m_strRemoteAddress, 51);
#else
	DDV_MaxChars(pDX, m_strRemoteAddress, 63);
#endif
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strListenAddress);
#ifdef	EDBC
	DDV_MaxChars(pDX, m_strListenAddress, 5);
#else
	DDV_MaxChars(pDX, m_strListenAddress, 32);
#endif
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNodeData, CDialog)
	//{{AFX_MSG_MAP(CxDlgVirtualNodeData)
	ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckPrivate)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeProtocol)
	ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeIPAddress)
	ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangeListenPort)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CxDlgVirtualNodeData::EnableOK()
{
	if (m_bAlter && m_bDataAltered)
	{
		UpdateData();
	}
	else
	if (!m_bAlter)
	{
		UpdateData();
	}
	else
	{
		m_cOK.EnableWindow (FALSE);
		return;
	}
	if (m_strVNodeName.GetLength() == 0         ||
		m_strRemoteAddress.GetLength() == 0     ||
		m_strProtocol.GetLength() == 0          ||
		m_strListenAddress.GetLength() == 0)
	{
		m_cOK.EnableWindow (FALSE);
	}
	else
	{
		m_cOK.EnableWindow (TRUE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeData message handlers

BOOL CxDlgVirtualNodeData::OnInitDialog() 
{
	CDialog::OnInitDialog();
	InitProtocol();
	if (!m_strProtocol.IsEmpty())
	{
		int idx = m_cCombo1.FindStringExact (-1, m_strProtocol);
		if (idx != CB_ERR)
			m_cCombo1.SetCurSel (idx);
	}

	m_cOK.EnableWindow (FALSE);
	if (m_bAlter)
	{
		CString strTitle;
#ifdef	EDBC
		if (!strTitle.LoadString (IDS_ALTER_SERVERDATA_TITLE))
			strTitle = _T("Alter the Definition of Server");
#else
		if (!strTitle.LoadString (IDS_ALTER_NODEDATA_TITLE))
			strTitle = _T("Alter the Definition of Virtual Node");
#endif
		CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
		pEdit->SetReadOnly();
		pEdit->EnableWindow (FALSE);
		SetWindowText (strTitle);

		m_strOldVNodeName       = m_strVNodeName;
		m_strOldRemoteAddress   = m_strRemoteAddress;
		m_strOldListenAddress   = m_strListenAddress;
		m_bOldPrivate           = m_bPrivate;
		m_strOldProtocol        = m_strProtocol;
		CEdit* pEdit2 = (CEdit*)GetDlgItem (IDC_MFC_EDIT2);
		pEdit2->SetFocus();
		pEdit2->SetSel (0, -1);
		return FALSE;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE    
}

void CxDlgVirtualNodeData::OnCheckPrivate() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeData::OnSelchangeProtocol() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeData::OnChangeIPAddress() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeData::OnChangeListenPort() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeData::OnOK() 
{
	//
	// Call the low level to add or alter the VNode Data
	//
	// Call the low level to add or change the definition of Virtual Node.
	HRESULT hErr = NOERROR;
	try
	{
		CWaitCursor doWaitCursor;
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNodeConnectData connectData(m_strVNodeName, m_strRemoteAddress, m_strProtocol, m_strListenAddress, m_bPrivate);
		if (!m_bAlter)
		{
			hErr = VNODE_ConnectionAdd  (&connectData);

		}
		else
		{
			CaNodeConnectData connectDataOld(m_strOldVNodeName, m_strOldRemoteAddress, m_strOldProtocol, m_strOldListenAddress, m_bOldPrivate);
			hErr = VNODE_ConnectionAlter(&connectDataOld, &connectData);
		}
		if (hErr != NOERROR)
		{
			CString strMessage;
			if (m_bAlter)
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ALTER_SERVER_DATA_FAILED, m_strOldVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ALTER_NODE_DATA_FAILED, m_strOldVNodeName);
#endif
			else
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ADD_SERVER_DATA_FAILED, m_strVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ADD_NODE_DATA_FAILED, m_strVNodeName);
#endif
			AfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		CDialog::OnOK();
	}
	catch (CeNodeException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch (...)
	{
		AfxMessageBox(_T("Internal error: raise exception in CxDlgVirtualNodeData::OnOK()"));
	}
}

void CxDlgVirtualNodeData::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CxDlgVirtualNodeData::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nID = m_bAlter? IDHELP_IDD_VNODEDATA_ALTER: IDHELP_IDD_VNODEDATA_ADD;
	return APP_HelpInfo (nID);
}

void CxDlgVirtualNodeData::InitProtocol()
{
#if defined (EDBC)
	const int nCount = 1;
	TCHAR tchszProtocol [nCount][32] = {_T("tcp_ip")};
#else
	const int nCount = 5;
	TCHAR tchszProtocol [nCount][32] = 
	{
		_T("tcp_ip"),
		_T("lanman"),
		_T("nvlspx"),
		_T("decnet"),
		_T("wintcp")
	};
#endif
	m_cCombo1.ResetContent();
	for (int i=0; i<nCount; i++)
	{
		m_cCombo1.AddString (tchszProtocol[i]);
	}
}
