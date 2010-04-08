/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlview.h: interface for the CaView class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : View object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (DMLVIEW_HEADER)
#define DMLVIEW_HEADER
#include "dmlbase.h"
#include "dmlcolum.h"


//
// Object: CaView
// ************************************************************************************************
class CaView: public CaDBObject
{
	DECLARE_SERIAL (CaView)
public:
	CaView(): CaDBObject(){Initialize();}
	CaView(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE): CaDBObject(lpszName, lpszOwner, bSystem)
	{
		Initialize();
	}
	CaView (const CaView& c);
	CaView operator = (const CaView& c);

	void Initialize();
	BOOL Matched (CaView* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	virtual ~CaView(){};
	virtual void Serialize (CArchive& ar);

	int  GetFlag(){return m_nFlag;}
	void SetFlag(int nVal){ m_nFlag = nVal;}

protected:
	void Copy (const CaView& c);
	int  m_nFlag;           // OBJTYPE_UNKNOWN, OBJTYPE_STARNATIVE, OBJTYPE_STARLINK

};


//
// Object: CaViewDetail
// ************************************************************************************************
class CaViewDetail: public CaView
{
	DECLARE_SERIAL (CaViewDetail)
public:
	CaViewDetail():CaView(){Initialize();}
	CaViewDetail(LPCTSTR lpszName, LPCTSTR lpszOwner, BOOL bSystem = FALSE):CaView(lpszName, lpszOwner, bSystem)
	{
		Initialize();
	}
	CaViewDetail (const CaViewDetail& c);
	CaViewDetail operator = (const CaViewDetail& c);

	void Initialize(){m_strDetailText = _T("");}
	BOOL Matched (CaViewDetail* pObj, MatchObjectFlag nFlag = MATCHED_NAME);

	virtual ~CaViewDetail()
	{
		while (!m_listColumn.IsEmpty())
			delete m_listColumn.RemoveHead();
	}
	virtual void Serialize (CArchive& ar);
	void SetDetailText(LPCTSTR lpszText){m_strDetailText =  lpszText;}
	CString GetDetailText() const {return m_strDetailText;}
	CTypedPtrList < CObList, CaColumn* >& GetListColumns(){return m_listColumn;}

	void SetCheckOption(LPCTSTR lpszText){m_csCheckOption = lpszText;}
	void SetLanguageType(LPCTSTR lpszText){m_csLanguage = lpszText;}

	CString GetCheckOption(){return m_csCheckOption;}
	CString GetLanguageType(){return m_csLanguage;}


protected:
	void Copy (const CaViewDetail& c);
	CString m_strDetailText;
	CTypedPtrList < CObList, CaColumn* > m_listColumn;
	CString m_csLanguage;
	CString m_csCheckOption;
};


#endif // DMLVIEW_HEADER