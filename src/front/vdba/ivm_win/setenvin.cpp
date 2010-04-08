/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : setenvin.cpp, implementation file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Set ingres parameter dialog
**
** History:
**
** 29-Feb-2000 (uk$so01)
**    Created. (SIR # 100634)
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 05-Jun-2001 (uk$so01)
**    SIR #104857 Sort the paramters in the pop-up dialog Set Ingres Parameters
**    for System and User.
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "setenvin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int compare (const void* e1, const void* e2)
{
	UINT o1 = *((UINT*)e1);
	UINT o2 = *((UINT*)e2);

	CaEnvironmentParameter* p1 = (CaEnvironmentParameter*)o1;
	CaEnvironmentParameter* p2 = (CaEnvironmentParameter*)o2;
	if (p1 && p2)
		return p1->GetName().CompareNoCase (p2->GetName());
	else
		return 0;
}


CxDlgSetEnvironmentVariableIngres::CxDlgSetEnvironmentVariableIngres(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgSetEnvironmentVariableIngres::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgSetEnvironmentVariableIngres)
	m_strName = _T("");
	m_strValue = _T("");
	//}}AFX_DATA_INIT
	m_nParamType = EPARAM_SYSTEM;
}


void CxDlgSetEnvironmentVariableIngres::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgSetEnvironmentVariableIngres)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Control(pDX, IDC_COMBOBOXEX1, m_cComboEx1);
	DDX_CBString(pDX, IDC_COMBOBOXEX1, m_strName);
	DDX_Text(pDX, IDC_EDIT1, m_strValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgSetEnvironmentVariableIngres, CDialog)
	//{{AFX_MSG_MAP(CxDlgSetEnvironmentVariableIngres)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX1, OnSelchangeComboboxex1)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgSetEnvironmentVariableIngres message handlers

BOOL CxDlgSetEnvironmentVariableIngres::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_imageList.Create (IDB_EVIRONMENT_VARIABLE, 16, 0, RGB(255,0,255));
	m_cComboEx1.SetImageList(&m_imageList);
	FillEnvironment();
	EnableOKButton();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CxDlgSetEnvironmentVariableIngres::FillEnvironment()
{
	int nCount = 0;
	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));

	cbitem.mask   = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	if (m_nParamType == EPARAM_SYSTEM)
	{
		cbitem.iImage = IM_ENV_SYSTEM;
		cbitem.iSelectedImage = IM_ENV_SYSTEM;
	}
	else
	if (m_nParamType == EPARAM_USER)
	{
		cbitem.iImage = EPARAM_USER;
		cbitem.iSelectedImage = EPARAM_USER;
	}
	else
	{
		//
		// Type ?? 
		ASSERT (FALSE);
		cbitem.iImage = IM_ENV_SYSTEM;
		cbitem.iSelectedImage = IM_ENV_SYSTEM;
	}

	BOOL bOk = FALSE;
	if (m_nParamType == EPARAM_SYSTEM)
	{
		cbitem.iImage = IM_ENV_SYSTEM;
		cbitem.iSelectedImage = IM_ENV_SYSTEM;
		bOk = theApp.m_ingresVariable.QueryEnvironment (m_listEnvirenment, TRUE);
	}
	else
	if (m_nParamType == EPARAM_USER)
	{
		cbitem.iImage = IM_ENV_USER;
		cbitem.iSelectedImage = IM_ENV_USER;
		bOk = theApp.m_ingresVariable.QueryEnvironmentUser (m_listEnvirenment, TRUE);
	}

	if (bOk && m_listEnvirenment.GetCount() > 0)
	{
		//
		// Sort the parameters first:
		int i = 0;
		INT_PTR nCount = m_listEnvirenment.GetCount();
		UINT* pArray = new UINT[nCount];

		CaEnvironmentParameter* pParam = NULL;
		POSITION pos = m_listEnvirenment.GetHeadPosition();
		while (pos != NULL)
		{
			pParam = m_listEnvirenment.GetNext (pos);
			pArray[i] = (UINT)pParam;
			i++;
		}

		qsort ((void*)pArray, (size_t)nCount, (size_t)sizeof(UINT), compare);
		for (i=0; i<nCount; i++)
		{
			pParam = (CaEnvironmentParameter*)pArray[i];
			if (!pParam->IsSet())
			{
				cbitem.pszText = (LPTSTR)(LPCTSTR)pParam->GetName();
				cbitem.lParam  = (LPARAM)pParam;
				cbitem.iItem   = m_cComboEx1.GetCount();

				m_cComboEx1.InsertItem (&cbitem);
			}
		}

		delete pArray;
	}

	if (m_cComboEx1.GetCount() > 0)
		m_cComboEx1.SetCurSel(0);
	return TRUE;
}

