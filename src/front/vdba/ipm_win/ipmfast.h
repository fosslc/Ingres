/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmfast.h: header file 
**    Project  : INGRESII/Monitoring.
**    Author   : Emannuel Blattes
**    Purpose  : implementation file for fast access to an item.
**               thanks to double-click in right pane changing selection in the ipm tree
**
** History:
**
** xx-Feb-1998 (Emannuel Blattes)
**    Created
*/


#ifndef IPMFAST_INCLUDED
#define IPMFAST_INCLUDED
#include "ipmframe.h"

class CuIpmTreeFastItem: public CObject
{
public:
	CuIpmTreeFastItem(BOOL bStatic, int type, void* pFastStruct = NULL, BOOL bCheckName = FALSE, LPCTSTR lpsCheckName = NULL);
	virtual ~CuIpmTreeFastItem();

protected:
	// CuIpmTreeFastItem();    // serialization
	// DECLARE_SERIAL (CuIpmTreeFastItem)

public:
	BOOL    GetBStatic() { return m_bStatic; }
	int     GetType()    { return m_type; }
	void*   GetPStruct() { return m_pStruct; }
	BOOL    IsCheckNameNeeded()   { return m_bCheckName; }
	LPCTSTR GetCheckName() { return m_csCheckName; }

protected:
	void* m_pStruct;
private:
	BOOL  m_bStatic;
	int   m_type;
	BOOL  m_bCheckName;
	CString m_csCheckName;
};

BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuIpmTreeFastItem*>& rIpmTreeFastItemList, BOOL bAutomated = FALSE);
BOOL IpmCurSelHasStaticChildren(CWnd* pPropWnd);

class CuListCtrl;
void CuListCtrlCleanVoidItemData(CuListCtrl* pControl);

#endif  // IPMFAST_INCLUDED
