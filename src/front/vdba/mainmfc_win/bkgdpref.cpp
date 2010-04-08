/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/* 
**    Source   : bkgdpref.cpp, Implementation File                    
**                                                                    
**                                                                    
**    Project  : INGRES II / VDBA                                     
**    Author   : UK Sotheavut                                         
**                                                                    
**                                                                    
**    Purpose  : Dialog for Preference Settings for Background Refresh
**
** History:
**
** 25-Jan-2000 (uk$so01)
**    (Bug #100104):
**    Show only the IPM's objects when running on context for IPM.
**    Show only the DOM's object when running on context for DOM.
** 04-Apr-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**    Hide the checkbox "Save as Default".
** 09-Apr-2003 (noifr01)
**    (sir 107523) upon cleanup of refresh features for managing sequences,
**    removed obsolete or redundant definitions
**/


// Be careful !!! the member 'bSync4SameType' of struct FREQUENCY is not used.
// Instead, the global variable 'RefreshSyncAmongObject' is used.

#include "stdafx.h"
#include "mainmfc.h"
#include "bkgdpref.h"

extern "C"
{
#include "resource.h"
#include "dlgres.h"
#include "msghandl.h"
#include "settings.h"
#include "main.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Local header functions:
static int  GetPos (LPCTSTR lpszString);
static int Unit2Int(LPCTSTR lpszszChar);


/////////////////////////////////////////////////////////////////////////////
// CxDlgPreferencesBackgroundRefresh dialog


CxDlgPreferencesBackgroundRefresh::CxDlgPreferencesBackgroundRefresh(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgPreferencesBackgroundRefresh::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgPreferencesBackgroundRefresh)
	m_bSaveAsDefault = FALSE;
	m_bSynchronizeAmongObjects = FALSE;
	//}}AFX_DATA_INIT
	m_bSaveAsDefault = SettingLoadFlagSaveAsDefault();
	m_bSynchronizeAmongObjects = SettingLoadFlagRefreshSyncAmongObject();
}


void CxDlgPreferencesBackgroundRefresh::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgPreferencesBackgroundRefresh)
	DDX_Control(pDX, IDC_MFC_CHECK2, m_cCheckRefreshOnLoad);
	DDX_Control(pDX, IDC_MFC_CHECK1, m_cCheckSychronizeParent);
	DDX_Control(pDX, IDC_MFC_SPIN1, m_cSpin1);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditFrequency);
	DDX_Control(pDX, IDC_MFC_COMBO2, m_cComboUnit);
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cComboObjectType);
	DDX_Check(pDX, IDC_MFC_CHECK4, m_bSaveAsDefault);
	DDX_Check(pDX, IDC_MFC_CHECK5, m_bSynchronizeAmongObjects);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgPreferencesBackgroundRefresh, CDialog)
	//{{AFX_MSG_MAP(CxDlgPreferencesBackgroundRefresh)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_MFC_COMBO1, OnSelchangeComboObjectType)
	ON_CBN_SELCHANGE(IDC_MFC_COMBO2, OnSelchangeComboUnit)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT1, OnKillfocusEditFrequency)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnButtonLoadDefault)
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeEditFrequency)
	ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckSynchonizeParent)
	ON_BN_CLICKED(IDC_MFC_CHECK2, OnCheckRefreshOnLoad)
	ON_CBN_KILLFOCUS(IDC_MFC_COMBO2, OnKillfocusComboUnit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgPreferencesBackgroundRefresh message handlers

void CxDlgPreferencesBackgroundRefresh::OnOK() 
{
	CDialog::OnOK();
	FREQUENCY* pCur = NULL;
	int   i, nCount = m_cComboObjectType.GetCount();
	int nMessage = REFRESH_NONE;
	int nID = GetCheckedRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO3);
	switch (nID)
	{
	case IDC_MFC_RADIO1:
		nMessage = REFRESH_WINDOW_POPUP;
		break;
	case IDC_MFC_RADIO2:
		nMessage = REFRESH_STATUSBAR;
		break;
	case IDC_MFC_RADIO3:
		nMessage = REFRESH_NONE;
		break;
	}
	RefreshMessageOption    = nMessage;
	RefreshSyncAmongObject  = m_bSynchronizeAmongObjects;

	ASSERT (nCount == (GetMaxRefreshObjecttypes() +1));
	for (i = 1; i < nCount; i++)
	{
		pCur = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
		if (!pCur)
			continue;
		memcpy (&(FreqSettings[RefObjTypePosInArray(i-1)]), pCur, sizeof (FREQUENCY));
	}

	if (theApp.IsSavePropertyAsDefault())
		SaveAsDefaults();
}

