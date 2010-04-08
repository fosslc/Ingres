/******************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : icebunitfp.cpp : implementation file
**
**
**    Project  : Visual DBA / Ice
**    Author   : Schalk Philippe
**
**    Purpose  : Popup Dialog Box for create Business unit Page or Facet.
**  History after 04-May-1999:
**
**    23-Sep-1999 (schph01)
**      bug #98885
**
******************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "icebunfp.h"

extern "C"
{
#include "domdata.h"
#include "msghandl.h"
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateIceFacetPage ( LPICEBUSUNITDOCDATA lpcreateIceDoc,int nHnode)
{
	CxDlgIceBunitFacetPage dlg;

	dlg.m_StructDoc = lpcreateIceDoc;
	dlg.m_csVirtNodeName = GetVirtNodeName (nHnode);
	dlg.m_nNodeHandle = nHnode;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBunitFacetPage dialog


CxDlgIceBunitFacetPage::CxDlgIceBunitFacetPage(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgIceBunitFacetPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgIceBunitFacetPage)
	m_bAccess = FALSE;
	m_nRepository = -1;
	m_nPreCache = -1;
	m_csLocation = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgIceBunitFacetPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgIceBunitFacetPage)
	DDX_Control(pDX, IDC_STATICRELOADFILE, m_ctrlstReloadFile);
	DDX_Control(pDX, IDC_CHECKRELOADFILE, m_ctrlReloadFrom);
	DDX_Control(pDX, IDC_PRE_CACHE, m_ctrlbtPreCache);
	DDX_Control(pDX, IDC_ST_REPOSITORY, m_ctrlbtRepository);
	DDX_Control(pDX, IDC_ICE_STATIC_CACHE, m_ctrlcbStaticCache);
	DDX_Control(pDX, IDC_ICE_STAT_LOCATION, m_ctrlctLocation);
	DDX_Control(pDX, IDC_ICE_LOCATION, m_ctrlcbLocation);
	DDX_Control(pDX, IDC_ICE_FILENAME_EXT, m_ctrledFileNameExt);
	DDX_Control(pDX, IDC_ICE_EDIT_FILENAME, m_ctrledFileName);
	DDX_Control(pDX, IDC_BROWSE_FILE, m_ctrlcbBrowseFile);
	DDX_Control(pDX, IDOK, m_ctrlOK);
	DDX_Control(pDX, IDC_ICE_STATIC_SEPARATOR, m_ctrlstSeparator);
	DDX_Control(pDX, IDC_ICE_STATIC_PATH, m_ctrlstPath);
	DDX_Control(pDX, IDC_ICE_STATIC_FILENAME, m_ctrlstFileName);
	DDX_Control(pDX, IDC_ICE_DOC_NAME, m_ctrledDocName);
	DDX_Control(pDX, IDC_ICE_DOC_EXT, m_ctrledExt);
	DDX_Control(pDX, IDC_PATH, m_ctrledPathName);
	DDX_Check(pDX, IDC_ICE_ACCESS, m_bAccess);
	DDX_Radio(pDX, IDC_ST_REPOSITORY, m_nRepository);
	DDX_Radio(pDX, IDC_PRE_CACHE, m_nPreCache);
	DDX_CBString(pDX, IDC_ICE_LOCATION, m_csLocation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgIceBunitFacetPage, CDialog)
	//{{AFX_MSG_MAP(CxDlgIceBunitFacetPage)
	ON_BN_CLICKED(IDC_BROWSE_FILE, OnBrowseFile)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_ST_REPOSITORY, OnStRepository)
	ON_BN_CLICKED(IDC_ST_FILE_SYSTEM, OnStFileSystem)
	ON_EN_CHANGE(IDC_ICE_DOC_NAME, OnChangeIceDocName)
	ON_EN_CHANGE(IDC_ICE_EDIT_FILENAME, OnChangeIceEditFilename)
	ON_EN_CHANGE(IDC_ICE_FILENAME_EXT, OnChangeIceFilenameExt)
	ON_CBN_SELCHANGE(IDC_ICE_LOCATION, OnSelchangeIceLocation)
	ON_EN_CHANGE(IDC_PATH, OnChangePath)
	ON_BN_CLICKED(IDC_PERM_CACHE, OnPermCache)
	ON_BN_CLICKED(IDC_PRE_CACHE, OnPreCache)
	ON_BN_CLICKED(IDC_SESSION_CACHE, OnSessionCache)
	ON_BN_CLICKED(IDC_CHECKRELOADFILE, OnCheckreloadfile)
	ON_EN_CHANGE(IDC_ICE_DOC_EXT, OnChangeIceDocExt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBunitFacetPage message handlers

BOOL CxDlgIceBunitFacetPage::OnInitDialog() 
{
	CString csTitle;
	CString csFormat;
	CDialog::OnInitDialog();

	m_ctrledDocName.LimitText    (sizeof(m_StructDoc->name)-1);
	m_ctrledFileNameExt.LimitText(sizeof(m_StructDoc->suffix)-1);
	m_ctrledPathName.LimitText   (sizeof(m_StructDoc->doc_file)-1);
	m_ctrledFileName.LimitText   (sizeof(m_StructDoc->ext_file)-1);
	m_ctrledFileNameExt.LimitText(sizeof(m_StructDoc->ext_suffix)-1);
	if (m_StructDoc->bAlter)
	{
		m_ctrledDocName.SetWindowText(m_StructDoc->name);
		m_ctrledExt.SetWindowText(m_StructDoc->suffix);
		m_ctrledExt.EnableWindow(FALSE);
		m_ctrledDocName.EnableWindow(FALSE);
		GetDlgItem(IDC_BROWSE_FILE)->EnableWindow(FALSE);
		m_bAccess = m_StructDoc->bPublic;
		
		if (m_StructDoc->bpre_cache)
		{
			m_nPreCache = 0;
			m_nRepository = 0;// This is a Repository storage type
		}
		else if (m_StructDoc->bperm_cache)
		{
			m_nPreCache = 1;
			m_nRepository = 0;// This is a Repository storage type
		}
		else if (m_StructDoc->bsession_cache)
		{
			m_nPreCache = 2;
			m_nRepository = 0;// This is a Repository storage type
		}
		else
		{
			m_nPreCache = 0;
			m_nRepository = 1;// This is a File System storage type
		}
		m_ctrledPathName.SetWindowText(m_StructDoc->doc_file);
		m_csLocation = m_StructDoc->ext_loc.LocationName;
		m_ctrledFileNameExt.SetWindowText(m_StructDoc->ext_suffix);
		m_ctrledFileName.SetWindowText(m_StructDoc->ext_file);
	}
	else
		m_nRepository = 0;// This is a repository storage type
	UpdateData(FALSE);
	// Generate title
	GetWindowText(csFormat);
	if (!m_StructDoc->bFacet)
	{
		if (!m_StructDoc->bAlter)
		{
			csTitle.Format(csFormat,_T("Page"),m_StructDoc->icebusunit.Name);
			PushHelpId (HELPID_IDD_ICE_BUSINESS_PAGE);
		}
		else
		{
			csTitle.Format(IDS_ICE_ALTER_BUSINESS_PAGE_TITLE,m_StructDoc->name,
															 m_StructDoc->icebusunit.Name);
			PushHelpId (HELPID_ALTER_IDD_ICE_BUSINESS_PAGE);
		}
	}
	else
	{
		if (!m_StructDoc->bAlter)
		{
			csTitle.Format(csFormat,_T("Facet"),m_StructDoc->icebusunit.Name);
			PushHelpId (HELPID_IDD_ICE_BUSINESS_FACET);
		}
		else
		{
			csTitle.Format(IDS_ICE_ALTER_BUSINESS_FACET_TITLE,m_StructDoc->name,
															  m_StructDoc->icebusunit.Name);
			PushHelpId (HELPID_ALTER_IDD_ICE_BUSINESS_FACET);
		}
	}
	SetWindowText(csTitle);
	fillIceLocationData();
	EnableDisableControl();
	EnableDisableOK();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
void CxDlgIceBunitFacetPage::fillIceLocationData()
{
	LPUCHAR aparentsTemp[MAXPLEVEL];
	UCHAR 	buf[MAXOBJECTNAME];
	UCHAR 	bufOwner[MAXOBJECTNAME];
	UCHAR 	bufComplim[MAXOBJECTNAME];
	int iret;
	CWaitCursor wait;

	aparentsTemp[0] = m_StructDoc->icebusunit.Name;
	aparentsTemp[1] = aparentsTemp[2] = NULL;

	memset (&bufComplim, '\0', sizeof(bufComplim));
	memset (&bufOwner, '\0', sizeof(bufOwner));
	iret =	DOMGetFirstObject(m_nNodeHandle,
							OT_ICE_BUNIT_LOCATION,
							1,							// level,
							aparentsTemp, 				// Temp mandatory!
							TRUE,						// bwithsystem
							NULL, 						// lpowner
							buf,
							bufOwner,
							bufComplim);
	while (iret == RES_SUCCESS) {
		m_ctrlcbLocation.AddString((LPCTSTR)buf);
		iret = DOMGetNextObject(buf, bufOwner, bufComplim);
	}
	if(m_StructDoc->bAlter)
	{
		m_csLocation = m_StructDoc->ext_loc.LocationName;
		UpdateData(FALSE);
	}
	else
		m_ctrlcbLocation.SetCurSel(0);
}



void CxDlgIceBunitFacetPage::OnBrowseFile() 
{
	BOOL bRet;
	CString csFileName,csFilter;
	m_ctrledPathName.GetWindowText(csFileName);
	if (m_StructDoc->bFacet)
		csFilter = _T("Pictures (*.gif,*.jpg)|*.gif;*.jpg|HTML (*.htm,*.html)|*.htm;*.html|All Files (*.*)|*.*||");
	else
		csFilter = _T("HTML (*.htm,*.html)|*.htm;*.html|Pictures (*.gif,*.jpg)|*.gif;*.jpg|All Files (*.*)|*.*||");
	CFileDialog cFiledlg(TRUE,
						NULL,
						csFileName,
						OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
						(LPCTSTR)csFilter);
	bRet = cFiledlg.DoModal();
	if (bRet == IDOK)
	{
		if (!m_StructDoc->bAlter)
		{
			m_ctrledExt.SetWindowText(cFiledlg.GetFileExt( ));
			m_ctrledDocName.SetWindowText(cFiledlg.GetFileTitle( ));
		}
		m_ctrledPathName.SetWindowText(cFiledlg.GetPathName());
	}
}

void CxDlgIceBunitFacetPage::OnDestroy() 
{
	CDialog::OnDestroy();

	PopHelpId();
}

void CxDlgIceBunitFacetPage::OnOK() 
{
	ICEBUSUNITDOCDATA newDocs;
	memset (&newDocs,0,sizeof(ICEBUSUNITDOCDATA));
	lstrcpy((char *)newDocs.icebusunit.Name,(char *)m_StructDoc->icebusunit.Name);
	newDocs.bFacet = m_StructDoc->bFacet;

	FillStructureWithControl(&newDocs);

	if(m_StructDoc->bFacet)
	{
		if ( !m_StructDoc->bAlter )
		{
			CWaitCursor wait;
			if ( ICEAddobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName ,OT_ICE_BUNIT_FACET,&newDocs ) == RES_ERR )
			{
				AfxMessageBox (IDS_E_ICE_BUSINESS_UNIT_FACET_FAILED);
				return;
			}
		}
		else
		{
			CWaitCursor wait;
			if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName,
								OT_ICE_BUNIT_FACET,
								m_StructDoc, &newDocs) == RES_ERR )
			{
				AfxMessageBox (IDS_E_ICE_BUSINESS_UNIT_ALTER_FACET_FAILED);
				return;
			}
		}
	}
	else
	{
		if ( !m_StructDoc->bAlter )
		{
			CWaitCursor wait;
			if ( ICEAddobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName ,OT_ICE_BUNIT_PAGE,&newDocs ) == RES_ERR )
			{
				AfxMessageBox (IDS_E_ICE_BUSINESS_UNIT_PAGE_FAILED);
				return;
			}
		}
		else
		{
			CWaitCursor wait;
			if ( ICEAlterobject((LPUCHAR)(LPCTSTR)m_csVirtNodeName,
								OT_ICE_BUNIT_PAGE,
								m_StructDoc, &newDocs) == RES_ERR )
			{
				AfxMessageBox (IDS_E_ICE_BUSINESS_UNIT_ALTER_PAGE_FAILED);
				return;
			}
		}
	}
	CDialog::OnOK();
}

void CxDlgIceBunitFacetPage::OnStRepository() 
{
	EnableDisableControl();
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnStFileSystem() 
{
	int nNbItem = m_ctrlcbLocation.GetCount();
	if (nNbItem == 0 || nNbItem == CB_ERR)
		fillIceLocationData();
	EnableDisableControl();
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::EnableDisableControl()
{
	if (m_ctrlbtRepository.GetCheck() == 1)
	{	
		m_ctrlcbStaticCache.ShowWindow(SW_SHOW);
		m_ctrlbtPreCache.ShowWindow(SW_SHOW);
		GetDlgItem(IDC_PERM_CACHE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SESSION_CACHE)->ShowWindow(SW_SHOW);
		if (m_StructDoc->bAlter)
		{
			m_ctrlstPath.ShowWindow(SW_SHOW);
			m_ctrlcbBrowseFile.ShowWindow(SW_SHOW);
			m_ctrledPathName.ShowWindow(SW_SHOW);
			m_ctrlstReloadFile.ShowWindow(SW_SHOW);
			m_ctrlReloadFrom.ShowWindow(SW_SHOW);
			m_ctrlReloadFrom.EnableWindow(TRUE);
			if (m_ctrlReloadFrom.GetCheck() == 1)
			{
				m_ctrledPathName.ShowWindow(SW_SHOW);
				m_ctrlstPath.ShowWindow(SW_SHOW);
				m_ctrlcbBrowseFile.ShowWindow(SW_SHOW);
				m_ctrlcbBrowseFile.EnableWindow(TRUE);
			}
			else
			{
				m_ctrledPathName.ShowWindow(SW_HIDE);
				m_ctrlstPath.ShowWindow(SW_HIDE);
				m_ctrlcbBrowseFile.ShowWindow(SW_HIDE);
			}
		}
		else
		{
			m_ctrlstReloadFile.ShowWindow(SW_SHOW);
			m_ctrlReloadFrom.EnableWindow(FALSE);
			m_ctrlReloadFrom.ShowWindow(SW_SHOW);
			m_ctrledPathName.ShowWindow(SW_SHOW);
			m_ctrlstPath.ShowWindow(SW_SHOW);
			m_ctrledPathName.EnableWindow(TRUE);
			m_ctrlstPath.EnableWindow(TRUE);
			m_ctrlcbBrowseFile.EnableWindow(TRUE);
			m_ctrlcbBrowseFile.ShowWindow(SW_SHOW);
		}

		m_ctrlctLocation.ShowWindow(SW_HIDE);
		m_ctrlcbLocation.ShowWindow(SW_HIDE);
		m_ctrlstFileName.ShowWindow(SW_HIDE);
		m_ctrledFileName.ShowWindow(SW_HIDE);
		m_ctrledFileNameExt.ShowWindow(SW_HIDE);
		m_ctrlstSeparator.ShowWindow(SW_HIDE);
	}
	else
	{
		m_ctrlcbStaticCache.ShowWindow(SW_HIDE);
		m_ctrlbtPreCache.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_PERM_CACHE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SESSION_CACHE)->ShowWindow(SW_HIDE);
		m_ctrledPathName.ShowWindow(SW_HIDE);
		m_ctrlstPath.ShowWindow(SW_HIDE);
		m_ctrlcbBrowseFile.ShowWindow(SW_HIDE);
		m_ctrlReloadFrom.ShowWindow(SW_HIDE);
		m_ctrlstReloadFile.ShowWindow(SW_HIDE);

		m_ctrlctLocation.ShowWindow(SW_SHOW);
		m_ctrlcbLocation.ShowWindow(SW_SHOW);
		m_ctrlstFileName.ShowWindow(SW_SHOW);
		m_ctrledFileName.ShowWindow(SW_SHOW);
		m_ctrledFileNameExt.ShowWindow(SW_SHOW);
		m_ctrlstSeparator.ShowWindow(SW_SHOW);
	}

}

void CxDlgIceBunitFacetPage::FillStructureWithControl(LPICEBUSUNITDOCDATA IbuDdta)
{
	UpdateData(TRUE);
	m_ctrledDocName.GetWindowText((char *)IbuDdta->name,sizeof(IbuDdta->name));
	m_ctrledExt.GetWindowText((char *)IbuDdta->suffix,sizeof(IbuDdta->suffix));
	IbuDdta->bPublic=m_bAccess;

	if (m_ctrlbtRepository.GetCheck() != 1)
	{
		lstrcpy((char *)IbuDdta->ext_loc.LocationName,m_csLocation);//ICELOCATIONDATA 
		m_ctrledFileNameExt.GetWindowText((char *)IbuDdta->ext_suffix,sizeof(IbuDdta->ext_suffix));
		m_ctrledFileName.GetWindowText((char *)IbuDdta->ext_file,sizeof(IbuDdta->ext_file));
		
		IbuDdta->bpre_cache		= FALSE;
		IbuDdta->bperm_cache	= FALSE;
		IbuDdta->bsession_cache	= FALSE;
		IbuDdta->doc_file[0]	= 0;
		IbuDdta->doc_owner[0]	= 0;
		IbuDdta->btransfer		= FALSE;
	}
	else
	{
		IbuDdta->bpre_cache		= FALSE;
		IbuDdta->bperm_cache	= FALSE;
		IbuDdta->bsession_cache = FALSE;

		switch (m_nPreCache)
		{
		case 0:
			IbuDdta->bpre_cache		= TRUE;
			break;
		case 1:
			IbuDdta->bperm_cache	= TRUE;
			break;
		case 2:
			IbuDdta->bsession_cache = TRUE;
			break;
		default:
			break;
		}
		if (m_StructDoc->bAlter &&  m_ctrlReloadFrom.GetCheck() != 1)
		{
			lstrcpy(IbuDdta->doc_file,_T(""));
		}
		else
		{
			m_ctrledPathName.GetWindowText((char *)IbuDdta->doc_file,sizeof(IbuDdta->doc_file));
		}

		IbuDdta->doc_owner[0] = 0;  // No onformation on this field, not filled in dialog box.
		IbuDdta->btransfer = FALSE; // No information on this field, not filled in dialog box.

		IbuDdta->ext_loc.LocationName[0]=0;
		IbuDdta->ext_file[0]=0;
		IbuDdta->ext_suffix[0]=0;
	}
}
void CxDlgIceBunitFacetPage::EnableDisableOK()
{
	BOOL bEnable = TRUE;
	if (m_ctrledDocName.LineLength() == 0 ||m_ctrledExt.LineLength() == 0 )
	{
		m_ctrlOK.EnableWindow(FALSE);
		return;
	}
	if (m_ctrlbtRepository.GetCheck() == 1)
	{
		if (!m_StructDoc->bAlter && m_ctrledPathName.LineLength() == 0)
		{
			m_ctrlOK.EnableWindow(FALSE);
			return;
		}
		else
		{
			if (m_ctrlReloadFrom.GetCheck() == 1 &&
				m_ctrledPathName.LineLength() == 0)
			{
				m_ctrlOK.EnableWindow(FALSE);
				return;
			}
		}
		if (IsDlgButtonChecked( IDC_PRE_CACHE ) == BST_CHECKED ||
			IsDlgButtonChecked( IDC_PERM_CACHE ) == BST_CHECKED||
			IsDlgButtonChecked( IDC_SESSION_CACHE ) == BST_CHECKED )
		{
			m_ctrlOK.EnableWindow(TRUE);
			return;
		}
		else
		{
			m_ctrlOK.EnableWindow(FALSE);
			return;
		}
	}
	else
	{
		if (m_ctrlcbLocation.GetCurSel() == CB_ERR)
		{
			m_ctrlOK.EnableWindow(FALSE);
			return;
		}

		if( m_ctrledFileNameExt.LineLength() == 0 || m_ctrledFileName.LineLength() == 0)
		{
			m_ctrlOK.EnableWindow(FALSE);
			return;
		}
	}
	m_ctrlOK.EnableWindow(TRUE);
}

void CxDlgIceBunitFacetPage::OnChangeIceDocName() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnChangeIceEditFilename() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnChangeIceFilenameExt() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnSelchangeIceLocation() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnChangePath() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnPermCache() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnPreCache() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnSessionCache() 
{
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnCheckreloadfile() 
{
	EnableDisableControl();
	EnableDisableOK();
}

void CxDlgIceBunitFacetPage::OnChangeIceDocExt() 
{
	EnableDisableOK();
}
