/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsintegr.cpp, Implementation file
**    Project  : INGRES II/ Monitoring
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of a static item Replication.  (Integrity)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 07-Jul-1999 (schph01)
**    bug #97797
**    VDBA / Monitor / Replication / Integrities tab : select a table
**    registered in a CDDS different than zero, then press the GO button:
**    an erroneous message will appear in the output, mentioning for example
**    that a database is unprotected-readonly
** 02-Aug-99 (schph01)
**    bug #98166
**    the combobox control m_cComboDatabase1 is fill only with the
**    local database.
** 20-Sep-1999 (schph01)
**    #98170,  Enable or Disable the combobox "In CDDS" when you restore
**    a VDBA environnement.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "rsintegr.h"
#include "ipmprdta.h"
#include "repinteg.h"
#include "ipmframe.h"
#include "ipmdoc.h"
extern "C"
{
#include "libmon.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageIntegrity dialog


CuDlgReplicationStaticPageIntegrity::CuDlgReplicationStaticPageIntegrity(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageIntegrity::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageIntegrity)
	m_strTimeBegin = _T("");
	m_strTimeEnd = _T("");
	m_strResult = _T("");
	m_strComboTable = _T("");
	m_strComboCdds = _T("");
	m_strComboDatabase1 = _T("");
	m_strComboDatabase2 = _T("");
	//}}AFX_DATA_INIT
}


void CuDlgReplicationStaticPageIntegrity::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageIntegrity)
	DDX_Control(pDX, IDC_UP, m_cButtonUp);
	DDX_Control(pDX, IDC_DOWN, m_cButtonDown);
	DDX_Control(pDX, IDC_GO, m_cButtonGo);
	DDX_Control(pDX, IDC_EDIT4, m_cEdit4);
	DDX_Control(pDX, IDC_COMBO4, m_cComboDatabase2);
	DDX_Control(pDX, IDC_COMBO3, m_cComboDatabase1);
	DDX_Control(pDX, IDC_COMBO2, m_cComboCdds);
	DDX_Control(pDX, IDC_COMBO1, m_cComboTable);
	DDX_Text(pDX, IDC_EDIT1, m_strTimeBegin);
	DDX_Text(pDX, IDC_EDIT2, m_strTimeEnd);
	DDX_Text(pDX, IDC_EDIT4, m_strResult);
	DDX_CBString(pDX, IDC_COMBO1, m_strComboTable);
	DDX_CBString(pDX, IDC_COMBO2, m_strComboCdds);
	DDX_CBString(pDX, IDC_COMBO3, m_strComboDatabase1);
	DDX_CBString(pDX, IDC_COMBO4, m_strComboDatabase2);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_LIST1, m_cListColumnOrder);
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageIntegrity, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageIntegrity)
	ON_BN_CLICKED(IDC_GO, OnButtonGo)
	ON_BN_CLICKED(IDC_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_DOWN, OnButtonDown)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeRegTblName)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSelchangeComboCDDS)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnSelchangeComboDB1)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnSelchangeComboDb2)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageIntegrity message handlers

void CuDlgReplicationStaticPageIntegrity::OnButtonGo() 
{
	CaReplicIntegrityRegTable* pItem;
	CString strOrderColumns;
	int cddsNo, db1, db2 ;
	
	int tblindex = m_cComboTable.GetCurSel();
	int cddsindex= m_cComboCdds.GetCurSel();
	int indexdb1 = m_cComboDatabase1.GetCurSel();
	int indexdb2 = m_cComboDatabase2.GetCurSel();

	if ( tblindex != CB_ERR && cddsindex != CB_ERR && indexdb1 != CB_ERR && indexdb2 != CB_ERR)
	{
		pItem = ( CaReplicIntegrityRegTable *)m_cComboTable.GetItemDataPtr(tblindex);
		cddsNo = m_cComboCdds.GetItemData(cddsindex);
		db1 = m_cComboDatabase1.GetItemData(indexdb1);
		db2 = m_cComboDatabase2.GetItemData(indexdb2);
	}
	else
	{
		CString strMsg; // _T("No information selected in table name or cdds number or databases.")
		strMsg.LoadString (IDS_MSG_NOINFORMATION_SELECTED);
		m_cEdit4.SetWindowText(strMsg);
		return;
	}

	strOrderColumns.Empty();
	m_strResult.Empty();
	if (m_bColumnsOrderModifyByUser)
		GetOrderColumns(&strOrderColumns);
	CWaitCursor hourglass;
	CString strTimeBegin;
	CString strTimeEnd;
	GetDlgItem(IDC_EDIT1)->GetWindowText(strTimeBegin);
	GetDlgItem(IDC_EDIT2)->GetWindowText(strTimeEnd);

	CdIpmDoc* pIpmDoc = NULL;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
	ASSERT(pIpmFrame);
	if (pIpmFrame)
	{
		pIpmDoc = pIpmFrame->GetIpmDoc();
		ASSERT (pIpmDoc);
	}
	if (!pIpmDoc)
		return;

	CString strOutput;
	IPM_QueryIntegrityOutput (pIpmDoc, pItem,db1,db2,cddsNo,(LPTSTR)(LPCTSTR)strTimeBegin,
	                          (LPTSTR)(LPCTSTR)strTimeEnd,(LPTSTR)(LPCTSTR)strOrderColumns, strOutput);
	if (!strOutput.IsEmpty())
		m_cEdit4.SetWindowText(strOutput);
	else
		m_cEdit4.SetWindowText(_T(""));
}

