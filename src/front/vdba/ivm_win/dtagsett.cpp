/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dtagsett.cpp, Implementation File.
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data defintion for General Display Settings. 
**
** History:
**
** 22-Dec-1998 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 29-Jun-2000 (uk$so01)
**    SIR #105160, created for adding custom message box. 
**
**/

#include "stdafx.h"
#include "ivm.h"
#include "dtagsett.h"
#include "ivmdml.h"
#include "ivmsgdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define GENERAL_EVENT_ID                 _T("IVMVERSION ID")
#define GENERAL_EVENT_IDVAL              _T("Current ID")
#define GENERAL_EVENT_IDVAL_DEF          _T("{5D335950-F340-11d2-A2E1-00609725DDAF}")

#define GENERAL_EVENT_LAST_SAVE          _T("DATE OF THE LAST SAVE")
#define GENERAL_EVENT_LAST_SAVE_VAL      _T("Date")

//
// Display Event:
#define GENERAL_EVENT_DISPLAY            _T("LOGGED EVENTS DISPLAY")
#define GENERAL_EVENT_MAXTYPE            _T("Max Event.Type")
#define GENERAL_EVENT_COUNT              _T("Max Event.Count")
#define GENERAL_EVENT_MEMORY             _T("Max Event.Memory")
#define GENERAL_EVENT_SINCE_DAYS         _T("Max Event.Since Day")

#define GENERAL_EVENT_MAXTYPE_DEF         EVMAX_COUNT
#define GENERAL_EVENT_COUNT_DEF           50000
#define GENERAL_EVENT_MEMORY_DEF          2
#define GENERAL_EVENT_SINCE_DAYS_DEF      1

//
// Notification Event:
#define GENERAL_EVENT_NOTIFY             _T("LOGGED EVENTS NOTIFY")
#define GENERAL_EVENT_SENDINGMAIL        _T("Notify.Send mail")
#define GENERAL_EVENT_MESSAGEBOX         _T("Notify.Message Box")
#define GENERAL_EVENT_SPECIALICON        _T("Notify.Special Icon")
#define GENERAL_EVENT_SOUNDBEEP          _T("Notify.Sound Beep")
#define GENERAL_EVENT_SENDINGMAIL_DEF     FALSE
#define GENERAL_EVENT_MESSAGEBOX_DEF      FALSE
#define GENERAL_EVENT_SPECIALICON_DEF     TRUE
#define GENERAL_EVENT_SOUNDBEEP_DEF       TRUE

//
// Ingres State: (Service Mode)
#define GENERAL_INGRES_STATE             _T("INGRES STATE")
#define GENERAL_INGRES_SERVICE_MODE      _T("Ingres.Service Mode")
#define GENERAL_INGRES_SERVICE_MODE_DEF  TRUE
#define GENERAL_INGRES_SERVICE_MSGBOX    _T("Ingres.Service Confirm Account")
#define GENERAL_INGRES_SERVICE_MSGBOX_ACCOUNT_DEF TRUE

CaGeneralSetting::CaGeneralSetting()
{
	LoadDefault();
}

CaGeneralSetting::~CaGeneralSetting()
{
}

CaGeneralSetting::CaGeneralSetting(const CaGeneralSetting& c)
{
	Copy (c);
}

CaGeneralSetting CaGeneralSetting::operator = (const CaGeneralSetting& c)
{
	Copy (c);
	return *this;
}

BOOL CaGeneralSetting::operator ==(const CaGeneralSetting& c)
{
	if (m_nEventMaxType != c.m_nEventMaxType)
		return FALSE;
	if (m_nMaxEvent != c.m_nMaxEvent)
		return FALSE;
	if (m_nMaxMemUnit != c.m_nMaxMemUnit)
		return FALSE;
	if (m_nMaxDay != c.m_nMaxDay)
		return FALSE;

	if (m_bSendingMail != c.m_bSendingMail)
		return FALSE;
	if (m_bMessageBox != c.m_bMessageBox)
		return FALSE;
	if (m_bSpecialIcon != c.m_bSpecialIcon)
		return FALSE;
	if (m_bSoundBeep != c.m_bSoundBeep)
		return FALSE;

	return TRUE;
}

void CaGeneralSetting::Copy (const CaGeneralSetting& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	m_nEventMaxType = c.m_nEventMaxType;
	m_nMaxEvent = c.m_nMaxEvent;
	m_nMaxMemUnit = c.m_nMaxMemUnit;
	m_nMaxDay = c.m_nMaxDay;
	m_bSendingMail = c.m_bSendingMail;
	m_bMessageBox = c.m_bMessageBox;
	m_bSpecialIcon = c.m_bSpecialIcon;
	m_bSoundBeep = c.m_bSoundBeep;

	m_bAsService = c.m_bAsService;
	m_strTimeSet = c.m_strTimeSet;

}


