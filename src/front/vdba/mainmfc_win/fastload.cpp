/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  fastload.cpp : implementation file
**
**  History:
**	26-Jul-2001 (schph01)
**	    BUG #105343 remove in the vnode name, the user name and/or the
**	    server type before comparing this one with "(local)"
**	24-Jun-2002 (hanje04)
**	    Cast all CStrings being passed to other functions or methods
**	    using %s with (LPCTSTR) and made calls more readable on UNIX.
**	27-aug-2002 (somsa01)
**	    Corrected last cross-integration.
*/
#include "stdafx.h"
#include "mainmfc.h"
#include "msghandl.h"
#include "fastload.h"
extern "C" {
#include "dbadlg1.h"
#include "dbaginfo.h"
#include "domdata.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgFastLoad dialog


CxDlgFastLoad::CxDlgFastLoad(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgFastLoad::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgFastLoad)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CxDlgFastLoad::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgFastLoad)
	DDX_Control(pDX, IDOK, m_crtlOKButton);
	DDX_Control(pDX, IDC_EDIT_FILE_NAME, m_ctrledFileName);
	DDX_Control(pDX, IDC_BROWSE_FILE, m_ctrlbtBrowseFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgFastLoad, CDialog)
	//{{AFX_MSG_MAP(CxDlgFastLoad)
	ON_BN_CLICKED(IDC_BROWSE_FILE, OnBrowseFile)
	ON_EN_CHANGE(IDC_EDIT_FILE_NAME, OnChangeEditFileName)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgFastLoad message handlers

void CxDlgFastLoad::OnBrowseFile() 
{
	BOOL bRet;
	CString csFileName;

	m_ctrledFileName.GetWindowText(csFileName);

	CString csFilter = "Data Files (*.out)|*.out|All Files (*.*)|*.*||";
	CFileDialog dlgFile(TRUE,
						"*.out",
						csFileName,
						OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
						(LPCTSTR)csFilter);

	bRet = dlgFile.DoModal();
	if (bRet)
		m_ctrledFileName.SetWindowText(dlgFile.GetPathName());
}

BOOL CxDlgFastLoad::OnInitDialog() 
{
	LPUCHAR parent [MAXPLEVEL];
	TCHAR buffer   [MAXOBJECTNAME];
	TCHAR szVnodeWithoutUser[MAXOBJECTNAME*4];
	CString csTitle,csCaption;
	CString csLocal;
	CDialog::OnInitDialog();

	if (!csLocal.LoadString(IDS_I_LOCALNODE))
		m_ctrlbtBrowseFile.EnableWindow(FALSE);

	//keep only the Vnode Name
	_tcscpy(szVnodeWithoutUser,(LPTSTR)(LPCTSTR)m_csNodeName);
	RemoveConnectUserFromString((LPUCHAR)szVnodeWithoutUser);
	RemoveGWNameFromString((LPUCHAR)szVnodeWithoutUser);

	if (csLocal.CompareNoCase(szVnodeWithoutUser) == 0)
		m_ctrlbtBrowseFile.EnableWindow(TRUE);
	else
		m_ctrlbtBrowseFile.EnableWindow(FALSE);

	GetWindowText(csTitle);
	//
	// Make up title:
	parent [0] = (LPUCHAR)(LPTSTR)(LPCTSTR)m_csDatabaseName;
	parent [1] = NULL;
	GetExactDisplayString (m_csTableName, m_csOwnerTbl, OT_TABLE, parent, buffer);

	csCaption.Format("%s %s::%s %s",(LPCTSTR)csTitle,(LPCTSTR)m_csNodeName,(LPCTSTR)m_csDatabaseName,buffer);
	SetWindowText(csCaption);
	EnableDisableOKbutton();
	PushHelpId (IDD_FASTLOAD);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgFastLoad::OnOK() 
{
	int hnode;
	CString csCommandLine,csFileName,csTitle,csUserNameConnect;

	m_ctrledFileName.GetWindowText(csFileName);
	if (!csFileName.IsEmpty())
	{
		UCHAR szusernamebuf[MAXOBJECTNAME];
		DBAGetUserName ((LPUCHAR)(LPCTSTR)m_csNodeName,szusernamebuf);
		hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)m_csNodeName);
		if (hnode<0)
		{
			CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
			strMsg += VDBA_MfcResourceString (IDS_E_LAUNCH_FASTLOAD);    //" - Cannot Launch FastLoad.");
			AfxMessageBox (strMsg);
			return;
		}
		// Temporary for activate a session
		UCHAR buf[MAXOBJECTNAME];
		DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

		csTitle.Format("Fastload Table");
		csCommandLine.Format("fastload -u%s %s -file=%s -table=%s",
		szusernamebuf,(LPCTSTR)m_csDatabaseName, 
		(LPCTSTR)csFileName, (LPCTSTR)m_csTableName);
		execrmcmd1(	(char *)(LPCTSTR)m_csNodeName,
					(char *)(LPCTSTR)csCommandLine,
					(char *)(LPCTSTR)csTitle,
					TRUE);
		CloseNodeStruct(hnode,FALSE);
	}
	CDialog::OnOK();
}


void CxDlgFastLoad::OnChangeEditFileName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	EnableDisableOKbutton();
	
}

void CxDlgFastLoad::EnableDisableOKbutton()
{
	CString csFileName;
	m_ctrledFileName.GetWindowText(csFileName);
	if (!csFileName.IsEmpty())
		m_crtlOKButton.EnableWindow(TRUE);
	else
		m_crtlOKButton.EnableWindow(FALSE);
}

void CxDlgFastLoad::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
