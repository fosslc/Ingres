/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmldbase.h: interface for the CaDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object
**
** History:
**
** 05-Feb-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
*/

#if !defined (DMLDBASE_HEADER)
#define DMLDBASE_HEADER
#include "dmlbase.h"

//
// Object: Database 
// ****************
class CaDatabase: public CaDBObject
{
	DECLARE_SERIAL (CaDatabase)
public:
	CaDatabase():CaDBObject() {Initialize();}
	CaDatabase(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaDBObject(lpszItem, lpszOwner, bSystem) 
	{
		Initialize();
	}
	CaDatabase(const CaDatabase& c);
	CaDatabase operator = (const CaDatabase& c);
	void Initialize();
	BOOL Matched (CaDatabase* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaDatabase();
	virtual void Serialize (CArchive& ar);

	void SetReadOnly(BOOL bSet);
	void SetPrivate (BOOL bSet);
	void SetDBService (long nService){m_nService = nService;}
	void SetStar    (int  nStar);
	void SetFlag(UINT lSet){m_nFlags = lSet;}
	BOOL IsReadOnly();
	BOOL IsPrivate ();
	int  GetStar   ();
	long GetService (){return m_nService;}
	UINT GetFlag(){return m_nFlags;}

protected:
	void Copy(const CaDatabase& c);
	UINT m_nFlags;
	long m_nService;
};

//
// Object: DatabaseDetail 
// **********************
class CaDatabaseDetail: public CaDatabase
{
	DECLARE_SERIAL (CaDatabaseDetail)
public:
	CaDatabaseDetail():CaDatabase(){Initialize();}
	CaDatabaseDetail(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaDatabase(lpszItem, lpszOwner, bSystem )
	{
		Initialize();
	}
	CaDatabaseDetail(const CaDatabaseDetail& c);
	CaDatabaseDetail operator = (const CaDatabaseDetail& c);

	void Initialize();
	BOOL Matched (CaDatabaseDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	virtual ~CaDatabaseDetail(){}
	virtual void Serialize (CArchive& ar);

	void SetLocationDatabase(LPCTSTR lpszLocation) {m_strLocDatabase = lpszLocation;}
	void SetLocationJournal(LPCTSTR lpszLocation) {m_strLocJournal = lpszLocation;}
	void SetLocationWork(LPCTSTR lpszLocation){m_strLocWork = lpszLocation;}
	void SetLocationCheckPoint(LPCTSTR lpszLocation){m_strLocCheckPoint = lpszLocation;}
	void SetLocationDump(LPCTSTR lpszLocation){m_strLocDump = lpszLocation;}
	void SetCompatLevel(LPCTSTR lpszText){m_strCompatLevel = lpszText;}
	void SetSecurityLabel(LPCTSTR lpszText){m_strSecurityLabel = lpszText;}
	void SetUnicodeEnable(BOOL bSet){m_bUnicodeEnable = bSet;}

	CString GetLocationDatabase() {return m_strLocDatabase;}
	CString GetLocationJournal() {return m_strLocJournal;}
	CString GetLocationWork(){return m_strLocWork;}
	CString GetLocationCheckPoint(){return m_strLocCheckPoint;}
	CString GetLocationDump(){return m_strLocDump;}
	CString GetCompatLevel(){return m_strCompatLevel;}
	CString GetSecurityLabel(){return m_strSecurityLabel;}
	BOOL GetUnicodeEnable(){return m_bUnicodeEnable;}

protected:
	void Copy(const CaDatabaseDetail& c);
	//
	// Primary locations:
	CString m_strLocDatabase;
	CString m_strLocJournal;
	CString m_strLocWork;
	CString m_strLocCheckPoint;
	CString m_strLocDump;
	CString m_strCompatLevel;
	CString m_strSecurityLabel;
	BOOL m_bUnicodeEnable;

	//
	// Extended locations:

};


#endif // DMLDBASE_HEADER
