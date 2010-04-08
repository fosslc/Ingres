/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage2.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 2 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 17-Oct-2001 (uk$so01)
**    SIR #103852, miss some behaviour.
** 30-Oct-2001 (hanje04)
**    BUG 105486
**    Resize was causing problem with MAINWIN. Removed for MAINWIN builds.
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 29-Nov-2001 (hanje04)
**    Bug 105486
**    Replace ifdef with ifndef for 31-Oct change.
** 17-Jan-2002 (uk$so01)
**    (bug 106844). Add BOOL m_bStopScanning to indicate if we really stop
**     scanning the file due to the limitation.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 03-Apr-2002 (uk$so01)
**    BUG #107488, Make svriia.dll enable to import empty .DBF file
**    It just create the table.
** 23-Mar-2004 (uk$so01)
**    BUG #112007, Use the CString[0] to compare the first character
**    with a TCHAR.
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/

#include "stdafx.h"
#include "resource.h"
#include "rctools.h"
#include "iappage2.h"
#include "iapsheet.h"
#include "fformat3.h" // CSV format (field separator format)
#include "wmusrmsg.h"
#include "xdlgdete.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuPPage2, CPropertyPage)
CuPPage2::CuPPage2() : CPropertyPage(CuPPage2::IDD)
{
	//{{AFX_DATA_INIT(CuPPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);
	m_pCurrentPage = NULL;
	m_nFixedWidthCount = 0;

	CString strStep;
	strStep.LoadString (IDS_IIA_STEP_2);
	m_strPageTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPPage2::~CuPPage2()
{
}

void CuPPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPPage2)
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonShowRejectedSolution);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckEnforceFixedWidth);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CuPPage2)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckEnforceFixedWidth)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonShowRejectedSolution)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CuPPage2::OnSetActive() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;

		ImportedFileType fileType = dataPage1.GetImportedFileType();
		if (fileType != IMPORT_TXT && fileType != IMPORT_CSV)
		{
			m_cCheckEnforceFixedWidth.ShowWindow(SW_HIDE);
			m_cButtonShowRejectedSolution.ShowWindow(SW_HIDE);
		}
		else
		{
			m_cCheckEnforceFixedWidth.ShowWindow(SW_SHOW);
			m_cButtonShowRejectedSolution.ShowWindow(SW_SHOW);
		}

		int nPreviousPage = pParent->GetPreviousPage ();
		//
		// Do we come from the step 1 page ?:
		if (nPreviousPage != 1)
			return CPropertyPage::OnSetActive();

		BOOL bFormatedUpdated = FALSE;
		ImportedFileType ft = dataPage1.GetImportedFileType();
		switch (ft)
		{
		case IMPORT_TXT:
		case IMPORT_CSV:
		case IMPORT_DBF:
			//
			// Is the detection of the file format has been performed
			// just before we are comming here ?
			bFormatedUpdated = pParent->GetFileFormatUpdated();
			if (bFormatedUpdated)
			{
				//
				// Construct the Tab Control:
				ConstructPagesFileFormat();
			}
			break;

		default:
			ASSERT (FALSE);
			break;
		}

		if (m_cTab1.GetItemCount() > 0)
			pParent->SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
		else
			pParent->SetWizardButtons(PSWIZB_BACK);

		//
		// Load/Save:
		Loading();

		return CPropertyPage::OnSetActive();
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuPPage2::OnSetActive): Failed to activate page"));
		return FALSE;
	}
}

void CuPPage2::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor doWaitCursor;
	DisplayPage();
	*pResult = 0;
}

void CuPPage2::DisplayPage()
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CRect r;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_PARAM;
	item.cchTextMax = 32;
	int nSel = m_cTab1.GetCurSel();

	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->ShowWindow  (SW_HIDE);
		m_pCurrentPage->SendMessage (WMUSRMSG_CLEANDATA, 0, 0);
	}
	m_pCurrentPage = NULL;

	if (m_cTab1.GetItem (nSel, &item))
		m_pCurrentPage = (CWnd*)item.lParam;
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		CaIIAInfo& data = pParent->GetData();

		m_pCurrentPage->MoveWindow (r);
		m_pCurrentPage->SendMessage (WMUSRMSG_UPDATEDATA, (WPARAM)0, (LPARAM)&data);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}

