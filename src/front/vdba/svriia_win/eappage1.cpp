/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eappage1.cpp : implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 1 of Export assistant
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Date conversion and numeric display format.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 16-Mar-2004 (uk$so01)
**    BUG #111967 / ISSUE 13298850
**    The query does not get re-executed.
** 09-June-2004 (nansa02)
**    BUG #112435,Removed a default flag set for CFileDialog.
** 31-Jan-2005 (komve01)
**    BUG #113766,Issue# 13902259. 
**    Added additional validation for the existence of destination
**    folder for IEA.
** 07-Feb-2005 (lakvi01)
**    BUG 113849, Made 'Exponential Format' check-boxes in IEA to 
**    disappear in all cases of *.dbf output file format.
** 20-Jun-2007 (drivi01)
**    Move a default directory to save an export file to USERPROFILE
**    space.
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
** 05-Sep-2008 (drivi01)
**    Add a default flag OFN_EXPLORER back to display
**    nicer dialog consistent with the rest of the application.
**    Could not reproduce the hang.
**/


#include "stdafx.h"
#include "svriia.h"
#include "eappage1.h"

#include "rcdepend.h"
#include "eapsheet.h"
#include "rctools.h"  // Resource symbols of rctools.rc
#include "libguids.h" // guids
#include "sqlasinf.h" // sql assistant interface
#include "dmlexpor.h"
#include "tlsfunct.h"

#define RESULT_SET          0x00000001
#define RESULT_EXPORTTYPE   0x00000002

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CuIeaPPage1, CPropertyPage)
CuIeaPPage1::CuIeaPPage1() : CPropertyPage(CuIeaPPage1::IDD)
{
	//{{AFX_DATA_INIT(CuIeaPPage1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bAllowExpandTree = TRUE;
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);
	m_hIcon = theApp.LoadIcon (IDI_ICON_LOAD);
	m_hIconAssistant = (HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_ASSISTANT), IMAGE_ICON, 16, 16, LR_SHARED);

	CString strStep;
	strStep.LoadString (IDS_IEA_STEP_1);
	m_strPageTitle.LoadString(IDS_TITLE_EXPORT);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuIeaPPage1::~CuIeaPPage1()
{

}


void CuIeaPPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuIeaPPage1)
	DDX_Control(pDX, IDC_CHECK5, m_cCheckF8Display);
	DDX_Control(pDX, IDC_CHECK3, m_cCheckF4Display);
	DDX_Control(pDX, IDC_SPIN4, m_cSpinF8Scale);
	DDX_Control(pDX, IDC_SPIN3, m_cSpinF8Precision);
	DDX_Control(pDX, IDC_SPIN2, m_cSpinF4Scale);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinF4Precision);
	DDX_Control(pDX, IDC_EDIT6, m_cEditF8Scale);
	DDX_Control(pDX, IDC_EDIT5, m_cEditF8Precision);
	DDX_Control(pDX, IDC_EDIT4, m_cEditF4Scale);
	DDX_Control(pDX, IDC_EDIT3, m_cEditF4Precision);
	DDX_Control(pDX, IDC_COMBO1, m_cComboFormat);
	DDX_Control(pDX, IDC_EDIT2, m_cEditSql);
	DDX_Control(pDX, IDC_EDIT1, m_cEditFile2BeCreated);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonSqlAssistant);
	DDX_Control(pDX, IDC_BUTTON2, m_cButton3Periods);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	DDX_Control(pDX, IDC_TREE1, m_cTreeIngresDatabase);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonLoad);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuIeaPPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CuIeaPPage1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonLoadParameter)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonFile2BeCreated)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonSqlAssistant)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnSelchangedTree1)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TREE1, OnItemexpandedTree1)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboFileFormat)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEditF4Precision)
	ON_EN_CHANGE(IDC_EDIT4, OnChangeEditF4Scale)
	ON_EN_CHANGE(IDC_EDIT5, OnChangeEditF8Precision)
	ON_EN_CHANGE(IDC_EDIT6, OnChangeEditF8Scale)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuIeaPPage1 message handlers

void CuIeaPPage1::OnButtonLoadParameter() 
{
	//
	// Wrong file type: please select the export parameter file (*.ii_exp).
	CString strMsg;
	strMsg.LoadString(IDS_MSG_SELECT_EXPORT_PARAM_FILE);

	CString strFullName;
	CFileDialog dlg(
	    TRUE,
	    NULL,
	    NULL,
	    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Export Parameter Files (*.ii_exp)|*.ii_exp||"));
     
	/*Added as fix for bug 112435*/
	/* 09/05/2008 Put back OFN_EXPLORER flag to display nice "Open/Save As"
	** dialog. Hang doesn't seem to occur anymore. Remove the flag 
	** if it does.
	** dlg.m_ofn.Flags &= ~(OFN_EXPLORER); 
	*/
	
	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (IDS_MSG_SELECT_EXPORT_PARAM_FILE);
		return;
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (strExt.CompareNoCase (_T("ii_exp")) != 0)
		{
			AfxMessageBox (IDS_MSG_SELECT_EXPORT_PARAM_FILE);
			return;
		}
	}

	Loading(strFullName);
}

