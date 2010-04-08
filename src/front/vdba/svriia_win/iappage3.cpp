/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage3.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 3 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 18-Oct-2001 ()
**    BUG #106084, Fixed the static message text (in step 3) 
**    when the column layouts are not allowed to change.
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 09-Jan-2001 (noifr01)
**    (bug 106737) when using the "servers" branch, the server still may be
**    an ingres server
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 03-Apr-2002 (uk$so01)
**    BUG #107488, Make svriia.dll enable to import empty .DBF file
**    It just create the table.
** 04-Apr-2002 (uk$so01)
**    BUG #107505, import column that has length 0, record length != sum (column's length)
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 23-Mar-2004 (uk$so01)
**    BUG #112007, Use the CString[0] to compare the first character
**    with a TCHAR.
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/

#include "stdafx.h"
#include "resource.h"
#include "iappage3.h"
#include "iapsheet.h"
#include "wmusrmsg.h"
#include "rctools.h"
#include "dmltable.h"
#include "ingobdml.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
static void CALLBACK DisplayListCtrlLower(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo);



IMPLEMENT_DYNCREATE(CuPPage3, CPropertyPage)
CuPPage3::CuPPage3() : CPropertyPage(CuPPage3::IDD)
{
	//{{AFX_DATA_INIT(CuPPage3)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);
	m_pDummyColumn = new CaColumn(_T("<dummy>"), _T(""));
	m_strCreateTableDefaultType = _T("varchar");
	m_nLengthMargin = 0;
	m_nLength4NullColum = 0;
	m_bFirstActivate = TRUE;
	m_nCurrentDetection = 0;
	m_nCurrentSeparatorType = FMT_UNKNOWN;
	m_pSeparatorSet = NULL;
	m_pSeparator = NULL;
	m_bCurrentFixedWidth = FALSE;

	CString strStep;
	strStep.LoadString (IDS_IIA_STEP_3);
	m_strPageTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}


CuPPage3::~CuPPage3()
{
	if (m_pDummyColumn)
		delete m_pDummyColumn;
	m_pDummyColumn = NULL;
}

void CuPPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPPage3)
	DDX_Control(pDX, IDC_COMBO2, m_cComboNumericFormat);
	DDX_Control(pDX, IDC_COMBO1, m_cComboDataFormat);
	DDX_Control(pDX, IDC_STATIC_CHECKMARK, m_cStaticCheckMark);
	DDX_Control(pDX, IDC_STATIC_TEXT1, m_cStaticTextNewTableInfo);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CuPPage3)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CuPPage3::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	try
	{
		CRect r;
		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		
		m_wndHeaderRows.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, r, this, 5000);
		m_wndHeaderRows.MoveWindow (r);
		m_wndHeaderRows.ShowWindow (SW_SHOW);

		ComboDataFormat(&m_cComboDataFormat);
		ComboNumericFormat(&m_cComboNumericFormat);
	}
	catch (...)
	{
		TRACE0("Raise exception at CuPPage3::OnInitDialog()\n");
		return FALSE;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuPPage3::OnSetActive() 
{
	try
	{
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		pParent->SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
		int nPrev = pParent->GetPreviousPage();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage2& dataPage2 = data.m_dataPage2;
		CaDataPage3& dataPage3 = data.m_dataPage3;
		m_wndHeaderRows.SetDisplayFunction(DisplayListCtrlLower, NULL);
		//
		// Check to see if we need to re-initialize the data:
		CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
		BOOL bReInitialize = FALSE;
		if (m_bFirstActivate)
			bReInitialize = TRUE;
		if (nPrev < 3 && m_bFirstActivate == FALSE)
		{
			//
			// Check if we realy need to re-initialize the characteristics of column layout ?
			// We need to re-initialize if one of the following occures:
			// 1) User has modified the choice in STEP 2.
			// 2) The solutions have been re-detected.
			bReInitialize = FALSE;
			if (m_nCurrentDetection != dataPage1.GetLastDetection())
			{
				bReInitialize = TRUE;
			}
			else
			{
				ColumnSeparatorType nCurrentSeparatorType = dataPage2.GetCurrentSeparatorType();
				CaSeparatorSet* pSeparatorSet;
				CaSeparator* pSeparator;

				if (nCurrentSeparatorType != m_nCurrentSeparatorType)
					bReInitialize = TRUE;
				else
				{
					if (m_nCurrentSeparatorType != FMT_FIXEDWIDTH)
					{
						dataPage2.GetCurrentFormat(pSeparatorSet, pSeparator);
						if ((m_pSeparatorSet) != pSeparatorSet || (m_pSeparator != pSeparator))
						{
							bReInitialize = TRUE;
						}
					}
					else
					{
						if (m_bCurrentFixedWidth != dataPage2.GetFixedWidthUser())
							bReInitialize = TRUE;
					}
				}
			}
		}

		TRACE1("CuPPage3::OnSetActive(re-initialize column layout = %d\n)", bReInitialize);
		if (bReInitialize)
		{
			commonItemData.Cleanup();
			ShowRecords (&data);
		}

		Loading();
		TRACE1 ("Previous page = %d\n", nPrev);

		m_bFirstActivate = FALSE;
		m_nCurrentDetection = dataPage1.GetLastDetection();
		m_nCurrentSeparatorType = dataPage2.GetCurrentSeparatorType();
		m_bCurrentFixedWidth = dataPage2.GetFixedWidthUser();
		dataPage2.GetCurrentFormat(m_pSeparatorSet, m_pSeparator);

		return CPropertyPage::OnSetActive();
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return FALSE;
	}
	catch(...)
	{
		AfxMessageBox (_T("System error(CuPPage3::OnSetActive): Failed to activate the page."));
		return FALSE;
	}
}

BOOL CuPPage3::OnKillActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	pParent->SetPreviousPage (3);
	
	return CPropertyPage::OnKillActive();
}


