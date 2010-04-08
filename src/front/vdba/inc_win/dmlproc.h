/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlproc.h: interface for the CaProcedure class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Procedure object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (DMLPROC_HEADER)
#define DMLPROC_HEADER
#include "dmlbase.h"


//
// Object: Procedure 
// ************************************************************************************************
class CaProcedure: public CaDBObject
{
	DECLARE_SERIAL (CaProcedure)
public:
	CaProcedure():CaDBObject() {SetObjectID(OBT_PROCEDURE);}
	CaProcedure(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		:CaDBObject(lpszItem, lpszOwner, bSystem) {SetObjectID(OBT_PROCEDURE);};
	CaProcedure (const CaProcedure& c);
	CaProcedure operator = (const CaProcedure& c);

	virtual ~CaProcedure(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaProcedure* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	int  GetFlag(){return m_nFlag;}
	void SetFlag(int nVal){ m_nFlag = nVal;}

protected:
	void Copy(const CaProcedure& c);

	int  m_nFlag;           // OBJTYPE_UNKNOWN, OBJTYPE_STARNATIVE, OBJTYPE_STARLINK, OBJTYPE_NOTSTAR
};


//
// Object: Procedure Detail
// ************************************************************************************************
class CaProcedureDetail: public CaProcedure
{
	DECLARE_SERIAL (CaProcedureDetail)
public:
	CaProcedureDetail():CaProcedure() {}
	CaProcedureDetail(LPCTSTR lpszItem, LPCTSTR lpszOwner, BOOL bSystem = FALSE)
		:CaProcedure(lpszItem, lpszOwner, bSystem) {};
	CaProcedureDetail (const CaProcedureDetail& c);
	CaProcedureDetail operator = (const CaProcedureDetail& c);

	virtual ~CaProcedureDetail(){}
	virtual void Serialize (CArchive& ar);
	BOOL Matched (CaProcedureDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	void SetDetailText(LPCTSTR lpszText){m_strDetailText = lpszText;}
	CString GetDetailText(){return m_strDetailText;}

protected:
	void Copy(const CaProcedureDetail& c);

	CString m_strDetailText;
};



#endif // DMLPROC_HEADER
