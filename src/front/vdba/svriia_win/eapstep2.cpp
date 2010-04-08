/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eapstep2.cpp: implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Common behaviour of Export Assistant in page 2.
**
** History:
**
** 16-Jan-2002 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 09-June-2004 (nansa02)
**    BUG #112435,Removed a default flag set for CFileDialog.
** 05-Sep-2008 (drivi01)
**    Add a default flag OFN_EXPLORER back to display
**    nicer dialog consistent with the rest of the application.
**    Could not reproduce the hang.
**/

#include "stdafx.h"
#include "eapstep2.h"
#include "rcdepend.h"
#include "eapsheet.h"
#include "sqlselec.h"
#include "dmlexpor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuIeaPPage2CommonData::CuIeaPPage2CommonData()
{
	m_pDummyColumn = new CaColumnExport(_T("<dummy>"), _T(""));
}

CuIeaPPage2CommonData::~CuIeaPPage2CommonData()
{
	if (m_pDummyColumn)
		delete m_pDummyColumn;
}


BOOL CuIeaPPage2CommonData::CreateDoubleListCtrl (CWnd* pWnd, CRect& r)
{
	m_wndHeaderRows.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, r, pWnd, 5000);
	m_wndHeaderRows.MoveWindow (r);
	m_wndHeaderRows.ShowWindow (SW_SHOW);

	return TRUE;
}

void CuIeaPPage2CommonData::DisplayResults(CWnd* pWnd, ExportedFileType expft)
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)pWnd->GetParent();
	CaIEAInfo& data = pParent->GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;

	CaQueryResult& result = dataPage1.GetQueryResult();
	CTypedPtrList< CObList, CaColumnExport* >& listLayout = result.GetListColumnLayout();
	CTypedPtrList< CObList, CaColumn* >& listColum  = result.GetListColumn();
	POSITION pos = listLayout.GetHeadPosition();
	//
	// The rows information (Headers):
	int nHeaderRows = 4;
	switch (expft)
	{
	case EXPORT_CSV:
		nHeaderRows = 4;
		break;
	case EXPORT_DBF:
		nHeaderRows = 5;
		break;
	case EXPORT_XML:
		nHeaderRows = 5;
		break;
	case EXPORT_FIXED_WIDTH:
		nHeaderRows = 5;
		break;
	case EXPORT_COPY_STATEMENT:
		nHeaderRows = 5;
		break;
	default:
		//
		// You must define new type.
		ASSERT(FALSE);
		break;
	}
	//
	//'nHeaderRows' line in the first list control:
	m_rowInfo.m_arrayColumn.RemoveAll();

	m_wndHeaderRows.SetFixedRowInfo(nHeaderRows);
	m_rowInfo.SetExportType(expft);
	m_rowInfo.m_arrayColumn.Add (m_pDummyColumn);
	while (pos != NULL)
	{
		CaColumnExport* pCol = (CaColumnExport*)listLayout.GetNext (pos);
		m_rowInfo.m_arrayColumn.Add (pCol);
	}

	pos = listColum.GetHeadPosition(); // Use the initial value to display the list control header
	CString strCol;
	int i, nSize = listColum.GetCount() +1; // +1 means that we add an extra column at position 0:
	LSCTRLHEADERPARAMS* pLsp = new LSCTRLHEADERPARAMS[nSize];
	for (i=0; i<nSize; i++)
	{
		if (i==0)
		{
			pLsp[i].m_fmt = LVCFMT_LEFT;
			lstrcpy (pLsp[i].m_tchszHeader, _T(""));
			pLsp[i].m_cxWidth= 160;
			pLsp[i].m_bUseCheckMark = FALSE;
			continue;
		}

		if (pos == NULL)
			break;
		pLsp[i].m_fmt = LVCFMT_LEFT;
		pLsp[i].m_cxWidth= 100;
		pLsp[i].m_bUseCheckMark = FALSE;

		CaColumn* pCol = listColum.GetNext(pos);
		strCol = pCol->GetName();
		if (strCol.GetLength() > MAXLSCTRL_HEADERLENGTH)
			lstrcpyn (pLsp[i].m_tchszHeader, strCol, MAXLSCTRL_HEADERLENGTH);
		else
			lstrcpy  (pLsp[i].m_tchszHeader, strCol);
	}
	m_wndHeaderRows.InitializeHeader(pLsp, nSize);
	m_wndHeaderRows.SetDisplayFunction(DisplayExportListCtrlLower, NULL);
	delete pLsp;


	CuListCtrlDoubleUpper& listCtrlHeader = m_wndHeaderRows.GetListCtrlInfo();
	int iIndex = -1;
	int nCount = listCtrlHeader.GetItemCount();
	for (i=0; i<m_wndHeaderRows.GetFixedRowInfo(); i++)
	{
		iIndex = listCtrlHeader.InsertItem (nCount, _T("xxx"));
		if (iIndex != -1)
		{
			listCtrlHeader.SetItemData (iIndex, (DWORD)&m_rowInfo);
			listCtrlHeader.SetFgColor  (iIndex, RGB(0, 0, 255));
		}
		nCount = listCtrlHeader.GetItemCount();
	}
	listCtrlHeader.DisplayItem ();

	CPtrArray& arrayRecord = result.GetListRecord();
	m_wndHeaderRows.SetSharedData(&arrayRecord);
	m_wndHeaderRows.ResizeToFit();
}