void CuPPage2::ResizeTab()
{
#ifndef MAINWIN
	if (!IsWindow (m_cTab1.m_hWnd))
		return;
	/*CRect r, rDlg;
	m_cTab1.GetWindowRect (r);
	ScreenToClient (r);
	GetClientRect (rDlg);
	r.right = rDlg.right - r.left;
	r.bottom = rDlg.bottom  - r.left;
	m_cTab1.MoveWindow (r);
	*/DisplayPage();
#endif
}



void CuPPage2::OnSize(UINT nType, int cx, int cy) 
{
	CPropertyPage::OnSize(nType, cx, cy);
	ResizeTab();
}

BOOL CuPPage2::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuPPage2::ConstructPagesFileFormat()
{
	m_nFixedWidthCount = 0;
	try
	{
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage2& dataPage2 = data.m_dataPage2;
		//
		// Which file format are we manipulating ?
		ImportedFileType fileType = dataPage1.GetImportedFileType();

		CString strTabHeader;
		CRect   r;
		TCITEM item;
		memset (&item, 0, sizeof (item));
		//
		// Destroy all Tabs if any:
		if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		{
			m_pCurrentPage->SendMessage (WMUSRMSG_CLEANDATA, 0, 0);
		}

		int i, nCount = m_cTab1.GetItemCount();
		item.mask = TCIF_PARAM;
		for (i=0; i<nCount; i++)
		{
			if (m_cTab1.GetItem (i, &item))
			{
				CWnd* pWnd = (CWnd*)item.lParam;
				if (pWnd && IsWindow (pWnd->m_hWnd))
				{
					pWnd->ShowWindow(SW_HIDE);
					pWnd->DestroyWindow();
				}
			}
		}
		while (m_cTab1.GetItem (0, &item))
			m_cTab1.DeleteItem (0);
		m_pCurrentPage=NULL;

		//
		// Construct the Tabs of File format Identification:
		item.mask       = TCIF_TEXT|TCIF_PARAM;
		item.cchTextMax = 32;

		if (fileType == IMPORT_CSV || fileType == IMPORT_TXT)
		{
			m_cCheckEnforceFixedWidth.EnableWindow (TRUE);
			m_cCheckEnforceFixedWidth.SetCheck (0);
			//
			// Csv (txt) Pages:
			CTypedPtrList < CObList, CaSeparatorSet* >& listChoice = dataPage2.GetListChoice();
			POSITION p = NULL, pos = listChoice.GetHeadPosition();

			pos = listChoice.GetHeadPosition();
			while (pos != NULL)
			{
				CaSeparatorSet* pSeparatorSet = listChoice.GetNext (pos);
				//
				// Handle Special case (each line is considered as single column):
				if (pSeparatorSet->GetListCharSeparator().IsEmpty() && pSeparatorSet->GetListStrSeparator().IsEmpty())
				{
					CreatePage (fileType, pSeparatorSet, NULL);
					continue;
				}

				CTypedPtrList< CObList, CaSeparator* >& l1 = pSeparatorSet->GetListCharSeparator();
				CTypedPtrList< CObList, CaSeparator* >& l2 = pSeparatorSet->GetListStrSeparator();

				p = l1.GetHeadPosition();
				while (p != NULL)
				{
					CaSeparator* pSeparator = l1.GetNext (p);
					CreatePage (fileType, pSeparatorSet, pSeparator);
				}

				p = l2.GetHeadPosition();
				while (p != NULL)
				{
					CaSeparator* pSeparator = l2.GetNext (p);
					CreatePage (fileType, pSeparatorSet, pSeparator);
				}
			}
			
			//
			// Fixed width Pages:
			if (fileType == IMPORT_CSV || fileType == IMPORT_TXT)
			{
				CString strMsg;
				CStringArray& arrayString = dataPage1.GetArrayTextLine();
				int nMax = dataPage1.GetMaxRecordLength();
				if (nMax > MAX_FIXEDWIDTHLEN)
				{
					CString strFormat;
					// The record length exceeds the limit of %d characters.\n
					// Fixed widths formats cannot be handled and will not be proposed.
					strFormat.LoadString (IDS_RECORD_EXCEEDS_FIXWIDTH_LIMIT);

					strMsg.Format (strFormat, MAX_FIXEDWIDTHLEN);
					AfxMessageBox (strMsg);
					m_cCheckEnforceFixedWidth.EnableWindow (FALSE);
				}
				else
				{
					int nArraySize = 0;
					TCHAR* pArrayPos = NULL;
					AutoDetectFixedWidth (&data, pArrayPos, nArraySize);
					if (pArrayPos)
					{
						CString strPos = pArrayPos;
						if (strPos.Find(_T(' ')) != -1)
						{
							//
							// The fixed width has been detected:
							if (dataPage1.GetKBToScan() > 0 && dataPage1.GetStopScanning())
							{
								CString strFormat;
								//
								// Some fixed widths formats were detected, which are ambiguous because you have asked\n
								// to pre-scan only %d K-bytes out of the %d K-bytes of the whole file.\n
								// These formats will not be proposed (you may need to define one manually)
								strFormat.LoadString(IDS_NOT_PROPOSE_AMBIGEOUS_FIXEDWIDTH);
								strMsg.Format (strFormat, dataPage1.GetKBToScan(), dataPage1.GetFileSize());
	
								AfxMessageBox (strMsg);
								delete pArrayPos;
							}
							else
							{
								CWnd* pPage = CreatePage (fileType, NULL, NULL, TRUE);
								if (pPage)
								{
									CuDlgPageFixedWidth* pWndFixedWidth = (CuDlgPageFixedWidth*)pPage;
									pWndFixedWidth->SetArrayPos(pArrayPos);
									pArrayPos = NULL;
								}
							}
						}
						else
						{
							delete pArrayPos;
						}
					}
				}
			}
		}
		else
		if (fileType == IMPORT_DBF)
		{
			CWnd* pWnd = CreatePage (fileType, NULL, NULL);
		}

		m_cTab1.SetCurSel (0);
		DisplayPage();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuPPage2::OnInitDialog() ): \nFailed to create the Tab of solutions"));
	}
}