BOOL CaGeneralSetting::Save()
{
	try
	{
		CString strAch;
		CString strFile;
		if (strFile.LoadString (IDS_INIFILENAME) == 0)
			return FALSE;

		//
		//[IVM VERSION ID]
		//****************
		strAch.Format (_T("%s"), GENERAL_EVENT_IDVAL_DEF);
		WritePrivateProfileString(GENERAL_EVENT_ID, GENERAL_EVENT_IDVAL, strAch, strFile);

		//
		//[DATE OF THE LAST SAVE]
		//***********************
		CTime t = CTime::GetCurrentTime();
		m_strTimeSet = t.Format (theApp.m_strTimeFormat);
		WritePrivateProfileString(GENERAL_EVENT_LAST_SAVE, GENERAL_EVENT_LAST_SAVE_VAL, m_strTimeSet, strFile);

		//
		//[DISPLAY EVENTS]
		//****************
		//
		// Max Event Type:
		strAch.Format (_T("%d"), m_nEventMaxType);
		WritePrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_MAXTYPE, strAch, strFile);
	
		// Event Count:
		strAch.Format (_T("%d"), m_nMaxEvent);
		WritePrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_COUNT, strAch, strFile);
		//
		// Event Memory:
		strAch.Format (_T("%d"), m_nMaxMemUnit);
		WritePrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_MEMORY, strAch, strFile);
		//
		// Event Since days:
		strAch.Format (_T("%d"), m_nMaxDay);
		WritePrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_SINCE_DAYS, strAch, strFile);

		//
		//[EVENT NOTIFICATION]
		//********************
		//
		// Send mail:
		strAch.Format (_T("%d"), m_bSendingMail);
		WritePrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SENDINGMAIL, strAch, strFile);
		//
		// Message Box:
		strAch.Format (_T("%d"), m_bMessageBox);
		WritePrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_MESSAGEBOX, strAch, strFile);
		//
		// Special Icon:
		strAch.Format (_T("%d"), m_bSpecialIcon);
		WritePrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SPECIALICON, strAch, strFile);
		//
		// Sound Beep:
		strAch.Format (_T("%d"), m_bSoundBeep);
		WritePrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SOUNDBEEP, strAch, strFile);

		//
		// Ingres Service Mode:
		strAch.Format (_T("%d"), m_bAsService);
		WritePrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_SERVICE_MODE, strAch, strFile);
		//
		// Ingres Service Confirm account Msgbox:
		strAch.Format (_T("%d"), m_bShowThisMsg1);
		WritePrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_SERVICE_MSGBOX, strAch, strFile);
	}
	catch (CArchiveException* e)
	{
		IVM_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (_T("System error (CaGeneralSetting::Save): \nCannot save the event category data"));
		return FALSE;
	}

	return TRUE;
}

