/*
**  Copyright (C) 2005-2008 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgvnode.cpp, Implementation File.
** 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut, Detail implementation.
** 
**    Purpose  : Popup Dialog Box for Creating the Virtual Node.
**               The convenient dialog box.
**	History
** xx-Mar-1997 (uk$so01)
**    Created
** xx-Mar-1999 (uk$so01)
**    Modified  to be used independently in the new application for managing the virtual node
** 14-may-2001 (zhayu01) SIR 104724
**    Vnode becomes Server for EDBC
** 16-may-2001 (zhayu01) SIR 104730
**    Do not propagate the server name to the host.
** 17-may-2001 (zhayu01) Bug 104735
**    Map the OnCheckPrivate() with the IDC_MFC_CHECK_LOGIN_PRIVATE instead
**	  of IDC_MFC_CHECK1 for EDBC.
** 21-may-2001 (zhayu01) SIR 104751
**    Handle the new Save login check box for EDBC.
** 27-aug-2001 (zhayu01) SIR 105616 for EDBC
**    Make 51 the maximum length for Host and 5 the maximum length for Listen Address.
** 07-Sep-2001 (uk$so01)
**    BUG #105704 ("Private" checkbox of of Login data is not stored correctly when adding vnode)
** 19-Sep-2001 (uk$so01)
**    BUG #105806, Fixe the "Private" checkbox problem on Alter Virtual Node.
** 25-Sep-2001 (noifr01)
**    propagated change 450613, which after merging results in no change, given that an
**    equivalent change has been done inbetween (and not #ifdef'ed EDBC)
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (schph01)
**    SIR #109864, when ingnet is launched with -vnode option retrieve the
**    local_vnode name with INGRESII_QueryNetLocalVnode() function instead
**    of GCHostname()
** 11-Aug-2004 (uk$so01)
**    BUG #112804, ISSUE #13403470, If user alters the virtual node and changes
**    only the connection data information, then the password should not be 
**    needed to re-enter.
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
#include "dgvnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CxDlgVirtualNode::CxDlgVirtualNode(CWnd* pParent /*=NULL*/)
    : CDialog(CxDlgVirtualNode::IDD, pParent)
{
	m_bAlter = FALSE;
	m_bClearPassword = TRUE;
	m_bDataAltered   = FALSE;
	//{{AFX_DATA_INIT(CxDlgVirtualNode)
	m_strVNodeName = _T("");
	m_strUserName = _T("");
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	m_strRemoteAddress = _T("");
	m_strListenAddress = _T("");
	m_bPrivate = FALSE;
	m_strProtocol = _T("");
	//}}AFX_DATA_INIT
	m_bConnectionPrivate = FALSE;
	m_bLoginPrivate = FALSE;
#ifdef EDBC
	m_bLoginSave = FALSE;
#endif
	m_bVNode2RemoteAddess = FALSE; // TRUE: copy VNODE to RemoteAddress
}


void CxDlgVirtualNode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgVirtualNode)
	DDX_Control(pDX, IDC_MFC_EDIT2, m_cEditUserName);
	DDX_Control(pDX, IDC_MFC_EDIT6, m_cEditListenAddess);
	DDX_Control(pDX, IDC_MFC_EDIT5, m_cEditRemoteAddress);
	DDX_Control(pDX, IDC_MFC_EDIT4, m_cEditConfirmPassword);
	DDX_Control(pDX, IDC_MFC_EDIT3, m_cEditPassword);
	DDX_Control(pDX, IDOK, m_cOK);
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cCombo1);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strVNodeName);
	DDV_MaxChars(pDX, m_strVNodeName, 32);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strUserName);
	DDV_MaxChars(pDX, m_strUserName, 24);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 16);
	DDX_Text(pDX, IDC_MFC_EDIT4, m_strConfirmPassword);
	DDV_MaxChars(pDX, m_strConfirmPassword, 16);
	DDX_Text(pDX, IDC_MFC_EDIT5, m_strRemoteAddress);
#ifdef	EDBC
	DDV_MaxChars(pDX, m_strRemoteAddress, 51);
#else
	DDV_MaxChars(pDX, m_strRemoteAddress, 63);
#endif
	DDX_Text(pDX, IDC_MFC_EDIT6, m_strListenAddress);
#ifdef	EDBC
	DDV_MaxChars(pDX, m_strListenAddress, 5);
#else
	DDV_MaxChars(pDX, m_strListenAddress, 32);