void CuDlgReplicationStaticPageIntegrity::OnButtonUp() 
{
	CaReplicCommon *pItemCol;

	int nCount = m_cListColumnOrder.GetCount();
	if (nCount == 0)
		return;
	int nSel = m_cListColumnOrder.GetCurSel();
	if (nSel == LB_ERR || nSel == 0)
		return;
	CString strCurItem;
	m_cListColumnOrder.GetText (nSel, strCurItem);
	pItemCol = (CaReplicCommon *)m_cListColumnOrder.GetItemDataPtr(nSel);
	m_cListColumnOrder.DeleteString (nSel);
	m_cListColumnOrder.InsertString (nSel-1, strCurItem);
	m_cListColumnOrder.SetItemDataPtr(nSel-1, pItemCol);
	m_cListColumnOrder.SetCurSel (nSel-1);
	// the order columns is changed by user 
	m_bColumnsOrderModifyByUser = TRUE;
}

void CuDlgReplicationStaticPageIntegrity::OnButtonDown() 
{
	CaReplicCommon *pItemCol;

	int nCount = m_cListColumnOrder.GetCount();
	if (nCount == 0)
		return;
	int nSel = m_cListColumnOrder.GetCurSel();
	if (nSel == LB_ERR || nSel == (nCount-1))
		return;
	CString strCurItem;
	m_cListColumnOrder.GetText (nSel, strCurItem);
	pItemCol = (CaReplicCommon *)m_cListColumnOrder.GetItemDataPtr(nSel);
	m_cListColumnOrder.DeleteString (nSel);
	m_cListColumnOrder.InsertString (nSel+1, strCurItem);
	m_cListColumnOrder.SetItemDataPtr(nSel+1, pItemCol);
	m_cListColumnOrder.SetCurSel (nSel+1);
	// the order columns is changed by user 
	m_bColumnsOrderModifyByUser = TRUE;
}

void CuDlgReplicationStaticPageIntegrity::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationStaticPageIntegrity::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationStaticPageIntegrity::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit4.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cEdit4.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right  - r.left;
	r.bottom = rDlg.bottom - r.left;
	m_cEdit4.MoveWindow (r);
}