void CuIeaPPage1::Loading(LPCTSTR lpszFile)
{
	CString strMsg;
	CString strFullName =lpszFile; 
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();

	data.LoadData (strFullName);
	CaIeaDataForLoadSave* pLoadData = data.GetLoadSaveData();
	CaIeaDataPage1& dataPage1 = pLoadData->m_dataPage1;

	//
	// The name of the file to be Created:
	m_cEditFile2BeCreated.SetWindowText (dataPage1.GetFile2BeExported());

	//
	// Export Type:
	for (int i=0; i<m_cComboFormat.GetCount(); i++)
	{
		if (dataPage1.GetExportedFileType() == (ExportedFileType)m_cComboFormat.GetItemData(i))
		{
			m_cComboFormat.SetCurSel(i);
			break;
		}
	}

	//
	// Statement:
	m_cEditSql.SetWindowText(dataPage1.GetStatement());

	//
	// Select Object in the tree:
	CString strItem;
	CStringArray& itemPath = dataPage1.GetTreeItemPath();
	CTreeCtrl& cTree = m_cTreeIngresDatabase;
	HTREEITEM hItem = cTree.GetRootItem();
	int nSize = itemPath.GetSize();
	int nLevel = nSize-1;
	while (hItem && nSize > 0 && nLevel >= 0)
	{
		strItem = cTree.GetItemText (hItem);
		CString strPath(itemPath.GetAt(nLevel));
		if (strItem.CompareNoCase(strPath) == 0)
		{
			nLevel--;
			if (nLevel < 0)
			{
				cTree.SelectItem (hItem);
				break;
			}

			cTree.Expand (hItem, TVE_EXPAND );
			hItem = cTree.GetChildItem(hItem);
			continue;
		}

		hItem = cTree.GetNextSiblingItem(hItem);
	}

	CaIeaDataPage1& dataPage1Current = data.m_dataPage1;
	dataPage1Current = dataPage1;
	CaFloatFormat& floatFormat = dataPage1Current.GetFloatFormat();
	UpdateFloatPreferences(&floatFormat);
	UpdateExponentialFormat();
}


void CuIeaPPage1::FillComboFormat()
{
	const int nSize = 4;
	UINT nFormatIDs[nSize] = {IDS_EXPORT_CSV, IDS_EXPORT_DBF, IDS_EXPORT_FIXED,   /*IDS_EXPORT_COPY,*/       IDS_EXPORT_XML};
	UINT nFormatCode[nSize]= {EXPORT_CSV,     EXPORT_DBF,     EXPORT_FIXED_WIDTH, /*EXPORT_COPY_STATEMENT,*/ EXPORT_XML};

	int nIdx;
	CString strFormat;
	for (int i=0; i<nSize; i++)
	{
		strFormat.LoadString(nFormatIDs[i]);
		nIdx = m_cComboFormat.AddString(strFormat);
		ASSERT(strFormat != CB_ERR);
		if (strFormat != CB_ERR)
			m_cComboFormat.SetItemData (nIdx, (DWORD)nFormatCode[i]);
	}

	m_cComboFormat.SetCurSel(0);
}

ExportedFileType CuIeaPPage1::VerifyFileFormat (CString& strFullName, CString& strExt, int nFilterIndex)
{
	BOOL b2AppendExt = FALSE;
	ExportedFileType exportType = EXPORT_UNKNOWN;
	
	if (nFilterIndex != -1) // User chooses file by using CFileDialog.
	{
		//
		// Append automatically the file extension extention if the file name has no extention
		// and the file does not exist yet:
		if (strExt.IsEmpty())
		{
			//
			// Check the existence of file:
			if (_taccess(strFullName, 0) == -1)
				b2AppendExt = TRUE;
		}
		else
		{
			if (strExt.CompareNoCase (_T("csv")) == 0)
			{
				b2AppendExt = FALSE;
				exportType = EXPORT_CSV;
			}
			else
			if (strExt.CompareNoCase (_T("dbf")) == 0)
			{
				b2AppendExt = FALSE;
				exportType = EXPORT_DBF;
			}
			else
			if (strExt.CompareNoCase (_T("xml")) == 0)
			{
				b2AppendExt = FALSE;
				exportType = EXPORT_XML;
			}
		}

		if (b2AppendExt)
		{
			switch (nFilterIndex)
			{
			case 1:
				strFullName += _T(".csv");
				exportType = EXPORT_CSV;
				break;
			case 2:
				strFullName += _T(".dbf");
				exportType = EXPORT_DBF;
				break;
			case 3:
				strFullName += _T(".txt");
				break;
			case 4:
				strFullName += _T(".xml");
				exportType = EXPORT_XML;
				break;
			default:
				exportType = EXPORT_UNKNOWN;
				break;
			}
		}
	}

	return exportType;
}

