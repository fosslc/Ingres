/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlinteg.h: interface for the CaIntegrity class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Integrity object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLINTEGRITY_HEADER)
#define DMLINTEGRITY_HEADER
#include "dmlbase.h"

//
// Object: Integrity
// ************************************************************************************************
class CaIntegrity: public CaDBObject
{
	DECLARE_SERIAL (CaIntegrity)
public:
	CaIntegrity();
	CaIntegrity(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE);
	virtual ~CaIntegrity(){}
	virtual void Serialize (CArchive& ar);
	CaIntegrity(const CaIntegrity& c);
	CaIntegrity operator = (const CaIntegrity& c);

	BOOL Matched (CaIntegrity* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void SetDetailText(LPCTSTR lpszStr){m_strDetailText = lpszStr;}
	CString GetDetailText(){return m_strDetailText;}
	void SetNumber(int iNum){m_nNumber = iNum;}
	int  GetNumber(){return m_nNumber;}
protected:
	void Copy(const CaIntegrity& c);

	int m_nNumber;
	CString m_strDetailText;

};


//
// Object: IntegrityDetail
// ************************************************************************************************
class CaIntegrityDetail: public CaIntegrity
{
	DECLARE_SERIAL (CaIntegrityDetail)
public:
	CaIntegrityDetail():CaIntegrity(){}
	CaIntegrityDetail(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		:CaIntegrity(lpszName, lpszOwner, bSystem){}
	virtual ~CaIntegrityDetail(){}
	virtual void Serialize (CArchive& ar);
	CaIntegrityDetail(const CaIntegrityDetail& c);
	CaIntegrityDetail operator = (const CaIntegrityDetail& c);

	BOOL Matched (CaIntegrityDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void SetBaseTable (LPCTSTR lpszTable, LPCTSTR lpszTableOwner)
	{
		m_strBaseTable = lpszTable;
		m_strBaseTableOwner = lpszTableOwner;
	}
	CString GetBaseTable (){return m_strBaseTable;}
	CString GetBaseTableOwner (){return m_strBaseTableOwner;}

protected:
	void Copy(const CaIntegrityDetail& c);
	CString m_strBaseTable;
	CString m_strBaseTableOwner;

};



#endif // DMLINTEGRITY_HEADER