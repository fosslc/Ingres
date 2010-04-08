/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : pshresto.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: PropertySheet Dialog box
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Now the help context IDs are available.
** 03-Oct-2004 (schph01)
**    BUG #113371, search the parenthesis to determine the begining of the
**    platform type and the end of the version number, instead of the fix value.
**	10-May-2005 (lakvi01/komve01)
**	    When comparing for platform (during restore), do not compare build 
**	    number, just compare "(platform-id".
** 08-Sep-2008 (drivi01)
**    Updated wizard to use Wizard97 style and updated smaller
**    wizard graphic.
**    Added resource file.
*/

#include "stdafx.h"
#include "resource.h"
#include "pshresto.h"
#include "udlgmain.h"
#include "rctools.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
//
// This is a control ID of help button (I use the SPY++ to detect it)
#define ID_W_HELP 9

/////////////////////////////////////////////////////////////////////////////
// CxPSheetRestore

IMPLEMENT_DYNAMIC(CxPSheetRestore, CPropertySheet)
CxPSheetRestore::CxPSheetRestore(CTypedPtrList< CObList, CaCdaDifference* >& listDifference, CWnd* pWndParent)
	 : CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().


	BOOL bDiffInMain = FALSE;
	BOOL bPlatformIdentical = TRUE;
	BOOL bVersionIdentical = TRUE;
	BOOL bPatchIdentical = TRUE;
	POSITION pos = listDifference.GetHeadPosition();
	while (pos != NULL)
	{
		CaCdaDifference* pDiff = listDifference.GetNext(pos);
		if (pDiff->GetType() != CDA_GENERAL)
			break;
		if (pDiff->GetName().CompareNoCase(_T("VERSION")) == 0)
		{
			int iPos1,iPos2;
			int iePos1,iePos2;
			CString strMsg = _T("");
			CString strV1,strV2;
			iPos1 = pDiff->GetValue1().Find(_T('('));
			iPos2 = pDiff->GetValue2().Find(_T('('));
			iePos1 = pDiff->GetValue1().Find(_T('/'));
			iePos2 = pDiff->GetValue2().Find(_T('/'));
			if (iPos1!=-1 && (iePos1 > iPos1))
				strV1 = pDiff->GetValue1().Mid(iPos1,(iePos1-iPos1));
			else
				strV1 = pDiff->GetValue1();

			if (iPos2!=-1 && (iePos2 > iPos2))
				strV2 = pDiff->GetValue2().Mid(iPos2,(iePos2-iPos2));
			else
				strV2 = pDiff->GetValue2();

			if (strV1.CompareNoCase(strV2) != 0)
			{
				bPlatformIdentical = FALSE;
				AfxFormatString2(strMsg, IDS_MSG_DIFF_PLATFORM, (LPCTSTR)strV1, (LPCTSTR)strV2);
				m_PagePatch1.SetMessage(strMsg);
				bDiffInMain = TRUE;
				break; // When the platform is different, we don't check anything else !
			}

			if (iPos1!=-1)
				strV1 = pDiff->GetValue1().Left(iPos1-1);
			else
				strV1 = pDiff->GetValue1();

			if (iPos2!=-1)
				strV2 = pDiff->GetValue2().Left(iPos2-1);
			else
				strV2 = pDiff->GetValue2();

			if (strV1.CompareNoCase(strV2) != 0)
			{
				bVersionIdentical = FALSE;
				AfxFormatString2(strMsg, IDS_MSG_DIFF_VERSION, (LPCTSTR)pDiff->GetValue1(), (LPCTSTR)pDiff->GetValue2());
				m_PagePatch1.SetMessage(strMsg);
				bDiffInMain = TRUE;
			}
		}
		else
		if (pDiff->GetName().CompareNoCase(_T("PATCHES")) == 0)
		{
			bPatchIdentical = FALSE;
			CString strIngresVersion, strIngresPatches;
			VCDA_GetIngresVersionIDxPatches (strIngresVersion, strIngresPatches);
			m_PagePatch2.SetPatches (pDiff->GetValue1(), pDiff->GetValue2(), strIngresVersion);
			bDiffInMain = TRUE;
		}
		else
		{
			bDiffInMain = TRUE;
		}
	}
	if (!bPlatformIdentical)
	{
		AddPage(&m_PagePatch1);
	}
	else
	if (!bVersionIdentical)
	{
		AddPage(&m_PagePatch1);
	}
	else
	if (!bPatchIdentical)
	{
		AddPage(&m_PagePatch2);
	}
	else
	{
		m_Page2.SetBackStep(FALSE);
	}
	AddPage(&m_Page2);
	if (bDiffInMain)
	{
		m_Page2.SetFinalStep(FALSE);
		AddPage(&m_Page3);
	}

	SetWizardMode();
	m_psh.dwFlags |= PSH_WIZARD97|PSH_HEADER|PSH_USEHBMHEADER|PSH_WATERMARK|PSH_HASHELP;

	m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_IMPORTAS_3);
	m_psh.hbmHeader = NULL;
	m_psh.hInstance = AfxGetInstanceHandle();
}

CxPSheetRestore::~CxPSheetRestore()
{
}


BEGIN_MESSAGE_MAP(CxPSheetRestore, CPropertySheet)
	//{{AFX_MSG_MAP(CxPSheetRestore)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CxPSheetRestore message handlers

void CxPSheetRestore::OnHelp()
{
	CTabCtrl* pTab = GetTabControl();
	ASSERT(pTab);
	if (pTab)
	{
		int nActivePage = pTab->GetCurSel();
		TRACE1("CxPSheetRestore::OnHelp() Page=%d\n", nActivePage + 1);
		CPropertyPage* p = GetPage(nActivePage);
		if (p->IsKindOf(RUNTIME_CLASS(CuPropertyPageRestoreOption)))
			APP_HelpInfo(m_hWnd, 40028);
		else
		if (p->IsKindOf(RUNTIME_CLASS(CuPropertyPageRestorePatch1)))
		{
			APP_HelpInfo(m_hWnd, 40025);
		}
		else
		if (p->IsKindOf(RUNTIME_CLASS(CuPropertyPageRestorePatch2)))
		{
			APP_HelpInfo(m_hWnd, 40027);
		}
		else
		if (p->IsKindOf(RUNTIME_CLASS(CuPropertyPageRestoreFinal)))
		{
			APP_HelpInfo(m_hWnd, 40029);
		}
	}
}

BOOL CxPSheetRestore::OnInitDialog() 
{
	BOOL bOk = CPropertySheet::OnInitDialog();

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	ASSERT (pHelpButton);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);

	return bOk;
}

LRESULT CxPSheetRestore::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	//
	// The HELP Button is always disable. This is the reason why I overwrite this
	// function.
	// Due to the documentation, if PSH_HASHELP is set for the page (in property page,
	// the member m_psp.dwFlags |= PSH_HASHELP) then the help button is enable. But it
	// does not work.

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);
	
	return CPropertySheet::WindowProc(message, wParam, lParam);
}

