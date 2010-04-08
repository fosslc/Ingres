/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dlgmain.cpp : implementation file
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main dialog of vdda.exe (container of vdda.ocx)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
** 22-Apr-2003 (schph01)
**    SIR 107523 Add Sequence Object
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 17-May-2004 (uk$so01)
**    SIR #109220, Fix some bugs when save schema
** 17-May-2004 (uk$so01)
**    SIR #109220, Show the message if user click on save
**    without selecting the Schema.
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 13-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should be disconnected
**    after the comparison has been done.
*/

#include "stdafx.h"
#include "vsda.h"
#include "mainfrm.h"
#include "xvsdadml.h"
#include "vnodedml.h"
#include "dmldbase.h"
#include "dmluser.h"
#include "vnodedml.h"
#include "rctools.h"
#include "imageidx.h"
#include "ingobdml.h"
#include "compfile.h"
#include <io.h>
#include ".\dlgmain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REFRESH_NODE1      0x00000001
#define REFRESH_NODE2     (0x00000001 << 1)
#define REFRESH_USER1     (0x00000001 << 2)
#define REFRESH_USER2     (0x00000001 << 3)
#define REFRESH_DATABASE1 (0x00000001 << 4)
#define REFRESH_DATABASE2 (0x00000001 << 5)

class CaUserAll: public CaUser
{
public:
	CaUserAll():CaUser(){}
	CaUserAll(LPCTSTR lpszName):CaUser(lpszName){}
	virtual ~CaUserAll(){}

	virtual CString GetItem() {return _T("");}
};

class CaUserDefault: public CaUserAll
{
public:
	CaUserDefault():CaUserAll(), m_strVirtualNodeUser(_T("")){}
	CaUserDefault(LPCTSTR lpszName, CaNode* pNode):CaUserAll(lpszName){Init(pNode);}
	virtual ~CaUserDefault(){}

	virtual CString GetItem() {return m_strVirtualNodeUser;}
protected:
	CString m_strVirtualNodeUser;
	void Init(CaNode* pNode);
};
inline void CaUserDefault::Init(CaNode* pNode)
{
	m_strVirtualNodeUser = _T("");
	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	CaSession session;
	session.SetNode(pNode->GetName());
	session.SetDatabase(_T("iidbdb"));
	session.SetIndependent(TRUE);
	session.SetDescription(sessionMgr.GetDescription());
	CaSessionUsage UseSession (&sessionMgr, &session);
	m_strVirtualNodeUser = INGRESII_llDBMSInfo(_T("username")); // session_user?
}

class CaInstallationItem: public CaDatabase
{
public:
	CaInstallationItem():CaDatabase(){}
	CaInstallationItem(LPCTSTR lpszName):CaDatabase(lpszName, _T("")){}
	virtual ~CaInstallationItem(){}

	virtual CString GetItem() {return _T("");}
};



static CString GetSchemaFile() 
{
	CString strFullName;
	CString strFileFilter;
	strFileFilter = _T("VDDA and VSDA Files (*.ii_vdda, *.ii_vsda)|*.ii_vdda;*.ii_vsda|");
	strFileFilter+= _T("VDDA Files (*.ii_vdda)|*.ii_vdda|");
	strFileFilter+= _T("VSDA Files (*.ii_vsda)|*.ii_vsda||");
	CFileDialog dlg(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		strFileFilter);

	if (dlg.DoModal() != IDOK)
		return _T(""); 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (IDS_MSG_UNKNOWN_ENVIRONMENT_FILE);
		return _T("");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (!(strExt.CompareNoCase (_T("ii_vdda")) == 0 || strExt.CompareNoCase (_T("ii_vsda")) == 0))
		{
			AfxMessageBox (IDS_MSG_UNKNOWN_ENVIRONMENT_FILE);
			return _T("");
		}
	}
	if (strFullName.IsEmpty())
		return _T("");
	if (_taccess(strFullName, 0) == -1)
		return _T("");

	return strFullName;
}

static CString GetSchemaFile4Save(CString& strDatabase) 
{
	CString strFullName;
	CString strFileFilter;
	CString strFileExtention;

	if (strDatabase.IsEmpty())
	{
		strFileFilter = _T("VDDA Files (*.ii_vdda)|*.ii_vdda||");
		strFileExtention = _T("ii_vdda");
	}
	else
	{
		strFileFilter = _T("VSDA Files (*.ii_vsda)|*.ii_vsda||");
		strFileExtention = _T("ii_vsda");
	}

	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    strFileFilter);

	if (dlg.DoModal() != IDOK)
		return _T(""); 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".");
		strFullName += strFileExtention;
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
		{
			strFullName += _T(".");
			strFullName += strFileExtention;
		}
		else
		if (strExt.CompareNoCase (strFileExtention) != 0)
		{
			strFullName += _T(".");
			strFullName += strFileExtention;
		}
	}

	return strFullName;
}


