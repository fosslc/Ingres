/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : fformat3.cpp : implementation file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Page of Tab (CSV format)
**
** History:
**
** 26-Dec-2000 (uk$so01)
**    Created
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Cleanup + make code homogeneous.
**	06-Feb-2003 (hanch04)
**	    Don't do animation if built using MAINWIN
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 25-Nov-2004 (uk$so01)
**    BUG #113532: IIA displays incorrectly the separator character if it has an accent.
** 10-Mar-2010 (drivi01) SIR 123397
**    If IIA has only 1 version of imported format, then check 
**    the tab for selection automatically.
**/


#include "stdafx.h"
#include "svriia.h"
#include "formdata.h"
#include "fformat3.h"
#include "wmusrmsg.h"
#include "iapsheet.h"
#include "taskanim.h"
#include "colorind.h" // UxCreateFont

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgPageCsv::CuDlgPageCsv(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgPageCsv::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgPageCsv)
	//}}AFX_DATA_INIT

	m_pData = NULL;
	m_pSeparatorSet = NULL;
	m_pSeparator = NULL;

	memset (&m_sizeText, 0, sizeof(m_sizeText));
	m_sizeText.cx =  9;
	m_sizeText.cx = 16;
	m_nCoumnCount = -1;
	m_bFirstUpdate = TRUE;
}