void CuIeaPPage1::UpdateFloatPreferences(CaFloatFormat* pFloatFormat)
{
	CString strNum;
	strNum.Format(_T("%d"), pFloatFormat->GetFloat4Width());
	m_cEditF4Precision.SetWindowText(strNum);
	strNum.Format(_T("%d"), pFloatFormat->GetFloat4Decimal());
	m_cEditF4Scale.SetWindowText(strNum);

	strNum.Format(_T("%d"), pFloatFormat->GetFloat8Width());
	m_cEditF8Precision.SetWindowText(strNum);
	strNum.Format(_T("%d"), pFloatFormat->GetFloat8Decimal());
	m_cEditF8Scale.SetWindowText(strNum);

	BOOL bExpF4 = (pFloatFormat->GetFloat4DisplayMode() == _T('e'))? TRUE: FALSE;
	BOOL bExpF8 = (pFloatFormat->GetFloat8DisplayMode() == _T('e'))? TRUE: FALSE;
	m_cCheckF4Display.SetCheck (bExpF4);
	m_cCheckF8Display.SetCheck(bExpF8);
}


BOOL CuIeaPPage1::OnInitDialog() 
{
	USES_CONVERSION;
	try
	{
		CRect r;
		CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
		CaIEAInfo& data = pParent->GetData();
		CaIeaDataPage1& dataPage1 = data.m_dataPage1;
		CPropertyPage::OnInitDialog();

		m_imageList.Create (IDB_INGRESOBJECT16x16, 16, 0, RGB(255,0,255));
		m_cTreeIngresDatabase.SetImageList (&m_imageList, TVSIL_NORMAL);
		m_cButtonLoad.SetIcon(m_hIcon);
		m_cButtonSqlAssistant.SetIcon(m_hIconAssistant);

		FillComboFormat();

		CaIexportTree& ieaTree = m_ieaTree; 
		if (!ieaTree.Initialize())
			throw (int)0;
		ieaTree.SetFunctionQueryObject(NULL);

		HTREEITEM hRoot =  TVI_ROOT;
		ieaTree.RefreshData(&m_cTreeIngresDatabase, hRoot, NULL);
		CaFloatFormat& floatFormat = dataPage1.GetFloatFormat();
		m_cSpinF4Precision.SetRange(MINF4_PREC, MAXF4_PREC);
		m_cSpinF4Scale.SetRange(MINF4_SCALE, MAXF4_SCALE);
		m_cSpinF8Precision.SetRange(MINF8_PREC, MAXF8_PREC);
		m_cSpinF8Scale.SetRange(MINF8_SCALE, MAXF8_SCALE);
		UpdateFloatPreferences(&floatFormat);

		//
		// Handle command line parameters:
		IEASTRUCT* pParamStruct = pParent->GetInputParam();
		if (!pParamStruct)
			return TRUE;

		if (pParamStruct && pParamStruct->nSessionStart > 0)
		{
			CmtSessionManager* pSessionManager = ieaTree.GetSessionManager();
			ASSERT(pSessionManager);
			if (pSessionManager)
				pSessionManager->SetSessionStart(pParamStruct->nSessionStart + 1);
		}

		if (pParamStruct->wczNode[0])
		{
			CString strItem;
			CString strObject;
			CTreeCtrl& cTree = m_cTreeIngresDatabase;
			HTREEITEM hItem = cTree.GetRootItem();

			//
			// Select Node name:
			strObject = W2CT(pParamStruct->wczNode);
			while (hItem)
			{
				strItem = cTree.GetItemText (hItem);
				if (strItem.CompareNoCase(strObject) == 0)
				{
					cTree.SelectItem (hItem);
					cTree.Expand (hItem, TVE_EXPAND );
					hItem = cTree.GetChildItem(hItem);
					break;
				}

				hItem = cTree.GetNextSiblingItem(hItem);
			}
			
			//
			// Select and expand the folder of servers:
			if (pParamStruct->wczServerClass[0])
			{
				while (hItem)
				{
					CtrfItemData* pItemData = (CtrfItemData*)cTree.GetItemData (hItem);
					if (pItemData->GetTreeItemType() == O_FOLDER_NODE_SERVER)
					{
						cTree.SelectItem (hItem);
						cTree.Expand (hItem, TVE_EXPAND );
						hItem = cTree.GetChildItem(hItem);
						break;
					}

					hItem = cTree.GetNextSiblingItem(hItem);
				}
				//
				// Select Server Class:
				strObject = W2CT(pParamStruct->wczServerClass);
				while (hItem)
				{
					strItem = cTree.GetItemText (hItem);
					if (strItem.CompareNoCase(strObject) == 0)
					{
						cTree.SelectItem (hItem);
						cTree.Expand (hItem, TVE_EXPAND );
						hItem = cTree.GetChildItem(hItem);
						break;
					}

					hItem = cTree.GetNextSiblingItem(hItem);
				}
			}

			//
			// Select the Database:
			if (pParamStruct->wczDatabase[0])
			{
				strObject = W2CT(pParamStruct->wczDatabase);
				while (hItem)
				{
					strItem = cTree.GetItemText (hItem);
					if (strItem.CompareNoCase(strObject) == 0)
					{
						cTree.SelectItem (hItem);
						cTree.Expand (hItem, TVE_EXPAND );
						hItem = cTree.GetChildItem(hItem);
						break;
					}

					hItem = cTree.GetNextSiblingItem(hItem);
				}
			}
			//
			// Select the Statement:
			if (pParamStruct->lpbstrStatement)
			{
				strObject = W2CT(pParamStruct->lpbstrStatement);
				m_cEditSql.SetWindowText(strObject);
			}
			//
			// Export File name:
			if (pParamStruct->wczExportFile[0])
			{
				strObject = W2CT(pParamStruct->wczExportFile);
				m_cEditFile2BeCreated.SetWindowText(strObject);
			}

			//
			// Export Format:
			if (pParamStruct->wczExportFormat[0])
			{
				ExportedFileType expt = EXPORT_CSV;
				strObject = W2CT(pParamStruct->wczExportFormat);
				if (strObject.CompareNoCase (_T("csv")) == 0)
					expt = EXPORT_CSV;
				else
				if (strObject.CompareNoCase (_T("dbf")) == 0)
					expt = EXPORT_DBF;
				else
				if (strObject.CompareNoCase (_T("xml")) == 0)
					expt = EXPORT_XML;
				else
				if (strObject.CompareNoCase (_T("fixed")) == 0)
					expt = EXPORT_FIXED_WIDTH;
				else
				if (strObject.CompareNoCase (_T("copy")) == 0)
					expt = EXPORT_COPY_STATEMENT;
				//
				// Export Type:
				for (int i=0; i<m_cComboFormat.GetCount(); i++)
				{
					if (expt == (ExportedFileType)m_cComboFormat.GetItemData(i))
					{
						m_cComboFormat.SetCurSel(i);
						break;
					}
				}
			}
		}

		//
		// Specify the export parameter file ?
		if (pParamStruct->wczParamFile[0])
		{
			CString strItem = W2CT(pParamStruct->wczParamFile);
			if (_taccess(strItem, 0) != -1)
			{
				Loading(strItem);
			}
			else
			{
				// Load Export Parameters error.
				CString strMsg;
				AfxFormatString1 (strMsg, IDS_LOADPARAM_ERR_FILE_NOT_EXIST, (LPCTSTR)strItem);
				AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
			}
		}
		return TRUE;
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
		return FALSE;
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (CFileException* e)
	{
		MSGBOX_FileExceptionMessage (e);
		e->Delete();
	}
	catch(...)
	{
		//
		// Failed to initialize tables.
		AfxMessageBox(IDS_MSG_FAIL_TO_INITIALIZE_EXPORT_STEP1);
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuIeaPPage1::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	if (m_hIconAssistant)
		DestroyIcon(m_hIconAssistant);
	m_hIcon = NULL;
	CPropertyPage::OnDestroy();
}

BOOL CuIeaPPage1::OnSetActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;

	pParent->SetWizardButtons(PSWIZB_NEXT);
	TRACE1 ("Previous page = %d\n", pParent->GetPreviousPage ());
	return CPropertyPage::OnSetActive();
}

BOOL CuIeaPPage1::OnKillActive() 
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();

	pParent->SetPreviousPage (1);
	return CPropertyPage::OnKillActive();
}