#endif
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
	DDX_CBString(pDX, IDC_MFC_COMBO1, m_strProtocol);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditVNodeName);
	DDX_Check(pDX,   IDC_MFC_CHECK_COM_PRIVATE, m_bConnectionPrivate);
	DDX_Check(pDX,   IDC_MFC_CHECK_LOGIN_PRIVATE, m_bLoginPrivate);
#ifdef	EDBC
	DDX_Check(pDX,   IDC_MFC_CHECK_LOGIN_SAVE, m_bLoginSave);
#endif
	DDX_Control(pDX, IDC_MFC_CHECK_INSTALLATION, m_cCheckInstallationPassword);
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNode, CDialog)
	//{{AFX_MSG_MAP(CxDlgVirtualNode)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeProtocol)
#ifdef	EDBC
	ON_BN_CLICKED(IDC_MFC_CHECK_LOGIN_SAVE, OnCheckSave)
#endif
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeVNodeName)
	ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeUserName)
	ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangePassword)
	ON_EN_CHANGE(IDC_MFC_EDIT4, OnChangeConfirmPassword)
	ON_EN_CHANGE(IDC_MFC_EDIT5, OnChangeIPAddress)
	ON_EN_CHANGE(IDC_MFC_EDIT6, OnChangeListenPort)
	ON_EN_SETFOCUS(IDC_MFC_EDIT3, OnSetfocusPassword)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT3, OnKillfocusPassword)
	ON_EN_SETFOCUS(IDC_MFC_EDIT4, OnSetfocusConfirmPassword)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT4, OnKillfocusConfirmPassword)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_MFC_CHECK_INSTALLATION, OnCheckInstallationPassword)
	ON_EN_SETFOCUS(IDC_MFC_EDIT1, OnSetfocusVNodeName)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT1, OnKillfocusVNodeName)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_MFC_CHECK_LOGIN_PRIVATE, OnCheckPrivate)
	ON_BN_CLICKED(IDC_MFC_CHECK_COM_PRIVATE, OnCheckConnectPrivate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNode message handlers

BOOL CxDlgVirtualNode::OnInitDialog() 
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
	m_strOldVNodeName       = m_strVNodeName;
	m_strOldUserName        = m_strUserName;
	m_strOldPassword        = m_strPassword;
	m_strOldConfirmPassword = m_strConfirmPassword;
	m_strOldRemoteAddress   = m_strRemoteAddress;
	m_strOldListenAddress   = m_strListenAddress;
	m_bOldConnectionPrivate = m_bConnectionPrivate;
	m_bOldLoginPrivate      = m_bLoginPrivate;
#ifdef	EDBC
	m_bOldLoginSave         = m_bLoginSave;
#endif
	m_strOldProtocol        = m_strProtocol;


	if (m_bAlter)
	{
		CString strTitle;
#ifdef	EDBC
		if (!strTitle.LoadString (IDS_ALTER_SERVER_TITLE))
			strTitle = _T("Alter the Definition of Server");
#else
		if (!strTitle.LoadString (IDS_ALTER_NODE_TITLE))
			strTitle = _T("Alter the Definition of Virtual Node");
#endif
		m_cCheckInstallationPassword.EnableWindow (FALSE);
		m_cEditVNodeName.SetReadOnly();
		m_cEditVNodeName.EnableWindow (FALSE);
		SetWindowText (strTitle);

		CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT2);
		pEdit->SetFocus();
		pEdit->SetSel (0, -1);
#ifdef	EDBC
		if (!m_bLoginSave)
		{
			m_cEditUserName.SetWindowText(_T("")); 
			m_cEditPassword.SetWindowText(_T("")); 
			m_cEditConfirmPassword.SetWindowText(_T("")); 
			m_cEditUserName.EnableWindow(FALSE);
			m_cEditPassword.EnableWindow(FALSE);
			m_cEditConfirmPassword.EnableWindow(FALSE);
			m_cOK.EnableWindow (FALSE);
		}
#endif
		return FALSE;
	}

#ifdef	EDBC
	m_cEditUserName.EnableWindow(FALSE);
	m_cEditPassword.EnableWindow(FALSE);
	m_cEditConfirmPassword.EnableWindow(FALSE);
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNode::ClearPassword()
{
	if (!m_bAlter)
		return;
	if (m_bClearPassword)
		return;
	m_bClearPassword = TRUE;   
	m_cEditPassword       .SetWindowText(_T("")); 
	m_cEditConfirmPassword.SetWindowText(_T(""));
}

