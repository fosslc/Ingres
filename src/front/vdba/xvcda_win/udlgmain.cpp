/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : udlgmain.cpp : implementation file
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main dialog for vcda
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 30-Dec-2005 (komve01)
**    BUG #113689 / ISSUE 13858745,
**    VCDA snapshot1 snapshot2 must compare the two and show the results.
**    It was failing since, command line arguments were not picked properly
**	  from the argument list.
**/

#include "stdafx.h"
#include "vcda.h"
#include "mainfrm.h"
#include "udlgmain.h"
#include "rctools.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CuVcdaControl, CuVcda)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

void CuVcdaControl::HideFrame(short nShow)
{
	CuDlgMain* pParent = (CuDlgMain*)GetParent();
	if (pParent && IsWindow(pParent->m_hWnd))
	{
		pParent->m_bCompared = FALSE;
	}
	CuVcda::HideFrame(nShow);
}

BOOL CuVcdaControl::OnHelpInfo(HELPINFO* pHelpInfo)
{
	CuDlgMain* pParent = (CuDlgMain*)GetParent();

	if (pParent && IsWindow(pParent->m_hWnd))
	{
		if (pParent->m_bCompared)
		{
			CuVcda::OnHelpInfo(pHelpInfo);
			return FALSE;
		}
	}
	APP_HelpInfo(m_hWnd, 0);
	return FALSE;
}


static CString GetFile4Save() 
{
	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Visual Configuration Differences Files (*.ii_vcda)|*.ii_vcda||"));

	if (dlg.DoModal() != IDOK)
		return _T(""); 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".ii_vcda");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("ii_vcda");
		else
		if (strExt.CompareNoCase (_T("ii_vcda")) != 0)
			strFullName += _T(".ii_vcda");
	}

	return strFullName;
}

static CString GetSnapshotFile() 
{
	CString strFullName;
	CFileDialog dlg(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Visual Configuration Differences Files (*.ii_vcda)|*.ii_vcda||"));

	if (dlg.DoModal() != IDOK)
		return _T(""); 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_ENVIRONMENT_FILE);
		return _T("");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (strExt.CompareNoCase (_T("ii_vcda")) != 0)
		{
			AfxMessageBox (IDS_MSG_UNKNOWN_ENVIRONMENT_FILE);
			return _T("");
		}
	}
	if (strFullName.IsEmpty())
		return _T("");
	if (_taccess(strFullName, 0) == -1)
		return _T("");

	return strFullName;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain dialog


CuDlgMain::CuDlgMain(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgMain::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgMain)
	//}}AFX_DATA_INIT
	m_bCompared = FALSE;
}


void CuDlgMain::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgMain)
	DDX_Control(pDX, IDC_COMBO1, m_cComboFile1);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonSaveCurrent);
	DDX_Control(pDX, IDC_STATIC_DIFFERENCE, m_cCaptionDifference);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonFile1);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonFile2);
	DDX_Control(pDX, IDC_EDIT2, m_cEditFile2);
	DDX_Control(pDX, IDC_VCDACTRL1, m_cVcda);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgMain, CDialog)
	//{{AFX_MSG_MAP(CuDlgMain)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSaveCurrentInstallation)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonFile1)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonFile2)
	ON_BN_CLICKED(IDC_BUTTON4, OnButtonCompare)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboFile1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain message handlers

BOOL CuDlgMain::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_hIconOpen = theApp.LoadIcon (IDI_OPEN);
	m_hIconSave = theApp.LoadIcon (IDI_SAVE);
	m_cButtonSaveCurrent.SetIcon(m_hIconSave);
	m_cButtonFile1.SetIcon(m_hIconOpen);
	m_cButtonFile2.SetIcon(m_hIconOpen);
	m_cComboFile1.SetCurSel(0);

	BOOL bCurrent = FALSE;
	CString strFile1 = _T("");
	CString strFile2 = _T("");
	CTypedPtrList <CObList, CaKeyValueArg*>& lValue = theApp.m_cmdArg.GetKeys();
	int nCount = 0;
	POSITION pos = lValue.GetHeadPosition();
	while (pos != NULL)
	{
		CaKeyValueArg* pObj = lValue.GetNext(pos);
		if (pObj->GetKey().CompareNoCase(_T("-c")) == 0)
		{
			if (pObj->GetValue().CompareNoCase(_T("TRUE")) == 0)
				bCurrent = TRUE;
		}
		else
		if (pObj->GetKey().CompareNoCase(_T("-file")) == 0)
		{
			if (!pObj->GetValue().IsEmpty())
			{
				if (nCount == 1)
					strFile1 = pObj->GetValue();
				else
					strFile2 = pObj->GetValue();
			}
		}
		if (nCount >= 2) // Only 2 arguments
			break;
		nCount++;	//increment the counter at the end
					//vcda adds -c flag by default and makes its value
					//TRUE or FALSE, depending on if it is specified or not.
	}

	if (!strFile1.IsEmpty() || !strFile2.IsEmpty())
	{
		if (bCurrent)
		{
			CString strFile = strFile1.IsEmpty()? strFile2: strFile1;
			m_cEditFile2.SetWindowText(strFile);
		}
		else
		if (!strFile1.IsEmpty() && !strFile2.IsEmpty())
		{
			int nCur = m_cComboFile1.AddString(strFile1);
			if (nCur != CB_ERR)
			{
				m_cComboFile1.SetCurSel(nCur);
				m_cComboFile1.SetItemData(nCur, (DWORD)1);
			}
			m_cEditFile2.SetWindowText(strFile2);
		}
		DoCompare();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgMain::OnDestroy() 
{
	if (m_hIconOpen)
		DestroyIcon(m_hIconOpen);
	if (m_hIconSave)
		DestroyIcon(m_hIconSave);
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CuDlgMain::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgMain::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_cVcda.m_hWnd))
	{
		CRect rDlg, r;
		GetClientRect (rDlg);
		m_cVcda.GetWindowRect (r);
		ScreenToClient(r);
		r.right = rDlg.right - r.left;
		r.bottom = rDlg.bottom - r.left;
		m_cVcda.MoveWindow (r);

		m_cCaptionDifference.GetWindowRect (r);
		ScreenToClient(r);
		r.right = rDlg.right - r.left;
		r.bottom = rDlg.bottom - r.left;
		m_cCaptionDifference.MoveWindow (r);
	}
}

