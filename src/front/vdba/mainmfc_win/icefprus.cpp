// icefprus.cpp : implementation file
//

#include "stdafx.h"
#include "mainmfc.h"
#include "icefprus.h"
#include "dgerrh.h"

extern "C"
{
#include "dba.h"
#include "domdata.h"
#include "msghandl.h"
#include "ice.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgBuDefRoleUserForFacetPage( int nType ,LPVOID lpStruct,int nHnode)
{
	CxDlgIceBuDefRoleUserForFacetPage dlg;
	dlg.m_pStructObj = lpStruct;
	dlg.m_csCurrentVnodeName = GetVirtNodeName (nHnode);
	dlg.m_nHnode = nHnode;
	dlg.m_nTypeObj = nType;
	switch (nType)
	{
	case OT_ICE_BUNIT_FACET_ROLE:
		dlg.m_csStaticValue.LoadString(IDS_ICE_STATIC_ROLE);
		if (((LPICEBUSUNITDOCROLEDATA)lpStruct)->bAlter)
		{
			//"Alter Role Access Definition for Facet %s"
			dlg.m_csTitle.Format(IDS_ICE_ALTER_BUNIT_FACET_ROLE_TITLE,
				((LPICEBUSUNITDOCROLEDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_ALTER_BUNIT_FACET_ROLE;
			//"Cannot Alter Role Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ALTER_ROLE_FAILED;
		}
		else
		{
			//"Role Access Definition for Page %s"
			dlg.m_csTitle.Format(IDS_ICE_BUNIT_FACET_ROLE_TITLE,((LPICEBUSUNITDOCROLEDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_BUNIT_FACET_ROLE;
			//"Cannot Add Role Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ADD_ROLE_FAILED;
		}
		break;
	case OT_ICE_BUNIT_FACET_USER:
		dlg.m_csStaticValue.LoadString(IDS_ICE_STATIC_USER);
		if (((LPICEBUSUNITDOCUSERDATA)lpStruct)->bAlter)
		{
			//"Alter User Access Definition for Facet %s"
			dlg.m_csTitle.Format(IDS_ICE_BUNIT_FACET_USER_TITLE,((LPICEBUSUNITDOCUSERDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_ALTER_BUNIT_FACET_USER;
			//"Cannot Alter User Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ALTER_USER_FAILED;
		}
		else
		{
			dlg.m_nHelpId       = IDD_ICE_BU_FACETPAGE_ROLEUSER;
			//"Cannot Add User Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ADD_USER_FAILED;
		}
		break;
	case OT_ICE_BUNIT_PAGE_ROLE:
		//IDS_ICE_STATIC_ROLE = "Role:"
		dlg.m_csStaticValue.LoadString(IDS_ICE_STATIC_ROLE);
		if (((LPICEBUSUNITDOCROLEDATA)lpStruct)->bAlter)
		{
			//"Alter Role Access Definition for Page %s"
			dlg.m_csTitle.Format(IDS_ICE_ALTER_BUNIT_PAGE_ROLE_TITLE,((LPICEBUSUNITDOCROLEDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_ALTER_BUNIT_PAGE_ROLE;
			//"Cannot Alter Role Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ALTER_ROLE_FAILED;
		}
		else
		{
			//"Role Access Definition for Page %s"
			dlg.m_csTitle.Format(IDS_ICE_BUNIT_PAGE_ROLE_TITLE,((LPICEBUSUNITDOCROLEDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_BUNIT_PAGE_ROLE;
			//"Cannot Add Role Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ADD_ROLE_FAILED;
		}
		break;
	case OT_ICE_BUNIT_PAGE_USER:
		dlg.m_csStaticValue.LoadString(IDS_ICE_STATIC_USER);
		if (((LPICEBUSUNITDOCUSERDATA)lpStruct)->bAlter)
		{
			//"Alter User Access Definition for Page %s"
			dlg.m_csTitle.Format(IDS_ICE_ALTER_BUNIT_PAGE_USER_TITLE,((LPICEBUSUNITDOCUSERDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_ALTER_BUNIT_PAGE_USER;
			//"Cannot Alter User Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ALTER_USER_FAILED;
		}
		else
		{
			//"User Access Definition for Page %s"
			dlg.m_csTitle.Format(IDS_ICE_BUNIT_PAGE_USER_TITLE,((LPICEBUSUNITDOCUSERDATA)lpStruct)->icebusunitdoc.name);
			dlg.m_nHelpId       = HELPID_ICE_BUNIT_PAGE_USER;
			//"Cannot Add User Access Definition."
			dlg.m_nMessageID    = IDS_E_ICE_ADD_USER_FAILED;
		}
		break;
	default:
		ASSERT(FALSE);
		return FALSE;
	}

	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBuDefRoleUserForFacetPage dialog


CxDlgIceBuDefRoleUserForFacetPage::CxDlgIceBuDefRoleUserForFacetPage(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceBuDefRoleUserForFacetPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceBuDefRoleUserForFacetPage)
	m_bDelete = FALSE;
	m_bExecute = FALSE;
	m_bRead = FALSE;
	m_bUpdate = FALSE;
	m_csCurrentSelected = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceBuDefRoleUserForFacetPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceBuDefRoleUserForFacetPage)
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_COMBO_ROLE_USER, m_ctrlCombo);
	DDX_Control(pDX, IDC_STATIC_TYPE, m_ctrlStaticType);
	DDX_Check(pDX, IDC_CHECK_DELETE, m_bDelete);
	DDX_Check(pDX, IDC_CHECK_EXECUTE, m_bExecute);
	DDX_Check(pDX, IDC_CHECK_READ, m_bRead);
	DDX_Check(pDX, IDC_CHECK_UPDATE, m_bUpdate);
	DDX_CBString(pDX, IDC_COMBO_ROLE_USER, m_csCurrentSelected);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceBuDefRoleUserForFacetPage, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceBuDefRoleUserForFacetPage)
	ON_CBN_EDITCHANGE(IDC_COMBO_ROLE_USER, OnEditchangeComboRoleUser)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_DELETE, OnCheckDelete)
	ON_BN_CLICKED(IDC_CHECK_EXECUTE, OnCheckExecute)
	ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
	ON_BN_CLICKED(IDC_CHECK_UPDATE, OnCheckUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBuDefRoleUserForFacetPage message handlers

BOOL CxDlgIceBuDefRoleUserForFacetPage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	FillCombo();
	if (m_nTypeObj==OT_ICE_BUNIT_FACET_USER && 
		!((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bAlter)
	{
			//"User Access Definition for Facet %s"
			CString csFormat;
			GetWindowText(csFormat);
			m_csTitle.Format(csFormat,((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->icebusunitdoc.name);
	}
	GetDlgItem(IDC_STATIC_TYPE)->SetWindowText(m_csStaticValue);
	SetWindowText(m_csTitle);
	if (m_nTypeObj == OT_ICE_BUNIT_FACET_ROLE ||
		m_nTypeObj == OT_ICE_BUNIT_PAGE_ROLE)
	{
		m_bDelete           = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bDelete;
		m_bExecute          = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bExec;
		m_bRead             = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bRead;
		m_bUpdate           = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bUpdate;
		m_csCurrentSelected = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->icebusunitdocrole.RoleName;
		if (((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bAlter)
			m_ctrlCombo.EnableWindow(FALSE);
		else
			m_ctrlCombo.EnableWindow(TRUE);
	}
	else
	{
		m_bDelete           = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bDelete;
		m_bExecute          = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bExec;
		m_bRead             = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bRead;
		m_bUpdate           = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bUpdate;
		m_csCurrentSelected = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->user.UserName;
		if (((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bAlter)
			m_ctrlCombo.EnableWindow(FALSE);
		else
			m_ctrlCombo.EnableWindow(TRUE);
	}
	UpdateData(FALSE);
	EnableDisableOk();

	PushHelpId (m_nHelpId);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgIceBuDefRoleUserForFacetPage::OnEditchangeComboRoleUser() 
{
	EnableDisableOk();
}

void CxDlgIceBuDefRoleUserForFacetPage::OnOK() 
{
	ICEBUSUNITDOCROLEDATA IceDtaRole;
	ICEBUSUNITDOCUSERDATA IceDtaUser;
	CString csMsg;
	memset(&IceDtaRole,0,sizeof(ICEBUSUNITDOCROLEDATA));
	memset(&IceDtaUser,0,sizeof(ICEBUSUNITDOCUSERDATA));
	UpdateData(TRUE);
	switch(m_nTypeObj)
	{
	case OT_ICE_BUNIT_FACET_ROLE:
	case OT_ICE_BUNIT_PAGE_ROLE:
		{
			lstrcpy(IceDtaRole.icebusunitdoc.name,((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->icebusunitdoc.name);
			lstrcpy((char *)(IceDtaRole.icebusunitdoc.icebusunit.Name),(char *) (((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->icebusunitdoc.icebusunit.Name));

			IceDtaRole.icebusunitdoc.bFacet = ((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->icebusunitdoc.bFacet;

			IceDtaRole.bDelete = m_bDelete;
			IceDtaRole.bExec   = m_bExecute;
			IceDtaRole.bRead   = m_bRead;
			IceDtaRole.bUpdate = m_bUpdate;
			lstrcpy((char *)IceDtaRole.icebusunitdocrole.RoleName,m_csCurrentSelected);
			if (((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bAlter)
			{
				if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName,
									m_nTypeObj,
									(LPICEBUSUNITDOCROLEDATA)m_pStructObj,
									&IceDtaRole) == RES_ERR )
				{
					csMsg.LoadString(m_nMessageID);
					MessageWithHistoryButton(m_hWnd,csMsg);
					return;
				}
			}
			else
			{
				if ( ICEAddobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName ,
									m_nTypeObj,&IceDtaRole ) == RES_ERR )
				{
					csMsg.LoadString(m_nMessageID);
					MessageWithHistoryButton(m_hWnd,csMsg);
					return;
				}
			}
		}
		break;
	case OT_ICE_BUNIT_FACET_USER:
	case OT_ICE_BUNIT_PAGE_USER:
		{
			lstrcpy(IceDtaUser.icebusunitdoc.name,((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->icebusunitdoc.name);
			lstrcpy((char *)(IceDtaUser.icebusunitdoc.icebusunit.Name),(char *) (((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->icebusunitdoc.icebusunit.Name));

			IceDtaUser.icebusunitdoc.bFacet = ((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->icebusunitdoc.bFacet;
			IceDtaUser.bDelete  = m_bDelete;
			IceDtaUser.bExec    = m_bExecute;
			IceDtaUser.bRead    = m_bRead;
			IceDtaUser.bUpdate  = m_bUpdate;
			lstrcpy((char *)IceDtaUser.user.UserName,m_csCurrentSelected);
			if (((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bAlter)
			{
				if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName,
									m_nTypeObj,
									(LPICEBUSUNITDOCUSERDATA)m_pStructObj,
									&IceDtaUser) == RES_ERR )
				{
					csMsg.LoadString(m_nMessageID);
					MessageWithHistoryButton(m_hWnd,csMsg);
					return;
				}
			}
			else
			{
				if ( ICEAddobject((LPUCHAR)(LPCTSTR)m_csCurrentVnodeName ,
									m_nTypeObj,&IceDtaUser ) == RES_ERR )
				{
					csMsg.LoadString(m_nMessageID);
					MessageWithHistoryButton(m_hWnd,csMsg);
					return;
				}
			}
		}
		break;
	default :
		ASSERT(FALSE);
		return;
	}
	CDialog::OnOK();
}
void CxDlgIceBuDefRoleUserForFacetPage::FillCombo()
{
	CString csMsg,csType;
	int ires;
	UCHAR buf [MAXOBJECTNAME];
	UCHAR bufOwner[MAXOBJECTNAME];
	UCHAR extradata[EXTRADATASIZE];

	m_ctrlCombo.ResetContent();

	if (m_nTypeObj == OT_ICE_BUNIT_FACET_ROLE ||
		m_nTypeObj == OT_ICE_BUNIT_PAGE_ROLE)
	{
		ires = DOMGetFirstObject (	m_nHnode,
									OT_ICE_ROLE, 0, NULL,
									FALSE, NULL, buf, bufOwner, extradata);
		while (ires == RES_SUCCESS)
		{
			m_ctrlCombo.AddString ((char *)buf);
			ires  = DOMGetNextObject (buf, NULL, extradata);
		}

		if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
		{
			csType.LoadString(IDS_E_ICE_FILL_COMBO_ROLE);
			MessageWithHistoryButton(m_hWnd,csType);
		}
		if (!((LPICEBUSUNITDOCROLEDATA)m_pStructObj)->bAlter)
			m_ctrlCombo.SetCurSel(0);
	}
	else
	{
		ires = DOMGetFirstObject (	m_nHnode,
									OT_ICE_WEBUSER, 0, NULL,
									FALSE, NULL, buf, bufOwner, extradata);
		while (ires == RES_SUCCESS)
		{
			m_ctrlCombo.AddString ((char *)buf);
			ires  = DOMGetNextObject (buf, NULL, extradata);
		}

		if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
		{
			csType.LoadString(IDS_E_ICE_FILL_COMBO_WEB_USER);
			MessageWithHistoryButton(m_hWnd,csType);
		}
		if (!((LPICEBUSUNITDOCUSERDATA)m_pStructObj)->bAlter)
			m_ctrlCombo.SetCurSel(0);
	}
}

void CxDlgIceBuDefRoleUserForFacetPage::EnableDisableOk()
{
	UpdateData(TRUE);
	if ( ( m_bDelete || m_bExecute || m_bRead ||m_bUpdate) &&
		 m_ctrlCombo.GetCurSel() != CB_ERR)
		m_ctrlOK.EnableWindow(TRUE);
	else
		m_ctrlOK.EnableWindow(FALSE);
}

void CxDlgIceBuDefRoleUserForFacetPage::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgIceBuDefRoleUserForFacetPage::OnCheckDelete() 
{
	EnableDisableOk();
}

void CxDlgIceBuDefRoleUserForFacetPage::OnCheckExecute() 
{
	EnableDisableOk();
}

void CxDlgIceBuDefRoleUserForFacetPage::OnCheckRead() 
{
	EnableDisableOk();
}

void CxDlgIceBuDefRoleUserForFacetPage::OnCheckUpdate() 
{
	EnableDisableOk();
}