LRESULT CuIeaPPage1::OnWizardNext() 
{
	CString strFile;
	CString strMsg;
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;

	try
	{
		CWaitCursor doWaitCursor;
		if (!CanGoNext())
			return E_FAIL;

		UINT nChange = CollectData(&data);

		pParent->RemovePage(1);
		if (dataPage1.GetExportedFileType() == EXPORT_CSV)
			pParent->AddPage(&(pParent->m_Page2Csv));
		else
			pParent->AddPage(&(pParent->m_Page2Common));

		CaIeaDataForLoadSave* pLoadData = data.GetLoadSaveData();
		if (pLoadData)
		{
			CaIeaDataPage1& dataPage1Loaded = pLoadData->m_dataPage1;
			CaQueryResult& resultLoaded = dataPage1Loaded.GetQueryResult();

			//
			// If parameters have been loaded, then we must execute the statement
			// to see if the result headers still matched the loaded ones:
			dataPage2.Cleanup();
			CaQueryResult& result = dataPage1.GetQueryResult();
			result.Cleanup();
			if (!IEA_FetchRows (&data, m_hWnd))
			{
				dataPage1.SetStatement(_T(""));
				return E_FAIL;
			}
			CTypedPtrList< CObList, CaColumn* >& lc = result.GetListColumn();
			CTypedPtrList< CObList, CaColumn* >& lcLoaded = resultLoaded.GetListColumn();
			if (lc.GetCount() == lcLoaded.GetCount())
			{
				BOOL bNotMatch = FALSE;
				POSITION p1, p2;
				p1 = lc.GetHeadPosition();
				p2 = lcLoaded.GetHeadPosition();
				while (p1 != NULL && p2 != NULL)
				{
					CaColumn* pCol = lc.GetNext(p1);
					CaColumn* pColLoaded = lcLoaded.GetNext(p2);
					if (!MatchColumn (pCol, pColLoaded))
					{
						bNotMatch = TRUE;
						break;
					}
				}

				if (!bNotMatch)
				{
					//
					// The queried columns still match the loaded ones.
					// Use the loaded preview layout...
					CTypedPtrList< CObList, CaColumnExport* >& lcLayout = result.GetListColumnLayout();
					while (!lcLayout.IsEmpty())
						delete lcLayout.RemoveHead();

					CTypedPtrList< CObList, CaColumnExport* >& lcLayoutLoaded = resultLoaded.GetListColumnLayout();
					while (!lcLayoutLoaded.IsEmpty())
					{
						CaColumnExport* pLayout = lcLayoutLoaded.RemoveHead();
						lcLayout.AddTail(pLayout);
					}
	
					CaIeaDataPage2& dataPage2Loaded = pLoadData->m_dataPage2;
					dataPage2 = dataPage2Loaded;
				}
			}
			data.CleanLoadSaveData();
		}
		else
		{
			if ((nChange & RESULT_SET) || (nChange & RESULT_EXPORTTYPE))
			{
				dataPage2.Cleanup();
				CaQueryResult& result = dataPage1.GetQueryResult();
				result.Cleanup();
				if (!IEA_FetchRows (&data, m_hWnd))
				{
					dataPage1.SetStatement(_T(""));
					return E_FAIL;
				}
			}
			/*
			else
			if (nChange & RESULT_EXPORTTYPE)
			{
				CaQueryResult& result = dataPage1.GetQueryResult();
				ConfigureLayout (dataPage1.GetExportedFileType(), result, NULL);
			}
			*/
		}
		CaQueryResult& resultRows = dataPage1.GetQueryResult();
		CPtrArray& arrayRecord = resultRows.GetListRecord();
		if (arrayRecord.GetSize() == 0)
		{
			AfxMessageBox (IDS_MSG_NOROWS_TO_EXPORT);
			dataPage1.SetStatement(_T(""));
			return E_FAIL;
		}

		return CPropertyPage::OnWizardNext();
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		AfxMessageBox(strErr, MB_ICONEXCLAMATION|MB_OK);
		e->Delete();
		return E_FAIL;
	}
	catch(...)
	{
		//
		// Failed to construct file format identification.
	}
	dataPage1.SetStatement(_T(""));
	return E_FAIL;
}