static int IsInstallationFile(LPCTSTR lpszFile)
{
	IStream* pStream = NULL;
	CaCompoundFile cmpFile(lpszFile);
	IStorage* pRoot = cmpFile.GetRootStorage();
	ASSERT(pRoot);
	if (!pRoot)
		return -1;
	DWORD grfMode = STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
	int nInstallation = -1;
	//
	// Load Schema Information:
	pStream = cmpFile.OpenStream(NULL, _T("SCHEMAINFO"), grfMode);
	if (pStream)
	{
		int nVersion = 0;
		CString strItem;
		COleStreamFile file (pStream);
		CArchive ar(&file, CArchive::load);

		ar >> strItem;  // Date
		ar >> nVersion; // Version
		ar >> strItem;  // Node
		ar >> strItem;  // Database
		nInstallation = strItem.IsEmpty()? 1: 0;
		ar >> strItem;  // Schema
		ar.Flush();
		pStream->Release();
	}

	return nInstallation;
}

BEGIN_MESSAGE_MAP(CuVddaControl, CuVsda)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

void CuVddaControl::HideFrame(short nShow)
{
	CuDlgMain* pParent = (CuDlgMain*)GetParent();
	if (pParent && IsWindow(pParent->m_hWnd))
	{
		pParent->m_bCompared = FALSE;
	}
	CuVsda::HideFrame(nShow);
}

BOOL CuVddaControl::OnHelpInfo(HELPINFO* pHelpInfo)
{
	CuDlgMain* pParent = (CuDlgMain*)GetParent();
	if (pParent && IsWindow(pParent->m_hWnd))
	{
		if (pParent->m_bCompared)
		{
			CuVsda::OnHelpInfo(pHelpInfo);
			return FALSE;
		}
	}
	APP_HelpInfo(m_hWnd, 0);
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain dialog


CuDlgMain::CuDlgMain(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgMain::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgMain)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nAlreadyRefresh = 0;
	m_bLoadedSchema1 = FALSE;
	m_bLoadedSchema2 = FALSE;
	m_bInitIncluding = FALSE;
	m_bPreselectDBxUser = FALSE;
	m_bCompared = FALSE;
}


void CuDlgMain::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgMain)
	DDX_Control(pDX, IDC_BUTTON_SC2_SAVE, m_cButtonSc2Save);
	DDX_Control(pDX, IDC_BUTTON_SC2_OPEN, m_cButtonSc2Open);
	DDX_Control(pDX, IDC_BUTTON_SC1_SAVE, m_cButtonSc1Save);
	DDX_Control(pDX, IDC_BUTTON_SC1_OPEN, m_cButtonSc1Open);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckIgnoreOwner);
	DDX_Control(pDX, IDC_EDIT1_FILE2, m_cEditFile2);
	DDX_Control(pDX, IDC_EDIT1_FILE1, m_cEditFile1);
	DDX_Control(pDX, IDC_COMBOBOXEX6, m_cComboUser2);
	DDX_Control(pDX, IDC_COMBOBOXEX5, m_cComboDatabase2);
	DDX_Control(pDX, IDC_COMBOBOXEX4, m_cComboNode2);
	DDX_Control(pDX, IDC_COMBOBOXEX3, m_cComboUser1);
	DDX_Control(pDX, IDC_COMBOBOXEX2, m_cComboDatabase1);
	DDX_Control(pDX, IDC_COMBOBOXEX1, m_cComboNode1);
	DDX_Control(pDX, IDC_VSDACTRL1, m_vsdaCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgMain, CDialog)
	//{{AFX_MSG_MAP(CuDlgMain)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX1, OnDropdownComboNode1)
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX4, OnDropdownComboNode2)
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX2, OnDropdownComboDatabase1)
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX5, OnDropdownComboDatabase2)
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX3, OnDropdownComboUser1)
	ON_CBN_DROPDOWN(IDC_COMBOBOXEX6, OnDropdownComboUser2)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonCompare)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX1, OnSelchangeComboNode1)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX4, OnSelchangeComboNode2)
	ON_BN_CLICKED(IDC_BUTTON_SC1_OPEN, OnButtonSc1Open)
	ON_BN_CLICKED(IDC_BUTTON_SC2_OPEN, OnButtonSc2Open)
	ON_BN_CLICKED(IDC_BUTTON_SC1_SAVE, OnButtonSc1Save)
	ON_BN_CLICKED(IDC_BUTTON_SC2_SAVE, OnButtonSc2Save)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX2, OnSelchangeComboDatabase1)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX5, OnSelchangeComboDatabase2)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX3, OnSelchangeComboboxexUser1)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX6, OnSelchangeComboboxexUser2)
	ON_BN_CLICKED(IDC_CHECK1, OnCheckGroup)
	//}}AFX_MSG_MAP
	ON_CLBN_CHKCHANGE (IDC_LIST1, OnCheckChangeIncluding)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgMain message handlers

