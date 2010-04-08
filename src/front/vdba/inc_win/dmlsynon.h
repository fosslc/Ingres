/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlsynon.h: interface for the CaSynonym class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synonym object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLSYNONYM_HEADER)
#define DMLSYNONYM_HEADER
#include "dmlbase.h"

//
// Object: CaSynonym
// ************************************************************************************************
class CaSynonym: public CaDBObject
{
	DECLARE_SERIAL (CaSynonym)
public:
	CaSynonym(): CaDBObject(){SetObjectID(OBT_SYNONYM);}
	CaSynonym(LPCTSTR lpszName, LPCTSTR lpszOwner): CaDBObject(lpszName, lpszOwner){SetObjectID(OBT_SYNONYM);}
	CaSynonym (const CaSynonym& c);
	CaSynonym operator = (const CaSynonym& c);

	BOOL Matched (CaSynonym* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaSynonym(){};
	virtual void Serialize (CArchive& ar);

protected:
	void Copy (const CaSynonym& c);
};


//
// Object: CaSynonymDetail
// ************************************************************************************************
class CaSynonymDetail: public CaSynonym
{
	DECLARE_SERIAL (CaSynonymDetail)
public:
	CaSynonymDetail():CaSynonym(){Init();}
	CaSynonymDetail(LPCTSTR lpszName, LPCTSTR lpszOwner): CaSynonym(lpszName, lpszOwner){Init();}
	CaSynonymDetail (const CaSynonymDetail& c);
	CaSynonymDetail operator = (const CaSynonymDetail& c);

	BOOL Matched (CaSynonymDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	virtual ~CaSynonymDetail(){};
	virtual void Serialize (CArchive& ar);
	TCHAR GetOnType(){return m_cOnType;}
	CString GetOnObject(){return m_strOnObject;}
	CString GetOnObjectOwner(){return m_strOnObjectOwner;}

	void SetOnType(TCHAR cType = _T('T')){m_cOnType=cType;} 
	void SetOnObject(LPCTSTR lpszText){m_strOnObject = lpszText;}
	void SetOnObjectOwner(LPCTSTR lpszText){m_strOnObjectOwner = lpszText;}

protected:
	void Copy (const CaSynonymDetail& c);
	void Init();

	TCHAR m_cOnType; // 'T'=Table, 'V'=View, 'I'=Index
	CString m_strOnObject;
	CString m_strOnObjectOwner;

};



#endif // DMLSYNONYM_HEADER