void CuPPage3::ShowRecords(CaIIAInfo* pData)
{
	CString strMsg;
	CString strCol;
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	CaDataPage2& dataPage2 = pData->m_dataPage2;
	CaDataPage3& dataPage3 = pData->m_dataPage3;

	//
	// How many columns detected from the imported file ?
	int i, nSize, nColumnCount = dataPage2.GetColumnCount();
	CStringArray& arrayColumns = dataPage2.GetArrayColumns();
	CStringArray& arrayColumnType = dataPage2.GetArrayColumnType();

	//
	// nColumnCount+1 means that we add an extra column at position 0:
	nSize = nColumnCount+1;
	LSCTRLHEADERPARAMS* pLsp = new LSCTRLHEADERPARAMS[nSize];
	CStringArray arrayHeader;
	CaSeparatorSet* pSet = NULL;
	CaSeparator* pSeparator = NULL;
	dataPage2.GetCurrentFormat (pSet, pSeparator);
	BOOL bFirstLineAsHeader = GenerateHeader(dataPage1, pSet, pSeparator, nColumnCount, arrayHeader);
	for (i=0; i<nSize; i++)
	{
		if (i==0)
		{
			pLsp[i].m_fmt = LVCFMT_LEFT;
			lstrcpy (pLsp[i].m_tchszHeader, _T(""));
			pLsp[i].m_cxWidth= 120;
			pLsp[i].m_bUseCheckMark = FALSE;
			continue;
		}

		pLsp[i].m_fmt = LVCFMT_LEFT;
		pLsp[i].m_cxWidth= 100;
		pLsp[i].m_bUseCheckMark = FALSE;

		if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
		{
			//
			// If we handle the DBF file, then display the columns queried from the file
			// instead of col1, col2, ...
			strCol = arrayColumns.GetAt(i-1);
		}
		else
		{
			if (bFirstLineAsHeader)
				strCol = arrayHeader.GetAt(i-1);
			else
				strCol.Format (_T("col%i"), i);
		}

#if defined (MAXLENGTH_AND_EFFECTIVE)
		CString strSize;
		int& nFieldMax = dataPage2.GetFieldSizeMax(i-1);
		int& nFieldEff = dataPage2.GetFieldSizeEffectiveMax(i-1);
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
		{
			if (nFieldEff < nFieldMax)
				strSize.Format(_T(" (%d+%d)"), nFieldEff, nFieldMax-nFieldEff);
			else
				strSize.Format(_T(" (%d)"),  nFieldMax);
		}
		else
		{
			strSize.Format(_T(" (%d)"),  nFieldMax);
		}
		strCol+=strSize;
#endif

		if (strCol.GetLength() > MAXLSCTRL_HEADERLENGTH)
			lstrcpyn (pLsp[i].m_tchszHeader, strCol, MAXLSCTRL_HEADERLENGTH);
		else
			lstrcpy  (pLsp[i].m_tchszHeader, strCol);
	}

	m_wndHeaderRows.SetFixedRowInfo(5); // 5 line in the first list control
	m_wndHeaderRows.InitializeHeader(pLsp, nSize);
	m_wndHeaderRows.SetDisplayFunction(DisplayListCtrlLower, NULL);
	delete pLsp;

	//
	// Display the CHARACTERISTIC of Target TABLE (the first 4 lines):
	BOOL bMatchTableAndFileColumns = (dataPage1.IsExistingTable() && dataPage1.GetFileMatchTable() == 1);
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
	commonItemData.Cleanup();
	commonItemData.m_bExistingTable = dataPage1.IsExistingTable();
	commonItemData.m_bAllowChange   = bMatchTableAndFileColumns? FALSE: TRUE;
	commonItemData.m_bGateway = (!dataPage1.GetServerClass().IsEmpty() || dataPage1.GetGatewayDatabase());

	//
	// Dumy column:
	commonItemData.m_arrayColumn.Add (m_pDummyColumn);

	CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
	if (dataPage1.IsExistingTable())
	{
		if (commonItemData.m_bAllowChange)
			strMsg.LoadString(IDS_COLUMN_LAYOUT);
		else
			strMsg.LoadString(IDS_COLUMN_LAYOUT2);
		m_cStaticTextNewTableInfo.SetWindowText(strMsg);
		//
		// The columns from the existing table:
		CTypedPtrList< CObList, CaDBObject* >& listColumn = dataPage1.GetTableColumns();
		POSITION pos = listColumn.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumn* pCol = (CaColumn*)listColumn.GetNext (pos);
			commonItemData.m_arrayColumn.Add (pCol);
			//
			// Default position of columns are the same as those queried
			// from the database:
			commonItemData.m_arrayColumnOrder.Add (pCol);
		}

		//
		// The columns layout (the columns that are queried from the file).
		// nColumnCount + 1, +1 for the dummy column at the position 0.
		commonItemData.m_arrayColumnOrder.SetSize (nColumnCount + 1);
		commonItemData.m_arrayColumnOrder.SetAt (0, m_pDummyColumn);
		pos = listColumn.GetHeadPosition();
		for (i=1; i<nColumnCount + 1; i++)
		{
			if (pos != NULL)
			{
				CaColumn* pCol = (CaColumn*)listColumn.GetNext (pos);
				commonItemData.m_arrayColumnOrder.SetAt(i, pCol);
			}
			else
			{
				//
				// NULL, the control will display the text "<not imported>".
				// This is the case when the columns queried from the file are more than
				// those in the table.
				commonItemData.m_arrayColumnOrder.SetAt(i, NULL);
			}
		}
	}
	else
	{
		CString strDefaultColumnType;
		UINT nIDS = dataPage3.GetCreateTableOnly() ? IDS_INPUTTABLE_CHARACTERISTICS_CREATEONLY: IDS_INPUTTABLE_CHARACTERISTICS;
		strMsg.LoadString(nIDS);
		m_cStaticTextNewTableInfo.SetWindowText(strMsg);
		commonItemData.m_arrayColumnOrder.Add (m_pDummyColumn);
		//
		// The DEFAULT characteristics of columns of table to be created:
		//    - name     : col1, col2, ...
		//    - type     : char
		//    - length   : empty (user must explicitely enter the length)
		//    - nullable : with null
		//    - default  : null
		CTypedPtrList< CObList, CaDBObject* >& listColumn = dataPage1.GetTableColumns();
		if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
		{
			ASSERT (nColumnCount == arrayColumnType.GetSize());
			ASSERT (nColumnCount == arrayColumns.GetSize());
		}

		for (i=0; i<nColumnCount; i++)
		{
			if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
			{
				strCol = arrayColumns.GetAt(i);
				strDefaultColumnType = arrayColumnType.GetAt(i);
			}
			else
			{
				if (bFirstLineAsHeader)
					strCol = arrayHeader.GetAt(i);
				else
					strCol.Format (_T("col%i"), i+1);
				strDefaultColumnType = m_strCreateTableDefaultType;
			}
			CaColumn* pNewCol = new CaColumn(strCol, strDefaultColumnType);
			pNewCol->SetNullable (TRUE); // With NULL
			pNewCol->SetDefault  (TRUE); // With default
			listColumn.AddTail (pNewCol);

#if defined (MAXLENGTH_AND_EFFECTIVE)
			int& nFieldMax = dataPage2.GetFieldSizeMax(i);
			int& nFieldEff = dataPage2.GetFieldSizeEffectiveMax(i);
			if (nFieldMax > 0)
			{
				if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
				{
					if (nFieldEff > 0 && nFieldEff < nFieldMax)
						pNewCol->SetLength(nFieldEff+m_nLengthMargin);
					else
						pNewCol->SetLength(nFieldMax+m_nLengthMargin);
				}
				else
				{
					pNewCol->SetLength(nFieldMax);
				}
			}
			else
			{
				//
				// Default length for NULL column or length = 0.
				pNewCol->SetLength(m_nLength4NullColum);
			}
#endif

			commonItemData.m_arrayColumn.Add (pNewCol);
			//
			// Default position of columns are the same as those of table
			// being created
			commonItemData.m_arrayColumnOrder.Add (pNewCol);
		}
	}

	
	//
	// The rows information: 
	int iIndex = -1;
	int nCount = listCtrlHeader.GetItemCount();
	for (i=0; i<m_wndHeaderRows.GetFixedRowInfo(); i++)
	{
		iIndex = listCtrlHeader.InsertItem (nCount, _T(""));
		if (iIndex != -1)
		{
			listCtrlHeader.SetItemData (iIndex, (DWORD)&commonItemData);
			listCtrlHeader.SetFgColor  (iIndex, RGB(0, 0, 255));
		}
	}
	listCtrlHeader.DisplayItem ();
	

	//
	// Display the rows:
	CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
	m_wndHeaderRows.SetSharedData(&arrayRecord);
	m_wndHeaderRows.ResizeToFit();
}



