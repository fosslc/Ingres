/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : dmlindex.h Header File                                                //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Index class for Data manipulation                  .                  //
****************************************************************************************/
#if !defined (DMLINDEX_HEADER)
#define DMLINDEX_HEADER
#include "dmlbase.h"


class CaComponentValue: public CObject
{
public:
	CaComponentValue(): m_strComponentName(_T("")), m_strValue(_T("")){}
	CaComponentValue(LPCTSTR lpszComponent, LPCTSTR lpszValue)
	{
		m_strComponentName = lpszComponent? lpszComponent : _T("");
		m_strValue         = lpszValue? lpszValue: _T("");
	}
	virtual ~CaComponentValue(){}

	CString m_strComponentName;
	CString m_strValue;
};


class CaComponentValueConstraint: public CaComponentValue
{
public:
	enum ConstraintType {CONSTRAINT_EDIT, CONSTRAINT_EDITNUMBER, CONSTRAINT_SPIN, CONSTRAINT_COMBO, CONSTRAINT_CHECK};
	CaComponentValueConstraint(): CaComponentValue(), m_nConstraint(CONSTRAINT_EDIT), m_bSpinRange(FALSE){}
	CaComponentValueConstraint(LPCTSTR lpszComponent, ConstraintType nConstraint, LPCTSTR lpszValue)
		:CaComponentValue(lpszComponent, lpszValue), m_nConstraint(nConstraint)
	{
		m_bSpinRange = FALSE;
	}
	virtual ~CaComponentValueConstraint(){}

	void SetRange (int nLower, int nUpper)
	{
		ASSERT (m_nConstraint == CONSTRAINT_SPIN || m_nConstraint == CONSTRAINT_EDITNUMBER);
		if (m_nConstraint == CONSTRAINT_SPIN || m_nConstraint == CONSTRAINT_EDITNUMBER)
		{
			m_bSpinRange = (m_nConstraint == CONSTRAINT_SPIN)? TRUE: FALSE;
			m_nLower = nLower;
			m_nUpper = nUpper;
		}
	}

	int  m_nLower;
	int  m_nUpper;
	int  m_nConstraint;
private:
	BOOL m_bSpinRange; // FALSE: ignore m_nLower and m_nUpper
};


class CaIndexStructure: public CObject
{
public:
	CaIndexStructure(): m_nStructType(0){}
	CaIndexStructure(int nStructType, UINT idsStructName);
	CaIndexStructure(int nStructType, LPCTSTR strStructure);

	virtual ~CaIndexStructure();
	void AddComponent (LPCTSTR lpszComponent, CaComponentValueConstraint::ConstraintType cType, LPCTSTR lpszValue = NULL);
	void AddComponent (LPCTSTR lpszComponent, int nLower, int nUpper, LPCTSTR lpszValue = NULL);
	void AddComponent (LPCTSTR lpszComponent, int nLower, int nUpper, CaComponentValueConstraint::ConstraintType cType, LPCTSTR lpszValue = NULL);
	
	int m_nStructType;
	CString m_strStructure;
	CTypedPtrList<CObList, CaComponentValueConstraint*> m_listComponent;
};


class CaColumnIndex: public CaColumn
{
	DECLARE_SERIAL (CaColumnIndex)
public:
	CaColumnIndex ():CaColumn(_T("")), m_bKeyed(FALSE), m_nSort(-1){}
	CaColumnIndex (LPCTSTR lpszColumn, BOOL bKeyed, int nSort = -1)
		:CaColumn(lpszColumn), m_bKeyed(bKeyed), m_nSort(nSort){}

	virtual ~CaColumnIndex(){}
	virtual void Serialize (CArchive& ar);

	BOOL m_bKeyed; // TRUE: the column is part of keys
	int  m_nSort;  // -1: not used, 0: asc, 1: desc

};



//
// Object: Index 
// -------------
class CaIndex: public CaDBObject
{
	DECLARE_SERIAL (CaIndex)
public:
	enum {INDEX_UNIQUE_SCOPE_ROW, INDEX_UNIQUE_SCOPE_STATEMENT};
	CaIndex();
	CaIndex(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE);
	virtual ~CaIndex();
	virtual void Serialize (CArchive& ar);
	BOOL IsValide(CString& strMsg);
	BOOL GenerateWithClause(CString& strWithClause, CString& strListKey);

	//
	// List of Columns of Index
	CTypedPtrList< CObList, CaColumnIndex* > m_listColumn;
	//
	// List of locations
	CStringList m_listLocation;

	//
	// Index name is  'm_strItem'
	// Index owner is 'm_strItemOwner'

	BOOL m_bPersistence;
	BOOL m_bUnique;         // TRUE if Unique index.
	int  m_nUniqueScope;    // Available only if m_bUnique = TRUE;

	BOOL m_bCompressData;
	BOOL m_bCompressKey;
	
	CString m_strStructure; // Structure name
	CString m_strPageSize;  // Index Page size

	CString m_strMinPage;
	CString m_strMaxPage;
	CString m_strLeaffill;
	CString m_strNonleaffill;
	CString m_strAllocation;
	CString m_strExtend;
	CString m_strFillfactor;
	CString m_strMaxX;
	CString m_strMaxY;
	CString m_strMinX;
	CString m_strMinY;

	//
	// Visual Design Only (zero based index of item in combo box):
	int m_nStructure; 
	int m_nPageSize;
};



#endif