BOOL CxDlgPreferencesBackgroundRefresh::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int nContext = theApp.GetOneWindowType();
	switch (nContext)
	{
	case TYPEDOC_DOM:
		if (m_strAllObject.LoadString (IDS_ALL_DOMOBJECT))
			m_strAllObject = _T("All DOM Objects");
		break;
	case TYPEDOC_MONITOR:
		if (m_strAllObject.LoadString (IDS_ALL_IPMOBJECT))
			m_strAllObject = _T("All Monitor Objects");
		break;
	default:
		if (m_strAllObject.LoadString (IDS_I_ALLOBJECT))
			m_strAllObject = _T("All Objects");
		break;
	}
	m_cEditFrequency.LimitText (4);
	m_cSpin1.SetRange (1, 9999);
	InitUnit ();

	FillObjectType();
	FillUnit();

	m_cComboObjectType.SetCurSel (0);
	m_cComboUnit.SetCurSel (0);
	ShowCharacteristics (0);

	SetDefaultMessageOption ();
	PushHelpId (IDD_REFRESHSET);

	if (nContext == TYPEDOC_MONITOR)
		m_cComboObjectType.EnableWindow (FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgPreferencesBackgroundRefresh::OnDestroy() 
{
	FREQUENCY* pObj = NULL;
	int i, nCount = m_cComboObjectType.GetCount();
	for (i=0; i<nCount; i++)
	{
		pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
		if (pObj)
			delete pObj;
	}
	CDialog::OnDestroy();
	PopHelpId ();
}

void CxDlgPreferencesBackgroundRefresh::OnSelchangeComboObjectType() 
{
	int nSel = m_cComboObjectType.GetCurSel();
	ShowCharacteristics (nSel);
}

void CxDlgPreferencesBackgroundRefresh::OnSelchangeComboUnit() 
{
	UpdateFrequencyUnit();
}

void CxDlgPreferencesBackgroundRefresh::OnKillfocusEditFrequency() 
{
	CString strFreq;
	m_cEditFrequency.GetWindowText (strFreq);
	strFreq.TrimLeft();
	strFreq.TrimRight();
	if (strFreq.IsEmpty())
	{
		strFreq = _T("10");
		m_cEditFrequency.SetWindowText (strFreq);
	}
	UpdateFrequencyUnit(TRUE);
}

void CxDlgPreferencesBackgroundRefresh::OnButtonLoadDefault() 
{
	FREQUENCY* pObj = NULL;
	int i, nSel, nCount = m_cComboObjectType.GetCount();
	FREQUENCY fr [MAXREFRESHOBJTYPES];
	LoadBkRefreshDefaultSettingsFromIni (fr);

	for (i=1; i<nCount; i++)
	{
		pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
		if (pObj)
		{
			memcpy (pObj, &(fr[RefObjTypePosInArray(i-1)]), sizeof(FREQUENCY));
		}
	}
	nSel = m_cComboObjectType.GetCurSel();
	ShowCharacteristics (nSel);
	SetDefaultMessageOption ();
}

void CxDlgPreferencesBackgroundRefresh::OnChangeEditFrequency() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
}