void CuIeaPPage1::OnButtonFile2BeCreated() 
{
	BOOL b2AppendExt = FALSE;
	CString strFullName, strExt;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY /*| OFN_OVERWRITEPROMPT*/,
	    _T("CSV File (*.csv)|*.csv|dBASE File (*.dbf)|*.dbf|Text File (*.txt)|*.txt|XML file (*.xml)|*.xml||"));
    
	 /*Added as fix for bug 112435*/
 
	TCHAR* pUProfile = _tgetenv(_T("USERPROFILE"));
	if (pUProfile != NULL)
	{
	      strcat(pUProfile, "\\Documents");
	      if (access(pUProfile, 00) == 0)	
		      dlg.m_ofn.lpstrInitialDir=pUProfile;
	}
	else
	     dlg.m_ofn.lpstrInitialDir=NULL;

	int nSel = m_cComboFormat.GetCurSel();
	if (nSel != CB_ERR)
	{
		ExportedFileType exptype = (ExportedFileType)m_cComboFormat.GetItemData(nSel);
		switch (exptype)
		{
		case EXPORT_CSV:
			dlg.m_ofn.nFilterIndex = 1;
			break;
		case EXPORT_DBF:
			dlg.m_ofn.nFilterIndex = 2;
			break;
		case EXPORT_FIXED_WIDTH:
		case EXPORT_COPY_STATEMENT:
			dlg.m_ofn.nFilterIndex = 3;
			break;
		case EXPORT_XML:
			dlg.m_ofn.nFilterIndex = 4;
			break;
		default:
			break;
		}
	}

	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	strExt = dlg.GetFileExt();
	ExportedFileType nExport = VerifyFileFormat (strFullName, strExt, dlg.m_ofn.nFilterIndex);
	m_cEditFile2BeCreated.SetWindowText(strFullName);
	
	for (int i=0; i<m_cComboFormat.GetCount(); i++)
	{
		if (nExport == (ExportedFileType)m_cComboFormat.GetItemData(i))
		{
			m_cComboFormat.SetCurSel(i);
			UpdateExponentialFormat();
			break;
		}
	}
}

static HRESULT GetInterfacePointer(IUnknown* pObj, REFIID riid, PVOID* ppv)
{
	HRESULT hError = NOERROR;

	*ppv=NULL;

	if (NULL != pObj)
	{
		hError = pObj->QueryInterface(riid, ppv);
	}
	return hError;
}

