/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpagexml.cpp : implementation file 
**    Project  : INGRES II / VDBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The result tab of SQL/Test that show XML/XML Resultset.
**
** History:
**	05-Jul-2001 (uk$so01)
**	    Created
**	    SIR #105199. Integrate & Generate XML.
**	23-Oct-2001 (uk$so01)
**	    SIR #106057
**	    Transform code to be an ActiveX Control
**	27-May-2002 (uk$so01)
**	    BUG #107880, Add the XML decoration encoding='UTF-8' or
**	    'WINDOWS-1252'...  depending on the Ingres II_CHARSETxx.
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**	    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**	    Write()/Read() can also write more than 64K-1 bytes of data.
*/

#include "stdafx.h"
#include  <io.h>
#include "rcdepend.h"
#include "qpagexml.h"
#include "qframe.h"
#include "webbrows.h"
#include "taskxprm.h"
#include "lsselres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgSqlQueryPageXML::CuDlgSqlQueryPageXML(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageXML::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgSqlQueryPageXML)
	m_strStatement = _T("");
	m_strDatabase = _T("");
	//}}AFX_DATA_INIT
	m_bFirstRun  = TRUE;
	m_bXMLSource = TRUE;
	m_bShowStatement = FALSE;
	m_strPreview = _T("Tabular Form");
	m_pQueryRowParam = new CaExecParamGenerateXMLfromSelect();
	m_pQueryRowParam->SetEncoding(theApp.GetCharacterEncoding());
}


void CuDlgSqlQueryPageXML::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageXML)
	DDX_Control(pDX, IDC_EDIT2, m_cEditStatement);
	DDX_Control(pDX, IDC_EDIT1, m_cEditDatabase);
	DDX_Control(pDX, IDC_STATIC1, m_cStaticDatabase);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonXML);
	DDX_Text(pDX, IDC_EDIT1, m_strDatabase);
	DDX_Text(pDX, IDC_EDIT2, m_strStatement);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageXML, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageXML)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonXML)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonSave)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_PAGE_SHOWHEADER, OnDisplayHeader)
	ON_MESSAGE (WM_SQL_QUERY_PAGE_HIGHLIGHT,  OnHighlightStatement)
	ON_MESSAGE (WM_SQL_STATEMENT_SHOWRESULT,  OnDisplayResult)
	ON_MESSAGE (WM_SQL_GETPAGE_DATA,          OnGetData)
	ON_MESSAGE (WMUSRMSG_SQL_FETCH,           OnFetchRows)
	ON_MESSAGE (WM_SQL_TAB_BITMAP,            OnGetTabBitmap)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,      OnChangeSetting)
END_MESSAGE_MAP()

BOOL CuDlgSqlQueryPageXML::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pFrame->GetActiveDocument();
	if (pDoc)
		m_bXMLSource = !(pDoc->GetProperty().GetXmlDisplayMode() == 1);
	else
		m_bXMLSource = FALSE;
	m_pQueryRowParam->SetMode(m_bXMLSource);
	m_strPreview.LoadString (IDS_PREVIEW);
	m_cButtonXML.GetWindowText (m_strXmlSource);
	if (m_bXMLSource)
		m_cButtonXML.SetWindowText (m_strPreview);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageXML::NotifyLoad (CaQueryXMLPageData* pData)
{
	ASSERT (pData);
	if (!pData)
		return;

	CfSqlQueryFrame* pParent = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT_VALID (pParent);
	if (!pParent)
		return;
	CdSqlQueryRichEditDoc*  pDoc = pParent->GetSqlDocument();

	m_bShowStatement = pData->m_bShowStatement;
	m_nStart         = pData->m_nStart;
	m_nEnd           = pData->m_nEnd;
	m_strStatement   = pData->m_strStatement;
	m_strDatabase    = pData->m_strDatabase;
	m_bXMLSource     = pData->m_pParam->GetMode();
	m_bXMLSource     = !m_bXMLSource; // Because OnButtonXML() will invert this value.
	m_pQueryRowParam->Copy(*(pData->m_pParam));

	UpdateData (FALSE);
	OnButtonXML();
}

void CuDlgSqlQueryPageXML::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
	if (pIe && IsWindow (pIe->m_hWnd))
	{
		CRect rcDlg, rcExp;
		GetClientRect (rcDlg);

		pIe->GetWindowRect (rcExp);
		ScreenToClient(rcExp);
		rcExp.right = rcDlg.right -1;
		rcExp.bottom = rcDlg.bottom -1;
		pIe->MoveWindow (rcExp);
	}
	if (m_bShowStatement)
	{
		if (IsWindow(m_cStaticDatabase.m_hWnd))
			m_cStaticDatabase.ShowWindow (SW_SHOW);
		if (IsWindow(m_cEditDatabase.m_hWnd))
			m_cEditDatabase.ShowWindow (SW_SHOW);
		if (IsWindow(m_cEditStatement.m_hWnd))
			m_cEditStatement.ShowWindow (SW_SHOW);
	}
}