void CuDlgMain::OnButtonSaveCurrentInstallation() 
{
	CString strFile = GetFile4Save();
	if (strFile.IsEmpty())
		return;
	CWaitCursor doWaitCursor;
	m_cVcda.SaveInstallation(strFile);
}

void CuDlgMain::OnButtonFile1() 
{
	CString strFile = GetSnapshotFile();
	if (strFile.IsEmpty())
		return;
	int nCur = m_cComboFile1.FindStringExact(-1, strFile);
	if (nCur == CB_ERR)
	{
		nCur = m_cComboFile1.AddString(strFile);
		if (nCur != CB_ERR)
		{
			m_cComboFile1.SetCurSel(nCur);
			m_cComboFile1.SetItemData(nCur, (DWORD)1);
		}
	}
	else
	{
		m_cComboFile1.SetCurSel(nCur);
	}

	CRect r;
	SIZE size;
	HDC hDC = ::GetDC(m_cComboFile1.m_hWnd);
	m_cComboFile1.GetClientRect(r);
	int nWidth = r.Width();
	int i, nCount = m_cComboFile1.GetCount();
	for (i=0; i<nCount; i++)
	{
		m_cComboFile1.GetLBText(i, strFile);
		if (GetTextExtentPoint32 (hDC, strFile, strFile.GetLength(), &size))
		{
			if (nWidth < size.cx)
				nWidth = size.cx;
		}
	}
	nWidth = (int)((double)nWidth*0.85);
	::ReleaseDC(m_cComboFile1.m_hWnd, hDC);
	m_cComboFile1.SetDroppedWidth(nWidth);
	m_cVcda.HideFrame(0);
}

void CuDlgMain::OnButtonFile2() 
{
	CString strFile = GetSnapshotFile();
	if (strFile.IsEmpty())
		return;
	m_cEditFile2.SetWindowText(strFile);
	m_cVcda.HideFrame(0);
}


void CuDlgMain::DoCompare()
{
	CString strMsg;
	CString strFile1, strFile2;
	try
	{
		int nSel = m_cComboFile1.GetCurSel();
		if (nSel == CB_ERR)
			throw (int)0;
		int nData = (int)m_cComboFile1.GetItemData(nSel);

		if (nData == 0)
		{
			//
			// Compare current Installation with a snap-shot file:
			m_cEditFile2.GetWindowText(strFile2);
			strFile2.TrimLeft();
			strFile2.TrimRight();
			if (!strFile2.IsEmpty())
			{
				if (_taccess(strFile2, 0) == -1)
					throw (int)2;
				m_cVcda.SetCompareFile(strFile2);
				m_cVcda.Compare();
			}
			else
			{
				throw (int)0;
			}
		}
		else
		{
			//
			// Comparetwo snap-shot files:
			m_cComboFile1.GetLBText(nSel, strFile1);
			m_cEditFile2.GetWindowText(strFile2);
			strFile1.TrimLeft();
			strFile1.TrimRight();
			strFile2.TrimLeft();
			strFile2.TrimRight();
			if (!strFile1.IsEmpty() && !strFile2.IsEmpty())
			{
				if (_taccess(strFile1, 0) == -1)
					throw (int)1;
				if (_taccess(strFile2, 0) == -1)
					throw (int)2;
				m_cVcda.SetCompareFiles(strFile1, strFile2);
				m_cVcda.Compare();
			}
			else
			{
				throw (int)0;
			}
		}
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 0: // must specify file name
			AfxMessageBox(IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 1: // File1 does not exist
			AfxFormatString1(strMsg, IDS_MSG_NOSNAPSHOT, (LPCTSTR)strFile1);
			AfxMessageBox(strMsg);
			break;
		case 2: // File2 does not exist
			AfxFormatString1(strMsg, IDS_MSG_NOSNAPSHOT, (LPCTSTR)strFile2);
			AfxMessageBox(strMsg);
			break;
		default:
			break;
		}
	}
}

void CuDlgMain::OnButtonCompare() 
{
	CWaitCursor doWaitCursor;
	DoCompare();
	m_bCompared = TRUE;
}

void CuDlgMain::OnSelchangeComboFile1() 
{
	m_cVcda.HideFrame(0);
}

void CuDlgMain::DoSaveInstallation()
{
	OnButtonSaveCurrentInstallation();
}