void CuDlgPageCsv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgPageCsv)
	DDX_Control(pDX, IDC_CHECK3, m_cCheckIgnoreTrailingSeparator);
	DDX_Control(pDX, IDC_CHECK2, m_cCheckConfirmChoice);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_COMBO2, m_cComboTextQualifier);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckConsecutiveSeparatorAsOne);
	DDX_Control(pDX, IDC_EDIT1, m_cEditSeparator);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgPageCsv, CDialog)
	//{{AFX_MSG_MAP(CuDlgPageCsv)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK2, OnCheckConfirmChoice)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CLEANDATA,  OnCleanData)
	ON_MESSAGE (WMUSRMSG_GETITEMDATA,OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgPageCsv message handlers

BOOL CuDlgPageCsv::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	try
	{
		CRect r;
		m_cStatic1.GetWindowRect (r);
		ScreenToClient (r);
		m_listCtrlHuge.CreateEx (WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, r, this, IDC_LIST1);
		m_listCtrlHuge.MoveWindow (r);
		m_listCtrlHuge.ShowWindow (SW_SHOW);
		m_listCtrlHuge.SetFullRowSelected(TRUE);
		m_listCtrlHuge.SetLineSeperator(TRUE, TRUE, FALSE);
		m_listCtrlHuge.SetAllowSelect(FALSE);
	}
	catch (...)
	{

	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgPageCsv::SetFormat (CaIIAInfo* pData, CaSeparatorSet* pSet, CaSeparator* pSeparator)
{
	m_pData = pData;
	m_pSeparatorSet = pSet;
	m_pSeparator = pSeparator;
}


//
// wParam = CHECK_CONFIRM_CHOICE (Update the control m_cCheckConfirmChoice. lParam = 0|1)
// wParam = 0 (General, lParam = address of class CaIIAInfo)
LONG CuDlgPageCsv::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	if (nMode == CHECK_CONFIRM_CHOICE)
	{
		int nCheck = (int)lParam;
		m_cCheckConfirmChoice.SetCheck (nCheck);
		return 0;
	}

	ASSERT (m_pData);
	ASSERT (m_pSeparatorSet);
	if (!(m_pData && m_pSeparatorSet))
		return 0;

	CaDataPage1& dataPage1 = m_pData->m_dataPage1;
	CaDataPage2& dataPage2 = m_pData->m_dataPage2;
	CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
	int i, nSize = arrayRecord.GetSize();
	for (i=0; i<nSize; i++)
	{
		CaRecord* pRec = (CaRecord*)arrayRecord.GetAt(i);
		delete pRec;
	}
	arrayRecord.RemoveAll();
	m_listCtrlHuge.ResizeToFit();

	CaExecParamQueryRecords* p = new CaExecParamQueryRecords(m_pData, m_pSeparatorSet, &m_nCoumnCount);
#if defined (_ANIMATION_)
	CString strMsgAnimateTitle;
	strMsgAnimateTitle.LoadString(IDS_ANIMATE_TITLE_QUERYING);
	CxDlgWait dlg (strMsgAnimateTitle);
	dlg.SetUseAnimateAvi(AVI_CLOCK);
	dlg.SetExecParam (p);
	dlg.SetDeleteParam(FALSE);
	dlg.SetShowProgressBar(TRUE);
	dlg.SetUseExtraInfo();
	dlg.DoModal();
#else
	p->Run();
	p->OnTerminate(m_hWnd);
#endif
	delete p;
	SetForegroundWindow ();

	m_bFirstUpdate = FALSE;
	//
	// Display the separator:
	DisplaySeparatorSetInfo(m_pSeparatorSet, m_pSeparator);

	//
	// Initialize headers:
	CString strHeader;
	if (m_nCoumnCount > 0)
	{
		LSCTRLHEADERPARAMS* pLsp = new LSCTRLHEADERPARAMS[m_nCoumnCount];
		CStringArray arrayHeader;
		BOOL bFirstLineAsHeader = GenerateHeader(dataPage1, m_pSeparatorSet, m_pSeparator, m_nCoumnCount, arrayHeader);
		for (i=0; i<m_nCoumnCount; i++)
		{
			if (bFirstLineAsHeader)
				strHeader = arrayHeader.GetAt(i);
			else
				strHeader.Format(_T("col%d"), i+1);

#if defined (MAXLENGTH_AND_EFFECTIVE)
			int& nFieldMax = dataPage2.GetFieldSizeMax(i);
			int& nFieldEff = dataPage2.GetFieldSizeEffectiveMax(i);
			//if (nFieldEff < nFieldMax)
			//	strHeader.Format(_T("col%d (%d+%d)"), i+1, nFieldEff, nFieldMax-nFieldEff);
			//else
			CString strSize;
			strSize.Format(_T(" (%d)"), nFieldMax);
			strHeader += strSize;
#endif
			pLsp[i].m_fmt = LVCFMT_LEFT;
			if (strHeader.GetLength() > MAXLSCTRL_HEADERLENGTH)
				lstrcpyn (pLsp[i].m_tchszHeader, strHeader, MAXLSCTRL_HEADERLENGTH);
			else
				lstrcpy  (pLsp[i].m_tchszHeader, strHeader);
			pLsp[i].m_cxWidth= 100;
			pLsp[i].m_bUseCheckMark = FALSE;
		}

		m_listCtrlHuge.InitializeHeader(pLsp, m_nCoumnCount);
		m_listCtrlHuge.SetDisplayFunction(DisplayListCtrlHugeItem, NULL);
		m_listCtrlHuge.SetSharedData (&arrayRecord);
		delete pLsp;
	}

	//
	// Display the new records:
	m_listCtrlHuge.ResizeToFit();

	//If one tab displays for import format, then just check 
	//the tab checkbox automatically
	CTabCtrl *cTab = (CTabCtrl *) this->GetParent();
	int size = cTab->GetItemCount();
	if (cTab->GetItemCount()==1)
		m_cCheckConfirmChoice.SetCheck(TRUE);

	return TRUE;
}

LONG CuDlgPageCsv::OnCleanData (WPARAM wParam, LPARAM lParam)
{
	ASSERT (m_pData);
	CaDataPage1& dataPage1 = m_pData->m_dataPage1;
	CaDataPage2& dataPage2 = m_pData->m_dataPage2;

	CPtrArray& arrayRecord = dataPage2.GetArrayRecord();
	int i, nSize = arrayRecord.GetSize();
	for (i=0; i<nSize; i++)
	{
		CaRecord* pRec = (CaRecord*)arrayRecord.GetAt(i);
		delete pRec;
	}
	arrayRecord.RemoveAll();
	m_listCtrlHuge.ResizeToFit();

	return 0;
}

LONG CuDlgPageCsv::OnGetData (WPARAM wParam, LPARAM lParam)
{
	int nMode = (int)wParam;
	switch (nMode)
	{
	case 0: // Separator Type
		return (LONG)FMT_FIELDSEPARATOR;
	case 1: // Separator Set
		return (long)m_pSeparatorSet;
	case 2: // Separator
		return (long)m_pSeparator;
	case 3: // Column Count
		return (long)m_nCoumnCount;
	case CHECK_CONFIRM_CHOICE:
		return m_cCheckConfirmChoice.GetCheck();
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	return 0;
}



void CuDlgPageCsv::DisplaySeparatorSetInfo(CaSeparatorSet* pSeparatorSet, CaSeparator* pSeparator)
{
	//
	// Display separator:
	CString strSep = _T("");
	if (pSeparator)
		strSep = pSeparator->GetSeparator();
	if (strSep.GetLength() > 1)
		m_cEditSeparator.SetWindowText (strSep);
	else
	if (strSep.GetLength() == 1)
	{
		if (strSep[0] ==_T(' '))
			m_cEditSeparator.SetWindowText (_T("<SPACE>"));
		else
		if (strSep[0] == _T('\t'))
			m_cEditSeparator.SetWindowText (_T("<TAB>"));
		else
		if (_istprint ((int)(unsigned char)strSep[0]))
		{
			m_cEditSeparator.SetWindowText (strSep);
		}
		else
		{
			CString strDisplay;
			strDisplay.Format (_T("0x%x"), (int)(unsigned char)strSep[0]);
			m_cEditSeparator.SetWindowText (strDisplay);
		}
	}
	//
	// Display the consecutive separators considered as one:
	int nCheck = pSeparatorSet->GetConsecutiveSeparatorsAsOne()? 1: 0;
	m_cCheckConsecutiveSeparatorAsOne.SetCheck (nCheck);
	//
	// Display the Ignore Trailing Separators:
	nCheck = pSeparatorSet->GetIgnoreTrailingSeparator()? 1: 0;
	m_cCheckIgnoreTrailingSeparator.SetCheck (nCheck);
	//
	// Display the Text Qualifier:
	CString strTQ = pSeparatorSet->GetTextQualifier();
	int nFound = m_cComboTextQualifier.FindStringExact (-1, strTQ);
	if (nFound == CB_ERR)
		m_cComboTextQualifier.SetCurSel (0);
	else
		m_cComboTextQualifier.SetCurSel (nFound);
	m_cComboTextQualifier.EnableWindow (FALSE);
}


void CuDlgPageCsv::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgPageCsv::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_listCtrlHuge.m_hWnd))
	{
		CRect r, r2, rClient;
		GetClientRect (rClient);
		m_listCtrlHuge.GetWindowRect (r);
		ScreenToClient (r);
		int nCxMargin = r.left;
		r.bottom = rClient.bottom - nCxMargin;

#if defined (CONFIRM_TABCHOICE_WITH_CHECKBOX)
		//
		// Checkbox confirm choice:
		m_cCheckConfirmChoice.GetWindowRect (r2);
		ScreenToClient (r2);
		int nH = r2.Height();
		r2.bottom = rClient.bottom - nCxMargin;
		r2.top = r2.bottom - nH;
		m_cCheckConfirmChoice.MoveWindow(r2);
		m_cCheckConfirmChoice.EnableWindow(TRUE);
		m_cCheckConfirmChoice.ShowWindow(SW_SHOW);

		r.bottom = r2.top-4;
#endif

		r.right = rClient.right - nCxMargin;
		m_listCtrlHuge.MoveWindow (r);
		m_listCtrlHuge.ResizeToFit();
	}
}