BOOL CuPPage2::OnKillActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	pParent->SetPreviousPage (2);

	return CPropertyPage::OnKillActive();
}


CWnd* CuPPage2::CreatePage(ImportedFileType fileType, CaSeparatorSet* pSet, CaSeparator* pSeparator, BOOL bFixedWidth)
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();

	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT|TCIF_PARAM;
	item.cchTextMax = 32;
	CWnd* pWnd = NULL;
	int nTabCount = m_cTab1.GetItemCount();
	CString strTitle;
	CString strTabHeader = _T("Format %d");
	switch (fileType)
	{
	case IMPORT_DBF:
		strTabHeader = _T("dBASE Format");
		pWnd = (CWnd*)new CuDlgPageDbf(&m_cTab1);
		((CuDlgPageDbf*)pWnd)->Create (IDD_PROPPAGE_FORMAT_DBF, &m_cTab1);
		item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
		item.lParam  = (LPARAM)pWnd;
		m_cTab1.InsertItem (nTabCount, &item);
		break;
	
	case IMPORT_TXT:
	case IMPORT_CSV:
		if (!bFixedWidth)
		{
			strTabHeader.Format (_T("Csv %d"), nTabCount+1);
			pWnd = (CWnd*)new CuDlgPageCsv(&m_cTab1);
			((CuDlgPageCsv*)pWnd)->Create (IDD_PROPPAGE_FORMAT_CSV, &m_cTab1);
			((CuDlgPageCsv*)pWnd)->SetFormat (&data, pSet, pSeparator);
			item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
			item.lParam  = (LPARAM)pWnd; 
			m_cTab1.InsertItem (nTabCount, &item);
		}
		else
		{
			BOOL bReadOnly = (m_cCheckEnforceFixedWidth.GetCheck() == 0);
			m_nFixedWidthCount++;
			strTabHeader.Format (_T("Fixed Width %d"), m_nFixedWidthCount);
			pWnd = (CWnd*)new CuDlgPageFixedWidth(&m_cTab1, bReadOnly);
			((CuDlgPageFixedWidth*)pWnd)->Create (IDD_PROPPAGE_FORMAT_FIX, &m_cTab1);
			item.pszText = (LPTSTR)(LPCTSTR)strTabHeader;
			item.lParam  = (LPARAM)pWnd; 
			m_cTab1.InsertItem (nTabCount, &item);
		}
		break;

	case IMPORT_XML:
		strTabHeader = _T("XML Format %d");
		break;
	default:
		break;
	}

	return pWnd;
}

