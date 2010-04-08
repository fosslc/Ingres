/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.    	       //
//                                                                                     //
//    Source   : DgVNData.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Connection Data)     //
//               The Advanced Concept of Virtual Node.                                 //
//                                                                                     //
//    History  : 10/16/2006 (wridu01)                                                  //
//               Bug 116863 Increase maximum vnodeaddress length from 16 characters to //
//                          63 characters for IPV6 Project                             //
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "dgvndata.h"

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
    DDX_Control(pDX, IDOK, m_cOK);
    DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
    DDX_Text(pDX, IDC_MFC_EDIT1, m_strVNodeName);
    DDV_MaxChars(pDX, m_strVNodeName, 32);
    DDX_Text(pDX, IDC_MFC_EDIT2, m_strRemoteAddress);
    DDV_MaxChars(pDX, m_strRemoteAddress, 63);
    DDX_Text(pDX, IDC_MFC_EDIT3, m_strListenAddress);
    DDV_MaxChars(pDX, m_strListenAddress, 32);
    DDX_CBString(pDX, IDC_MFC_COMBO1, m_strProtocol);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNodeData, CDialog)
    //{{AFX_MSG_MAP(CxDlgVirtualNodeData)
    ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckPrivate)
    ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeProtocol)
    ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeIPAddress)
    ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangeListenPort)
    ON_WM_DESTROY()
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
    m_cOK.EnableWindow (FALSE);

    if (m_bAlter)
        PushHelpId(IDHELP_IDD_VNODEDATA_ALTER);
    else
        PushHelpId(IDHELP_IDD_VNODEDATA_ADD);

    if (m_bAlter)
    {
        CString strTitle;
        if (strTitle.LoadString (IDS_ALTER_NODEDATA_TITLE) == 0)
            strTitle = _T("Alter the Definition of Virtual Node");
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
    int result;
    NODECONNECTDATAMIN s;
    CWinApp* pApp = AfxGetApp();
    pApp->DoWaitCursor (1);
    memset (&s, 0, sizeof (s));
    s.bPrivate            = m_bPrivate;
   
    lstrcpy ((LPTSTR)s.NodeName, (LPCTSTR)m_strVNodeName);
    lstrcpy ((LPTSTR)s.Address,  (LPCTSTR)m_strRemoteAddress);
    lstrcpy ((LPTSTR)s.Protocol, (LPCTSTR)m_strProtocol);
    lstrcpy ((LPTSTR)s.Listen,   (LPCTSTR)m_strListenAddress);
  
    if (!m_bAlter)
    {
        if (NodeLLInit() == RES_SUCCESS)
        {
            result = LLAddNodeConnectData (&s);
            UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
            NodeLLTerminate();
        }
        else
            result = RES_ERR;
    }
    else
    {
        NODECONNECTDATAMIN so;
        
        memset (&so, 0, sizeof (so));
        lstrcpy ((LPTSTR)so.NodeName, (LPCTSTR)m_strOldVNodeName);
        lstrcpy ((LPTSTR)so.Address,  (LPCTSTR)m_strOldRemoteAddress);
        lstrcpy ((LPTSTR)so.Protocol, (LPCTSTR)m_strOldProtocol);
        lstrcpy ((LPTSTR)so.Listen,   (LPCTSTR)m_strOldListenAddress);
        so.bPrivate = m_bOldPrivate ;
        if (NodeLLInit() == RES_SUCCESS)
        {
            result =  LLAlterNodeConnectData (&so, &s);
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
            AfxFormatString1 (strMessage, IDS_ALTER_NODE_DATA_FAILED, m_strOldVNodeName);
        else
            AfxFormatString1 (strMessage, IDS_ADD_NODE_DATA_FAILED, m_strVNodeName);
        BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
        return;
    }
    CDialog::OnOK();
}

void CxDlgVirtualNodeData::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