void CxDlgPreferencesBackgroundRefresh::FillObjectType ()
{
	int  nIdx;
	FREQUENCY* pObj = NULL;
	CString strMsg = VDBA_MfcResourceString(IDS_E_INITIALIZE_OBJECT);//_T("Cannot Initialize Object Types.");
	InitObjectType ();
	nIdx = m_cComboObjectType.AddString (m_strAllObject);
	if (nIdx == CB_ERR)
	{
		AfxMessageBox (strMsg);
		return;
	}
	pObj = new FREQUENCY;
	memset (pObj, 0, sizeof(FREQUENCY));
	m_cComboObjectType.SetItemData (nIdx, (DWORD)pObj);

	for (int i = 0; i < GetMaxRefreshObjecttypes(); i++)
	{
		nIdx = m_cComboObjectType.AddString ((LPCTSTR)RefreshSettingsObjectTypeString [RefObjTypePosInArray(i)].szString);
		if (nIdx == CB_ERR)
		{
			AfxMessageBox (strMsg);
			return;
		}
		pObj = new FREQUENCY;
		memcpy (pObj, &(FreqSettings[RefObjTypePosInArray(i)]), sizeof (FREQUENCY));
		m_cComboObjectType.SetItemData (nIdx, (DWORD)pObj);
	}
}


void CxDlgPreferencesBackgroundRefresh::FillUnit ()
{
	for (int i =0; i<MAX_UNIT_STRING; i++)
	{
		m_cComboUnit.AddString ((LPCTSTR)UnitString [i].szString);
	}
}


void CxDlgPreferencesBackgroundRefresh::ShowCharacteristics (int nSel)
{
	int nIdx, i;
	CString strComboText;
	CString strFreq;
	ASSERT (nSel != -1);
	if (nSel == CB_ERR)
		return;
	if (m_cComboObjectType.GetCount() < 2)
		return;

	FREQUENCY* pObj    = (FREQUENCY*)m_cComboObjectType.GetItemData (1);
	FREQUENCY* pObjAll = (FREQUENCY*)m_cComboObjectType.GetItemData (0);
	ASSERT (pObj && pObjAll);
	if (!(pObj || pObjAll))
		return;
	m_cComboObjectType.GetLBText (nSel, strComboText);
	if (strComboText.CompareNoCase (m_strAllObject) == 0)
	{
		//
		// Show characteristics of 'All objects'
		if (IsEqualAllObjectType (0))
		{
			//
			// Only the frequency and unit are affected !
			if (pObj && pObjAll)
			{
				nIdx = CB_ERR;
				pObjAll->count = pObj->count;
				pObjAll->unit  = pObj->unit;
				for (i = 0; i < MAX_UNIT_STRING; i++)
				{
					if (UnitString [i].number == pObj->unit)
					{
						nIdx = m_cComboUnit.FindStringExact (-1, (LPCTSTR)UnitString [i].szString);
						break;
					}
				}
				m_cComboUnit.SetCurSel (nIdx);
				strFreq.Format (_T("%d"), pObj->count);
				m_cEditFrequency.SetWindowText (strFreq);
			}
		}
		else
		{
			m_cComboUnit.SetCurSel (-1);
			m_cEditFrequency.SetWindowText (_T(""));
		}

		//
		// Tri-states check boxes:
		m_cCheckSychronizeParent.SetButtonStyle(BS_AUTO3STATE);
		m_cCheckRefreshOnLoad.SetButtonStyle(BS_AUTO3STATE);
		m_cCheckSychronizeParent.SetCheck (GetSynchronizeParent());
		m_cCheckRefreshOnLoad.SetCheck(GetRefreshOnLoad());
	}
	else
	{
		int nPositionType = GetPos (strComboText);
		if (nPositionType != -1 && nPositionType < GetMaxRefreshObjecttypes())
		{
			FREQUENCY* pCur   = (FREQUENCY*)m_cComboObjectType.GetItemData (nSel);
			ASSERT (pCur);
			if (!pCur)
				return;
			//
			// Show the periode
			strFreq.Format (_T("%d"), pCur->count);
			m_cEditFrequency.SetWindowText (strFreq);
			//
			// Show the unit
			nIdx = CB_ERR;
			for (i = 0; i < MAX_UNIT_STRING; i++)
			{
				if (UnitString [i].number == pCur->unit)
				{
					nIdx = m_cComboUnit.FindStringExact (-1, (LPCTSTR)UnitString [i].szString);
					break;
				}
			}
			m_cComboUnit.SetCurSel (nIdx);
			strFreq.Format (_T("%d"), pCur->count);
			m_cEditFrequency.SetWindowText (strFreq);
			//
			// Tri-states check boxes:
			m_cCheckSychronizeParent.SetButtonStyle(BS_AUTOCHECKBOX);
			m_cCheckRefreshOnLoad.SetButtonStyle(BS_AUTOCHECKBOX);
			m_cCheckSychronizeParent.SetCheck ((int)pCur->bSyncOnParent);
			m_cCheckRefreshOnLoad.SetCheck ((int)pCur->bOnLoad);
		}
	}
}


