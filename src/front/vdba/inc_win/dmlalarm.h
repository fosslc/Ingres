/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlalarm.h: interface for the CaAlarm class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Alarm object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLALARM_HEADER)
#define DMLALARM_HEADER
#include "dmlbase.h"

//
// Object: Alarm
// ************************************************************************************************
class CaAlarm: public CaDBObject
{
	DECLARE_SERIAL (CaAlarm)
public:
	CaAlarm();
	CaAlarm(LPCTSTR lpszName, int nNumber);
	CaAlarm (const CaAlarm& c);
	CaAlarm operator = (const CaAlarm& c);

	void Init();
	virtual ~CaAlarm(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaAlarm* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	void SetSecurityNumber(int nNum){m_nSecurityNumber = nNum;}
	void SetSubjectType(TCHAR tchType){m_tchSubjectType = tchType;}
	void SetSecurityUser(LPTSTR lpszText){m_strSecurityUser = lpszText;}
	void SetDBEvent(LPTSTR lpszName, LPTSTR lpszOwner){m_strDBEvent = lpszName; m_strDBEventOwner = lpszOwner;}
	void SetDetailText(LPCTSTR lpszText){m_strDetailText = lpszText;}
	void AppendDetailText(LPCTSTR lpszText){m_strDetailText+= lpszText;}
	BOOL ParseSecurityAlarmFlags();

	int GetSecurityNumber(){return m_nSecurityNumber;}
	TCHAR GetSubjectType(){return m_tchSubjectType;}
	CString GetSecurityUser();
	CString GetDBEvent(){return m_strDBEvent;}
	CString GetDBEventOwner(){return m_strDBEventOwner;}
	CString GetDetailText(){return m_strDetailText;}
	CStringList& GetListEventAction(){return m_listEventAction;}

	BOOL GetIfSuccess(){return m_bIfSuccess;}
	BOOL GetIfFailure(){return m_bIfFailure;}

protected:
	void Copy (const CaAlarm& c);
	void AlreadyParsed(BOOL bSet){m_bParesed = bSet;}
	void HandleToken(CString& strToken);

	int     m_nSecurityNumber;
	TCHAR   m_tchSubjectType;
	CString m_strSecurityUser;
	CString m_strDBEvent;
	CString m_strDBEventOwner;
	CString m_strDetailText;

	BOOL m_bIfSuccess;
	BOOL m_bIfFailure;
	CStringList m_listEventAction;
private:
	BOOL m_bParesed;
};


#endif // DMLALARM_HEADER