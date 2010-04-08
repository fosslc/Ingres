/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdafrm.cpp : implementation file (Manual Created Frame).
**    Project  : INGRES II/VSDA Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for Visual Schema Diff control windows
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
** 17-Nov-2004 (uk$so01)
**    SIR #113475, Just replace the field separator form ";" to "," in the CSV
** 30-Mar-2005 (komve01)
**    BUG #114176/Issue# 13941970
**    Added additional checks to validate the path entered by the user
**    and an appropriate message is displayed indicating the cause of failure.
*/

#include "stdafx.h"
#include <htmlhelp.h>
#include "vsda.h"
#include "vsdafrm.h"
#include "viewleft.h"
#include "viewrigh.h"
#include "vsdadoc.h"
#include "dgdiffls.h"
#include "ingobdml.h"
#include "usrexcep.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);
IMPLEMENT_DYNCREATE(CfSdaFrame, CFrameWnd)
void CfSdaFrame::Init()
{
	m_pSdaDoc = NULL;
	m_bAllViewCreated = FALSE;
}

CfSdaFrame::CfSdaFrame()
{
	Init();
}

CfSdaFrame::~CfSdaFrame()
{
}

CWnd* CfSdaFrame::GetLeftPane()
{
	return m_WndSplitter.GetPane (0, 0);
}

CWnd* CfSdaFrame::GetRightPane () 
{
	return m_WndSplitter.GetPane (0, 1);
}


BEGIN_MESSAGE_MAP(CfSdaFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfSdaFrame)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfSdaFrame message handlers

BOOL CfSdaFrame::DoCompare (short nMode)
{
	CvSdaLeft* pVleft = (CvSdaLeft*)CfSdaFrame::GetLeftPane();
	ASSERT(pVleft);
	if (pVleft)
	{
		return pVleft->DoCompare(nMode);
	}

	return FALSE;
}

BOOL CfSdaFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	BOOL bOK = TRUE;
	CCreateContext context1, context2;
	int ncxCur = 200, ncxMin = 10;

	if (!m_pSdaDoc)
		m_pSdaDoc  = new CdSdaDoc();
	CRuntimeClass* pViewRight= RUNTIME_CLASS(CvSdaRight);
	CRuntimeClass* pViewLeft = RUNTIME_CLASS(CvSdaLeft);
	context1.m_pNewViewClass = pViewLeft;
	context1.m_pCurrentDoc   = m_pSdaDoc;
	context2.m_pNewViewClass = pViewRight;
	context2.m_pCurrentDoc   = context1.m_pCurrentDoc;

	// Create a splitter of 1 rows and 2 columns.
	if (!m_WndSplitter.CreateStatic (this, 1, 2)) 
	{
		TRACE0 ("CfSdaFrame::OnCreateClient: Failed to create Splitter\n");
		return FALSE;
	}

	// associate the default view (CvSdaLeft) with pane 0
	if (!m_WndSplitter.CreateView (0, 0, context1.m_pNewViewClass, CSize (400, 300), &context1)) 
	{
		TRACE0 ("CfSdaFrame::OnCreateClient: Failed to create first pane\n");
		return FALSE;
	}

	// associate the CvSdaRight view with pane 1
	if (!m_WndSplitter.CreateView (0, 1, context2.m_pNewViewClass, CSize (50, 300), &context2)) 
	{
		TRACE0 ("CfSdaFrame::OnCreateClient: Failed to create second pane\n");
		return FALSE;
	}

	m_WndSplitter.SetColumnInfo(0, ncxCur, ncxMin);
	m_WndSplitter.RecalcLayout();
	
	m_bAllViewCreated = TRUE;
	return bOK;
}

BOOL CfSdaFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo(m_hWnd, 40050);
	
	return CFrameWnd::OnHelpInfo(pHelpInfo);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vdda.chm");
	}

	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}