void CxDlgPreferencesBackgroundRefresh::SetDefaultMessageOption ()
{
	int nID = IDC_MFC_RADIO3;
	switch (RefreshMessageOption)
	{
	case REFRESH_WINDOW_POPUP:
		nID = IDC_MFC_RADIO1;
		break;
	case REFRESH_STATUSBAR:
		nID = IDC_MFC_RADIO2;
		break;
	case REFRESH_NONE:
		nID = IDC_MFC_RADIO3;
		break;
	default:
		nID = IDC_MFC_RADIO3;
		break;
	}
	CheckRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO3, nID);
	m_bSynchronizeAmongObjects = RefreshSyncAmongObject;
	UpdateData (FALSE);
}


void CxDlgPreferencesBackgroundRefresh::SaveAsDefaults ()
{
	TCHAR freq [10];
	TCHAR unit [10];
	TCHAR load_opt    [3];
	TCHAR parent_opt  [3];
	TCHAR privateString [30];
	CString strFile;
	FREQUENCY* pCur = NULL;
	int   i, nCount = m_cComboObjectType.GetCount();

	ASSERT (nCount == (GetMaxRefreshObjecttypes() +1));
	if (strFile.LoadString (IDS_INIFILENAME) == 0)
		strFile = _T("vdba.ini");
	for (i = 1; i < nCount; i++)
	{
		pCur = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
		if (!pCur)
		{
			CString strMsg = VDBA_MfcResourceString(IDS_E_SETTING_DEFAULT);//_T("Cannot Save Setting as Defaults.");
			AfxMessageBox (strMsg);
			return;
		}
		wsprintf (freq,         _T("%d"), pCur->count); 
		wsprintf (unit,         _T("%d"), pCur->unit );
		wsprintf (load_opt    , _T("%d"), pCur->bOnLoad);
		wsprintf (parent_opt  , _T("%d"), pCur->bSyncOnParent);
		
		wsprintf (privateString, "%s %s %s %s", freq, unit, load_opt, parent_opt);
		
		WritePrivateProfileString (
			"BACKGROUND REFRESH SETTINGS",
			RefreshSettingsObjectTypeString [RefObjTypePosInArray(i-1)].szString,
			privateString,
			strFile);
	}

	int nMessage = REFRESH_NONE;
	int nID = GetCheckedRadioButton (IDC_MFC_RADIO1, IDC_MFC_RADIO3);
	switch (nID)
	{
	case IDC_MFC_RADIO1:
		nMessage = REFRESH_WINDOW_POPUP;
		break;
	case IDC_MFC_RADIO2:
		nMessage = REFRESH_STATUSBAR;
		break;
	case IDC_MFC_RADIO3:
		nMessage = REFRESH_NONE;
		break;
	}
	SettingSaveFlagSaveAsDefault(m_bSaveAsDefault);
	SettingSaveFlagRefreshSyncAmongObject(m_bSynchronizeAmongObjects);
	SettingSaveFlagRefreshMessageOption(nMessage);
}


