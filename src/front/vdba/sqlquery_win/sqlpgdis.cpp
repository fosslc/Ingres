/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlpgdis.cpp : implementation file
**    Project  : SqlTest ActiveX.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page for Display
**
** History:
**
** 06-Mar-2002 (uk$so01)
**    Created
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Preference Options with different help context ID.
*/

#include "stdafx.h"
#include "sqlquery.h"
#include "rcdepend.h"
#include "sqlpgdis.h"
#include "sqlselec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSqlqueryPropPageDisplay, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSqlqueryPropPageDisplay, COlePropertyPage)
	//{{AFX_MSG_MAP(CSqlqueryPropPageDisplay)
	ON_EN_CHANGE(IDC_EDIT_F8WIDTH, OnChangeEditF8width)
	ON_EN_CHANGE(IDC_EDIT_F8SCALE, OnChangeEditF8scale)
	ON_EN_CHANGE(IDC_EDIT_F4WIDTH, OnChangeEditF4width)
	ON_EN_CHANGE(IDC_EDIT_F4SCALE, OnChangeEditF4scale)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid
#if defined (EDBC)
// {CFAA4E69-7805-4ffc-9C98-44AB52BEBDE4}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPageDisplay, "EDBQUERY.CSqlqueryPropPageDisplay",
	0xcfaa4e69, 0x7805, 0x4ffc, 0x9c, 0x98, 0x44, 0xab, 0x52, 0xbe, 0xbd, 0xe4)
#else
// {FC40B05B-2CEE-11D6-87A5-00C04F1F754A}
IMPLEMENT_OLECREATE_EX(CSqlqueryPropPageDisplay, "SQLQUERY.CSqlqueryPropPageDisplay",
	0xfc40b05b, 0x2cee, 0x11d6, 0x87, 0xa5, 0x0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)
#endif

/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageDisplay::CSqlqueryPropPageDisplayFactory::UpdateRegistry -
// Adds or removes system registry entries for CSqlqueryPropPageDisplay

BOOL CSqlqueryPropPageDisplay::CSqlqueryPropPageDisplayFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_xSQLQUERY_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageDisplay::CSqlqueryPropPageDisplay - Constructor

