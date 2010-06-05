/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgpref.cpp Implementation File
**
**
**    Project  : INGRES II / VDBA
**    Author   : UK Sotheavut
**
**
**    Purpose  : Dialog for Preference Settings 
**
** History:
** 25-Jan-2000 (uk$so01)
**    bug #100104: When running through the context driven mode,
**    the preferece items available are those that are aplicable
**    in that context.
** 16-feb-2000 (somsa01)
**    Removed extra comma in enum declaration.
** 19-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 05-Aug-2003 (uk$so01)
**    SIR #106648, Remove printer setting
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 31-Oct-2003 (noifr01)
**    restored trailing } that had been lost upon ingres30->main
**    codeline propagation, and added trailing CR for avoiding
**    future similar problems
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 27-Jul-2004 (uk$so01)
**    SIR #111701, Use PushHelpId(0) to disable Global F1-key for help
**    when the ActiveX property is invoked.
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
** 11-May-2010 (drivi01)
**    Remove resource.h from the file.  resource.h doesn't
**    exist in this directory. dlgres.h will be used instead.
**    The files are now generated.
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "xdlgpref.h"
#include "xdlggset.h"
#include "bkgdpref.h"
#include "qsqlview.h"
#include "axipm.h"
#include "property.h"
#include "rcdepend.h"

extern "C"
{
#include "dlgres.h"
//
// These following functions are coded in file "main.c":
void ManageSessionPreferenceSettup(HWND hWndDialog);
BOOL ManageDomPreferences(HWND hwndMain, HWND hwndParent);
void ManageNodesPreferences(HWND hwndParent);
#include "main.h"
}

enum
{
	IM_SESSION = 0,  // Session
	IM_DOM,          // DOM
	IM_SQLTEST,      // SQL Test
	IM_REFRESH,      // Refresh
	IM_NODE,         // Node
	IM_MONITOR,      // Monitor
	IM_DISPLAY       // General Display
};

//
// Use with care, these are global variables from the old c code !!
extern "C" HWND hwndFrame;


#define LISTICONCOUNT  8
#define DISPLAY_BIGICON   0
#define DISPLAY_SMALLICON 1
#define DISPLAY_REPORT    2
#define DISPLAY_LIST      3

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const BOOL b_use_displaysetting = TRUE;

typedef struct tagDISPLAYSORT
{
	BOOL bAsc;
	int  iSubItem;
} DISPLAYSORT;


class CaPreferenceItemData: public CObject
{
public:
	CaPreferenceItemData(): m_nID (0), m_strItem (_T("")), m_strDescription (_T("")){}
	CaPreferenceItemData(UINT nID, LPCTSTR lpszItem);
	virtual ~CaPreferenceItemData(){}

public:
	UINT m_nID;
	CString m_strItem;
	CString m_strDescription;
};

CaPreferenceItemData::CaPreferenceItemData(UINT nID, LPCTSTR lpszItem)
{
	m_nID =  nID;
	m_strItem = lpszItem;
	m_strDescription.LoadString (m_nID);
}


static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	DISPLAYSORT* pData = (DISPLAYSORT*)lParamSort;
	CaPreferenceItemData* pItem1 = (CaPreferenceItemData*)lParam1;
	CaPreferenceItemData* pItem2 = (CaPreferenceItemData*)lParam2;

	if (pData->iSubItem == 0)
	{
		if (pData->bAsc)
			return pItem1->m_strItem.CompareNoCase (pItem2->m_strItem);
		else
			return pItem2->m_strItem.CompareNoCase (pItem1->m_strItem);
	}
	else
	{
		if (pData->bAsc)
			return pItem1->m_strDescription.CompareNoCase (pItem2->m_strDescription);
		else
			return pItem2->m_strDescription.CompareNoCase (pItem1->m_strDescription);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgPreferences dialog


CxDlgPreferences::CxDlgPreferences(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgPreferences::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgPreferences)
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
	m_nViewMode = DISPLAY_BIGICON;
	m_nSortColumn = 0;
	m_bAsc = TRUE;
	m_bPropertyChange = FALSE;
}


void CxDlgPreferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgPreferences)
	DDX_Control(pDX, IDC_MFC_LIST1, m_cListCtrl);
	DDX_Text(pDX, IDC_MFC_STATIC1, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgPreferences, CDialog)
	//{{AFX_MSG_MAP(CxDlgPreferences)
	ON_WM_DESTROY()
	ON_NOTIFY(NM_DBLCLK, IDC_MFC_LIST1, OnDblclkList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFC_LIST1, OnItemchangedList1)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickList1)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_SETTINGS_SESSIONS, OnSettingSessions)
	ON_COMMAND(ID_SETTINGS_DOM, OnSettingDOM)
	ON_COMMAND(ID_SETTINGS_SQLTEST, OnSettingSQLTest)
	ON_COMMAND(ID_SETTINGS_REFRESH, OnSettingRefresh)
	ON_COMMAND(ID_SETTINGS_NODES, OnSettingNodes)
	ON_COMMAND(ID_SETTINGS_MONITOR, OnSettingMonitor)
	ON_COMMAND(ID_SETTINGS_GENERALDISPLAY, OnSettingGeneralDisplay)
	ON_COMMAND(ID_SETTINGS_CLOSE, OnSettingClose)

	ON_COMMAND(ID_DISPLAY_BIGICON, OnDisplayBigIcon)
	ON_COMMAND(ID_DISPLAY_SMALLICON, OnDisplaySmallIcon)
	ON_COMMAND(ID_DISPLAY_REPORT, OnDisplayReport)
	ON_COMMAND(ID_DISPLAY_LIST, OnDisplayList)
END_MESSAGE_MAP()

static const int SqlPropertiesChange  = 1;
static const int IpmPropertiesChange = 2;

BEGIN_EVENTSINK_MAP(CxDlgPreferences, CDialog)
	ON_EVENT(CxDlgPreferences, IDC_STATIC_01, SqlPropertiesChange, OnPropertyChange, VTS_NONE)
	ON_EVENT(CxDlgPreferences, IDC_STATIC_01, IpmPropertiesChange, OnPropertyChange, VTS_NONE)
END_EVENTSINK_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgPreferences message handlers