void CxDlgPreferencesBackgroundRefresh::UpdateFrequencyUnit(BOOL bFrequencyKillFocus)
{
	FREQUENCY* pObj = NULL;
	int i, nSel, nUnit;
	CString strFreq;
	CString strUnit;
	CString strComboText;
	if (!IsWindow (m_cComboObjectType.m_hWnd) || !IsWindow (m_cComboUnit.m_hWnd) || !IsWindow (m_cEditFrequency.m_hWnd))
		return;
	nSel = m_cComboObjectType.GetCurSel();
	ASSERT (nSel != -1);
	if (nSel == CB_ERR)
		return;
	m_cComboObjectType.GetLBText (nSel, strComboText);
	nUnit = m_cComboUnit.GetCurSel();
	if (nUnit == CB_ERR)
		return;

	m_cComboUnit.GetLBText(nUnit, strUnit);
	//
	// Update Unit:
	int nUnitType = Unit2Int (strUnit);
	//
	// Update Frequency:
	m_cEditFrequency.GetWindowText (strFreq);
	strFreq.TrimLeft();
	strFreq.TrimRight();
	int nFreg = atoi (strFreq);
	if (nFreg == 0)
		nFreg = 1;
	if (nUnitType == FREQ_UNIT_SECONDS && nFreg < 10)
	{
		nFreg = 10;
		if (bFrequencyKillFocus)
		{
			strFreq.Format (_T("%d"), nFreg);
			m_cEditFrequency.SetWindowText (strFreq);
		}
		else
			return;
	}

	if (strComboText.CompareNoCase (m_strAllObject) == 0)
	{
		int nCount = m_cComboObjectType.GetCount();
		for (i=0; i<nCount; i++)
		{
			pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
			if (!pObj)
				continue;
			pObj->count = nFreg;
			pObj->unit  = nUnitType;
		}
	}
	else
	{
		pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (nSel);
		if (!pObj)
			return;
		pObj->count = nFreg;
		pObj->unit  = nUnitType;
	}
}

//
// nCheckBox = 0: Synchronize with Parent.
// nCheckBox = 1: Refresh on Load
void CxDlgPreferencesBackgroundRefresh::UpdateCharacteristics(int nCheckBox)
{
	FREQUENCY* pObj = NULL;
	int nSel, i;
	CString strComboText;
	
	if (!IsWindow (m_cComboObjectType.m_hWnd) || !IsWindow (m_cComboUnit.m_hWnd) || !IsWindow (m_cEditFrequency.m_hWnd))
		return;

	nSel = m_cComboObjectType.GetCurSel();
	ASSERT (nSel != -1);
	if (nSel == CB_ERR)
		return;
	m_cComboObjectType.GetLBText (nSel, strComboText);

	//
	// Tri-states check boxes:
	int nCheckSycParent = m_cCheckSychronizeParent.GetCheck();
	int nCheckOnLoad    = m_cCheckRefreshOnLoad.GetCheck();

	if (strComboText.CompareNoCase (m_strAllObject) == 0)
	{
		int nCount = m_cComboObjectType.GetCount();
		for (i=0; i<nCount; i++)
		{
			pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
			if (!pObj)
				continue;
			if (nCheckBox == 0 && nCheckSycParent != 2)
			{
				pObj->bSyncOnParent = (BOOL)nCheckSycParent;
			}

			if (nCheckBox == 1 && nCheckOnLoad != 2)
			{
				pObj->bOnLoad = (BOOL)nCheckOnLoad;
			}
		}
	}
	else
	{
		pObj = (FREQUENCY*)m_cComboObjectType.GetItemData (nSel);
		if (!pObj)
			return;

		if (nCheckBox == 0 && nCheckSycParent != 2)
		{
			pObj->bSyncOnParent = (BOOL)nCheckSycParent;
		}

		if (nCheckBox == 1 && nCheckOnLoad != 2)
		{
			pObj->bOnLoad = (BOOL)nCheckOnLoad;
		}
	}
}