BOOL CfSdaFrame::DoWriteFile()
{
	CvSdaLeft* pVleft = (CvSdaLeft*)GetLeftPane();
	int nExceptionCause = 0;
	ASSERT(pVleft);
	if (!pVleft)
		return FALSE;
	try
	{
		if (pVleft)
		{
			CTreeCtrl& treeCtrl = pVleft->GetTreeCtrl();
			HTREEITEM hRoot = treeCtrl.GetRootItem();
			CtrfItemData* pItem = (CtrfItemData*)treeCtrl.GetItemData(hRoot);

			if (ExistDifferences())
			{
				//
				// Prompt for file name:
				TCHAR tchszFilters[] = _T("CSV File (*.csv)|*.csv||");
				BOOL b2AppendExt = FALSE;
				CString strFullName, strExt;
				CFileDialog dlg(
					FALSE,
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, tchszFilters);
				dlg.m_ofn.Flags &= ~(OFN_EXPLORER); 
				dlg.m_ofn.nFilterIndex = 1;
				if (dlg.DoModal() != IDOK)
					return FALSE; 

				short nMode = 0;
				strFullName = dlg.GetPathName ();
				strExt = dlg.GetFileExt();
				CString strDir = strFullName.Left(strFullName.ReverseFind('\\'));
				if (strExt.CompareNoCase(_T("xml")) == 0)
					nMode = 1;

				if( (_access( strDir.GetBuffer(strDir.GetLength()), 0 )) == -1 )
				{
					//Invalid directory
					nExceptionCause = CFileException::badPath;
				}

				CFile file (strFullName, CFile::modeCreate|CFile::modeWrite);
				CaVsdDisplayInfo* pDisplayInfo = (CaVsdDisplayInfo*)pItem->GetDisplayInfo();
				ASSERT(pDisplayInfo);
				if (pDisplayInfo)
				{
					CString strHeader;
					CString s1, s2, s3;
					s1.LoadString(IDS_TAB_OBJECT);
					s2.LoadString(IDS_TAB_TYPE);
					s3.LoadString(IDS_TAB_DIFF);
					if (nMode == 0)
					{
						strHeader.Format(_T("\"%s\",\"%s\",\"%s\",%s,%s%s"),
							s1,
							s2,
							s3,
							INGRESII_llQuote(pDisplayInfo->GetCompareTitle1()),
							INGRESII_llQuote(pDisplayInfo->GetCompareTitle2()),
							consttchszReturn);

						file.Write((LPCTSTR)strHeader, strHeader.GetLength());
					}
				}

				WriteFile(file, pItem, 3);
			}
			else
			{
				//
				// There is no difference to be written.
				AfxMessageBox(IDS_MSG_NODIFF_4WRITE);
			}
		}
	}
	catch (CFileException* e)
	{
		e->m_cause = nExceptionCause;
		MSGBOX_FileExceptionMessage(e);
		//e->ReportError();
		//e->Delete();
		return FALSE;
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}


void CfSdaFrame::WriteFile (CFile& file, CtrfItemData* pItem, int nDepth)
{
	if (!pItem->IsFolder())
	{
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		//
		// Write the differences related to the object:
		WriteFileItem(file, pVsdItemData, pItem);
		//
		// Display the differences related to the object's sub-branches:
		if (nDepth > 0)
		{
			CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
			POSITION pos = lo.GetHeadPosition();
			while (pos != NULL)
			{
				CtrfItemData* pIt = lo.GetNext(pos);
				WriteFile (file, pIt, pIt->IsFolder()? nDepth: nDepth - 1);
			}
		}
	}
	else
	{
		CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
		POSITION pos = lo.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* pIt = lo.GetNext(pos);
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pIt->GetUserData();
			if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
			{
				WriteFileItem(file, pVsdItemData, pIt);
			}
			else
			{
				if (nDepth > 0)
					WriteFile (file, pIt, nDepth-1);
				else
					WriteFileItem(file, pVsdItemData, pIt);
			}
		}
	}
}