void CuIeaPPage1::OnButtonSqlAssistant() 
{
	USES_CONVERSION;
	IUnknown*   pAptSqlAssistant = NULL;
	ISqlAssistant* pSqlAssistant;
	BOOL bSelectedDatabase = FALSE;
	HRESULT hError = NOERROR;

	try
	{
		CString strDatabase;
		UINT nDbFlag = 0;
		CaLLQueryInfo* pQueryInfo = NULL;
		CtrfItemData* pSelected = NULL;
		HTREEITEM hItemSelected = m_cTreeIngresDatabase.GetSelectedItem();
		if (hItemSelected)
		{
			pSelected = (CtrfItemData*)m_cTreeIngresDatabase.GetItemData (hItemSelected);
			if (pSelected && pSelected->IsKindOf(RUNTIME_CLASS(CaIeaDatabase)))
			{
				bSelectedDatabase = TRUE;
				CtrfDatabase* ptrfDatabase = (CtrfDatabase*)pSelected;
				CaDatabase& database = ptrfDatabase->GetObject();
				strDatabase = database.GetName();
				nDbFlag = database.GetStar();
				pQueryInfo = ptrfDatabase->GetQueryInfo();
			}
		}

		if (!bSelectedDatabase)
		{
			//
			// Please select a database.
			AfxMessageBox (IDS_MSG_SELECT_DATABASE, MB_ICONEXCLAMATION|MB_OK);
			return;
		}
		ASSERT(pQueryInfo && pSelected);
		if (!pQueryInfo || !pSelected)
			return;

		hError = CoCreateInstance(
			CLSID_SQLxASSISTANCT,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&pAptSqlAssistant);

		if (SUCCEEDED(hError))
		{
			CString strSessionDescription;
			strSessionDescription.Format (_T("%s / Sql Assistant"), (LPCTSTR)theApp.GetSessionManager().GetDescription());
			hError = GetInterfacePointer(pAptSqlAssistant, IID_ISqlAssistant, (LPVOID*)&pSqlAssistant);
			if (SUCCEEDED(hError))
			{
				BSTR bstrStatement = NULL;
				SQLASSISTANTSTRUCT iiparam;
				memset(&iiparam, 0, sizeof(iiparam));
				wcscpy (iiparam.wczNode, T2W((LPTSTR)(LPCTSTR)pQueryInfo->GetNode()));
				wcscpy (iiparam.wczDatabase, T2W((LPTSTR)(LPCTSTR)strDatabase));
				iiparam.nDbFlag = nDbFlag;
				iiparam.nSessionStart = theApp.m_sessionManager.GetSessionStart() + 100;
				iiparam.nActivation   = 0;
				wcscpy (iiparam.wczSessionDescription, T2W((LPTSTR)(LPCTSTR)strSessionDescription));

				POINT p;
				p.x = 40;
				p.y = 40;
				iiparam.lpPoint = &p;

				hError = pSqlAssistant->SqlAssistant (m_hWnd, &iiparam, &bstrStatement);
				if (SUCCEEDED(hError) && bstrStatement)
				{
					CString strStatement = W2T(bstrStatement);
					SysFreeString(bstrStatement);
					strStatement.TrimLeft();
					strStatement.TrimRight();
					if (!strStatement.IsEmpty())
					{
						m_cEditSql.ReplaceSel(strStatement, TRUE);
					}
				}
				pSqlAssistant->Release();
			}
			if (!SUCCEEDED(hError))
			{
				AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQLASSISTANT, MB_ICONEXCLAMATION|MB_OK);
			}
			
			pAptSqlAssistant->Release();
			CoFreeUnusedLibraries();
		}
		else
		{
			AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SQLASSISTANT, MB_ICONEXCLAMATION|MB_OK);
		}
	}
	catch(...)
	{

	}
}

void CuIeaPPage1::OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CuIeaPPage1::OnItemexpandedTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	if (!m_bAllowExpandTree)
	{
		MessageBeep(0xFFFF);
		*pResult = 1;
		return;
	}

	CTreeCtrl& cTree = m_cTreeIngresDatabase;
	try
	{
		CtrfItemData* pItem = NULL;
		HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
		if (pNMTreeView->action & TVE_COLLAPSE)
		{
			//
			// Collapseing:
			pItem = (CtrfItemData*)cTree.GetItemData (hExpand);
			pItem->Collapse (&cTree, hExpand);
		}
		else
		if (pNMTreeView->action & TVE_EXPAND)
		{
			//
			// Expanding:
			pItem = (CtrfItemData*)cTree.GetItemData (hExpand);
			if (!pItem)
				return;
			//
			// Expanding:
			CWaitCursor doWaitCursor; 
			pItem->Expand (&cTree, hExpand);
		}
	}
	catch(...)
	{
		*pResult = 1;
		TRACE0 ("System error: CvViewLeft::OnItemexpanded\n");
	}
	
	*pResult = 0;
}