LONG CuDlgReplicationStaticPageIntegrity::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int index, nNodeHandle;
	CPtrList listInfoStruct;
	TCHAR szDbName[100];
	int iReplicVersion;

	LPIPMUPDATEPARAMS pUps    = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	try
	{
		LPRESOURCEDATAMIN pSvrDta = (LPRESOURCEDATAMIN)pUps->pStruct;
		CaReplicIntegrityRegTable *pItem = NULL;

		CdIpmDoc* pIpmDoc = NULL;
		CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
		ASSERT(pIpmFrame);
		if (pIpmFrame)
		{
			pIpmDoc = pIpmFrame->GetIpmDoc();
			ASSERT (pIpmDoc);
		}
		if (!pIpmDoc)
			return 0;
		m_RegTableList.ClearList();
		m_RegTableList.RemoveAll();
		m_ColList.ClearList();
		m_ColList.RemoveAll();
		// the order columns isn't changed by user 
		m_bColumnsOrderModifyByUser = FALSE;
		m_cComboDatabase1.ResetContent();
		m_cComboDatabase2.ResetContent();
		m_cComboTable.ResetContent();
		m_cComboCdds.ResetContent();
		m_cListColumnOrder.ResetContent();

		nNodeHandle = pIpmDoc->GetNodeHandle();

		IPM_GetInfoName(pIpmDoc , OT_DATABASE, pSvrDta, szDbName, sizeof(szDbName));
		iReplicVersion = MonGetReplicatorVersion(nNodeHandle, szDbName);
		CString VnodeName = (LPCTSTR)GetVirtNodeName(nNodeHandle);

		CaIpmQueryInfo queryInfo(pIpmDoc, GetRepObjectType4ll(OT_REPLIC_REGTBLS,iReplicVersion),pUps);
		queryInfo.SetNode((LPCTSTR)GetVirtNodeName(nNodeHandle));
		queryInfo.SetDatabase ((LPCTSTR)szDbName);
		queryInfo.GetException().SetErrorCode(-1);

		BOOL bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			POSITION pos = listInfoStruct.GetHeadPosition();
			while (pos != NULL)
			{
				DD_REGISTERED_TABLES *pRegTmp = (DD_REGISTERED_TABLES*) listInfoStruct.GetNext(pos);
				pItem = new CaReplicIntegrityRegTable((LPTSTR)pRegTmp->tablename ,(LPTSTR)pRegTmp->tableowner,
				                                      (LPTSTR)pRegTmp->cdds_lookup_table_v11 , (LPTSTR)szDbName,
				                                       VnodeName ,nNodeHandle,iReplicVersion, pRegTmp->table_no,pRegTmp->cdds);
				m_RegTableList.AddTail(pItem);

				index  = m_cComboTable.AddString ((LPTSTR)pRegTmp->tablename);
				if (index != CB_ERR && index != CB_ERRSPACE)
					m_cComboTable.SetItemDataPtr (index,pItem);
			}
		}

		if ( queryInfo.IsReportError() )
		{
			if (queryInfo.GetException().GetErrorCode() ==  RES_NOGRANT)
			{
				TCHAR UserName[200];
				CString cTmp;
				// Get the current user name
				memset (UserName,'\0',sizeof(UserName));
				DBAGetUserName((unsigned char *)GetVirtNodeName(nNodeHandle),
				               (unsigned char *)UserName);

				//_T("No Grant for User '%s' on Database '%s'.")
				cTmp.Format(IDS_E_NO_GRANT ,UserName,szDbName);
				m_strResult = cTmp;
				UpdateData(FALSE);
				EnableButton();
				return 0L;
			}
		}
		while (!listInfoStruct.IsEmpty())
			delete (DD_REGISTERED_TABLES*)listInfoStruct.RemoveHead();

		queryInfo.SetObjectType(GetRepObjectType4ll(OT_REPLIC_CONNECTION,iReplicVersion));
		queryInfo.GetException().SetErrorCode(-1);
		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			if ( !listInfoStruct.IsEmpty() )
			{
				int index1,index2;
				CStringList* csTempoLst;
				CString csTempo;
				BOOL bFirst = TRUE;
				POSITION pos,postmp;
				pos = (POSITION) listInfoStruct.GetHeadPosition();
				if (pos != NULL)
				{
					csTempoLst = (CStringList *)listInfoStruct.GetNext(pos);
					postmp = csTempoLst->GetHeadPosition();
					while ( postmp != NULL )
					{
						csTempo = csTempoLst->GetNext(postmp);
						int nDbNumber, nTemp = csTempo.Find(_T(" "));
						nDbNumber = atoi(csTempo.Left(nTemp));
						if ( bFirst ) // the first item is the local database.
						{
							index1 = m_cComboDatabase1.AddString(csTempo);
							m_cComboDatabase1.SetItemData(index1,nDbNumber);
							bFirst = FALSE;
						}
						index2 = m_cComboDatabase2.AddString(csTempo);
						m_cComboDatabase2.SetItemData(index2,nDbNumber);
					}
				}
				while (!listInfoStruct.IsEmpty())
					delete (CStringList*)listInfoStruct.RemoveHead();
	
			}
		}

		m_cComboTable.SetCurSel(0);
		m_cComboCdds.SetCurSel(0);
		m_cComboDatabase1.SetCurSel(0);
		m_cComboDatabase2.SetCurSel(0);
		m_cComboDatabase1.EnableWindow(FALSE); // The first combobox database is always disable.

		m_strTimeBegin = _T("");
		m_strTimeEnd   = _T("");
		m_strResult    = _T("");
		
		UpdateData (FALSE);

		// Simulate click on first item
		m_cComboTable.SetFocus();
		OnSelchangeRegTblName();

		EnableButton();
	}

	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;

}