void CfSdaFrame::WriteFileItem (CFile& file, CaVsdItemData* pVsdItemData, CtrfItemData* pItem)
{
	CString strLine = _T("");
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	CString strObjectType = _T("");
	CString strTemp;
	int nImage = -1;
	QueryImageID(pItem, nImage, strObjectType);
	CString strName = GetFullObjectName(pItem);

	if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
	{
		strTemp.LoadString(IDS_AVAILABLE);
		BOOL bQueryOwner = QueryOwner(nObjectType);
		CString strAvailable = bQueryOwner? pVsdItemData->GetOwner(): strTemp;

		strTemp.Format(_T("%s,\"%s\","), 
		    (LPCTSTR)INGRESII_llQuote(strName), 
		     strObjectType);
		strLine = strTemp;

		CString csMissingObj,csNotAvailable;
		csNotAvailable.LoadString(IDS_NOT_AVAILABLE);
		csMissingObj.LoadString(IDS_MISSING_OBJ);
		if (pVsdItemData->GetDifference() == _T('+'))
		{
			strTemp.Format(_T("\"%s\",\"%s\",\"%s\"%s"), 
			    (LPCTSTR)csMissingObj, 
			    (LPCTSTR)strAvailable, 
			    (LPCTSTR)csNotAvailable,
				consttchszReturn);
			strLine += strTemp;
		}
		else
		if (pVsdItemData->GetDifference() == _T('-'))
		{
			strTemp.Format(_T("\"%s\",\"%s\",\"%s\"%s"), 
			    (LPCTSTR)csMissingObj, 
			    (LPCTSTR)csNotAvailable, 
			    (LPCTSTR)strAvailable,
				consttchszReturn);
			strLine += strTemp;
		}
		file.Write((LPCTSTR)strLine, strLine.GetLength());
	}
	else
	if (pVsdItemData->GetDifference() == _T('#') || pVsdItemData->GetDifference() == _T('*'))
	{
		CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pVsdItemData->GetListDifference();
		{
			POSITION pos = listDiff.GetHeadPosition();
			while (pos != NULL)
			{
				CaVsdDifference* pDiff = listDiff.GetNext(pos);
				strTemp.Format(_T("%s,\"%s\",\"%s\",%s,%s%s"), 
				    (LPCTSTR)INGRESII_llQuote(strName), 
				    strObjectType, 
				    pDiff->GetType(),
				    (LPCTSTR)INGRESII_llQuote(pDiff->GetInfoSchema1()),
				    (LPCTSTR)INGRESII_llQuote(pDiff->GetInfoSchema2()),
					consttchszReturn);
				file.Write((LPCTSTR)strTemp, strTemp.GetLength());
			}
		}
	}
}


BOOL CfSdaFrame::ExistDifferences ()
{
	CvSdaLeft* pVleft = (CvSdaLeft*)GetLeftPane();
	ASSERT(pVleft);
	if (!pVleft)
		return FALSE;
	try
	{
		if (pVleft)
		{
			CTreeCtrl& treeCtrl = pVleft->GetTreeCtrl();
			HTREEITEM hRoot = treeCtrl.GetRootItem();
			CtrfItemData* pItem = (CtrfItemData*)treeCtrl.GetItemData(hRoot);
			BOOL bHas = FALSE;
			HasDifference(pItem, 3, bHas);
			return bHas;
		}
	}
	catch(...)
	{
		return FALSE;
	}
	return TRUE;
}

void CfSdaFrame::HasDifference (CtrfItemData* pItem, int nDepth, BOOL& bHas)
{
	if (!pItem->IsFolder())
	{
		CaVsdItemData* pVsdItemData = (CaVsdItemData*)pItem->GetUserData();
		HasDifferenceItem(pVsdItemData, pItem, bHas);
		if (nDepth > 0)
		{
			CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
			POSITION pos = lo.GetHeadPosition();
			while (pos != NULL)
			{
				CtrfItemData* pIt = lo.GetNext(pos);
				HasDifference (pIt, pIt->IsFolder()? nDepth: nDepth - 1, bHas);
			}
		}
	}
	else
	{
		CTypedPtrList< CObList, CtrfItemData* >& lo = pItem->GetListObject();
		POSITION pos = lo.GetHeadPosition();
		while (pos != NULL)
		{
			CtrfItemData* pIt = lo.GetNext(pos);
			CaVsdItemData* pVsdItemData = (CaVsdItemData*)pIt->GetUserData();
			if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
			{
				HasDifferenceItem(pVsdItemData, pIt, bHas);
			}
			else
			{
				if (nDepth > 0)
					HasDifference (pIt, nDepth-1, bHas);
				else
					HasDifferenceItem(pVsdItemData, pIt, bHas);
			}
		}
	}
}

void CfSdaFrame::HasDifferenceItem (CaVsdItemData* pVsdItemData, CtrfItemData* pItem, BOOL& bHas)
{
	CString strLine = _T("");
	int nObjectType = pItem->GetDBObject()->GetObjectID();
	CString strObjectType = _T("");
	CString strTemp;
	int nImage = -1;
	QueryImageID(pItem, nImage, strObjectType);
	CString strName = GetFullObjectName(pItem);

	if (pVsdItemData->GetDifference() == _T('+') || pVsdItemData->GetDifference() == _T('-'))
	{
		bHas = TRUE;
		return;
	}
	else
	if (pVsdItemData->GetDifference() == _T('#') || pVsdItemData->GetDifference() == _T('*'))
	{
		CTypedPtrList< CObList, CaVsdDifference* >& listDiff = pVsdItemData->GetListDifference();
		POSITION pos = listDiff.GetHeadPosition();
		while (pos != NULL)
		{
			bHas = TRUE;
			return;
		}
	}
}