BOOL CuDlgMain::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_hIconOpen = theApp.LoadIcon (IDI_OPEN);
	m_hIconSave = theApp.LoadIcon (IDI_SAVE);
	m_hIconNode = theApp.LoadIcon (IDI_NODE);
	m_cButtonSc2Save.SetIcon(m_hIconSave);
	m_cButtonSc2Open.SetIcon(m_hIconOpen);
	m_cButtonSc1Save.SetIcon(m_hIconSave);
	m_cButtonSc1Open.SetIcon(m_hIconOpen);

	VERIFY (m_cListCheckBox.SubclassDlgItem (IDC_LIST1, this));
	m_ImageListNode.Create(IDB_NODES, 16, 1, RGB(255, 0, 255));
	m_ImageListUser.Create(IDB_USER, 16, 1, RGB(255, 0, 255));
	m_ImageListDatabase.Create(IDB_DATABASE, 16, 1, RGB(255, 0, 255));

	UINT narrayImage[3] = {IDB_INGRESOBJECT3_16x16, IDB_DATABASE_DISTRIBUTED, IDB_DATABASE_COORDINATOR};
	for (int i = 0; i<3; i++)
	{
		CImageList im;
		if (im.Create(narrayImage[i], 16, 1, RGB(255, 0, 255)))
		{
			HICON hIconBlock = im.ExtractIcon(0);
			if (hIconBlock)
			{
				m_ImageListDatabase.Add (hIconBlock);
				DestroyIcon (hIconBlock);
			}
		}
	}

	m_cComboNode1.SetImageList(&m_ImageListNode);
	m_cComboNode2.SetImageList(&m_ImageListNode);
	m_cComboDatabase1.SetImageList(&m_ImageListDatabase);
	m_cComboDatabase2.SetImageList(&m_ImageListDatabase);
	m_cComboUser1.SetImageList(&m_ImageListUser);
	m_cComboUser2.SetImageList(&m_ImageListUser);
	
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	m_cListCheckBox.SendMessage(WM_SETFONT, (WPARAM)hFont);
	m_cCheckIgnoreOwner.SetCheck(1);
	m_vsdaCtrl.SetSessionStart(500);
	m_vsdaCtrl.SetSessionDescription(_T("Ingres Visual Database Objects Differences Analyzer (vsda.ocx)"));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgMain::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (IsWindow (m_vsdaCtrl.m_hWnd))
	{
		CRect r, rDlg;
		GetClientRect (rDlg);
		m_vsdaCtrl.GetWindowRect(r);
		ScreenToClient(r);
		r.right  = rDlg.right  - r.left;
		r.bottom = rDlg.bottom - r.left;
		m_vsdaCtrl.MoveWindow(r);
	}
}

void CuDlgMain::OnDestroy() 
{
	if (m_hIconOpen)
		DestroyIcon(m_hIconOpen);
	if (m_hIconSave)
		DestroyIcon(m_hIconSave);
	if (m_hIconNode)
		DestroyIcon(m_hIconNode);
	CDialog::OnDestroy();
	while (!m_lNode1.IsEmpty())
		delete m_lNode1.RemoveHead();
	while (!m_lUser1.IsEmpty())
		delete m_lUser1.RemoveHead();
	while (!m_lDatabase1.IsEmpty())
		delete m_lDatabase1.RemoveHead();

	while (!m_lNode2.IsEmpty())
		delete m_lNode2.RemoveHead();
	while (!m_lUser2.IsEmpty())
		delete m_lUser2.RemoveHead();
	while (!m_lDatabase2.IsEmpty())
		delete m_lDatabase2.RemoveHead();
	while (!m_lInluding1.IsEmpty())
		delete m_lInluding1.RemoveHead();
	while (!m_lInluding2.IsEmpty())
		delete m_lInluding2.RemoveHead();
}

void CuDlgMain::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

//
// Ensure that all input parameters have been provided correctly !
void CuDlgMain::CheckValidParameters()
{
	CheckInputParam (1);
	CheckInputParam (2);
}

void CuDlgMain::CheckInputParam(int nParam)
{
	CComboBoxEx* pComboNode = (nParam == 1)? &m_cComboNode1: &m_cComboNode2;
	CComboBoxEx* pComboDatabase   = (nParam == 1)? &m_cComboDatabase1: &m_cComboDatabase2;
	CComboBoxEx* pComboUser   = (nParam == 1)? &m_cComboUser1: &m_cComboUser2;
	CEdit* pEdit = (nParam == 1)? &m_cEditFile1: &m_cEditFile2;
	if (pComboNode->IsWindowVisible() && pComboDatabase->IsWindowVisible() && pComboUser->IsWindowVisible())
	{
		int nSel = pComboNode->GetCurSel();
		if (nSel == CB_ERR)
			throw (int)1;
		CaNode* pNode = (CaNode*)pComboNode->GetItemDataPtr(nSel);
		if (!pNode)
			throw (int)1;
		nSel = pComboDatabase->GetCurSel();
		if (nSel == CB_ERR)
			throw (int)2;
		CaDatabase* pDatabase = (CaDatabase*)pComboDatabase->GetItemDataPtr(nSel);
		if (!pDatabase)
			throw (int)2;
		nSel = pComboUser->GetCurSel();
		if (nSel == CB_ERR)
			throw (int)3;
		CaUser* pUser = (CaUser*)pComboUser->GetItemDataPtr(nSel);
		if (!pUser)
			throw (int)3;
	}
	else
	{
		CString strFile;
		pEdit->GetWindowText(strFile);
		strFile.TrimLeft();
		strFile.TrimRight();
		if (strFile.IsEmpty())
			throw (int)3;
		if (_taccess(strFile, 0) == -1)
			throw (int)4;
	}
}

