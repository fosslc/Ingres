/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlselec.h Header File
**    Project  : CA-OpenIngres 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : SQL file, use cursor to retrieve data from the table or view.
**
** History:
** xx-Jan-1998 (uk$so01)
**    Created.
** 03-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 28-May-2001 (schph01)
**    BUG 104636 Add in CaCursor a new variable member m_pSqlFloatSetting.
** 29-May-2001 (uk$so01)
**    SIR #104736, Integrate Ingres Import Assistant.
** 28-Jun-2001 (noifr01)
**    (SIR 103694) added new member variable used for the support of UNICODE
**    constants in the where clause of a select statement
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#if !defined (SQLSELECT_HEADER)
#define SQLSELECT_HEADER
#include "dmlbase.h"
#include "constdef.h"

class CeSQLException
{
public:
	CeSQLException():m_strReason(_T("")), m_nErrorCode(0){}
	CeSQLException(LPCTSTR lpszReason, int nError = 0)
		:m_strReason(lpszReason), m_nErrorCode(nError){}
	~CeSQLException(){}

	int     m_nErrorCode;
	CString m_strReason;
};


class CaObjectChildofTable: public CaDBObject
{
public:
	CaObjectChildofTable(LPCTSTR lpszName, LPCTSTR lpszOwner, LPCTSTR lpszBase, LPCTSTR lpszBaseOwner)
		:CaDBObject(lpszName, lpszOwner), m_strBase(lpszBase), m_strBaseOwner(lpszBaseOwner) 
	{
		m_strBase.TrimRight();
		m_strBaseOwner.TrimRight();
		m_strItem.TrimRight();
		m_strItemOwner.TrimRight();
	}
	virtual ~CaObjectChildofTable(){}
	CString GetName(){return m_strItem;}
	CString GetOwner(){return m_strItemOwner;}
	CString GetBaseTable(){return m_strBase;}
	CString GetBaseTableOwner(){return m_strBaseOwner;}

protected:
	CString m_strBase;
	CString m_strBaseOwner;

};
BOOL INGRESII_llQueryIndex (CTypedPtrList< CObList, CaObjectChildofTable* >& listObject);
int INGRESII_llHasCompositeHistogram (int nOT, LPCTSTR lpszObject, LPCTSTR lpszObjectOwner);
void ParseJoinTable (CString& strLine, CStringList& listSource);

#endif