LONG CuDlgReplicationStaticPageIntegrity::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationStaticDataPageIntegrity")) == 0);
	CaReplicationStaticDataPageIntegrity* pData = (CaReplicationStaticDataPageIntegrity*)lParam;
	if (!pData)
		return 0L;
	
	POSITION pos = NULL;
	CString strItem;
	CaReplicIntegrityRegTable *pItem,*pCurItem;
	CaReplicCommon *pItemCommon,*pItemCommon1;
	int index,index1,index2;
	try
	{
		if ( !pData->m_RegTableList.IsEmpty() )	{
			pos = pData->m_RegTableList.GetHeadPosition();
			while (pos != NULL)
			{
				pCurItem = pData->m_RegTableList.GetNext(pos);
				pItem = new CaReplicIntegrityRegTable(
					pCurItem->GetTable_Name(),
					pCurItem->GetOwner_name(),
					pCurItem->GetLookupTable_Name(),
					pCurItem->GetDbName(),
					pCurItem->GetVnodeName(),
					pCurItem->GetNodeHandle(),
					pCurItem->GetReplicVersion(),
					pCurItem->GetTable_no(),
					pCurItem->GetCdds_No());
				m_RegTableList.AddTail(pItem);

				index  = m_cComboTable.AddString (pCurItem->GetTable_Name());
				if (index != CB_ERR && index != CB_ERRSPACE)
					m_cComboTable.SetItemDataPtr (index,pItem);
			}
		}
		pos = pData->m_CDDSList.GetHeadPosition();
		while (pos != NULL)
		{
			pItemCommon = pData->m_CDDSList.GetNext (pos);
			index = m_cComboCdds.AddString(pItemCommon->GetCommonString());
			m_cComboCdds.SetItemData(index,pItemCommon->GetCommonNumber());
		}
		// grayed 'In CDDS' if necessary.
		if (pCurItem->GetLookupTable_Name().IsEmpty())
			m_cComboCdds.EnableWindow(FALSE);
		else
			m_cComboCdds.EnableWindow(TRUE);
		pos = pData->m_DbList1.GetHeadPosition();
		while (pos != NULL)
		{
			pItemCommon = pData->m_DbList1.GetNext (pos);
			index1 = m_cComboDatabase1.AddString(pItemCommon->GetCommonString());
			m_cComboDatabase1.SetItemData(index1,pItemCommon->GetCommonNumber());
			m_cComboDatabase1.EnableWindow(FALSE); // The first combobox database is always grayed.
		}
		pos = pData->m_DbList2.GetHeadPosition();
		while (pos != NULL)
		{
			pItemCommon = pData->m_DbList2.GetNext (pos);
			index2 = m_cComboDatabase2.AddString(pItemCommon->GetCommonString());
			m_cComboDatabase2.SetItemData(index2,pItemCommon->GetCommonNumber());
		}
		pos = pData->m_ColList.GetHeadPosition();
		while (pos != NULL)
		{
			pItemCommon = pData->m_ColList.GetNext (pos);
			if (pItemCommon)	{
				pItemCommon1 = (CaReplicCommon *)new CaReplicCommon(pItemCommon->GetCommonString(),pItemCommon->GetCommonNumber());
				m_ColList.AddTail(pItemCommon1);
				index = m_cListColumnOrder.AddString(pItemCommon->GetCommonString());
				if (index != CB_ERR)
					m_cListColumnOrder.SetItemDataPtr(index,pItemCommon1);
			}
		}

		m_strComboCdds       = pData->m_strComboCdds;
		m_strComboDatabase1  = pData->m_strComboDatabase1;
		m_strComboDatabase2  = pData->m_strComboDatabase2;
		m_strComboTable      = pData->m_strComboTable;
		m_strTimeBegin       = pData->m_strTimeBegin;
		m_strTimeEnd         = pData->m_strTimeEnd;
		m_strResult          = pData->m_strResult;
		UpdateData (FALSE);
		/*
		pos = pData->m_listComboTable.GetHeadPosition();
		while (pos != NULL)
		{
			strItem = pData->m_listComboTable.GetNext (pos);
			m_cComboTable.AddString(strItem);
		}
		pos = pData->m_listComboCDDS.GetHeadPosition();
		while (pos != NULL)
		{
			strItem = pData->m_listComboCDDS.GetNext (pos);
			m_cComboCdds.AddString(strItem);
		}
		pos = pData->m_listDatabase1.GetHeadPosition();
		while (pos != NULL)
		{
			strItem = pData->m_listDatabase1.GetNext (pos);
			m_cComboDatabase1.AddString(strItem);
		}
		pos = pData->m_listDatabase2.GetHeadPosition();
		while (pos != NULL)
		{
			strItem = pData->m_listDatabase2.GetNext (pos);
			m_cComboDatabase2.AddString(strItem);
		}
		pos = pData->m_listColumnOrder.GetHeadPosition();
		while (pos != NULL)
		{
			strItem = pData->m_listColumnOrder.GetNext (pos);
			m_cListColumnOrder.AddString(strItem);
		}
		m_strComboTable      = pData->m_strComboTable;
		m_strComboCdds       = pData->m_strComboCdds;
		m_strComboDatabase1  = pData->m_strComboDatabase1;
		m_strComboDatabase2  = pData->m_strComboDatabase2;
		m_strTimeBegin       = pData->m_strTimeBegin;
		m_strTimeEnd         = pData->m_strTimeEnd;
		m_strResult          = pData->m_strResult;
		*/
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageIntegrity::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CString strItem;
	CaReplicationStaticDataPageIntegrity* pData = NULL;
	try
	{
		pData = new CaReplicationStaticDataPageIntegrity();
		UpdateData (TRUE);
		pData->m_strComboTable     = m_strComboTable;
		pData->m_strComboCdds      = m_strComboCdds;
		pData->m_strComboDatabase1 = m_strComboDatabase1;
		pData->m_strComboDatabase2 = m_strComboDatabase2;
		pData->m_strTimeBegin      = m_strTimeBegin;
		pData->m_strTimeEnd        = m_strTimeEnd;
		pData->m_strResult         = m_strResult; // Edit4

		int i, nCount = m_cComboTable.GetCount();
		for (i=0; i<nCount; i++)
		{
			CaReplicIntegrityRegTable *pItem, *pCurItem;
			m_cComboTable.GetLBText (i, strItem);
			pCurItem = (CaReplicIntegrityRegTable *)m_cComboTable.GetItemDataPtr(i);
			
			pItem = new CaReplicIntegrityRegTable(
				pCurItem->GetTable_Name(),
				pCurItem->GetOwner_name(),
				pCurItem->GetLookupTable_Name(), 
				pCurItem->GetDbName(),
				pCurItem->GetVnodeName(),
				pCurItem->GetNodeHandle(),
				pCurItem->GetReplicVersion(), 
				pCurItem->GetTable_no(),
				pCurItem->GetCdds_No());
			pData->m_RegTableList.AddTail (pItem);
		}
		nCount = m_cComboCdds.GetCount();
		for (i=0; i<nCount; i++)
		{
			int value;
			value = m_cComboCdds.GetItemData(i);
			m_cComboCdds.GetLBText (i, strItem);
			CaReplicCommon *pItemCommon = (CaReplicCommon *) new CaReplicCommon (strItem,value);
			pData->m_CDDSList.AddTail (pItemCommon);
		}
		// Combo Database 1 
		nCount = m_cComboDatabase1.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboDatabase1.GetLBText (i, strItem);
			int Value = m_cComboDatabase1.GetItemData (i);
			CaReplicCommon *pItemCommon = (CaReplicCommon *) new CaReplicCommon ( strItem, Value );
			pData->m_DbList1.AddTail (pItemCommon);
		}
		// Combo Database 2
		nCount = m_cComboDatabase2.GetCount();
		for (i=0; i<nCount; i++)
		{
			m_cComboDatabase2.GetLBText (i, strItem);
			int Value = m_cComboDatabase2.GetItemData (i);
			CaReplicCommon *pItemCommon = (CaReplicCommon *) new CaReplicCommon ( strItem, Value );
			pData->m_DbList2.AddTail (pItemCommon);
		}
		nCount = m_cListColumnOrder.GetCount();
		for (i=0; i<nCount; i++)
		{
			//m_cListColumnOrder.GetText (i, strItem);
			CaReplicCommon *pItemCur;
			pItemCur = (CaReplicCommon *)m_cListColumnOrder.GetItemDataPtr(i);
			CaReplicCommon *pItemCommon = (CaReplicCommon *) new CaReplicCommon (pItemCur->GetCommonString(),pItemCur->GetCommonNumber());
			pData->m_ColList.AddTail (pItemCommon);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgReplicationStaticPageIntegrity::OnSelchangeRegTblName()
{
	CaReplicIntegrityRegTable* pItem = NULL;
	CString csTempo;
	int iret;

	CdIpmDoc* pIpmDoc = NULL;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)GetParentFrame();
	ASSERT(pIpmFrame);
	if (pIpmFrame)
	{
		pIpmDoc = pIpmFrame->GetIpmDoc();
		ASSERT (pIpmDoc);
	}
	if (!pIpmDoc)
		return;
	try
	{
		m_ColList.ClearList();
		m_ColList.RemoveAll();
		m_cListColumnOrder.ResetContent();
		m_cComboCdds.ResetContent();

		// now get the columns for this table
		int index = m_cComboTable.GetCurSel();
		if ( index == CB_ERR )
			return;
		m_cComboTable.GetLBText(index,csTempo);
		pItem =  (CaReplicIntegrityRegTable *)m_cComboTable.GetItemDataPtr(index);
		if (!pItem->IsValidTbl())
			return;

		CPtrList listInfoStruct;
		CaIpmQueryInfo queryInfo(pIpmDoc,
		                         GetRepObjectType4ll(OT_REPLIC_REGCOLS,
		                                             pItem->GetReplicVersion())
		                         );
		queryInfo.SetNode((LPCTSTR)pItem->GetVnodeName());
		queryInfo.SetDatabase ((LPCTSTR)pItem->GetDbName());
		queryInfo.SetItem2(csTempo,pItem->GetOwner_name());

		BOOL bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			POSITION pos;
			pos = (POSITION)listInfoStruct.GetHeadPosition();
			CaReplicCommon *pItemCol;
			while(pos != NULL)
			{
				pItemCol = (CaReplicCommon *)listInfoStruct.GetNext (pos);
				index = m_cListColumnOrder.AddString (pItemCol->GetCommonString());
				if (index != CB_ERR && index != CB_ERRSPACE)
					m_cListColumnOrder.SetItemDataPtr(index,pItemCol);
				m_ColList.AddTail(pItemCol);
			}

			if (m_cListColumnOrder.GetCount())
			{
				index = m_cListColumnOrder.AddString (_T("trans_time"));
				if (index != CB_ERR && index != CB_ERRSPACE) {
					CaReplicCommon *pItemCol = new CaReplicCommon(_T("trans_time"),index+1);
					m_cListColumnOrder.SetItemDataPtr(index,pItemCol);
					m_ColList.AddTail(pItemCol);
				}
			}
		}

		RETRIEVECDDSNUM RetCddsNum;
		memset(&RetCddsNum,NULL,sizeof(RETRIEVECDDSNUM));
		csTempo = pItem->GetLookupTable_Name();
		if (!csTempo.IsEmpty())
		{
			iret = LIBMON_GetCddsNumInLookupTbl( pItem->GetNodeHandle(),
			                                    (LPTSTR)(LPCTSTR)pItem->GetDbName(),
			                                    (LPTSTR)(LPCTSTR)pItem->GetLookupTable_Name(),
			                                     &RetCddsNum);
			if (iret != RES_SUCCESS)// TO BE FINISH PS
			{
				BOOL bWithHistory = FALSE;
				switch (iret)
				{
				case RES_E_CONNECTION_FAILED:
					bWithHistory = TRUE;
					AfxMessageBox(IDS_E_GET_SESSION);
					break;
				case RES_E_CANNOT_ALLOCATE_MEMORY:
					AfxMessageBox(_T("Cannot allocate memory."));
					break;
				case RES_ERR:
					bWithHistory = TRUE;
					AfxMessageBox(_T("Error whille retrieve information.")); // TO BE FINISH PS
					break;
				case RES_E_NO_INFORMATION_FOUND:
					bWithHistory = TRUE;
					AfxMessageBox(IDS_NO_INFORMATION_FOUND);
					break;
				}
				EnableButton();
				return;
			}
			else
			{
				int iIndex;
				LPCDDSINFO lpCurInf;
				lpCurInf = RetCddsNum.lpCddsInfo;
				for (int i=0;i<RetCddsNum.iNumCdds;i++,lpCurInf++)
				{
					if (lpCurInf)
					{
						iIndex = m_cComboCdds.AddString(lpCurInf->tcDisplayInfo);
						if (iIndex != CB_ERR)
							m_cComboCdds.SetItemData(iIndex,lpCurInf->CddsNumber);
					}
				}
				delete (RetCddsNum.lpCddsInfo);
				RetCddsNum.lpCddsInfo = NULL;
				RetCddsNum.iNumCdds = 0;
			}
			if (m_cComboCdds.GetCount() >0 )
				m_cComboCdds.SetCurSel(0);
			else{
				//_T("No valid CDDS in lookup table.")
				AfxMessageBox(IDS_E_NO_CDDS); // E_RM0073_No_valid_cdds
				m_strResult.LoadString(IDS_E_NO_CDDS);
				UpdateData(FALSE);
			}
			m_cComboCdds.EnableWindow(TRUE);
		}
		else
		{
			CString csTemp;
			TCHAR cddsname[MAXOBJECTNAME];
			int iret;
			iret = LIBMON_GetCddsName ( pItem->GetNodeHandle(),
			                            (LPTSTR)(LPCTSTR)pItem->GetDbName(),
			                            pItem->GetCdds_No(),
			                            cddsname);
			if ( iret != RES_SUCCESS )
				csTemp.Format(_T("%d"),pItem->GetCdds_No());
			else
				csTemp.Format(_T("%d %s"),pItem->GetCdds_No(),cddsname);
			index = m_cComboCdds.AddString(csTemp);
			if (index != CB_ERR)
			{
				m_cComboCdds.SetItemData(index,pItem->GetCdds_No());
				m_cComboCdds.SetCurSel(index);
			}
			m_cComboCdds.EnableWindow(FALSE);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}

	EnableButton();
}

void CuDlgReplicationStaticPageIntegrity::EnableButton()
{
	bool bEnableGo = FALSE;
	bool bEnableUpDown = FALSE;
	if (m_RegTableList.GetCount() && m_ColList.GetCount()&&
		m_cComboCdds.GetCount()   && m_cComboDatabase1.GetCount()&&
		m_cComboDatabase2.GetCount())
		bEnableGo = TRUE;
	
	if (m_ColList.GetCount())
		bEnableUpDown = TRUE;
	
	m_cButtonUp.EnableWindow (bEnableUpDown);
	m_cButtonDown.EnableWindow (bEnableUpDown);
	m_cButtonGo.EnableWindow (bEnableGo);
}

void CuDlgReplicationStaticPageIntegrity::OnSelchangeComboCDDS() 
{
	m_cEdit4.SetWindowText(_T(""));
}

void CuDlgReplicationStaticPageIntegrity::OnSelchangeComboDB1() 
{
	m_cEdit4.SetWindowText(_T(""));
}

void CuDlgReplicationStaticPageIntegrity::OnSelchangeComboDb2() 
{
	m_cEdit4.SetWindowText(_T(""));
}

void CuDlgReplicationStaticPageIntegrity::GetOrderColumns(CString * strListCol)
{
	CString strTemp;
	int i = 0 ,nCount = m_cListColumnOrder.GetCount();
	CaReplicCommon *pItemCol;

	if (nCount == 0)
		return;
	for (i=0;i<nCount;i++) {
		pItemCol = (CaReplicCommon *)m_cListColumnOrder.GetItemDataPtr(i);
		strTemp.Empty();
		if ( pItemCol ) 
		{
			// +3 : when the select statement is generate
			// there are three columns before the columns of this table.
			// See TBLINTEG.SC
			strTemp.Format(_T("%d"),pItemCol->GetCommonNumber()+3);
		}
		*strListCol = *strListCol + strTemp ;
		if ( i != nCount-1 )
			*strListCol = *strListCol + _T(", ");
	}
}

