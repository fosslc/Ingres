/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iappage1.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : STEP 1 of Import assistant
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 24-Jul-2001 (uk$so01)
**    SIR #105322, Do not show the item "<new table>" when invoking
**    in context to select the specific table given in the IIASTRUCT::wczTable parameter.
** 25-Oct-2001 (noifr01)
**    (bug 105484) provide an error message when trying to import a dbf
**    file under UNIX
** 31-Oct-2001 (hanje04)
**    Replaced out of date 'CString& name=' constructs with CString name() as
**    they was causing problems on Linux.
** 17-Jan-2002 (uk$so01)
**    (bug 106844). Add BOOL m_bStopScanning to indicate if we really stop
**     scanning the file due to the limitation.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 04-Apr-2002 (uk$so01)
**    BUG #107505, import column that has length 0, record length != sum (column's length)
**      06-Feb-2003 (hanch04)
**          Don't do animation if built using MAINWIN
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Cleanup + make code homogeneous.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 14-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should not be in the cache
**    for IIA & IEA.
** 25-Jul-2007 (drivi01)
**    Move a default directory to USERPROFILE space. When we're trying 
**    to import a file it will open by default to USERPROFILE.
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
** 10-Mar-2010 (drivi01) SIR 123397
**    Control "Specify Custom Separator" option with a checkbox.
**    Add routines for checking empty files.
**    Switch "First Line Header" option with "Skip lines" options,
**    allow header option to set skip lines at 1.  If skip lines is
**    increased to 2 or above, the headers become unchecked as we're 
**    no longer just skipping headers.
** 07-Sep-2010 (drivi01)
**    Fix SEGV on some machines. Update routines to consistently
**    use tchar functions.  Do not use pointer from _tgetenv
**    as a directory buffer, copy it to another variable first
**    and use a new buffer.  Otherwise routines may end up
**    accessing unallocated memory in the buffer.
**/

#include "stdafx.h"
#include "resource.h"
#include "rctools.h"
#include "iappage1.h"
#include "constdef.h"
#include "rcdepend.h"
#include "iapsheet.h"
#include "taskanim.h"
#include "usrexcep.h"
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#include "rctools.h"  // Resource symbols of rctools.rc
#include "ingobdml.h" // Low level query objects
#include "vnodedml.h" // Low level query nodes
#define _SPECIAL_ONECOLUMN


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static BOOL handlerQueryNode(CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	USES_CONVERSION;
	CaDatabase* pItem = NULL;
	IIASTRUCT* pData = theApp.GetInputParameter();

	HRESULT hErr = VNODE_QueryNode(listObject);
	if (hErr == NOERROR)
	{
		CString strNode = W2CT(pData->wczNode);

		POSITION p, pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaNode* pNode = (CaNode*)listObject.GetNext(pos);
			if (pNode->GetName().CompareNoCase(strNode) != 0)
			{
				listObject.RemoveAt (p);
				delete pNode;
			}
		}
	}
	return TRUE;
}

static BOOL handlerQueryServer(CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	USES_CONVERSION;
	CaNodeServer* pItem = NULL;
	IIASTRUCT* pData = theApp.GetInputParameter();
	CString strNodeServer = W2CT(pData->wczServerClass);

	if (!strNodeServer.IsEmpty())
	{
		pItem = new CaNodeServer (pNode->GetName(), strNodeServer, FALSE);
		listObject.AddTail (pItem);
	}

	return TRUE;
}

static BOOL handlerQueryDatabase(CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	USES_CONVERSION;
	CaDatabase* pItem = NULL;
	IIASTRUCT* pData = theApp.GetInputParameter();
	CString strDatabase = W2CT(pData->wczDatabase);
	
	if (!strDatabase.IsEmpty())
	{
		pItem = new CaDatabase (strDatabase, _T(""));
		listObject.AddTail (pItem);
	}

	return TRUE;
}

static BOOL handlerQueryTable(CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	USES_CONVERSION;
	CaTable* pItem = NULL;
	IIASTRUCT* pData = theApp.GetInputParameter();
	CString strTable = W2CT(pData->wczTable);
	CString strTableOwner = W2CT(pData->wczTableOwner);

	if (!strTable.IsEmpty())
	{
		pItem = new CaTable (strTable, strTableOwner);
		listObject.AddTail (pItem);
	}
	else
	{
		CaLLQueryInfo info;
		info.SetObjectType(OBT_TABLE);
		info.SetNode (W2CT(pData->wczNode));
		info.SetDatabase(W2CT(pData->wczDatabase));
		info.SetServerClass (W2CT(pData->wczServerClass));
		info.SetIncludeSystemObjects(FALSE);
		info.SetIndependent(TRUE);
		CmtSessionManager* pSessionManager = &theApp.m_sessionManager;
		ASSERT (pSessionManager);
		if (!pSessionManager)
			return FALSE;
		BOOL bOk = INGRESII_llQueryObject (&info, listObject, pSessionManager);
		if (bOk)
		{
			CaTable* pDummyTable = new CaTable (_T("<new table>"), _T(""));
			listObject.AddHead(pDummyTable);
		}

		return bOk;
	}

	return TRUE;
}


static BOOL CALLBACK UserQueryObject(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CmtSessionManager* pSmgr)
{
	HRESULT hErr = NOERROR;
	BOOL bOk = TRUE;
	CaNode node;

	try
	{
		ASSERT (pInfo);
		if (!pInfo)
			return FALSE;
		switch (pInfo->GetObjectType())
		{
		case OBT_VIRTNODE:
			bOk = handlerQueryNode(listObject);
			break;
		case OBT_VNODE_SERVERCLASS:
			node.SetName(pInfo->GetNode());
			bOk = handlerQueryServer(&node, listObject);
			break;

		case OBT_DATABASE:
			bOk = handlerQueryDatabase(listObject);
			break;
		case OBT_TABLE:
			bOk = handlerQueryTable(listObject);
			break;
		default:
			ASSERT (FALSE);
			bOk = FALSE;
			break;
		}
	}
	catch (...)
	{
		bOk = FALSE;
		while (!listObject.IsEmpty())
			delete listObject.RemoveHead();
	}

	return bOk;
}