void CuPPage2::EnforceFixedWidth(BOOL bAutoDetect)
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT|TCIF_PARAM;
	item.cchTextMax = 32;
	CWnd* pWnd = NULL;
	CString strTabHeader = _T("Fixed Width");

	ImportedFileType fileType = dataPage1.GetImportedFileType();
	int nCheck = m_cCheckEnforceFixedWidth.GetCheck();
	if (nCheck == 1 && (fileType == IMPORT_CSV || fileType == IMPORT_TXT))
	{
		int nArraySize = 0;
		TCHAR* pArrayPos = NULL;
		if (bAutoDetect)
		{
			AutoDetectFixedWidth (&data, pArrayPos, nArraySize);
			if (pArrayPos)
			{
				CWnd* pPage = CreatePage (fileType, NULL, NULL, TRUE);
				if (pPage)
				{
					CuDlgPageFixedWidth* pWndFixedWidth = (CuDlgPageFixedWidth*)pPage;
					pWndFixedWidth->SetArrayPos(pArrayPos);
					pArrayPos = NULL;
				}
			}
		}
		else
		{
			CreatePage (fileType, NULL, NULL, TRUE);
		}

		int nTabCount = m_cTab1.GetItemCount();
		m_cCheckEnforceFixedWidth.EnableWindow (FALSE);
		m_cTab1.SetCurSel (nTabCount-1);
		DisplayPage();
	}

	if (m_cTab1.GetItemCount() > 0)
		pParent->SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
	else
		pParent->SetWizardButtons(PSWIZB_BACK);
}

void CuPPage2::OnCheckEnforceFixedWidth() 
{
	BOOL bAutoDetect = TRUE;
	EnforceFixedWidth(bAutoDetect);
}

LRESULT CuPPage2::OnWizardNext() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CString strMsg;
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage2& dataPage2 = data.m_dataPage2;

		TCITEM item;
		memset (&item, 0, sizeof (item));
		item.mask       = TCIF_PARAM;
		CWnd* pWnd = NULL;
		int nSel, nTabCount = m_cTab1.GetItemCount();