BOOL CxDlgPreferences::OnInitDialog() 
{
	LV_COLUMN lvcolumn;
	TCHAR     rgtsz[2][32];
	lstrcpy(rgtsz [0],VDBA_MfcResourceString(IDS_TC_NAME));        // _T("Name")
	lstrcpy(rgtsz [1],VDBA_MfcResourceString(IDS_TC_DESCRIPTION)); // _T("Description")
	CDialog::OnInitDialog();

	m_ImageListBig.Create(32, 32, TRUE, LISTICONCOUNT, 8);
	m_ImageListSmall.Create (IDB_PREFERENCE_SMALL, 16, 1, RGB (255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageListBig, LVSIL_NORMAL);
	m_cListCtrl.SetImageList(&m_ImageListSmall, LVSIL_SMALL);

	m_ImageListBig.Add(theApp.LoadIcon(IDI_CFGICON2));        // Session
	m_ImageListBig.Add(theApp.LoadIcon(IDI_CFGICON3));        // DOM
	m_ImageListBig.Add(theApp.LoadIcon(IDI_SQL));             // SQL Test
	m_ImageListBig.Add(theApp.LoadIcon(IDI_CFGICON1));        // Refresh
	m_ImageListBig.Add(theApp.LoadIcon(IDI_CFGICON14));       // Node
	m_ImageListBig.Add(theApp.LoadIcon(IDI_CFGICON15));       // Monitor
	m_ImageListBig.Add(theApp.LoadIcon(IDR_GENERAL_DISPLAY)); // General Display

	for (int i = 0; i < 2; i++) 
	{
		lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = rgtsz[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = (i==0)? 100: 200;
		m_cListCtrl.InsertColumn(i, &lvcolumn);
	}

	//
	// Setup the display mode:
	CaGeneralDisplaySetting* pGD = theApp.GetGeneralDisplaySetting();
	if (pGD)
	{
		m_nViewMode = pGD->m_nGeneralDisplayView;
		m_nSortColumn = pGD->m_nSortColumn;
		m_bAsc  = pGD->m_bAsc;
	}

	InitializeItems();
	CMenu* pMenu = GetMenu();
	if (pMenu)
	{
		if (!b_use_displaysetting)
		{
			pMenu->RemoveMenu (ID_DISPLAY_BIGICON, MF_BYCOMMAND);
			pMenu->RemoveMenu (ID_DISPLAY_SMALLICON, MF_BYCOMMAND);
			pMenu->RemoveMenu (ID_DISPLAY_REPORT, MF_BYCOMMAND);
			pMenu->RemoveMenu (ID_DISPLAY_LIST, MF_BYCOMMAND);
			pMenu->RemoveMenu (1, MF_BYPOSITION);
		}
		else
		{
			int nID = ID_DISPLAY_BIGICON;
			switch (m_nViewMode)
			{
			case DISPLAY_BIGICON:
				nID = ID_DISPLAY_BIGICON;
				break;
			case DISPLAY_SMALLICON:
				nID = ID_DISPLAY_SMALLICON;
				break;
			case DISPLAY_REPORT:
				nID = ID_DISPLAY_REPORT;
				break;
			case DISPLAY_LIST:
				nID = ID_DISPLAY_LIST;
				break;
			default:
				break;
			}
			ChangeViewMode (m_nViewMode);
			pMenu->CheckMenuRadioItem (ID_DISPLAY_BIGICON, ID_DISPLAY_LIST, nID, MF_BYCOMMAND);
		}
	}
	OnSort(m_nSortColumn);
	PushHelpId (IDD_PREFERENCES_WINDOW);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgPreferences::OnDestroy() 
{
	CaPreferenceItemData* pItem = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pItem = (CaPreferenceItemData*)m_cListCtrl.GetItemData (i);
		if (pItem)
			delete pItem;
	}
	CDialog::OnDestroy();
	PopHelpId();
}

void CxDlgPreferences::OnExecuteItem()
{
	CaPreferenceItemData* pItem = NULL;
	int nSel = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (nSel == -1)
		return;
	pItem = (CaPreferenceItemData*)m_cListCtrl.GetItemData (nSel);
	if (!pItem)
		return;
	switch (pItem->m_nID)
	{
	case ID_SETTINGS_SESSIONS:
		OnSettingSessions();
		break;
	case ID_SETTINGS_DOM:
		OnSettingDOM();
		break;
	case ID_SETTINGS_SQLTEST:
		OnSettingSQLTest();
		break;
	case ID_SETTINGS_REFRESH:
		OnSettingRefresh();
		break;
	case ID_SETTINGS_NODES:
		OnSettingNodes();
		break;
	case ID_SETTINGS_MONITOR:
		OnSettingMonitor();
		break;
	case ID_SETTINGS_GENERALDISPLAY:
		OnSettingGeneralDisplay();
		break;
	default:
		break;
	}
}


void CxDlgPreferences::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnExecuteItem();
	*pResult = 0;
}


void CxDlgPreferences::OnSettingSessions()
{
	ManageSessionPreferenceSettup(m_hWnd);
}

void CxDlgPreferences::OnSettingDOM()
{
	ManageDomPreferences(hwndFrame, m_hWnd);
}

void CxDlgPreferences::OnSettingSQLTest()
{
	USES_CONVERSION;
	HRESULT hr = NOERROR;
	IPersistStreamInit* pPersistStreammInit = NULL;
	IUnknown* pUnk[256];
	BOOL bOK = FALSE;
	BOOL bOpenProperty = FALSE;

	if (!theApp.m_pUnknownSqlQuery)
	{
		CuSqlQueryControl ccl;
		HRESULT hError = CoCreateInstance(
			ccl.GetClsid(),
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&(theApp.m_pUnknownSqlQuery));
		if (!SUCCEEDED(hError))
			theApp.m_pUnknownSqlQuery = NULL;
	}

	CuSqlQueryControl ctrl;
	CPtrArray arrayUnknown;
	GetActiveControlUnknow(arrayUnknown, AXCONTROL_SQL);
	int nSize = arrayUnknown.GetSize();
	if (nSize > 256)
		nSize = 256;
	if (nSize == 0)
	{
		CRect r;
		GetClientRect (r);
		bOK = ctrl.Create (NULL, NULL, WS_CHILD, r, this, IDC_STATIC_01);
		if (bOK)
		{
			ctrl.ShowWindow(SW_HIDE);
			ctrl.SetHelpFile(VDBA_GetHelpFileName());
			pUnk[0] = ctrl.GetControlUnknown();

			if (!theApp.m_bsqlStreamFile)
			{
				LoadSqlQueryPreferences(pUnk[0]);
			}
			//
			// Load the global preferences:
			if (theApp.m_bsqlStreamFile)
			{
				IPersistStreamInit* pPersistStreammInit = NULL;
				HRESULT hr = pUnk[0]->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
				if(SUCCEEDED(hr) && pPersistStreammInit)
				{
					theApp.m_sqlStreamFile.SeekToBegin();
					IStream* pStream = theApp.m_sqlStreamFile.GetStream();
					hr = pPersistStreammInit->Load(pStream);
					pPersistStreammInit->Release();
				}
			}
			bOpenProperty = TRUE;
		}
	}
	else
	{
		for (int i=0; i<nSize; i++)
			pUnk[i] = (IUnknown*)arrayUnknown.GetAt(i);
		bOpenProperty = TRUE;
	}

	if (!bOpenProperty)
		return;

	CString strPropertyCaption;
	strPropertyCaption.LoadString(IDS_CAPT_SQLTEST);
	LPCOLESTR lpCaption = T2COLE((LPTSTR)(LPCTSTR)strPropertyCaption);

	ISpecifyPropertyPagesPtr pSpecifyPropertyPages = pUnk[0];
	CAUUID pages;
	hr = pSpecifyPropertyPages->GetPages( &pages );
	if(FAILED(hr ))
		return;

	PushHelpId ((UINT)0/*SQLACTPREFDIALOG*/); // use ID = 0 to disable F1 key
	theApp.PropertyChange(FALSE);
	hr = OleCreatePropertyFrame(
		m_hWnd,
		10,
		10,
		lpCaption,
		(nSize==0)? 1: nSize,
		(IUnknown**) pUnk,
		pages.cElems,
		pages.pElems,
		0, 
		0L,
		NULL );
	PopHelpId ();

	CoTaskMemFree( (void*) pages.pElems );
	if (theApp.IsPropertyChange())
	{
		//
		// Save to Global:
		IPersistStreamInit* pPersistStreammInit = NULL;
		hr = pUnk[0]->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			IStream* pStream = theApp.m_sqlStreamFile.Detach();
			pStream->Release();
			if (theApp.m_sqlStreamFile.CreateMemoryStream())
			{
				pStream = theApp.m_sqlStreamFile.GetStream();
				hr = pPersistStreammInit->Save(pStream, TRUE);
				pPersistStreammInit->Release();
			}
		}
		if (theApp.IsSavePropertyAsDefault())
		{
			BOOL bSav = SaveStreamInit(pUnk[0], _T("SqlQuery"));
			if (!bSav)
				AfxMessageBox (IDS_FAILED_2_SAVESTORAGE);
		}
	}
	if (nSize == 0)
		ctrl.DestroyWindow();
}

