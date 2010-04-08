/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dtagsett.h, Header File.
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data defintion for General Display Settings. 
**
** History:
**
** 22-Dec-1998 (uk$so01)
**    Created
** 29-Jun-2000 (uk$so01)
**    SIR #105160, created for adding custom message box. 
**
*/

#if !defined (DTAGSETT_HEADER)
#define DTAGSETT_HEADER
#include "msgcateg.h"

#define EVMAX_NOLIMIT   0
#define EVMAX_COUNT     1
#define EVMAX_MEMORY    2
#define EVMAX_SINCEDAY  3
#define EVMAX_SINCENAME 4

//
// General Setting:
class CaGeneralSetting: public CObject
{
public:
	CaGeneralSetting();
	CaGeneralSetting(const CaGeneralSetting& c);
	CaGeneralSetting operator = (const CaGeneralSetting& c);
	BOOL operator ==(const CaGeneralSetting& c);

	virtual ~CaGeneralSetting();

	BOOL Save();
	BOOL Load();
	BOOL LoadDefault();
	void NotifyChanges();
	int  GetEventMaxType(){return m_nEventMaxType;}
	void SetEventMaxType(int nIdRadioType);

	UINT GetMaxCount() {return m_nMaxEvent;}   // Total number of events
	UINT GetMaxSize()  {return m_nMaxMemUnit;} // Total memory (in MB) of events
	UINT GetMaxDay()   {return m_nMaxDay;}

	CTime GetLastSaveTime();
public:
	int  m_nEventMaxType; // EVMAX_NOLIMIT, ...,
	UINT m_nMaxEvent;     // Count of event        (m_nEventMaxType = EVMAX_COUNT)
	UINT m_nMaxMemUnit;   // Count of memory       (m_nEventMaxType = EVMAX_MEMORY)
	UINT m_nMaxDay;       // All events since days (m_nEventMaxType = EVMAX_SINCEDAY)

	BOOL m_bSendingMail;  // Enable sending mail automatically
	BOOL m_bMessageBox;   // Show message box when event arrives
	BOOL m_bSpecialIcon;  // Use special icons to alert user of events
	BOOL m_bSoundBeep;    // Use a beep to alert user.
	CString m_strTimeSet; // Time when the data is writen to the .INI file.
	BOOL m_bShowThisMsg1; // Show this messagebox later (Service Account Confirmation)
	CaMessageManager m_messageManager;

	//
	// Ingres State:
	BOOL m_bAsService;    // Ingres is running as service;
protected:
	BOOL TestCompatibleFile();

	void Copy (const CaGeneralSetting& c);
};



#endif // DTAGSETT_HEADER
