/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppagpat2.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Preliminary Step - Uninstall patches
**
** History:
**
** 04-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 08-Sep-2008 (drivi01)
**    Updated wizard to use Wizard97 style and updated smaller
**    wizard graphic.
*/

#include "stdafx.h"
#include "vcda.h"
#include "pshresto.h"
#include "ppagpat2.h"
#include "rctools.h"  // Resource symbols of rctools.rc
#include "vcdadml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestorePatch2  property page
IMPLEMENT_DYNCREATE(CuPropertyPageRestorePatch2, CPropertyPage)

void CuPropertyPageRestorePatch2::FillPatchListBox (CListBox** pListBox, CString** PatchArr, int nDim)
{
	for (int i=0; i<nDim; i++)
	{
		CString& strPatch = *(PatchArr[i]);
		CString strItem = strPatch;
		if (!strItem.IsEmpty())
		{
			int nPos = strPatch.Find(_T(','));
			if (nPos == -1)
			{
				pListBox[i]->AddString(strItem);
			}
			else
			{
				while (nPos != -1)
				{
					strItem = strPatch.Left(nPos);
					strPatch = strPatch.Mid(nPos+1);
					strPatch.TrimLeft();
					pListBox[i]->AddString(strItem);

					nPos = strPatch.Find(_T(','));
				}
				if (!strPatch.IsEmpty())
					pListBox[i]->AddString(strPatch);
			}
		}
	}
}


CuPropertyPageRestorePatch2::CuPropertyPageRestorePatch2() : CPropertyPage(CuPropertyPageRestorePatch2::IDD)
{
	//{{AFX_DATA_INIT(CuPropertyPageRestorePatch2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetCenterVertical(TRUE);
	m_strPatch1 = _T("");
	m_strPatch2 = _T("");
	m_strVersion = _T("");
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPropertyPageRestorePatch2::~CuPropertyPageRestorePatch2()
{
}



void CuPropertyPageRestorePatch2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPropertyPageRestorePatch2)
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonAccessPatch);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonUninstallPatch);
	DDX_Control(pDX, IDC_LIST2, m_cListPatch2);
	DDX_Control(pDX, IDC_LIST1, m_cListPatch1);
	DDX_Control(pDX, IDC_STATIC_INFO1, m_cInfoVersion);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonRefresh);
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPropertyPageRestorePatch2, CPropertyPage)
	//{{AFX_MSG_MAP(CuPropertyPageRestorePatch2)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonUninstallPatch)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonRefreshInstalledPatch)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonAccessPatch)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeList2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestorePatch2 message handlers

