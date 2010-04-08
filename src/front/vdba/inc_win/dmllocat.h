/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmllocat.h: interface for the CaLocation class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Location object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLLOCATION_HEADER)
#define DMLLOCATION_HEADER
#include "dmlbase.h"

//
// Object: CaLocation 
// ************************************************************************************************
class CaLocation: public CaDBObject
{
	DECLARE_SERIAL (CaLocation)
public:
	CaLocation(): CaDBObject(){SetObjectID(OBT_LOCATION);}
	CaLocation(LPCTSTR lpszName): CaDBObject(lpszName, _T("")){SetObjectID(OBT_LOCATION);}
	CaLocation(const CaLocation& c);
	CaLocation operator = (const CaLocation& c);

	BOOL Matched (CaLocation* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaLocation(){};
	virtual void Serialize (CArchive& ar);

protected:
	void Copy(const CaLocation& c);

}; 

//
// Object: CaLocationDetail 
// ************************************************************************************************
class CaLocationDetail: public CaLocation
{
	DECLARE_SERIAL (CaLocationDetail)
public:
	CaLocationDetail(): CaLocation(){Init();}
	CaLocationDetail(LPCTSTR lpszName): CaLocation(lpszName){Init();}
	CaLocationDetail(const CaLocationDetail& c);
	CaLocationDetail operator = (const CaLocationDetail& c);

	BOOL Matched (CaLocationDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaLocationDetail(){};
	virtual void Serialize (CArchive& ar);

	TCHAR GetDataUsage(){return m_tchDataUsage;}
	TCHAR GetJournalUsage(){return m_tchJournalUsage;}
	TCHAR GetCheckPointUsage(){return m_tchCheckPointUsage;}
	TCHAR GetWorkUsage(){return m_tchWorkUsage;}
	TCHAR GetDumpUsage(){return m_tchDumpUsage;}
	CString GetLocationArea(){return m_strLocationArea;}

	void SetDataUsage(TCHAR tchC){m_tchDataUsage = tchC;}
	void SetJournalUsage(TCHAR tchC){m_tchJournalUsage = tchC;}
	void SetCheckPointUsage(TCHAR tchC){m_tchCheckPointUsage = tchC;}
	void SetWorkUsage(TCHAR tchC){m_tchWorkUsage = tchC;}
	void SetDumpUsage(TCHAR tchC){m_tchDumpUsage = tchC;}
	void SetLocationArea(LPCTSTR lpszText){m_strLocationArea = lpszText;}

protected:
	void Copy(const CaLocationDetail& c);
	void Init();

	TCHAR m_tchDataUsage;
	TCHAR m_tchJournalUsage;
	TCHAR m_tchCheckPointUsage;
	TCHAR m_tchWorkUsage;
	TCHAR m_tchDumpUsage;
	CString m_strLocationArea;
};

#endif // DMLLOCATION_HEADER