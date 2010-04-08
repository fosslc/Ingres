/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdgvnatr.cpp, Implementation File 
** 
**    Project  : Ingres II./ VDBA 
**    Author   : UK Sotheavut
** 
**    Purpose  : Popup Dialog Box for Creating a VNode Attribute
**               The Advanced concept of Virtual Node.
** History:
** xx-Mar-1999 (uk$so01)
**    Modified on march 1999 to be used independently in the new application
**    for managing the virtual node
** 14-may-2001 (zhayu01) SIR 104724
**    Vnode becomes Server for EDBC
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdgvnatr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeAttribute dialog


CxDlgVirtualNodeAttribute::CxDlgVirtualNodeAttribute(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgVirtualNodeAttribute::IDD, pParent)
{
	m_strVNodeName = _T("");
	m_bAlter = FALSE;
	//{{AFX_DATA_INIT(CxDlgVirtualNodeAttribute)
	m_strAttributeName = _T("");
	m_strAttributeValue = _T("");
	m_bPrivate = FALSE;
	//}}AFX_DATA_INIT

	m_strOldAttributeName = _T("");
	m_strOldAttributeValue= _T("");
	m_bOldPrivate = TRUE;
}


void CxDlgVirtualNodeAttribute::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgVirtualNodeAttribute)
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cComboAttribute);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strAttributeValue);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNodeAttribute, CDialog)
	//{{AFX_MSG_MAP(CxDlgVirtualNodeAttribute)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeAttribute message handlers

BOOL CxDlgVirtualNodeAttribute::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_bAlter)
	{
		CString strTitle;
#ifdef	EDBC
		if (!strTitle.LoadString (IDS_ALTER_SERVER_TITLE))
			strTitle = _T("Alter a Server Attribute");
#else
		if (!strTitle.LoadString (IDS_ALTER_NODE_TITLE))
			strTitle = _T("Alter a Node Attribute");
#endif
		SetWindowText (strTitle);
		m_cComboAttribute.EnableWindow (FALSE);
		m_cComboAttribute.SetWindowText (m_strAttributeName);

		m_strOldAttributeName  = m_strAttributeName;
		m_strOldAttributeValue = m_strAttributeValue;
		m_bOldPrivate          = m_bPrivate;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeAttribute::OnOK() 
{
	UpdateData (TRUE);
	HRESULT hErr = NOERROR;
	try
	{
		CWaitCursor doWaitCursor;
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNodeAttribute attr(m_strVNodeName, m_strAttributeName, m_strAttributeValue, m_bPrivate);
		if (!m_bAlter)
		{
			hErr = VNODE_AttributeAdd (&attr);
		}
		else
		{
			CaNodeAttribute attrOld(m_strVNodeName, m_strOldAttributeName, m_strOldAttributeValue, m_bOldPrivate);
			hErr = VNODE_AttributeAlter (&attrOld, &attr);
		}

		if (hErr != NOERROR)
		{
			CString strMessage;
			if (m_bAlter)
				// _T("Alter a Node Attribute failed.");
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ALTER_SERVER_FAILED, m_strVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strVNodeName);
#endif
			else
				// _T("Add a Node Attribute failed.");
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ADD_SERVER_FAILED, m_strVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strVNodeName);
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
		AfxMessageBox(_T("Internal error: raise exception in CxDlgVirtualNodeAttribute::OnOK()"));
	}
}

void CxDlgVirtualNodeAttribute::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CxDlgVirtualNodeAttribute::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nID = m_bAlter? HELPID_IDD_NODE_ATTRIBUTE_ALTER: HELPID_IDD_NODE_ATTRIBUTE_ADD;
	return APP_HelpInfo (nID);
}