BOOL CuDlgMain::CheckInstallation()
{
	int nSel = CB_ERR;
	CComboBoxEx* pArray[2] = {&m_cComboDatabase1, &m_cComboDatabase2};
	for (int i=0; i<2; i++)
	{
		if (pArray[i]->IsWindowVisible())
		{
			nSel = pArray[i]->GetCurSel();
			if (nSel != CB_ERR)
			{
				CaDatabase* pDatabase = (CaDatabase*)pArray[i]->GetItemDataPtr(nSel);
				if (pDatabase)
				{
					if (pDatabase->GetItem().IsEmpty())
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}



BOOL CuDlgMain::GetSchemaParam (int nSchema, CString& strNode, CString& strDatabase, CString& strUser)
{
	int nSel = -1;
	CComboBoxEx* pComboNode = (nSchema == 1)? &m_cComboNode1: &m_cComboNode2;
	CComboBoxEx* pComboDatabase   = (nSchema == 1)? &m_cComboDatabase1: &m_cComboDatabase2;
	CComboBoxEx* pComboUser   = (nSchema == 1)? &m_cComboUser1: &m_cComboUser2;

	nSel = pComboNode->GetCurSel();
	if (nSel == CB_ERR)
		throw (int)1;
	CaNode* pNode = (CaNode*)pComboNode->GetItemDataPtr(nSel);
	if (!pNode)
		throw (int)1;
	nSel = pComboDatabase->GetCurSel();
	if (nSel == CB_ERR)
		throw (int)2;
	CaDatabase* pDatabase = (CaDatabase*)pComboDatabase->GetItemDataPtr(nSel);
	if (!pDatabase)
		throw (int)2;
	nSel = pComboUser->GetCurSel();
	if (nSel == CB_ERR)
		throw (int)3;
	CaUser* pUser = (CaUser*)pComboUser->GetItemDataPtr(nSel);
	if (!pUser)
		throw (int)3;

	strNode = pNode->GetName();
	strDatabase = pDatabase->GetItem();
	strUser = pUser->GetItem();
	if (strNode.IsEmpty())
		return FALSE;
	if (strDatabase.IsEmpty())
		strUser = _T("");

	return TRUE;
}

void CuDlgMain::ShowShemaFile(int nSchema, BOOL bShow, LPCTSTR lpszFile)
{
	CRect r1, r2, re;

	CComboBoxEx* pComboNode = (nSchema == 1)? &m_cComboNode1: &m_cComboNode2;
	CComboBoxEx* pComboDatabase = (nSchema == 1)? &m_cComboDatabase1: &m_cComboDatabase2;
	CComboBoxEx* pComboUser = (nSchema == 1)? &m_cComboUser1: &m_cComboUser2;
	CEdit*       pEditFile  = (nSchema == 1)? &m_cEditFile1: &m_cEditFile2;
	CButton*     pSaveButton= (nSchema == 1)? &m_cButtonSc1Save: &m_cButtonSc2Save;

	pComboNode->GetWindowRect (r1);
	ScreenToClient (r1);
	pComboUser->GetWindowRect (r2);
	ScreenToClient (r2);
	pEditFile->GetWindowRect (re);
	ScreenToClient (re);
	int nHeight = re.Height();
	re.left =  r1.left;
	re.top  =  r1.top;
	re.right = r2.right;
	re.bottom =  re.top + nHeight;
	pEditFile->MoveWindow(re);

	int nDbms  = bShow? SW_HIDE: SW_SHOW;
	int nDFile = bShow? SW_SHOW: SW_HIDE;
	HICON hIcon= bShow? m_hIconNode: m_hIconSave;

	pComboNode->ShowWindow (nDbms);
	pComboDatabase->ShowWindow (nDbms);
	pComboUser->ShowWindow (nDbms);
	pEditFile->ShowWindow (nDFile);

	if (lpszFile)
		pEditFile->SetWindowText (lpszFile);
	pSaveButton->SetIcon(hIcon);
}



void CuDlgMain::QueryNode(CComboBoxEx* pComboNode, CString& strSelectedNode)
{
	int nIdx = CB_ERR;
	int nLocal = -1;
	UINT uFlag = (pComboNode == &m_cComboNode1)? REFRESH_NODE1: REFRESH_NODE2;
	CTypedPtrList< CObList, CaDBObject* >* lNode = (pComboNode == &m_cComboNode1)? &m_lNode1: &m_lNode2;

	pComboNode->ResetContent();
	if ((m_nAlreadyRefresh & uFlag) == 0)
	{
		CWaitCursor doWaitCursor;
		while (!lNode->IsEmpty())
			delete lNode->RemoveHead();
		DML_QueryNode(NULL, *lNode);
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	POSITION pos = lNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaNode* pNode = (CaNode*)lNode->GetNext(pos);
		if (pNode->IsLocalNode())
		{
			cbitem.iImage = 1;
			cbitem.iSelectedImage = 1;
		}
		else
		{
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
		}

		cbitem.pszText = (LPTSTR)(LPCTSTR)pNode->GetName();
		cbitem.lParam  = (LPARAM)pNode;
		cbitem.iItem   = pComboNode->GetCount();
		nIdx = pComboNode->InsertItem (&cbitem);
		if (pNode->IsLocalNode())
			nLocal = nIdx;
	}

	if (m_bPreselectDBxUser)
	{
		if (!strSelectedNode.IsEmpty())
		{
			nIdx = pComboNode->FindStringExact(-1, strSelectedNode);
			if (nIdx != CB_ERR)
				pComboNode->SetCurSel(nIdx);
			else
				pComboNode->SetCurSel(0);
		}
		else
		if (nLocal != -1)
		{
			pComboNode->SetCurSel (nLocal);
		}
		else
			pComboNode->SetCurSel (0);
	}

	m_nAlreadyRefresh |= uFlag;
}

void CuDlgMain::QueryUser(CComboBoxEx* pComboUser, CaNode* pNode, CString& strSelectedUser)
{
	int nIdx = CB_ERR;
	CWaitCursor doWaitCursor;
	UINT uFlag = (pComboUser == &m_cComboUser1)? REFRESH_USER1: REFRESH_USER2;
	CTypedPtrList< CObList, CaDBObject* >* lUser = (pComboUser == &m_cComboUser1)? &m_lUser1: &m_lUser2;
	pComboUser->ResetContent();
	if ((m_nAlreadyRefresh & uFlag) == 0)
	{
		while (!lUser->IsEmpty())
			delete lUser->RemoveHead();
		CaUserDefault* pDefaultUser = new CaUserDefault (_T("<Default Schema>"), pNode);
		lUser->AddHead(pDefaultUser);
		CaUserAll* pAllUser = new CaUserAll (_T("<All Schemas>"));
		lUser->AddHead(pAllUser);
		DML_QueryUser(NULL, pNode, *lUser);
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	int nDefault = 0;

	POSITION pos = lUser->GetHeadPosition();
	while (pos != NULL)
	{
		CaUser* pUser = (CaUser*)lUser->GetNext(pos);
		cbitem.pszText = (LPTSTR)(LPCTSTR)pUser->GetName();
		cbitem.lParam  = (LPARAM)pUser;
		cbitem.iItem   = pComboUser->GetCount();
		pComboUser->InsertItem (&cbitem);
	}
	if (m_bPreselectDBxUser)
	{
		if (!strSelectedUser.IsEmpty())
		{
			nIdx = pComboUser->FindStringExact(-1, strSelectedUser);
			if (nIdx != CB_ERR)
				pComboUser->SetCurSel(nIdx);
			else
				pComboUser->SetCurSel(nDefault);
		}
		else
		{
			pComboUser->SetCurSel(nDefault);
		}
	}
	m_nAlreadyRefresh |= uFlag;
}

void CuDlgMain::QueryDatabase(CComboBoxEx* pComboDatabase, CaNode* pNode, CString& strSelectedDatabase)
{
	int nIdx = CB_ERR;
	CWaitCursor doWaitCursor;
	UINT uFlag = (pComboDatabase == &m_cComboDatabase1)? REFRESH_DATABASE1: REFRESH_DATABASE2;
	CTypedPtrList< CObList, CaDBObject* >* lDatabase = (pComboDatabase == &m_cComboDatabase1)? &m_lDatabase1: &m_lDatabase2;
	//
	// Clean the combobox:
	pComboDatabase->ResetContent();
	if ((m_nAlreadyRefresh & uFlag) == 0)
	{
		while (!lDatabase->IsEmpty())
			delete lDatabase->RemoveHead();
		CaInstallationItem* pInstallation = new CaInstallationItem (_T("<Installation>"));
		lDatabase->AddHead(pInstallation);
		DML_QueryDatabase(NULL, pNode, *lDatabase);
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;

	POSITION pos = lDatabase->GetHeadPosition();
	while (pos != NULL)
	{
		CaDatabase* pDatabase = (CaDatabase*)lDatabase->GetNext(pos);
		cbitem.pszText = (LPTSTR)(LPCTSTR)pDatabase->GetName();
		cbitem.lParam  = (LPARAM)pDatabase;
		cbitem.iItem   = pComboDatabase->GetCount();
		switch ( pDatabase->GetStar() )
		{
		case OBJTYPE_NOTSTAR:
			cbitem.iImage  = (pDatabase->GetItem().IsEmpty())? 1: 0;
			break;
		case OBJTYPE_STARNATIVE:
			cbitem.iImage  = 2;
			break;
		case OBJTYPE_STARLINK:
			cbitem.iImage  = 3;
			break;
		}
		cbitem.iSelectedImage = cbitem.iImage;
		pComboDatabase->InsertItem (&cbitem);
	}
	if (m_bPreselectDBxUser)
	{
		if (!strSelectedDatabase.IsEmpty())
		{
			nIdx = pComboDatabase->FindStringExact(-1, strSelectedDatabase);
			if (nIdx != CB_ERR)
				pComboDatabase->SetCurSel(nIdx);
			else
				pComboDatabase->SetCurSel(0);
		}
		else
		{
			pComboDatabase->SetCurSel(0);
		}
	}
	m_nAlreadyRefresh |= uFlag;
}


void CuDlgMain::OnDropdownComboNode1() 
{
	try
	{
		CString strNode = _T("(local)");
		QueryNode(&m_cComboNode1, strNode);
	}
	catch (CeNodeException e)
	{
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
}

void CuDlgMain::OnDropdownComboNode2() 
{
	try
	{
		CString strNode = _T("(local)");
		QueryNode(&m_cComboNode2, strNode);
	}
	catch (CeNodeException e)
	{
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
}

void CuDlgMain::OnDropdownComboDatabase1() 
{
	try
	{
		int nSel = m_cComboNode1.GetCurSel();
		if (nSel != CB_ERR)
		{
			CaNode* pNode = (CaNode*)m_cComboNode1.GetItemDataPtr(nSel);
			if (pNode)
			{
				CString strSelectedDatabase = _T("");
				QueryDatabase(&m_cComboDatabase1, pNode, strSelectedDatabase);
			}
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
	m_vsdaCtrl.HideFrame(1);
}

void CuDlgMain::OnDropdownComboDatabase2() 
{
	try
	{
		int nSel = m_cComboNode2.GetCurSel();
		if (nSel != CB_ERR)
		{
			CaNode* pNode = (CaNode*)m_cComboNode2.GetItemDataPtr(nSel);
			if (pNode)
			{
				CString strSelectedDatabase = _T("");
				QueryDatabase(&m_cComboDatabase2, pNode, strSelectedDatabase);
			}
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
	m_vsdaCtrl.HideFrame(1);
}

void CuDlgMain::OnDropdownComboUser1() 
{
	int nSel = m_cComboNode1.GetCurSel();
	if (nSel != CB_ERR)
	{
		CaNode* pNode = (CaNode*)m_cComboNode1.GetItemDataPtr(nSel);
		if (pNode)
		{
			CString strSelectedUser = _T("");
			QueryUser(&m_cComboUser1, pNode, strSelectedUser);
		}
	}
}

void CuDlgMain::OnDropdownComboUser2() 
{
	int nSel = m_cComboNode2.GetCurSel();
	if (nSel != CB_ERR)
	{
		CaNode* pNode = (CaNode*)m_cComboNode2.GetItemDataPtr(nSel);
		if (pNode)
		{
			CString strSelectedUser = _T("");
			QueryUser(&m_cComboUser2, pNode, strSelectedUser);
		}
	}
}

void CuDlgMain::DoCompare()
{
	CString strNode;
	CString strDatabase;
	CString strUser;
	try
	{
		CheckValidParameters();
		if (m_bLoadedSchema1)
		{
			CString strFile;
			m_cEditFile1.GetWindowText(strFile);
			m_vsdaCtrl.LoadSchema1Param(strFile);
		}
		else
		if (GetSchemaParam (1, strNode, strDatabase, strUser))
		{
			m_vsdaCtrl.SetSchema1Param (strNode, strDatabase, strUser);
		}
		else
			return;

		if (m_bLoadedSchema2)
		{
			CString strFile;
			m_cEditFile2.GetWindowText(strFile);
			m_vsdaCtrl.LoadSchema2Param(strFile);
		}
		else
		if (GetSchemaParam (2, strNode, strDatabase, strUser))
		{
			m_vsdaCtrl.SetSchema2Param (strNode, strDatabase, strUser);
		}
		else
			return;
		short nCheck = m_cCheckIgnoreOwner.GetCheck();
		m_vsdaCtrl.DoCompare(nCheck);
		CString strFilter = _T("");
		GetFilter(strFilter);
		m_vsdaCtrl.UpdateDisplayFilter(strFilter);
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 1:
		case 2:
		case 3:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 4:
			AfxMessageBox (IDS_MSG_FILE_NOTFOUND);
			break;
		default:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		}
	}
	m_bCompared = TRUE;
}

void CuDlgMain::OnButtonCompare() 
{
	DoCompare();
	theApp.GetSessionManager().Cleanup(); // Disconnect all the DBMS sessions
}

void CuDlgMain::OnSelchangeComboNode1() 
{
	try
	{
		m_nAlreadyRefresh &= ~REFRESH_DATABASE1;
		m_nAlreadyRefresh &= ~REFRESH_USER1;
		int nSel = m_cComboNode1.GetCurSel();
		CaNode* pNode = (CaNode*)m_cComboNode1.GetItemDataPtr(nSel);
		if (m_bPreselectDBxUser)
		{
			if (pNode)
			{
				CString strSelectedItem = _T("");
				QueryDatabase(&m_cComboDatabase1, pNode, strSelectedItem);
				QueryUser(&m_cComboUser1, pNode, strSelectedItem);
			}
		}
		else
		{
			CString strSelectedUser = _T("");
			if (pNode)
				QueryUser(&m_cComboUser1, pNode, strSelectedUser);
			m_cComboDatabase1.SetCurSel(-1);
			m_cComboUser1.SetCurSel(-1);
		}
		m_cComboUser1.EnableWindow(TRUE);
	}
	catch (CeSqlException e)
	{
		if (e.GetErrorCode() == -30140)
			AfxMessageBox (IDS_MSG_USER_UNAVAILABLE);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
	m_vsdaCtrl.HideFrame(1);
}

void CuDlgMain::OnSelchangeComboNode2() 
{
	try
	{
		m_nAlreadyRefresh &= ~REFRESH_DATABASE2;
		m_nAlreadyRefresh &= ~REFRESH_USER2;
		int nSel = m_cComboNode2.GetCurSel();
		CaNode* pNode = (CaNode*)m_cComboNode2.GetItemDataPtr(nSel);
		if (m_bPreselectDBxUser)
		{
			if (pNode)
			{
				CString strSelectedItem = _T("");
				QueryDatabase(&m_cComboDatabase2, pNode, strSelectedItem);
				QueryUser(&m_cComboUser2, pNode, strSelectedItem);
			}
		}
		else
		{
			CString strSelectedUser = _T("");
			CaNode* pNode = (CaNode*)m_cComboNode2.GetItemDataPtr(nSel);
			if (pNode)
				QueryUser(&m_cComboUser2, pNode, strSelectedUser);
			m_cComboDatabase2.SetCurSel(-1);
			m_cComboUser2.SetCurSel(-1);
		}
		m_cComboUser2.EnableWindow(TRUE);
	}
	catch (CeSqlException e)
	{
		if (e.GetErrorCode() == -30140)
			AfxMessageBox (IDS_MSG_USER_UNAVAILABLE);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
	}
	m_vsdaCtrl.HideFrame(1);
}

void CuDlgMain::OnButtonSc1Open() 
{
	try
	{
		CString strFullName = GetSchemaFile();
		if (!strFullName.IsEmpty())
		{
			ShowShemaFile(1, TRUE, strFullName);
			TRACE1("CuDlgMain::OnButtonSc1Open() = %s\n", strFullName);
			m_bLoadedSchema1 = TRUE;
		}
		InitializeInluding();
		InitializeCheckIOW();
		m_vsdaCtrl.HideFrame(1);
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 1:
		case 2:
		case 3:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 4:
			AfxMessageBox (IDS_MSG_FILE_NOTFOUND);
			break;
		default:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		}
	}	
}

void CuDlgMain::OnButtonSc2Open() 
{
	try
	{
		CString strFullName = GetSchemaFile();
		if (!strFullName.IsEmpty())
		{
			ShowShemaFile(2, TRUE, strFullName);

			TRACE1("CuDlgMain::OnButtonSc2Open() = %s\n", strFullName);
			m_bLoadedSchema2 = TRUE;
		}
		InitializeInluding();
		InitializeCheckIOW();
		m_vsdaCtrl.HideFrame(1);
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 1:
		case 2:
		case 3:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 4:
			AfxMessageBox (IDS_MSG_FILE_NOTFOUND);
			break;
		default:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		}
	}
}

void CuDlgMain::OnButtonSc1Save() 
{
	try
	{
		if (m_bLoadedSchema1)
		{
			m_bLoadedSchema1 = FALSE;
			ShowShemaFile(1, FALSE, NULL);
			return;
		}

		CString strNode;
		CString strDatabase;
		CString strUser;
		if (!GetSchemaParam (1, strNode, strDatabase, strUser))
			return;
		CString strFile = GetSchemaFile4Save(strDatabase);
		if (strFile.IsEmpty())
			return;
		CWaitCursor doWaitCursor;
		Sleep(2000); 
		m_vsdaCtrl.StoreSchema(strNode, strDatabase, strUser, strFile);
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 1:
		case 2:
		case 3:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 4:
			AfxMessageBox (IDS_MSG_FILE_NOTFOUND);
			break;
		default:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		}
	}
}

void CuDlgMain::OnButtonSc2Save() 
{
	try
	{
		if (m_bLoadedSchema2)
		{
			m_bLoadedSchema2 = FALSE;
			ShowShemaFile(2, FALSE, NULL);
			return;
		}

		CString strNode;
		CString strDatabase;
		CString strUser;
		if (!GetSchemaParam (2, strNode, strDatabase, strUser))
			return;
		CString strFile = GetSchemaFile4Save(strDatabase);
		if (strFile.IsEmpty())
			return;
		CWaitCursor doWaitCursor;
		Sleep(2000); 
		m_vsdaCtrl.StoreSchema(strNode, strDatabase, strUser, strFile);
	}
	catch(int nCode)
	{
		switch (nCode)
		{
		case 1:
		case 2:
		case 3:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		case 4:
			AfxMessageBox (IDS_MSG_FILE_NOTFOUND);
			break;
		default:
			AfxMessageBox (IDS_MSG_PROVIDE_INPUTPARAM);
			break;
		}
	}
}

static void PreselectUser(CComboBoxEx* pComboSource, CComboBoxEx* pComboUser)
{
	int nSel = pComboSource->GetCurSel();
	if (nSel != CB_ERR)
	{
		CaDatabase* pDatabase = (CaDatabase*)pComboSource->GetItemDataPtr(nSel);
		if (pDatabase)
		{
			if (pDatabase->GetItem().IsEmpty())
			{
				pComboUser->SetCurSel(0); // <All Schemas>
				pComboUser->EnableWindow(FALSE);
			}
			else
			{
				if (pComboUser->GetCurSel() == CB_ERR)
				{
					pComboUser->SetCurSel(1);  // <Default Schema>
				}
				pComboUser->EnableWindow(TRUE);
			}
		}
	}
}


void CuDlgMain::OnSelchangeComboDatabase1() 
{
	PreselectUser(&m_cComboDatabase1, &m_cComboUser1);
	InitializeInluding();
	InitializeCheckIOW();
}

void CuDlgMain::OnSelchangeComboDatabase2() 
{
	PreselectUser(&m_cComboDatabase2, &m_cComboUser2);
	InitializeInluding();
	InitializeCheckIOW();
}


void CuDlgMain::GetFilter(CString& strFilter)
{
	BOOL bOne = TRUE;
	CString strItem;
	int i, nCheck = 0, nCount = m_cListCheckBox.GetCount();
	for (i=0; i<nCount; i++)
	{
		CaIncluding* pObj = (CaIncluding*)m_cListCheckBox.GetItemData(i);
		if (!m_cListCheckBox.GetCheck(i))
		{
			pObj->SetInclude(FALSE);
			m_cListCheckBox.GetText (i, strItem);
			if (!bOne)
				strFilter += _T(",");
			strFilter += strItem;
			bOne = FALSE;
		}
		else
		{
			pObj->SetInclude(TRUE);
		}
	}
}

void CuDlgMain::OnCheckChangeIncluding()
{
	TRACE0("CuDlgMain::OnCheckChangeIncluding()\n");

	CString strFilter = _T("");
	GetFilter(strFilter);
	m_vsdaCtrl.UpdateDisplayFilter(strFilter);
}


int CuDlgMain::GetInstallationParam(int nSchema)
{
	int nInstallation = -1;
	CRect r1, r2, re;
	CComboBoxEx* pComboDatabase = (nSchema == 1)? &m_cComboDatabase1: &m_cComboDatabase2;
	CEdit*       pEditFile  = (nSchema == 1)? &m_cEditFile1: &m_cEditFile2;
	if (pEditFile->IsWindowVisible())
	{
		CString strFile;
		pEditFile->GetWindowText(strFile);
		strFile.TrimLeft();
		strFile.TrimRight();
		if (strFile.IsEmpty())
			return -1;
		if (_taccess(strFile, 0) == -1)
			return -1;
		nInstallation = IsInstallationFile(strFile);
	}
	else
	{
		int nSel = pComboDatabase->GetCurSel();
		if (nSel != CB_ERR)
		{
			CaDatabase* pDatabase = (CaDatabase*)pComboDatabase->GetItemDataPtr(nSel);
			if (pDatabase)
			{
				CString strDb = pDatabase->GetItem();
				nInstallation = strDb.IsEmpty()? 1: 0;
			}
		}
	}
	return nInstallation;
}


void CuDlgMain::InitializeInluding()
{
	if (!m_bInitIncluding)
	{
		CaIncluding* pObj = NULL;
		m_lInluding1.AddTail(new CaIncluding(_T("Users"),     TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Groups"),    TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Roles"),     TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Profiles"),  TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Location"),  TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Databases"), TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Tables"),    TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Views"),     TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Procedures"),TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Sequences"), TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("DBEvents"),  TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Synonym"),   TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Grants"),    TRUE));
		m_lInluding1.AddTail(new CaIncluding(_T("Alarms"),    TRUE));
		m_lInluding2.AddTail(new CaIncluding(_T("Tables"),    FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Views"),     FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Procedures"),FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Sequences"), FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("DBEvents"),  FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Synonym"),   FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Grants"),    FALSE));
		m_lInluding2.AddTail(new CaIncluding(_T("Alarms"),    FALSE));
		m_bInitIncluding = TRUE;
	}

	m_cListCheckBox.ResetContent();
	int nInst1 = GetInstallationParam(1);
	int nInst2 = GetInstallationParam(2);
	if (nInst1 != -1 && nInst2 != -1)
	{
		CTypedPtrList< CObList, CaIncluding*>* pList = NULL;
		BOOL bInstallation = (nInst1 == 1 && nInst2 == 1);
		if (bInstallation)
		{
			pList = &m_lInluding1;
		}
		else
		if(nInst1 == 0 && nInst2 == 0)
		{
			pList = &m_lInluding2;
		}

		if (pList)
		{
			POSITION pos = pList->GetHeadPosition();
			while (pos != NULL)
			{
				CaIncluding* pObj = pList->GetNext(pos);
				int nIdx = m_cListCheckBox.AddString(pObj->GetName());
				if (nIdx != LB_ERR)
				{
					m_cListCheckBox.SetItemData(nIdx, (DWORD)pObj);
					m_cListCheckBox.SetCheck(nIdx, pObj->GetInclude());
				}
			}
		}
	}
}

void CuDlgMain::InitializeCheckIOW()
{
	int nSel = CB_ERR;
	CStringArray arrayDB;
	CEdit* pEdit[2] = {&m_cEditFile1, &m_cEditFile2};
	CComboBoxEx* pArray[2] = {&m_cComboDatabase1, &m_cComboDatabase2};
	for (int i=0; i<2; i++)
	{
		if (pEdit[i]->IsWindowVisible())
		{
			CString strFile;
			pEdit[i]->GetWindowText(strFile);
			strFile.TrimLeft();
			strFile.TrimRight();
			if (!strFile.IsEmpty() && _taccess(strFile, 0) != -1)
			{
				int nInst = IsInstallationFile(strFile);
				if (nInst == 1)
					arrayDB.Add(_T(""));
				else
					arrayDB.Add(_T("???"));
			}
		}
		else
		{
			nSel = pArray[i]->GetCurSel();
			if (nSel != CB_ERR)
			{
				CaDatabase* pDatabase = (CaDatabase*)pArray[i]->GetItemDataPtr(nSel);
				if (pDatabase)
				{
					arrayDB.Add(pDatabase->GetItem());
				}
			}
		}
	}

	if (arrayDB.GetSize() == 2)
	{
		CString s1 = arrayDB[0];
		CString s2 = arrayDB[1];
		if (s1.IsEmpty() && s2.IsEmpty())
		{
			m_cCheckIgnoreOwner.ShowWindow (SW_SHOW);
		}
		else
		{
			m_cCheckIgnoreOwner.SetCheck(1);
			m_cCheckIgnoreOwner.ShowWindow (SW_HIDE);
		}
	}
}

void CuDlgMain::OnSelchangeComboboxexUser1() 
{
	m_vsdaCtrl.HideFrame(1);
	
}

void CuDlgMain::OnSelchangeComboboxexUser2() 
{
	m_vsdaCtrl.HideFrame(1);
}

void CuDlgMain::OnCheckGroup() 
{
	m_vsdaCtrl.HideFrame(1);
}