LRESULT CuPPage3::OnWizardBack() 
{
	CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
	listCtrlHeader.HideProperty (TRUE);

	return CPropertyPage::OnWizardBack();
}

LRESULT CuPPage3::OnWizardNext() 
{
	try
	{
		CString strItem;
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaDataPage2& dataPage2 = data.m_dataPage2;
		CaDataPage3& dataPage3 = data.m_dataPage3;


		CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
		listCtrlHeader.HideProperty (TRUE);
		
		if (!ValidateData(&data))
		{
			return E_FAIL;
		}
		
		//
		// Generate the create table syntax
		if (!dataPage1.IsExistingTable())
		{
			CString strStatement;
			CaTableDetail tableDetail (dataPage1.GetNewTable(), dataPage1.GetTableOwner());
			CTypedPtrList < CObList, CaColumn* >& listColumn =  tableDetail.GetListColumns();
			CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();
#if defined (_NEED_SPECIFYING_PAGESIZE)
			ASSERT(FALSE); // TODO
#endif
			//
			// The index = 0 is a dummy column !
			int i, nSize = commonItemData.m_arrayColumnOrder.GetSize();
			for (i=1; i<nSize; i++)
			{
				CaColumn* pCol = commonItemData.m_arrayColumnOrder.GetAt(i);
				listColumn.AddTail (new CaColumn (*pCol));

				pCol->GenerateStatement (strStatement);
			}
			tableDetail.GenerateSyntaxCreate (strStatement);
			dataPage3.SetStatement(strStatement);
		}
		
		//
		// II_DATE_FORMAT:
		m_cComboDataFormat.GetWindowText (strItem);
		if (!strItem.IsEmpty() && strItem.CompareNoCase(_T("(default)")) != 0)
		{
			dataPage3.SetIIDateFormat (strItem);
		}
		//
		// II_DECIMAL:
		m_cComboNumericFormat.GetWindowText (strItem);
		if (!strItem.IsEmpty() && strItem.CompareNoCase(_T("(default)")) != 0)
		{
			dataPage3.SetIIDecimalFormat (strItem);
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
		AfxMessageBox (_T("System error (CuPPage3::OnWizardNext): Failed to go to the next step."));
		return E_FAIL;
	}
}


BOOL CuPPage3::ValidateData(CaIIAInfo* pData)
{
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	CaDataPage2& dataPage2 = pData->m_dataPage2;
	CaDataPage3& dataPage3 = pData->m_dataPage3;
	CString strMsg;

	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();

	//
	// Verify that all characteristics of the columns of the table
	// to be created have been provided correctly:
	if (!dataPage1.IsExistingTable())
	{
		CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
		CHeaderCtrl* pHeader = listCtrlHeader.GetHeaderCtrl();
		ASSERT (pHeader);
		if (!pHeader)
			return FALSE;
		CString strItem;
		CString strColName;

		int nHeaderCount = pHeader->GetItemCount();
		for (int i=1; i<nHeaderCount; i++)
		{
			//
			// Verify the column name:
			strItem = listCtrlHeader.GetItemText (0, i);
			strItem.TrimLeft();
			strItem.TrimRight();
			if (strItem.IsEmpty())
			{
				// 
				// You must provide the column name of the table to be created.
				strMsg.LoadString(IDS_NEWTABLE_NEEDS_COLUMNS);
				AfxMessageBox (strMsg);
				return FALSE;
			}

			//
			// Verify the column length (if data type needs it):
			CaColumn* pCol = commonItemData.m_arrayColumnOrder.GetAt(i);
			ASSERT (pCol);
			if (!pCol)
				return FALSE;
			int nNeedLength = pCol->NeedLength();
			switch (nNeedLength)
			{
			case CaColumn::COLUMN_LENGTH:
				strItem = listCtrlHeader.GetItemText (2, i);
				if (_ttoi (strItem) == 0)
				{
					strColName = listCtrlHeader.GetItemText (0, i);
					strItem = listCtrlHeader.GetItemText (1, i);
					if (strItem == _T("float"))
					{
						//
						// You must provide the precision of column '%1'.;
						AfxFormatString1(strMsg, IDS_COLUMN_NEEDS_LENGTH1, (LPCTSTR)strColName);
					}
					else
					{
						// 
						// You must provide the length of column '%1'.
						AfxFormatString1(strMsg, IDS_COLUMN_NEEDS_LENGTH2, (LPCTSTR)strColName);
					}
					AfxMessageBox (strMsg);
					return FALSE;
				}
				break;

			case CaColumn::COLUMN_PRECISIONxSCALE:
				strColName = listCtrlHeader.GetItemText (0, i);
				strItem = listCtrlHeader.GetItemText (2, i);
				if (strItem.Find (_T(',')) == -1)
				{
					//
					// You must provide the precision and scale of the form 'p, s' of column '%1'.
					AfxFormatString1(strMsg, IDS_COLUMN_NEEDS_LENGTH3, (LPCTSTR)strColName);
					AfxMessageBox (strMsg);
					return FALSE;
				}
				else
				{
					int nComma = strItem.Find (_T(','));
					strItem = strItem.Left (nComma);
					if (_ttoi (strItem) == 0)
					{
						//
						// You must provide the precision and scale of column '%1'.
						AfxFormatString1(strMsg, IDS_COLUMN_NEEDS_LENGTH4, (LPCTSTR)strColName);
						AfxMessageBox (strMsg);
						return FALSE;
					}
				}
				break;
			case CaColumn::COLUMN_PRECISION:
				break;
			default:
				break;
			}

			strItem = listCtrlHeader.GetItemText (1, i);
			strItem.TrimLeft();
			strItem.TrimRight();
			if (strItem.IsEmpty())
			{
				// 
				// You must provide the column name of the table to be created.
				strMsg.LoadString(IDS_NEWTABLE_NEEDS_COLUMNS);
				AfxMessageBox (strMsg);
				return FALSE;
			}
		}
	}

	BOOL bExistColumn = FALSE;
	//
	// Verify if there are duplicated column names:
	int i, nSize = commonItemData.m_arrayColumnOrder.GetSize();
	//
	// Start from 1 because of the dummy column at position 0:
	for (i=1; i<nSize; i++)
	{
		CaColumn* pCol = commonItemData.m_arrayColumnOrder.GetAt(i);
		if (!pCol)
			continue;

		bExistColumn = TRUE;
		for (int j=i+1; j<nSize; j++)
		{
			CaColumn* pExistCol = commonItemData.m_arrayColumnOrder.GetAt(j);
			if (!pExistCol)
				continue;
			if (pExistCol->GetName().CompareNoCase (pCol->GetName()) == 0)
			{
				CString strFormat;
				//
				// Duplicate column name '%s' at positions %d and %d.
				strFormat.LoadString (IDS_DUPLICATE_COLUMN_NAME);
				strMsg.Format (strFormat, (LPCTSTR)pCol->GetName(), i, j);
				AfxMessageBox(strMsg);
				return FALSE;
			}
		}
	}

	//
	// Specify at lease one column ?
	if (!bExistColumn)
	{
		//
		// Please specify at least one column.
		strMsg.LoadString(IDS_PLEASE_SPECIFY_AT_LEAST_1COLUMN);
		AfxMessageBox(strMsg);
		return FALSE;
	}

	//
	// Are the column lengths large enough to contain data ?
	nSize = commonItemData.m_arrayColumnOrder.GetSize();
	//
	// Start from 1 because of the dummy column at position 0:
	for (i=1; i<nSize; i++)
	{
		CaColumn* pCol = commonItemData.m_arrayColumnOrder.GetAt(i);
		if (!pCol)
			continue;
		int nNeedLength = pCol->NeedLength();
		if (nNeedLength != CaColumn::COLUMN_LENGTH)
			continue;

#if defined (MAXLENGTH_AND_EFFECTIVE)
		CString strSize;
		BOOL bWarnning = FALSE;
		int& nFieldMax = dataPage2.GetFieldSizeMax(i-1);
		int& nFieldEff = dataPage2.GetFieldSizeEffectiveMax(i-1);
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
		{
			if (nFieldEff < nFieldMax && nFieldEff > 0)
			{
				if (pCol->GetLength() < nFieldEff)
					bWarnning = TRUE;
			}
			else
			if (pCol->GetLength() < nFieldMax)
			{
				bWarnning = TRUE;
			}
		}
		else
		if (pCol->GetLength() < nFieldMax)
		{
			bWarnning = TRUE;
		}

		if (bWarnning)
		{
			CString strFormat;
			//
			// The length of column '%s' at position %d is too short.\nThe data will be truncated.
			strFormat.LoadString (IDS_COLUMN_HAS_TOOSHORT_LEN);

			strMsg.Format (strFormat, (LPCTSTR)pCol->GetName(), i);
			AfxMessageBox(strMsg);
			return TRUE;
		}
#endif
	}

	return TRUE;
}

//
// Here we have an extra column at the position 0 = "Source Data".
static void CALLBACK DisplayListCtrlLower(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo)
{
	CString strSourceData;
	strSourceData.LoadString(IDS_SOURCE_DATA);
	CaRecord* pRecord = (CaRecord*)pItemData;
	CStringArray& arrayFields = pRecord->GetArrayFields();
	int i, nSize = arrayFields.GetSize();
	int nIdx = nItem;
	int nCount = pListCtrl->GetItemCount();
	ASSERT (nItem >= 0 && nItem < pListCtrl->GetCountPerPage());
	if (!bUpdate || nIdx >= nCount)
		nIdx = pListCtrl->InsertItem (nItem, strSourceData);

	for (i=0; i<nSize; i++)
	{
		CString strField = arrayFields.GetAt(i);
		if (nIdx != -1)
		{
			pListCtrl->SetItemText (nIdx, i+1, strField);
		}
	}
}

void CuPPage3::ComboDataFormat(CComboBox* pCombo)
{
	const int cbSize = 12;
	TCHAR tchszTab [cbSize][48] = 
	{
		_T("(default)"),       //  US
		_T("US"),              //  dd-mmm-yyyy
		_T("MULTINATIONAL"),   //  dd/mm/yy
		_T("MULTINATIONAL4"),  //  dd/mm/yyyy
		_T("ISO"),             //  yymmdd
		_T("ISO4"),            //  yyyymmdd
		_T("FINLAND"),         //  yyyy-mm-dd
		_T("SWEDEN"),          //  yyyy-mm-dd
		_T("GERMAN"),          //  dd.mm.yyyy
		_T("YMD"),             //  yyyy-mmm-dd
		_T("DMY"),             //  dd-mmm-yyyy
		_T("MDY")              //  mmm-dd-yyyy
	};

	for (int i=0; i<cbSize; i++)
		pCombo->AddString(tchszTab[i]);
	pCombo->SetCurSel(0);
}

void CuPPage3::ComboNumericFormat(CComboBox* pCombo)
{
	const int cbSize = 3;
	TCHAR tchszTab [cbSize][48] = 
	{
		_T("(default)"),
		_T(". Period"),
		_T(", Comma")
	};

	for (int i=0; i<cbSize; i++)
		pCombo->AddString(tchszTab[i]);

	pCombo->SetCurSel(0);
}

void CuPPage3::Loading()
{
	CString strMsg;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	CaDataPage2& dataPage2 = data.m_dataPage2;
	CaDataPage3& dataPage3 = data.m_dataPage3;
	CaDataForLoadSave* pData4LoadSave = data.GetLoadSaveData();
	if (!pData4LoadSave)
		return;
	if (pData4LoadSave->GetUsedByPage() >= 3)
		return;

	//
	// Check if the loaded format is still compatible with the current choice:
	BOOL bFormatChanged = FALSE;
	if (dataPage2.GetCurrentSeparatorType() != pData4LoadSave->GetSeparatorType() || 
	    dataPage2.GetColumnCount() != pData4LoadSave->GetColumnCount())
	{
		bFormatChanged = TRUE;
	}
	else
	{
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIXEDWIDTH)
		{
			//
			// Check to see if the number of dividers loaded is equal to 
			// the current number of dividers::
			if (pData4LoadSave->GetArrayDividerPos().GetSize() != dataPage2.GetArrayDividerPos().GetSize())
				bFormatChanged = TRUE;
		}
		else
		if (dataPage2.GetCurrentSeparatorType() == FMT_FIELDSEPARATOR)
		{
			//
			// Check to see if the loaded combination SEP, TQ, CS is equal to the current combination:
			CString strSep = pData4LoadSave->GetSeparator();
			CaSeparatorSet* pSet = NULL;
			CaSeparator* pSep = NULL;
			dataPage2.GetCurrentFormat (pSet, pSep);
			if (!pSep)
			{
				// Be careful, if pSep = NULL then then we are in the special case.
				// That is each line is considered as a single column:
				if (pData4LoadSave->GetSeparator().IsEmpty() == FALSE || 
				    pData4LoadSave->GetTextQualifier() != _T("\0") ||
				    pData4LoadSave->GetConsecutiveAsOne() == TRUE)
				{
					bFormatChanged = TRUE;
				}
			}
			else
			if (strSep.CompareNoCase(pSep->GetSeparator()) != 0 || 
			    pData4LoadSave->GetTextQualifier()[0] != pSet->GetTextQualifier() ||
			    pData4LoadSave->GetConsecutiveAsOne() != pSet->GetConsecutiveSeparatorsAsOne())
			{
				bFormatChanged = TRUE;
			}
		}
		else
		if (dataPage2.GetCurrentSeparatorType() == FMT_DBF)
		{

		}
		else
		{
			ASSERT (FALSE);
		}
	}

	if (bFormatChanged)
	{
		//
		// Load Import Parameters error.\nThe loaded format is no longer matched with this current choice.
		strMsg.LoadString(IDS_LOADPARAM_ERR_NO_MATCH_FORMAT);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		data.CleanLoadSaveData();
		return;
	}

	pData4LoadSave->SetUsedByPage (3);
	CString strItem = pData4LoadSave->GetIIDateFormat();
	int nFound = m_cComboDataFormat.FindStringExact(-1, strItem);
	if (nFound != CB_ERR)
		m_cComboDataFormat.SetCurSel (nFound);

	strItem = pData4LoadSave->GetIIDecimalFormat();
	nFound = m_cComboNumericFormat.FindStringExact(-1, strItem);
	if (nFound != CB_ERR)
		m_cComboNumericFormat.SetCurSel (nFound);

	//
	// Check if the loaded layout columns are still applicable to the new layout columns:
	CTypedPtrArray < CObArray, CaColumn* >& arrayColumnOrder = pData4LoadSave->GetColumnOrder();
	CaIngresHeaderRowItemData& commonItemData = dataPage3.GetUpperCtrlCommonData();

	if (!dataPage1.IsExistingTable())
	{
		//
		// New table: if number of loaded columns is not equal to the number of the current
		// layout columns => error.
		if (arrayColumnOrder.GetSize() != commonItemData.m_arrayColumnOrder.GetSize())
		{
			//
			// Load Import Parameters error.\nThe number of columns is not matched.
			strMsg.LoadString(IDS_LOADPARAM_ERR_NO_MATCH_COLUMN_COUNT);

			AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
			data.CleanLoadSaveData();
			return;
		}
		else
		{
			int i, nSize = arrayColumnOrder.GetSize();
			for (i=1; i<nSize; i++)
			{
				CaColumn* pCol = arrayColumnOrder.GetAt(i);
				if (pCol)
				{
					CaColumn* pCol2 = commonItemData.m_arrayColumnOrder.GetAt(i);
					if (pCol2)
					{
						*pCol2 = *pCol;
					}
				}
			}
			CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
			listCtrlHeader.DisplayItem();
		}
	}
	else
	{
		//
		// For the existing table, we cannot change the characteristics of the columns:
		// But if all the loaded column names are in the list of current layout columns
		// then we set up the new order the one we loaded:

		// The index = 0 is a dummy column !
		int i, nSize = arrayColumnOrder.GetSize();
		for (i=1; i<nSize; i++)
		{
			CaColumn* pCol = arrayColumnOrder.GetAt(i);
			if (pCol)
			{
				BOOL bExit = FALSE;
				int k, nSize2 = commonItemData.m_arrayColumnOrder.GetSize();
				for (k=0; k<nSize2; k++)
				{
					CaColumn* pCol2 = commonItemData.m_arrayColumnOrder.GetAt (k);
					if (!pCol2)
						continue;
					if (pCol2->GetName().CompareNoCase(pCol->GetName()) == 0)
					{
						bExit = TRUE;
						break;
					}
				}

				if (!bExit)
				{
					// Load Import Parameters error.\n
					// The list of saved layout columns does not match the list of current layout columns.
					strMsg.LoadString(IDS_LOADPARAM_ERR_NO_MATCH_COLUMN_LAYOUT);
					AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
					data.CleanLoadSaveData();
					return;
				}
			}
		}

		//
		// Restore the layout orders to match the loaded one:
		int nWorked = 1;
		for (i=0; i<nSize; i++)
			commonItemData.m_arrayColumnOrder.SetAt (i, NULL);

		CTypedPtrList< CObList, CaDBObject* >& listColumn = dataPage1.GetTableColumns();
		POSITION pos = listColumn.GetHeadPosition();
		while (pos != NULL)
		{
			CaColumn* pCol = (CaColumn*)listColumn.GetNext (pos);
			for (i=1; i<nSize; i++)
			{
				CaColumn* pCol2 = arrayColumnOrder.GetAt(i);
				if (pCol2 && pCol2->GetName().CompareNoCase(pCol->GetName()) == 0)
				{
					commonItemData.m_arrayColumnOrder.SetAt (i, pCol);
					break;
				}
			}
			nWorked++;
		}

		CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
		listCtrlHeader.DisplayItem();
	}
}