IMPLEMENT_DYNCREATE(CuPPage1, CPropertyPage)
CuPPage1::CuPPage1() : CPropertyPage(CuPPage1::IDD)
{
	//{{AFX_DATA_INIT(CuPPage1)
	//}}AFX_DATA_INIT
	m_bitmap.SetBitmpapId (IDB_IMPORTAS_1);
	m_bitmap.SetCenterVertical(TRUE);
	m_pStatusFile2BeImported = NULL;
	m_pStatusFileParameter = NULL;
	m_nCurrentCheckMatchFileAndTable = 1;

	m_hIcon = theApp.LoadIcon (IDI_ICON_LOAD);

	CString strStep;
	strStep.LoadString (IDS_IIA_STEP_1);
	m_strPageTitle.LoadString(AFX_IDS_APP_TITLE);
	m_strPageTitle += theApp.m_strInstallationID;
	m_strPageTitle += strStep;
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = (LPCTSTR)m_strPageTitle;

	m_bAllowExpandTree = TRUE;
	m_bAllowSelChangeTree = TRUE;
	theApp.SetInputParameter(NULL);
	m_bInterrupted = FALSE;

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPPage1::~CuPPage1()
{
	if (m_pStatusFile2BeImported)
		delete m_pStatusFile2BeImported;
	if (m_pStatusFileParameter)
		delete m_pStatusFileParameter;
}

void CuPPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPPage1)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckFirstLineHeader);
	DDX_Control(pDX, IDC_BUTTON2, m_cButton3Periods);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonLoad);
	DDX_Control(pDX, IDC_EDIT5, m_cEditLineCountToSkip);
	DDX_Control(pDX, IDC_SPIN1, m_cSpin1);
	DDX_Control(pDX, IDC_CHECK3, m_cCheckMatchNumberAndOrder);
	DDX_Control(pDX, IDC_EDIT4, m_cEditMoreSeparator);
	DDX_Control(pDX, IDC_EDIT3, m_cEditKB2Scan);
	DDX_Control(pDX, IDC_STATICFILE_SIZE, m_cStaticFileSize);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_EDIT1, m_cEditTable2Create);
	DDX_Control(pDX, IDC_EDIT2, m_cEditFile2BeImported);
	DDX_Control(pDX, IDC_TREE1, m_cTreeIgresTable);
	DDX_Control(pDX, IDC_STATIC_IMAGE1, m_bitmap);
	DDX_Control(pDX, IDC_CHECK_CUSTOM_SEPARATOR, m_cCustomSeparator);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CuPPage1)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnItemexpandingTreeIngresTable)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonParameterFile)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2ImportFile)
	ON_BN_CLICKED(IDC_CHECK_CUSTOM_SEPARATOR, OnButtonCustomSeparator)
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, OnSelchangedTreeIngresTable)
	ON_BN_CLICKED(IDC_CHECK3, OnCheckFileMatchTable)
	ON_BN_CLICKED(IDC_CHECK1, OnCheckFirstLineHeader)
	ON_EN_KILLFOCUS(IDC_EDIT2, OnKillfocusEditFile2BeImported)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE1, OnSelchangingTreeIngresTable)
	ON_EN_CHANGE(IDC_EDIT5, OnChangeEditSkipLine)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CuPPage1::OnInitDialog() 
{
	USES_CONVERSION;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CPropertyPage::OnInitDialog();
	m_imageList.Create (IDB_INGRESOBJECT16x16, 16, 0, RGB(255,0,255));
	m_cTreeIgresTable.SetImageList (&m_imageList, TVSIL_NORMAL);

	m_cButtonLoad.SetIcon(m_hIcon);
	try
	{
		CRect r;
		//
		// Place the button ... for file to be imported to be vertically centered
		// within the height of editctrl of file to be imported:
		m_cEditFile2BeImported.GetWindowRect (r);
		ScreenToClient(r);
		int nMid = r.top + r.Height()/2;
		m_cButton3Periods.GetWindowRect (r);
		ScreenToClient(r);
		int n3pH = r.Height();
		r.top    = nMid - n3pH/2;
		m_cButton3Periods.SetWindowPos (NULL, r.left, r.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

		//
		// Place the button load to fit the right margin:
		m_cTreeIgresTable.GetWindowRect (r);
		ScreenToClient(r);
		int nRightMargin = r.right-1;
		m_cButtonLoad.GetWindowRect (r);
		ScreenToClient(r);
		if (r.right > nRightMargin || r.right < nRightMargin)
		{
			int nNewPosX = nRightMargin - r.Width();
			m_cButtonLoad.SetWindowPos (NULL, nNewPosX, r.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}
		CWnd* pStatic = GetDlgItem (IDC_STATIC2);
		if (pStatic && IsWindow (pStatic->m_hWnd))
		{
			m_cButtonLoad.GetWindowRect (r);
			ScreenToClient(r);
			int nNewPosX = r.left - 4;
			
			pStatic->GetWindowRect (r);
			ScreenToClient(r);
			nNewPosX -= r.Width();
			pStatic->SetWindowPos (NULL, nNewPosX, r.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}

		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		CaIeaTree& ieaTree = dataPage1.GetTreeData();
		if (!ieaTree.Initialize())
			throw (int)0;
		ieaTree.SetFunctionQueryObject(NULL);
		IIASTRUCT* pParamStruct = pParent->GetInputParam();
		if (pParamStruct && pParamStruct->nSessionStart > 0)
		{
			CmtSessionManager* pSessionManager = ieaTree.GetSessionManager();
			ASSERT(pSessionManager);
			if (pSessionManager)
				pSessionManager->SetSessionStart(pParamStruct->nSessionStart + 1);
		}

		if (pParamStruct && pParamStruct->bStaticTreeData)
		{
			//
			// Objects in the tree control are from the input parameters:
			theApp.SetInputParameter(pParamStruct);
			ieaTree.SetFunctionQueryObject(UserQueryObject);
			m_cButtonLoad.EnableWindow (!pParamStruct->bDisableLoadParam);
		}
		HTREEITEM hRoot =  TVI_ROOT;
		ieaTree.RefreshData(&m_cTreeIgresTable, hRoot, NULL);
		
		m_cSpin1.SetRange (0, 100);

		//
		// ANSI/OEM
		int nSelected = (dataPage1.GetCodePage() == 0)? IDC_RADIO1: IDC_RADIO2;
		CheckRadioButton (IDC_RADIO1, IDC_RADIO2, nSelected);

		//
		// Number and order of columns of the file expected to match
		// exactly those of the Ingres Table ?
		m_cCheckMatchNumberAndOrder.SetCheck(dataPage1.GetFileMatchTable());
		//
		// First Line as Header:
		m_cCheckFirstLineHeader.SetCheck(FALSE);

		if (!pParamStruct)
			return TRUE;
		//
		// Setup data:
		BOOL bLoadError = FALSE;
		if (pParamStruct->wczNode[0])
		{
			CString strItem;
			CString strObject;
			CTreeCtrl& cTree = m_cTreeIgresTable;
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

				//
				// Select the Table:
				if (pParamStruct->wczTable[0])
				{
					strObject = W2CT(pParamStruct->wczTable);
					CString strOwner = W2CT(pParamStruct->wczTableOwner);
					while (hItem)
					{
						CtrfItemData* pItemData = (CtrfItemData*)cTree.GetItemData (hItem);
						CaDBObject* pObj = pItemData->GetDBObject();

						if (strObject.CompareNoCase(pObj->GetName()) == 0)
						{
							if (strOwner.IsEmpty())
							{
								cTree.SelectItem (hItem);
								cTree.Expand (hItem, TVE_EXPAND );
								hItem = cTree.GetChildItem(hItem);
								break;
							}
							else
							{
								if (pObj->GetOwner().CompareNoCase(strOwner) == 0)
								{
									cTree.SelectItem (hItem);
									cTree.Expand (hItem, TVE_EXPAND );
									hItem = cTree.GetChildItem(hItem);
									break;
								}
							}
						}

						hItem = cTree.GetNextSiblingItem(hItem);
					}
				}
			}
		}

		//
		// Specify the import parameter file ?
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

			if (pParamStruct->wczNewTable[0])
			{
				m_cEditTable2Create.SetWindowText (W2CT(pParamStruct->wczNewTable));
			}
		}

		if (pParamStruct && ieaTree.GetPfnUserQueryObject())
		{
			m_bAllowExpandTree = FALSE;
			m_bAllowSelChangeTree = !pParamStruct->bDisableTreeControl;
		}
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
		return FALSE;
	}
	catch(...)
	{
		//
		// Failed to initialize tables.
		CString strMsg;
		strMsg.LoadString(IDS_FAIL_TO_INITIALIZE_TABLES);
		AfxMessageBox(strMsg);
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuPPage1::OnItemexpandingTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
	if (!m_bAllowExpandTree)
	{
		MessageBeep(0xFFFF);
		*pResult = 1;
		return;
	}

	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;
	CTreeCtrl& cTree = m_cTreeIgresTable;
	try
	{
		CtrfItemData* pItem = NULL;
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
}

void CuPPage1::Loading(LPCTSTR lpszFile)
{
	CString strMsg;
	CString strFullName =lpszFile; 
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;

	data.LoadData (strFullName);
	dataPage1.SetFileImportParameter(strFullName);
	//
	// Setup data:
	BOOL bLoadError = FALSE;
	m_nCurrentCheckMatchFileAndTable = dataPage1.GetFileMatchTable();
	//
	// Number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	m_cCheckMatchNumberAndOrder.SetCheck (dataPage1.GetFileMatchTable());

	//
	// The name of the file to be imported:
	m_cEditFile2BeImported.SetWindowText (dataPage1.GetFile2BeImported());
	//
	// Imported file size:
	strFullName = dataPage1.GetFile2BeImported();
	CString strSize;
	if (_taccess(strFullName, 0) != -1)
	{
		DWORD dwFileSize = File_GetSize(strFullName);
		strSize.Format (_T("%d"), dwFileSize);
		m_cStaticFileSize.SetWindowText (strSize);
	}
	else
	{
		bLoadError = TRUE;
		// Load Import Parameters error.\nThe file to import does not exist.
		CString strMsg;
		AfxFormatString1 (strMsg, IDS_LOADPARAM_ERR_FILE_NOT_EXIST, (LPCTSTR)strFullName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}

	//
	// ANSI/OEM
	int nSelected = (dataPage1.GetCodePage() == 0)? IDC_RADIO1: IDC_RADIO2;
	CheckRadioButton (IDC_RADIO1, IDC_RADIO2, nSelected);
	//
	// Number of KB to scan:
	strSize = _T("");
	if (dataPage1.GetKBToScan() > 0)
		strSize.Format (_T("%d"), dataPage1.GetKBToScan());
	m_cEditKB2Scan.SetWindowText (strSize);
	//
	// Number of line to skip:
	strSize.Format (_T("%d"), dataPage1.GetLineCountToSkip());
	m_cEditLineCountToSkip.SetWindowText(strSize);
	//
	// Additional Separators:
	m_cEditMoreSeparator.SetWindowText (dataPage1.GetCustomSeparator());

	//
	// New Table to be created:
	m_cEditTable2Create.SetWindowText (dataPage1.GetNewTable());
	//
	// Select Object in the tree:
	CString strItem;
	CStringArray& itemPath = dataPage1.GetTreeItemPath();
	CTreeCtrl& cTree = m_cTreeIgresTable;
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

	if (bLoadError)
	{
		data.CleanLoadSaveData();
	}
}


void CuPPage1::OnButtonParameterFile() 
{
	//
	// Wrong file type: please select the import parameter file (*.ii_imp).
	CString strMsg;
	strMsg.LoadString(IDS_SELECT_PARAM_FILE);

	CString strFullName;
	CFileDialog dlg(
	    TRUE,
	    NULL,
	    NULL,
	    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Import Parameter Files (*.ii_imp)|*.ii_imp||"));

	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (strMsg);
		return;
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (strExt.CompareNoCase (_T("ii_imp")) != 0)
		{
			AfxMessageBox (strMsg);
			return;
		}
	}

	Loading(strFullName);
}

void CuPPage1::OnButton2ImportFile() 
{
	CString strFullName;
	CFileDialog dlg(
	    TRUE,
	    NULL,
	    NULL,
	    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
#ifdef MAINWIN
	    _T("CSV Files (*.csv)|*.csv|Text files (*.txt)|*.txt|All files (*.*)|*.*||"));
#else
	    _T("CSV Files (*.csv)|*.csv|dBASE Files (*.dbf)|*.dbf|Text files (*.txt)|*.txt|All files (*.*)|*.*||"));
#endif
	
	TCHAR profile[MAX_PATH];
	TCHAR *pUProfile = _tgetenv(_T("USERPROFILE"));

	memset(profile, 0, sizeof(profile));
	_tcsncpy(profile, pUProfile, sizeof(profile)-1);

	if (*profile != '\0')
	{
	      _tcscat(profile, _T("\\Documents"));
	      if (access(profile, 00) == 0)
		      dlg.m_ofn.lpstrInitialDir=profile;
	}
	else
	     dlg.m_ofn.lpstrInitialDir=NULL;

	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	m_cEditFile2BeImported.SetWindowText(strFullName);
	DWORD dwFileSize = File_GetSize(strFullName);
	TRACE2("File selected= %s (%s)\n", strFullName, dlg.GetFileExt ());
	CString strSize;
	strSize.Format (_T("%d"), dwFileSize);
	m_cStaticFileSize.SetWindowText (strSize);
	TRACE1 ("File size = %d KB\n", dwFileSize);

	PreSelectAnsiOem(strFullName);
}

BOOL CuPPage1::OnSetActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;

	pParent->SetWizardButtons(PSWIZB_NEXT);
	TRACE1 ("Previous page = %d\n", pParent->GetPreviousPage ());

	return CPropertyPage::OnSetActive();
}

BOOL CuPPage1::OnKillActive() 
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();

	pParent->SetPreviousPage (1);
	return CPropertyPage::OnKillActive();
}


void CuPPage1::OnDestroy() 
{
	if (m_hIcon)
		DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	CPropertyPage::OnDestroy();
}

void CuPPage1::CollectData(CaIIAInfo* pData, BOOL bNeed2Detect)
{
	CString strFile;
	CaDataPage1& dataPage1 = pData->m_dataPage1;
	CaDataPage2& dataPage2 = pData->m_dataPage2;
	CaDataPage3& dataPage3 = pData->m_dataPage3;

	//
	// ANSI/OEM ?
	int nSelected = GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2);
	dataPage1.SetCodePage((nSelected== IDC_RADIO1)? 0: 1);
	//
	// Setup the File to be imported:
	m_cEditFile2BeImported.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	dataPage1.SetFile2BeImported(strFile);

	//
	// File Size:
	int nFileSize = (int)File_GetSize(dataPage1.GetFile2BeImported());
	dataPage1.SetFileSize(nFileSize);

	//
	// How many KB to scan ?
	CString strItem;
	m_cEditKB2Scan.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	dataPage1.SetKBToScan (_ttoi(strItem));

	//
	// The Line Count to Skip:
	m_cEditLineCountToSkip.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	int nLine = _ttoi(strItem);
	dataPage1.SetLineCountToSkip(nLine);
	if (!m_cEditLineCountToSkip.IsWindowEnabled())
		dataPage1.SetLineCountToSkip(0);
	
	//
	// The First Line considered as header:
	dataPage1.SetFirstLineAsHeader((BOOL)m_cCheckFirstLineHeader.GetCheck());
	//
	// User provides additional separator ?
	m_cEditMoreSeparator.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	dataPage1.SetCustomSeparator (strItem);
	//
	// Verify if we must create table on the fly. (bSetExistingTable = FALSE)
	BOOL bSetExistingTable = FALSE;
	HTREEITEM hSelected = m_cTreeIgresTable.GetSelectedItem ();
	if (hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIgresTable.GetItemData (hSelected);
		if (pSelected->IsKindOf(RUNTIME_CLASS(CaIeaTable)))
		{
			bSetExistingTable = TRUE;
			CtrfTable* ptrfTable = (CtrfTable*)pSelected;
			CaTable& table = ptrfTable->GetObject();
			CaLLQueryInfo* pQueryInfo = ptrfTable->GetQueryInfo();
			ASSERT(pQueryInfo);
			if (!pQueryInfo)
				throw (int)0;

			//
			// Setup the Ingres Table:
			dataPage1.SetNode(pQueryInfo->GetNode());
			dataPage1.SetDatabase(pQueryInfo->GetDatabase());
			dataPage1.SetTable(pQueryInfo->GetItem2(), pQueryInfo->GetItem2Owner());
			if (dataPage1.GetTable().CompareNoCase (_T("<new table>")) == 0 && dataPage1.GetTableOwner().IsEmpty())
				bSetExistingTable = FALSE;
			
		}
	}
	dataPage1.SetExistingTable(bSetExistingTable);

	if (dataPage1.IsExistingTable())
	{
		//
		// Match number/order of existing table columns with those detected from the file ?
		dataPage1.SetFileMatchTable(m_cCheckMatchNumberAndOrder.GetCheck());
	}
	else
	if (dataPage1.GetTable().CompareNoCase (_T("<new table>")) == 0 && dataPage1.GetTableOwner().IsEmpty())
	{
		//
		// New table to be created on fly:
		CString strNewTable;
		m_cEditTable2Create.GetWindowText(strNewTable);
		strNewTable.TrimLeft();
		strNewTable.TrimRight();
		dataPage1.SetNewTable(strNewTable);

		dataPage1.SetFileMatchTable(0);
	}

	//
	// Identify the gateway:
	CString strGateway = _T("");
	hSelected = m_cTreeIgresTable.GetSelectedItem ();
	CtrfItemData* pGateway = NULL;
	while(hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIgresTable.GetItemData (hSelected);
		if (pSelected->GetTreeItemType() == O_NODE_SERVER)
		{
			pGateway = pSelected;
			break;
		}
		hSelected = m_cTreeIgresTable.GetParentItem(hSelected);
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
	dataPage1.SetServerClass(strGateway);

	//
	// Identify the file to be imported:
	CString strFile2BeImported = dataPage1.GetFile2BeImported();
	CString strExt = _T("");
	int nFound = strFile2BeImported.ReverseFind (_T('.'));
	if (nFound != -1)
	{
		strExt = strFile2BeImported.Mid (nFound +1);
		TRACE1 ("File ext = %s\n", (LPCTSTR)strExt);
	}
	
	if (strExt.IsEmpty())
	{

	}
	else
	{
		if (strExt.CompareNoCase(_T("dbf")) == 0)
			dataPage1.SetImportedFileType(IMPORT_DBF);
		else
		if (strExt.CompareNoCase(_T("csv")) == 0)
			dataPage1.SetImportedFileType(IMPORT_CSV);
		else
		if (strExt.CompareNoCase(_T("xml")) == 0)
			dataPage1.SetImportedFileType(IMPORT_XML);
		else
		if (strExt.CompareNoCase(_T("txt")) == 0)
			dataPage1.SetImportedFileType(IMPORT_TXT);
		else
		{
			//
			// Assume to be a text file:
			dataPage1.SetImportedFileType(IMPORT_TXT);
		}
	}

	//
	// Update the file status information:
	if (!m_pStatusFile2BeImported)
		m_pStatusFile2BeImported = new CFileStatus();
	CFileStatus& rStatus = *m_pStatusFile2BeImported;
	CFile file2BeImported (dataPage1.GetFile2BeImported(), CFile::modeRead);
	if (!file2BeImported.GetStatus(rStatus))
	{
		delete m_pStatusFile2BeImported;
		m_pStatusFile2BeImported = NULL;
	}
	file2BeImported.Close();

	if (!dataPage1.GetFileImportParameter().IsEmpty())
	{
		CString strFParam = dataPage1.GetFileImportParameter();
		//
		// Check the existence of file:
		if (_taccess(strFParam, 0) != -1)
		{
			if (!m_pStatusFileParameter)
				m_pStatusFileParameter = new CFileStatus();
			CFileStatus& rs= *m_pStatusFileParameter;
			CFile fileParam (strFParam, CFile::modeRead);
			if (!fileParam.GetStatus(rs))
			{
				delete m_pStatusFileParameter;
				m_pStatusFileParameter = NULL;
			}
			fileParam.Close();
		}
	}

	//
	// Construct the tree item path:
	if (bNeed2Detect)
	{
		CString strItem;
		CStringArray& itemPath = dataPage1.GetTreeItemPath();
		itemPath.RemoveAll();

		CTreeCtrl& cTree = m_cTreeIgresTable;
		HTREEITEM hSelected = cTree.GetSelectedItem ();
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

		CaLLQueryInfo info(-1, dataPage1.GetNode(), dataPage1.GetDatabase());
		info.SetServerClass(dataPage1.GetServerClass());
		info.SetIndependent(TRUE);
		CaIeaTree& ieaTree = dataPage1.GetTreeData();
		CmtSessionManager* pSessionManager = ieaTree.GetSessionManager();
		ASSERT (pSessionManager);
		if (!pSessionManager)
			throw (int)1;
		//
		// Check if we select a database from the gateway:
		hSelected = cTree.GetSelectedItem ();
		if (hSelected)
		{
			BOOL bGatewayDatabase = INGRESII_llIsDatabaseGateway(&info, pSessionManager);
			dataPage1.SetGatewayDatabase (bGatewayDatabase);
		}

		//
		// User has selected an existing table:
		if (dataPage1.IsExistingTable())
		{
			//
			// Query Table Columns:
			info.SetObjectType(OBT_TABLECOLUMN);
			info.SetItem2(dataPage1.GetTable(), dataPage1.GetTableOwner());
			CTypedPtrList< CObList, CaDBObject* >& listColumn = dataPage1.GetTableColumns();
			while (!listColumn.IsEmpty())
				delete listColumn.RemoveHead();

			INGRESII_llQueryObject (&info, listColumn, pSessionManager);
		}

#if defined (_NEED_SPECIFYING_PAGESIZE)
		if (!dataPage1.GetGatewayDatabase())
		{
			//
			// Enumerate ingres page sizes:
			CString strPageSize;
			TCHAR tchszConst[MAX_ENUMPAGESIZE][16] = 
			{
				_T("page_size_2k"),
				_T("page_size_4k"),
				_T("page_size_8k"),
				_T("page_size_16k"),
				_T("page_size_32k"),
				_T("page_size_64k"),
			};

			CaSession session;
			session.SetNode(info.GetNode());
			session.SetDatabase(info.GetDatabase());
			session.SetIndependent(TRUE);
			session.SetDescription(sessionMgr.GetDescription());
			CaSessionUsage UseSession(pSessionManager, &session);

			BOOL* bPageSize = dataPage1.GetArrayPageSize();
			for (int i=0; i<MAX_ENUMPAGESIZE; i++)
			{
				strPageSize = INGRESII_llDBMSInfo(tchszConst[i]);
				if (strPageSize.CompareNoCase(_T("Y")) == 0)
					bPageSize[i] = TRUE;
				else
					bPageSize[i] = FALSE;
			}
			UseSession.Release(SESSION_COMMIT);
		}
#endif
	}
}



LRESULT CuPPage1::OnWizardNext() 
{
	CString strFile;
	CString strMsg;
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;
	CaDataPage2& dataPage2 = data.m_dataPage2;
	CaDataPage3& dataPage3 = data.m_dataPage3;

	try
	{
		CWaitCursor doWaitCursor;

		if (!CanGoNext())
			return E_FAIL;


		BOOL bNeed2Detect = NeedToDetectFormat();
		pParent->SetFileFormatUpdated(bNeed2Detect);
		if (bNeed2Detect)
		{
			dataPage2.Cleanup();
			dataPage3.Cleanup();
		}

		CollectData(&data, bNeed2Detect);
#if defined (_GATEWAY_NOT_POSSIBLE)
		if (dataPage1.GetGatewayDatabase())
		{
			// This message is temporary used. Do not put it into the resource.
			AfxMessageBox(_T("Actually, import data into the Gateway table is not possible."));
			return E_FAIL;
		}
#endif

		TRACE1("Performe the detection (yes=1, no=0): %d\n", bNeed2Detect);
		//
		// Detect the file format identification:
		CaDataPage2& dataPage2 = data.m_dataPage2;
		CTypedPtrList < CObList, CaSeparatorSet* >& listChoice = dataPage2.GetListChoice();
		ImportedFileType ft = dataPage1.GetImportedFileType();
		switch (ft)
		{
		case IMPORT_TXT:
		case IMPORT_CSV:
			if (bNeed2Detect)
			{
				data.SetInterrupted(FALSE);
				CaExecParamDetectSolution* p = new CaExecParamDetectSolution(&data);
#if defined (_ANIMATION_)
				CString strMsgAnimateTitle;
				strMsgAnimateTitle.LoadString (IDS_ANIMATE_TITLE_DETECTING);
				CxDlgWait dlg (strMsgAnimateTitle);
				dlg.SetUseAnimateAvi(AVI_EXAMINE);
				dlg.SetExecParam (p);
				dlg.SetDeleteParam(FALSE);
				dlg.SetShowProgressBar(FALSE);
				dlg.DoModal();
#else
				int nError = p->Run();
				p->OnTerminate(m_hWnd);
				if (nError != 0)
				{
					m_bInterrupted = TRUE; // To cause to re-detect again next time
					return E_FAIL;
				}
#endif
				m_bInterrupted = p->IsInterrupted();
				CString strMsgFailed = p->GetFailMessage();
				delete p;
				SetForegroundWindow ();
				if (m_bInterrupted || data.GetInterrupted() || !strMsgFailed.IsEmpty())
				{
					m_bInterrupted = TRUE; // To cause to re-detect again next time
					return E_FAIL;
				}
			}

			if (listChoice.IsEmpty())
			{
				if (dataPage1.IsExistingTable() && dataPage1.GetFileMatchTable() == 1)
				{
					//
					// No solution has been detected. But we go to the STEP2.
					// In this case, the STEP2 will have no TAB of solutions and what
					// user can do is to enforce the fixed width format.
					if (dataPage1.GetTableColumns().GetCount() == 1)
					{
						//
						// Consider each line of text is a single column:
						CaSeparatorSet* pSpecialSet = new CaSeparatorSet();
						pSpecialSet->SetConsecutiveSeparatorsAsOne(FALSE);
						pSpecialSet->SetTextQualifier(_T('\0'));
						listChoice.AddTail(pSpecialSet);
					}
				}
				else
				{
#if defined (_SPECIAL_ONECOLUMN)
					//
					// Consider each line of text is a single column:
					CaSeparatorSet* pSpecialSet = new CaSeparatorSet();
					pSpecialSet->SetConsecutiveSeparatorsAsOne(FALSE);
					pSpecialSet->SetTextQualifier(_T('\0'));
					listChoice.AddTail(pSpecialSet);
#endif
				}
			}
			else
			if (listChoice.GetCount() > 100)
			{
				if (dataPage1.GetCustomSeparator().IsEmpty())
				{
					//
					// Too many possible formats.\n
					// The file format may be too ambiguous, or the file not be significant.\n
					// Do you wish to enforce a fixed width format ?
					strMsg.LoadString (IDS_TOO_MANY_FORMAT1);
				}
				else
				{
					//
					// Too many possible formats.\n
					// This may be due to the non-standard separators that were entered in step 1.\n
					// The file format may be too ambiguous, or the file not be significant.\n
					// Do you wish to enforce a fixed width format ?
					strMsg.LoadString (IDS_TOO_MANY_FORMAT2);
				}

				int nAnswer = AfxMessageBox(strMsg, MB_ICONQUESTION|MB_YESNO);
				if (nAnswer == IDYES)
				{
					while (!listChoice.IsEmpty())
						delete listChoice.RemoveHead();
					//
					// Consider each line of text is a single column:
					CaSeparatorSet* pSpecialSet = new CaSeparatorSet();
					pSpecialSet->SetConsecutiveSeparatorsAsOne(FALSE);
					pSpecialSet->SetTextQualifier(_T('\0'));
					listChoice.AddTail(pSpecialSet);
				}
				else
				{
					return E_FAIL;
				}
			}
			else
			{
				if (dataPage1.GetKBToScan() > 0 && dataPage1.GetStopScanning())
				{
					BOOL bWTQ = TRUE;
					BOOL bWCS = TRUE;
					POSITION pos = listChoice.GetHeadPosition();
					while (pos != NULL)
					{
						CaSeparatorSet* pObj2Test = listChoice.GetNext(pos);
						if (!pObj2Test->GetConsecutiveSeparatorsAsOne())
							bWCS = FALSE;
						if (pObj2Test->GetTextQualifier() != _T('\0'))
							bWTQ = FALSE;
						if (!bWCS && !bWTQ)
							break;
					}
					
					if (bWTQ)
					{
						//
						// No text qualifiers have been detected and the whole file has not been scanned.
						strMsg.LoadString(IDS_NO_TEXT_QUALIFIERxPARTIAL_SCAN);
						AfxMessageBox(strMsg);
					}
					if (bWCS)
					{
						//
						// No consecutive separators considered as one have been detected and the whole file has not been scanned.
						strMsg.LoadString(IDS_NO_CONSECUTIVE_SEPARATORxPARTIAL_SCAN);
						AfxMessageBox(strMsg);
					}
				}

				if (dataPage1.GetFileMatchTable() != 1)
				{
					//
					// If the table is to be created on the fly OR
					// the number of columns in table is not required to be equal to the one in file then
					// we add one extra solution: <each row is considered as a single column>
					CaSeparatorSet* pSpecialSet = new CaSeparatorSet();
					pSpecialSet->SetConsecutiveSeparatorsAsOne(FALSE);
					pSpecialSet->SetTextQualifier(_T('\0'));
					listChoice.AddTail(pSpecialSet);
				}

			}
			break;

		case IMPORT_DBF:
#ifdef MAINWIN
			{
				CString strMsg;
				strMsg.LoadString(IDS_DBF_NOT_SUPPORTED);
				AfxMessageBox (strMsg);
				return E_FAIL;
			}
#endif
			if (bNeed2Detect || (dataPage2.GetArrayRecord().GetSize() == 0 && !bNeed2Detect))
			{
				data.SetInterrupted(FALSE);
				CaExecParamDetectSolution* p = new CaExecParamDetectSolution(&data);
#if defined (_ANIMATION_)
				CString strMsgAnimateTitle;
				strMsgAnimateTitle.LoadString (IDS_ANIMATE_TITLE_DETECTING);
				CxDlgWait dlg (strMsgAnimateTitle);
				dlg.SetUseAnimateAvi(AVI_EXAMINE);
				dlg.SetExecParam (p);
				dlg.SetDeleteParam(FALSE);
				dlg.SetShowProgressBar(TRUE);
				dlg.DoModal();
#else
				int nError = p->Run();
				p->OnTerminate(m_hWnd);
				if (nError != 0)
				{
					m_bInterrupted = TRUE; // To cause to re-detect again next time
					return E_FAIL;
				}
#endif
				CString strError = p->GetFailMessage();
				m_bInterrupted = p->IsInterrupted();
				delete p;
				SetForegroundWindow ();
				if (m_bInterrupted || data.GetInterrupted() || !strError.IsEmpty())
				{
					m_bInterrupted = TRUE; // To cause to re-detect again next time
					return E_FAIL;
				}
			}

			break;

		case IMPORT_XML:
			TRACE0("TODO (CuPPage1::OnWizardNext): Detect xml, cretate the list of records b4 going to step 2\n");
			AfxMessageBox(_T("Import Assistant does not support XML files in this version."));
			ASSERT (FALSE);
			return E_FAIL;


			if (bNeed2Detect)
				AutoDetectFormatXml (&data);
			break;

		default:
			TRACE0("TODO (CuPPage1::OnWizardNext): Unknown file format\n");
			ASSERT (FALSE);
			return E_FAIL;
			break;
		}
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return E_FAIL;
	}
	catch(CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
		dataPage2.Cleanup();
		return E_FAIL;
	}
	catch (CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		AfxMessageBox(strErr, MB_ICONEXCLAMATION|MB_OK);
		e->Delete();
		dataPage2.Cleanup();
		return E_FAIL;
	}
	catch (int nException)
	{
		dataPage2.Cleanup();
		strMsg = MSG_DetectFileError(nException);
		AfxMessageBox(strMsg, MB_ICONEXCLAMATION|MB_OK);
		return E_FAIL;
	}
	catch(...)
	{
		dataPage2.Cleanup();
		//
		// Failed to construct file format identification.
		strMsg.LoadString (IDS_FAIL_TO_CONSTRUCT_FORMAT_IDENTIFICATION);
		AfxMessageBox(strMsg);
		return E_FAIL;
	}
		
	return CPropertyPage::OnWizardNext();
}

void CuPPage1::OnSelchangedTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	try
	{
		CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
		CaIIAInfo& data = pParent->GetData();
		CaDataPage1& dataPage1 = data.m_dataPage1;
		HTREEITEM hTreeItem = pNMTreeView->itemNew.hItem;
		CTreeCtrl& cTree = m_cTreeIgresTable;
		CtrfItemData* pItem = (CtrfItemData*)cTree.GetItemData (hTreeItem);
		BOOL bNewTable = FALSE;
		if (pItem->IsKindOf(RUNTIME_CLASS(CaIeaTable)))
		{
			CtrfTable* ptrfTable = (CtrfTable*)pItem;
			CaTable& table = ptrfTable->GetObject();
			if (table.GetOwner().IsEmpty() && table.GetName().CompareNoCase(_T("<new table>")) == 0)
				bNewTable = TRUE;
		}
		
		//
		// Ingres Table to be created on fly:
		m_cStatic1.EnableWindow (bNewTable);
		m_cEditTable2Create.EnableWindow (bNewTable);
		
		//
		// Are the Number and Order of Columns of the File expected to 
		// Match Exactly those of the Ingres Table ?
		m_cCheckMatchNumberAndOrder.EnableWindow (!bNewTable);
		if (bNewTable)
			m_cCheckMatchNumberAndOrder.SetCheck (0);
		else
			m_cCheckMatchNumberAndOrder.SetCheck (m_nCurrentCheckMatchFileAndTable);
	}
	catch(...)
	{

	}
	
	*pResult = 0;
}