void CuDlgPageCsv::OnCheckConfirmChoice() 
{
	CaDataPage1& dataPage1 = m_pData->m_dataPage1;
	if (m_cCheckConfirmChoice.GetCheck() == 1)
	{
		CStringArray arrayHeader;
		BOOL bFirstLineAsHeader = GenerateHeader(dataPage1, m_pSeparatorSet, m_pSeparator, m_nCoumnCount, arrayHeader);
		if (dataPage1.GetFirstLineAsHeader() && dataPage1.GetLineCountToSkip() == 1)
		{
			if (!bFirstLineAsHeader)
				AfxMessageBox (IDS_MSG_ERROR_HEADERLINE);
		}
	}
	//
	// You have already checked one.\nThe previous check will be cancelled.
	CString strMsg;
	strMsg.LoadString(IDS_CONFIRM_CHOICE_WITH_CHECKBOX);

	CWnd* pParent = GetParent();
	ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CTabCtrl)));
	if (pParent && pParent->IsKindOf(RUNTIME_CLASS(CTabCtrl)))
	{
		BOOL nAsked = FALSE;
		TCITEM item;
		memset (&item, 0, sizeof (item));
		item.mask = TCIF_PARAM;
		CTabCtrl* pTab = (CTabCtrl*)pParent;
		int nCheck, i, nCount = pTab->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (pTab->GetItem (i, &item))
			{
				CWnd* pPage = (CWnd*)item.lParam;
				ASSERT (pPage);
				if (!pPage)
					continue;
				if (pPage == this)
					continue;
				nCheck = pPage->SendMessage(WMUSRMSG_GETITEMDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
				if (nCheck == 1)
				{
					if (!nAsked)
					{
						nAsked = TRUE;
						int nAnswer = AfxMessageBox (strMsg, MB_OKCANCEL|MB_ICONEXCLAMATION);
						if (nAnswer != IDOK)
						{
							m_cCheckConfirmChoice.SetCheck (0);
							return;
						}
						//
						// Uncheck the checkbox m_cCheckConfirmChoice:
						pPage->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
					}
					else
					{
						//
						// Uncheck the checkbox m_cCheckConfirmChoice:
						pPage->SendMessage(WMUSRMSG_UPDATEDATA, (WPARAM)CHECK_CONFIRM_CHOICE, 0);
					}
				}
			}
		}
	}
}
