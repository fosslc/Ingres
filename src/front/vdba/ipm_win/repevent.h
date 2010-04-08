/*
/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : repevent.h, Header File
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : Philippe SCHALK
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module.
** History:
**
** xx-May-1997 (Philippe SCHALK)
**    Created.
*/

#if !defined (REPRAISEEVENT_HEADER)
#define REPRAISEEVENT_HEADER

class CaReplicationRaiseDbevent: public CObject
{
	DECLARE_SERIAL (CaReplicationRaiseDbevent)
public:
	CaReplicationRaiseDbevent():m_strAction(_T("")),m_strEvent(_T("")),m_strServerFlag(_T(""))
	{
		m_nServerNumber = -1;
		m_bAskQuestion  = FALSE;
		m_strQuestionText.Empty();
		m_strQuestionCaption.Empty();
	}
	CaReplicationRaiseDbevent(
		LPCTSTR lpszAction,
		LPCTSTR lpszEvent, 
		LPCTSTR lpszServerFlag ,
		int nServerNumber = -1,
		BOOL bAskQuestion = FALSE ,
		LPCTSTR lpszQuestionText = NULL ,
		LPCTSTR lpszQuestionCaption = NULL,int iMax = -1 ,int iMin = -1)
		:m_strAction(lpszAction),m_strEvent(lpszEvent),
			m_strServerFlag(lpszServerFlag),m_strQuestionText(lpszQuestionText),
			m_strQuestionCaption(lpszQuestionCaption)
	{
		m_bAskQuestion = bAskQuestion;
		if (m_bAskQuestion) {
			ASSERT(m_strQuestionText);
			ASSERT(m_strQuestionCaption);
			m_iMaxValue = iMax;
			ASSERT(m_iMaxValue!=-1);
			m_iMinValue = iMin;
			ASSERT(m_iMinValue!=-1);
		}
	}
	CaReplicationRaiseDbevent(
		UINT uiActionStrId,
		LPCTSTR lpszEvent, 
		LPCTSTR lpszServerFlag ,
		int nServerNumber = -1,
		BOOL bAskQuestion = FALSE ,
		LPCTSTR lpszQuestionText = NULL ,
		LPCTSTR lpszQuestionCaption = NULL,int iMax = -1 ,int iMin = -1)
		:m_strEvent(lpszEvent),
			m_strServerFlag(lpszServerFlag),m_strQuestionText(lpszQuestionText),
			m_strQuestionCaption(lpszQuestionCaption)
	{
		m_strAction.LoadString(uiActionStrId);
		m_bAskQuestion = bAskQuestion;
		if (m_bAskQuestion) {
			ASSERT(m_strQuestionText);
			ASSERT(m_strQuestionCaption);
			m_iMaxValue = iMax;
			ASSERT(m_iMaxValue!=-1);
			m_iMinValue = iMin;
			ASSERT(m_iMinValue!=-1);
		}
	}

	virtual ~CaReplicationRaiseDbevent(){}
	virtual void Serialize (CArchive& ar);
	//
	// Operations
	// ----------
	BOOL AskQuestion(){return m_bAskQuestion;}
	CString GetQuestionText() {return m_strQuestionText;}
	CString GetQuestionCaption() {return m_strQuestionCaption;}
	CString GetServerFlag() {return m_strServerFlag;}
	CString GetEvent() {return m_strEvent;}
	CString GetAction() {return m_strAction;}
	int AskQuestion4FilledServerFlag(CString *SvrFlag);

protected:
	CString m_strAction;        // Main Item (col 0)
	CString m_strEvent;         // Value     (col 1)
	CString m_strServerFlag;    //           (col 2)
	BOOL    m_bAskQuestion;     // TRUE if the question is required for this event
	CString m_strQuestionText;
	CString m_strQuestionCaption;
	int     m_iMaxValue;
	int     m_iMinValue;
	int     m_nServerNumber;
};


class CaReplicRaiseDbeventList: public CTypedPtrList < CObList, CaReplicationRaiseDbevent* >
{
public:
	CaReplicRaiseDbeventList(){}
	virtual ~CaReplicRaiseDbeventList();
	void DefineAllDbeventWithDefaultValues(int iHandle, CString csDataBaseName, int nSvrNumber);
};

#endif // REPRAISEEVENT_HEADER
