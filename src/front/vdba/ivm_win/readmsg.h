/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : readmsg.h, header file
**    Project  : Ingres II/IVM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Read and parse the message description text from file
**               $(II_SYSTEM)\IngresII\ingres\files\english\messages\messages.txt.
**               The file is maintained opened. Each message read is stored in a couple
**               (Text=MessageID, pos = file position).
**               The set of couples is stored in a sorted array (quick sort). 
**
** History:
**
** 12-Feb-2003 (uk$so01)
**    Created
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/


#if !defined (READ_MESSAGE_DESCRIPTION_HEADER)
#define READ_MESSAGE_DESCRIPTION_HEADER
#include "evtcats.h"

typedef struct tagMSGxFILEPOSENTRY
{
	MSGCLASSANDID msg;
	long lFilePosition;
} MSGxFILEPOSENTRY;

class CaMessageDescription
{
public:
	CaMessageDescription()
	{
		m_strMessage = _T("N/A");
		m_strParameter = _T("N/A");
		m_strDescription = _T("N/A");
		m_strStatus = _T("N/A");
		m_strRecommendation = _T("N/A");
	}
	~CaMessageDescription(){}

	void SetMessage(LPCTSTR lpszText){m_strMessage = lpszText;}
	void SetParemeter(LPCTSTR lpszText){m_strParameter = lpszText;}
	void SetDescription(LPCTSTR lpszText){m_strDescription = lpszText;}
	void SetStatus(LPCTSTR lpszText){m_strStatus = lpszText;}
	void SetRecommendation(LPCTSTR lpszText){m_strRecommendation = lpszText;}

	CString GetMessage(){return m_strMessage;}
	CString GetParemeter(){return m_strParameter;}
	CString GetDescription(){return m_strDescription;}
	CString GetStatus(){return m_strStatus;}
	CString GetRecommendation(){return m_strRecommendation;}

protected:
	CString m_strMessage;
	CString m_strParameter;
	CString m_strDescription;
	CString m_strStatus;
	CString m_strRecommendation;
};

class CaMessageDescriptionManager: public CObject
{
public:
	CaMessageDescriptionManager(LPCTSTR lpszFile);
	~CaMessageDescriptionManager();
	void Initialize();
	BOOL Lookup (MSGCLASSANDID* pMessage, CaMessageDescription& msgDescription);

private:
	CString m_strFileName;
	FILE* m_pFile;
	int m_nMsgIndex;
	int m_nArrayFilePosSize;
	MSGxFILEPOSENTRY* m_pArrayFilePos;
};


#endif // READ_MESSAGE_DESCRIPTION_HEADER