BOOL CuIeaPPage2CommonData::Findish(CaIEAInfo& data)
{
	BOOL bOk = FALSE;
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CaIeaDataPage2& dataPage2 = data.m_dataPage2;

	switch (dataPage1.GetExportedFileType())
	{
	case EXPORT_CSV:
		bOk = ExportToCsv(&data);
		break;
	case EXPORT_DBF:
		bOk = ExportToDbf(&data);
		break;
	case EXPORT_XML:
		bOk = ExportToXml(&data);
		break;
	case EXPORT_FIXED_WIDTH:
		bOk = ExportToFixedWidth(&data);
		break;
	case EXPORT_COPY_STATEMENT:
		bOk = ExportToCopyStatement(&data);
		break;
	default:
		//
		// You must define new type.
		ASSERT(FALSE);
		return FALSE;
	}


	return bOk;
}



void CuIeaPPage2CommonData::SaveExportParameter(CWnd* pWnd)
{
	CuIEAPropertySheet* pParent = (CuIEAPropertySheet*)pWnd->GetParent();
	CaIEAInfo& data = pParent->GetData();

	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
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
		strFullName += _T(".ii_exp");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("ii_exp");
		else
		if (strExt.CompareNoCase (_T("ii_exp")) != 0)
			strFullName += _T(".ii_exp");
	}
	data.SaveData (strFullName);
}


//
// Here we have an extra column at the position 0 = "Source Data".
void CALLBACK DisplayExportListCtrlLower(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo)
{
	CString strSourceData;
	strSourceData.LoadString(IDS_SOURCE_DATA);
	CaRowTransfer* pRecord = (CaRowTransfer*)pItemData;
	CStringList& listFields = pRecord->GetRecord();

	int i, nSize = listFields.GetCount();
	int nIdx = nItem;
	int nCount = pListCtrl->GetItemCount();
	ASSERT (nItem >= 0 && nItem < pListCtrl->GetCountPerPage());
	if (!bUpdate || nIdx >= nCount)
		nIdx = pListCtrl->InsertItem (nItem, strSourceData);

	i = 0;
	POSITION pos = listFields.GetHeadPosition();
	while (pos != NULL)
	{
		CString strField = listFields.GetNext (pos);
		if (nIdx != -1)
		{
			pListCtrl->SetItemText (nIdx, i+1, strField);
		}
		i++;
	}
}