void CxDlgVirtualNode::EnableOK()
{
	BOOL bCheckPassword = FALSE;
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
#ifdef	EDBC
	if (m_bLoginSave)
	{
	bCheckPassword = (m_strPassword .GetLength() == 0 || m_strConfirmPassword.GetLength() == 0)? TRUE: FALSE;
	if (m_strVNodeName.GetLength() > 0 && m_strUserName.CompareNoCase (_T("*")) == 0 && bCheckPassword != TRUE)
	{
		m_cOK.EnableWindow (TRUE);
		return;
	}
	if (m_strVNodeName.GetLength() == 0         ||
		m_strUserName .GetLength() == 0         ||
		bCheckPassword ||
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
	else
	{
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
#else
	bCheckPassword = (m_strPassword .GetLength() == 0 || m_strConfirmPassword.GetLength() == 0)? TRUE: FALSE;
	if (m_strVNodeName.GetLength() > 0 && m_strUserName.CompareNoCase (_T("*")) == 0 && bCheckPassword != TRUE)
	{
		m_cOK.EnableWindow (TRUE);
		return;
	}
	if (m_strVNodeName.GetLength() == 0         ||
		m_strUserName .GetLength() == 0         ||
		bCheckPassword ||
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
#endif
}


void CxDlgVirtualNode::OnSelchangeProtocol() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNode::OnCheckPrivate() 
{
	m_bDataAltered = TRUE;
	ClearPassword();
	EnableOK();
}

void CxDlgVirtualNode::OnCheckConnectPrivate() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

#ifdef	EDBC
void CxDlgVirtualNode::OnCheckSave() 
{
	m_bDataAltered = TRUE;
	UpdateData();
	if (m_bLoginSave)
	{
		m_cEditUserName.EnableWindow(TRUE);
		m_cEditPassword.EnableWindow(TRUE);
		m_cEditConfirmPassword.EnableWindow(TRUE);
		m_cEditUserName.SetFocus();
	}
	else
	{
		m_cEditUserName.SetWindowText(_T("")); 
		m_cEditPassword.SetWindowText(_T("")); 
		m_cEditConfirmPassword.SetWindowText(_T("")); 
		m_cEditUserName.EnableWindow(FALSE);
		m_cEditPassword.EnableWindow(FALSE);
		m_cEditConfirmPassword.EnableWindow(FALSE);
	}
	EnableOK();
}
#endif

void CxDlgVirtualNode::OnChangeVNodeName() 
{
	m_bDataAltered = TRUE;
	ClearPassword();
	EnableOK();
}

void CxDlgVirtualNode::OnChangeUserName() 
{
	m_bDataAltered = TRUE;
	ClearPassword();
	EnableOK();
}

void CxDlgVirtualNode::OnChangePassword() 
{
	m_bDataAltered = TRUE;
	if (!m_bClearPassword)
	{
		m_bClearPassword = TRUE;
		m_cEditConfirmPassword.SetWindowText(_T(""));
	}
	EnableOK();
}

void CxDlgVirtualNode::OnChangeConfirmPassword() 
{
	m_bDataAltered = TRUE;
	if (!m_bClearPassword)
	{
		m_bClearPassword = TRUE;
		m_cEditPassword.SetWindowText(_T(""));
	}
	EnableOK();
}

void CxDlgVirtualNode::OnChangeIPAddress() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNode::OnChangeListenPort() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNode::OnSetfocusPassword() 
{
	if (!m_bDataAltered)
	{
		m_strPassword = _T("");
		UpdateData (FALSE);
	}
}


void CxDlgVirtualNode::OnKillfocusPassword() 
{
	if (!m_bDataAltered)
	{
		m_strPassword = _T("**********");
		UpdateData (FALSE);
	}
}

void CxDlgVirtualNode::OnSetfocusConfirmPassword() 
{
	if (!m_bDataAltered)
	{
		m_strConfirmPassword = _T("");
		UpdateData (FALSE);
	}
}

void CxDlgVirtualNode::OnKillfocusConfirmPassword() 
{
	if (!m_bDataAltered)
	{
		m_strConfirmPassword = _T("**********");
		UpdateData (FALSE);
	}
}

void CxDlgVirtualNode::OnOK() 
{
	UpdateData(TRUE);
	CWaitCursor doWaitCursor;
	if (m_strPassword != m_strConfirmPassword)
	{
		CString strPasswordError;
		if (!strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD))
			strPasswordError = _T("Password is not correct.\nPlease retype the password");
		AfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	CTypedPtrList<CObList, CaDBObject*> listNode;
	try
	{
		CString strLocalMachineName;
		//
		// Ensure that Virtual Node Name must be different from the local host (machine) name
		if ( theApp.m_pNodeAccess->IsVnodeOptionDefined() )
			INGRESII_QueryNetLocalVnode ( strLocalMachineName );
		else
			strLocalMachineName = GetLocalHostName();
		if (m_strUserName.CompareNoCase (_T("*")) != 0)
		{
			if (strLocalMachineName.CompareNoCase (m_strVNodeName) == 0)
			{
				AfxMessageBox (IDS_E_VIRTUAL_NODE, MB_OK, 0);
				return;
			}
		}
		//
		// Check the existance of Virtual Node Name if create mode
		HRESULT hErr = VNODE_QueryNode (listNode);
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		if (!m_bAlter)
		{
			POSITION pos = listNode.GetHeadPosition();
			while (pos != NULL)
			{
				CaNode* pNode = (CaNode*)listNode.GetNext(pos);
				if (!m_strVNodeName.IsEmpty() && m_strVNodeName.CompareNoCase(pNode->GetName()) == 0)
				{
					CString strMessage;
#ifdef EDBC
					AfxFormatString1 (strMessage, IDS_SERVER_ALREADY_EXIST, m_strVNodeName);
#else
					AfxFormatString1 (strMessage, IDS_NODE_ALREADY_EXIST, m_strVNodeName);
#endif
					AfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
					throw (int)1;
				}
			}
		}

		//
		// Call the low level to add or change the definition of Virtual Node.
		CaNode node(m_strVNodeName);
#if defined (EDBC)
		node.SetSaveLogin(m_bLoginSave);
#endif
		node.SetLogin(m_strUserName);
		node.SetPassword(m_strPassword);
		node.SetIPAddress(m_strRemoteAddress);
		node.SetProtocol (m_strProtocol);
		node.SetListenAddress(m_strListenAddress);
		node.ConnectionPrivate(m_bConnectionPrivate);
		node.LoginPrivate(m_bLoginPrivate);
		node.SetNbConnectData (1);
		node.SetNbLoginData(1);
		if (!m_bAlter)
		{
			hErr = VNODE_AddNode (&node);
		}
		else
		{
			CaNode nodeOld(m_strOldVNodeName);
	#if defined (EDBC)
			nodeOld.SetSaveLogin(m_bOldLoginSave);
	#endif
			nodeOld.SetLogin(m_strOldUserName);
			nodeOld.SetPassword(m_strOldPassword);
			nodeOld.SetIPAddress(m_strOldRemoteAddress);
			nodeOld.SetProtocol (m_strOldProtocol);
			nodeOld.SetListenAddress(m_strOldListenAddress);
			nodeOld.ConnectionPrivate(m_bOldConnectionPrivate);
			nodeOld.LoginPrivate(m_bOldLoginPrivate);
			nodeOld.SetNbConnectData (1);
			nodeOld.SetNbLoginData(1);
			if (m_bClearPassword)
				hErr = VNODE_AlterNode (&nodeOld, &node);
			else
			{
				CaNodeConnectData connectData(m_strVNodeName, m_strRemoteAddress, m_strProtocol, m_strListenAddress, m_bConnectionPrivate);
				CaNodeConnectData connectDataOld(m_strOldVNodeName, m_strOldRemoteAddress, m_strOldProtocol, m_strOldListenAddress, m_bOldConnectionPrivate);
				hErr = VNODE_ConnectionAlter(&connectDataOld, &connectData);
			}
		}
		if (hErr != NOERROR)
		{
			CString strMessage;
			if (m_bAlter)
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ALTER_SERVER_FAILED, m_strOldVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strOldVNodeName);
#endif
			else
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ADD_SERVER_FAILED, m_strVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strVNodeName);
#endif
			AfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
		}
		CDialog::OnOK();
	}
	catch (CeNodeException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch (int nNumber)
	{
		if (nNumber == 1)
		{
			TRACE0("CxDlgVirtualNode::OnOK, node already exists\n");
		}
	}
	catch (...)
	{
		AfxMessageBox(_T("Internal error: raise exception in CxDlgVirtualNode::OnOK()"));
	}
	while (!listNode.IsEmpty())
		delete listNode.RemoveHead();
}


void CxDlgVirtualNode::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CxDlgVirtualNode::OnCheckInstallationPassword() 
{
	int nCheck = m_cCheckInstallationPassword.GetCheck();
	BOOL bCheckInstallation = (nCheck == 0)? TRUE: FALSE;
	UpdateData (TRUE);
	if (!IsDefaultValues(bCheckInstallation))
	{
	}
	m_cEditVNodeName.EnableWindow (bCheckInstallation);
	switch (nCheck)
	{
	case 0:
		m_strVNodeName = m_strOldVNodeName;
		m_strUserName  = m_strOldUserName;
		m_strPassword  = m_strOldPassword;
		m_strConfirmPassword  = m_strOldConfirmPassword;
		m_strRemoteAddress = m_strOldRemoteAddress;
		m_strListenAddress = m_strOldListenAddress;
		m_bConnectionPrivate = m_bOldConnectionPrivate;
		m_bLoginPrivate    = m_bOldLoginPrivate;
#ifdef	EDBC
		m_bLoginSave    = m_bOldLoginSave;
#endif
		m_strProtocol = m_strOldProtocol;

		m_cEditUserName.EnableWindow (TRUE);
		m_cEditListenAddess.EnableWindow (TRUE);
		m_cCombo1.EnableWindow (TRUE);
		m_cEditRemoteAddress.EnableWindow (TRUE);
		(GetDlgItem (IDC_MFC_CHECK_COM_PRIVATE))->EnableWindow (TRUE);
		break;
	case 1:
		if ( theApp.m_pNodeAccess->IsVnodeOptionDefined() )
			INGRESII_QueryNetLocalVnode ( m_strVNodeName );
		else
			m_strVNodeName = GetLocalHostName();
		m_strUserName  = _T("*");
		m_strPassword  = _T("");
		m_strOldConfirmPassword  = _T("");
		m_strRemoteAddress = _T("");
		m_cEditUserName.EnableWindow (FALSE);
		m_cEditListenAddess.EnableWindow (FALSE);
		m_cCombo1.EnableWindow (FALSE);
		m_cEditRemoteAddress.EnableWindow (FALSE);
		(GetDlgItem (IDC_MFC_CHECK_COM_PRIVATE))->EnableWindow (FALSE);
		break;
	default:
		break;
	}
	UpdateData (FALSE);
}

void CxDlgVirtualNode::OnSetfocusVNodeName() 
{
	CString strVNode;
	CString strRemodeAddr;
	m_cEditVNodeName.GetWindowText (strVNode);
	m_cEditRemoteAddress.GetWindowText (strRemodeAddr);
	if (strVNode.CompareNoCase (strRemodeAddr) == 0)
		m_bVNode2RemoteAddess = TRUE;
	else
		m_bVNode2RemoteAddess = FALSE;
}

void CxDlgVirtualNode::OnKillfocusVNodeName() 
{
	if (m_bVNode2RemoteAddess)
	{
		CString strVNode;
		m_cEditVNodeName.GetWindowText (strVNode);
		if (!strVNode.IsEmpty() && strVNode.GetLength() <= 16)
		{
#ifndef	EDBC
/*
** SIR 104730
** Do not propagate the server name to the host text field.
*/
			m_strRemoteAddress = strVNode;
			UpdateData (FALSE);
#endif
			m_bVNode2RemoteAddess = FALSE;
		}
	}
}


BOOL CxDlgVirtualNode::IsDefaultValues(BOOL bCheckInstallation)
{
	//
	// Installation password: -> ignore VNode Name and User Name.
	if (!bCheckInstallation && m_strOldVNodeName.CompareNoCase(m_strVNodeName) != 0)
		return FALSE;
	if (!bCheckInstallation && m_strOldUserName.CompareNoCase(m_strUserName) != 0)
		return FALSE;
	if (m_strOldPassword.CompareNoCase (m_strPassword) != 0)
		return FALSE;
	if (m_strOldConfirmPassword.CompareNoCase(m_strConfirmPassword) != 0)
		return FALSE;
	if (m_strOldRemoteAddress.CompareNoCase(m_strRemoteAddress) != 0)
		return FALSE;
	if (m_strOldListenAddress.CompareNoCase(m_strListenAddress) != 0)
		return FALSE;
	if (m_bOldConnectionPrivate != m_bConnectionPrivate)
		return FALSE;
	if (m_bOldLoginPrivate != m_bLoginPrivate)
		return FALSE;
#ifdef	EDBC
	if (m_bOldLoginSave != m_bLoginSave)
		return FALSE;
#endif
	if (m_strOldProtocol.CompareNoCase(m_strProtocol) != 0)
		return FALSE;
	return TRUE;
}

BOOL CxDlgVirtualNode::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nID = m_bAlter? IDHELP_IDD_VNODE_ALTER: IDHELP_IDD_VNODE_ADD;
	return APP_HelpInfo (nID);
}


void CxDlgVirtualNode::InitProtocol()
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
