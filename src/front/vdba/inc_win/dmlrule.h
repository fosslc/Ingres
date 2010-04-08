/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlrule.h: interface for the CaRule class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Rule object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLRULE_HEADER)
#define DMLRULE_HEADER
#include "dmlbase.h"

//
// Object: RULE
// ************************************************************************************************
class CaRule: public CaDBObject
{
	DECLARE_SERIAL (CaRule)
public:
	CaRule(): CaDBObject(){SetObjectID(OBT_RULE);}
	CaRule(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		: CaDBObject(lpszName, lpszOwner, bSystem){SetObjectID(OBT_RULE);}
	CaRule (const CaRule& c);
	CaRule operator = (const CaRule& c);
	
	virtual ~CaRule(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaRule* pObj, MatchObjectFlag nFlag = MATCHED_NAME);


protected:
	void Copy (const CaRule& c);

};

//
// Object: RULE Detail
// ************************************************************************************************
class CaRuleDetail: public CaRule
{
	DECLARE_SERIAL (CaRuleDetail)
public:
	CaRuleDetail():CaRule(){}
	CaRuleDetail(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		: CaRule(lpszName, lpszOwner, bSystem){m_strDetailText = _T("");}

	CaRuleDetail (const CaRuleDetail& c);
	CaRuleDetail operator = (const CaRuleDetail& c);
	
	virtual ~CaRuleDetail(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaRuleDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	void SetDetailText(LPCTSTR lpszStr){m_strDetailText = lpszStr;}
	CString GetDetailText(){return m_strDetailText;}
	void SetBaseTable (LPCTSTR lpszTable, LPCTSTR lpszTableOwner)
	{
		m_strBaseTable = lpszTable;
		m_strBaseTableOwner = lpszTableOwner;
	}
	CString GetBaseTable (){return m_strBaseTable;}
	CString GetBaseTableOwner (){return m_strBaseTableOwner;}

protected:
	void Copy (const CaRuleDetail& c);

	CString m_strDetailText;
	CString m_strBaseTable;
	CString m_strBaseTableOwner;
};


#endif // DMLRULE_HEADER