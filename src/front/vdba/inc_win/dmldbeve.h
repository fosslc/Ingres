/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmldbeve.h: interface for the CaDBEvent class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : DBEvent object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLDBEVENT_HEADER)
#define DMLDBEVENT_HEADER
#include "dmlbase.h"

//
// Object: CaDBEvent 
// ************************************************************************************************
class CaDBEvent: public CaDBObject
{
	DECLARE_SERIAL (CaDBEvent)
public:
	CaDBEvent(): CaDBObject(){SetObjectID(OBT_DBEVENT);}
	CaDBEvent(LPCTSTR lpszName, LPCTSTR lpszOwner): CaDBObject(lpszName, lpszOwner){SetObjectID(OBT_DBEVENT);}
	CaDBEvent(const CaDBEvent& c);
	CaDBEvent operator =(const CaDBEvent& c);

	virtual ~CaDBEvent(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaDBEvent* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

protected:
	void Copy (const CaDBEvent& c);
};


//
// Object: CaDBEventDetail 
// ************************************************************************************************
class CaDBEventDetail: public CaDBEvent
{
	DECLARE_SERIAL (CaDBEventDetail)
public:
	CaDBEventDetail(): CaDBEvent(){}
	CaDBEventDetail(LPCTSTR lpszName, LPCTSTR lpszOwner): CaDBEvent(lpszName, lpszOwner){}
	CaDBEventDetail(const CaDBEventDetail& c);
	CaDBEventDetail operator =(const CaDBEventDetail& c);

	virtual ~CaDBEventDetail(){};
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaDBEventDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	void SetDetailText(LPCTSTR lpszText){m_strDetailText = lpszText;}
	CString GetDetailText(){return m_strDetailText;}


protected:
	void Copy (const CaDBEventDetail& c);
	CString m_strDetailText;
};


#endif // DMLDBEVENT_HEADER