BOOL CuPropertyPageRestorePatch2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	m_hIconRefresh = theApp.LoadIcon (IDI_REFRESH);
	m_cButtonRefresh.SetIcon(m_hIconRefresh);
	CString strTemp, strMsg;
	m_cInfoVersion.GetWindowText(strTemp);
	strMsg.Format(strTemp, (LPCTSTR)m_strVersion);
	m_cInfoVersion.SetWindowText(strMsg);

	CListBox* pListBox [2] = {&m_cListPatch1, &m_cListPatch2};
	CString*  PatchArr [2] = {&m_strPatch1, &m_strPatch2};
	FillPatchListBox (pListBox, PatchArr, 2);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuPropertyPageRestorePatch2::OnDestroy() 
{
	if (m_hIconRefresh)
		DestroyIcon(m_hIconRefresh);

	CPropertyPage::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CuPropertyPageRestorePatch2::OnButtonUninstallPatch() 
{
	CString strMsg;
	CString strPatch, strLaterPatch = _T(" ");
	CString strCurItem = _T("");
	int i, nSel = m_cListPatch1.GetCurSel();
	if (nSel != -1)
	{
		m_cListPatch1.GetText(nSel, strPatch);
		BOOL bOne = TRUE;
		CString strItem;
		int nCount = m_cListPatch1.GetCount();
		for (i=nSel+1; i<nCount; i++)
		{
			m_cListPatch1.GetText(i, strItem);
			if (!bOne)
				strLaterPatch += _T(", ");
			strLaterPatch += strItem;
			bOne = FALSE;
		}
		strLaterPatch.TrimRight();
		if (strLaterPatch.IsEmpty())
			strLaterPatch = _T(".");
		else
			strLaterPatch+= _T(".");
	}

	AfxFormatString2 (strMsg, IDS_MSG_UNINSTALL_PATCH, (LPCTSTR)strPatch, (LPCTSTR)strLaterPatch);
	int nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
	if (nAnswer == IDYES)
	{
		CString strFile;
		TCHAR* pEnv = _tgetenv(_T("II_SYSTEM"));
		#if defined (MAINWIN)
		CString strPatchInfo = _T("%s/ingres/install/backup/pre-%s/ingres/patch.txt");
		#else
		CString strPatchInfo = _T("%s\\ingres\\install\\backup\\pre-%s\\ingres\\patch.txt");
		#endif
		strFile.Format(strPatchInfo, pEnv, (LPCTSTR)strPatch);
		SHELLEXECUTEINFO shell;
		memset (&shell, 0, sizeof(shell));
		shell.cbSize = sizeof(shell);
		shell.fMask  = 0;
		shell.lpFile = strFile;
		shell.nShow  = SW_SHOWNORMAL;
		shell.hwnd = m_hWnd;
		ShellExecuteEx(&shell);
	}
}

void CuPropertyPageRestorePatch2::OnButtonRefreshInstalledPatch() 
{
	CString strCurItem = _T("");
	int nSel = m_cListPatch1.GetCurSel();
	if (nSel != -1)
	{
		m_cListPatch1.GetText(nSel, strCurItem);
		//
		// Refresh the list:
		m_cListPatch1.ResetContent();
		CString strIngresVersion, strIngresPatches;
		VCDA_GetIngresVersionIDxPatches (strIngresVersion, strIngresPatches);
		CListBox* pListBox [1] = {&m_cListPatch1};
		CString*  PatchArr [1] = {&strIngresPatches};
		FillPatchListBox (pListBox, PatchArr, 1);
	}

	if (!strCurItem.IsEmpty())
	{
		nSel = m_cListPatch1.FindStringExact(-1, strCurItem);
		m_cListPatch1.SetCurSel(nSel);
	}
	OnSelchangeList1();
}

void CuPropertyPageRestorePatch2::GeneratePatchURL (LPCTSTR lpszPatchNum, CString& strURL)
{
	//
	// I suppose that the version always has the form:
	// <string1><splace><string2>/<string3><splace>(<string4>/<string5>)
	// EX:
	//   string1 = II
	//   string2 = 2.6
	//   string3 = 0207
	//   string4 = int.w32
	//   string5 = 00
	//   The version is: II 2.6/0207 (int.w32/00)
	//
	// The URL will be generated as following:
	//  -replace the <.> by <_> in the string4, let call string4bis
	//  -ftp://ftp.ca.com/pub/ingres/<string4bis>/Ingres_II/<string2>_<string3>_<string5>/p<lpszPatchNum>/
	//   For the above values, and the patch number is 6830, then the URL is:
	//
	//   ftp://ftp.ca.com/pub/ingres/int_w32/Ingres_II/2.6_0207_00/p6830/
	strURL = _T("ftp://ftp.ca.com/pub/ingres/");
	CString s1, s2, s3, s4, s5;
	CString strVersion = m_strVersion;
	int nPos = strVersion.Find(_T(' '));
	if (nPos != -1)
	{
		s1 = strVersion.Left(nPos);
		strVersion = strVersion.Mid(nPos+1);

		nPos = strVersion.Find(_T('/'));
		if (nPos != -1)
		{
			s2 = strVersion.Left(nPos);
			strVersion = strVersion.Mid(nPos+1);

			nPos = strVersion.Find(_T('('));
			if (nPos != -1)
			{
				s3 = strVersion.Left(nPos);
				s3.TrimRight();
				strVersion = strVersion.Mid(nPos+1);

				nPos = strVersion.Find(_T('/'));
				if (nPos != -1)
				{
					s4 = strVersion.Left(nPos);
					s4.Replace(_T('.'), _T('_'));
					strVersion = strVersion.Mid(nPos+1);

					nPos = strVersion.Find(_T(')'));
					if (nPos != -1)
					{
						s5 = strVersion.Left(nPos);
						strURL += s4 + _T("/Ingres_II/");
						strURL += s2 + _T('_');
						strURL += s3 + _T('_');
						strURL += s5 + _T("/p");
						strURL += lpszPatchNum;
						strURL +=  _T('/');
					}
				}
			}
		}
	}
}


void CuPropertyPageRestorePatch2::OnButtonAccessPatch() 
{
	CString strURL = _T("ftp://ftp.ca.com/pub/ingres/");
	CString strPatch;
	int nSel = m_cListPatch2.GetCurSel();
	if (nSel != -1)
	{
		m_cListPatch2.GetText(nSel, strPatch);
		GeneratePatchURL (strPatch, strURL);
	}

	SHELLEXECUTEINFO shell;
	memset (&shell, 0, sizeof(shell));
	shell.cbSize = sizeof(shell);
	shell.fMask  = 0;
	shell.lpFile = strURL;
	shell.nShow  = SW_SHOWNORMAL;
	CWnd* pMainWnd = AfxGetMainWnd();
	if (pMainWnd)
		shell.hwnd = m_hWnd;
	ShellExecuteEx(&shell);
}

void CuPropertyPageRestorePatch2::OnSelchangeList1() 
{
	int nSel = m_cListPatch1.GetCurSel();
	if (nSel == -1)
		m_cButtonUninstallPatch.EnableWindow(FALSE);
	else
		m_cButtonUninstallPatch.EnableWindow(TRUE);
}

void CuPropertyPageRestorePatch2::OnSelchangeList2() 
{
	int nSel = m_cListPatch2.GetCurSel();
	if (nSel == -1)
		m_cButtonAccessPatch.EnableWindow(FALSE);
	else
		m_cButtonAccessPatch.EnableWindow(TRUE);
}

BOOL CuPropertyPageRestorePatch2::OnSetActive() 
{
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	UINT nMode = PSWIZB_NEXT;

	pParent->SetWizardButtons(nMode);
	return CPropertyPage::OnSetActive();
}