BOOL CaGeneralSetting::Load()
{
	try
	{
		TCHAR   tchszAch [32];
		TCHAR   tchszDef [32];
		CString strFile;
		if (strFile.LoadString (IDS_INIFILENAME) == 0)
			return FALSE;
		//
		//[IVM VERSION ID]
		//****************
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_IDVAL_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_ID, GENERAL_EVENT_IDVAL, tchszDef, tchszAch, sizeof(tchszAch), strFile))
		{
			//
			// Nothing to do.
		}

		//
		//[DATE OF THE LAST SAVE]
		//***********************
		wsprintf (tchszDef, _T("%s"), theApp.m_strStartupTime);
		if (GetPrivateProfileString(GENERAL_EVENT_LAST_SAVE, GENERAL_EVENT_LAST_SAVE_VAL, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_strTimeSet = tchszAch;
		else
			m_strTimeSet = theApp.m_strStartupTime;

		//
		//[DISPLAY EVENTS]
		//****************
		//
		// Max Event Type:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_MAXTYPE_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_MAXTYPE, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_nEventMaxType = _ttol (tchszAch);
		else
			m_nEventMaxType = GENERAL_EVENT_MAXTYPE_DEF;
		//
		// Event Count:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_COUNT_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_COUNT, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_nMaxEvent = (long)_ttol (tchszAch);
		else
			m_nMaxEvent = GENERAL_EVENT_COUNT_DEF;
		//
		// Event Memory:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_MEMORY_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_MEMORY, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_nMaxMemUnit = (long)_ttol (tchszAch);
		else
			m_nMaxMemUnit = GENERAL_EVENT_MEMORY_DEF;
		//
		// Event Since days:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_SINCE_DAYS_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_DISPLAY, GENERAL_EVENT_SINCE_DAYS, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_nMaxDay = (long)_ttol (tchszAch);
		else
			m_nMaxDay = GENERAL_EVENT_SINCE_DAYS_DEF;

		//
		//[EVENT NOTIFICATION]
		//********************
		//
		// Send mail:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_SENDINGMAIL_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SENDINGMAIL, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bSendingMail = (BOOL)_ttoi (tchszAch);
		else
			m_bSendingMail = GENERAL_EVENT_SENDINGMAIL_DEF;
		//
		// Message Box:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_MESSAGEBOX_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_MESSAGEBOX, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bMessageBox = (BOOL)_ttoi (tchszAch);
		else
			m_bMessageBox = GENERAL_EVENT_MESSAGEBOX_DEF;
		//
		// Special Icon:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_SPECIALICON_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SPECIALICON, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bSpecialIcon = (BOOL)_ttoi (tchszAch);
		else
			m_bSpecialIcon = GENERAL_EVENT_SPECIALICON_DEF;
		//
		// Sound beep:
		wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_SOUNDBEEP_DEF);
		if (GetPrivateProfileString(GENERAL_EVENT_NOTIFY, GENERAL_EVENT_SOUNDBEEP, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bSoundBeep = (BOOL)_ttoi (tchszAch);
		else
			m_bSoundBeep = GENERAL_EVENT_SOUNDBEEP_DEF;

		//
		// Ingres Service Mode:
		wsprintf (tchszDef, _T("%d"), GENERAL_INGRES_SERVICE_MODE_DEF);
		if (GetPrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_SERVICE_MODE, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bAsService = (BOOL)_ttoi (tchszAch);
		else
			m_bAsService = GENERAL_INGRES_SERVICE_MODE_DEF;
		//
		// Ingres Service Confirm account Msgbox:
		wsprintf (tchszDef, _T("%d"), GENERAL_INGRES_SERVICE_MSGBOX_ACCOUNT_DEF);
		if (GetPrivateProfileString(GENERAL_INGRES_STATE, GENERAL_INGRES_SERVICE_MSGBOX, tchszDef, tchszAch, sizeof(tchszAch), strFile))
			m_bShowThisMsg1 = (BOOL)_ttoi (tchszAch);
		else
			m_bShowThisMsg1 = GENERAL_INGRES_SERVICE_MSGBOX_ACCOUNT_DEF;

		IVM_InitializeUnclassifyCategory (theApp.m_setting.m_messageManager);
		CaCategoryDataUserManager usrFolder; // Dummy
		BOOL bStoring = FALSE;
		IVM_SerializeMessageSetting(&(theApp.m_setting.m_messageManager), &usrFolder, bStoring);
	}
	catch (CArchiveException* e)
	{
		IVM_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CaGeneralSetting::Load): \nCannot load event category data"));
		return FALSE;
	}
	return TRUE;
}


BOOL CaGeneralSetting::LoadDefault()
{
	m_nEventMaxType= GENERAL_EVENT_MAXTYPE_DEF;

	m_nMaxEvent    = GENERAL_EVENT_COUNT_DEF;
	m_nMaxMemUnit  = GENERAL_EVENT_MEMORY_DEF;
	m_nMaxDay      = GENERAL_EVENT_SINCE_DAYS_DEF;

	m_bSendingMail = GENERAL_EVENT_SENDINGMAIL_DEF;
	m_bMessageBox  = GENERAL_EVENT_MESSAGEBOX_DEF;
	m_bSpecialIcon = GENERAL_EVENT_SPECIALICON_DEF;

	m_strTimeSet   = _T("");
	m_bShowThisMsg1= GENERAL_INGRES_SERVICE_MSGBOX_ACCOUNT_DEF;
	return TRUE;
}

void CaGeneralSetting::NotifyChanges()
{

}

BOOL CaGeneralSetting::TestCompatibleFile()
{
	TCHAR   tchszAch [32];
	TCHAR   tchszDef [32];
	CString strFile;
	if (strFile.LoadString (IDS_INIFILENAME) == 0)
		return TRUE;
	wsprintf (tchszDef, _T("%d"), GENERAL_EVENT_IDVAL_DEF);
	if (!GetPrivateProfileString(GENERAL_EVENT_ID, GENERAL_EVENT_IDVAL, tchszDef, tchszAch, sizeof(tchszAch), strFile))
	{
		return FALSE;
	}
	return TRUE;
}

void CaGeneralSetting::SetEventMaxType(int nIdRadioType)
{
	switch (nIdRadioType)
	{
	case IDC_RADIO1:
		m_nEventMaxType = EVMAX_MEMORY;
		break;
	case IDC_RADIO2:
		m_nEventMaxType = EVMAX_COUNT;
		break;
	case IDC_RADIO3:
		m_nEventMaxType = EVMAX_SINCEDAY;
		break;
	case IDC_RADIO4:
		m_nEventMaxType = EVMAX_SINCENAME;
		break;
	case IDC_RADIO5:
		m_nEventMaxType = EVMAX_NOLIMIT;
		break;
	default:
		m_nEventMaxType = EVMAX_NOLIMIT;
		break;
	}
}

CTime CaGeneralSetting::GetLastSaveTime()
{
	ASSERT (FALSE);
	return CTime::GetCurrentTime();
}
