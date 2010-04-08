/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dmlconst.h , header File 
**    Project  : IJA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Constraint Object (Primary Key, Unique Key, ...) of Table
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
**
**/

#if !defined (DMLCONSTRAINT_HEADER)
#define DMLCONSTRAINT_HEADER
#include "dmlbase.h"

class CaConstraint: public CaDBObject
{
	DECLARE_SERIAL (CaConstraint)
public:
	CaConstraint(): CaDBObject(), m_strTable(_T("")), m_strText(_T("")), m_strType(_T("")){}
	CaConstraint(LPCTSTR lpszName, LPCTSTR lpszOwner, LPCTSTR lpszTable, LPCTSTR lpszText, LPCTSTR lpszType)
		: CaDBObject(lpszName, lpszOwner)
	{
		m_strText  = lpszText;
		m_strTable = lpszTable;
		m_strType  = lpszType;
	}

	virtual ~CaConstraint(){};
	virtual void Serialize (CArchive& ar);
	CString GetName() {return m_strItem;}
	CString GetTable() {return m_strTable;}
	CString GetTableOwner() {return GetOwner();}
	CString GetText () {return m_strText;}
	CString GetType () {return m_strType;}

	CString m_strTable;
	CString m_strText;
	CString m_strType;

};


#endif