#if defined (CONFIRM_TABCHOICE_WITH_CHECKBOX)
		nSel = m_cTab1.GetCurSel();
		//
		// Actually, the DBF format will have only one possible solution.
		// Any other file that behave like DBF must be in the following test !
		if (dataPage1.GetImportedFileType() != IMPORT_DBF)
		{
			//
			// Please check one of the proposed solutions.
			strMsg.LoadString (IDS_CHECK_ONEOF_PROPOSED_SOLUTION);

			int i, nCheck = 0;
			for (i=0; i<nTabCount; i++)
			{
				if (m_cTab1.GetItem (i, &item))
				{
					pWnd = (CWnd*)item.lParam;
					ASSERT (pWnd);
					if (!pWnd)
						continue;
					nCheck = pWnd->SendMessage(WMUSRMSG_GETITEMDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
					if (nCheck == 1 && i == nSel)
						break;

					if (nCheck == 1 && i != nSel)
					{
						m_cTab1.SetCurSel(i);
						DisplayPage();
						break;
					}
				}
			}

			if (nCheck == 0)
			{
				AfxMessageBox(strMsg);
				return E_FAIL;
			}
		}

#endif
		nSel = m_cTab1.GetCurSel();
		if (!m_cTab1.GetItem (nSel, &item))
		{
			AfxMessageBox (_T("System Error(CuPPage2::OnWizardNext): failed to get file format."));
			return E_FAIL;
		}

		pWnd = (CWnd*)item.lParam;
		ColumnSeparatorType nKindOf = FMT_UNKNOWN;
		if (pWnd)
		{
			nKindOf = (ColumnSeparatorType)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 0, 0);
			dataPage2.SetCurrentSeparatorType(nKindOf);

			if (nKindOf == FMT_FIELDSEPARATOR)
			{
				CaSeparatorSet* pSet = (CaSeparatorSet*)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 1, 0);
				CaSeparator* pSep = (CaSeparator*)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 2, 0);
				int nColumnCount = (int)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 3, 0);
				dataPage2.SetCurrentFormat (pSet, pSep);
				dataPage2.SetColumnCount (nColumnCount);
			}
			else
			if (nKindOf == FMT_FIXEDWIDTH)
			{
				ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CuDlgPageFixedWidth)));
				if (!pWnd->IsKindOf(RUNTIME_CLASS(CuDlgPageFixedWidth)))
					return E_FAIL;
				CuDlgPageFixedWidth* pWndFw = (CuDlgPageFixedWidth*)pWnd;
				CuWndFixedWidth& fixw = pWndFw->GetFixedWidthCtrl();

				int  i, nSize, nDividerCount = (int)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 3, 0);
				int* pArrayDividerPos = (int*)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 1, 0);
				//
				// CHECK validation:
				if (dataPage1.IsExistingTable() && dataPage1.GetFileMatchTable() == 1)
				{
					int nTableColumns = dataPage1.GetTableColumns().GetCount();
					if (nTableColumns != (nDividerCount+1))
					{
						//
						// The number of columns you set is not equal to the number of columns in the table.
						CString strMsg;
						strMsg.LoadString(IDS_FIXEDWIDTH_COLUMNxNOT_MATCH_TABLE_COLUMNS);

						AfxMessageBox (strMsg);
						if (pArrayDividerPos)
							delete pArrayDividerPos;
						return E_FAIL;
					}
				}

				//
				// The number of columns = the number of dividers + 1.
				dataPage2.SetColumnCount (nDividerCount+1);
				CArray < int, int >& arrayDividerPos = dataPage2.GetArrayDividerPos();
				arrayDividerPos.RemoveAll();

				for (i=0; i<nDividerCount; i++)
				{
					arrayDividerPos.Add(pArrayDividerPos[i]);
				}

				//
				// Construct the array of fields:
				CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
				nSize = arrayRecord.GetSize();
				for (i=0; i<nSize; i++)
				{
					CaRecord* pRec = (CaRecord*)arrayRecord.GetAt(i);
					delete pRec;
				}
				arrayRecord.RemoveAll();

				int nNumConumnSize = 0;
				CStringArray& arrayLine = dataPage1.GetArrayTextLine();
				nSize = arrayLine.GetSize();
				arrayRecord.SetSize(nSize);
				for (i=0; i<nSize; i++)
				{
					CString strLine(arrayLine.GetAt(i));
					CaRecord* pNewRecord = new CaRecord();
					CStringArray& arrayFields = pNewRecord->GetArrayFields();
					arrayRecord.SetAt(i, (void*)pNewRecord);

					GetFieldsFromLineFixedWidth (strLine, arrayFields, pArrayDividerPos, nDividerCount);
					if (i == 0)
					{
						nNumConumnSize = arrayFields.GetSize();
						dataPage2.CleanFieldSizeInfo();
						dataPage2.SetFieldSizeInfo (nNumConumnSize);
					}

#if defined (MAXLENGTH_AND_EFFECTIVE)
					//
					// Calculate the effective length and the trailing spaces
					// for each fields:
					for (int j=0; j<nNumConumnSize; j++)
					{
						int nLen1, nLen2;
						CString strField = arrayFields.GetAt(j);
						nLen1 = strField.GetLength();
						strField.TrimRight(_T(' '));
						nLen2 = strField.GetLength();

						int&  nFieldMax = dataPage2.GetFieldSizeMax(j);
						int&  nFieldEffMax = dataPage2.GetFieldSizeEffectiveMax(j);
						BOOL& bTrailingSpace = dataPage2.IsTrailingSpace(j);
						if (nLen1 > 0 && nLen2 < nLen1)
							bTrailingSpace = TRUE;

						if (nLen1 > nFieldMax)
							nFieldMax = nLen1;
						if (nLen2 > nFieldEffMax)
							nFieldEffMax = nLen2;
					}
#endif

				}
				if (pArrayDividerPos)
					delete pArrayDividerPos;
				
				BOOL bUser = (fixw.GetReadOnly() == FALSE);
				dataPage2.SetFixedWidthUser(bUser);
			}
			else
			if (nKindOf == FMT_DBF)
			{
				int nColumnCount = (int)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 3, 0);
				dataPage2.SetColumnCount (nColumnCount);
			}
		}

		if (dataPage2.GetArrayRecord().GetSize() == 0)
		{
			if (dataPage1.IsExistingTable() || nKindOf != FMT_DBF)
			{
				//
				// The file has no records. \nNothing to be imported.
				strMsg.LoadString(IDS_FILE_HAS_NO_RECORDS);
				AfxMessageBox (strMsg);
				return E_FAIL;
			}
			else 
			{
				//
				// The file has no records. \nNothing to be imported.
				int nAnswer = AfxMessageBox (IDS_MSG_CREATE_EMPTY_TABLE, MB_ICONQUESTION|MB_YESNO);
				if (nAnswer != IDYES)
					return E_FAIL;
				CaDataPage3& dataPage3 = data.m_dataPage3;
				dataPage3.SetCreateTableOnly(TRUE);

				//
				// Add 1 row (all fielsds are empty) just for purpose of scrolling.
				// When there are no row, we cannot scroll the control horizontally.
				CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
				arrayRecord.SetSize(1);
				CaRecord* pNewRecord = new CaRecord();
				arrayRecord.SetAt(0, pNewRecord);
				CStringArray& arrayField = pNewRecord->GetArrayFields();
				arrayField.SetSize(dataPage2.GetColumnCount());

				for (int k=0; k<dataPage2.GetColumnCount(); k++)
				{
					arrayField.SetAt(k, _T(""));
				}
			}
		}

		return CPropertyPage::OnWizardNext();
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return E_FAIL;
	}
	catch(...)
	{
		AfxMessageBox (_T("System error(CuPPage2::OnWizardNext): Fail to go to the next step."));
		return E_FAIL;
	}
}