void CxDlgPreferences::OnSettingRefresh()
{
	CxDlgPreferencesBackgroundRefresh dlg;
	dlg.DoModal();
}

void CxDlgPreferences::OnSettingNodes()
{
	ManageNodesPreferences(m_hWnd);
}

void CxDlgPreferences::OnSettingMonitor()
{
	USES_CONVERSION;
	HRESULT hr = NOERROR;
	IPersistStreamInit* pPersistStreammInit = NULL;
	IUnknown* pUnk[256];
	BOOL bOK = FALSE;
	BOOL bOpenProperty = FALSE;

	CuIpm ctrl;
	CPtrArray arrayUnknown;
	GetActiveControlUnknow(arrayUnknown, AXCONTROL_IPM);
	int nSize = arrayUnknown.GetSize();
	if (nSize > 256)
		nSize = 256;
	if (nSize == 0)
	{
		CRect r;
		GetClientRect (r);
		bOK = ctrl.Create (NULL, NULL, WS_CHILD, r, this, IDC_STATIC_01);
		if (bOK)
		{
			ctrl.ShowWindow(SW_HIDE);
			ctrl.SetHelpFile(VDBA_GetHelpFileName());
			pUnk[0] = ctrl.GetControlUnknown();

			if (!theApp.m_bipmStreamFile)
			{
				LoadIpmPreferences(pUnk[0]);
			}

			//
			// Load the global preferences:
			if (theApp.m_bipmStreamFile)
			{
				IPersistStreamInit* pPersistStreammInit = NULL;
				HRESULT hr = pUnk[0]->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
				if(SUCCEEDED(hr) && pPersistStreammInit)
				{
					theApp.m_ipmStreamFile.SeekToBegin();
					IStream* pStream = theApp.m_ipmStreamFile.GetStream();
					hr = pPersistStreammInit->Load(pStream);
					pPersistStreammInit->Release();
				}
			}
			bOpenProperty = TRUE;
		}
	}
	else
	{
		for (int i=0; i<nSize; i++)
			pUnk[i] = (IUnknown*)arrayUnknown.GetAt(i);
		bOpenProperty = TRUE;
	}

	if (!bOpenProperty)
		return;

	CString strPropertyCaption;
	strPropertyCaption.LoadString(IDS_CAPT_IPM);
	LPCOLESTR lpCaption = T2COLE((LPTSTR)(LPCTSTR)strPropertyCaption);

	ISpecifyPropertyPagesPtr pSpecifyPropertyPages = pUnk[0];
	CAUUID pages;
	hr = pSpecifyPropertyPages->GetPages( &pages );
	if(FAILED(hr ))
		return;

	PushHelpId ((UINT)0/*IDD_IPM_FONTSETUP*/); // use ID = 0 to disable F1 key
	theApp.PropertyChange(FALSE);
	hr = OleCreatePropertyFrame(
		m_hWnd,
		10,
		10,
		lpCaption,
		(nSize==0)? 1: nSize,
		(IUnknown**) pUnk,
		pages.cElems,
		pages.pElems,
		0, 
		0L,
		NULL );
	PopHelpId();

	CoTaskMemFree( (void*) pages.pElems );
	if (theApp.IsPropertyChange())
	{
		//
		// Save to Global:
		IPersistStreamInit* pPersistStreammInit = NULL;
		hr = pUnk[0]->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			IStream* pStream = theApp.m_ipmStreamFile.Detach();
			pStream->Release();
			if (theApp.m_ipmStreamFile.CreateMemoryStream())
			{
				pStream = theApp.m_ipmStreamFile.GetStream();
				hr = pPersistStreammInit->Save(pStream, TRUE);
				pPersistStreammInit->Release();
			}
		}
		if (theApp.IsSavePropertyAsDefault())
		{
			BOOL bSav = SaveStreamInit(pUnk[0], _T("Ipm"));
			if (!bSav)
				AfxMessageBox (IDS_FAILED_2_SAVESTORAGE);
		}
	}
	if (nSize == 0)
		ctrl.DestroyWindow();
}