BOOL CuPPage1::NeedToDetectFormat()
{
	CuIIAPropertySheet* pParent = (CuIIAPropertySheet*)GetParent();
	CaIIAInfo& data = pParent->GetData();
	CaDataPage1& dataPage1 = data.m_dataPage1;

	//
	// The name of the file to be imported has been changed ?
	CString strFile;
	m_cEditFile2BeImported.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	if (strFile.CompareNoCase(dataPage1.GetFile2BeImported()) != 0)
		return TRUE;
	//
	// If the detection has been interrupted then do always the detection:
	if (m_bInterrupted)
		return TRUE;
	//
	// The file to be imported has been updated or modified ?
	if (!m_pStatusFile2BeImported)
	{
		//
		// Perform the next button for the first time:
		return TRUE;
	}
	else
	{
		CFileStatus rStatus;
		CFile f(dataPage1.GetFile2BeImported(), CFile::modeRead);
		if (f.GetStatus(rStatus))
		{
			if (rStatus.m_mtime != m_pStatusFile2BeImported->m_mtime)
				return TRUE;
		}
		else
		{
			CString strMsg;
			//
			// Failed to check the file status: \n<file name>
			AfxFormatString1(strMsg, IDS_FAIL_TO_CHECK_FILE_STATUS, (LPCTSTR)dataPage1.GetFile2BeImported());
			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			return TRUE;
		}
		f.Close();
	}

	//
	// The file that stores the parameters to be imported has been updated ?
	if (!dataPage1.GetFileImportParameter().IsEmpty())
	{
		if (!m_pStatusFileParameter)
			return TRUE;

		strFile = dataPage1.GetFileImportParameter();
		CFileStatus rStatus;
		CFile f(strFile, CFile::modeRead);

		if (f.GetStatus(rStatus))
		{
			if (rStatus.m_mtime != m_pStatusFileParameter->m_mtime)
				return TRUE;
		}
		else
		{
			CString strMsg;
			//
			// Failed to check the file status: \n<file name>
			AfxFormatString1(strMsg, IDS_FAIL_TO_CHECK_FILE_STATUS, (LPCTSTR)strFile);
			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			return TRUE;
		}
		f.Close();
	}

	//
	// The limit size (in KB) to scan the file has been changed ?
	CString strItem;
	m_cEditKB2Scan.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	int nKB = _ttoi(strItem);
	if (nKB != dataPage1.GetKBToScan())
		return TRUE;

	//
	// The Line Count to Skip has been changed ?
	m_cEditLineCountToSkip.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	int nLine = _ttoi(strItem);
	if (nLine != dataPage1.GetLineCountToSkip())
		return TRUE;
	//
	// The First Line considered as header has been changed ?
	if ((BOOL)m_cCheckFirstLineHeader.GetCheck() != dataPage1.GetFirstLineAsHeader())
		return TRUE;
	//
	// Number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	if (m_cCheckMatchNumberAndOrder.GetCheck() != dataPage1.GetFileMatchTable())
		return TRUE;

	//
	// ANSI/OEM ?
	int nSelected = (GetCheckedRadioButton (IDC_RADIO1, IDC_RADIO2) == IDC_RADIO1)? 0: 1;
	if (dataPage1.GetCodePage() != nSelected)
		return TRUE;

	//
	// The additional separators have been changed ?
	m_cEditMoreSeparator.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (dataPage1.GetCustomSeparator().CompareNoCase(strItem) != 0)
		return TRUE;


	//
	// The name of the table into which to import data has been changed ?
	HTREEITEM hSelected = m_cTreeIgresTable.GetSelectedItem ();
	if (hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIgresTable.GetItemData (hSelected);
		if (pSelected->IsKindOf(RUNTIME_CLASS(CaIeaTable)))
		{
			CtrfTable* ptrfTable = (CtrfTable*)pSelected;
			CaTable& table = ptrfTable->GetObject();
			CaLLQueryInfo* pQueryInfo = ptrfTable->GetQueryInfo();
			ASSERT(pQueryInfo);
			if (!pQueryInfo)
				throw (int)0;

			//
			// Node name has been changed:
			if (pQueryInfo->GetNode().CompareNoCase (dataPage1.GetNode()) != 0)
				return TRUE;
			//
			// Database name has been changed:
			if (pQueryInfo->GetDatabase().CompareNoCase (dataPage1.GetDatabase()) != 0)
				return TRUE;
			//
			// Table name has been changed:
			if (pQueryInfo->GetItem2().CompareNoCase (dataPage1.GetTable()) != 0 ||
			    pQueryInfo->GetItem2Owner().CompareNoCase (dataPage1.GetTableOwner()) != 0)
				return TRUE;
		}
	}

	//
	// Identify the gateway:
	CString strGateway = _T("");
	hSelected = m_cTreeIgresTable.GetSelectedItem ();
	CtrfItemData* pGateway = NULL;
	while(hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIgresTable.GetItemData (hSelected);
		if (pSelected->GetTreeItemType() == O_NODE_SERVER)
		{
			pGateway = pSelected;
			break;
		}
		hSelected = m_cTreeIgresTable.GetParentItem(hSelected);
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
	if (strGateway.CompareNoCase(dataPage1.GetServerClass()) != 0)
		return TRUE;

	CaDataForLoadSave* pData4LoadSave = data.GetLoadSaveData();
	if (pData4LoadSave)
	{
		if (pData4LoadSave->GetUsedByPage() <= 1)
			return TRUE;
	}

	return FALSE;
}


BOOL CuPPage1::CanGoNext()
{
	CString strMsg = _T("");
	//
	// Check the File to be imported:
	CString strFile;
	m_cEditFile2BeImported.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	if (strFile.IsEmpty())
	{
		//
		// Please select the file to import.
		strMsg.LoadString(IDS_PLEASE_SELECT_FILE_TO_IMPORT);

		AfxMessageBox (strMsg);
		m_cEditFile2BeImported.SetFocus();
		return FALSE;
	}
	//
	// Verify that the file must exist:
	if (_taccess(strFile, 0) == -1)
	{
		//
		// The file to be imported does not exist.
		strMsg.LoadString(IDS_FILE_TO_IMPORT_NOT_EXIST);
		AfxMessageBox (strMsg);
		return FALSE;
	}
	
	//
	// Verify that the file must have CRLF if the file is not a .DBF extension:
	BOOL b2CheckCRLF = TRUE;
	int nFound = strFile.ReverseFind (_T('.'));
	if (nFound != -1)
	{
		CString strExt = strFile.Mid (nFound+1);
		if (strExt.CompareNoCase(_T("dbf")) == 0)
			b2CheckCRLF = FALSE;
	}
	if (b2CheckCRLF && File_Empty(strFile))
	{
		//The file is empty
		strMsg.LoadString(IDS_FILE_EMPTY);
		AfxMessageBox (strMsg);
		return FALSE;
	}
	if (b2CheckCRLF && !File_HasCRLF(strFile))
	{
		//
		// The file to be imported has no "Carriage Return / Line Feed (CRLF)" sequences, or has too long lines.
		strMsg.LoadString(IDS_FILE_HAS_NO_CRLF);
		AfxMessageBox (strMsg);
		return FALSE;
	}

	//
	// Verify if user has select table object
	BOOL bSelectedObject = FALSE;
	HTREEITEM hSelected = m_cTreeIgresTable.GetSelectedItem ();
	if (hSelected)
	{
		CtrfItemData* pSelected = (CtrfItemData*)m_cTreeIgresTable.GetItemData (hSelected);
		if (pSelected->IsKindOf(RUNTIME_CLASS(CaIeaTable)))
		{
			bSelectedObject = TRUE;
		}
	}
	if (!bSelectedObject)
	{
		//
		// Please select a table into which to import data.
		strMsg.LoadString(IDS_PLEASE_SELECT_TABLE);
		AfxMessageBox (strMsg);
		m_cTreeIgresTable.SetFocus();
		return FALSE;
	}
	if (m_cEditTable2Create.IsWindowEnabled())
	{
		CString strNewTable;
		m_cEditTable2Create.GetWindowText(strNewTable);
		strNewTable.TrimLeft();
		strNewTable.TrimRight();
		if (strNewTable.IsEmpty())
		{
			// 
			// Please type the name of the table to be created.
			strMsg.LoadString(IDS_PLEASE_ENTER_TABLE_NAME);
			AfxMessageBox (strMsg);
			m_cEditTable2Create.SetFocus();
			return FALSE;
		}
	}

	return TRUE;
}


void CuPPage1::OnCheckFileMatchTable() 
{
	m_nCurrentCheckMatchFileAndTable = m_cCheckMatchNumberAndOrder.GetCheck();
}

void CuPPage1::PreSelectAnsiOem(LPCTSTR lpszFile)
{
	CString strFile = lpszFile;
	int nFound = strFile.ReverseFind (_T('.'));
	if (nFound != -1)
	{
		CString strExt = strFile.Mid (nFound+1);
		if (strExt.CompareNoCase(_T("dbf")) == 0)
		{
			//
			// Check the OEM code page:
			CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
			BOOL bEnable = FALSE;
			CWnd* pWnd = GetDlgItem(IDC_STATIC_SKIPFIRSTLINE);
			ASSERT (pWnd);
			if (pWnd)
				pWnd->EnableWindow(bEnable);
			pWnd = GetDlgItem(IDC_STATIC_FIRSTLINE);
			ASSERT (pWnd);
			if (pWnd)
				pWnd->EnableWindow(bEnable);
			m_cEditLineCountToSkip.EnableWindow(bEnable);
			m_cSpin1.EnableWindow(bEnable);
		}
		else
		{
			BOOL bEnable = TRUE;
			CWnd* pWnd = GetDlgItem(IDC_STATIC_SKIPFIRSTLINE);
			ASSERT (pWnd);
			if (pWnd)
				pWnd->EnableWindow(bEnable);
			pWnd = GetDlgItem(IDC_STATIC_FIRSTLINE);
			ASSERT (pWnd);
			if (pWnd)
				pWnd->EnableWindow(bEnable);
			m_cEditLineCountToSkip.EnableWindow(bEnable);
			m_cSpin1.EnableWindow(bEnable);
		}
	}
}

void CuPPage1::OnKillfocusEditFile2BeImported() 
{
	CString strSize;
	CString strFile;
	CString strMsg;

	m_cEditFile2BeImported.GetWindowText (strFile);
	strFile.TrimLeft();
	strFile.TrimRight();
	if (strFile.IsEmpty())
		return;
	
	//
	// Check the file existence:
	if (_taccess(strFile, 0) != -1)
	{
		DWORD dwFileSize = File_GetSize(strFile);
		strSize.Format (_T("%d"), dwFileSize);
		m_cStaticFileSize.SetWindowText (strSize);
		PreSelectAnsiOem(strFile);
	}
	else
	{
		// The file to be imported does not exist.
		strMsg.LoadString(IDS_FILE_TO_IMPORT_NOT_EXIST);
		AfxMessageBox (strMsg);
	}
}


void CuPPage1::OnSelchangingTreeIngresTable(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	if (!m_bAllowSelChangeTree)
	{
		MessageBeep(0xFFFF);
		*pResult = 1;
	}
}

void CuPPage1::OnChangeEditSkipLine() 
{
	CString s;
	if (!IsWindow(m_cEditLineCountToSkip.m_hWnd))
		return;
	m_cEditLineCountToSkip.GetWindowText(s);
	s.TrimLeft();
	s.TrimRight();
	int nCount = _ttoi(s);
	if (nCount != 1)
		m_cCheckFirstLineHeader.SetCheck(FALSE);
	else
		m_cCheckFirstLineHeader.SetCheck(TRUE);
}

void CuPPage1::OnCheckFirstLineHeader()
{
	if (m_cCheckFirstLineHeader.GetCheck() == BST_CHECKED)
		m_cEditLineCountToSkip.SetWindowTextA("1");
	else
		m_cEditLineCountToSkip.SetWindowTextA("0");

}

void CuPPage1::OnButtonCustomSeparator()
{
	if (m_cCustomSeparator.GetCheck() == BST_CHECKED)
	{
		m_cEditMoreSeparator.EnableWindow(TRUE);
	}
	else
	{
		m_cEditMoreSeparator.EnableWindow(FALSE);
	}
}