void CuPPage2::OnDestroy() 
{
	TCITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	CWnd* pWnd = NULL;
	while (m_cTab1.GetItem (0, &item))
	{
		pWnd = (CWnd*)item.lParam;
		if (pWnd)
			pWnd->DestroyWindow();
		m_cTab1.DeleteItem (0);
	}

	CPropertyPage::OnDestroy();
}

void CuPPage2::OnButtonShowRejectedSolution() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	if (dataPage1.GetListFailureInfo().GetCount() > 0)
	{
		CxDlgDetectionInfo dlg;
		dlg.SetData(&data);
		dlg.DoModal();
	}
	else
	{
		CString strMsg;
		int iSolfound = m_cTab1.GetItemCount();
		if (m_cCheckEnforceFixedWidth.GetCheck() != 0)
			iSolfound--;
		if (iSolfound==0)
		{
			//
			// All possible formats were rejected at the first line of the file.\n\n
			// If you are importing into an existing table, the reason may be that you left the\n
			// 'Matches[...]Columns' checkbox checked in STEP 1, and the number of\n
			// columns of the file to import is different from that of the Ingres Table. ]
			strMsg.LoadString (IDS_REJECTED_INFO1);
		}
		else
		{
			//
			// All other possible formats were rejected at the first line of the file.\n\n
			strMsg.LoadString (IDS_REJECTED_INFO2);
		}

		AfxMessageBox (strMsg);
	}
}