BOOL CuIeaPPage1::CanGoNext()
{
	// 
	// 1) Must select a database.
	// 2) Must specify file to be created.
	// 3) Must verify if the file path exists or not.
	// 4) The file extention and the export format must be compatible
	// 5) Must specify the select statement

	// 
	// 1) Must select a database.
	BOOL bSelectDatabase = FALSE;
	HTREEITEM hItemSelected = m_cTreeIngresDatabase.GetSelectedItem();
	if (hItemSelected)
	{
		CtrfItemData* pSelected = NULL;
		pSelected = (CtrfItemData*)m_cTreeIngresDatabase.GetItemData (hItemSelected);
		if (pSelected && pSelected->IsKindOf(RUNTIME_CLASS(CaIeaDatabase)))
			bSelectDatabase = TRUE;
	}
	if (!bSelectDatabase)
	{
		AfxMessageBox (IDS_MSG_SELECT_DATABASE, MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}

	//
	// 2) Must specify file to be created.
	CString strFile = _T("");
	m_cEditFile2BeCreated.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	if (strFile.IsEmpty())
	{
		AfxMessageBox (IDS_MSG_SPECIFY_EXPORT_FILE, MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}
	if (_taccess(strFile, 0) != -1)
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_OVERWRITE_FILE, (LPCTSTR)strFile);
		int nAnswher = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
		if (nAnswher != IDYES)
			return FALSE;
	}

	//
	// 3) Must verify if the file path exists or not.
	CString strFilePath = _T("");
	strFile.Replace("/","\\");
	int nBackSlash = strFile.ReverseFind('\\');
	if (nBackSlash == -1)
	{
		AfxMessageBox(IDS_MSG_INVALID_FILE_PATH, MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}else
	{
		strFilePath = strFile.Left(nBackSlash);
		if (_taccess(strFilePath, 0) == -1)
		{
			AfxMessageBox(IDS_MSG_INVALID_FILE_PATH, MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
	}

	//
	// 4) The file extention and the export format must be compatible
	int nDot = strFile.ReverseFind(_T('.'));
	if (nDot != -1)
	{
		CString strExt = strFile.Mid(nDot+1);
		int nSel = m_cComboFormat.GetCurSel();
		if (nSel == CB_ERR)
		{
			AfxMessageBox (IDS_MSG_WRONG_EXTxFORMAT, MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
		ExportedFileType nFormat = (ExportedFileType)m_cComboFormat.GetItemData(nSel);


		if (strExt.CompareNoCase (_T("csv")) == 0 && nFormat != EXPORT_CSV)
		{
			AfxMessageBox (IDS_MSG_WRONG_EXTxFORMAT, MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
		else
		if (strExt.CompareNoCase (_T("dbf")) == 0 && nFormat != EXPORT_DBF)
		{
			AfxMessageBox (IDS_MSG_WRONG_EXTxFORMAT, MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
		else
		if (strExt.CompareNoCase (_T("xml")) == 0 && nFormat != EXPORT_XML)
		{
			AfxMessageBox (IDS_MSG_WRONG_EXTxFORMAT, MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
	}

	//
	// 5) Must specify the select statement
	BOOL bHasStatement = FALSE;
	CString strStatement = _T("");
	m_cEditSql.GetWindowText(strStatement);
	strStatement.TrimLeft();
	strStatement.TrimRight();
	strStatement.MakeLower();
	if (!strStatement.IsEmpty())
	{
		if (strStatement.Find(_T("select"), 0) == 0)
			bHasStatement = TRUE;
	}
	if (!bHasStatement)
	{
		AfxMessageBox (IDS_MSG_SPECIFY_SELECT, MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}
	return TRUE;
}


UINT CuIeaPPage1::CollectData(CaIEAInfo* pData)
{
	CString strFile;
	CString strItem;
	CaIeaDataPage1& dataPage1 = pData->m_dataPage1;
	UINT nChange = 0;

	//
	// File to be created:
	m_cEditFile2BeCreated.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	dataPage1.SetFile2BeExported(strFile);

	//
	// Export Format
	int nSel = m_cComboFormat.GetCurSel();
	if (nSel != CB_ERR)
	{
		ExportedFileType nFormat = (ExportedFileType)m_cComboFormat.GetItemData(nSel);
		if (nFormat != dataPage1.GetExportedFileType())
			nChange |= RESULT_EXPORTTYPE;
		dataPage1.SetExportedFileType(nFormat);
	}
	//
	// Float Format
	CaFloatFormat& fl = dataPage1.GetFloatFormat();
	int  nPrec, nScale, nExpo;
	TCHAR cExpo = _T('n');
	m_cEditF4Precision.GetWindowText(strItem);
	nPrec = _ttoi(strItem);
	m_cEditF4Scale.GetWindowText(strItem);
	nScale = _ttoi(strItem);
	nExpo = m_cCheckF4Display.GetCheck();
	if (dataPage1.GetExportedFileType() == EXPORT_DBF)
		nExpo = 0;

	cExpo = nExpo? _T('e'): _T('n');
	if (fl.GetFloat4Width() != nPrec)
		nChange |= RESULT_SET;
	if (fl.GetFloat4Decimal() != nScale)
		nChange |= RESULT_SET;
	if (fl.GetFloat4DisplayMode() != cExpo)
		nChange |= RESULT_SET;

	fl.SetFloat4Format(nPrec, nScale, cExpo);
	m_cEditF8Precision.GetWindowText(strItem);
	nPrec = _ttoi(strItem);
	m_cEditF8Scale.GetWindowText(strItem);
	nScale = _ttoi(strItem);
	nExpo = m_cCheckF8Display.GetCheck();
	if (dataPage1.GetExportedFileType() == EXPORT_DBF)
		nExpo = 0;

	cExpo = nExpo? _T('e'): _T('n');
	if (fl.GetFloat8Width() != nPrec)
		nChange |= RESULT_SET;
	if (fl.GetFloat8Decimal() != nScale)
		nChange |= RESULT_SET;
	if (fl.GetFloat8DisplayMode() != cExpo)
		nChange |= RESULT_SET;
	fl.SetFloat8Format(nPrec, nScale, cExpo);

	//
	// The select statement:
	m_cEditSql.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (strItem.CompareNoCase (dataPage1.GetStatement()) != 0)
		nChange |= RESULT_SET;
	dataPage1.SetStatement(strItem);

	//
	// Database Objects (Node, Server, Database):
	HTREEITEM hSelected = m_cTreeIngresDatabase.GetSelectedItem ();
	if (hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIngresDatabase.GetItemData (hSelected);
		if (pSelected->IsKindOf(RUNTIME_CLASS(CaIeaDatabase)))
		{
			CtrfDatabase* ptrfDatabase = (CtrfDatabase*)pSelected;
			CaDatabase& database = ptrfDatabase->GetObject();
			CaLLQueryInfo* pQueryInfo = ptrfDatabase->GetQueryInfo();
			ASSERT(pQueryInfo);
			if (!pQueryInfo)
				throw (int)0;

			//
			// Setup the Ingres Objects:
			if (pQueryInfo->GetNode().CompareNoCase(dataPage1.GetNode()) != 0)
				nChange |= RESULT_SET;
			if (pQueryInfo->GetDatabase().CompareNoCase(dataPage1.GetDatabase()) != 0)
				nChange |= RESULT_SET;
			dataPage1.SetNode(pQueryInfo->GetNode());
			dataPage1.SetDatabase(pQueryInfo->GetDatabase());
			
		}
	}

	//
	// Identify the gateway:
	CString strGateway = _T("");
	hSelected = m_cTreeIngresDatabase.GetSelectedItem ();
	CtrfItemData* pGateway = NULL;
	while(hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIngresDatabase.GetItemData (hSelected);
		if (pSelected->GetTreeItemType() == O_NODE_SERVER)
		{
			pGateway = pSelected;
			break;
		}
		hSelected = m_cTreeIngresDatabase.GetParentItem(hSelected);
	}
	
	if (pGateway)
	{
		ASSERT (pGateway->IsKindOf(RUNTIME_CLASS(CtrfNodeServer)));
		if (pGateway->IsKindOf(RUNTIME_CLASS(CtrfNodeServer)))
		{
			CtrfNodeServer* ptrfNodeServer = (CtrfNodeServer*)pGateway;
			strGateway = ptrfNodeServer->GetObject().GetName();
		}
	}
	if (dataPage1.GetServerClass().CompareNoCase(strGateway) != 0)
		nChange |= RESULT_SET;

	dataPage1.SetServerClass(strGateway);
	BOOL bGateway = (!strGateway.IsEmpty() && !INGRESII_IsIngresGateway(strGateway));
	dataPage1.SetGatewayDatabase(bGateway);

	//
	// Construct the tree item path:
	CStringArray& itemPath = dataPage1.GetTreeItemPath();
	itemPath.RemoveAll();
	CTreeCtrl& cTree = m_cTreeIngresDatabase;
	hSelected = cTree.GetSelectedItem ();
	if (hSelected)
	{
		strItem = cTree.GetItemText (hSelected);
		itemPath.Add (strItem);
		hSelected = cTree.GetParentItem(hSelected);
		while (hSelected)
		{
			strItem = cTree.GetItemText (hSelected);
			if (!strItem.IsEmpty())
				itemPath.Add (strItem);

			hSelected = cTree.GetParentItem(hSelected);
		}
	}

	//
	// Check to see if the current parameters matches the loaded parameters:
	CaIeaDataForLoadSave* pLoadData = pData->GetLoadSaveData();
	if (pLoadData)
	{
		//
		// Data has been loaded ...
		CaIeaDataPage1& dataPage1Loaded = pLoadData->m_dataPage1;
		if (dataPage1.GetFile2BeExported().CompareNoCase (dataPage1Loaded.GetFile2BeExported()) != 0)
		{
			pData->CleanLoadSaveData();
			return nChange;
		}

		if (dataPage1.GetExportedFileType() != dataPage1Loaded.GetExportedFileType())
		{
			pData->CleanLoadSaveData();
			return nChange;
		}

		if (dataPage1.GetNode().CompareNoCase(dataPage1Loaded.GetNode()) != 0)
		{
			pData->CleanLoadSaveData();
			return nChange;
		}
		if (dataPage1.GetDatabase().CompareNoCase(dataPage1Loaded.GetDatabase()) != 0)
		{
			pData->CleanLoadSaveData();
			return nChange;
		}
		if (dataPage1.GetServerClass().CompareNoCase(dataPage1Loaded.GetServerClass()) != 0)
		{
			pData->CleanLoadSaveData();
			return nChange;
		}
	}

	return nChange;
}

void CuIeaPPage1::UpdateExponentialFormat()
{
	int nSel = m_cComboFormat.GetCurSel();
	if (nSel != CB_ERR)
	{
		ExportedFileType nExpFmt = (ExportedFileType)m_cComboFormat.GetItemData(nSel);
		if (nExpFmt == EXPORT_DBF)
		{
			m_cCheckF4Display.ShowWindow(SW_HIDE);
			m_cCheckF8Display.ShowWindow(SW_HIDE);
		}
		else
		{
			m_cCheckF4Display.ShowWindow(SW_SHOW);
			m_cCheckF8Display.ShowWindow(SW_SHOW);
		}
	}
}

void CuIeaPPage1::OnSelchangeComboFileFormat() 
{
	UpdateExponentialFormat();
}

void CuIeaPPage1::OnChangeEditF4Precision() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	F4_OnChangeEditPrecision(&m_cEditF4Precision);
}

void CuIeaPPage1::OnChangeEditF4Scale() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F4_OnChangeEditScale(&m_cEditF4Scale);
}

void CuIeaPPage1::OnChangeEditF8Precision() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F8_OnChangeEditPrecision(&m_cEditF8Precision);
}

void CuIeaPPage1::OnChangeEditF8Scale() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F8_OnChangeEditScale(&m_cEditF8Scale);
}