void CxDlgPreferences::OnSettingGeneralDisplay()
{
	CxDlgGeneralDisplaySetting dlg;
	dlg.DoModal();
}

void CxDlgPreferences::OnSettingClose()
{
	CDialog::OnOK();
	CaGeneralDisplaySetting* pGD = theApp.GetGeneralDisplaySetting();
	if (pGD)
	{
		pGD->m_nGeneralDisplayView = m_nViewMode;
		pGD->m_nSortColumn = m_nSortColumn;
		pGD->m_bAsc = m_bAsc;
		pGD->Save();
	}
}

void CxDlgPreferences::OnClose() 
{
	OnSettingClose();
	CDialog::OnClose();
}

void CxDlgPreferences::OnOK()
{
	OnExecuteItem();
	return;
}


void CxDlgPreferences::InitializeItems()
{
	CaPreferenceItemData* pItem = NULL;
	int  nItem = 0;
	int  nPrefGroup = theApp.GetOneWindowType();
	CMenu* pMenu = GetMenu();

	if (nPrefGroup == TYPEDOC_NONE || nPrefGroup == TYPEDOC_DOM || nPrefGroup== TYPEDOC_MONITOR)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_SESSIONS, _T("Sessions"));
		m_cListCtrl.InsertItem (nItem, _T("Sessions"), IM_SESSION);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_SESSIONS, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE || nPrefGroup == TYPEDOC_DOM)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_DOM, _T("DOM"));
		m_cListCtrl.InsertItem (nItem, _T("DOM"), IM_DOM);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_DOM, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE || nPrefGroup == TYPEDOC_SQLACT)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_SQLTEST, _T("SQL Test"));
		m_cListCtrl.InsertItem (nItem, _T("SQL Test"), IM_SQLTEST);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_SQLTEST, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE || nPrefGroup == TYPEDOC_DOM || nPrefGroup == TYPEDOC_MONITOR)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_REFRESH, _T("Refresh"));
		m_cListCtrl.InsertItem (nItem, _T("Refresh"), IM_REFRESH);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_REFRESH, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_NODES, _T("Nodes"));
		m_cListCtrl.InsertItem (nItem, _T("Nodes"), IM_NODE);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_NODES, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE || nPrefGroup == TYPEDOC_MONITOR)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_MONITOR, _T("Monitor"));
		m_cListCtrl.InsertItem (nItem, _T("Monitor"), IM_MONITOR);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_MONITOR, MF_BYCOMMAND);
	}

	if (nPrefGroup == TYPEDOC_NONE)
	{
		pItem = new CaPreferenceItemData (ID_SETTINGS_GENERALDISPLAY, _T("General Display"));
		m_cListCtrl.InsertItem (nItem, _T("General Display"), IM_DISPLAY);
		m_cListCtrl.SetItemData(nItem, (DWORD)pItem);
		nItem++;
	}
	else
	if (pMenu)
	{
		pMenu->RemoveMenu (ID_SETTINGS_GENERALDISPLAY, MF_BYCOMMAND);
	}

	for (int i = 0; i<nItem; i++)
	{
		pItem = (CaPreferenceItemData*)m_cListCtrl.GetItemData (i);
		if (pItem)
		{
			m_cListCtrl.SetItemText (i, 1, pItem->m_strDescription);
		}
	}
}