void CuPPage2::Loading()
{
	CString strMsg;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	CaDataForLoadSave* pData4LoadSave = data.GetLoadSaveData();
	if (!pData4LoadSave)
		return;
	if (pData4LoadSave->GetUsedByPage() >= 2)
		return;
	
	//
	// Check if the loaded parameters are still applicable:
	BOOL bLoadError = FALSE;
	if (dataPage1.GetFile2BeImported().CompareNoCase(pData4LoadSave->GetFile2BeImported()) != 0)
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.GetFileMatchTable() != pData4LoadSave->GetFileMatchTable())
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.GetLineCountToSkip() != pData4LoadSave->GetLineCountToSkip())
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.GetCustomSeparator() != pData4LoadSave->GetCustomSeparator())
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.GetCustomSeparator() != pData4LoadSave->GetCustomSeparator())
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.IsExistingTable() != pData4LoadSave->IsExistingTable())
		bLoadError = TRUE;
	if (!bLoadError && dataPage1.IsExistingTable())
	{
		if (!bLoadError && dataPage1.GetNode().CompareNoCase (pData4LoadSave->GetNode()) != 0)
			bLoadError = TRUE;
		if (!bLoadError && dataPage1.GetDatabase().CompareNoCase (pData4LoadSave->GetDatabase()) != 0)
			bLoadError = TRUE;
		if (!bLoadError && dataPage1.GetTableOwner().CompareNoCase (pData4LoadSave->GetTableOwner()) != 0)
			bLoadError = TRUE;
		if (!bLoadError && dataPage1.GetTable().CompareNoCase (pData4LoadSave->GetTable()) != 0)
			bLoadError = TRUE;
	}

	if (bLoadError)
	{
		// Load Import Parameters error.\n
		// The loaded parameters in step 1 are no longer matches the current ones.
		strMsg.LoadString(IDS_LOADPARAM_ERR_STEP1_NO_MATCH);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		data.CleanLoadSaveData();
		return;
	}

	pData4LoadSave->SetUsedByPage (2);
	if (pData4LoadSave->GetFixedWidthUser())
	{
		m_cCheckEnforceFixedWidth.SetCheck(1);
		EnforceFixedWidth(FALSE);
	}

	BOOL bLoadChoice = FALSE;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask = TCIF_PARAM;
	int i, nCount = m_cTab1.GetItemCount();
	item.mask = TCIF_PARAM;
	for (i=0; i<nCount; i++)
	{
		if (!m_cTab1.GetItem (i, &item))
			continue;

		CWnd* pWnd = (CWnd*)item.lParam;
		ASSERT(pWnd);
		if (!pWnd)
			continue;

		ASSERT(IsWindow (pWnd->m_hWnd));
		if (!IsWindow (pWnd->m_hWnd))
			continue;

		ColumnSeparatorType nKindOf = (ColumnSeparatorType)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 0, 0);
		if (nKindOf != pData4LoadSave->GetSeparatorType())
			continue;

		if (nKindOf == FMT_FIELDSEPARATOR)
		{
			//
			// We found the Tab (CSV Result that has been saved). We check then to see
			// if the separator matches the loaded one:
			CaSeparatorSet* pSet = (CaSeparatorSet*)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 1, 0);
			CaSeparator* pSep = (CaSeparator*)pWnd->SendMessage(WMUSRMSG_GETITEMDATA, 2, 0);
			CString strSep = pData4LoadSave->GetSeparator();
		
			if (strSep.IsEmpty() && !pSep)
			{
				//
				// Be careful, if pSep = NULL then then we are in the special case.
				// That is each line is considered as a single column:
				m_cTab1.SetCurSel (i);
				DisplayPage();
				pWnd->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, (LPARAM)1);
				bLoadChoice = TRUE;
				break;
			}
			else
			if (strSep.CompareNoCase(pSep->GetSeparator()) == 0 && 
			    pData4LoadSave->GetTextQualifier()[0]== pSet->GetTextQualifier() &&
			    pData4LoadSave->GetConsecutiveAsOne() == pSet->GetConsecutiveSeparatorsAsOne())
			{
				m_cTab1.SetCurSel (i);
				DisplayPage();
				pWnd->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, (LPARAM)1);
				bLoadChoice = TRUE;
				break;
			}
		}
		else
		if (nKindOf == FMT_FIXEDWIDTH)
		{
			BOOL bMatch = FALSE;
			ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CuDlgPageFixedWidth)));
			if (!pWnd->IsKindOf(RUNTIME_CLASS(CuDlgPageFixedWidth)))
				continue;
			CuDlgPageFixedWidth* pWndFw = (CuDlgPageFixedWidth*)pWnd;
			CuWndFixedWidth& fixw = pWndFw->GetFixedWidthCtrl();

			if (!fixw.GetReadOnly() && pData4LoadSave->GetFixedWidthUser())
			{
				m_cTab1.SetCurSel (i);
				bMatch = TRUE;
			}

			if (fixw.GetReadOnly() && !pData4LoadSave->GetFixedWidthUser())
			{
				m_cTab1.SetCurSel (i);
				bMatch = TRUE;
			}
			if (bMatch)
			{
				CArray < int , int >& arrayDivider = pData4LoadSave->GetArrayDividerPos();
				int k, nPosCount = arrayDivider.GetSize();
				for (k=0; k<nPosCount; k++)
				{
					fixw.AddDivider (CPoint (arrayDivider.GetAt(k), 0));
				}

				DisplayPage();
				pWnd->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, (LPARAM)1);
				bLoadChoice = TRUE;
				break;
			}
		}
		else
		if (nKindOf == FMT_DBF)
		{
			bLoadChoice = TRUE;
		}
		else
		{
			ASSERT(FALSE);
		}
	}

	if (!bLoadChoice)
	{
		// Load Import Parameters error.\n
		// The choice of solution loaded does not match the detected solutions.
		strMsg.LoadString (IDS_LOADPARAM_ERR_NO_MATCH_SOLUTION);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		data.CleanLoadSaveData();
	}
}

