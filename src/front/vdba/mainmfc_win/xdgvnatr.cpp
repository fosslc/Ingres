/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgvnatr.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II./ VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating a VNode Attribute                       //
//               The Advanced concept of Virtual Node.                                 //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "xdgvnatr.h"

extern "C"
{
#include "msghandl.h"
#include "dba.h"
#include "dbaginfo.h"
#include "domdloca.h"
#include "domdata.h"
#include "monitor.h"
}
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
		//if (strTitle.LoadString (IDS_ALTER_NODE_TITLE) == 0)
		strTitle.LoadString(IDS_ALTER_NODE_ATTRIBUTE);// = _T("Alter a Node Attribute");
		SetWindowText (strTitle);
		m_cComboAttribute.EnableWindow (FALSE);
		m_cComboAttribute.SetWindowText (m_strAttributeName);

		m_strOldAttributeName  = m_strAttributeName;
		m_strOldAttributeValue = m_strAttributeValue;
		m_bOldPrivate          = m_bPrivate;
	}
	PushHelpId(m_bAlter? HELPID_IDD_NODE_ATTRIBUTE_ALTER: HELPID_IDD_NODE_ATTRIBUTE_ADD);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeAttribute::OnOK() 
{
	int result  = RES_ERR;
	UpdateData (TRUE);
	NODEATTRIBUTEDATAMIN nattr, noldattr;
	memset (&nattr,    0, sizeof(nattr));
	memset (&noldattr, 0, sizeof(noldattr));
	m_cComboAttribute.GetWindowText (m_strAttributeName);
	m_strAttributeName.TrimLeft();
	m_strAttributeName.TrimRight();

	//
	// New:
	lstrcpy ((LPTSTR)nattr.NodeName,       m_strVNodeName);
	lstrcpy ((LPTSTR)nattr.AttributeName,  m_strAttributeName);
	lstrcpy ((LPTSTR)nattr.AttributeValue, m_strAttributeValue);
	nattr.bPrivate = m_bPrivate;
	//
	// Old:
	lstrcpy ((LPTSTR)noldattr.NodeName,       m_strVNodeName);
	lstrcpy ((LPTSTR)noldattr.AttributeName,  m_strOldAttributeName);
	lstrcpy ((LPTSTR)noldattr.AttributeValue, m_strOldAttributeValue);
	noldattr.bPrivate = m_bOldPrivate;
	if (NodeLLInit() == RES_SUCCESS)
	{
		if (!m_bAlter)
		{
			result = LLAddNodeAttributeData (&nattr);
		}
		else
		{
			result = LLAlterNodeAttributeData (&noldattr, &nattr);
		}
		UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
		NodeLLTerminate();
	}
	else
		result = RES_ERR;

	if (result != RES_SUCCESS)
	{
		CString strMessage;
		if (m_bAlter)
			//AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strOldVNodeName);
			strMessage = VDBA_MfcResourceString(IDS_E_ALTER_NODE_ATTRIBUTE);//_T("Alter a Node Attribute failed.");
		else
			//AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strVNodeName);
			strMessage = VDBA_MfcResourceString(IDS_E_ADD_NODE_ATTRIBUTE);//_T("Add a Node Attribute failed.");
		BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	

	CDialog::OnOK();
}

void CxDlgVirtualNodeAttribute::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
