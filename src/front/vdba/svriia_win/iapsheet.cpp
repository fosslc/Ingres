/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iapsheet.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuIIAPropertySheet.
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 19-Sep-2001 (uk$so01)
**    BUG #105759 (Win98 only). Exit iia in the Step 2, the Disconnect
**    session did not return.
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/

#include "stdafx.h"
#include "resource.h"
#include "iapsheet.h"
#include "afxpriv.h"
#include "rctools.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL GenerateHeader(CaDataPage1& dataPage1, CaSeparatorSet* pSet, CaSeparator* pSeparator, int nColumnCount, CStringArray& arrayHeader)
{
	BOOL bFirstLineAsHeader = FALSE;
	arrayHeader.RemoveAll();
	if (dataPage1.GetImportedFileType() != IMPORT_CSV)
		return bFirstLineAsHeader;

	if (dataPage1.GetFirstLineAsHeader() && dataPage1.GetLineCountToSkip() == 1)
	{
		CString strHeaderLine = _T("");
		CSV_GetHeaderLines(dataPage1.GetFile2BeImported(), dataPage1.GetCodePage(), strHeaderLine);
		CStringArray array;
		GetFieldsFromLine (strHeaderLine, array, pSet);
		if (array.GetSize() != nColumnCount)
			return bFirstLineAsHeader;
		int i, nSize = array.GetSize();
		arrayHeader.SetSize(nSize);

		for (i=0; i<nSize; i++)
		{
			CString s = array.GetAt(i);
			s.TrimLeft();
			s.TrimRight();
			arrayHeader.SetAt(i, s);
		}

		bFirstLineAsHeader = TRUE;
	}

	return bFirstLineAsHeader;
}



IMPLEMENT_DYNAMIC(CuIIAPropertySheet, CPropertySheet)
CuIIAPropertySheet::CuIIAPropertySheet(CWnd* pWndParent)
	 : CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().
	m_psh.dwFlags |= PSH_HASHELP|PSH_PROPSHEETPAGE;
	m_psh.nPages = 4;

	AddPage(&m_Page1);
	AddPage(&m_Page2);
	AddPage(&m_Page3);
	AddPage(&m_Page4);

	SetPreviousPage (0);
	SetFileFormatUpdated(TRUE);
	SetWizardMode();

	m_pStruct = NULL;

	
	m_psh.dwFlags |= PSH_WIZARD97|PSH_USEHBMHEADER|PSH_WATERMARK ;

	m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_IMPORTAS_1);
	m_psh.hbmHeader = NULL;

	m_psh.hInstance = AfxGetInstanceHandle(); 
}

CuIIAPropertySheet::~CuIIAPropertySheet()
{
}

BOOL CuIIAPropertySheet::OnInitDialog() 
{
	BOOL bOk = CPropertySheet::OnInitDialog();

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	ASSERT (pHelpButton);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);

	return bOk;
}

void CuIIAPropertySheet::OnHelp()
{
	CTabCtrl* pTab = GetTabControl();
	ASSERT(pTab);
	if (pTab)
	{
		int nActivePage = pTab->GetCurSel();
		TRACE1("CuIIAPropertySheet::OnHelp() Page=%d\n", nActivePage + 1);
		APP_HelpInfo(m_hWnd, IDHELP_BASE + nActivePage +1);
	}
}

int CuIIAPropertySheet::DoModal()
{
	CaUserData4GetRow* pUserData = theApp.GetUserData4GetRow();
	try
	{
		pUserData->SetIIAData(&m_data);
		int nRes = CPropertySheet::DoModal();
		pUserData->SetIIAData(NULL);
		return nRes;
	}
	catch(...)
	{
		pUserData->SetIIAData(NULL);
		return IDCANCEL;
	}
}



BEGIN_MESSAGE_MAP(CuIIAPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CuIIAPropertySheet)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuIIAPropertySheet message handlers


void CuIIAPropertySheet::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertySheet::OnShowWindow(bShow, nStatus);
	CRect r;
	int nWidth  = GetSystemMetrics(SM_CXSCREEN);
	int nHeigth = GetSystemMetrics(SM_CYSCREEN);
	nWidth = nWidth/2;
	nHeigth= nHeigth/2;
	GetWindowRect (r);
	nWidth = nWidth - r.Width()/2;
	nHeigth= nHeigth -r.Height()/2;
	if (nWidth < 0)
		nWidth = 0;
	if (nHeigth < 0)
		nHeigth = 0;

	SetWindowPos (&wndTop, nWidth, nHeigth, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
}


void CALLBACK DisplayListCtrlHugeItem(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo)
{
	CaRecord* pRecord = (CaRecord*)pItemData;
	CStringArray& arrayFields = pRecord->GetArrayFields();
	int i, nSize = arrayFields.GetSize();
	int nIdx = nItem;
	int nCount = pListCtrl->GetItemCount();
	ASSERT (nItem >= 0 && nItem < pListCtrl->GetCountPerPage());
	CaIngresHeaderRowItemData* pUserInfo = (CaIngresHeaderRowItemData*)pInfo;
	CHeaderCtrl* pHeaderCtrl = pListCtrl->GetHeaderCtrl();
	ASSERT (pHeaderCtrl);
	if (!pHeaderCtrl)
		return;

	if (pUserInfo)
	{
		//
		// Because UserInfo->m_arrayColumnOrder has a dummy column at position 0.
		ASSERT (pUserInfo->m_arrayColumnOrder.GetSize() == (nSize+1));
	}

	int nSetHeader = 0;
	int nHeaderCount = pHeaderCtrl->GetItemCount();

	for (i=0; i<nSize && nSetHeader<nHeaderCount; i++)
	{
		if (pUserInfo && !pUserInfo->m_arrayColumnOrder.GetAt(i+1)) // i+1, means ignore the 0 index.
		{
			//
			// Skip this field !
			continue;
		}

		CString strField = arrayFields.GetAt(i);
		if (nSetHeader==0)
		{
			if (!bUpdate || nIdx >= nCount)
				nIdx = pListCtrl->InsertItem (nItem, strField);
			else
			{
				pListCtrl->SetItemText (nItem, nSetHeader, strField);
			}
			nSetHeader++;
		}
		else
		if (nIdx != -1)
		{
			pListCtrl->SetItemText (nIdx, nSetHeader, strField);
			nSetHeader++;
		}
	}
}



LRESULT CuIIAPropertySheet::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
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

void CuIIAPropertySheet::OnDestroy() 
{
	theApp.m_sessionManager.Cleanup();
	CPropertySheet::OnDestroy();
}
