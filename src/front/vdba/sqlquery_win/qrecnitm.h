/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qrecnitm.h, Header file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : OLE Support for (CRichEditDoc)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(QRECNITM_HEADER)
#define QRECNITM_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CdSqlQueryRichEditDoc;
class CvSqlQueryRichEditView;

class CuRichCtrlCntrItem : public CRichEditCntrItem
{
	DECLARE_SERIAL(CuRichCtrlCntrItem)

public:
	CuRichCtrlCntrItem(REOBJECT* preo = NULL, CdSqlQueryRichEditDoc* pContainer = NULL);
	//  Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
	//  IMPLEMENT_SERIALIZE requires the class have a constructor with
	//  zero arguments.  Normally, OLE items are constructed with a
	//  non-NULL document pointer.

public:
	CdSqlQueryRichEditDoc* GetDocument()
		{ return (CdSqlQueryRichEditDoc*)CRichEditCntrItem::GetDocument(); }
	CvSqlQueryRichEditView* GetActiveView()
		{ return (CvSqlQueryRichEditView*)CRichEditCntrItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuRichCtrlCntrItem)
	public:
	protected:
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	~CuRichCtrlCntrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

#endif