void CuDlgSqlQueryPageXML::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgSqlQueryPageXML::OnButtonXML() 
{
	m_bXMLSource = !m_bXMLSource;
	try
	{
		CWaitCursor doWaitCursor;
		ASSERT(m_pQueryRowParam);
		if (!m_pQueryRowParam)
			return;
		if (m_bXMLSource)
			m_cButtonXML.SetWindowText (m_strPreview);
		else
			m_cButtonXML.SetWindowText (m_strXmlSource);

		m_pQueryRowParam->SetMode(m_bXMLSource);
		m_pQueryRowParam->Run(m_hWnd);
		CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
		if (pIe && IsWindow (pIe->m_hWnd))
		{
			pIe->ShowWindow(SW_SHOW);
			CString strFile = m_bXMLSource? m_pQueryRowParam->GetFileXML(): m_pQueryRowParam->GetFileXSL();
			if (!strFile.IsEmpty())
			{
#if defined(MAINWIN)
				CString noBrowser =_T("Unable to display XML, saved to ") + strFile;
				AfxMessageBox (noBrowser, MB_ICONWARNING | MB_OK);
#else
				COleVariant vaURL(strFile);
				pIe->Navigate2 (vaURL, 0, NULL, NULL, NULL);
#endif
			}
		}
	}
	catch(...)
	{
		AfxMessageBox (_T("CuDlgSqlQueryPageXML::OnButtonXML(): unknown error"));
	}
}

void CuDlgSqlQueryPageXML::OnButtonSave() 
{
	CWaitCursor doWaitCursor;
	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("XML Files (*.xml)|*.xml||"));

	ASSERT(m_pQueryRowParam);
	if (!m_pQueryRowParam)
		return;
	if (dlg.DoModal() != IDOK)
		return; 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".xml");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("xml");
		else
		if (strExt.CompareNoCase (_T("xml")) != 0)
			strFullName += _T(".xml");
	}
	if (strFullName.IsEmpty())
		return;

	if (m_bXMLSource || m_pQueryRowParam->IsAlreadyGeneratedXML())
	{
		CopyFile (m_pQueryRowParam->GetFileXML(), strFullName, FALSE);
	}
	else
	{
		const int nSize = 2048;
		TCHAR tchszBuffer[nSize + 1];
		CFile rf(m_pQueryRowParam->GetFileXSL(), CFile::modeRead);
		CFile wf(strFullName, CFile::modeCreate|CFile::modeWrite);

		//
		// Construct XML file from from the translated XMLXSL file:
		DWORD dw = rf.Read(tchszBuffer, nSize);
		tchszBuffer[dw] = _T('\0');
		BOOL bFoundStylesheet = FALSE;
		CString strLine = _T("");
		while (dw > 0)
		{
			strLine += tchszBuffer;

			if (!bFoundStylesheet)
			{
				int nPos0x0d = strLine.Find((TCHAR)0x0d);
				while (nPos0x0d != -1)
				{
					CString strHeaderLine = strLine.Left(nPos0x0d);
					strHeaderLine.MakeLower();
					if (strHeaderLine.Find(_T("<?xml-stylesheet type")) == 0)
					{
						bFoundStylesheet = TRUE;
					}
					else
					{
						strHeaderLine = strLine.Left(nPos0x0d);
						wf.Write ((const void*)(LPCTSTR)strHeaderLine, strHeaderLine.GetLength());
						wf.Write ((const void*)consttchszReturn, _tcslen(consttchszReturn));
					}

					if (strLine.GetAt(nPos0x0d+1) == 0x0a)
						strLine = strLine.Mid(nPos0x0d+2);
					else
						strLine = strLine.Mid(nPos0x0d+1);
					if (strLine.IsEmpty())
					{
						dw = rf.Read(tchszBuffer, nSize);
						tchszBuffer[dw] = _T('\0');

						break;
					}

					if (bFoundStylesheet)
						break;
					nPos0x0d = strLine.Find((TCHAR)0x0d);
				}
				dw = rf.Read(tchszBuffer, nSize);
				tchszBuffer[dw] = _T('\0');
			}
			else
			{
				wf.Write((const void*)(LPCTSTR)strLine, strLine.GetLength());
				strLine = _T("");

				dw = rf.Read(tchszBuffer, nSize);
				tchszBuffer[dw] = _T('\0');
			}
		}

		wf.Flush();
	}
}


LONG CuDlgSqlQueryPageXML::OnDisplayHeader (WPARAM wParam, LPARAM lParam)
{
	m_bShowStatement = TRUE;
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
	return 0L;
}

LONG CuDlgSqlQueryPageXML::OnHighlightStatement (WPARAM wParam, LPARAM lParam)
{
	CfSqlQueryFrame* pParent = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT_VALID (pParent);
	if (!pParent)
		return 0L;
	CvSqlQueryRichEditView* pView2 = pParent->GetRichEditView();
	CdSqlQueryRichEditDoc*  pDoc = pParent->GetSqlDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return 0L;

	pView2->SetColor (-1,  0, pDoc->m_crColorText);
	if (m_bShowStatement)
		return 0L;
	pView2->SetColor (m_nStart, m_nEnd, pDoc->m_crColorSelect);
	return 0L;
}