void CxDlgSetEnvironmentVariableIngres::OnDestroy() 
{
	while (!m_listEnvirenment.IsEmpty())
		delete m_listEnvirenment.RemoveHead();
	CDialog::OnDestroy();
}

void CxDlgSetEnvironmentVariableIngres::EnableOKButton()
{
	BOOL bEnable = FALSE;
	int nSel = m_cComboEx1.GetCurSel();
	if (nSel != CB_ERR)
	{
		CString strText;
		m_cComboEx1.GetLBText (nSel, strText);
		if (!strText.IsEmpty())
		{
			m_cEdit1.GetWindowText (strText);
			strText.TrimLeft();
			strText.TrimRight();
			if (!strText.IsEmpty())
				bEnable = TRUE;
		}
	}
	m_cButtonOK.EnableWindow (bEnable);
}

void CxDlgSetEnvironmentVariableIngres::OnChangeEdit1() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	EnableOKButton();
}

void CxDlgSetEnvironmentVariableIngres::OnSelchangeComboboxex1() 
{
	int nSel = m_cComboEx1.GetCurSel();
	if (nSel != CB_ERR)
	{
		COMBOBOXEXITEM cbitem;
		memset (&cbitem, 0, sizeof(cbitem));

		cbitem.mask  = CBEIF_LPARAM ;
		cbitem.iItem = nSel;

		CString strText;
		if (m_cComboEx1.GetItem(&cbitem))
		{
			CaEnvironmentParameter* pParam = (CaEnvironmentParameter*)cbitem.lParam;
			if (pParam)
			{
				ParameterType nType = pParam->GetType();
				long lStyle = ::GetWindowLong (m_cEdit1.m_hWnd, GWL_STYLE);

				switch (nType)
				{
				case P_NUMBER:
				case P_NUMBER_SPIN:
					lStyle |= ES_NUMBER;
					break;
				default:
					lStyle &= ~ES_NUMBER;
					break;
				}
				::SetWindowLong(m_cEdit1.m_hWnd, GWL_STYLE, lStyle);
				m_cEdit1.UpdateWindow();
			}
		}
	}

	m_cEdit1.SetWindowText (_T(""));
	EnableOKButton();
}

void CxDlgSetEnvironmentVariableIngres::OnOK() 
{
	UpdateData (TRUE);
	int nSel = m_cComboEx1.GetCurSel();
	if (nSel != CB_ERR)
	{
		COMBOBOXEXITEM cbitem;
		memset (&cbitem, 0, sizeof(cbitem));
		cbitem.mask  = CBEIF_LPARAM ;
		cbitem.iItem = nSel;
		int nNewVale = 0;
		int nMin = 0, nMax = 0;
		if (m_cComboEx1.GetItem(&cbitem))
		{
			CaEnvironmentParameter* pParam = (CaEnvironmentParameter*)cbitem.lParam;
			if (pParam)
			{
				CString strMin;
				CString strMax;
				ParameterType nType = pParam->GetType();
				switch (nType)
				{
				case P_NUMBER_SPIN:
					nNewVale = _ttoi(m_strValue);
					pParam->GetMinMax (nMin, nMax);
					if (nNewVale < nMin || nNewVale > nMax)
					{
						CString strMsg;
						strMin.Format (_T("%d"), nMin);
						strMax.Format (_T("%d"), nMax);
						AfxFormatString2 (strMsg, IDS_ERROR_NUMERIC_RANGE, (LPCTSTR)strMin, (LPCTSTR)strMax);
						AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
						return;
					}
				default:
					break;
				}
			}
		}
	}
	CDialog::OnOK();

	//
	// ComboboxEx does not set correctly the member m_strName
	// Bug in DoDataExchange ?
	if (nSel != CB_ERR)
		m_cComboEx1.GetLBText (nSel, m_strName);
	else
		m_strName = _T("");
}

BOOL CxDlgSetEnvironmentVariableIngres::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID = IDD;
	return APP_HelpInfo(nHelpID);
}