CSqlqueryPropPageDisplay::CSqlqueryPropPageDisplay() :
	COlePropertyPage(IDD, IDS_SQLQUERY_PPGDISPLAY_CAPTION)
{
	//{{AFX_DATA_INIT(CSqlqueryPropPageDisplay)
	m_nF4Scale = F4_DEFAULT_SCALE;
	m_nF4Width = F4_DEFAULT_PREC;
	m_bF4Exponential = (F4_DEFAULT_DISPLAY == _T('e'))? TRUE: FALSE;
	m_nF8Scale = F8_DEFAULT_SCALE;
	m_nF8Width = F8_DEFAULT_PREC;
	m_bF8Exponential =(F8_DEFAULT_DISPLAY == _T('e'))? TRUE: FALSE;
	m_bShowGrid = FALSE;
	//}}AFX_DATA_INIT
	m_nQepDisplayMode = 1;
	m_nXmlDisplayMode = 0;
	SetHelpInfo(_T(""), theApp.m_strHelpFile, 0);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageDisplay::DoDataExchange - Moves data between page and properties

void CSqlqueryPropPageDisplay::DoDataExchange(CDataExchange* pDX)
{
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//{{AFX_DATA_MAP(CSqlqueryPropPageDisplay)
	DDX_Control(pDX, IDC_EDIT_F4SCALE, m_cEditF4Scale);
	DDX_Control(pDX, IDC_EDIT_F4WIDTH, m_cEditF4Precision);
	DDX_Control(pDX, IDC_EDIT_F8SCALE, m_cEditF8Scale);
	DDX_Control(pDX, IDC_EDIT_F8WIDTH, m_cEditF8Precision);
	DDX_Control(pDX, IDC_SPIN4, m_cSpinF8Scale);
	DDX_Control(pDX, IDC_SPIN3, m_cSpinF8Precision);
	DDX_Control(pDX, IDC_SPIN2, m_cSpinF4Scale);
	DDX_Control(pDX, IDC_SPIN1, m_cSpinF4Precision);
	DDP_Text(pDX, IDC_EDIT_F4SCALE, m_nF4Scale, _T("F4Scale") );
	DDX_Text(pDX, IDC_EDIT_F4SCALE, m_nF4Scale);
	DDP_Text(pDX, IDC_EDIT_F4WIDTH, m_nF4Width, _T("F4Width") );
	DDX_Text(pDX, IDC_EDIT_F4WIDTH, m_nF4Width);
	DDP_Check(pDX, IDC_CHECK_F4EXPONENTIAL, m_bF4Exponential, _T("F4Exponential") );
	DDX_Check(pDX, IDC_CHECK_F4EXPONENTIAL, m_bF4Exponential);
	DDP_Text(pDX, IDC_EDIT_F8SCALE, m_nF8Scale, _T("F8Scale") );
	DDX_Text(pDX, IDC_EDIT_F8SCALE, m_nF8Scale);
	DDP_Text(pDX, IDC_EDIT_F8WIDTH, m_nF8Width, _T("F8Width") );
	DDX_Text(pDX, IDC_EDIT_F8WIDTH, m_nF8Width);
	DDP_Check(pDX, IDC_CHECK_F8EXPONENTIAL, m_bF8Exponential, _T("F8Exponential") );
	DDX_Check(pDX, IDC_CHECK_F8EXPONENTIAL, m_bF8Exponential);
	DDP_Check(pDX, IDC_CHECK_SHOWGRID, m_bShowGrid, _T("ShowGrid") );
	DDX_Check(pDX, IDC_CHECK_SHOWGRID, m_bShowGrid);
	//}}AFX_DATA_MAP

	DDX_Radio (pDX, IDC_RADIO3, m_nQepDisplayMode);
	DDP_Radio (pDX, IDC_RADIO3, m_nQepDisplayMode, _T("QepDisplayMode") );
	DDX_Radio (pDX, IDC_RADIO7, m_nXmlDisplayMode);
	DDP_Radio (pDX, IDC_RADIO7, m_nXmlDisplayMode, _T("XmlDisplayMode") );

	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSqlqueryPropPageDisplay message handlers

BOOL CSqlqueryPropPageDisplay::OnInitDialog() 
{
	COlePropertyPage::OnInitDialog();
	
	m_cSpinF4Precision.SetRange(MINF4_PREC, MAXF4_PREC);
	m_cSpinF4Scale.SetRange(MINF4_SCALE, MAXF4_SCALE);
	m_cSpinF8Precision.SetRange(MINF8_PREC, MAXF8_PREC);
	m_cSpinF8Scale.SetRange(MINF8_SCALE, MAXF8_SCALE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSqlqueryPropPageDisplay::OnChangeEditF8width() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the COlePropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F8_OnChangeEditPrecision(&m_cEditF8Precision);
}

void CSqlqueryPropPageDisplay::OnChangeEditF8scale() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the COlePropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F8_OnChangeEditScale(&m_cEditF8Scale);
}

void CSqlqueryPropPageDisplay::OnChangeEditF4width() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the COlePropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F4_OnChangeEditPrecision(&m_cEditF4Precision);
}

void CSqlqueryPropPageDisplay::OnChangeEditF4scale() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the COlePropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	F4_OnChangeEditScale(&m_cEditF4Scale);
}

BOOL CSqlqueryPropPageDisplay::OnHelp(LPCTSTR lpszHelpDir)
{
	UINT nHelpID = 5509; // OLD vdba: SQLACTPREFDIALOG = 5507
	return APP_HelpInfo(m_hWnd, nHelpID);
}