void CxDlgPreferences::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CaPreferenceItemData* pItem = NULL;

	CString strDescription;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		pItem = (CaPreferenceItemData*)m_cListCtrl.GetItemData (pNMListView->iItem);
		m_strDescription = pItem? pItem->m_strDescription: CString(_T(""));
		UpdateData (FALSE);
	}
	
	*pResult = 0;
}


void CxDlgPreferences::OnDisplayBigIcon()
{
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->CheckMenuRadioItem (ID_DISPLAY_BIGICON, ID_DISPLAY_LIST, ID_DISPLAY_BIGICON, MF_BYCOMMAND);
	ChangeViewMode (DISPLAY_BIGICON);
}

void CxDlgPreferences::OnDisplaySmallIcon()
{
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->CheckMenuRadioItem (ID_DISPLAY_BIGICON, ID_DISPLAY_LIST, ID_DISPLAY_SMALLICON, MF_BYCOMMAND);
	ChangeViewMode (DISPLAY_SMALLICON);
}

void CxDlgPreferences::OnDisplayReport()
{
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->CheckMenuRadioItem (ID_DISPLAY_BIGICON, ID_DISPLAY_LIST, ID_DISPLAY_REPORT, MF_BYCOMMAND);
	ChangeViewMode (DISPLAY_REPORT);
}

void CxDlgPreferences::OnDisplayList()
{
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->CheckMenuRadioItem (ID_DISPLAY_BIGICON, ID_DISPLAY_LIST, ID_DISPLAY_LIST, MF_BYCOMMAND);
	ChangeViewMode (DISPLAY_LIST);
}

void CxDlgPreferences::ChangeViewMode (int nMode)
{
	long lStyle, lStyleOld;
	m_nViewMode = nMode;
	switch (nMode)
	{
	case DISPLAY_BIGICON:
		lStyle = LVS_ICON;
		break;
	case DISPLAY_SMALLICON:
		lStyle = LVS_SMALLICON;
		break;
	case DISPLAY_REPORT:
		lStyle = LVS_REPORT;
		break;
	case DISPLAY_LIST:
		lStyle = LVS_LIST;
		break;
	default:
		lStyle = LVS_ICON;
		m_nViewMode = DISPLAY_BIGICON;
		break;
	}

	lStyleOld = GetWindowLong(m_cListCtrl.m_hWnd, GWL_STYLE);
	lStyleOld &= ~(LVS_TYPEMASK);  // turn off all the style (view mode) bits
	lStyleOld |= lStyle;           // Set the new Style for the control
	SetWindowLong(m_cListCtrl.m_hWnd, GWL_STYLE, lStyleOld);  // done!
}

void CxDlgPreferences::OnSort(int nSubItem)
{
	DISPLAYSORT data;
	
	data.bAsc = m_bAsc;
	data.iSubItem = nSubItem;
	m_cListCtrl.SortItems(CompareFunc, (LPARAM)&data);
	m_nSortColumn = nSubItem;
}


void CxDlgPreferences::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
}


void CxDlgPreferences::OnPropertyChange() 
{
	theApp.PropertyChange(TRUE);
}
