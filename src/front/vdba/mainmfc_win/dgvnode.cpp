/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/* 
**    Source   : dgvnode.cpp, Implementation File.
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut, Detail implementation.
**    Purpose  : Popup Dialog Box for Creating the Virtual Node.
**               The convenient dialog box. 
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 07-Sep-2001 (uk$so01)
**    BUG #105704 ("Private" checkbox of of Login data is not stored correctly when adding vnode)
** 19-Sep-2001 (uk$so01)
**    BUG #105806, Fixe the "Private" checkbox problem on Alter Virtual Node.
** 09-Sep-2004 (uk$so01)
**    BUG #112804, ISSUE #13403470, If user alters the virtual node and changes
**    only the connection data information, then the password should not be
**    needed to re-enter.
** 18-Feb-2005 (komve01)
**    BUG #113915, ISSUE #13965039 
**    Check if UpdateData has failed due to an error before we proceed
**    on an OK button click.
** 16-Oct-2006 (wridu01)
**    Bug #116863 Increase maximum vnodeaddress length from 16 characters to
**                63 characters for IPV6 Project
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgvnode.h"
#include "starutil.h"
extern "C"
{
#include "msghandl.h"
#include "dba.h"
#include "dbaginfo.h"
#include "domdloca.h"
#include "domdata.h"
}

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
    DDV_MaxChars(pDX, m_strRemoteAddress, 63);
    DDX_Text(pDX, IDC_MFC_EDIT6, m_strListenAddress);
    DDV_MaxChars(pDX, m_strListenAddress, 32);
    DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
    DDX_CBString(pDX, IDC_MFC_COMBO1, m_strProtocol);
    //}}AFX_DATA_MAP
    DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditVNodeName);
    DDX_Check(pDX,   IDC_MFC_CHECK_COM_PRIVATE, m_bConnectionPrivate);
    DDX_Check(pDX,   IDC_MFC_CHECK_LOGIN_PRIVATE, m_bLoginPrivate);
    DDX_Control(pDX, IDC_MFC_CHECK_INSTALLATION, m_cCheckInstallationPassword);
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNode, CDialog)
    //{{AFX_MSG_MAP(CxDlgVirtualNode)
    ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeProtocol)
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
    //}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_MFC_CHECK_LOGIN_PRIVATE, OnCheckPrivate)
	ON_BN_CLICKED(IDC_MFC_CHECK_COM_PRIVATE, OnCheckConnectPrivate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNode message handlers

BOOL CxDlgVirtualNode::OnInitDialog() 
{
    CDialog::OnInitDialog();
    m_cOK.EnableWindow (FALSE);

    if (m_bAlter)
        PushHelpId(IDHELP_IDD_VNODE_ALTER);
    else
        PushHelpId(IDHELP_IDD_VNODE_ADD);
   
    m_strOldVNodeName       = m_strVNodeName;
    m_strOldUserName        = m_strUserName;
    m_strOldPassword        = m_strPassword;
    m_strOldConfirmPassword = m_strConfirmPassword;
    m_strOldRemoteAddress   = m_strRemoteAddress;
    m_strOldListenAddress   = m_strListenAddress;
    m_bOldConnectionPrivate = m_bConnectionPrivate;
    m_bOldLoginPrivate      = m_bLoginPrivate;
    m_strOldProtocol        = m_strProtocol;


    if (m_bAlter)
    {
        CString strTitle;
        if (strTitle.LoadString (IDS_ALTER_NODE_TITLE) == 0)
            strTitle = _T("Alter the Definition of Virtual Node");
        m_cCheckInstallationPassword.EnableWindow (FALSE);
        m_cEditVNodeName.SetReadOnly();
        m_cEditVNodeName.EnableWindow (FALSE);
        SetWindowText (strTitle);

        CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT2);
        pEdit->SetFocus();
        pEdit->SetSel (0, -1);
        return FALSE;
    }
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
    m_bDataAltered   = TRUE;    
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
	if(!UpdateData(TRUE))
		return;       //Return if UpdateData failed.

	if (m_strPassword != m_strConfirmPassword)
    {
        CString strPasswordError;
        if (strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD) == 0)
            strPasswordError = _T("Password is not correct.\nPlease retype the password");
        BfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
        return;
    }
    //
    // Ensure that Virtual Node Name must be different from the local host (machine) name
    CString strLocalMachineName = GetLocalHostName();
    if (m_strUserName.CompareNoCase (_T("*")) != 0)
    {
        if (strLocalMachineName.CompareNoCase (m_strVNodeName) == 0)
        {
            //CString strMsg = _T("The Virtual Node Name must be different from the ""local_vnode"" (usually the hostname), "
            //                    "if you are not defining an installation password.");
            AfxMessageBox (VDBA_MfcResourceString(IDS_E_VIRTUAL_NODE));
            return;
        }
    }
    //
    // Check the existance of Virtual Node Name if create mode
    if (!m_bAlter)
    {
        NODEDATAMIN nodedata;
        int ires;
        ires = GetFirstMonInfo (0,0,NULL,OT_NODE, (void *)&nodedata, NULL);
        while (ires == RES_SUCCESS) 
        {
            if (m_strVNodeName.CompareNoCase ((LPCTSTR)nodedata.NodeName) == 0)
            {
                CString strMessage;
                AfxFormatString1 (strMessage, IDS_NODE_ALREADY_EXIST, m_strVNodeName);
                BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
                return;
            }
            ires = GetNextMonInfo((void *)&nodedata);
        }
    }

    //
    // Call the low level to add or change the definition of Virtual Node.
    int result;
    NODEDATAMIN s;
    CWinApp* pApp = AfxGetApp();
    pApp->DoWaitCursor (1);
    memset (&s, 0, sizeof (s));
    s.ConnectDta.bPrivate = m_bConnectionPrivate;
    s.LoginDta.bPrivate   = m_bLoginPrivate;
    s.isSimplifiedModeOK  = TRUE;    // always
    s.inbConnectData = 1;
    s.inbLoginData=1;
    
    lstrcpy ((LPTSTR)s.NodeName,            (LPCTSTR)m_strVNodeName);
    lstrcpy ((LPTSTR)s.LoginDta.NodeName,   (LPCTSTR)m_strVNodeName);
    lstrcpy ((LPTSTR)s.LoginDta.Login,      (LPCTSTR)m_strUserName);
    lstrcpy ((LPTSTR)s.LoginDta.Passwd,     (LPCTSTR)m_strPassword);
    lstrcpy ((LPTSTR)s.ConnectDta.NodeName, (LPCTSTR)m_strVNodeName);
    lstrcpy ((LPTSTR)s.ConnectDta.Address,  (LPCTSTR)m_strRemoteAddress);
    lstrcpy ((LPTSTR)s.ConnectDta.Protocol, (LPCTSTR)m_strProtocol);
    lstrcpy ((LPTSTR)s.ConnectDta.Listen,   (LPCTSTR)m_strListenAddress);

    if (!m_bAlter)
    {
        if (NodeLLInit() == RES_SUCCESS)
        {
            // If defining local node with installation password,
            // check we have no connections through replicator monitoring in ipm
            if (strLocalMachineName.CompareNoCase (m_strVNodeName) == 0)
            {
                ASSERT (m_strUserName.CompareNoCase (_T("*")) == 0);
                int nbConn = NodeConnections((LPUCHAR)(LPCTSTR)m_strVNodeName);
                ASSERT (nbConn >= 0);
                if (nbConn > 0)
                {
                    NodeLLTerminate();
                    //CString strMsg = _T("Please Close All Monitor Windows before defining an installation password node definition");
                    AfxMessageBox (VDBA_MfcResourceString(IDS_E_CLOSE_ALL_MONITOR));
                    return;
                }
            }
            result = LLAddFullNode (&s);
            UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
            NodeLLTerminate();
        }
        else
            result = RES_ERR;
    }
    else
    {
        NODEDATAMIN so;
        
        memset (&so, 0, sizeof (so));
        so.ConnectDta.bPrivate = m_bOldConnectionPrivate;
        so.LoginDta.bPrivate   = m_bOldLoginPrivate;
        so.isSimplifiedModeOK  = TRUE;    // always
        
        lstrcpy ((LPTSTR)so.NodeName,            (LPCTSTR)m_strOldVNodeName);
        lstrcpy ((LPTSTR)so.LoginDta.NodeName,   (LPCTSTR)m_strOldVNodeName);
        lstrcpy ((LPTSTR)so.LoginDta.Login,      (LPCTSTR)m_strOldUserName);
        lstrcpy ((LPTSTR)so.LoginDta.Passwd,     (LPCTSTR)m_strOldPassword);
        lstrcpy ((LPTSTR)so.ConnectDta.NodeName, (LPCTSTR)m_strOldVNodeName);
        lstrcpy ((LPTSTR)so.ConnectDta.Address,  (LPCTSTR)m_strOldRemoteAddress);
        lstrcpy ((LPTSTR)so.ConnectDta.Protocol, (LPCTSTR)m_strOldProtocol);
        lstrcpy ((LPTSTR)so.ConnectDta.Listen,   (LPCTSTR)m_strOldListenAddress);
        if (NodeLLInit() == RES_SUCCESS)
        {
			if (m_bClearPassword)
				result =  LLAlterFullNode (&so, &s);
			else
			{
				NODECONNECTDATAMIN c;
				NODECONNECTDATAMIN co;

				memset (&c, 0, sizeof (c));
				memset (&co, 0, sizeof (co));
				lstrcpy ((LPTSTR)co.NodeName, (LPCTSTR)m_strOldVNodeName);
				lstrcpy ((LPTSTR)co.Address,  (LPCTSTR)m_strOldRemoteAddress);
				lstrcpy ((LPTSTR)co.Protocol, (LPCTSTR)m_strOldProtocol);
				lstrcpy ((LPTSTR)co.Listen,   (LPCTSTR)m_strOldListenAddress);
				co.bPrivate = m_bOldConnectionPrivate ;
				
				c.bPrivate            = m_bConnectionPrivate;
				lstrcpy ((LPTSTR)c.NodeName, (LPCTSTR)m_strVNodeName);
				lstrcpy ((LPTSTR)c.Address,  (LPCTSTR)m_strRemoteAddress);
				lstrcpy ((LPTSTR)c.Protocol, (LPCTSTR)m_strProtocol);
				lstrcpy ((LPTSTR)c.Listen,   (LPCTSTR)m_strListenAddress);

				result =  LLAlterNodeConnectData (&co, &c);

			}
            UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
            NodeLLTerminate();
        }
        else
            result = RES_ERR;
    }
    pApp->DoWaitCursor (-1);
    if (result != RES_SUCCESS)
    {
        CString strMessage;
        if (m_bAlter)
            AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strOldVNodeName);
        else
            AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strVNodeName);
        BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
        return;
    }
    CDialog::OnOK();
}





void CxDlgVirtualNode::OnDestroy() 
{
    CDialog::OnDestroy();
    PopHelpId();
}

void CxDlgVirtualNode::OnCheckInstallationPassword() 
{
    int nCheck = m_cCheckInstallationPassword.GetCheck();
    BOOL bCheckInstallation = (nCheck == 0)? TRUE: FALSE;
    UpdateData (TRUE);
    if (!IsDefaultValues(bCheckInstallation))
    {
        //CString strMsg =_T("The current definition will be lost.\nDo you wish to continue ?");
        int nS = AfxMessageBox (VDBA_MfcResourceString(IDS_E_CURRENT_DEF), MB_ICONQUESTION|MB_YESNO);
        if (nS == IDNO)
        {
            nCheck = (nCheck == 0)? 1: 0;
            m_cCheckInstallationPassword.SetCheck (nCheck);
            return;
        }
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
        m_strProtocol = m_strOldProtocol;

        m_cEditUserName.EnableWindow (TRUE);
        m_cEditListenAddess.EnableWindow (TRUE);
        m_cCombo1.EnableWindow (TRUE);
        m_cEditRemoteAddress.EnableWindow (TRUE);
        (GetDlgItem (IDC_MFC_CHECK_COM_PRIVATE))->EnableWindow (TRUE);
        break;
    case 1:
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
        m_strRemoteAddress = strVNode;
        UpdateData (FALSE);
    }
    m_bVNode2RemoteAddess = FALSE;
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
    if (m_strOldProtocol.CompareNoCase(m_strProtocol) != 0)
        return FALSE;
    return TRUE;
}