//
// nTest = 0: Frequency and Unit
// nTest = 1: Synchronize on Parent
// nTest = 2: Refresh on Load
BOOL CxDlgPreferencesBackgroundRefresh::IsEqualAllObjectType( int nTest)
{
	FREQUENCY* pFirst = NULL;
	FREQUENCY* pCur   = NULL;
	int   i, nCount = m_cComboObjectType.GetCount();

	ASSERT (nCount >= 2 );
	ASSERT (nCount == (GetMaxRefreshObjecttypes() +1));
	if (nCount < 2)
		return FALSE;
	pFirst = (FREQUENCY*)m_cComboObjectType.GetItemData (1);
	if (!pFirst)
	{
		//
		// Data corrupted:
		ASSERT (FALSE);
		return FALSE;
	}

	for (i = 2; i < nCount; i++)
	{
		pCur = (FREQUENCY*)m_cComboObjectType.GetItemData (i);
		if (!pCur)
		{
			//
			// Data corrupted:
			ASSERT (FALSE);
			return FALSE;
		}
		switch (nTest)
		{
		case 0:
			if (pFirst->count != pCur->count || pFirst->unit != pCur->unit)
				return FALSE;
			break;
		case 1:
			if (pFirst->bSyncOnParent != pCur->bSyncOnParent)
				return FALSE;
			break;
		case 2:
			if (pFirst->bOnLoad != pCur->bOnLoad)
				return FALSE;
			break;
		default:
			return FALSE;
		}
	}
	return TRUE;
}

int CxDlgPreferencesBackgroundRefresh::GetSynchronizeParent()
{
	FREQUENCY* pFirst = NULL;
	int nCount = m_cComboObjectType.GetCount();

	ASSERT (nCount >= 2 );
	if (nCount < 2)
		return 2;
	pFirst = (FREQUENCY*)m_cComboObjectType.GetItemData (1);
	if (!pFirst)
	{
		//
		// Data corrupted:
		ASSERT (FALSE);
		return 2;
	}
	return IsEqualAllObjectType (1)? (int)pFirst->bSyncOnParent: 2;
}

int CxDlgPreferencesBackgroundRefresh::GetRefreshOnLoad()
{
	FREQUENCY* pFirst = NULL;
	int nCount = m_cComboObjectType.GetCount();

	ASSERT (nCount >= 2 );
	if (nCount < 2)
		return 2;
	pFirst = (FREQUENCY*)m_cComboObjectType.GetItemData (1);
	if (!pFirst)
	{
		//
		// Data corrupted:
		ASSERT (FALSE);
		return 2;
	}
	return IsEqualAllObjectType (2)? (int)pFirst->bOnLoad: 2;
}


void CxDlgPreferencesBackgroundRefresh::OnCheckSynchonizeParent() 
{
	if (m_cCheckSychronizeParent.GetCheck() == 2)
		m_cCheckSychronizeParent.SetCheck (1);
	m_cCheckSychronizeParent.SetButtonStyle(BS_AUTOCHECKBOX);
	UpdateCharacteristics(0);
}

void CxDlgPreferencesBackgroundRefresh::OnCheckRefreshOnLoad() 
{
	if (m_cCheckRefreshOnLoad.GetCheck() == 2)
		m_cCheckRefreshOnLoad.SetCheck (1);
	m_cCheckRefreshOnLoad.SetButtonStyle(BS_AUTOCHECKBOX);
	UpdateCharacteristics(1);
}



static int GetPos (LPCTSTR lpszString)
{
	for (int i = 0; i < GetMaxRefreshObjecttypes(); i++)
	{
		if (lstrcmpi (lpszString, RefreshSettingsObjectTypeString [RefObjTypePosInArray(i)].szString) == 0)
			return RefreshSettingsObjectTypeString [RefObjTypePosInArray(i)].number;
	}
	//
	// Should never happen:
	ASSERT (FALSE);
	return -1;
}

static int Unit2Int (LPCTSTR lpszszChar)
{
	for (int i = 0; i < MAX_UNIT_STRING; i++)
	{
		if (lstrcmpi (UnitString [i].szString, lpszszChar) == 0)
			return UnitString [i].number;
	}
	//
	// Should never happen:
	ASSERT (FALSE);
	return FREQ_UNIT_HOURS;
}


void CxDlgPreferencesBackgroundRefresh::OnKillfocusComboUnit() 
{
	OnKillfocusEditFrequency();
}