LONG CuDlgSqlQueryPageXML::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaQueryXMLPageData* pData    = NULL;
	try
	{
		pData    = new CaQueryXMLPageData();
		pData->m_nID            = IDD;
		pData->m_bShowStatement = m_bShowStatement;
		pData->m_strStatement   = m_strStatement;
		pData->m_nStart         = m_nStart;
		pData->m_nEnd           = m_nEnd;
		pData->m_strDatabase    = m_strDatabase;
		pData->m_nTabImage      = (int)OnGetTabBitmap(0, 0);
		pData->m_pParam->Copy(*m_pQueryRowParam);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return (LONG)NULL;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_GETDATA_FOR_SAVE, MB_ICONEXCLAMATION|MB_OK);
		if (pData)
			delete pData;
		return (LONG)NULL;
	}

	return (LONG)pData;
}

LONG CuDlgSqlQueryPageXML::OnGetTabBitmap (WPARAM wParam, LPARAM lParam)
{
	return (LONG)BM_XML;
}

LONG CuDlgSqlQueryPageXML::OnDisplayResult (WPARAM wParam, LPARAM lParam)
{
	CaSqlQuerySelectData* pData = (CaSqlQuerySelectData*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0L;
	ASSERT (m_pQueryRowParam);
	if (!m_pQueryRowParam)
		return 0L;
	CWaitCursor waitCursor;
	try
	{
		CaSession* pCurrentSession = m_pQueryRowParam->GetSession();
		pCurrentSession->SetSessionNone();

		CString strTitle;
		strTitle.LoadString(IDS_TITLE_FETCHING_ROW);
#if defined (_ANIMATION_)
		CxDlgWait dlgWait(strTitle, NULL);
		dlgWait.SetDeleteParam(FALSE);
		dlgWait.SetExecParam (m_pQueryRowParam);
		dlgWait.SetUseAnimateAvi(AVI_FETCHR);
		dlgWait.SetUseExtraInfo();
		dlgWait.SetShowProgressBar(FALSE);
		dlgWait.SetHideCancelBottonAlways(TRUE);
		dlgWait.DoModal();
#else
		m_pQueryRowParam->Run();
#endif
		CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
		if (pIe && IsWindow (pIe->m_hWnd))
		{
			pIe->ShowWindow(SW_SHOW);
			CString strFile = m_bXMLSource? m_pQueryRowParam->GetFileXML(): m_pQueryRowParam->GetFileXSL();
			if (!strFile.IsEmpty() && _taccess(strFile, 0) == 0)
			{
				COleVariant vaURL(strFile);
				pIe->Navigate2 (vaURL, 0, NULL, NULL, NULL);
			}
		}
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (_T("CuDlgSqlQueryPageXML::OnDisplayResult: unknown error"));
	}
	return (LONG)0;
}


void CuDlgSqlQueryPageXML::OnDestroy()
{
	if (m_pQueryRowParam)
		delete m_pQueryRowParam;
	m_pQueryRowParam = NULL;
	CDialog::OnDestroy();
}

LONG CuDlgSqlQueryPageXML::OnFetchRows (WPARAM wParam, LPARAM lParam)
{
	int nStatus = (int)wParam;
	switch (nStatus)
	{
	case CaExecParamQueryRows::FETCH_NORMAL_ENDING:
		// LPARAM = (Total number of rows fetched)
		break;
	case CaExecParamQueryRows::FETCH_REACH_LIMIT:
		break;
	case CaExecParamQueryRows::FETCH_ERROR:
		break;
	case CaExecParamQueryRows::FETCH_TRACEINFO:
		{
			CaExecParamQueryRows* pQueryRowParam = (CaExecParamQueryRows*)lParam;
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
			if (pFrame && pQueryRowParam)
			{
				CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
				if (pDlgResult)
				{
					CuDlgSqlQueryPageRaw* pRawPage = pDlgResult->GetRawPage();
					ASSERT (pRawPage);
					CString strNew = (LPCTSTR)lParam;
					pDlgResult->DisplayTraceLine (strNew, NULL);
				}
			}
		}
		break;

	default:
		break;
	}
	return 0;
}

long CuDlgSqlQueryPageXML::OnChangeSetting(WPARAM wParam, LPARAM lParam)
{
	CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)lParam;
	if (pDoc)
	{
		CaSqlQueryProperty& property = pDoc->GetProperty();
		if (property.GetFont())
		{
			CuWebBrowser2* pIe = (CuWebBrowser2*)GetDlgItem (IDC_EXPLORER1);
			if (pIe && IsWindow (pIe->m_hWnd))
			{
				pIe->SendMessage (WM_SETFONT, (WPARAM)property.GetFont(), MAKELPARAM(TRUE, 0));
			}
		}
	}
	return 0;
}
